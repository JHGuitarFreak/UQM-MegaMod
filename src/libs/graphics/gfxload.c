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

#include <string.h>
#include <stdio.h>

#include "options.h"
#include "port.h"
#include "libs/uio.h"
#include "libs/reslib.h"
		// for _cur_resfile_name
#include "libs/log.h"
#include "libs/memlib.h"
#include "libs/graphics/tfb_draw.h"
#include "libs/graphics/drawable.h"
#include "libs/graphics/font.h"


typedef struct anidata
{
	int transparent_color;
	int colormap_index;
	int hotspot_x;
	int hotspot_y;
} AniData;

extern uio_Repository *repository;
static uio_AutoMount *autoMount[] = { NULL };

static void
process_image (FRAME FramePtr, TFB_Canvas img[], AniData *ani, int cel_ct)
{
	TFB_Image *tfbimg;
	int hx, hy;

	FramePtr->Type = ROM_DRAWABLE;
	FramePtr->Index = cel_ct;

	// handle transparency cases
	if (TFB_DrawCanvas_IsPaletted (img[cel_ct]))
	{	// indexed color image
		if (ani[cel_ct].transparent_color >= 0)
		{
		    TFB_DrawCanvas_SetTransparentIndex (img[cel_ct],
					ani[cel_ct].transparent_color, FALSE);
		}
	}
	else
	{	// special transparency cases for truecolor images
		if (ani[cel_ct].transparent_color == 0)
		{	// make RGB=0,0,0 transparent
			Color color = {0, 0, 0, 0};
		    TFB_DrawCanvas_SetTransparentColor (img[cel_ct], color, FALSE);
		}
	}
	if (ani[cel_ct].transparent_color == -1)
	{	// enforce -1 to mean 'no transparency'
		TFB_DrawCanvas_SetTransparentIndex (img[cel_ct], -1, FALSE);
		// set transparent_color == -2 to use PNG tRNS transparency
	}
	
	hx = ani[cel_ct].hotspot_x;
	hy = ani[cel_ct].hotspot_y;

	FramePtr->image = TFB_DrawImage_New (img[cel_ct]);

	tfbimg = FramePtr->image;
	tfbimg->colormap_index = ani[cel_ct].colormap_index;
	img[cel_ct] = tfbimg->NormalImg;
	
	FramePtr->HotSpot = MAKE_HOT_SPOT (hx, hy);
	SetFrameBounds (FramePtr, tfbimg->extent.width, tfbimg->extent.height);

#ifdef CLIPDEBUG
	{
		/* for debugging clipping:
		   draws white (or most matching color from palette) pixels to
	       every corner of the image
		 */
		Color color = {0xff, 0xff, 0xff, 0xff};
		RECT r = {{0, 0}, {1, 1}};
		if (tfbimg->extent.width > 2 && tfbimg->extent.height > 2)
		{
			TFB_DrawImage_Rect (&r, color, tfbimg);
			r.corner.x = tfbimg->extent.width - 1;
			TFB_DrawImage_Rect (&r, color, tfbimg);
			r.corner.y = tfbimg->extent.height - 1;
			TFB_DrawImage_Rect (&r, color, tfbimg);
			r.corner.x = 0;
			TFB_DrawImage_Rect (&r, color, tfbimg);
		}
	}
#endif
}

static void
processFontChar (TFB_Char* CharPtr, TFB_Canvas canvas)
{
	BYTE* newdata;
	size_t dpitch;

	TFB_DrawCanvas_GetExtent (canvas, &CharPtr->extent);

	// Currently, each font char has its own separate data
	//  but that can change to common mem area
	dpitch = CharPtr->extent.width;
	newdata = HMalloc (dpitch * CharPtr->extent.height * sizeof (BYTE));
	TFB_DrawCanvas_GetFontCharData (canvas, newdata, dpitch);

	CharPtr->data = newdata;
	CharPtr->pitch = dpitch;
	CharPtr->disp.width = CharPtr->extent.width + 1;
	CharPtr->disp.height = CharPtr->extent.height + 1;
			// XXX: why the +1?
			// I brought it into this function from the only calling
			// function, but I don't know why it was there in the first
			// place.
			// XXX: the +1 appears to be for character and line spacing
			//  text_blt just adds the frame width to move to the next char
	
	{
		// This tunes the font positioning to be about what it should
		// TODO: prolly needs a little tweaking still

		int tune_amount = 0;

		if (CharPtr->extent.height == 8)
			tune_amount = -1;
		else if (CharPtr->extent.height == 9)
			tune_amount = -2;
		else if (CharPtr->extent.height > 9)
			tune_amount = -3;

		CharPtr->HotSpot = MAKE_HOT_SPOT (0,
				CharPtr->extent.height + tune_amount);
	}
}

