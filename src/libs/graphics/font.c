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

#include <math.h>

#include "gfxintrn.h"
#include "tfb_prim.h"
#include "libs/log.h"
#include "uqm/units.h"

static inline TFB_Char *getCharFrame (FONT_DESC *fontPtr, UniChar ch);


FONT
SetContextFont (FONT Font)
{
	FONT LastFont;

	LastFont = _CurFontPtr;
	_CurFontPtr = Font;
	if (ContextActive ())
		SwitchContextFont (Font);

	return (LastFont);
}

BOOLEAN
DestroyFont (FONT FontRef)
{
	if (FontRef == NULL)
		return (FALSE);

	if (_CurFontPtr && _CurFontPtr == FontRef)
		SetContextFont ((FONT)NULL);

	return (FreeFont (FontRef));
}

// Returns the RECT of any given TEXT
RECT
font_GetTextRect (TEXT *lpText)
{
	RECT ClipRect;
	POINT origin;
	TEXT text;

	memset(&ClipRect, 0, sizeof(RECT));

	FixContextFontEffect ();
	if (!GraphicsSystemActive () || !GetContextValidRect (NULL, &origin))
		return ClipRect;

	// TextRect() clobbers TEXT.CharCount so we have to make a copy
	text = *lpText;
	if (!TextRect (&text, &ClipRect, NULL))
		return ClipRect;

	return ClipRect;
}

// XXX: Should be in frame.c (renamed to something decent?)
void
font_DrawText (TEXT *lpText)
{
	RECT ClipRect;
	POINT origin;
	TEXT text;

	FixContextFontEffect ();
	if (!GraphicsSystemActive () || !GetContextValidRect (NULL, &origin))
		return;

	// TextRect() clobbers TEXT.CharCount so we have to make a copy
	text = *lpText;
	if (!TextRect (&text, &ClipRect, NULL))
		return;
	// ClipRect is relative to origin
	_text_blt (&ClipRect, &text, origin);
}

/* Draw the stroke by drawing the same text in the
 * background color one pixel shifted to all 4 directions.
 */
void
font_DrawTracedText (TEXT *pText, Color text, Color trace)
{
	// Preserve current foreground color for full correctness
	const Color oldfg = SetContextForeGroundColor (trace);
	const BYTE stroke = RES_SCALE (1);
	const POINT t_baseline = pText->baseline;
	POINT offset;

	for (offset.x = -stroke; offset.x <= stroke; ++offset.x)
	{
		for (offset.y = -stroke; offset.y <= stroke; ++offset.y)
		{
			if (hypot (offset.x, offset.y) > stroke) continue;
			pText->baseline =
					MAKE_POINT (
							t_baseline.x + offset.x,
							t_baseline.y + offset.y
						);
			font_DrawText (pText);
		}
	}
	pText->baseline = t_baseline;

	SetContextForeGroundColor (text);
	font_DrawText (pText);
	SetContextForeGroundColor (oldfg);
}

// Alt stuff to handle 2 fonts at once (for Orz)
void
font_DrawTextAlt (TEXT* lpText, FONT AltFontPtr, UniChar key, BOOLEAN force)
{
	RECT ClipRect;
	POINT origin;
	TEXT text;

	FixContextFontEffect ();
	if (!GraphicsSystemActive () || !GetContextValidRect (NULL, &origin))
		return;

	// TextRect() clobbers TEXT.CharCount so we have to make a copy
	text = *lpText;
	if (!TextRect (&text, &ClipRect, NULL))
		return;
	// ClipRect is relative to origin
	_text_blt_alt (&ClipRect, &text, origin, AltFontPtr, key, force);
}

void
font_DrawTracedTextAlt (TEXT* pText, Color text, Color trace, FONT AltFontPtr, 
		UniChar key, BOOLEAN force)
{
	// Preserve current foreground color for full correctness
	const Color oldfg = SetContextForeGroundColor (trace);
	const BYTE stroke = RES_SCALE(1);
	const POINT t_baseline = pText->baseline;
	POINT offset;

	for (offset.x = -stroke; offset.x <= stroke; ++offset.x)
	{
		for (offset.y = -stroke; offset.y <= stroke; ++offset.y)
		{
			if (hypot (offset.x, offset.y) > stroke) continue;
			pText->baseline =
				MAKE_POINT (
					t_baseline.x + offset.x,
					t_baseline.y + offset.y
				);
			font_DrawTextAlt (pText, AltFontPtr, key, force);
		}
	}
	pText->baseline = t_baseline;

	SetContextForeGroundColor (text);
	font_DrawTextAlt (pText, AltFontPtr, key, force);
	SetContextForeGroundColor (oldfg);
}

