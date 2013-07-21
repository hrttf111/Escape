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
#include <sys/mem/paging.h>
#include <sys/mem/cache.h>
#include <sys/mem/virtmem.h>
#include <sys/mem/cow.h>
#include <sys/mem/dynarray.h>
#include <sys/arch/mmix/mem/addrspace.h>
#include <sys/task/proc.h>
#include <sys/task/thread.h>
#include <sys/task/event.h>
#include <sys/task/sched.h>
#include <sys/task/elf.h>
#include <sys/task/uenv.h>
#include <sys/task/timer.h>
#include <sys/task/smp.h>
#include <sys/task/terminator.h>
#include <sys/vfs/node.h>
#include <sys/vfs/vfs.h>
#include <sys/log.h>
#include <sys/cpu.h>
#include <sys/config.h>
#include <sys/boot.h>
#include <sys/util.h>
#include <sys/video.h>
#include <string.h>

static const sBootTask tasks[] = {
	{"Initializing physical memory-management...",PhysMem::init},
	{"Initializing address spaces...",aspace_init},
	{"Initializing paging...",paging_init},
	{"Preinit processes...",Proc::preinit},
	{"Initializing dynarray...",DynArray::init},
	{"Initializing SMP...",SMP::init},
	{"Initializing VFS...",vfs_init},
	{"Initializing event system...",Event::init},
	{"Initializing processes...",Proc::init},
	{"Initializing scheduler...",Sched::init},
	{"Initializing terminator...",Terminator::init},
	{"Start logging to VFS...",log_vfsIsReady},
	{"Initializing copy-on-write...",CopyOnWrite::init},
	{"Initializing timer...",Timer::init},
	{"Initializing signal handling...",Signals::init},
};
const sBootTaskList bootTaskList(tasks,ARRAY_SIZE(tasks));

static sLoadProg progs[MAX_PROG_COUNT];
static sBootInfo info;
static int bootState = 0;
static int bootFinished = 1;

void boot_arch_start(sBootInfo *binfo) {
	int argc;
	const char **argv;

	/* make a copy of the bootinfo, since the location it is currently stored in will be overwritten
	 * shortly */
	memcpy(&info,binfo,sizeof(sBootInfo));
	info.progs = progs;
	memcpy((void*)info.progs,binfo->progs,sizeof(sLoadProg) * binfo->progCount);
	/* start idle-thread, load programs (without kernel) and wait for programs */
	bootFinished = (info.progCount - 1) * 2 + 1;

	vid_init();
	CPU::setSpeed(info.cpuHz);

	/* parse the boot parameter */
	argv = boot_parseArgs(binfo->progs[0].command,&argc);
	Config::parseBootParams(argc,argv);
}

const sBootInfo *boot_getInfo(void) {
	return &info;
}

size_t boot_getKernelSize(void) {
	return progs[0].size;
}

size_t boot_getModuleSize(void) {
	uintptr_t start = progs[1].start;
	uintptr_t end = progs[info.progCount - 1].start + progs[info.progCount - 1].size;
	return end - start;
}

uintptr_t boot_getModuleRange(const char *name,size_t *size) {
	size_t i;
	for(i = 1; i < info.progCount; i++) {
		if(strcmp(progs[i].command,name) == 0) {
			*size = progs[i].size;
			return progs[i].start;
		}
	}
	return 0;
}

int boot_loadModules(A_UNUSED sIntrptStackFrame *stack) {
	size_t i;
	int child;

	/* it's not good to do this twice.. */
	if(bootState == bootFinished)
		return 0;

	/* note that we can't do more than one Proc::clone at once here, because we have to leave the
	 * kernel immediatly to choose another stack (the created process gets our stack) */
	if(bootState == 0) {
		/* start idle-thread */
		Proc::startThread((uintptr_t)&thread_idle,T_IDLE,NULL);
		/* start termination-thread */
		Proc::startThread((uintptr_t)&Terminator::start,0,NULL);
		bootState++;
	}
	else if((bootState % 2) == 1) {
		i = (bootState / 2) + 1;
		/* clone proc */
		if((child = Proc::clone(P_BOOT)) == 0) {
			int res,argc;
			/* parse args */
			const char **argv = boot_parseArgs(progs[i].command,&argc);
			if(argc < 2)
				util_panic("Invalid arguments for boot-module: %s\n",progs[i].path);
			/* exec */
			res = Proc::exec(argv[0],argv,(void*)progs[i].start,progs[i].size);
			if(res < 0)
				util_panic("Unable to exec boot-program %s: %d\n",progs[i].path,res);
			/* we don't want to continue ;) */
			return 0;
		}
		else if(child < 0)
			util_panic("Unable to clone process for boot-program %s: %d\n",progs[i].command,child);

		bootState++;
	}
	else {
		i = bootState / 2;
		inode_t nodeNo;
		int argc;
		const char **argv = boot_parseArgs(progs[i].command,&argc);

		/* wait until the device is registered */
		/* don't create a pipe- or channel-node here */
		while(vfs_node_resolvePath(argv[1],&nodeNo,NULL,VFS_NOACCESS) < 0) {
			/* Note that we HAVE TO sleep here because we may be waiting for ata and fs is not
			 * started yet. I.e. if ata calls sleep() there is no other runnable thread (except
			 * idle, but its just chosen if nobody else wants to run), so that we wouldn't make
			 * a switch but stay here for ever (and get no timer-interrupts to wakeup ata) */
			Timer::sleepFor(Thread::getRunning()->getTid(),20,true);
			Thread::switchAway();
		}

		bootState++;
	}

	/* TODO */
#if 0
	/* start the swapper-thread. it will never return */
	if(PhysMem::canSwap())
		Proc::startThread((uintptr_t)&PhysMem::swapper,0,NULL);
#endif

	if(bootState == bootFinished) {
		/* if not requested otherwise, from now on, print only to log */
		if(!Config::get(Config::LOG2SCR))
			vid_setTargets(TARGET_LOG);
	}
	return bootState == bootFinished ? 0 : 1;
}

void boot_print(void) {
	size_t i;
	vid_printf("Memory size: %zu bytes\n",info.memSize);
	vid_printf("Disk size: %zu bytes\n",info.diskSize);
	vid_printf("Kernelstack-begin: %p\n",info.kstackBegin);
	vid_printf("Kernelstack-end: %p\n",info.kstackEnd);
	vid_printf("Boot modules:\n");
	/* skip kernel */
	for(i = 0; i < info.progCount; i++) {
		vid_printf("\t%s\n\t\t[%p .. %p]\n",info.progs[i].command,
				info.progs[i].start,info.progs[i].start + info.progs[i].size);
	}
}
