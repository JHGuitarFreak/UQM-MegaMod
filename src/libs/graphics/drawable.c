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


#include "libs/graphics/sdl/sdl_common.h"
#include "libs/graphics/gfx_common.h"
#include "libs/gfxlib.h"
#include "libs/graphics/context.h"
#include "libs/graphics/drawable.h"
#include "libs/graphics/tfb_draw.h"
#include "libs/memlib.h"
#include "tfb_draw.h"
#include <math.h>
#include "libs/log.h"

#ifndef M_PI
#	define M_PI 3.14159265358979323846
#endif

FRAME _CurFramePtr;

FRAME
SetContextFGFrame (FRAME Frame)
{
	FRAME LastFrame;

	if (Frame != (LastFrame = (FRAME)_CurFramePtr))
	{
		if (LastFrame)
			DeactivateDrawable ();

		_CurFramePtr = Frame;
		if (_CurFramePtr)
			ActivateDrawable ();

		if (ContextActive ())
		{
			SwitchContextFGFrame (Frame);
		}
	}

	return (LastFrame);
}

FRAME
GetContextFGFrame (void)
{
	return _CurFramePtr;
}

static DRAWABLE
request_drawable (COUNT NumFrames, DRAWABLE_TYPE DrawableType,
		CREATE_FLAGS flags, SIZE width, SIZE height)
{
	DRAWABLE Drawable;
	COUNT i;

	Drawable = AllocDrawable (NumFrames);
	if (!Drawable)
		return NULL;

	Drawable->Flags = flags;
	Drawable->MaxIndex = NumFrames - 1;

	for (i = 0; i < NumFrames; ++i)
	{
		FRAME FramePtr = &Drawable->Frame[i];

		if (DrawableType == RAM_DRAWABLE && width > 0 && height > 0)
		{
			FramePtr->image = TFB_DrawImage_New (TFB_DrawCanvas_New_TrueColor (
					width, height, (flags & WANT_ALPHA) ? TRUE : FALSE));
		}

		FramePtr->Type = DrawableType;
		FramePtr->Index = i;
		SetFrameBounds (FramePtr, width, height);
	}

	return Drawable;
}

// Kruzen: New construct to create paletted drawable
// Uses previously unused TFB_DrawCanvas_New_Paletted()
static DRAWABLE
request_indexed_drawable (Color *palette, SIZE width, SIZE height)
{
	DRAWABLE Drawable;

	Drawable = AllocDrawable (1);
	if (!Drawable)
		return NULL;

	Drawable->Flags = WANT_PIXMAP;
	Drawable->MaxIndex =0;

	{
		FRAME FramePtr = &Drawable->Frame[0];

		if (width > 0 && height > 0)
		{
			FramePtr->image = TFB_DrawImage_New (
					TFB_DrawCanvas_New_Paletted (
						width, height, palette, -1));
		}
		else
			return NULL;

		FramePtr->Type = RAM_DRAWABLE;
		FramePtr->Index = 0;
		SetFrameBounds (FramePtr, width, height);
	}

	return Drawable;
}

DRAWABLE
CreateDisplay (CREATE_FLAGS CreateFlags, SIZE *pwidth, SIZE *pheight)
{
	DRAWABLE Drawable;

	// TODO: CanvasWidth and CanvasHeight should be passed in
	//   instead of returned.
	Drawable = request_drawable (1, SCREEN_DRAWABLE,
			(CreateFlags & (WANT_PIXMAP | WANT_MASK)),
			CanvasWidth, CanvasHeight);
	if (Drawable)
	{
		FRAME F;

		F = CaptureDrawable (Drawable);
		if (F == 0)
			DestroyDrawable (Drawable);
		else
		{
			*pwidth = GetFrameWidth (F);
			*pheight = GetFrameHeight (F);

			ReleaseDrawable (F);
			return (Drawable);
		}
	}

	*pwidth = *pheight = 0;
	return (0);
}