BOOLEAN
GetContextFontLeading (SIZE *pheight)
{
	if (_CurFontPtr != 0)
	{
		*pheight = (SIZE)_CurFontPtr->Leading;
		return (TRUE);
	}

	*pheight = 0;
	return (FALSE);
}

BOOLEAN
GetContextFontDispHeight (SIZE *pheight)
{
	if (_CurFontPtr != 0)
	{
		*pheight = (SIZE)_CurFontPtr->disp.height;
		return (TRUE);
	}

	*pheight = 0;
	return (FALSE);
}

BOOLEAN
GetContextFontDispWidth (SIZE *pwidth)
{
	if (_CurFontPtr != 0)
	{
		*pwidth = (SIZE)_CurFontPtr->disp.width;
		return (TRUE);
	}

	*pwidth = 0;
	return (FALSE);
}

BOOLEAN
TextRect (TEXT *lpText, RECT *pRect, BYTE *pdelta)
{
	BYTE char_delta_array[MAX_DELTAS];
	FONT FontPtr;

	FontPtr = _CurFontPtr;
	if (FontPtr != 0 && lpText->CharCount != 0)
	{
		COORD top_y, bot_y;
		SIZE width;
		UniChar next_ch = 0;
		const char *pStr;
		COUNT num_chars;
	
		num_chars = lpText->CharCount;
		/* At this point lpText->CharCount contains the *maximum* number of
		 * characters that lpText->pStr may contain.
		 * After the while loop below, it will contain the actual number.
		 */
		if (pdelta == 0)
		{
			pdelta = char_delta_array;
			if (num_chars > MAX_DELTAS)
			{
				num_chars = MAX_DELTAS;
				lpText->CharCount = MAX_DELTAS;
			}
		}

		top_y = 0;
		bot_y = 0;
		width = 0;
		pStr = lpText->pStr;
		if (num_chars > 0)
		{
			next_ch = getCharFromString (&pStr);
			if (next_ch == '\0')
				num_chars = 0;
		}
		while (num_chars--)
		{
			UniChar ch;
			SIZE last_width;
			TFB_Char *charFrame;

			last_width = width;

			ch = next_ch;
			if (num_chars > 0)
			{
				next_ch = getCharFromString (&pStr);
				if (next_ch == '\0')
				{
					lpText->CharCount -= num_chars;
					num_chars = 0;
				}
			}

			charFrame = getCharFrame (FontPtr, ch);
			if (charFrame != NULL && charFrame->disp.width)
			{
				COORD y;

				y = -charFrame->HotSpot.y;
				if (y < top_y)
					top_y = y;
				y += charFrame->disp.height;
				if (y > bot_y)
					bot_y = y;

				width += charFrame->disp.width + FontPtr->CharSpace;

				if (num_chars && FontPtr->KernTab[ch] != NULL
						&& !(FontPtr->KernTab[ch]
						& (FontPtr->KernTab[next_ch] >> 2)))
					width -= FontPtr->KernAmount;
			}

			*pdelta++ = (BYTE)(width - last_width);
		}

		if (width > 0 && (bot_y -= top_y) > 0)
		{
			/* subtract off character spacing */
			if (pdelta[-1] > 0)
			{
				--pdelta[-1];
				width -= FontPtr->CharSpace;
			}

			if (lpText->align == ALIGN_LEFT)
				pRect->corner.x = 0;
			else if (lpText->align == ALIGN_CENTER)
				pRect->corner.x = -RES_SCALE ((RES_DESCALE (width) >> 1));
			else
				pRect->corner.x = -width;
			pRect->corner.y = top_y;
			pRect->extent.width = width;
			pRect->extent.height = bot_y;

			pRect->corner.x += lpText->baseline.x;
			pRect->corner.y += lpText->baseline.y;

			return (TRUE);
		}
	}

	pRect->corner = lpText->baseline;
	pRect->extent.width = 0;
	pRect->extent.height = 0;

	return (FALSE);
}

