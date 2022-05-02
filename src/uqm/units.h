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

#ifndef UQM_UNITS_H_
#define UQM_UNITS_H_

#include "libs/gfxlib.h"
#include "options.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern int ScreenWidth;
extern int ScreenHeight;

		/* Most basic resolution units. */
#define HD 2
#define SCREEN_WIDTH ScreenWidth
#define SCREEN_HEIGHT ScreenHeight
#define RESOLUTION_FACTOR resolutionFactor
#define IS_HD (RESOLUTION_FACTOR != HD ? FALSE : TRUE)
#define RES_SCALE(a) ((a) << RESOLUTION_FACTOR)
#define RES_DESCALE(a) ((a) >> RESOLUTION_FACTOR)
#define RES_BOOL(a,b) (!IS_HD ? (a) : (b))
#define RES_DBL(a) (RES_BOOL ((a), (a) * RESOLUTION_FACTOR))
#define RES_TRP(a) (RES_BOOL ((a), (a) * 3))
#define IF_HD(a) (RES_BOOL (0, (a)))

#define ASPECT_RATIO aspectRatio
#define IS_WS (ASPECT_RATIO != 1 ? FALSE : TRUE)

		/* Margins. */
#define SIS_ORG_X RES_SCALE (6)
#define SIS_ORG_Y RES_SCALE (9)

/* Status bar & play area sizes. */
#define STATUS_WIDTH RES_SCALE (64)
/* Width of the status "window" (the right part of the screen) */
#define STATUS_HEIGHT SCREEN_HEIGHT
/* Height of the status "window" (the right part of the screen) */
#define SPACE_WIDTH (SCREEN_WIDTH - STATUS_WIDTH)
/* Width of the space "window" (the left part of the screen) */
#define SPACE_HEIGHT SCREEN_HEIGHT
/* Height of the space "window" (the left part of the screen) */
#define SIS_SCREEN_WIDTH (SPACE_WIDTH - RES_SCALE (13))
/* Width of the usable part of the space "window" */
#define SIS_SCREEN_HEIGHT (SPACE_HEIGHT - RES_SCALE (13))
/* Height of the usable part of the space "window" */

#define ORIG_SIS_SCREEN_WIDTH (RES_DESCALE (SIS_SCREEN_WIDTH))
#define ORIG_SIS_SCREEN_HEIGHT (RES_DESCALE (SIS_SCREEN_HEIGHT))
#define PC_SIS_SCREEN_HEIGHT (187)
#define THREEDO_SIS_SCREEN_WIDTH (210)
#define THREEDO_SIS_SCREEN_HEIGHT (195)
#define HDMOD_SIS_SCREEN_WIDTH (1074)
#define HDMOD_SIS_SCREEN_HEIGHT (924)


		/* Radar. */
#define RADAR_X (RES_SCALE (4) + SPACE_WIDTH)
#define RADAR_WIDTH (STATUS_WIDTH - RES_SCALE (8))
#define RADAR_HEIGHT RES_SCALE (53)
#define RADAR_Y (SIS_ORG_Y + SIS_SCREEN_HEIGHT - RADAR_HEIGHT)

		/* Blue boxes which display messages and the green date box. */
#define SIS_TITLE_BOX_WIDTH    RES_SCALE (57)
#define SIS_TITLE_WIDTH        (SIS_TITLE_BOX_WIDTH - RES_SCALE (2))
#define SIS_TITLE_HEIGHT       RES_SCALE (8)
#define SIS_SPACER_BOX_WIDTH   RES_SCALE (12)

#define SIS_MESSAGE_BOX_WIDTH  (SIS_SCREEN_WIDTH - SIS_TITLE_BOX_WIDTH - SIS_SPACER_BOX_WIDTH)
#define SIS_MESSAGE_WIDTH      (SIS_MESSAGE_BOX_WIDTH - RES_SCALE (2))
#define SIS_MESSAGE_HEIGHT     SIS_TITLE_HEIGHT

#define STATUS_MESSAGE_WIDTH   (STATUS_WIDTH - RES_SCALE (4))
#define STATUS_MESSAGE_HEIGHT  RES_SCALE (7) 

#define SHIP_NAME_WIDTH        (STATUS_WIDTH - RES_SCALE (4))
#define SHIP_NAME_HEIGHT       RES_SCALE (7) 

		/* A lot of other shit. */
#define MAX_REDUCTION 3
#define MAX_VIS_REDUCTION 2
#define REDUCTION_SHIFT 1
#define NUM_VIEWS (MAX_VIS_REDUCTION + 1)

#define ZOOM_SHIFT 8
#define MAX_ZOOM_OUT (1 << (ZOOM_SHIFT + MAX_REDUCTION - 1))

#define ONE_SHIFT 2
#define BACKGROUND_SHIFT 3
#define SCALED_ONE (1 << ONE_SHIFT)
#define DISPLAY_TO_WORLD(x) ((x)<<ONE_SHIFT)
#define WORLD_TO_DISPLAY(x) ((x)>>ONE_SHIFT)

