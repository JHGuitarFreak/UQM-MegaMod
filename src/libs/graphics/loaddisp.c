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

#include "libs/gfxlib.h"
#include "libs/graphics/drawable.h"
#include "libs/log.h"


// Reads a piece of screen into a passed FRAME or a newly created one
DRAWABLE
LoadDisplayPixmap (const RECT *area, FRAME frame)
{
	// TODO: This should just return a FRAME instead of DRAWABLE
	DRAWABLE buffer = GetFrameParentDrawable (frame);
	COUNT index;

	if (!buffer)
	{	// asked to create a new DRAWABLE instead
		buffer = CreateDrawable (WANT_PIXMAP | MAPPED_TO_DISPLAY,
				area->extent.width, area->extent.height, 1);
		if (!buffer)
			return NULL;

		index = 0;
	}
	else
	{
		index = GetFrameIndex (frame);
	}

	frame = SetAbsFrameIndex (CaptureDrawable (buffer), index);

	if (_CurFramePtr->Type != SCREEN_DRAWABLE
			|| frame->Type == SCREEN_DRAWABLE
			|| !(GetFrameParentDrawable (frame)->Flags & MAPPED_TO_DISPLAY))
	{
		log_add (log_Warning, "Unimplemented function activated: "
				"LoadDisplayPixmap()");
	}
	else
	{
		TFB_Image *img = frame->image;
		TFB_DrawScreen_CopyToImage (img, area, TFB_SCREEN_MAIN);
	}

	ReleaseDrawable (frame);

	return buffer;
}

