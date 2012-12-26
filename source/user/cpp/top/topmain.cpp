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

#include <esc/common.h>
#include <esc/driver/vterm.h>
#include <esc/esccodes.h>
#include <esc/keycodes.h>
#include <esc/thread.h>
#include <esc/conf.h>
#include <info/process.h>
#include <info/thread.h>
#include <info/cpu.h>
#include <info/memusage.h>
#include <usergroup/user.h>
#include <env.h>
#include <fstream>
#include <iomanip>
#include <string>
#include <stdlib.h>
#include <assert.h>

#define PAGE_SCROLL		10

using namespace std;
using namespace info;

static volatile bool run = true;
static sVTSize consSize;
static size_t pagesize;
static size_t cpubarwidth;
static ssize_t yoffset;
static sUser *users;

template<typename T>
static void printBar(size_t barwidth,double ratio,const T& name) {
	uint filled = (uint)(barwidth * ratio);
	cout << "  " << setw(3) << left << name << "[";
	uint j = 0;
	for(; j < filled; ++j)
		cout << '|';
	for(; j < barwidth; ++j)
		cout << ' ';
}

static void printCPUBar(size_t barwidth,double ratio,cpu::id_type id) {
	printBar(barwidth,ratio,id);
	cout << right << setw(12) << setprecision(1) << 100 * ratio << "%]\n";
}

static void printMemBar(size_t barwidth,size_t used,size_t total,const char *name) {
	char stats[14];
	snprintf(stats,sizeof(stats),"%u/%uMB",used / (1024 * 1024),total / (1024 * 1024));
	double ratio = total == 0 ? 0 : used / (double)(total);
	printBar(barwidth,ratio,name);
	cout << right << setw(13) << stats << "]\n";
}

template<class T>
static void freevector(vector<T*> vec) {
	for(typename vector<T*>::iterator it = vec.begin(); it != vec.end(); ++it)
		delete *it;
}

static bool compareProcs(const process* a,const process* b) {
	return b->cycles() < a->cycles();
}

