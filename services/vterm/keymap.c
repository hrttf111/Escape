/**
 * @version		$Id$
 * @author		Nils Asmussen <nils@script-solution.de>
 * @copyright	2008 Nils Asmussen
 */

#include <common.h>
#include <keycodes.h>
#include "keymap.h"

static sKeymapEntry keymap[] = {
	/* - none - */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_ACCENT */		{'`',		'~',		NPRINT},
	/* VK_0 */			{'0',		')',		NPRINT},
	/* VK_1 */			{'1',		'!',		NPRINT},
	/* VK_2 */			{'2',		'@',		NPRINT},
	/* VK_3 */			{'3',		'#',		NPRINT},
	/* VK_4 */			{'4',		'$',		NPRINT},
	/* VK_5 */			{'5',		'%',		NPRINT},
	/* VK_6 */			{'6',		'^',		NPRINT},
	/* VK_7 */			{'7',		'&',		NPRINT},
	/* VK_8 */			{'8',		'*',		NPRINT},
	/* VK_9 */			{'9',		'(',		NPRINT},
	/* VK_MINUS */		{'-',		'_',		NPRINT},
	/* VK_EQ */			{'=',		'+',		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_BACKSP */		{'\b',		'\b',		NPRINT},
	/* VK_TAB */		{'\t',		'\t',		NPRINT},
	/* VK_Q */			{'q',		'Q',		NPRINT},
	/* VK_W */			{'w',		'W',		NPRINT},
	/* VK_E */			{'e',		'E',		NPRINT},
	/* VK_R */			{'r',		'R',		NPRINT},
	/* VK_T */			{'t',		'T',		NPRINT},
	/* VK_Y */			{'y',		'Y',		NPRINT},
	/* VK_U */			{'u',		'U',		NPRINT},
	/* VK_I */			{'i',		'I',		NPRINT},
	/* VK_O */			{'o',		'O',		NPRINT},
	/* VK_P */			{'p',		'P',		NPRINT},
	/* VK_LBRACKET */	{'[',		'{',		NPRINT},
	/* VK_RBRACKET */	{']',		'}',		NPRINT},
	/* VK_BACKSLASH */	{'\\',		'|',		NPRINT},
	/* VK_CAPS */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_A */			{'a',		'A',		NPRINT},
	/* VK_S */			{'s',		'S',		NPRINT},
	/* VK_D */			{'d',		'D',		NPRINT},
	/* VK_F */			{'f',		'F',		NPRINT},
	/* VK_G */			{'g',		'G',		NPRINT},
	/* VK_H */			{'h',		'H',		NPRINT},
	/* VK_J */			{'j',		'J',		NPRINT},
	/* VK_K */			{'k',		'K',		NPRINT},
	/* VK_L */			{'l',		'L',		NPRINT},
	/* VK_SEM */		{';',		':',		NPRINT},
	/* VK_APOS */		{'\'',		'"',		NPRINT},
	/* non-US-1 */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_ENTER */		{'\n',		'\n',		NPRINT},
	/* VK_LSHIFT */		{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_Z */			{'z',		'Z',		NPRINT},
	/* VK_X */			{'x',		'X',		NPRINT},
	/* VK_C */			{'c',		'C',		NPRINT},
	/* VK_V */			{'v',		'V',		NPRINT},
	/* VK_B */			{'b',		'B',		NPRINT},
	/* VK_N */			{'n',		'N',		NPRINT},
	/* VK_M */			{'m',		'M',		NPRINT},
	/* VK_COMMA */		{',',		'<',		NPRINT},
	/* VK_DOT */		{'.',		'>',		NPRINT},
	/* VK_SLASH */		{'/',		'?',		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_RSHIFT */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_LCTRL */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_LSUPER */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_LALT */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_SPACE */		{' ',		' ',		NPRINT},
	/* VK_RALT */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_APPS */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_RCTRL */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_RSUPER */		{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_INSERT */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_DELETE */		{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_LEFT */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_HOME */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_END */		{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_UP */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_DOWN */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_PGUP */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_PGDOWN */		{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_RIGHT */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_NUM */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_KP7 */		{'7',		'7',		NPRINT},
	/* VK_KP4 */		{'4',		'4',		NPRINT},
	/* VK_KP1 */		{'1',		'1',		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_KPDIV */		{'/',		'/',		NPRINT},
	/* VK_KP8 */		{'8',		'8',		NPRINT},
	/* VK_KP5 */		{'5',		'5',		NPRINT},
	/* VK_KP2 */		{'2',		'2',		NPRINT},
	/* VK_KP0 */		{'0',		'0',		NPRINT},
	/* VK_KPMUL */		{'*',		'*',		NPRINT},
	/* VK_KP9 */		{'9',		'9',		NPRINT},
	/* VK_KP6 */		{'6',		'6',		NPRINT},
	/* VK_KP3 */		{'3',		'3',		NPRINT},
	/* VK_KPDOT */		{'.',		'.',		NPRINT},
	/* VK_KPSUB */		{'-',		'-',		NPRINT},
	/* VK_KPADD */		{'+',		'+',		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_KPENTER */	{'\n',		'\n',		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_ESC */		{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F1 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F2 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F3 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F4 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F5 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F6 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F7 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F8 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F9 */			{NPRINT,	NPRINT,		NPRINT},
	/* VK_F10 */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_F11 */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_F12 */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_PRINT */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_SCROLL */		{NPRINT,	NPRINT,		NPRINT},
	/* VK_PAUSE */		{NPRINT,	NPRINT,		NPRINT},
	/* --- */			{NPRINT,	NPRINT,		NPRINT},
};

sKeymapEntry *keymap_get(u8 keyCode) {
	if(keyCode >= ARRAY_SIZE(keymap))
		return NULL;
	return keymap + keyCode;
}

#if 0
	bool brk;
	u8 keycode = 0;/*kb_set1_getKeycode(scanCode,&brk);*/
	if(keycode) {
		sKeymapEntry *e;
		s8 c;
		switch(keycode) {
			case VK_LSHIFT:
			case VK_RSHIFT:
				shift = !brk;
				break;
			case VK_LALT:
			case VK_RALT:
				alt = !brk;
				break;
			case VK_LCTRL:
			case VK_RCTRL:
				ctrl = !brk;
				break;
		}

		/* don't print break-keycodes */
		if(brk)
			return;

		e = keymap + keycode;
		if(shift)
			c = e->shift;
		else if(alt)
			c = e->alt;
		else
			c = e->def;

		/*if(c != NPRINT)
			vid_putchar(c);*/
	}
}
#endif