DRAWABLE
AllocDrawable (COUNT n)
{
	DRAWABLE Drawable;
	Drawable = (DRAWABLE) HCalloc(sizeof (DRAWABLE_DESC));
	if (Drawable)
	{
		int i;
		Drawable->Frame = (FRAME)HMalloc (sizeof (FRAME_DESC) * n);
		if (Drawable->Frame == NULL)
		{
			HFree (Drawable);
			return NULL;
		}

		/* Zero out the newly allocated frames, since HMalloc doesn't have
		 * MEM_ZEROINIT. */
		for (i = 0; i < n; i++) {
			FRAME F;
			F = &Drawable->Frame[i];
			F->parent = Drawable;
			F->Type = 0;
			F->Index = 0;
			F->image = 0;
			F->Bounds.width = 0;
			F->Bounds.height = 0;
			F->HotSpot.x = 0;
			F->HotSpot.y = 0;
		}
	}
	return Drawable;
}

DRAWABLE
CreateDrawable (CREATE_FLAGS CreateFlags, SIZE width, SIZE height, COUNT
		num_frames)
{
	DRAWABLE Drawable;

	Drawable = request_drawable (num_frames, RAM_DRAWABLE,
			(CreateFlags & (WANT_MASK | WANT_PIXMAP
				| WANT_ALPHA | MAPPED_TO_DISPLAY)),
			width, height);
	if (Drawable)
	{
		FRAME F;

		F = CaptureDrawable (Drawable);
		if (F)
		{
			ReleaseDrawable (F);

			return (Drawable);
		}
	}

	return (0);
}

// Kruzen: New construct to create paletted drawable
DRAWABLE
CreateIndexedDrawable (Color *palette, SIZE width, SIZE height)
{
	DRAWABLE Drawable;

	Drawable = request_indexed_drawable (palette, width, height);
		
	if (Drawable)
	{
		FRAME F;

		F = CaptureDrawable (Drawable);
		if (F)
		{
			ReleaseDrawable (F);

			return (Drawable);
		}
	}

	return (0);
}

BOOLEAN
DestroyDrawable (DRAWABLE Drawable)
{
	if (_CurFramePtr && (Drawable == _CurFramePtr->parent))
		SetContextFGFrame ((FRAME)NULL);

	if (Drawable)
	{
		FreeDrawable (Drawable);

		return (TRUE);
	}

	return (FALSE);
}

BOOLEAN
GetFrameRect (FRAME FramePtr, RECT *pRect)
{
	if (FramePtr)
	{
		pRect->corner.x = -FramePtr->HotSpot.x;
		pRect->corner.y = -FramePtr->HotSpot.y;
		pRect->extent = GetFrameBounds (FramePtr);

		return (TRUE);
	}

	return (FALSE);
}

HOT_SPOT
SetFrameHot (FRAME FramePtr, HOT_SPOT HotSpot)
{
	if (FramePtr)
	{
		HOT_SPOT OldHot;

		OldHot = FramePtr->HotSpot;
		FramePtr->HotSpot = HotSpot;

		return (OldHot);
	}

	return (MAKE_HOT_SPOT (0, 0));
}

HOT_SPOT
GetFrameHot (FRAME FramePtr)
{
	if (FramePtr)
	{
		return FramePtr->HotSpot;
	}

	return (MAKE_HOT_SPOT (0, 0));
}

DRAWABLE
RotateFrame (FRAME Frame, int angle_deg)
{
	DRAWABLE Drawable;
	FRAME RotFramePtr;
	double dx, dy;
	double d;
	double angle = angle_deg * M_PI / 180;

	if (!Frame)
		return NULL;

	assert (Frame->Type != SCREEN_DRAWABLE);

	Drawable = request_drawable (1, RAM_DRAWABLE, WANT_PIXMAP, 0, 0);
	if (!Drawable)
		return 0;
	RotFramePtr = CaptureDrawable (Drawable);
	if (!RotFramePtr)
	{
		FreeDrawable (Drawable);
		return 0;
	}

	RotFramePtr->image = TFB_DrawImage_New_Rotated (
			Frame->image, angle_deg);
	SetFrameBounds (RotFramePtr, RotFramePtr->image->extent.width,
			RotFramePtr->image->extent.height);

	/* now we need to rotate the hot-spot, eww */
	dx = Frame->HotSpot.x - (GetFrameWidth (Frame) / 2);
	dy = Frame->HotSpot.y - (GetFrameHeight (Frame) / 2);
	d = sqrt ((double)dx*dx + (double)dy*dy);
	if ((int)d != 0)
	{
		double organg = atan2 (-dy, dx);
		dx = cos (organg + angle) * d;
		dy = -sin (organg + angle) * d;
	}
	RotFramePtr->HotSpot.x = (GetFrameWidth (RotFramePtr) / 2) + (int)dx;
	RotFramePtr->HotSpot.y = (GetFrameHeight (RotFramePtr) / 2) + (int)dy;

	ReleaseDrawable (RotFramePtr);

	return Drawable;
}

