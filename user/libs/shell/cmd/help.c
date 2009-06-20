/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/io.h>
#include <esc/fileio.h>
#include <stdlib.h>
#include "help.h"

s32 shell_cmdHelp(u32 argc,char **argv) {
	UNUSED(argc);
	UNUSED(argv);

	printf("Currently you can use the following commands/programs:\n");
	printf("	cat [<file>]					Read file or from STDIN and print\n");
	printf("	colortbl						Print a table with all fg- and bg-colors\n");
	printf("	date [<format>]					Print date (with specified format or %%c)\n");
	printf("	kill <pid>						Kill process\n");
	printf("	libctest						Run libc-tests\n");
	printf("	login							Just for fun ;)\n");
	printf("	ls [-ali] [<dir>]				List current or specified directory\n");
	printf("	progress						Shows a progress-bar\n");
	printf("	ps								Print processes\n");
	printf("	echo <string>,...				Print given arguments\n");
	printf("	env [<name>|<name>=<value>]		Print or set env-variable(s)\n");
	printf("	help							Print this message\n");
	printf("	cd <dir>						Change to given directory\n");
	printf("	pwd								Print current directory\n");
	printf("	wc								Count and/or print words\n");
	printf("	stat <file>						Print file-information\n");
	printf("	less [<file>]					Reads from <file> or STDIN and allows\n");
	printf("									navigation through it:\n");
	printf("										up/down	- one line up/down\n");
	printf("										pageup/pagedown - one page up/down\n");
	printf("										q - quit\n");
	printf("\n");
	printf("	gui								Start GUI\n");
	printf("									(be carefull: this is a oneway-ticket ;))\n");
	printf("	guishell						A shell for the GUI\n");

	printf("\n");
	printf("Additionally there is a basic shell-'language', that supports '|', '\"' and ';'\n");
	printf("So for example you can do:\n");
	printf("	ls; ps; echo \"test\";\n");
	printf("	ls | cat; ps\n");
	printf("	echo \"word1\" word2 \"word3 and 4\" | wc\n");
	printf("	cat file:/bigfile | less\n");
	printf("	ps | wc | wc -p\n");

	printf("\n");
	printf("Other:\n");
	printf("	Tab-Completion works for programs in /bin and files/directories\n");
	printf("	You can send EOF by CTRL+D and kill the current process with CTRL+C\n");
	printf("	You can scroll the screen with shift + up/down/pageUp/-Down\n");
	printf("	You can switch between different vterms via CTRL+1/2/...\n");
	printf("\n");

	return EXIT_SUCCESS;
}
