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

// JMS 2011: Added DPOINT type - a coordinate point with larger values to avoid overflows in hires modes.

#ifndef LIBS_GFXLIB_H_
#define LIBS_GFXLIB_H_

#include "port.h"
#include "libs/compiler.h"
#include <math.h>

typedef struct Color Color;
struct Color {
	BYTE r;
	BYTE g;
	BYTE b;
	BYTE a;
};

#include "libs/reslib.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct context_desc CONTEXT_DESC;
typedef struct frame_desc FRAME_DESC;
typedef struct font_desc FONT_DESC;
typedef struct drawable_desc DRAWABLE_DESC;

typedef CONTEXT_DESC *CONTEXT;
typedef FRAME_DESC *FRAME;
typedef FONT_DESC *FONT;
typedef DRAWABLE_DESC *DRAWABLE;

typedef UWORD TIME_VALUE;

#define TIME_SHIFT 8
#define MAX_TIME_VALUE ((1 << TIME_SHIFT) + 1)

typedef SWORD COORD;

static inline bool
sameColor(Color c1, Color c2)
{
	return c1.r == c2.r &&
			c1.g == c2.g &&
			c1.b == c2.b &&
			c1.a == c2.a;
}

static inline bool
sameColor24(Color c1, Color c2)
{
	return c1.r == c2.r &&
		c1.g == c2.g &&
		c1.b == c2.b;
}

// Transform a 5-bits color component to an 8-bits color component.
// Form 1, calculates '(r5 / 31.0) * 255.0, highest value is 0xff:
#define CC5TO8(c) (((c) << 3) | ((c) >> 2))
// Form 2, calculates '(r5 / 32.0) * 256.0, highest value is 0xf8:
//#define CC5TO8(c) ((c) << 3)

#define BUILD_COLOR(col, c256) col
		// BUILD_COLOR used to combine a 15-bit RGB color tripple with a
		// destination VGA palette index into a 32-bit value.
		// Now, it is an empty wrapper which returns the first argument,
		// which is of type Color, and ignores the second argument,
		// the palette index.
		//
		// It is a remnant of 8bpp hardware paletted display (VGA).
		// The palette index would be overwritten with the RGB value
		// and the drawing op would use this index on screen.
		// The palette indices 0-15, as used in DOS SC2, are unchanged
		// from the standard VGA palette and are identical to 16-color EGA.
		// Various frames, borders, menus, etc. frequently refer to these
		// first 16 colors and normally do not change the RGB values from
		// the standard ones (see colors.h; most likely unchanged from SC1)
		// The palette index is meaningless in UQM for the most part.
		// New code should just use index 0.

// Turn a 15 bits color into a 24-bits color.
// r, g, and b are each 5-bits color components.
static inline Color
colorFromRgb15 (BYTE r, BYTE g, BYTE b)
{
	Color c;
	c.r = CC5TO8 (r);
	c.g = CC5TO8 (g);
	c.b = CC5TO8 (b);
	c.a = 0xff;

	return c;
}
#define MAKE_RGB15(r, g, b) colorFromRgb15 ((r), (g), (b))

#ifdef NOTYET  /* Need C'99 support */
#define MAKE_RGB15(r, g, b) (Color) { \
		.r = CC5TO8 (r), \
		.g = CC5TO8 (g), \
		.b = CC5TO8 (b), \
		.a = 0xff \
	}
#endif

// Temporary, until we can use C'99 features. Then MAKE_RGB15 will be usable
// anywhere.
// This define is intended for global initialisations, where the
// expression must be constant.
#define MAKE_RGB15_INIT(r, g, b) { \
		CC5TO8 (r), \
		CC5TO8 (g), \
		CC5TO8 (b), \
		0xff \
	}

// This define is intended for global initialisations, where the
// expression must be constant.
#define MAKE_RGBA_INIT(r, g, b, a) { \
		(r), \
		(g), \
		(b), \
		(a) \
	}

static inline Color
buildColorRgba (BYTE r, BYTE g, BYTE b, BYTE a)
{
	Color c;
	c.r = r;
	c.g = g;
	c.b = b;
	c.a = a;

	return c;
}
#define BUILD_COLOR_RGBA(r, g, b, a) \
		buildColorRgba ((r), (g), (b), (a))

