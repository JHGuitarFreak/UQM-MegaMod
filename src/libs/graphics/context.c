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

GRAPHICS_STATUS _GraphicsStatusFlags;
CONTEXT _pCurContext;

#ifdef DEBUG
// We keep track of all contexts
CONTEXT firstContext;
		// The first one in the list.
CONTEXT *contextEnd = &firstContext;
		// Where to put the next context.
#endif

PRIMITIVE _locPrim;

FONT _CurFontPtr;

#define DEFAULT_FORE_COLOR  BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)
#define DEFAULT_BACK_COLOR  BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00)

#define DEFAULT_DRAW_MODE   MAKE_DRAW_MODE (DRAW_DEFAULT, 255)

CONTEXT
SetContext (CONTEXT Context)
{
	CONTEXT LastContext;

	LastContext = _pCurContext;
	if (Context != LastContext)
	{
		if (LastContext)
		{
			UnsetContextFlags (
					MAKE_WORD (0, GRAPHICS_ACTIVE | DRAWABLE_ACTIVE));
			SetContextFlags (
					MAKE_WORD (0, _GraphicsStatusFlags
							& (GRAPHICS_ACTIVE | DRAWABLE_ACTIVE)));

			DeactivateContext ();
		}

		_pCurContext = Context;
		if (_pCurContext)
		{
			ActivateContext ();

			_GraphicsStatusFlags &= ~(GRAPHICS_ACTIVE | DRAWABLE_ACTIVE);
			_GraphicsStatusFlags |= HIBYTE (_get_context_flags ());

			SetPrimColor (&_locPrim, _get_context_fg_color ());

			_CurFramePtr = _get_context_fg_frame ();
			_CurFontPtr = _get_context_font ();
		}
	}

	return (LastContext);
}

#ifdef DEBUG
CONTEXT
CreateContextAux (const char *name)
#else  /* if !defined(DEBUG) */
CONTEXT
CreateContextAux (void)
#endif  /* !defined(DEBUG) */
{
	CONTEXT NewContext;

	NewContext = AllocContext ();
	if (NewContext)
	{
		/* initialize context */
#ifdef DEBUG
		NewContext->name = name;
		NewContext->next = NULL;
		*contextEnd = NewContext;
		contextEnd = &NewContext->next;
#endif  /* DEBUG */

		NewContext->Mode = DEFAULT_DRAW_MODE;
		NewContext->ForeGroundColor = DEFAULT_FORE_COLOR;
		NewContext->BackGroundColor = DEFAULT_BACK_COLOR;
	}

	return NewContext;
}

#ifdef DEBUG
// Loop through the list of context to the pointer which points to the
// specified context. This is either 'firstContext' or the address of
// the 'next' field of some other context.
static CONTEXT *
FindContextPtr (CONTEXT context) {
	CONTEXT *ptr;
	
	for (ptr = &firstContext; *ptr != NULL; ptr = &(*ptr)->next) {
		if (*ptr == context)
			break;
	}
	return ptr;
}
#endif  /* DEBUG */

BOOLEAN
DestroyContext (CONTEXT ContextRef)
{
	TFB_Image *img;

	if (ContextRef == 0)
		return (FALSE);

	if (_pCurContext && _pCurContext == ContextRef)
		SetContext ((CONTEXT)0);

#ifdef DEBUG
	// Unlink the context.
	{
		CONTEXT *contextPtr = FindContextPtr (ContextRef);
		if (contextEnd == &ContextRef->next)
			contextEnd = contextPtr;
		*contextPtr = ContextRef->next;
	}
#endif  /* DEBUG */

	img = ContextRef->FontBacking;
	if (img)
		TFB_DrawImage_Delete (img);

	FreeContext (ContextRef);
	return TRUE;
}

Color
SetContextForeGroundColor (Color color)
{
	Color oldColor;

	if (!ContextActive ())
		return DEFAULT_FORE_COLOR;

	oldColor = _get_context_fg_color ();
	if (!sameColor(oldColor, color))
	{
		SwitchContextForeGroundColor (color);

		if (!(_get_context_fbk_flags () & FBK_IMAGE))
		{
			SetContextFBkFlags (FBK_DIRTY);
		}
	}
	SetPrimColor (&_locPrim, color);

	return (oldColor);
}

Color
GetContextForeGroundColor (void)
{
	if (!ContextActive ())
		return DEFAULT_FORE_COLOR;

	return _get_context_fg_color ();
}

Color
SetContextBackGroundColor (Color color)
{
	Color oldColor;

	if (!ContextActive ())
		return DEFAULT_BACK_COLOR;

	oldColor = _get_context_bg_color ();
	if (!sameColor(oldColor, color))
		SwitchContextBackGroundColor (color);

	return oldColor;
}

Color
GetContextBackGroundColor (void)
{
	if (!ContextActive ())
		return DEFAULT_BACK_COLOR;

	return _get_context_bg_color ();
}