#define LOG_SPACE_WIDTH   (DISPLAY_TO_WORLD (SPACE_WIDTH) << MAX_REDUCTION)
#define LOG_SPACE_HEIGHT  (DISPLAY_TO_WORLD (SPACE_HEIGHT) << MAX_REDUCTION)
#define TRANSITION_WIDTH  (DISPLAY_TO_WORLD (SPACE_WIDTH) << MAX_VIS_REDUCTION)
#define TRANSITION_HEIGHT (DISPLAY_TO_WORLD (SPACE_HEIGHT) << MAX_VIS_REDUCTION)

// JMS_GFX: Changed from COORD to SDWORD and from COUNT to DWORD
#define DISPLAY_ALIGN(x) ((SDWORD)(x)&~(SCALED_ONE-1))
#define DISPLAY_ALIGN_X(x) ((SDWORD)((DWORD)(x)%LOG_SPACE_WIDTH)&~(SCALED_ONE-1))
#define DISPLAY_ALIGN_Y(y) ((SDWORD)((DWORD)(y)%LOG_SPACE_HEIGHT)&~(SCALED_ONE-1))

#define MAX_X_UNIVERSE 9999
#define MAX_Y_UNIVERSE 9999
#define MAX_X_LOGICAL \
((UNIVERSE_TO_LOGX (MAX_X_UNIVERSE + 1) > UNIVERSE_TO_LOGX (-1) ? \
UNIVERSE_TO_LOGX (MAX_X_UNIVERSE + 1) : UNIVERSE_TO_LOGX (-1)) - 1L)
#define MAX_Y_LOGICAL \
((UNIVERSE_TO_LOGY (MAX_Y_UNIVERSE + 1) > UNIVERSE_TO_LOGY (-1) ? \
UNIVERSE_TO_LOGY (MAX_Y_UNIVERSE + 1) : UNIVERSE_TO_LOGY (-1)) - 1L)

#define SPHERE_RADIUS_INCREMENT 11
#define MAX_FLEET_STRENGTH (254 * SPHERE_RADIUS_INCREMENT)

// XXX: These corrected for the weird screen aspect ratio on DOS
//   In part because of them, hyperflight is slower vertically
#define UNIT_SCREEN_WIDTH RES_SCALE (63)
#define UNIT_SCREEN_HEIGHT RES_SCALE (50)


// Bug #945: Simplified, these set the speed of SIS in Hyperspace and
//   Quasispace. The ratio between UNIVERSE_UNITS_ and LOG_UNITS_ is
//   what sets the speed, and it should be 1:16 to match the original.
//   The unit factors are reduced to keep the translation math within
//   32 bits. The original math is unnecessarily complex and depends
//   on the screen resolution when it should not.
//   Using the new math will break old savegames.

#define UNIVERSE_UNITS_X (((MAX_X_UNIVERSE + 1) >> 4))
#define UNIVERSE_UNITS_Y (((MAX_Y_UNIVERSE + 1) >> 4))
#define LOG_UNITS_X      ((SDWORD)(UNIVERSE_UNITS_X * RES_SCALE (16))) 
#define LOG_UNITS_Y      ((SDWORD)(UNIVERSE_UNITS_Y * RES_SCALE (16))) 

// Original (and now broken) Hyperspace speed factors
// Now being utilized to load Vanilla saves properly
#define SECTOR_WIDTH 195
#define SECTOR_HEIGHT 25

#define OLD_LOG_UNITS_X      ((SDWORD)(8192 >> 4) * SECTOR_WIDTH)
#define OLD_LOG_UNITS_Y      ((SDWORD)(7680 >> 4) * SECTOR_HEIGHT)
#define OLD_UNIVERSE_UNITS_X (((MAX_X_UNIVERSE + 1) >> 4) * 10)
#define OLD_UNIVERSE_UNITS_Y (((MAX_Y_UNIVERSE + 1) >> 4))


#define ROUNDING_ERROR(div)  ((div) >> 1)

static inline SDWORD
logxToUniverse (SDWORD lx)
{
	return (SDWORD) ((lx * UNIVERSE_UNITS_X + ROUNDING_ERROR(LOG_UNITS_X))
			/ LOG_UNITS_X);
}
#define LOGX_TO_UNIVERSE(lx) \
		logxToUniverse (lx)
static inline SDWORD
logyToUniverse (SDWORD ly)
{
	return (SDWORD) (MAX_Y_UNIVERSE -
			((ly * UNIVERSE_UNITS_Y + ROUNDING_ERROR(LOG_UNITS_Y))
			/ LOG_UNITS_Y));
}
#define LOGY_TO_UNIVERSE(ly) \
		logyToUniverse (ly)
