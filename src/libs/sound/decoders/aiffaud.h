/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* AIFF decoder */

#ifndef AIFFAUD_H
#define AIFFAUD_H

#include "decoder.h"

extern TFB_SoundDecoderFuncs aifa_DecoderVtbl;

typedef enum
{
	// positive values are the same as in errno
	aifae_None = 0,
	aifae_Unknown = -1,
	aifae_BadFile = -2,
	aifae_BadArg = -3,
	aifae_Other = -1000,
} aifa_Error;

#endif /* AIFFAUD_H */