DrawMode
SetContextDrawMode (DrawMode mode)
{
	DrawMode oldMode;

	if (!ContextActive ())
		return DEFAULT_DRAW_MODE;

	oldMode = _get_context_draw_mode ();
	SwitchContextDrawMode (mode);

	return oldMode;
}

DrawMode
GetContextDrawMode (void)
{
	if (!ContextActive ())
		return DEFAULT_DRAW_MODE;

	return _get_context_draw_mode ();
}

// Returns a rect based at 0,0 and the size of context foreground frame
static inline RECT
_get_context_fg_rect (void)
{
	RECT r = { {0, 0}, {0, 0} };
	if (_CurFramePtr)
		r.extent = GetFrameBounds (_CurFramePtr);
	return r;
}

BOOLEAN
SetContextClipRect (RECT *lpRect)
{
	if (!ContextActive ())
		return (FALSE);

	if (lpRect)
	{
		if (rectsEqual (*lpRect, _get_context_fg_rect ()))
		{	// Cliprect is undefined to mirror GetContextClipRect()
			_pCurContext->ClipRect.extent.width = 0;
		}
		else
		{	// We have a cliprect
			_pCurContext->ClipRect = *lpRect;
		}
	}
	else
	{	// Set cliprect as undefined
		_pCurContext->ClipRect.extent.width = 0;
	}

	return TRUE;
}

BOOLEAN
GetContextClipRect (RECT *lpRect)
{
	if (!ContextActive ())
		return (FALSE);

	*lpRect = _pCurContext->ClipRect;
	if (!_pCurContext->ClipRect.extent.width)
	{	// Though the cliprect is undefined, drawing will be clipped
		// to the extent of the foreground frame
		*lpRect = _get_context_fg_rect ();
	}

	return (_pCurContext->ClipRect.extent.width != 0);
}

POINT
SetContextOrigin (POINT orgOffset)
{
	// XXX: This is a hack, kind of. But that's what the original did.
	return SetFrameHot (_CurFramePtr, orgOffset);
}

FRAME
SetContextFontEffect (FRAME EffectFrame)
{
	FRAME LastEffect;

	if (!ContextActive ())
		return (NULL);

	LastEffect = _get_context_fonteff ();
	if (EffectFrame != LastEffect)
	{
		SwitchContextFontEffect (EffectFrame);

		if (EffectFrame != 0)
		{
			SetContextFBkFlags (FBK_IMAGE);
		}
		else
		{
			UnsetContextFBkFlags (FBK_IMAGE);
		}
	}

	return LastEffect;
}

void
FixContextFontEffect (void)
{
	SIZE w, h;
	TFB_Image* img;

	if (!ContextActive () || (_get_context_font_backing () != 0
			&& !(_get_context_fbk_flags () & FBK_DIRTY)))
		return;

	if (!GetContextFontLeading (&h) || !GetContextFontLeadingWidth (&w))
		return;

	img = _pCurContext->FontBacking;
	if (img)
		TFB_DrawScreen_DeleteImage (img);

	img = TFB_DrawImage_CreateForScreen (w, h, TRUE);
	if (_get_context_fbk_flags () & FBK_IMAGE)
	{	// image pattern backing
		FRAME EffectFrame = _get_context_fonteff ();
		
		TFB_DrawImage_Image (EffectFrame->image,
				-EffectFrame->HotSpot.x, -EffectFrame->HotSpot.y,
				0, 0, NULL, DRAW_REPLACE_MODE, img);
	}
	else
	{	// solid color backing
		RECT r = { {0, 0}, {w, h} };
		Color color = _get_context_fg_color ();

		TFB_DrawImage_Rect (&r, color, DRAW_REPLACE_MODE, img);
	}
	
	_pCurContext->FontBacking = img;
	UnsetContextFBkFlags (FBK_DIRTY);
}

// 'area' may be NULL to copy the entire CONTEXT cliprect
// 'area' is relative to the CONTEXT cliprect
DRAWABLE
CopyContextRect (const RECT* area)
{
	RECT clipRect;
	RECT fgRect;
	RECT r;
	
	if (!ContextActive () || !_CurFramePtr)
		return NULL;

	fgRect = _get_context_fg_rect ();
	GetContextClipRect (&clipRect);
	r = clipRect;
	if (area)
	{	// a portion of the context
		r.corner.x += area->corner.x;
		r.corner.y += area->corner.y;
		r.extent = area->extent;
	}
	// TODO: Should this take CONTEXT origin into account too?
	// validate the rect
	if (!BoxIntersect (&r, &fgRect, &r))
		return NULL;
	
	if (_CurFramePtr->Type == SCREEN_DRAWABLE)
		return LoadDisplayPixmap (&r, NULL);
	else
		return CopyFrameRect (_CurFramePtr, &r);
}

#ifdef DEBUG
const char *
GetContextName (CONTEXT context)
{
	return context->name;
}

CONTEXT
GetFirstContext (void)
{
	return firstContext;
}

CONTEXT
GetNextContext (CONTEXT context)
{
	return context->next;
}
#endif  /* DEBUG */

