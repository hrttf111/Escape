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

#pragma once

#include <sys/common.h>

#define MAX_ESCC_LENGTH			19

/* the known escape-codes */
enum {
	ESCC_INVALID						= -1,
	ESCC_INCOMPLETE						= -2,
	ESCC_MOVE_LEFT						= 0,
	ESCC_MOVE_RIGHT						= 1,
	ESCC_MOVE_UP						= 2,
	ESCC_MOVE_DOWN						= 3,
	ESCC_MOVE_HOME						= 4,
	ESCC_MOVE_LINESTART					= 5,
	ESCC_MOVE_LINEEND					= 6,
	ESCC_DEL_FRONT						= 7,
	ESCC_DEL_BACK						= 8,
	ESCC_KEYCODE						= 9,
	ESCC_COLOR							= 10,
	ESCC_GOTO_XY						= 11,
	ESCC_SIM_INPUT						= 12,
};

enum {
	STATE_SHIFT							= 1 << 0,
	STATE_CTRL							= 1 << 1,
	STATE_ALT							= 1 << 2,
	STATE_BREAK							= 1 << 3,
	STATE_CAPS							= 1 << 4,
};

static const int ESCC_ARG_UNUSED 		= -1;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * Determines the escape-code from the given string. Assumes that the last char is '\033' and that
 * the string is null-terminated. If the code is incomplete or invalid ESCC_INCOMPLETE or
 * ESCC_INVALID will be returned, respectivly.
 *
 * @param str the string; will be changed if a valid escape-code has been read to the char behind
 * 	the code.
 * @param n1 will be set to the first argument (ESCC_ARG_UNUSED if unused)
 * @param n2 will be set to the second argument (ESCC_ARG_UNUSED if unused)
 * @param n3 will be set to the third argument (ESCC_ARG_UNUSED if unused)
 * @return the scanned escape-code (ESCC_*)
 */
int escc_get(const char **str,int *n1,int *n2,int *n3);

#if defined(__cplusplus)
}
#endif
