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

#ifndef CMAP_H
#define CMAP_H

#include "libs/gfxlib.h"

#define MAX_COLORMAPS           250

// These are pertinent to colortable file format
// We load colormaps as binary and parse them when needed
#define PLUTVAL_BYTE_SIZE       3
// Channel order in colormap tables
#define PLUTVAL_RED             0
#define PLUTVAL_GREEN           1
#define PLUTVAL_BLUE            2

#define NUMBER_OF_PLUTVALS      256
// Size of the colormap in a colortable file
#define PLUT_BYTE_SIZE          (PLUTVAL_BYTE_SIZE * NUMBER_OF_PLUTVALS)

#define FADE_NO_INTENSITY      0
#define FADE_NORMAL_INTENSITY  255
#define FADE_FULL_INTENSITY    510

typedef struct NativePalette NativePalette;

typedef struct tfb_colormap
{
	int index;
			// Colormap index as the game sees it
	int version;
			// Version goes up every time the colormap changes. This may
			// be due to SetColorMap() or at every transformation step
			// of XFormColorMap(). Paletted TFB_Images track the last
			// colormap version they were drawn with for optimization.
	int refcount;
	struct tfb_colormap *next;
			// for spares linking
	NativePalette *palette;
} TFB_ColorMap;

extern int GetFadeAmount (void);

extern void InitColorMaps (void);
extern void UninitColorMaps (void);

extern void GetColorMapColors (Color *colors, TFB_ColorMap *);

extern TFB_ColorMap * TFB_GetColorMap (int index);
extern void TFB_ReturnColorMap (TFB_ColorMap *map);

extern BOOLEAN XFormColorMap_step (void);

// Native
NativePalette* AllocNativePalette (void);
void FreeNativePalette (NativePalette *);
void SetNativePaletteColor (NativePalette *, int index, Color);
Color GetNativePaletteColor (NativePalette *, int index);

#endif /* CMAP_H */
