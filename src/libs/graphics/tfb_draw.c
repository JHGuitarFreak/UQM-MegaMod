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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "gfx_common.h"
#include "tfb_draw.h"
#include "drawcmd.h"
#include "libs/log.h"
#include "libs/memlib.h"


static const HOT_SPOT NullHs = {0, 0};

void
TFB_DrawScreen_Line (int x1, int y1, int x2, int y2, Color color,
		DrawMode mode, SCREEN dest)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_LINE;
	DC.data.line.x1 = x1;
	DC.data.line.y1 = y1;
	DC.data.line.x2 = x2;
	DC.data.line.y2 = y2;
	DC.data.line.color = color;
	DC.data.line.drawMode = mode;
	DC.data.line.destBuffer = dest;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_Rect (RECT *rect, Color color, DrawMode mode, SCREEN dest)
{
	RECT locRect;
	TFB_DrawCommand DC;

	if (!rect)
	{
		locRect.corner.x = locRect.corner.y = 0;
		locRect.extent.width = ScreenWidth;
		locRect.extent.height = ScreenHeight;
		rect = &locRect;
	}

	DC.Type = TFB_DRAWCOMMANDTYPE_RECTANGLE;
	DC.data.rect.rect = *rect;
	DC.data.rect.color = color;
	DC.data.rect.drawMode = mode;
	DC.data.rect.destBuffer = dest;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_Image (TFB_Image *img, int x, int y, int scale,
		int scaleMode, TFB_ColorMap *cmap, DrawMode mode, SCREEN dest)
{
	TFB_DrawCommand DC;
	
	DC.Type = TFB_DRAWCOMMANDTYPE_IMAGE;
	DC.data.image.image = img;
	DC.data.image.colormap = cmap;
	DC.data.image.x = x;
	DC.data.image.y = y;
	DC.data.image.scale = (scale == GSCALE_IDENTITY) ? 0 : scale;
	DC.data.image.scaleMode = scaleMode;
	DC.data.image.drawMode = mode;
	DC.data.image.destBuffer = dest;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_FilledImage (TFB_Image *img, int x, int y, int scale,
		int scaleMode, Color color, DrawMode mode, SCREEN dest)
{
	TFB_DrawCommand DC;
	
	DC.Type = TFB_DRAWCOMMANDTYPE_FILLEDIMAGE;
	DC.data.filledimage.image = img;
	DC.data.filledimage.x = x;
	DC.data.filledimage.y = y;
	DC.data.filledimage.scale = (scale == GSCALE_IDENTITY) ? 0 : scale;
	DC.data.filledimage.scaleMode = scaleMode;
	DC.data.filledimage.color = color;
	DC.data.filledimage.drawMode = mode;
	DC.data.filledimage.destBuffer = dest;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_FontChar (TFB_Char *fontChar, TFB_Image *backing,
		int x, int y, DrawMode mode, SCREEN dest)
{
	TFB_DrawCommand DC;
	
	DC.Type = TFB_DRAWCOMMANDTYPE_FONTCHAR;
	DC.data.fontchar.fontchar = fontChar;
	DC.data.fontchar.backing = backing;
	DC.data.fontchar.x = x;
	DC.data.fontchar.y = y;
	DC.data.fontchar.drawMode = mode;
	DC.data.fontchar.destBuffer = dest;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_CopyToImage (TFB_Image *img, const RECT *r, SCREEN src)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_COPYTOIMAGE;
	DC.data.copytoimage.rect = *r;
	DC.data.copytoimage.image = img;
	DC.data.copytoimage.srcBuffer = src;
	
	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_Copy (const RECT *r, SCREEN src, SCREEN dest)
{
	RECT locRect;
	TFB_DrawCommand DC;

	if (!r)
	{
		locRect.corner.x = locRect.corner.y = 0;
		locRect.extent.width = ScreenWidth;
		locRect.extent.height = ScreenHeight;
		r = &locRect;
	}

	DC.Type = TFB_DRAWCOMMANDTYPE_COPY;
	DC.data.copy.rect = *r;
	DC.data.copy.srcBuffer = src;
	DC.data.copy.destBuffer = dest;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_SetMipmap (TFB_Image *img, TFB_Image *mmimg, int hotx, int hoty)
{
	TFB_DrawCommand DC;

	DC.Type = TFB_DRAWCOMMANDTYPE_SETMIPMAP;
	DC.data.setmipmap.image = img;
	DC.data.setmipmap.mipmap = mmimg;
	DC.data.setmipmap.hotx = hotx;
	DC.data.setmipmap.hoty = hoty;

	TFB_EnqueueDrawCommand (&DC);
}

void
TFB_DrawScreen_DeleteImage (TFB_Image *img)
{
	if (img)
	{
		TFB_DrawCommand DC;

		DC.Type = TFB_DRAWCOMMANDTYPE_DELETEIMAGE;
		DC.data.deleteimage.image = img;

		TFB_EnqueueDrawCommand (&DC);
	}
}

void
TFB_DrawScreen_DeleteData (void *data)
		// data must be a result of HXalloc() call
{
	if (data)
	{
		TFB_DrawCommand DC;

		DC.Type = TFB_DRAWCOMMANDTYPE_DELETEDATA;
		DC.data.deletedata.data = data;

		TFB_EnqueueDrawCommand (&DC);
	}
}

void
TFB_DrawScreen_WaitForSignal (void)
{
	TFB_DrawCommand DrawCommand;
	Semaphore s;
	s = GetMyThreadLocal ()->flushSem;
	DrawCommand.Type = TFB_DRAWCOMMANDTYPE_SENDSIGNAL;
	DrawCommand.data.sendsignal.sem = s;
	Lock_DCQ (1);
	TFB_BatchReset ();
	TFB_EnqueueDrawCommand (&DrawCommand);
	Unlock_DCQ();
	SetSemaphore (s);	
}

void
TFB_DrawScreen_ReinitVideo (int driver, int flags, int width, int height)
{
	TFB_DrawCommand DrawCommand;
	DrawCommand.Type = TFB_DRAWCOMMANDTYPE_REINITVIDEO;
	DrawCommand.data.reinitvideo.driver = driver;
	DrawCommand.data.reinitvideo.flags = flags;
	DrawCommand.data.reinitvideo.width = width;
	DrawCommand.data.reinitvideo.height = height;
	TFB_EnqueueDrawCommand (&DrawCommand);
}

void
TFB_DrawScreen_Callback (void (*callback) (void *arg), void *arg)
{
	TFB_DrawCommand DrawCommand;
	DrawCommand.Type = TFB_DRAWCOMMANDTYPE_CALLBACK;
	DrawCommand.data.callback.callback = callback;
	DrawCommand.data.callback.arg = arg;
	TFB_EnqueueDrawCommand(&DrawCommand);
}

void
TFB_DrawImage_Line (int x1, int y1, int x2, int y2, Color color,
		DrawMode mode, TFB_Image *target)
{
	LockMutex (target->mutex);
	TFB_DrawCanvas_Line (x1, y1, x2, y2, color, mode, target->NormalImg);
	target->dirty = TRUE;
	UnlockMutex (target->mutex);
}

void
TFB_DrawImage_Rect (RECT *rect, Color color, DrawMode mode, TFB_Image *target)
{
	LockMutex (target->mutex);
	TFB_DrawCanvas_Rect (rect, color, mode, target->NormalImg);
	target->dirty = TRUE;
	UnlockMutex (target->mutex);
}

void
TFB_DrawImage_Image (TFB_Image *img, int x, int y, int scale,
		int scaleMode, TFB_ColorMap *cmap, DrawMode mode, TFB_Image *target)
{
	LockMutex (target->mutex);
	TFB_DrawCanvas_Image (img, x, y, scale, scaleMode, cmap,
			mode, target->NormalImg);
	target->dirty = TRUE;
	UnlockMutex (target->mutex);
}

void
TFB_DrawImage_FilledImage (TFB_Image *img, int x, int y, int scale,
		int scaleMode, Color color, DrawMode mode, TFB_Image *target)
{
	LockMutex (target->mutex);
	TFB_DrawCanvas_FilledImage (img, x, y, scale, scaleMode, color,
			mode, target->NormalImg);
	target->dirty = TRUE;
	UnlockMutex (target->mutex);
}

void
TFB_DrawImage_FontChar (TFB_Char *fontChar, TFB_Image *backing,
		int x, int y, DrawMode mode, TFB_Image *target)
{
	LockMutex (target->mutex);
	TFB_DrawCanvas_FontChar (fontChar, backing, x, y, mode, target->NormalImg);
	target->dirty = TRUE;
	UnlockMutex (target->mutex);
}


TFB_Image *
TFB_DrawImage_New (TFB_Canvas canvas)
{
	TFB_Image *img = HMalloc (sizeof (TFB_Image));
	img->mutex = CreateMutex ("image lock", SYNC_CLASS_VIDEO);
	img->ScaledImg = NULL;
	img->MipmapImg = NULL;
	img->FilledImg = NULL;
	img->colormap_index = -1;
	img->colormap_version = 0;
	img->NormalHs = NullHs;
	img->MipmapHs = NullHs;
	img->last_scale_hs = NullHs;
	img->last_scale_type = -1;
	img->last_scale = 0;
	img->dirty = FALSE;
	TFB_DrawCanvas_GetExtent (canvas, &img->extent);

	if (TFB_DrawCanvas_IsPaletted (canvas))
	{
		img->NormalImg = canvas;
	}
	else
	{
		img->NormalImg = TFB_DrawCanvas_ToScreenFormat (canvas);
	}

	return img;
}

TFB_Image*
TFB_DrawImage_CreateForScreen (int w, int h, BOOLEAN withalpha)
{
	TFB_Image* img = HMalloc (sizeof (TFB_Image));
	img->mutex = CreateMutex ("image lock", SYNC_CLASS_VIDEO);
	img->ScaledImg = NULL;
	img->MipmapImg = NULL;
	img->FilledImg = NULL;
	img->colormap_index = -1;
	img->colormap_version = 0;
	img->NormalHs = NullHs;
	img->MipmapHs = NullHs;
	img->last_scale_hs = NullHs;
	img->last_scale_type = -1;
	img->last_scale = 0;
	img->extent.width = w;
	img->extent.height = h;

	img->NormalImg = TFB_DrawCanvas_New_ForScreen (w, h, withalpha);

	return img;
}

TFB_Image *
TFB_DrawImage_New_Rotated (TFB_Image *img, int angle)
{
	TFB_Canvas dst;
	EXTENT size;
	TFB_Image* newimg;

	/* sanity check */
	if (!img->NormalImg)
	{
		log_add (log_Warning, "TFB_DrawImage_New_Rotated: "
				"source canvas is NULL! Failing.");
		return NULL;
	}

	TFB_DrawCanvas_GetRotatedExtent (img->NormalImg, angle, &size);
	dst = TFB_DrawCanvas_New_RotationTarget (img->NormalImg, angle);
	if (!dst)
	{
		log_add (log_Warning, "TFB_DrawImage_New_Rotated: "
				"rotation target canvas not created! Failing.");
		return NULL;
	}
	TFB_DrawCanvas_Rotate (img->NormalImg, dst, angle, size);
	
	newimg = TFB_DrawImage_New (dst);
	return newimg;
}

void
TFB_DrawImage_SetMipmap (TFB_Image *img, TFB_Image *mmimg, int hotx, int hoty)
{
	bool imgpal;
	bool mmpal;

	if (!img || !mmimg)
		return;

	LockMutex (img->mutex);
	LockMutex (mmimg->mutex);
	
	// Either both images must be using the same colormap, or mipmap image
	// must not be paletted. This restriction is due to the current
	// implementation of fill-stamp, which replaces the palette with
	// fill color.
	imgpal = TFB_DrawCanvas_IsPaletted (img->NormalImg);
	mmpal = TFB_DrawCanvas_IsPaletted (mmimg->NormalImg);
	if (!mmpal || (mmpal && imgpal &&
			img->colormap_index == mmimg->colormap_index))
	{
		img->MipmapImg = mmimg->NormalImg;
		img->MipmapHs.x = hotx;
		img->MipmapHs.y = hoty;
	}
	else
	{
		img->MipmapImg = NULL;
	}

	UnlockMutex (mmimg->mutex);
	UnlockMutex (img->mutex);
}

void 
TFB_DrawImage_Delete (TFB_Image *image)
{
	if (image == 0)
	{
		log_add (log_Warning, "INTERNAL ERROR: Tried to delete a null image!");
		/* Should we die here? */
		return;
	}
	LockMutex (image->mutex);

	TFB_DrawCanvas_Delete (image->NormalImg);
			
	if (image->ScaledImg)
	{
		TFB_DrawCanvas_Delete (image->ScaledImg);
		image->ScaledImg = 0;
	}

	if (image->FilledImg)
	{
		TFB_DrawCanvas_Delete (image->FilledImg);
		image->FilledImg = 0;
	}

	UnlockMutex (image->mutex);
	DestroyMutex (image->mutex);
			
	HFree (image);
}

void
TFB_DrawImage_FixScaling (TFB_Image *image, int target, int type)
{
	if (image->dirty || !image->ScaledImg ||
			target != image->last_scale ||
			type != image->last_scale_type)
	{
		image->dirty = FALSE;
		image->ScaledImg = TFB_DrawCanvas_New_ScaleTarget (image->NormalImg,
			image->ScaledImg, type, image->last_scale_type);
		
		if (type == TFB_SCALE_NEAREST)
			TFB_DrawCanvas_Rescale_Nearest (image->NormalImg,
					image->ScaledImg, target, &image->NormalHs,
					&image->extent, &image->last_scale_hs);
		else if (type == TFB_SCALE_BILINEAR)
			TFB_DrawCanvas_Rescale_Bilinear (image->NormalImg,
					image->ScaledImg, target, &image->NormalHs,
					&image->extent, &image->last_scale_hs);
		else
			TFB_DrawCanvas_Rescale_Trilinear (image->NormalImg,
					image->MipmapImg, image->ScaledImg, target,
					&image->NormalHs, &image->MipmapHs,
					&image->extent, &image->last_scale_hs);

		image->last_scale_type = type;
		image->last_scale = target;
	}
}

BOOLEAN
TFB_DrawImage_Intersect (TFB_Image *img1, POINT img1org,
		TFB_Image *img2, POINT img2org, const RECT *interRect)
{
	BOOLEAN ret;

	LockMutex (img1->mutex);
	LockMutex (img2->mutex);
	ret = TFB_DrawCanvas_Intersect (img1->NormalImg, img1org,
			img2->NormalImg, img2org, interRect);
	UnlockMutex (img2->mutex);
	UnlockMutex (img1->mutex);

	return ret;
}

void
TFB_DrawImage_CopyRect (TFB_Image *source, const RECT *srcRect,
		TFB_Image *target, POINT dstPt)
{
	LockMutex (source->mutex);
	LockMutex (target->mutex);
	TFB_DrawCanvas_CopyRect (source->NormalImg, srcRect,
			target->NormalImg, dstPt);
	target->dirty = TRUE;
	UnlockMutex (target->mutex);
	UnlockMutex (source->mutex);
}
