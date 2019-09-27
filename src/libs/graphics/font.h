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

#ifndef LIBS_GRAPHICS_FONT_H_
#define LIBS_GRAPHICS_FONT_H_

#include "libs/memlib.h"

#define MAX_DELTAS 100

typedef struct FontPage
{
	struct FontPage *next;
	UniChar pageStart;
#define CHARACTER_PAGE_MASK 0xfffff800
	UniChar firstChar;
	size_t numChars;
	TFB_Char *charDesc;
} FONT_PAGE;

static inline FONT_PAGE *
AllocFontPage (int numChars)
{
	FONT_PAGE *result = HMalloc (sizeof (FONT_PAGE));
	result->charDesc = HCalloc (numChars * sizeof *result->charDesc);
	return result;
}

static inline void
FreeFontPage (FONT_PAGE *page)
{
	HFree (page->charDesc);
	HFree (page);
}

struct font_desc
{
	UWORD Leading;
	UWORD LeadingWidth;
	FONT_PAGE *fontPages;
};

#define CHAR_DESCPTR PCHAR_DESC

#define FONT_PRIORITY DEFAULT_MEM_PRIORITY

#define AllocFont(size) (FONT)HCalloc (sizeof (FONT_DESC) + (size))
#define FreeFont _ReleaseFontData

extern FONT _CurFontPtr;

extern void *_GetFontData (uio_Stream *fp, DWORD length);
extern BOOLEAN _ReleaseFontData (void *handle);

#endif /* LIBS_GRAPHICS_FONT_H_ */

