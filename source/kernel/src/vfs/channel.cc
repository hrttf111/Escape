/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <esc/ipc/ipcbuf.h>
#include <esc/proto/file.h>
#include <esc/proto/device.h>
#include <mem/cache.h>
#include <mem/useraccess.h>
#include <mem/virtmem.h>
#include <sys/messages.h>
#include <task/filedesc.h>
#include <task/proc.h>
#include <task/thread.h>
#include <vfs/channel.h>
#include <vfs/device.h>
#include <vfs/node.h>
#include <vfs/openfile.h>
#include <vfs/vfs.h>
#include <assert.h>
#include <common.h>
#include <errno.h>
#include <log.h>
#include <spinlock.h>
#include <string.h>
#include <video.h>

VFSChannel::VFSChannel(const fs::User &u,VFSNode *p,bool &success)
		/* permissions are basically irrelevant here since the userland can't open a channel directly. */
		/* but in order to allow devices to be created by non-root users, give permissions for everyone */
		/* otherwise, if root uses that device, the driver is unable to open this channel. */
		: VFSNode(u,generateId(),MODE_TYPE_CHANNEL | 0777,success), fd(-1),
		  handler(), closed(false), driver_gone(false),
		  shmem(NULL), shmemSize(0), sendList(), recvList() {
	if(!success)
		return;

	append(p);

	/* ensure that we don't do that in parallel with VFSDevice::bindto */
	VFSNode::acquireTree();
	handler = static_cast<VFSDevice*>(p)->getCreator();
	VFSNode::releaseTree();
}

void VFSChannel::invalidate() {
	/* notify potentially waiting clients */
	Sched::wakeup(EV_RECEIVED_MSG,(evobj_t)this);

	// this is okay, because we have the treelock acquired here
	static_cast<VFSDevice*>(getParent())->chanRemoved(this);

	// we only get here if the node has no references left. so it's safe to access the lists.
	recvList.deleteAll();
	sendList.deleteAll();
}

int VFSChannel::isSupported(int op) const {
	if(!isAlive())
		return -EDESTROYED;
	/* if the driver doesn't implement read, its an error */
	if(!static_cast<VFSDevice*>(parent)->supports(op))
		return -ENOTSUP;
	return 0;
}

pid_t VFSChannel::getDeviceProc() const {
	return static_cast<const VFSDevice*>(getParent())->getOwner();
}

ssize_t VFSChannel::open(const fs::User &u,const char *path,ssize_t *sympos,ino_t root,uint flags,
		int msgid,mode_t mode) {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));
	ssize_t res;
	msgid_t mid;

	/* give the driver a file-descriptor for this new client; note that we have to do that
	 * immediatly because in close() we assume that the device has already one reference to it */
	res = VFS::openFileDesc(getDeviceProc(),0,VFS_MSGS | VFS_DEVICE,this,getNo(),VFS_DEV_NO);
	if(res < 0)
		return res;
	fd = res;

	/* do we need to send an open to the driver? */
	res = isSupported(DEV_OPEN);
	if(res == -ENOTSUP)
		return 0;
	if(res < 0)
		goto error;

	/* send msg to driver */
	ib << esc::FileOpen::Request(flags,u,esc::CString(path),root,mode);
	if(ib.error()) {
		res = -EINVAL;
		goto error;
	}
	res = send(0,msgid,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		goto error;

	/* receive response */
	ib.reset();
	mid = res;
	res = receive(0,&mid,ib.buffer(),ib.max());
	if(res < 0)
		goto error;

	{
		esc::FileOpen::Response r;
		ib >> r;
		if(r.err < 0) {
			res = r.err;
			goto error;
		}
		if(sympos)
			*sympos = r.res.sympos;
		return r.res.ino;
	}

error:
	VFS::closeFileDesc(getDeviceProc(),fd);
	return res;
}