void *
_GetCelData (uio_Stream *fp, DWORD length)
{
	int cel_total, cel_index, n;
	DWORD opos;
	char CurrentLine[1024], filename[PATH_MAX];
	TFB_Canvas *img;
	AniData *ani;
	DRAWABLE Drawable;
	uio_MountHandle *aniMount = 0;
	uio_DirHandle *aniDir = 0;
	uio_Stream *aniFile = 0;
	
	opos = uio_ftell (fp);

	{
		char *s1, *s2;
		char aniDirName[PATH_MAX];			
		const char *aniFileName;
		uint8 buf[4] = { 0, 0, 0, 0 };
		uint32 header;

		if (_cur_resfile_name == 0
				|| (((s2 = 0), (s1 = strrchr (_cur_resfile_name, '/')) == 0)
						&& (s2 = strrchr (_cur_resfile_name, '\\')) == 0))
		{
			n = 0;
		}
		else
		{
			if (s2 > s1)
				s1 = s2;
			n = s1 - _cur_resfile_name + 1;
		}

		uio_fread(buf, 4, 1, fp);
		header = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
 		if (_cur_resfile_name && header == 0x04034b50)
		{
			// zipped ani file
			if (n)
			{
				strncpy (aniDirName, _cur_resfile_name, n - 1);
				aniDirName[n - 1] = 0;
				aniFileName = _cur_resfile_name + n;
			}
			else
			{
				strcpy(aniDirName, ".");
				aniFileName = _cur_resfile_name;
			}
			aniDir = uio_openDir (repository, aniDirName, 0);
			aniMount = uio_mountDir (repository, aniDirName, uio_FSTYPE_ZIP,
							aniDir, aniFileName, "/", autoMount,
							uio_MOUNT_RDONLY | uio_MOUNT_TOP,
							NULL);
			aniFile = uio_fopen (aniDir, aniFileName, "r");
			opos = 0;
			n = 0;
		}
		else
		{
			// unpacked ani file
			strncpy (filename, _cur_resfile_name, n);
			aniFile = fp;
			aniDir = contentDir;
		}
	}

	cel_total = 0;
	uio_fseek (aniFile, opos, SEEK_SET);
	while (uio_fgets (CurrentLine, sizeof (CurrentLine), aniFile))
	{
		++cel_total;
	}

	img = HMalloc (sizeof (TFB_Canvas) * cel_total);
	ani = HMalloc (sizeof (AniData) * cel_total);
	if (!img || !ani)
	{
		log_add (log_Warning, "Couldn't allocate space for '%s'", _cur_resfile_name);
		if (aniMount)
		{
			uio_fclose(aniFile);
			uio_closeDir(aniDir);
			uio_unmountDir(aniMount);
		}
		HFree (img);
		HFree (ani);
		return NULL;
	}

	cel_index = 0;
	uio_fseek (aniFile, opos, SEEK_SET);
	while (uio_fgets (CurrentLine, sizeof (CurrentLine), aniFile) && cel_index < cel_total)
	{
		sscanf (CurrentLine, "%s %d %d %d %d", &filename[n], 
			&ani[cel_index].transparent_color, &ani[cel_index].colormap_index, 
			&ani[cel_index].hotspot_x, &ani[cel_index].hotspot_y);
	
		img[cel_index] = TFB_DrawCanvas_LoadFromFile (aniDir, filename);
		if (img[cel_index] == NULL)
		{
			const char *err;

			err = TFB_DrawCanvas_GetError ();
			log_add (log_Warning, "_GetCelData: Unable to load image!");
			if (err != NULL)
				log_add (log_Warning, "Gfx Driver reports: %s", err);
		}
		else
		{
			++cel_index;
		}

		if ((int)uio_ftell (aniFile) - (int)opos >= (int)length)
			break;
	}

	Drawable = NULL;
	if (cel_index && (Drawable = AllocDrawable (cel_index)))
	{
		if (!Drawable)
		{
			while (cel_index--)
				TFB_DrawCanvas_Delete (img[cel_index]);

			HFree (Drawable);
			Drawable = NULL;
		}
		else
		{
			FRAME FramePtr;

			Drawable->Flags = WANT_PIXMAP;
			Drawable->MaxIndex = cel_index - 1;

			FramePtr = &Drawable->Frame[cel_index];
			while (--FramePtr, cel_index--)
				process_image (FramePtr, img, ani, cel_index);
		}
	}

	if (Drawable == NULL)
		log_add (log_Warning, "Couldn't get cel data for '%s'",
				_cur_resfile_name);

	if (aniMount)
	{
		uio_fclose(aniFile);
		uio_closeDir(aniDir);
		uio_unmountDir(aniMount);
	}

	HFree (img);
	HFree (ani);
	return Drawable;
}

