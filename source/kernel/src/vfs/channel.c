/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <sys/common.h>
#include <sys/mem/cache.h>
#include <sys/mem/vmm.h>
#include <sys/task/thread.h>
#include <sys/task/event.h>
#include <sys/task/proc.h>
#include <sys/vfs/vfs.h>
#include <sys/vfs/node.h>
#include <sys/vfs/request.h>
#include <sys/vfs/channel.h>
#include <sys/vfs/device.h>
#include <sys/vfs/devmsgs.h>
#include <sys/video.h>
#include <sys/klock.h>
#include <esc/messages.h>
#include <esc/sllist.h>
#include <string.h>
#include <errors.h>

#define ARGS_MSG_COUNT		256

typedef struct {
	bool used;
	bool closed;
	/* a list for sending messages to the device */
	sSLList *sendList;
	/* a list for reading messages from the device */
	sSLList *recvList;
} sChannel;

typedef struct {
	msgid_t id;
	size_t length;
} sMessage;

static void vfs_chan_destroy(sVFSNode *n);
static off_t vfs_chan_seek(pid_t pid,sVFSNode *node,off_t position,off_t offset,uint whence);
static void vfs_chan_close(pid_t pid,file_t file,sVFSNode *node);

sVFSNode *vfs_chan_create(pid_t pid,sVFSNode *parent) {
	sChannel *chan;
	sVFSNode *node;
	char *name = vfs_node_getId(pid);
	if(!name)
		return NULL;
	node = vfs_node_create(pid,name);
	if(node == NULL) {
		cache_free(name);
		return NULL;
	}

	node->mode = MODE_TYPE_CHANNEL | S_IRUSR | S_IWUSR;
	node->read = (fRead)vfs_devmsgs_read;
	node->write = (fWrite)vfs_devmsgs_write;
	node->seek = vfs_chan_seek;
	node->close = vfs_chan_close;
	node->destroy = vfs_chan_destroy;
	node->data = NULL;
	chan = (sChannel*)cache_alloc(sizeof(sChannel));
	if(!chan) {
		vfs_node_destroy(node);
		return NULL;
	}
	chan->recvList = NULL;
	chan->sendList = NULL;
	chan->used = false;
	chan->closed = false;
	node->data = chan;
	vfs_node_append(parent,node);
	return node;
}

static void vfs_chan_destroy(sVFSNode *n) {
	sChannel *chan = (sChannel*)n->data;
	if(chan) {
		/* we have to reset the last client for the device here */
		vfs_device_clientRemoved(n->parent,n);
		/* free send and receive list */
		if(chan->recvList)
			sll_destroy(chan->recvList,true);
		if(chan->sendList)
			sll_destroy(chan->sendList,true);
		cache_free(chan);
		n->data = NULL;
	}
}

static off_t vfs_chan_seek(A_UNUSED pid_t pid,A_UNUSED sVFSNode *node,off_t position,
		off_t offset,uint whence) {
	switch(whence) {
		case SEEK_SET:
			return offset;
		case SEEK_CUR:
			return position + offset;
		default:
		case SEEK_END:
			/* not supported for devices */
			return ERR_INVALID_ARGS;
	}
}

static void vfs_chan_close(pid_t pid,file_t file,sVFSNode *node) {
	sChannel *chan = (sChannel*)node->data;
	if(node->name == NULL)
		vfs_node_destroy(node);
	else {
		if(vfs_isDevice(file))
			chan->used = false;

		if(node->refCount == 0) {
			/* notify the driver, if it is one */
			vfs_devmsgs_close(pid,file,node);

			/* if there are message for the driver we don't want to throw them away */
			/* if there are any in the receivelist (and no references of the node) we
			 * can throw them away because no one will read them anymore
			 * (it means that the client has already closed the file) */
			/* note also that we can assume that the driver is still running since we
			 * would have deleted the whole device-node otherwise */
			if(sll_length(chan->sendList) == 0)
				vfs_node_destroy(node);
			else
				chan->closed = true;
		}
	}
}

void vfs_chan_setUsed(sVFSNode *node,bool used) {
	sChannel *chan = (sChannel*)node->data;
	chan->used = used;
}

void vfs_chan_lock(sVFSNode *node) {
	klock_aquire(&node->lock);
}

void vfs_chan_unlock(sVFSNode *node) {
	klock_release(&node->lock);
}

bool vfs_chan_hasReply(const sVFSNode *node) {
	sChannel *chan = (sChannel*)node->data;
	return sll_length(chan->recvList) > 0;
}

bool vfs_chan_hasWork(const sVFSNode *node) {
	sChannel *chan = (sChannel*)node->data;
	return !chan->used && sll_length(chan->sendList) > 0;
}

