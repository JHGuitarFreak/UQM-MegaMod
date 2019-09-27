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

/* .duk sound track decoder */

#ifndef DUKAUD_H
#define DUKAUD_H

#include "decoder.h"

extern TFB_SoundDecoderFuncs duka_DecoderVtbl;

typedef enum
{
	// positive values are the same as in errno
	dukae_None = 0,
	dukae_Unknown = -1,
	dukae_BadFile = -2,
	dukae_BadArg = -3,
	dukae_Other = -1000,
} DukAud_Error;

#endif // DUKAUD_H
