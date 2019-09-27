//Copyright Paul Reiche, Fred Ford. 1992-2002

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

#ifndef LIBS_GRAPHICS_DRAWABLE_H_
#define LIBS_GRAPHICS_DRAWABLE_H_

#include <stdio.h>
#include "tfb_draw.h"

#define ValidPrimType(pt) ((pt)<NUM_PRIMS)

typedef struct bresenham_line
{
	POINT first, second;
	SIZE abs_delta_x, abs_delta_y;
	SIZE error_term;
	BOOLEAN end_points_exchanged;
	INTERSECT_CODE intersect_code;
} BRESENHAM_LINE;

typedef UWORD DRAWABLE_TYPE;
#define ROM_DRAWABLE 0
#define RAM_DRAWABLE 1
#define SCREEN_DRAWABLE 2

struct frame_desc
{
	DRAWABLE_TYPE Type;
	UWORD Index;
	HOT_SPOT HotSpot;
	EXTENT Bounds;
	TFB_Image *image;
	struct drawable_desc *parent;
};

struct drawable_desc
{
	CREATE_FLAGS Flags;
	UWORD MaxIndex;
	FRAME_DESC *Frame;
};

#define GetFrameWidth(f) ((f)->Bounds.width)
#define GetFrameHeight(f) ((f)->Bounds.height)
#define GetFrameBounds(f) ((f)->Bounds)
#define SetFrameBounds(f,w,h) \
		((f)->Bounds.width=(w), \
		((f))->Bounds.height=(h))

#define DRAWABLE_PRIORITY DEFAULT_MEM_PRIORITY

extern DRAWABLE AllocDrawable (COUNT num_frames);
#define FreeDrawable(D) _ReleaseCelData (D)

typedef struct
{
	RECT Box;
	FRAME FramePtr;
} IMAGE_BOX;

extern INTERSECT_CODE _clip_line (const RECT *pClipRect,
		BRESENHAM_LINE *pLine);

extern void *_GetCelData (uio_Stream *fp, DWORD length);
extern BOOLEAN _ReleaseCelData (void *handle);

extern FRAME _CurFramePtr;

// ClipRect is relative to ctxOrigin
extern void _text_blt (RECT *pClipRect, TEXT *TextPtr, POINT ctxOrigin);

#endif /* LIBS_GRAPHICS_DRAWABLE_H_ */