BOOLEAN
_ReleaseCelData (void *handle)
{
	DRAWABLE DrawablePtr;
	int cel_ct;
	FRAME FramePtr = NULL;

	if ((DrawablePtr = handle) == 0)
		return (FALSE);

	cel_ct = DrawablePtr->MaxIndex + 1;
	FramePtr = DrawablePtr->Frame;

	HFree (handle);
	if (FramePtr)
	{
		int i;
		for (i = 0; i < cel_ct; i++)
		{
			TFB_Image *img = FramePtr[i].image;
			if (img)
			{
				FramePtr[i].image = NULL;
				TFB_DrawScreen_DeleteImage (img);
			}
		}
		HFree (FramePtr);
	}

	return (TRUE);
}

typedef struct BuildCharDesc
{
	TFB_Canvas canvas;
	UniChar index;
} BuildCharDesc;

static int
compareBCDIndex (const void *arg1, const void *arg2)
{
	const BuildCharDesc *bcd1 = (const BuildCharDesc *) arg1;
	const BuildCharDesc *bcd2 = (const BuildCharDesc *) arg2;

	return (int) bcd1->index - (int) bcd2->index;
}

void *
_GetFontData (uio_Stream *fp, DWORD length)
{
	COUNT numDirEntries;
	DIRENTRY fontDir = NULL;
	BuildCharDesc *bcds = NULL;
	size_t numBCDs = 0;
	int dirEntryI;
	uio_DirHandle *fontDirHandle = NULL;
	uio_MountHandle *fontMount = NULL;
	FONT fontPtr = NULL;

	if (_cur_resfile_name == 0)
		goto err;

	if (fp != (uio_Stream*)~0)
	{
		// font is zipped instead of being in a directory

		char *s1, *s2;
		int n;
		const char *fontZipName;
		char fontDirName[PATH_MAX];

		if ((((s2 = 0), (s1 = strrchr (_cur_resfile_name, '/')) == 0)
						&& (s2 = strrchr (_cur_resfile_name, '\\')) == 0))
		{
			strcpy(fontDirName, ".");
			fontZipName = _cur_resfile_name;
		}
		else
		{
			if (s2 > s1)
				s1 = s2;
			n = s1 - _cur_resfile_name + 1;
			strncpy (fontDirName, _cur_resfile_name, n - 1);
			fontDirName[n - 1] = 0;
			fontZipName = _cur_resfile_name + n;
		}

		fontDirHandle = uio_openDir (repository, fontDirName, 0);
		fontMount = uio_mountDir (repository, _cur_resfile_name, uio_FSTYPE_ZIP,
						fontDirHandle, fontZipName, "/", autoMount,
						uio_MOUNT_RDONLY | uio_MOUNT_TOP,
						NULL);
		uio_closeDir (fontDirHandle);
	}

	fontDir = CaptureDirEntryTable (LoadDirEntryTable (contentDir,
			_cur_resfile_name, ".", match_MATCH_SUBSTRING));
	if (fontDir == 0)
		goto err;
	numDirEntries = GetDirEntryTableCount (fontDir);

	fontDirHandle = uio_openDirRelative (contentDir, _cur_resfile_name, 0);
	if (fontDirHandle == NULL)
		goto err;
		
	bcds = HMalloc (numDirEntries * sizeof (BuildCharDesc));
	if (bcds == NULL)
		goto err;

	// Load the surfaces for all dir Entries
	for (dirEntryI = 0; dirEntryI < numDirEntries; dirEntryI++)
	{
		char *char_name;
		unsigned int charIndex;
		TFB_Canvas canvas;
		EXTENT size;

		char_name = GetDirEntryAddress (SetAbsDirEntryTableIndex (
				fontDir, dirEntryI));
		if (sscanf (char_name, "%x.", &charIndex) != 1)
			continue;
			
		if (charIndex > 0xffff)
			continue;

		canvas = TFB_DrawCanvas_LoadFromFile (fontDirHandle, char_name);
		if (canvas == NULL)
			continue;

		TFB_DrawCanvas_GetExtent (canvas, &size);
		if (size.width == 0 || size.height == 0)
		{
			TFB_DrawCanvas_Delete (canvas);
			continue;
		}

		bcds[numBCDs].canvas = canvas;
		bcds[numBCDs].index = charIndex;
		numBCDs++;
	}
	uio_closeDir (fontDirHandle);
	DestroyDirEntryTable (ReleaseDirEntryTable (fontDir));
	if (fontMount != 0)
		uio_unmountDir(fontMount);

#if 0
	if (numBCDs == 0)
		goto err;
#endif

	// sort on the character index
	qsort (bcds, numBCDs, sizeof (BuildCharDesc), compareBCDIndex);

	fontPtr = AllocFont (0);
	if (fontPtr == NULL)
		goto err;
	
	fontPtr->Leading = 0;
	fontPtr->LeadingWidth = 0;

	{
		size_t startBCD = 0;
		UniChar pageStart;
		FONT_PAGE **pageEndPtr = &fontPtr->fontPages;
		while (startBCD < numBCDs)
		{
			// Process one character page.
			size_t endBCD;
			pageStart = bcds[startBCD].index & CHARACTER_PAGE_MASK;

			endBCD = startBCD;
			while (endBCD < numBCDs &&
					(bcds[endBCD].index & CHARACTER_PAGE_MASK) == pageStart)
				endBCD++;

			{
				size_t bcdI;
				int numChars = bcds[endBCD - 1].index + 1
						- bcds[startBCD].index;
				FONT_PAGE *page = AllocFontPage (numChars);
				page->pageStart = pageStart;
				page->firstChar = bcds[startBCD].index;
				page->numChars = numChars;
				*pageEndPtr = page;
				pageEndPtr = &page->next;

				for (bcdI = startBCD; bcdI < endBCD; bcdI++)
				{
					// Process one character.
					BuildCharDesc *bcd = &bcds[bcdI];
					TFB_Char *destChar =
							&page->charDesc[bcd->index - page->firstChar];
				
					if (destChar->data != NULL)
					{
						// There's already an image for this character.
						log_add (log_Debug, "Duplicate image for character %d "
								"for font %s.", (int) bcd->index,
								_cur_resfile_name);
						TFB_DrawCanvas_Delete (bcd->canvas);
						continue;
					}
					
					processFontChar (destChar, bcd->canvas);
					TFB_DrawCanvas_Delete (bcd->canvas);

					if (destChar->disp.height > fontPtr->Leading)
						fontPtr->Leading = destChar->disp.height;
					if (destChar->disp.width > fontPtr->LeadingWidth)
						fontPtr->LeadingWidth = destChar->disp.width;
				}
			}

			startBCD = endBCD;
		}
		*pageEndPtr = NULL;
	}

	fontPtr->Leading++;

	HFree (bcds);

	(void) fp;  /* Satisfying compiler (unused parameter) */
	(void) length;  /* Satisfying compiler (unused parameter) */
	return fontPtr;

err:
	if (fontPtr != 0)
		HFree (fontPtr);
	
	if (bcds != NULL)
	{
		size_t bcdI;
		for (bcdI = 0; bcdI < numBCDs; bcdI++)
			TFB_DrawCanvas_Delete (bcds[bcdI].canvas);
		HFree (bcds);
	}
	
	if (fontDirHandle != NULL)
		uio_closeDir (fontDirHandle);
	
	if (fontDir != 0)
		DestroyDirEntryTable (ReleaseDirEntryTable (fontDir));

	if (fontMount != 0)
		uio_unmountDir(fontMount);

	return 0;
}

BOOLEAN
_ReleaseFontData (void *handle)
{
	FONT font = (FONT) handle;
	if (font == NULL)
		return FALSE;

	{
		FONT_PAGE *page;
		FONT_PAGE *nextPage;

		for (page = font->fontPages; page != NULL; page = nextPage)
		{
			size_t charI;
			for (charI = 0; charI < page->numChars; charI++)
			{
				TFB_Char *c = &page->charDesc[charI];
				
				if (c->data == NULL)
					continue;
				
				// XXX: fix this if fonts get per-page data
				//  rather than per-char
				TFB_DrawScreen_DeleteData (c->data);
			}
		
			nextPage = page->next;
			FreeFontPage (page);
		}
	}

	HFree (font);

	return TRUE;
}