void VFSChannel::close(OpenFile *file,int msgid) {
	ushort remRefs;

	if(!isAlive())
		remRefs = unref();
	else {
		/* if this is the driver, destroy it */
		if(file->isDevice()) {
			Sched::wakeup(EV_RECEIVED_MSG,(evobj_t)this);
			remRefs = destroy();
			driver_gone = true;
		}
		/* if there is only the default ref and the drivers left, do the real close */
		else if((remRefs = unref()) == 2) {
			send(0,msgid,NULL,0,NULL,0);
			closed = true;
		}
	}

	/* auto destroy if only the default ref is left. if the device was destroyed, the default ref
	 * is already gone. */
	if(driver_gone && remRefs == 1)
		unref();
}

off_t VFSChannel::seek(off_t position,off_t offset,uint whence) const {
	switch(whence) {
		case SEEK_SET:
			return offset;
		case SEEK_CUR:
			return position + offset;
		default:
		case SEEK_END:
			/* not supported for devices */
			return -ESPIPE;
	}
}

ssize_t VFSChannel::getSize() {
	ulong buffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(buffer,sizeof(buffer));

	if(isSupported(DEV_SIZE) < 0)
		return 0;

	/* send msg to device */
	ssize_t res = send(0,esc::FileSize::MSG,NULL,0,NULL,0);
	if(res < 0)
		return res;

	/* receive response */
	msgid_t mid = res;
	res = receive(0,&mid,ib.buffer(),ib.max());
	if(res < 0)
		return res;

	esc::FileSize::Response r;
	ib >> r;
	if(r.err < 0)
		return r.err;
	return r.res;
}

static bool useSharedMem(const void *shmem,size_t shmsize,const void *buffer,size_t bufsize) {
	return shmem && (uintptr_t)buffer >= (uintptr_t)shmem &&
		(uintptr_t)buffer + bufsize > (uintptr_t)buffer &&
		(uintptr_t)buffer + bufsize <= (uintptr_t)shmem + shmsize;
}

uint VFSChannel::getReceiveFlags() const {
	uint flags = 0;
	/* allow signals if either the cancel message or cancel signal is supported */
	if(isSupported(DEV_CANCEL | DEV_CANCELSIG) == 0)
		flags |= VFS_SIGNALS;
	/* but enforce blocking if the cancel message is not supported since we have to leave the
	 * channel in a consistent state */
	if(isSupported(DEV_CANCEL) < 0)
		flags |= VFS_BLOCK;
	return flags;
}