// color.a is ignored
void
SetFrameTransparentColor (FRAME frame, Color color)
{
	TFB_Image *img;

	if (!frame)
		return;

	assert (frame->Type != SCREEN_DRAWABLE);

	img = frame->image;
	LockMutex (img->mutex);

	// TODO: This should defer to TFB_DrawImage instead
	TFB_DrawCanvas_SetTransparentColor (img->NormalImg, color, FALSE);
	
	UnlockMutex (img->mutex);
}

Color
GetFramePixel (FRAME frame, POINT pixelPt)
{
	TFB_Image *img;
	Color ret;

	if (!frame)
		return BUILD_COLOR_RGBA (0, 0, 0, 0);

	assert (frame->Type != SCREEN_DRAWABLE);

	img = frame->image;
	LockMutex (img->mutex);

	// TODO: This should defer to TFB_DrawImage instead
	ret = TFB_DrawCanvas_GetPixel (img->NormalImg, pixelPt.x, pixelPt.y);

	UnlockMutex (img->mutex);

	return ret;
}

static FRAME
makeMatchingFrame (FRAME frame, int width, int height)
{
	DRAWABLE drawable;
	FRAME newFrame;
	CREATE_FLAGS flags;
	const bool dst_has_alpha =
			((SDL_Surface*)(frame->image->NormalImg))->format->Amask != 0;

	flags = dst_has_alpha ? WANT_ALPHA : GetFrameParentDrawable (frame)->Flags;
	drawable = CreateDrawable (flags, width, height, 1);
	if (!drawable)
		return NULL;
	newFrame = CaptureDrawable (drawable);
	if (!newFrame)
	{
		FreeDrawable (drawable);
		return NULL;
	}

	return newFrame;
}

// Kruzen: New construct to create paletted drawable
// Copies makeMatchingFrame() call hierarchy
static FRAME
makeMatchingIndexedFrame (FRAME frame, int width, int height)
{
	DRAWABLE drawable;
	FRAME newFrame;
	
	drawable =
			CreateIndexedDrawable (
					TFB_DrawCanvas_ExtractPalette (frame->image->NormalImg),
					width, height);
	if (!drawable)
		return NULL;
	newFrame = CaptureDrawable (drawable);
	if (!newFrame)
	{
		FreeDrawable (drawable);
		return NULL;
	}

	return newFrame;
}

// Creates an new DRAWABLE containing a copy of specified FRAME's rect
// Source FRAME must not be a SCREEN_DRAWABLE
DRAWABLE
CopyFrameRect (FRAME frame, const RECT *area)
{
	FRAME newFrame;
	POINT nullPt = MAKE_POINT (0, 0);

	if (!frame)
		return NULL;

	assert (frame->Type != SCREEN_DRAWABLE);

	newFrame = makeMatchingFrame (frame, area->extent.width,
			area->extent.height);
	if (!newFrame)
		return NULL;

	TFB_DrawImage_CopyRect (frame->image, area, newFrame->image, nullPt);

	return ReleaseDrawable (newFrame);
}

// Creates an new DRAWABLE mostly identical to specified FRAME
// Source FRAME must not be a SCREEN_DRAWABLE
DRAWABLE
CloneFrame (FRAME frame)
{
	FRAME newFrame;
	RECT r;

	if (!frame)
		return NULL;

	assert (frame->Type != SCREEN_DRAWABLE);

	GetFrameRect (frame, &r);
	r.corner.x = 0;
	r.corner.y = 0;

	newFrame = CaptureDrawable (CopyFrameRect (frame, &r));
	if (!newFrame)
		return NULL;

	// copy the hot-spot
	newFrame->HotSpot = frame->HotSpot;

	return ReleaseDrawable (newFrame);
}