void
_text_blt (RECT *pClipRect, TEXT *TextPtr, POINT ctxOrigin)
{
	FONT FontPtr;
	COUNT num_chars;
	UniChar next_ch;
	const char *pStr;
	POINT origin;
	TFB_Image *backing;
	DrawMode mode = _get_context_draw_mode ();

	FontPtr = _CurFontPtr;
	if (FontPtr == NULL)
		return;
	backing = _get_context_font_backing ();
	if (!backing)
		return;
	
	origin.x = pClipRect->corner.x;
	origin.y = TextPtr->baseline.y;
	num_chars = TextPtr->CharCount;
	if (num_chars == 0)
		return;

	pStr = TextPtr->pStr;

	next_ch = getCharFromString (&pStr);
	if (next_ch == '\0')
		num_chars = 0;
	while (num_chars--)
	{
		UniChar ch;
		TFB_Char* fontChar;

		ch = next_ch;
		if (num_chars > 0)
		{
			next_ch = getCharFromString (&pStr);
			if (next_ch == '\0')
				num_chars = 0;
		}

		fontChar = getCharFrame (FontPtr, ch);
		if (fontChar != NULL && fontChar->disp.width)
		{
			RECT r;

			r.corner.x = origin.x - fontChar->HotSpot.x;
			r.corner.y = origin.y - fontChar->HotSpot.y;
			r.extent.width = fontChar->disp.width;
			r.extent.height = fontChar->disp.height;
			if (BoxIntersect (&r, pClipRect, &r))
			{
				TFB_Prim_FontChar (origin, fontChar, backing, mode,
						ctxOrigin);
			}

			origin.x += fontChar->disp.width + FontPtr->CharSpace;

			if (num_chars && FontPtr->KernTab[ch] != NULL
					&& !(FontPtr->KernTab[ch]
					& (FontPtr->KernTab[next_ch] >> 2)))
				origin.x -= FontPtr->KernAmount;
		}
	}
}

void
_text_blt_alt (RECT* pClipRect, TEXT* TextPtr, POINT ctxOrigin, FONT AltFontPtr, 
		UniChar key, BOOLEAN force)
{// Kruzen: To create text using 2 fonts (Orz case)
 // Safest way to do so without going too deep into
 // original code
 // Warning: GetTextRect doesn't quite work since it looks
 // for chars only in 1 font
	FONT FontPtr;
	COUNT num_chars;
	UniChar next_ch;
	const char* pStr;
	POINT origin;
	TFB_Image* backing;
	DrawMode mode = _get_context_draw_mode();
	BYTE swap = 0;

	FontPtr = _CurFontPtr;
	if (FontPtr == NULL)
		return;
	backing = _get_context_font_backing();
	if (!backing)
		return;

	origin.x = pClipRect->corner.x;
	origin.y = TextPtr->baseline.y;
	num_chars = TextPtr->CharCount;
	if (num_chars == 0)
		return;

	pStr = TextPtr->pStr;

	next_ch = getCharFromString(&pStr);
	if (next_ch == '\0')
		num_chars = 0;
	while (num_chars--)
	{
		UniChar ch;
		TFB_Char* fontChar;

		ch = next_ch;
		if (num_chars > 0)
		{
			next_ch = getCharFromString(&pStr);
			if (next_ch == '\0')
				num_chars = 0;
		}

		if (force) // use alt font no matter what
			fontChar = getCharFrame(AltFontPtr, ch);
		else if (ch == key)
		{
			fontChar = getCharFrame(AltFontPtr, ch);
			swap ^= 1; // switch current font
		}
		else
			fontChar = getCharFrame(swap ? AltFontPtr : FontPtr, ch);

		if (fontChar != NULL && fontChar->disp.width)
		{
			RECT r;

			r.corner.x = origin.x - fontChar->HotSpot.x;
			r.corner.y = origin.y - fontChar->HotSpot.y;
			r.extent.width = fontChar->disp.width;
			r.extent.height = fontChar->disp.height;
			if (BoxIntersect(&r, pClipRect, &r))
			{
				TFB_Prim_FontChar(origin, fontChar, backing, mode,
					ctxOrigin);
			}

			origin.x += fontChar->disp.width + FontPtr->CharSpace;

			if (num_chars && FontPtr->KernTab[ch] != NULL
				&& !(FontPtr->KernTab[ch]
					& (FontPtr->KernTab[next_ch] >> 2)))
				origin.x -= FontPtr->KernAmount;
		}
	}
}

static inline TFB_Char *
getCharFrame (FONT_DESC *fontPtr, UniChar ch)
{
	UniChar pageStart = ch & CHARACTER_PAGE_MASK;
	size_t charIndex;

	FONT_PAGE *page = fontPtr->fontPages;
	for (;;)
	{
		if (page == NULL)
			return NULL;

		if (page->pageStart == pageStart)
			break;

		page = page->next;
	}

	charIndex = ch - page->firstChar;
	if (ch >= page->firstChar && charIndex < page->numChars
			&& page->charDesc[charIndex].data)
	{
		return &page->charDesc[charIndex];
	}
	else
	{
		//log_add (log_Debug, "Character %u not present", (unsigned int) ch);
		return NULL;
	}
}