#define BUILD_SHADE_RGBA(s) \
		buildColorRgba ((s), (s), (s), 0xFF)

#define BUILD_COLOR_RGB(r, g, b) \
		buildColorRgba ((r), (g), (b), 0xFF)

static inline void
IncreaseBrightness (BYTE *ch, BYTE value)
{
	int c;
	if (*ch < 128)
		c = ((*ch * 255) >> 7);
	else
		c = (((*ch + 255) << 1) - 255 - ((255 * *ch) >> 7));

	if (c > 0xFF)
		c = 0xFF;

	if (value == 0xFF)
		*ch = (BYTE)c;
	else
		*ch = (BYTE)(((c - *ch) * value) >> 8) + *ch;
}

static inline BOOLEAN
AreTheyShades (Color first_color, Color second_color)
{
	return ((first_color.r == first_color.g
			&& first_color.g == first_color.b)
			&& (second_color.r == second_color.g
			&& second_color.g == second_color.b));
}

static inline Color
CreateAvgShade (Color first_color, Color second_color)
{
	Color temp;

	temp = buildColorRgba (0, 0, 0, 0);

	if (first_color.r > second_color.r)
	{
		temp.r = first_color.r - second_color.r;
		temp.g = temp.r;
		temp.b = temp.r;
	}
	
	if (first_color.r < second_color.r)
	{
		temp.r = second_color.r - first_color.r;
		temp.g = temp.r;
		temp.b = temp.r;
	}

	if (sameColor (first_color, second_color))
		return first_color;

	if (temp.r == first_color.r || temp.r == second_color.r)
	{
		temp.r = (first_color.r + second_color.r) >> 1;
		temp.g = temp.r;
		temp.b = temp.r;
	}

	if (temp.r > 0)
		temp.a = 255;

	return temp;
}


typedef BYTE CREATE_FLAGS;
// WANT_MASK is deprecated (and non-functional). It used to generate a bitmap
// of changed pixels for a target DRAWABLE, so that DRAW_SUBTRACTIVE could
// paint background pixels over them, i.e. a revert draw. The backgrounds
// are fully erased now instead.
#define WANT_MASK (CREATE_FLAGS)(1 << 0)
#define WANT_PIXMAP (CREATE_FLAGS)(1 << 1)
// MAPPED_TO_DISPLAY is deprecated but still checked by LoadDisplayPixmap().
// Its former use was to indicate a pre-scaled graphic for the display.
#define MAPPED_TO_DISPLAY (CREATE_FLAGS)(1 << 2)
#define WANT_ALPHA (CREATE_FLAGS)(1 << 3)

typedef struct extent
{
	COORD width, height;
} EXTENT;

// JMS: Extent with larger values to avoid overflows in hires modes.
typedef struct dextent
{
	SDWORD width, height;
} DEXTENT;

typedef struct point
{
	COORD x, y;
} POINT;

// JMS: coordinate point with larger values to avoid overflows in hires modes.
typedef struct dpoint
{
	SDWORD x, y;
} DPOINT;

typedef struct stamp
{
	POINT origin;
	FRAME frame;
} STAMP;

typedef struct rect
{
	POINT corner;
	EXTENT extent;
} RECT;

// Kruzen: Thanks to JMS, using this to draw ovals
// Overflows happen in HD with Starmap and max zoom (Fuel circles)
typedef struct drect
{
	DPOINT corner;
	DEXTENT extent;
} DRECT;

typedef struct line
{
	POINT first, second;
} LINE;

static inline POINT
MAKE_POINT (COORD x, COORD y)
{
	POINT pt = {x, y};
	return pt;
}

static inline DPOINT
MAKE_DPOINT (SDWORD x, SDWORD y)
{
	DPOINT pt = { x, y };
	return pt;
}

static inline EXTENT
MAKE_EXTENT (COORD width, COORD height)
{
	EXTENT ext = {width, height};
	return ext;
}

static inline DEXTENT
MAKE_DEXTENT (SDWORD width, SDWORD height)
{
	DEXTENT ext = {width, height};
	return ext;
}

// Kruzen: Some DrawOval() calls still use standard rect where overflow is impossible as it's 2 figures away from that
// Used to draw SOI and planet orbits
// To avoid any typedef conflicts - transform standard RECT to DRECT
static inline DRECT
RECT_TO_DRECT (RECT r)
{
	DRECT dr = { {r.corner.x, r.corner.y}, {r.extent.width, r.extent.height} };

	return dr;
}

