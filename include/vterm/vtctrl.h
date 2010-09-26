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

#ifndef VTCTRL_H_
#define VTCTRL_H_

#include <esc/common.h>
#include <esc/ringbuffer.h>
#include <esc/esccodes.h>
#include <esc/messages.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TAB_WIDTH			4
#define HISTORY_SIZE		12
#define INPUT_BUF_SIZE		512
#define MAX_VT_NAME_LEN		15

/**
 * The handler for shortcuts
 */
typedef struct sVTerm sVTerm;
typedef bool (*fHandleShortcut)(sVTerm *vt,u32 keycode,u8 modifier,char c);
typedef void (*fSetCursor)(sVTerm *vt);

/* global configuration */
typedef struct {
	bool readKb;
	bool enabled;
} sVTermCfg;

/* our vterm-state */
struct sVTerm {
	/* identification */
	u8 index;
	tDrvId sid;
	char name[MAX_VT_NAME_LEN + 1];
	/* function-pointers */
	fHandleShortcut handlerShortcut;
	fSetCursor setCursor;
	/* number of cols/rows on the screen */
	u8 cols;
	u8 rows;
	/* position (on the current page) */
	u8 col;
	u8 row;
	/* colors */
	u8 defForeground;
	u8 defBackground;
	u8 foreground;
	u8 background;
	/* whether this vterm is currently active */
	u8 active;
	/* file-descriptors */
	tFD video;
	tFD speaker;
	/* the first line with content */
	u16 firstLine;
	/* the line where row+col starts */
	u16 currLine;
	/* the first visible line */
	u16 firstVisLine;
	/* a range that should be updated */
	u16 upStart;
	u16 upLength;
	s16 upScroll;
	/* whether entered characters should be echo'd to screen */
	u8 echo;
	/* whether the vterm should read until a newline occurrs */
	u8 readLine;
	/* whether navigation via up/down/pageup/pagedown is enabled */
	u8 navigation;
	/* whether all output should be printed into the readline-buffer */
	u8 printToRL;
	/* whether all output should be printed to COM1 */
	u8 printToCom1;
	/* a backup of the screen; initially NULL */
	char *screenBackup;
	u16 backupCol;
	u16 backupRow;
	/* the buffer for the input-stream */
	u8 inbufEOF;
	sRingBuf *inbuf;
	/* the pid of the shell for ctrl+c notifications */
	tPid shellPid;
	/* the escape-state */
	s32 escapePos;
	char escapeBuf[MAX_ESCC_LENGTH];
	/* readline-buffer */
	u8 rlStartCol;
	u32 rlBufSize;
	u32 rlBufPos;
	char *rlBuffer;
	char *buffer;
	char *titleBar;
};

/* the colors */
typedef enum {
	/* 0 */ BLACK,
	/* 1 */ BLUE,
	/* 2 */ GREEN,
	/* 3 */ CYAN,
	/* 4 */ RED,
	/* 5 */ MARGENTA,
	/* 6 */ ORANGE,
	/* 7 */ LIGHTGRAY,
	/* 8 */ GRAY,
	/* 9 */ LIGHTBLUE,
	/* 10 */ LIGHTGREEN,
	/* 11 */ LIGHTCYAN,
	/* 12 */ LIGHTRED,
	/* 13 */ LIGHTMARGENTA,
	/* 14 */ YELLOW,
	/* 15 */ WHITE
} eColor;

/**
 * Inits the vterm
 *
 * @param vt the vterm
 * @param vidSize the size of the screen
 * @param vidFd the file-descriptor for the video-driver (or whatever you need :))
 * @param speakerFd the file-descriptor for the speaker-driver
 * @return true if successfull
 */
bool vterm_init(sVTerm *vt,sVTSize *vidSize,tFD vidFd,tFD speakerFd);

/**
 * Handles the control-commands
 *
 * @param vt the vterm
 * @param cfg global configuration
 * @param cmd the command
 * @param data the data
 * @return the result
 */
s32 vterm_ctl(sVTerm *vt,sVTermCfg *cfg,u32 cmd,void *data);

/**
 * Scrolls the screen by <lines> up (positive) or down (negative)
 *
 * @param vt the vterm
 * @param lines the number of lines to move
 */
void vterm_scroll(sVTerm *vt,s16 lines);

/**
 * Marks the whole screen including title-bar dirty
 *
 * @param vt the vterm
 */
void vterm_markScrDirty(sVTerm *vt);

/**
 * Marks the given range as dirty
 *
 * @param vt the vterm
 * @param start the start-position
 * @param length the number of bytes
 */
void vterm_markDirty(sVTerm *vt,u16 start,u16 length);

/**
 * Releases resources
 *
 * @param vt the vterm
 */
void vterm_destroy(sVTerm *vt);

#ifdef __cplusplus
}
#endif

#endif /* VTCTRL_H_ */
