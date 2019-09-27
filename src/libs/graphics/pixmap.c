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

#include "gfxintrn.h"
#include "libs/log.h"

DRAWABLE
GetFrameParentDrawable (FRAME f)
{
	if (f != NULL)
	{
		return f->parent;
	}
	return NULL;
}

FRAME
CaptureDrawable (DRAWABLE DrawablePtr)
{
	if (DrawablePtr)
	{
		return &DrawablePtr->Frame[0];
	}

	return NULL;
}

DRAWABLE
ReleaseDrawable (FRAME FramePtr)
{
	if (FramePtr != 0)
	{
		DRAWABLE Drawable;

		Drawable = GetFrameParentDrawable (FramePtr);

		return (Drawable);
	}

	return NULL;
}

COUNT
GetFrameCount (FRAME FramePtr)
{
	DRAWABLE_DESC *DrawablePtr;

	if (FramePtr == 0)
		return (0);

	DrawablePtr = GetFrameParentDrawable (FramePtr);
	return DrawablePtr->MaxIndex + 1;
}

COUNT
GetFrameIndex (FRAME FramePtr)
{
	if (FramePtr == 0)
		return (0);

	return FramePtr->Index;
}

FRAME
SetAbsFrameIndex (FRAME FramePtr, COUNT FrameIndex)
{
	if (FramePtr != 0)
	{
		DRAWABLE_DESC *DrawablePtr;

		DrawablePtr = GetFrameParentDrawable (FramePtr);

		FrameIndex = FrameIndex	% (DrawablePtr->MaxIndex + 1);
		FramePtr = &DrawablePtr->Frame[FrameIndex];
	}

	return FramePtr;
}

FRAME
SetRelFrameIndex (FRAME FramePtr, SIZE FrameOffs)
{
	if (FramePtr != 0)
	{
		COUNT num_frames;
		DRAWABLE_DESC *DrawablePtr;

		DrawablePtr = GetFrameParentDrawable (FramePtr);
		num_frames = DrawablePtr->MaxIndex + 1;
		if (FrameOffs < 0)
		{
			while ((FrameOffs += num_frames) < 0)
				;
		}

		FrameOffs = ((SWORD)FramePtr->Index + FrameOffs) % num_frames;
		FramePtr = &DrawablePtr->Frame[FrameOffs];
	}

	return FramePtr;
}

FRAME
SetEquFrameIndex (FRAME DstFramePtr, FRAME SrcFramePtr)
{
	COUNT Index;

	if (!DstFramePtr || !SrcFramePtr)
		return 0;

	Index = GetFrameIndex (SrcFramePtr);
#ifdef DEBUG
	{
		DRAWABLE_DESC *DrawablePtr = GetFrameParentDrawable (DstFramePtr);
		if (Index > DrawablePtr->MaxIndex)
			log_add (log_Debug, "SetEquFrameIndex: source index (%d) beyond "
					"destination range (%d)", (int)Index,
					(int)DrawablePtr->MaxIndex);
	}
#endif
	
	return SetAbsFrameIndex (DstFramePtr, Index);
}

FRAME
IncFrameIndex (FRAME FramePtr)
{
	DRAWABLE_DESC *DrawablePtr;

	if (FramePtr == 0)
		return (0);

	DrawablePtr = GetFrameParentDrawable (FramePtr);
	if (FramePtr->Index < DrawablePtr->MaxIndex)
		return ++FramePtr;
	else
		return DrawablePtr->Frame;
}

FRAME
DecFrameIndex (FRAME FramePtr)
{
	if (FramePtr == 0)
		return (0);

	if (FramePtr->Index > 0)
		return --FramePtr;
	else
	{
		DRAWABLE_DESC *DrawablePtr;

		DrawablePtr = GetFrameParentDrawable (FramePtr);
		return &DrawablePtr->Frame[DrawablePtr->MaxIndex];
	}
}