//static inline void
//MAKE_LINE (LINE *line, int x1, int y1, int x2, int y2)
//{
//	line->first.x = x1;
//	line->first.y = y1;
//	line->second.x = x2;
//	line->second.y = y2;
//}

static inline void
MAKE_LINE (LINE *line, POINT first, POINT second)
{
	line->first = first;
	line->second = second;
}

static inline bool
pointsEqual (POINT p1, POINT p2)
{
	return p1.x == p2.x && p1.y == p2.y;
}

static inline bool
extentsEqual (EXTENT e1, EXTENT e2)
{
	return e1.width == e2.width && e1.height == e2.height;
}

static inline bool
rectsEqual (RECT r1, RECT r2)
{
	return pointsEqual (r1.corner, r2.corner)
			&& extentsEqual (r1.extent, r2.extent);
}

static inline bool
pointWithinRect (RECT r, POINT p)
{
	return p.x >= r.corner.x && p.y >= r.corner.y
			&& p.x < r.corner.x + r.extent.width
			&& p.y < r.corner.y + r.extent.height;
}

static inline double
ptDistance (POINT p1, POINT p2)
{
	return (sqrt (pow ((double)p2.x - (double)p1.x, 2)
			+ pow ((double)p2.y - (double)p1.y, 2)));
}

static inline double
calcDistance (COORD x1, COORD y1, COORD x2, COORD y2)
{
	double dx = (double)x2 - (double)x1;
	double dy = (double)y2 - (double)y1;

	return (sqrt (pow (dx, 2) + pow (dy, 2)));
}

static inline void
printPt (POINT pt, UNICODE *Str)
{
	printf ("%s = %d x %d\n", Str, pt.x, pt.y);
}

static inline void
printDPt (DPOINT dPt, UNICODE *Str)
{
	printf ("%s = %d x %d\n", Str, dPt.x, dPt.y);
}

static inline void
printExt (EXTENT ext, UNICODE *Str)
{
	printf ("%s = %d x %d\n", Str, ext.width, ext.height);
}

static inline void
printDExt (DEXTENT dExt, UNICODE *Str)
{
	printf ("%s = %d x %d\n", Str, dExt.width, dExt.height);
}

static inline void
printRect (RECT r, UNICODE *Str)
{
	printf ("%s.corner = %d x %d\n", Str, r.corner.x, r.corner.y);
	printf ("%s.extent = %d x %d\n", Str, r.extent.width, r.extent.height);
}

static inline void
ZeroPoint (POINT *pt)
{
	pt->x = pt->y = ~0;
}

static inline BOOLEAN
ValidPoint (POINT pt)
{
	return (pt.x != ~0 && pt.y != ~0);
}

typedef enum
{
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT
} TEXT_ALIGN;

typedef enum
{
	VALIGN_TOP,
	VALIGN_MIDDLE,
	VALIGN_BOTTOM
} TEXT_VALIGN;

typedef struct text
{
	POINT baseline;
	const UNICODE *pStr;
	TEXT_ALIGN align;
	COUNT CharCount;
} TEXT;

#if defined(__cplusplus)
}
#endif

#include "libs/strlib.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef STRING_TABLE COLORMAP_REF;
typedef STRING COLORMAP;
// COLORMAPPTR is really a pointer to colortable entry structure
// which is documented in doc/devel/strtab, .ct files section
typedef void *COLORMAPPTR;

#include "graphics/prim.h"


typedef BYTE BATCH_FLAGS;
// This flag is currently unused but it might make sense to restore it
#define BATCH_BUILD_PAGE (BATCH_FLAGS)(1 << 0)

typedef struct
{
	TIME_VALUE last_time_val;
	POINT EndPoint;
	STAMP IntersectStamp;
} INTERSECT_CONTROL;

typedef BYTE INTERSECT_CODE;

