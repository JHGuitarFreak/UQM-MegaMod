/*
 * Copyright (C) 2003  Serge van den Boom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 * Nota bene: later versions of the GNU General Public License do not apply
 * to this program.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _uio_TYPES_H
#define _uio_TYPES_H

#include "config.h"

#ifdef _MSC_VER
#	if (_MSC_VER >= 1800)
#		include <stdbool.h>
#	endif
#endif

// ISO C99 compatible boolean types. The ISO C99 standard defines:
// - An object declared as type _Bool, large enough to store the values 0
//   and 1, the rank of which is less than the rank of all other standard
//   integer types.
// - A macro "bool", which expands to "_Bool".
// - A macro "true", which expands to the integer constant 1, suitable for
//   use in #if preprocessing directives.
// - A macro "false", which expands to the integer constant 0, suitable for
//   use in #if preprocessing directives.
// - A macro "__bool_true_false_are_defined", which expands to the integer
//   constant 1, suitable for use in #if preprocessing directives.
#ifndef __bool_true_false_are_defined
#undef bool
#undef false
#undef true
#ifndef HAVE__BOOL
typedef unsigned char _Bool;
#endif  /* HAVE_BOOL */
#define bool _Bool
#define true 1
#define false 0
#define __bool_true_false_are_defined
#endif  /* __bool_true_false_are_defined */

typedef bool uio_bool;

typedef unsigned char  uio_uint8;
typedef   signed char  uio_sint8;
typedef unsigned short uio_uint16;
typedef   signed short uio_sint16;
typedef unsigned int   uio_uint32;
typedef   signed int   uio_sint32;

typedef unsigned long  uio_uintptr;
		// Needs to be adapted for 64 bits systems

#endif  /* _uio_TYPES_H */


