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

#include <esc/common.h>
#include <ipc/proto/input.h>

class Keyboard {
	Keyboard() = delete;

	enum {
		CMD_LEDS		= 0xED,
	};

public:
	enum {
		LED_SCROLL_LOCK	= 1 << 0,
		LED_NUM_LOCK	= 1 << 1,
		LED_CAPS_LOCK	= 1 << 2,
	};

	static void init();
	static int run(void*);

private:
	static void updateLEDs(const ipc::Keyb::Event &ev);
	static int irqThread(void*);

	static uint8_t _leds;
};