static inline SDWORD
universeToLogx (COORD ux)
{
	return (ux * LOG_UNITS_X + ROUNDING_ERROR(UNIVERSE_UNITS_X))
			/ UNIVERSE_UNITS_X;
}
#define UNIVERSE_TO_LOGX(ux) \
		universeToLogx (ux)
static inline SDWORD
universeToLogy (COORD uy)
{
	return ((MAX_Y_UNIVERSE - uy) * LOG_UNITS_Y
			+ ROUNDING_ERROR(UNIVERSE_UNITS_Y))
			/ UNIVERSE_UNITS_Y;
}
#define UNIVERSE_TO_LOGY(uy) \
		universeToLogy (uy)

static inline SDWORD
oldLogxToUniverse(SDWORD lx)
{
	return (SDWORD)((lx * OLD_UNIVERSE_UNITS_X + ROUNDING_ERROR(OLD_LOG_UNITS_X))
		/ OLD_LOG_UNITS_X);
}

static inline SDWORD
oldLogyToUniverse(SDWORD ly)
{
	return (SDWORD)(MAX_Y_UNIVERSE -
		((ly * OLD_UNIVERSE_UNITS_Y + ROUNDING_ERROR(OLD_LOG_UNITS_Y))
			/ OLD_LOG_UNITS_Y));
}

#define CIRCLE_SHIFT 6
#define FULL_CIRCLE (1 << CIRCLE_SHIFT)
#define OCTANT_SHIFT (CIRCLE_SHIFT - 3) /* (1 << 3) == 8 */
#define HALF_CIRCLE (FULL_CIRCLE >> 1)
#define QUADRANT (FULL_CIRCLE >> 2)
#define OCTANT (FULL_CIRCLE >> 3)

#define FACING_SHIFT 4

#define ANGLE_TO_FACING(a) (((a)+(1<<(CIRCLE_SHIFT-FACING_SHIFT-1))) \
										>>(CIRCLE_SHIFT-FACING_SHIFT))
#define FACING_TO_ANGLE(f) ((f)<<(CIRCLE_SHIFT-FACING_SHIFT))

#define NORMALIZE_ANGLE(a) ((DWORD)((a)&(FULL_CIRCLE-1)))
#define NORMALIZE_FACING(f) ((DWORD)((f)&((1 << FACING_SHIFT)-1)))

#define DEGREES_TO_ANGLE(d) NORMALIZE_ANGLE((((d) % 360) * FULL_CIRCLE \
				+ HALF_CIRCLE) / 360)
#define ANGLE_TO_DEGREES(d) (NORMALIZE_ANGLE(d) * 360 / FULL_CIRCLE)

#define SIN_SHIFT 14
#define SIN_SCALE (1 << SIN_SHIFT)
#define INT_ADJUST(x) ((x)<<SIN_SHIFT)
#define FLT_ADJUST(x) (SIZE)((x)*SIN_SCALE)
#define UNADJUST(x) (SIZE)((x)>>SIN_SHIFT)
#define ROUND(x,y) ((x)+((x)>=0?((y)>>1):-((y)>>1)))

extern SDWORD sinetab[];
#define SINVAL(a) sinetab[NORMALIZE_ANGLE(a)]
#define COSVAL(a) SINVAL((a)+QUADRANT)
#define SINE(a,m) ((SDWORD)((((long)SINVAL(a))*(long)(m))>>SIN_SHIFT)) // JMS: SDWORD was SIZE. Changed to avoid overflows in hires.
#define COSINE(a,m) SINE((a)+QUADRANT,m)
extern COUNT ARCTAN (SDWORD delta_x, SDWORD delta_y); // JMS: SDWORD was SIZE. Changed to avoid overflows in hires.

#define WRAP_VAL(v,w) ((DWORD)((v)<0?((v)+(w)):((v)>=(w)?((v)-(w)):(v)))) // JMS: DWORD was COUNT. Changed to avoid overflows in hires.
#define WRAP_X(x) WRAP_VAL(x,LOG_SPACE_WIDTH)
#define WRAP_Y(y) WRAP_VAL(y,LOG_SPACE_HEIGHT)
#define WRAP_DELTA_X(dx) ((dx)<0 ? \
				((-(dx)<=LOG_SPACE_WIDTH>>1)?(dx):(LOG_SPACE_WIDTH+(dx))) : \
				(((dx)<=LOG_SPACE_WIDTH>>1)?(dx):((dx)-LOG_SPACE_WIDTH)))
#define WRAP_DELTA_Y(dy) ((dy)<0 ? \
				((-(dy)<=LOG_SPACE_HEIGHT>>1)?(dy):(LOG_SPACE_HEIGHT+(dy))) : \
				(((dy)<=LOG_SPACE_HEIGHT>>1)?(dy):((dy)-LOG_SPACE_HEIGHT)))

#if defined(__cplusplus)
}
#endif

#endif /* UQM_UNITS_H_ */

