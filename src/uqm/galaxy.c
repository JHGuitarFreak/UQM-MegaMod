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

extern COUNT zoom_out;
extern PRIM_LINKS DisplayLinks;


#define BIG_STAR_COUNT 30
#define MED_STAR_COUNT 60
#define SML_STAR_COUNT 90
#define NUM_STARS (BIG_STAR_COUNT \
			+ MED_STAR_COUNT \
			+ SML_STAR_COUNT)

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
InitGalaxy (void)
{
	COUNT i, factor;
	DPOINT *ppt;
	PRIM_LINKS Links;
	BOOLEAN HSorQS = (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1 ? FALSE : TRUE);

	log_add (log_Debug, "InitGalaxy(): transition_width = %d, "
			"transition_height = %d",
			TRANSITION_WIDTH, TRANSITION_HEIGHT);

	Links = MakeLinks (END_OF_LIST, END_OF_LIST);
	factor = ONE_SHIFT + MAX_REDUCTION + (BACKGROUND_SHIFT - 3);
	for (i = 0, ppt = log_star_array; i < NUM_STARS; ++i, ++ppt)
	{
		COUNT p;

		p = AllocDisplayPrim ();

		if (i == BIG_STAR_COUNT || i == BIG_STAR_COUNT + MED_STAR_COUNT)
			++factor;

		ppt->x = (SDWORD)((UWORD)TFB_Random () % SPACE_WIDTH) << factor;
		ppt->y = (SDWORD)((UWORD)TFB_Random () % SPACE_HEIGHT) << factor;

		if (i < BIG_STAR_COUNT + MED_STAR_COUNT)
		{
			SetPrimType (&DisplayArray[p], STAMP_PRIM);
			SetPrimColor (&DisplayArray[p],
					BUILD_COLOR (MAKE_RGB15 (0x0B, 0x0B, 0x1F), 0x09));
			// JMS_GFX: This was originally only "DisplayArray[p].Object.Stamp.frame = stars_in_space;"
			if (!IS_HD || (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1))
				DisplayArray[p].Object.Stamp.frame = stars_in_space;
			else
				DisplayArray[p].Object.Stamp.frame = stars_in_quasispace;
		}
		else
		{
			if(IS_HD){
				// In HD the starpoints in HS and QS are images
				SetPrimType (&DisplayArray[p], STAMP_PRIM);
				if (LOBYTE (GLOBAL (CurrentActivity)) != IN_HYPERSPACE){
					SetPrimType (&DisplayArray[p], POINT_PRIM);
					SetPrimColor (&DisplayArray[p], BUILD_COLOR (MAKE_RGB15 (0x15, 0x15, 0x15), 0x07));
				} else
					DisplayArray[p].Object.Stamp.frame = SetAbsFrameIndex (StarPoints, HSorQS);
			} else {
				// Pixel starpoints in original res
				SetPrimType (&DisplayArray[p], POINT_PRIM);
				if (LOBYTE (GLOBAL (CurrentActivity)) != IN_HYPERSPACE)
					SetPrimColor (&DisplayArray[p], BUILD_COLOR (MAKE_RGB15 (0x15, 0x15, 0x15), 0x07));
				else if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
					SetPrimColor (&DisplayArray[p], BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x8C));
				else
					SetPrimColor (&DisplayArray[p], BUILD_COLOR (MAKE_RGB15 (0x00, 0x0E, 0x00), 0x8C));
			}
		}

		InsertPrim (&Links, p, GetPredLink (Links));
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

void
MoveGalaxy (VIEW_STATE view_state, SDWORD dx, SDWORD dy)
{
	PRIMITIVE *pprim;
	static const COUNT star_counts[] =
	{
		BIG_STAR_COUNT,
		MED_STAR_COUNT,
		SML_STAR_COUNT
	};
	static const COUNT star_frame_ofs[] = { 32 + 26, 26, 0 };

	if (view_state != VIEW_STABLE)
	{
		COUNT reduction, i, iss;
		DPOINT *ppt;
		FRAME tempframe;
		int wrap_around;

		reduction = zoom_out;

		// JMS_GFX
		if (!IS_HD || (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1))
			tempframe = stars_in_space;
		else
			tempframe = stars_in_quasispace;

		if (view_state == VIEW_CHANGE)
		{
			if (inHQSpace ())
			{
				for (iss = 0, pprim = DisplayArray; iss < 2; ++iss)
				{
					for (i = star_counts[iss]; i > 0; --i, ++pprim)
					{
						pprim->Object.Stamp.frame =	SetAbsFrameIndex (
								tempframe,
									(COUNT)(TFB_Random () & 31)
									+ star_frame_ofs[iss]);
					}
				}
			}
			else
			{
				GRAPHICS_PRIM star_object[2];
				FRAME star_frame[9]; // JMS_GFX: was 2. Added extra frames for more star .pngs.

				star_frame[0] = IncFrameIndex (stars_in_space);
				star_frame[1] = stars_in_space;
			
				if (optMeleeScale == TFB_SCALE_STEP)
				{	/* on PC, the closest stars are images when zoomed out */
					star_object[0] = STAMP_PRIM;
					if (reduction > 0)
					{
						star_object[1] = POINT_PRIM;
						star_frame[0] = star_frame[1];
					}
					else
					{
						star_object[1] = STAMP_PRIM;
					}
				}
				else
				{	/* on 3DO, the closest stars are pixels when zoomed out */
					star_object[1] = POINT_PRIM;
					if (reduction > (1 << ZOOM_SHIFT))
					{
						// JMS_GFX: In hi-res modes, Closest stars are images when zoomed out.
						if (!IS_HD)
							star_object[0] = POINT_PRIM;
						else
						{
							star_object[0] = STAMP_PRIM;
							star_object[1] = STAMP_PRIM;
							star_frame[0] = stars_in_space;
							star_frame[1] = IncFrameIndex (stars_in_space);
							
						}
					}
					else
					{
						star_object[0] = STAMP_PRIM;
						star_object[1] = STAMP_PRIM;
					}
				}

				// Normal handling of stars in 320x240.
				if (!IS_HD)
				{
					for (iss = 0, pprim = DisplayArray; iss < 2; ++iss)
					{
						for (i = star_counts[iss]; i > 0; --i, ++pprim)
						{
							SetPrimType (pprim, star_object[iss]);
							pprim->Object.Stamp.frame = star_frame[iss];
						}
					}
				}
				
				// JMS_GFX: Advanced handling of stars in hi-res modes.
				// Basically, draw a BIG star .png when zoomed close in
				// medium-sized when at med distance and a small .png when far away.
				else
				{
					COUNT zoomlevel;
					COUNT med_sml_zoom_limit = optMeleeScale == TFB_SCALE_STEP ? 0 : (1 << (ZOOM_SHIFT + 1));
					
					if (reduction == MAX_ZOOM_OUT)
						zoomlevel = 0;
					else if (reduction <= med_sml_zoom_limit)
						zoomlevel = 6;
					else
						zoomlevel = 3;
					
					for (i = 3; i < 9; i++)
					{
						star_frame[i] = SetAbsFrameIndex (stars_in_space, i);
					}
				
					for (iss = 0, pprim = DisplayArray; iss < 2; ++iss)
					{
						for (i = star_counts[iss]; i > 0; --i, ++pprim)
						{
							SetPrimType (pprim, star_object[iss]);
							pprim->Object.Stamp.frame = star_frame[iss + zoomlevel];
						}
					}
				}
			}
		}

		if (inHQSpace ())
		{
			for (i = BIG_STAR_COUNT + MED_STAR_COUNT, pprim = DisplayArray;
					i > 0; --i, ++pprim)
			{
				COUNT base_index;

				base_index = GetFrameIndex (pprim->Object.Stamp.frame) - 26;
				pprim->Object.Stamp.frame =
						SetAbsFrameIndex (pprim->Object.Stamp.frame,
						((base_index & ~31) + ((base_index + 1) & 31)) + 26);
			}

			dx <<= 3;
			dy <<= 3;
		}

		WrapStarBlock (2, dx, dy);
		WrapStarBlock (1, dx, dy);
		WrapStarBlock (0, dx, dy);

		if (!inHQSpace ())
		{
			dx = SpaceOrg.x;
			dy = SpaceOrg.y;
			if (optMeleeScale == TFB_SCALE_STEP)
				reduction += ONE_SHIFT;
			else
				reduction <<= ONE_SHIFT;
		}
		else
		{
			dx = (SDWORD)(LOG_SPACE_WIDTH >> 1)
					- (LOG_SPACE_WIDTH >> ((MAX_REDUCTION + 1)
					- MAX_VIS_REDUCTION));
			dy = (SDWORD)(LOG_SPACE_HEIGHT >> 1)
					- (LOG_SPACE_HEIGHT >> ((MAX_REDUCTION + 1)
					- MAX_VIS_REDUCTION));
			if (optMeleeScale == TFB_SCALE_STEP)
				reduction = MAX_VIS_REDUCTION + ONE_SHIFT;
			else
				reduction = MAX_ZOOM_OUT << ONE_SHIFT;
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
