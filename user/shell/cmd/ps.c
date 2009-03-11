/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <io.h>
#include <dir.h>
#include <proc.h>
#include <string.h>
#include "ps.h"

/**
 * Prints the given process
 *
 * @param p the process
 */
static void ps_printProcess(sProc *p);

static cstring states[] = {
	"Unused ",
	"Running",
	"Ready  ",
	"Blocked",
	"Zombie "
};

s32 shell_cmdPs(u32 argc,s8 **argv) {
	s32 dd,dfd;
	sProc proc;
	sDir *entry;
	s8 path[] = "system:/processes/";
	s8 ppath[255];

	printf("PID\t\tPPID\tPAGES\t\tSTATE\t\t\tCYCLES\t\t\t\t\t\t\tNAME\n");

	if((dd = opendir(path)) >= 0) {
		while((entry = readdir(dd)) != NULL) {
			if(strcmp(entry->name,".") == 0 || strcmp(entry->name,"..") == 0)
				continue;

			strncpy(ppath,path,strlen(path));
			strncat(ppath,entry->name,strlen(entry->name));
			if((dfd = open(ppath,IO_READ)) >= 0) {
				read(dfd,&proc,sizeof(sProc));
				ps_printProcess(&proc);
				close(dfd);
			}
			else {
				printf("Unable to open '%s'\n",ppath);
				return 1;
			}
		}
		closedir(dd);
	}
	else {
		printf("Unable to open '%s'\n",path);
		return 1;
	}

	printf("\n");

	return 0;
}

static void ps_printProcess(sProc *p) {
	u32 *ptr = &p->cycleCount;
	printf("%02d\t\t%02d\t\t%03d\t\t\t%s\t\t0x%08x%08x\t%s\n",
			p->pid,p->parentPid,p->textPages + p->dataPages + p->stackPages,
			states[p->state],*(ptr + 1),*ptr,p->name);
}