#define INTERSECT_LEFT (INTERSECT_CODE)(1 << 0)
#define INTERSECT_TOP (INTERSECT_CODE)(1 << 1)
#define INTERSECT_RIGHT (INTERSECT_CODE)(1 << 2)
#define INTERSECT_BOTTOM (INTERSECT_CODE)(1 << 3)
#define INTERSECT_NOCLIP (INTERSECT_CODE)(1 << 7)
#define INTERSECT_ALL_SIDES (INTERSECT_CODE)(INTERSECT_LEFT | \
								 INTERSECT_TOP | \
								 INTERSECT_RIGHT | \
								 INTERSECT_BOTTOM)

typedef POINT HOT_SPOT;

extern HOT_SPOT MAKE_HOT_SPOT (COORD, COORD);

extern INTERSECT_CODE BoxIntersect (RECT *pr1, RECT *pr2, RECT *printer);
extern void BoxUnion (RECT *pr1, RECT *pr2, RECT *punion);

typedef enum
{
	FadeAllToWhite = 250,
	FadeSomeToWhite,
	FadeAllToBlack,
	FadeAllToColor,
	FadeSomeToBlack,
	FadeSomeToColor
} ScreenFadeType;

typedef enum
{
	DRAW_REPLACE = 0,
			// Pixels in the target FRAME are replaced entirely.
			// Non-stamp primitives with Color.a < 255 to RGB targets are
			// equivalent to DRAW_ALPHA with (DrawMode.factor = Color.a),
			// except the Text primitives.
			// DrawMode.factor: ignored
			// Text: supported (except DRAW_ALPHA via Color.a)
			// RGBA sources (WANT_ALPHA): per-pixel alpha blending performed
			// RGBA targets (WANT_ALPHA): replace directly supported
	DRAW_ADDITIVE,
			// Pixel channels of the source FRAME or Color channels of
			// a primitive are modulated by (DrawMode.factor / 255) and added
			// to the pixel channels of the target FRAME.
			// DrawMode.factor range: -32767..32767 (negative values make
			//    draw subtractive); 255 = 1:1 ratio
			// Text: not yet supported
			// RGBA sources (WANT_ALPHA): alpha channel ignored
			// RGBA targets (WANT_ALPHA): not yet supported
	DRAW_ALPHA,
			// Pixel channels of the source FRAME or Color channels of
			// a primitive are modulated by (DrawMode.factor / 255) and added
			// to the pixel channels of the target FRAME, modulated by
			// (1 - DrawMode.factor / 255)
			// DrawMode.factor range: 0..255; 255 = fully opaque
			// Text: supported
			// RGBA sources (WANT_ALPHA): alpha channel ignored
			// RGBA targets (WANT_ALPHA): not yet supported
	DRAW_MULTIPLY,
			// Pixel channels of the source FRAME or Color channels of
			// a primitive are modulated by (channel / 255) and multiplied
			// by pixel channels of the target FRAME, modulated by
			// (channel / 255)
			// DrawMode.factor range: not yet supported
			// Text: supported
			// RGBA sources (WANT_ALPHA): alpha channel ignored
			// RGBA targets (WANT_ALPHA): not yet supported
	DRAW_OVERLAY,
	DRAW_SCREEN,
	DRAW_GRAYSCALE, // To get true grayscale - factor should be == 128
	DRAW_LINEARBURN,
	DRAW_HYPTOQUAS,

	DRAW_DEFAULT = DRAW_REPLACE,
} DrawKind;

typedef struct
{
	BYTE kind;
	SWORD factor;
} DrawMode;

#define DRAW_REPLACE_MODE   MAKE_DRAW_MODE (DRAW_REPLACE, 0)
#define DRAW_FACTOR_1       0xff
#define TRANSFER_ALPHA      0x7fff

static inline DrawMode
MAKE_DRAW_MODE (DrawKind kind, SWORD factor)
{
	DrawMode mode;
	mode.kind = kind;
	mode.factor = factor;
	return mode;
}

typedef enum
{
	NORTH_SHADOW,
	NORTH_EAST_SHADOW,
	EAST_SHADOW,
	SOUTH_EAST_SHADOW,
	SOUTH_SHADOW,
	SOUTH_WEST_SHADOW,
	WEST_SHADOW,
	NORTH_WEST_SHADOW,
} SHADOW_ANGLE;