static void display(void) {
	static tULock lock;
	locku(&lock);
	vector<cpu*> cpus = cpu::get_list();
	vector<process*> procs = process::get_list(false,0,true);
	std::sort(procs.begin(),procs.end(),compareProcs);
	memusage mem = memusage::get();

	cout << "\e[go;0;0]";
	for(uint y = 0; y < consSize.height; ++y) {
		for(uint x = 0; x < consSize.width; ++x)
			cout << " ";
	}
	cout << "\e[go;0;0]";

	for(vector<cpu*>::const_iterator it = cpus.begin(); it != cpus.end(); ++it) {
		double ratio = (*it)->usedCycles() / (double)((*it)->totalCycles());
		printCPUBar(cpubarwidth,ratio,(*it)->id());
	}
	freevector(cpus);

	printMemBar(cpubarwidth,mem.used(),mem.total(),"Mem");
	printMemBar(cpubarwidth,mem.swapUsed(),mem.swapTotal(),"Swp");
	cout << "\n";

	size_t wpid = 5;
	size_t wuser = 10;
	size_t wvirt = 6;
	size_t wphys = 6;
	size_t wshm = 6;
	size_t wcpu = 6;
	size_t wmem = 6;
	size_t wtime = 11;
	size_t cmdbegin = wpid + wuser + wvirt + wphys + wshm + wcpu + wmem + wtime;

	// print header
	cout << "\e[co;0;7]";
	cout << right << setw(wpid) << " PID" << left << setw(wuser) << " USER";
	cout << right << setw(wvirt) << " VIRT" << setw(wphys) << " PHYS" << setw(wshm) << " SHR";
	cout << setw(wcpu) << " CPU%" << setw(wmem) << " MEM%";
	cout << setw(wtime) << " TIME" << " Command";
	for(uint x = cmdbegin + SSTRLEN(" Command"); x < consSize.width; ++x)
		cout << ' ';
	cout << "\n\e[co]";

	// print threads
	size_t totalframes = 0;
	cpu::cycle_type totalcycles = 0;
	for(vector<process*>::const_iterator it = procs.begin(); it != procs.end(); ++it) {
		totalframes += (*it)->ownFrames();
		totalcycles += (*it)->cycles();
	}

	size_t i = 0;
	size_t y = cpus.size() + 4;
	size_t yoff = MIN(yoffset,MAX(0,(ssize_t)procs.size() - (ssize_t)(consSize.height - y)));
	vector<process*>::const_iterator it = procs.begin();
	advance(it,yoff);
	for(; y < consSize.height && it != procs.end(); ++it, ++i) {
		char time[12];
		const process &p = **it;
		sUser *user = user_getById(users,p.uid());

		cout << setw(wpid) << p.pid();
		cout << " " << left << setw(wuser - 1);
		if(user)
			cout.write(user->name,min(strlen(user->name),wuser - 1));
		else
			cout << p.uid();

		cout << right << setw(wvirt - 1) << ((p.pages() * pagesize) / 1024) << "K";
		cout << setw(wphys - 1) << ((p.ownFrames() * pagesize) / 1024) << "K";
		cout << setw(wshm - 1) << ((p.sharedFrames() * pagesize) / 1024) << "K";

		cout << setw(wcpu) << setprecision(1) << (100.0 * (p.cycles() / (double)totalcycles));
		cout << setw(wmem) << setprecision(1) << (100.0 * (p.ownFrames() / (double)totalframes));

		process::cycle_type ms = p.runtime() / 1000;
		process::cycle_type mins = ms / (60 * 1000);
		ms %= 60 * 1000;
		process::cycle_type secs = ms / 1000;
		ms %= 1000;
		snprintf(time,sizeof(time),"%Lu:%02Lu.%03Lu",mins,secs,ms);
		cout << setw(wtime) << time;

		cout << " ";
		cout.write(p.command().c_str(),min(p.command().length(),consSize.width - cmdbegin - 1));
		if(++y >= consSize.height)
			break;
		cout << "\n";
	}
	freevector(procs);
	cout.flush();
	unlocku(&lock);
}

static int refreshThread(void*) {
	pagesize = sysconf(CONF_PAGE_SIZE);
	if(vterm_getSize(STDIN_FILENO,&consSize) < 0)
		error("Unable to determine screensize");

	size_t usercount;
	users = user_parseFromFile(USERS_PATH,&usercount);

	cpubarwidth = consSize.width - SSTRLEN("  99 [10000/10000MB]  ");
	while(run) {
		display();
		sleep(1000);
	}
	return 0;
}

int main(void) {
	vterm_setNavi(STDIN_FILENO,false);
	vterm_setReadline(STDIN_FILENO,false);
	vterm_backup(STDIN_FILENO);

	int tid = startthread(refreshThread,NULL);
	if(tid < 0)
		error("startthread");

	/* open the "real" stdin, because stdin maybe redirected to something else */
	string vtermPath = string("/dev/") + env::get("TERM");
	ifstream vt(vtermPath.c_str());

	// read from vterm
	char c;
	while(run && (c = vt.get()) != EOF) {
		if(c == '\033') {
			istream::esc_type n1,n2,n3;
			istream::esc_type cmd = vt.getesc(n1,n2,n3);
			if(cmd != ESCC_KEYCODE || (n3 & STATE_BREAK))
				continue;
			switch(n2) {
				case VK_Q:
					run = false;
					break;
				case VK_UP:
					if(yoffset > 0)
						yoffset--;
					break;
				case VK_DOWN:
					yoffset++;
					break;
				case VK_PGUP:
					if(yoffset > PAGE_SCROLL)
						yoffset -= PAGE_SCROLL;
					else
						yoffset = 0;
					break;
				case VK_PGDOWN:
					yoffset += PAGE_SCROLL;
					break;
			}
			if(run)
				display();
		}
	}
	join(tid);

	vterm_setNavi(STDIN_FILENO,true);
	vterm_setReadline(STDIN_FILENO,true);
	vterm_restore(STDIN_FILENO);
	return EXIT_SUCCESS;
}