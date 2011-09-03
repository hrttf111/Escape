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
#include <esc/arch/i586/ports.h>
#include <esc/log.h>
#include <assert.h>

static bool reqPorts = false;

void logc(char c) {
	if(!reqPorts) {
		/* request io-ports for qemu and bochs */
		assert(requestIOPort(0xe9) >= 0);
		assert(requestIOPort(0x3f8) >= 0);
		assert(requestIOPort(0x3fd) >= 0);
		reqPorts = true;
	}
	while((inByte(0x3f8 + 5) & 0x20) == 0)
		;
	outByte(0x3f8,c);
}