ssize_t vfs_chan_send(A_UNUSED pid_t pid,file_t file,sVFSNode *n,msgid_t id,USER const void *data,
		size_t size) {
	sSLList **list;
	sThread *t = thread_getRunning();
	sChannel *chan = (sChannel*)n->data;
	if(n->name == NULL)
		return ERR_NODE_DESTROYED;

	/*vid_printf("%d:%s sent msg %d with %d bytes over chan %s:%x (device %s)\n",
			pid,proc_getByPid(pid)->command,id,size,n->name,n,n->parent->name);*/

	/* devices write to the receive-list (which will be read by other processes) */
	if(vfs_isDevice(file)) {
		/* if it is from a device or fs, don't enqueue it but pass it directly to
		 * the corresponding handler */
		if(vfs_device_accepts(n->parent,id)) {
			vfs_req_sendMsg(id,n,data,size);
			return 0;
		}

		list = &(chan->recvList);
	}
	/* other processes write to the send-list (which will be read by the driver) */
	else
		list = &(chan->sendList);

	/* create message and copy data to it */
	sMessage *msg = (sMessage*)cache_alloc(sizeof(sMessage) + size);
	if(msg == NULL)
		return ERR_NOT_ENOUGH_MEM;

	msg->length = size;
	msg->id = id;
	if(data) {
		thread_addHeapAlloc(t,msg);
		memcpy(msg + 1,data,size);
		thread_remHeapAlloc(t,msg);
	}

	if(*list == NULL)
		*list = sll_create();

	/* append to list */
	if(!sll_append(*list,msg)) {
		cache_free(msg);
		return ERR_NOT_ENOUGH_MEM;
	}

	/* notify the driver */
	if(list == &(chan->sendList)) {
		vfs_device_addMsg(n->parent);
		ev_wakeup(EVI_CLIENT,(evobj_t)n->parent);
	}
	/* notify all threads that wait on this node for a msg */
	else
		ev_wakeup(EVI_RECEIVED_MSG,(evobj_t)n);
	return 0;
}

ssize_t vfs_chan_receive(A_UNUSED pid_t pid,file_t file,sVFSNode *node,USER msgid_t *id,
		USER void *data,size_t size) {
	sSLList **list;
	sThread *t = thread_getRunning();
	sChannel *chan = (sChannel*)node->data;
	sVFSNode *waitNode;
	sMessage *msg;
	size_t event;
	ssize_t res;
	sProc *p;

	klock_aquire(&node->lock);
	/* node destroyed? */
	if(node->name == NULL) {
		klock_release(&node->lock);
		return ERR_NODE_DESTROYED;
	}

	/* determine list and event to use */
	if(vfs_isDevice(file)) {
		event = EVI_CLIENT;
		list = &chan->sendList;
		waitNode = node->parent;
	}
	else {
		event = EVI_RECEIVED_MSG;
		list = &chan->recvList;
		waitNode = node;
	}

	/* wait until a message arrives */
	while(sll_length(*list) == 0) {
		if(!vfs_shouldBlock(file)) {
			klock_release(&node->lock);
			return ERR_WOULD_BLOCK;
		}
		/* if the channel has already been closed, there is no hope of success here */
		if(chan->closed) {
			klock_release(&node->lock);
			return ERR_INVALID_FILE;
		}
		ev_wait(t,event,(evobj_t)waitNode);
		klock_release(&node->lock);

		thread_switch();

		if(sig_hasSignalFor(t->tid))
			return ERR_INTERRUPTED;
		/* if we waked up and there is no message, the driver probably died */
		klock_aquire(&node->lock);
		if(node->name == NULL) {
			klock_release(&node->lock);
			return ERR_NODE_DESTROYED;
		}
	}

	/* get first element and copy data to buffer */
	msg = (sMessage*)sll_get(*list,0);
	if(data && msg->length > size) {
		sll_removeFirst(*list);
		goto invArgs;
	}

	/*vid_printf("%d:%s received msg %d from chan %s:%x\n",
			pid,proc_getByPid(pid)->command,msg->id,node->name,node);*/

	/* copy data and id */
	p = proc_request(t->proc->pid,PLOCK_REGIONS);
	if(data) {
		if(!vmm_makeCopySafe(p,data,msg->length)) {
			proc_release(p,PLOCK_REGIONS);
			goto invArgs;
		}
		memcpy(data,msg + 1,msg->length);
	}
	if(id) {
		if(!vmm_makeCopySafe(p,id,sizeof(msgid_t))) {
			proc_release(p,PLOCK_REGIONS);
			goto invArgs;
		}
		*id = msg->id;
	}
	proc_release(p,PLOCK_REGIONS);

	res = msg->length;
	sll_removeFirst(*list);
	if(event == EVI_CLIENT)
		vfs_device_remMsg(node->parent);
	klock_release(&node->lock);
	cache_free(msg);
	return res;

invArgs:
	klock_release(&node->lock);
	cache_free(msg);
	return ERR_INVALID_ARGS;
}

void vfs_chan_print(const sVFSNode *n) {
	size_t i;
	sChannel *chan = (sChannel*)n->data;
	sSLList *lists[] = {chan->sendList,chan->recvList};
	for(i = 0; i < ARRAY_SIZE(lists); i++) {
		size_t j,count = sll_length(lists[i]);
		vid_printf("\t\tChannel %s %s: (%zu)\n",n->name,i ? "recvs" : "sends",count);
		for(j = 0; j < count; j++) {
			sMessage *msg = (sMessage*)sll_get(lists[i],j);
			vid_printf("\t\t\tid=%u len=%zu\n",msg->id,msg->length);
		}
	}
}
