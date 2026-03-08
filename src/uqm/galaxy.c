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

/* background starfield - used to generate agalaxy.asm */

#include "element.h"
#include "globdata.h"
#include "init.h"
#include "process.h"
#include "units.h"
#include "options.h"
#include "libs/compiler.h"
#include "libs/gfxlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/mathlib.h"
#include "libs/log.h"
#include "igfxres.h"
#include "nameref.h"
#include "libs/graphics/cmap.h"

extern COUNT zoom_out;
extern PRIM_LINKS DisplayLinks;
static BOOLEAN sceneries = 0;
static BYTE *HSOffsets = NULL;


#define BIG_STAR_COUNT 30
#define MED_STAR_COUNT 60
#define SML_STAR_COUNT 90
#define NUM_STARS (BIG_STAR_COUNT \
			+ MED_STAR_COUNT \
			+ SML_STAR_COUNT)
#define SCENERY(i) (sceneries && (i == (BIG_STAR_COUNT \
			+ MED_STAR_COUNT - 1)))

DPOINT SpaceOrg;
static DPOINT log_star_array[NUM_STARS];


#define NUM_STAR_PLANES 3

typedef struct
{
	COUNT min_star_index;
	COUNT num_stars;
	DPOINT *star_array;
	DPOINT *pmin_star;
	DPOINT *plast_star;
} STAR_BLOCK;

STAR_BLOCK StarBlock[NUM_STAR_PLANES] =
{
	{
		0, BIG_STAR_COUNT,
		&log_star_array[0],
		NULL, NULL,
	},
	{
		0, MED_STAR_COUNT,
		&log_star_array[BIG_STAR_COUNT],
		NULL, NULL,
	},
	{
		0, SML_STAR_COUNT,
		&log_star_array[BIG_STAR_COUNT + MED_STAR_COUNT],
		NULL, NULL,
	},
};

static void
SortStarBlock (STAR_BLOCK *pStarBlock)
{
	COUNT i;

	for (i = 0; i < pStarBlock->num_stars; ++i)
	{
		COUNT j;

		for (j = pStarBlock->num_stars - 1; j > i; --j)
		{
			if (pStarBlock->star_array[i].y > pStarBlock->star_array[j].y)
			{
				DPOINT temp;

				temp = pStarBlock->star_array[i];
				pStarBlock->star_array[i] = pStarBlock->star_array[j];
				pStarBlock->star_array[j] = temp;
			}
		}
	}

	pStarBlock->min_star_index = 0;
	pStarBlock->pmin_star = &pStarBlock->star_array[0];
	pStarBlock->plast_star =
			&pStarBlock->star_array[pStarBlock->num_stars - 1];
}

