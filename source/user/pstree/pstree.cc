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

#include <esc/proto/vterm.h>
#include <esc/stream/std.h>
#include <esc/env.h>
#include <info/process.h>
#include <sys/common.h>
#include <getopt.h>
#include <map>
#include <stdlib.h>

using namespace std;
using namespace esc;
using namespace info;

static const char PIPE				= 0xB3; // |
static const char PIPE_CHILD		= 0xC3; // |-
static const char PIPE_LASTCHILD	= 0xC0; // \-

struct ProcNode {
	process *proc;
	ProcNode *child;
	ProcNode *next;

	explicit ProcNode(process *p) : proc(p), child(), next() {
	}
};

typedef map<pid_t,ProcNode*> map_type;

static uint cols;
static char *prefix;

static void usage(const char *name) {
	serr << "Usage: " << name <<" [<pid>]" << '\n';
	exit(EXIT_FAILURE);
}

static void printRec(ProcNode *n,uint depth) {
	if(n) {
		// print prefix
		if(depth > 0)
			prefix[depth - 1] = n->next ? PIPE_CHILD : PIPE_LASTCHILD;
		sout.write(prefix,depth);
		if(depth > 0)
			prefix[depth - 1] = n->next ? PIPE : ' ';

		// print name
		size_t max = esc::Util::min(n->proc->command().length(),(size_t)(cols - depth - 3));
		if(max < n->proc->command().length()) {
			sout.write(n->proc->command().c_str(),max);
			sout << "...";
		}
		else
			sout << n->proc->command();
		sout << '\n';

		// leave room for '...'
		if(depth + 5 < cols) {
			// print childs
			ProcNode *c = n->child;
			prefix[depth + 1] = PIPE;
			while(c) {
				printRec(c,depth + 2);
				c = c->next;
			}
			prefix[depth + 1] = ' ';
		}
	}
}

static bool isLess(ProcNode *a,ProcNode *b) {
	return strcasecmp(a->proc->command().c_str(),b->proc->command().c_str()) < 0;
}

int main(int argc,char **argv) {
	if(getopt_ishelp(argc,argv))
		usage(argv[0]);

	int pid = argc > 1 ? atoi(argv[1]) : 0;

	// get console-size
	cols = VTerm::getSize(env::get("TERM").c_str()).first;
	prefix = new char[cols];
	memset(prefix,' ',cols);

	map_type pmap;
	vector<process*> procs = process::get_list(false,0,true);
	for(auto it = procs.begin(); it != procs.end(); ++it) {
		process *p = *it;

		// get node (we might already exist if a child was listed before us)
		ProcNode *node;
		map_type::iterator mynode = pmap.find(p->pid());
		if(mynode == pmap.end())
			node = new ProcNode(p);
		else {
			node = mynode->second;
			node->proc = p;
		}

		// special case for init, which is its own parent
		if(p->ppid() != p->pid()) {
			// get parent node
			ProcNode *pnode;
			map_type::iterator parent = pmap.find(p->ppid());
			if(parent == pmap.end()) {
				pnode = new ProcNode(NULL);
				pmap[p->ppid()] = pnode;
			}
			else
				pnode = parent->second;

			// append as child, sorted by name
			ProcNode *c = pnode->child;
			ProcNode *prev = NULL;
			while(c && isLess(c,node)) {
				prev = c;
				c = c->next;
			}
			if(prev) {
				node->next = prev->next;
				prev->next = node;
			}
			else {
				node->next = pnode->child;
				pnode->child = node;
			}
		}

		// insert in pmap
		pmap[p->pid()] = node;
	}

	map_type::iterator start = pmap.find(pid);
	if(start == pmap.end())
		errmsg("No process with pid " << pid);
	else
		printRec(start->second,0);
	return EXIT_SUCCESS;
}