extern CONTEXT SetContext (CONTEXT Context);
extern Color SetContextForeGroundColor (Color Color);
extern Color GetContextForeGroundColor (void);
extern Color SetContextBackGroundColor (Color Color);
extern Color GetContextBackGroundColor (void);
extern FRAME SetContextFGFrame (FRAME Frame);
extern FRAME GetContextFGFrame (void);
// Context cliprect defines the drawing bounds. Additionally, all
// drawing positions (x,y) are relative to the cliprect corner.
extern BOOLEAN SetContextClipRect (RECT *pRect);
// The returned rect is always filled in. If the context cliprect
// is undefined, the returned rect has foreground frame dimensions.
extern BOOLEAN GetContextClipRect (RECT *pRect);
// The actual origin will be orgOffset + context ClipRect.corner
extern POINT SetContextOrigin (POINT orgOffset);
extern DrawMode SetContextDrawMode (DrawMode);
extern DrawMode GetContextDrawMode (void);
// 'area' may be NULL to copy the entire CONTEXT cliprect
// 'area' is relative to the CONTEXT cliprect
extern DRAWABLE CopyContextRect (const RECT* area);

extern TIME_VALUE DrawablesIntersect (INTERSECT_CONTROL *pControl0,
		INTERSECT_CONTROL *pControl1, TIME_VALUE max_time_val);
extern void DrawStamp (STAMP *pStamp);
extern void DrawFilledStamp (STAMP *pStamp);
extern void DrawPoint (POINT *pPoint);
extern void DrawRectangle (RECT *pRect, BOOLEAN scaled);
extern void DrawFilledRectangle (RECT *pRect);
extern void DrawLine (LINE *pLine, BYTE thickness);
extern void ApplyMask (FRAME layer, FRAME base, DrawMode mode, Color *fill);
extern void InstaPoint (int x, int y);
extern void InstaRect (int x, int y, int w, int h, BOOLEAN scaled);
extern void InstaFilledRect (int x, int y, int w, int h);
extern void InstaLine (int x1, int y1, int x2, int y2);
extern RECT font_GetTextRect (TEXT* pText);
extern void font_DrawText (TEXT *pText);
extern void font_DrawText_Fade (TEXT *lpText, FRAME repair, BOOLEAN *skip);
extern void font_DrawTracedText (TEXT *pText, Color text, Color trace);
extern BYTE font_DrawTextAlt (TEXT *lpText, BYTE swap, FONT AltFontPtr, UniChar key);
extern void font_DrawTracedTextAlt (TEXT* pText, Color text, Color trace, FONT AltFontPtr,
		UniChar key);
extern void font_DrawShadowedText (TEXT *pText, BYTE direction,
		Color text_color, Color shadow_color);
extern void DrawBatch (PRIMITIVE *pBasePrim, PRIM_LINKS PrimLinks,
		BATCH_FLAGS BatchFlags);
extern void BatchGraphics (void);
extern void UnbatchGraphics (void);
extern void FlushGraphics (void);
extern void ClearDrawable (void);
extern void ClearScreen (void);
#ifdef DEBUG
extern CONTEXT CreateContextAux (const char *name);
#define CreateContext(name) CreateContextAux((name))
#else  /* if !defined(DEBUG) */
extern CONTEXT CreateContextAux (void);
#define CreateContext(name) CreateContextAux()
#endif  /* !defined(DEBUG) */
extern BOOLEAN DestroyContext (CONTEXT ContextRef);
extern DRAWABLE CreateDisplay (CREATE_FLAGS CreateFlags, SIZE *pwidth,
		SIZE *pheight);
extern DRAWABLE CreateDrawable (CREATE_FLAGS CreateFlags, SIZE width,
		SIZE height, COUNT num_frames);
extern BOOLEAN DestroyDrawable (DRAWABLE Drawable);
extern BOOLEAN GetFrameRect (FRAME Frame, RECT *pRect);
#ifdef DEBUG
extern const char *GetContextName (CONTEXT context);
extern CONTEXT GetFirstContext (void);
extern CONTEXT GetNextContext (CONTEXT context);
extern size_t GetContextCount (void);
#endif  /* DEBUG */

