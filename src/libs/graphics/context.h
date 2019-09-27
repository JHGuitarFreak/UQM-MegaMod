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

#ifndef LIBS_GRAPHICS_CONTEXT_H_
#define LIBS_GRAPHICS_CONTEXT_H_

#include "tfb_draw.h"
#include "libs/memlib.h"

typedef UWORD FBK_FLAGS;
#define FBK_DIRTY (1 << 0)
#define FBK_IMAGE (1 << 1)

struct context_desc
{
	UWORD Flags;
			// Low nibble currently unused
			// High nibble contains GRAPHICS_STATUS

	Color ForeGroundColor, BackGroundColor;
	DrawMode Mode;
	FRAME ForeGroundFrame;
	FONT Font;

	RECT ClipRect;

	FRAME FontEffect;
	TFB_Image *FontBacking;
	FBK_FLAGS BackingFlags;

#ifdef DEBUG
	const char *name;
	CONTEXT next;
#endif
};

#define AllocContext() HCalloc (sizeof (CONTEXT_DESC))
#define FreeContext HFree

extern CONTEXT _pCurContext;
extern PRIMITIVE _locPrim;

#define _get_context_fg_color() (_pCurContext->ForeGroundColor)
#define _get_context_bg_color() (_pCurContext->BackGroundColor)
#define _get_context_flags() (_pCurContext->Flags)
#define _get_context_fg_frame() (_pCurContext->ForeGroundFrame)
#define _get_context_font() (_pCurContext->Font)
#define _get_context_fbk_flags() (_pCurContext->BackingFlags)
#define _get_context_fonteff() (_pCurContext->FontEffect)
#define _get_context_font_backing() (_pCurContext->FontBacking)
#define _get_context_draw_mode() (_pCurContext->Mode)

#define SwitchContextDrawMode(m) \
{ \
	_pCurContext->Mode = (m); \
}
#define SwitchContextForeGroundColor(c) \
{ \
	_pCurContext->ForeGroundColor = (c); \
}
#define SwitchContextBackGroundColor(c) \
{ \
	_pCurContext->BackGroundColor = (c); \
}
#define SetContextFlags(f) \
{ \
	_pCurContext->Flags |= (f); \
}
#define UnsetContextFlags(f) \
{ \
	_pCurContext->Flags &= ~(f); \
}
#define SwitchContextFGFrame(f) \
{ \
	_pCurContext->ForeGroundFrame = (f); \
}
#define SwitchContextFont(f) \
{ \
	_pCurContext->Font = (f); \
	SetContextFBkFlags (FBK_DIRTY); \
}
#define SwitchContextBGFunc(f) \
{ \
	_pCurContext->BackGroundFunc = (f); \
}
#define SetContextFBkFlags(f) \
{ \
	_pCurContext->BackingFlags |= (f); \
}
#define UnsetContextFBkFlags(f) \
{ \
	_pCurContext->BackingFlags &= ~(f); \
}
#define SwitchContextFontEffect(f) \
{ \
	_pCurContext->FontEffect = (f); \
	SetContextFBkFlags (FBK_DIRTY); \
}

typedef BYTE GRAPHICS_STATUS;

extern GRAPHICS_STATUS _GraphicsStatusFlags;
#define GRAPHICS_ACTIVE (GRAPHICS_STATUS)(1 << 0)
#define GRAPHICS_VISIBLE (GRAPHICS_STATUS)(1 << 1)
#define CONTEXT_ACTIVE (GRAPHICS_STATUS)(1 << 2)
#define DRAWABLE_ACTIVE (GRAPHICS_STATUS)(1 << 3)
#define DeactivateGraphics() (_GraphicsStatusFlags &= ~GRAPHICS_ACTIVE)
#define ActivateGraphics() (_GraphicsStatusFlags |= GRAPHICS_ACTIVE)
#define GraphicsActive() (_GraphicsStatusFlags & GRAPHICS_ACTIVE)
#define DeactivateVisible() (_GraphicsStatusFlags &= ~GRAPHICS_VISIBLE)
#define ActivateVisible() (_GraphicsStatusFlags |= GRAPHICS_VISIBLE)
#define DeactivateContext() (_GraphicsStatusFlags &= ~CONTEXT_ACTIVE)
#define ActivateContext() (_GraphicsStatusFlags |= CONTEXT_ACTIVE)
#define ContextActive() (_GraphicsStatusFlags & CONTEXT_ACTIVE)
#define DeactivateDrawable() (_GraphicsStatusFlags &= ~DRAWABLE_ACTIVE)
#define ActivateDrawable() (_GraphicsStatusFlags |= DRAWABLE_ACTIVE)
#define DrawableActive() (_GraphicsStatusFlags & DRAWABLE_ACTIVE)

#define SYSTEM_ACTIVE (GRAPHICS_STATUS)(CONTEXT_ACTIVE | DRAWABLE_ACTIVE)

#define GraphicsSystemActive() \
		((_GraphicsStatusFlags & SYSTEM_ACTIVE) == SYSTEM_ACTIVE)
#define GraphicsStatus() \
		(_GraphicsStatusFlags & (GRAPHICS_STATUS)(GRAPHICS_ACTIVE \
							| GRAPHICS_VISIBLE))

// pValidRect or origin may be NULL
BOOLEAN GetContextValidRect (RECT *pValidRect, POINT *origin);
extern void FixContextFontEffect (void);

#endif /* LIBS_GRAPHICS_CONTEXT_H_ */

