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

#ifndef PROCESSMANAGER_H_
#define PROCESSMANAGER_H_

#include <esc/common.h>
#include <vector>
#include "process.h"
#include "progress.h"

class ProcessManager {
private:
	static const size_t KERNEL_PERCENT	= 40;

public:
	ProcessManager()
		: _lock(0), _downProg(NULL), _procs(std::vector<Process*>()) {
	};
	~ProcessManager() {
	};

	void start();
	void restart(pid_t pid);
	void shutdown();
	void setAlive(pid_t pid);
	void died(pid_t pid);
	void forceShutdown();

private:
	// no cloning
	ProcessManager(const ProcessManager& p);
	ProcessManager &operator=(const ProcessManager& p);

	Process *getByPid(pid_t pid);
	void addRunning();
	void waitForFS();
	void setVTermEnabled(bool enabled);

private:
	tULock _lock;
	Progress *_downProg;
	std::vector<Process*> _procs;
};

#endif /* PROCESSMANAGER_H_ */