static void
WrapStarBlock (SIZE plane, SDWORD dx, SDWORD dy)
{
	COUNT i;
	DPOINT *ppt;
	SDWORD offs_y;
	COUNT num_stars;
	STAR_BLOCK *pStarBlock;

	pStarBlock = &StarBlock[plane];

	i = pStarBlock->min_star_index;
	ppt = pStarBlock->pmin_star;
	num_stars = pStarBlock->num_stars;
	if (dy < 0)
	{
		COUNT first;

		first = i;

		dy = -dy;
		offs_y = (LOG_SPACE_HEIGHT << plane) - dy;

		while (ppt->y < dy)
		{
			ppt->y += offs_y;
			ppt->x += dx;
			if (++i < num_stars)
				++ppt;
			else
			{
				i = 0;
				ppt = &pStarBlock->star_array[0];
			}

			if (i == first)
				return;
		}
		pStarBlock->min_star_index = i;
		pStarBlock->pmin_star = ppt;

		if (first <= i)
		{
			i = num_stars - i;
			do
			{
				ppt->y -= dy;
				ppt->x += dx;
				++ppt;
			} while (--i);
			ppt = &pStarBlock->star_array[0];
		}

		if (first > i)
		{
			i = first - i;
			do
			{
				ppt->y -= dy;
				ppt->x += dx;
				++ppt;
			} while (--i);
		}
	}
	else
	{
		COUNT last;

		--ppt;
		if (i-- == 0)
		{
			i = num_stars - 1;
			ppt = pStarBlock->plast_star;
		}

		last = i;

		if (dy > 0)
		{
			offs_y = (LOG_SPACE_HEIGHT << plane) - dy;

			while (ppt->y >= offs_y)
			{
				ppt->y -= offs_y;
				ppt->x += dx;
				if (i-- > 0)
					--ppt;
				else
				{
					i = num_stars - 1;
					ppt = pStarBlock->plast_star;
				}

				if (i == last)
					return;
			}

			pStarBlock->pmin_star = ppt + 1;
			if ((pStarBlock->min_star_index = i + 1) == num_stars)
			{
				pStarBlock->min_star_index = 0;
				pStarBlock->pmin_star = &pStarBlock->star_array[0];
			}
		}

		if (last >= i)
		{
			++i;
			do
			{
				ppt->y += dy;
				ppt->x += dx;
				--ppt;
			} while (--i);
			i = num_stars - 1;
			ppt = pStarBlock->plast_star;
		}

		if (last < i)
		{
			i = i - last;
			do
			{
				ppt->y += dy;
				ppt->x += dx;
				--ppt;
			} while (--i);
		}
	}
}

void
InitStarOffsets(void)
{
	COUNT i;
	BYTE *offsets;

	HSOffsets = HCalloc (sizeof(BYTE) * (BIG_STAR_COUNT + MED_STAR_COUNT));
	for (i = BIG_STAR_COUNT + MED_STAR_COUNT, offsets = HSOffsets;
					i > 0; --i, ++offsets)
	{
		*offsets = (BYTE)(TFB_Random () & 0x1F);
	}
}

void
UninitStarOffsets(void)
{
	HFree (HSOffsets);
	HSOffsets = NULL;
}

void
InitGalaxy (void)
{
	COUNT i, factor;
	DPOINT *ppt;
	PRIM_LINKS Links;
	Color medStarColor, smallStarColor;

	// Kruzen: Condition if we want to put something in melee background
	// So far - only Sa-Matra in penultimate battle with EXTENDED on
	sceneries = (EXTENDED && GET_GAME_STATE (URQUAN_PROTECTING_SAMATRA)
			&& LOBYTE (GLOBAL (CurrentActivity)) != IN_HYPERSPACE
			&& LOBYTE (GLOBAL (CurrentActivity)) != IN_LAST_BATTLE);

	log_add (log_Debug, "InitGalaxy(): transition_width = %d, "
			"transition_height = %d",
			TRANSITION_WIDTH, TRANSITION_HEIGHT);

	Links = MakeLinks (END_OF_LIST, END_OF_LIST);
	factor = ONE_SHIFT + MAX_REDUCTION + (BACKGROUND_SHIFT - 3);

	// Kruzen: Get star colors from corresponding palettes
	medStarColor = GetColorMapColor (0x01, 0x09);
	smallStarColor = GetColorMapColor (inHQSpace() ? 0x35 : 0x01, 
					inHQSpace() ? 0xFE : 0x07);

	if (IS_HD)
		ApplyMask (NULL, DecFrameIndex (DecFrameIndex (stars_in_space)),
				MAKE_DRAW_MODE (DRAW_REPLACE, 0xFF), &smallStarColor);

	for (i = 0, ppt = log_star_array; i < NUM_STARS; ++i, ++ppt)
	{
		COUNT p;

		p = AllocDisplayPrim ();
		SetPrimFlags (&DisplayArray[p], 0);

		if (i == BIG_STAR_COUNT || i == BIG_STAR_COUNT + MED_STAR_COUNT)
			++factor;

		ppt->x = (SDWORD)((UWORD)TFB_Random () % SPACE_WIDTH) << factor;
		ppt->y = (SDWORD)((UWORD)TFB_Random () % SPACE_HEIGHT) << factor;

		if (i < BIG_STAR_COUNT + MED_STAR_COUNT)
		{// Big and Med stars
			SetPrimType (&DisplayArray[p], STAMP_PRIM);
			SetPrimColor (&DisplayArray[p], medStarColor);
			DisplayArray[p].Object.Stamp.frame = (SCENERY(i) ? scenery 
							: stars_in_space);

			// TODO: Remove these eventually
			if (inQuasiSpace ())
				SetPrimFlags (&DisplayArray[p], HYPER_TO_QUASI_COLOR);
		}
		else
		{// in SD, when prim type is POINT, nothing below color will matter
			SetPrimType (&DisplayArray[p], IS_HD ? STAMP_PRIM : POINT_PRIM);
			SetPrimColor (&DisplayArray[p], smallStarColor);
			SetPrimFlags (&DisplayArray[p], UNSCALED_STAMP);
			DisplayArray[p].Object.Stamp.frame =
							DecFrameIndex (DecFrameIndex (stars_in_space));
		}

		InsertPrim (&Links, p, GetPredLink(Links));
	}

	SortStarBlock (&StarBlock[0]);
	SortStarBlock (&StarBlock[1]);
	SortStarBlock (&StarBlock[2]);
}