// Creates a new DRAWABLE of specified size and scales the passed
// frame onto it. The aspect ratio is not preserved.
DRAWABLE
RescaleFrame (FRAME frame, int width, int height)
{
	FRAME newFrame;
	TFB_Image *img;
	TFB_Canvas src, dst;

	if (!frame)
		return NULL;

	assert (frame->Type != SCREEN_DRAWABLE);

	if (TFB_DrawCanvas_IsPaletted (frame->image->NormalImg))
	{	// request Paletted frame
		newFrame = makeMatchingIndexedFrame (frame, width, height);
	}
	else
	{	// request TrueColor frame
		newFrame = makeMatchingFrame (frame, width, height);
	}

	if (!newFrame)
		return NULL;

	// scale the hot-spot
	newFrame->HotSpot.x = frame->HotSpot.x * width / frame->Bounds.width;
	newFrame->HotSpot.y = frame->HotSpot.y * height / frame->Bounds.height;

	img = frame->image;
	LockMutex (img->mutex);
	// NOTE: We do not lock the target image because nothing has a
	//   reference to it yet!
	src = img->NormalImg;
	dst = newFrame->image->NormalImg;

	TFB_DrawCanvas_Rescale_Nearest (src, dst, -1, NULL, NULL, NULL);
	
	UnlockMutex (img->mutex);

	return ReleaseDrawable (newFrame);
}

// Creates a new DRAWABLE of specified percentage and scales the passed
// frame onto it. The aspect ratio is preserved.
DRAWABLE
RescalePercentage (FRAME frame, float percentage)
{
	FRAME newFrame;
	TFB_Image* img;
	TFB_Canvas src, dst;

	if (!frame || (frame->Bounds.height == 1 && frame->Bounds.width == 1))
		return NULL;

	assert(frame->Type != SCREEN_DRAWABLE);

	percentage = percentage / 100;

	newFrame = makeMatchingFrame (frame,
			(int)(frame->Bounds.width * percentage),
			(int)(frame->Bounds.height * percentage));

	if (!newFrame)
		return NULL;

	// scale the hot-spot
	newFrame->HotSpot.x = (COORD)(frame->HotSpot.x * percentage);
	newFrame->HotSpot.y = (COORD)(frame->HotSpot.y * percentage);

	img = frame->image;
	LockMutex (img->mutex);
	// NOTE: We do not lock the target image because nothing has a
	//   reference to it yet!
	src = img->NormalImg;
	dst = newFrame->image->NormalImg;

	TFB_DrawCanvas_Rescale_Bilinear (src, dst, -1, NULL, NULL, NULL);

	UnlockMutex (img->mutex);

	return ReleaseDrawable(newFrame);
}

BOOLEAN
ReadFramePixelColors (FRAME frame, Color *pixels, int width, int height)
{
	TFB_Image *img;

	if (!frame)
		return FALSE;

	assert (frame->Type != SCREEN_DRAWABLE);

	// TODO: Do we need to lock the img->mutex here?
	img = frame->image;
	return TFB_DrawCanvas_GetPixelColors (img->NormalImg, pixels,
			width, height);
}

// Warning: this functions bypasses DCQ, which is why it is not a DrawXXX
BOOLEAN
WriteFramePixelColors (FRAME frame, const Color *pixels, int width, int height)
{
	TFB_Image *img;

	if (!frame)
		return FALSE;

	assert (frame->Type != SCREEN_DRAWABLE);

	// TODO: Do we need to lock the img->mutex here?
	img = frame->image;
	return TFB_DrawCanvas_SetPixelColors (img->NormalImg, pixels,
			width, height);
}

BOOLEAN
ReadFramePixelIndexes (FRAME frame, BYTE *pixels, int width, int height, BOOLEAN paletted)
{
	TFB_Image *img;

	if (!frame)
		return FALSE;

	assert (frame->Type != SCREEN_DRAWABLE);

	// TODO: Do we need to lock the img->mutex here?
	img = frame->image;
	
	// JMS_GFX: Don't try to read pixel indexes for non-indexed images.
	if (paletted)
		return TFB_DrawCanvas_GetPixelIndexes (img->NormalImg, pixels,
			width, height);
	else
		return FALSE;
}

// Warning: this functions bypasses DCQ, which is why it is not a DrawXXX
BOOLEAN
WriteFramePixelIndexes (FRAME frame, const BYTE *pixels, int width, int height)
{
	TFB_Image *img;

	if (!frame)
		return FALSE;

	assert (frame->Type != SCREEN_DRAWABLE);

	// TODO: Do we need to lock the img->mutex here?
	img = frame->image;
	return TFB_DrawCanvas_SetPixelIndexes (img->NormalImg, pixels,
			width, height);
}
