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

#define le16tocpu(x)	(x)
#define le32tocpu(x)	(x)
#define cputole16(x)	(x)
#define cputole32(x)	(x)

#define be16tocpu(x)	((((x) & 0xFF) << 8) | ((x) >> 8))
#define be32tocpu(x)	((((x) & 0xFF) << 24) | \
						 (((x) & 0xFF00) << 8) | \
						 (((x) & 0xFF0000) >> 8) | \
						  ((x) >> 24))

#define cputobe16(x)	be16tocpu(x)
#define cputobe32(x)	be32tocpu(x)