ssize_t VFSChannel::read(OpenFile *file,USER void *buffer,off_t offset,size_t count) {
	ulong ibuffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(ibuffer,sizeof(ibuffer));
	ssize_t res;

	if((res = isSupported(DEV_READ)) < 0)
		return res;

	/* send msg to driver */
	bool useshm = useSharedMem(shmem,shmemSize,buffer,count);
	ib << esc::FileRead::Request(offset,count,useshm ? ((uintptr_t)buffer - (uintptr_t)shmem) : -1);
	res = file->sendMsg(esc::FileRead::MSG,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		return res;

	msgid_t mid = res;
	uint flags = getReceiveFlags();
	while(1) {
		/* read response and ensure that we don't get killed until we've received both messages
		 * (otherwise the channel might get in an inconsistent state) */
		ib.reset();
		res = file->receiveMsg(&mid,ib.buffer(),ib.max(),flags);
		if(res < 0) {
			if(res == -EINTR || res == -EWOULDBLOCK) {
				int cancelRes = cancel(file,mid);
				if(cancelRes == esc::DevCancel::READY) {
					/* if the result is already there, get it, but don't allow signals anymore
					 * and force blocking */
					flags = VFS_BLOCK;
					continue;
				}
			}
			return res;
		}

		/* handle response */
		esc::FileRead::Response r;
		ib >> r;
		if(r.err < 0)
			return r.err;

		/* read data */
		if(!useshm && r.res > 0)
			r.res = file->receiveMsg(&mid,buffer,count,0);
		return r.res;
	}
	A_UNREACHED;
}

ssize_t VFSChannel::write(OpenFile *file,USER const void *buffer,off_t offset,size_t count) {
	ulong ibuffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(ibuffer,sizeof(ibuffer));
	ssize_t res;
	bool useshm = useSharedMem(shmem,shmemSize,buffer,count);

	if((res = isSupported(DEV_WRITE)) < 0)
		return res;
	if(!buffer)
		return -EINVAL;

	/* send msg and data to driver */
	ib << esc::FileWrite::Request(offset,count,useshm ? ((uintptr_t)buffer - (uintptr_t)shmem) : -1);
	res = file->sendMsg(esc::FileWrite::MSG,ib.buffer(),ib.pos(),useshm ? NULL : buffer,count);
	if(res < 0)
		return res;

	msgid_t mid = res;
	uint flags = getReceiveFlags();
	while(1) {
		/* read response */
		ib.reset();
		res = file->receiveMsg(&mid,ib.buffer(),ib.max(),flags);
		if(res < 0) {
			if(res == -EINTR || res == -EWOULDBLOCK) {
				int cancelRes = cancel(file,mid);
				if(cancelRes == esc::DevCancel::READY) {
					/* if the result is already there, get it, but don't allow signals anymore
					 * and force blocking */
					flags = VFS_BLOCK;
					continue;
				}
			}
			return res;
		}

		esc::FileWrite::Response r;
		ib >> r;
		if(r.err < 0)
			return r.err;
		return r.res;
	}
	A_UNREACHED;
}

int VFSChannel::cancel(OpenFile *file,msgid_t mid) {
	ulong ibuffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(ibuffer,sizeof(ibuffer));

	/* send SIGCANCEL to the handling thread of this channel (this might be dead; so use getRef) */
	bool sent = false;
	if(isSupported(DEV_CANCELSIG) == 0) {
		Thread *t = Thread::getRef(handler);
		if(t) {
			sent = Signals::addSignalFor(t,SIGCANCEL);
			Thread::relRef(t);
		}
	}

	int res;
	if((res = isSupported(DEV_CANCEL)) < 0)
		return sent ? 0 : res;

	ib << esc::DevCancel::Request(mid);
	res = file->sendMsg(esc::DevCancel::MSG,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		return res;

	ib.reset();
	mid = res;
	res = file->receiveMsg(&mid,ib.buffer(),ib.max(),VFS_BLOCK);
	if(res < 0)
		return res;

	/* handle response */
	esc::DevCancel::Response r;
	ib >> r;
	return r.err;
}

int VFSChannel::delegate(pid_t pid,OpenFile *chan,OpenFile *file,uint perm,int arg) {
	ulong ibuffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(ibuffer,sizeof(ibuffer));
	int nfd;
	msgid_t mid;

	int res;
	if((res = isSupported(DEV_DELEGATE)) < 0)
		return res;

	/* establish shared memory? */
	if(arg == DEL_ARG_SHFILE) {
		if(shmem)
			return -EEXIST;
		/* it needs to be a virtual file, otherwise filesystems could not support shared memory
		 * (easily), because they would risk a deadlock when doing a fstat() on the received fd. */
		/* there is not really a point in using non-virtual files for this anyway.. */
		if(file->getDev() != VFS_DEV_NO)
			return -EINVAL;
		uintptr_t addr = ShFiles::getFor(file,pid);
		if(!addr)
			return -ENXIO;
		struct stat info;
		if((res = file->fstat(&info)) < 0)
			return res;
		shmem = reinterpret_cast<void*>(addr);
		shmemSize = info.st_size;
	}

	/* first, give the driver a file-descriptor for the new channel */
	uint flags = (file->getFlags() & ~O_ACCMODE) | perm;
	res = VFS::openFileDesc(getDeviceProc(),file->getMntPerms(),flags,
		file->getNode(),file->getNodeNo(),file->getDev());
	if(res < 0)
		goto errorShm;
	nfd = res;

	/* send msg to driver */
	ib << esc::DevDelegate::Request(nfd,arg);
	res = chan->sendMsg(esc::DevDelegate::MSG,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		goto error;

	/* read response */
	ib.reset();
	mid = res;
	res = chan->receiveMsg(&mid,ib.buffer(),ib.max(),0);
	if(res < 0)
		goto error;

	/* handle response */
	{
		esc::DevDelegate::Response r;
		ib >> r;
		if(r.err < 0) {
			res = r.err;
			goto error;
		}
	}
	return 0;

error:
	VFS::closeFileDesc(getDeviceProc(),nfd);
errorShm:
	if(arg == DEL_ARG_SHFILE)
		shmem = NULL;
	return res;
}

int VFSChannel::obtain(pid_t pid,OpenFile *chan,int arg) {
	ulong ibuffer[IPC_DEF_SIZE / sizeof(ulong)];
	esc::IPCBuf ib(ibuffer,sizeof(ibuffer));

	int res;
	if((res = isSupported(DEV_OBTAIN)) < 0)
		return res;

	/* send msg to driver */
	ib << esc::DevObtain::Request(arg);
	res = chan->sendMsg(esc::DevObtain::MSG,ib.buffer(),ib.pos(),NULL,0);
	if(res < 0)
		return res;

	msgid_t mid = res;
	uint flags = getReceiveFlags();
	while(1) {
		/* read response */
		ib.reset();
		res = chan->receiveMsg(&mid,ib.buffer(),ib.max(),flags);
		if(res < 0) {
			if(res == -EINTR || res == -EWOULDBLOCK) {
				int cancelRes = cancel(chan,mid);
				if(cancelRes == esc::DevCancel::READY) {
					/* if the result is already there, get it, but don't allow signals anymore
					 * and force blocking */
					flags = VFS_BLOCK;
					continue;
				}
			}
			return res;
		}
		break;
	}

	esc::DevObtain::Response r;
	ib >> r;
	if(r.err < 0)
		return r.err;

	/* request file from driver */
	Proc *pp = Proc::getRef(getDeviceProc());
	if(!pp)
		return -ESRCH;
	OpenFile *ofile = FileDesc::request(pp,r.fd);
	if(ofile == NULL) {
		res = -EBADF;
		goto errorProc;
	}

	/* check permissions */
	r.perm &= O_ACCMODE & ofile->getFlags();
	if(r.perm == 0) {
		res = -EINVAL;
		goto errorFile;
	}

	/* open file for client */
	{
		/* TODO currently, we simply remove VFS_DEVICE here to allow drivers to delegate the fd from
		 * createchan to the client and keep a copy for them. the drivers file has VFS_DEVICE set,
		 * but with this, we remove it while it's passed to the client. */
		uint flags = (ofile->getFlags() & ~(O_ACCMODE | VFS_DEVICE)) | r.perm;
		res = VFS::openFileDesc(pid,ofile->getMntPerms(),flags,
			ofile->getNode(),ofile->getNodeNo(),ofile->getDev());
	}

errorFile:
	FileDesc::release(ofile);
errorProc:
	Proc::relRef(pp);
	return res;
}

int VFSChannel::send(ushort flags,msgid_t id,USER const void *data1,
						 size_t size1,USER const void *data2,size_t size2) {
	return static_cast<VFSDevice*>(parent)->send(this,flags,id,data1,size1,data2,size2);
}

ssize_t VFSChannel::receive(ushort flags,msgid_t *id,void *data,size_t size) {
	return static_cast<VFSDevice*>(parent)->receive(this,flags,id,data,size);
}

void VFSChannel::print(OStream &os) const {
	const esc::SList<Message> *lists[] = {&sendList,&recvList};
	os.writef("%-8s: snd=%zu rcv=%zu closed=%d handler=%d fd=%02d shm=%zuK\n",
		name,sendList.length(),recvList.length(),closed,handler,fd,shmem ? shmemSize / 1024 : 0);
	for(size_t i = 0; i < ARRAY_SIZE(lists); i++) {
		for(auto it = lists[i]->cbegin(); it != lists[i]->cend(); ++it) {
			os.writef("\t%s id=%u:%u len=%zu\n",i == 0 ? "->" : "<-",
				it->id >> 16,it->id & 0xFFFF,it->length);
		}
	}
}