static BOOLEAN
CmpMovePoints (const POINT *pt1, const DPOINT *pt2, SDWORD dx, SDWORD dy,
			   SIZE reduction)
{
	if (optMeleeScale == TFB_SCALE_STEP)
	{
		return (int)pt1->x != (int)((pt2->x - dx) >> reduction)
			|| (int)pt1->y != (int)((pt2->y - dy) >> reduction);
	}
	else
	{
		return (int)pt1->x != (int)(((pt2->x - dx) << ZOOM_SHIFT) / reduction)
			|| (int)pt1->y != (int)(((pt2->y - dy) << ZOOM_SHIFT) / reduction);
	}
}

#define ANIM_ONE_OFFSET 37
#define ANIM_TWO_OFFSET 26
#define ANIM_BREAKPOINT 10

void
MoveGalaxy (VIEW_STATE view_state, SDWORD dx, SDWORD dy)
{
	if (view_state != VIEW_STABLE)
	{
		PRIMITIVE *pprim;
		BYTE *offsets;
		COUNT reduction = zoom_out, iss, i;
		int wrap_around;
		DPOINT *ppt;

		static const COUNT star_counts[] =
		{
			BIG_STAR_COUNT,
			MED_STAR_COUNT,
			SML_STAR_COUNT
		};

		if (inHQSpace ())
		{// Process stars in HyperSpace
			COUNT i;

			pprim = DisplayArray;
			offsets = HSOffsets;

			// We don't have corresponding elements, so we have to process
			// animations manually
			for (i = BIG_STAR_COUNT; i > 0; --i, ++pprim, ++offsets)
			{// Animation 1 - rings
				*offsets = (*offsets + 1) & 0x1F; // 32 frames total
				
				pprim->Object.Stamp.frame =
						SetAbsFrameIndex (pprim->Object.Stamp.frame,
						*offsets > ANIM_BREAKPOINT ? 48 : *offsets 
							+ ANIM_ONE_OFFSET);
			}

			for (i = MED_STAR_COUNT; i > 0; --i, ++pprim, ++offsets)
			{// Animation 2 - blinking lights
				*offsets = (*offsets + 1) & 0xF; // 16 frames total
				
				pprim->Object.Stamp.frame =
						SetAbsFrameIndex (pprim->Object.Stamp.frame,
						*offsets > ANIM_BREAKPOINT ? 48 : *offsets
							+ ANIM_TWO_OFFSET);
			}

			dx <<= 3;
			dy <<= 3;

			WrapStarBlock (2, dx, dy);
			WrapStarBlock (1, dx, dy);
			WrapStarBlock (0, dx, dy);

			dx = (COORD)(LOG_SPACE_WIDTH >> 1)
					- (LOG_SPACE_WIDTH >> ((MAX_REDUCTION + 1)
					- MAX_VIS_REDUCTION));
			dy = (COORD)(LOG_SPACE_HEIGHT >> 1)
					- (LOG_SPACE_HEIGHT >> ((MAX_REDUCTION + 1)
					- MAX_VIS_REDUCTION));
			reduction = optMeleeScale ? (MAX_ZOOM_OUT << ONE_SHIFT) : 
					(MAX_VIS_REDUCTION + ONE_SHIFT);
		}
		else
		{// Process stars in Melee combat
			COUNT scale = optMeleeScale ? reduction >> 9 : reduction;

			if (view_state == VIEW_CHANGE)
			{
				GRAPHICS_PRIM star_object[] =
				{
					(!IS_HD && optMeleeScale && reduction > 256) ? 
							POINT_PRIM : STAMP_PRIM,
					(!IS_HD && optMeleeScale) ? POINT_PRIM : 
							(IS_HD && scale == 1) ? (UNSCALED_STAMP << 7) |
							STAMP_PRIM : STAMP_PRIM
				};
				FRAME star_frame[] = 
				{
					SetAbsFrameIndex (stars_in_space, optMeleeScale ? 
							scale : min (scale, 1)),
					SetAbsFrameIndex (stars_in_space, min (scale + 1, 2))
				};

				for (iss = 0, pprim = DisplayArray; iss < 2; ++iss)
				{
					for (i = star_counts[iss]; i > (sceneries & iss); --i, ++pprim)
					{
						SetPrimType (pprim, star_object[iss] & 0xF);
						SetPrimFlags (pprim, star_object[iss] >> 7);
						pprim->Object.Stamp.frame = star_frame[iss];
					}
				}
				if (i > 0)
				{
					pprim->Object.Stamp.frame = SetAbsFrameIndex (scenery, scale);
				}
			}

			WrapStarBlock (2, dx, dy);
			WrapStarBlock (1, dx, dy);
			WrapStarBlock (0, dx, dy);

			dx = SpaceOrg.x;
			dy = SpaceOrg.y;
			reduction = optMeleeScale ? reduction << ONE_SHIFT :
						reduction + ONE_SHIFT;
		}

		ppt = log_star_array;
		for (iss = 0, pprim = DisplayArray, wrap_around = LOG_SPACE_WIDTH;
				iss < 3 && 
				(view_state == VIEW_CHANGE || CmpMovePoints (
					&pprim->Object.Point, ppt, dx, dy, reduction));
				++iss, wrap_around <<= 1, dx <<= 1, dy <<= 1)
		{
			for (i = star_counts[iss]; i > 0; --i, ++pprim, ++ppt)
			{
				// ppt->x &= (LOG_SPACE_WIDTH - 1);
				ppt->x = WRAP_VAL (ppt->x, wrap_around);
				if (optMeleeScale == TFB_SCALE_STEP)
				{
					pprim->Object.Point.x = (ppt->x - dx) >> reduction;
					pprim->Object.Point.y = (ppt->y - dy) >> reduction;
				}
				else
				{
					pprim->Object.Point.x = ((ppt->x - dx) << ZOOM_SHIFT)
							/ reduction;
					pprim->Object.Point.y = ((ppt->y - dy) << ZOOM_SHIFT)
							/ reduction;
				}
			}
			if (optMeleeScale == TFB_SCALE_STEP)
				++reduction;
			else
				reduction <<= 1;
		}
	}

	DisplayLinks = MakeLinks (NUM_STARS - 1, 0);
}

void
SetStarPoint (POINT pt, COUNT i)
{
	log_star_array[i].x = pt.x;
	log_star_array[i].y = pt.y;
}
