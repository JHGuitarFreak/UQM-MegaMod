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

#ifndef LIBS_VIDEO_DUKVID_H_
#define LIBS_VIDEO_DUKVID_H_

#include "libs/video/videodec.h"

extern TFB_VideoDecoderFuncs dukv_DecoderVtbl;

typedef enum
{
	// positive values are the same as in errno
	dukve_None = 0,
	dukve_Unknown = -1,
	dukve_BadFile = -2,
	dukve_BadArg = -3,
	dukve_OutOfBuf = -4,
	dukve_EOF = -5,
	dukve_Other = -1000,
} DukVid_Error;

#endif // LIBS_VIDEO_DUKVID_H_