extern HOT_SPOT SetFrameHot (FRAME Frame, HOT_SPOT HotSpot);
extern HOT_SPOT GetFrameHot (FRAME Frame);
extern BOOLEAN InstallGraphicResTypes (void);
extern DRAWABLE LoadGraphicFile (const char *pStr);
extern FONT LoadFontFile (const char *pStr);
extern void *LoadGraphicInstance (RESOURCE res);
extern DRAWABLE LoadDisplayPixmap (const RECT *area, FRAME frame);
extern FRAME SetContextFontEffect (FRAME EffectFrame);
extern FONT SetContextFont (FONT Font);
extern BOOLEAN DestroyFont (FONT FontRef);
// The returned pRect is relative to the context drawing origin
extern BOOLEAN TextRect (TEXT *pText, RECT *pRect, BYTE *pdelta);
extern BOOLEAN TextRectAlt (TEXT *lpText, RECT *pRect, BYTE *pdelta, BYTE swap, UniChar key, FONT AltFontPtr);
extern BOOLEAN GetContextFontLeading (SIZE *pheight);
extern BOOLEAN GetContextFontDispHeight (SIZE *pheight);
extern BOOLEAN GetContextFontDispWidth (SIZE *pwidth);
extern COUNT GetFrameCount (FRAME Frame);
extern COUNT GetFrameIndex (FRAME Frame);
extern FRAME SetAbsFrameIndex (FRAME Frame, COUNT FrameIndex);
extern FRAME SetRelFrameIndex (FRAME Frame, SIZE FrameOffs);
extern FRAME SetEquFrameIndex (FRAME DstFrame, FRAME SrcFrame);
extern FRAME IncFrameIndex (FRAME Frame);
extern FRAME DecFrameIndex (FRAME Frame);
extern DRAWABLE CopyFrameRect (FRAME Frame, const RECT *area);
extern DRAWABLE CloneFrame (FRAME Frame);
extern DRAWABLE RotateFrame (FRAME Frame, int angle_deg);
extern DRAWABLE RescaleFrame (FRAME frame, int width, int height);
extern DRAWABLE RescalePercentage (FRAME frame, float percentage);
// This pair works for both paletted and trucolor frames
extern BOOLEAN ReadFramePixelColors (FRAME frame, Color *pixels,
		int width, int height);
extern BOOLEAN WriteFramePixelColors (FRAME frame, const Color *pixels,
		int width, int height);
// This pair only works for paletted frames
extern BOOLEAN ReadFramePixelIndexes (FRAME frame, BYTE *pixels,
		int width, int height, BOOLEAN paletted);
extern BOOLEAN WriteFramePixelIndexes (FRAME frame, const BYTE *pixels,
		int width, int height);
extern void SetFrameTransparentColor (FRAME, Color);
extern BOOLEAN IsFrameIndexed (FRAME Frame);

// If the frame is an active SCREEN_DRAWABLE, this call must be
// preceeded by FlushGraphics() for draw commands to have taken effect
extern Color GetFramePixel (FRAME, POINT pixelPt);

extern FRAME CaptureDrawable (DRAWABLE Drawable);
extern DRAWABLE ReleaseDrawable (FRAME Frame);

extern DRAWABLE GetFrameParentDrawable (FRAME Frame);

extern BOOLEAN SetColorMap (COLORMAPPTR ColorMapPtr);
extern DWORD XFormColorMap (COLORMAPPTR ColorMapPtr, SIZE TimeInterval);
extern DWORD FadeScreen (ScreenFadeType fadeType, SIZE TimeInterval);
extern void FlushColorXForms (void);
extern UBYTE GetColorMapTableIndex (COLORMAP map);
#define InitColorMapResources InitStringTableResources
#define LoadColorMapFile LoadStringTableFile
#define LoadColorMapInstance LoadStringTableInstance
#define CaptureColorMap CaptureStringTable
#define ReleaseColorMap ReleaseStringTable
#define DestroyColorMap DestroyStringTable
#define GetColorMapRef GetStringTable
#define GetColorMapCount GetStringTableCount
#define GetColorMapIndex GetColorMapTableIndex //Originally used GetStringTableIndex, but there were no macro calls and it didn't do what it's supposed to do
#define SetAbsColorMapIndex SetAbsStringTableIndex
#define SetRelColorMapIndex SetRelStringTableIndex
#define GetColorMapLength GetStringLengthBin
#define CheckColorMap CheckResString

extern COLORMAPPTR GetColorMapAddress (COLORMAP);

void SetSystemRect (const RECT *pRect);
void ClearSystemRect (void);

#if defined(__cplusplus)
}
#endif

#endif /* LIBS_GFXLIB_H_ */
