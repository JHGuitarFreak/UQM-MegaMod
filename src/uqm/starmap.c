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

#include "starmap.h"
#include "gamestr.h"
#include "globdata.h"
#include "libs/gfxlib.h"


STAR_DESC *star_array;
STAR_DESC *CurStarDescPtr = 0;

STAR_DESC*
FindStar (STAR_DESC *LastSDPtr, POINT *puniverse, SIZE xbounds,
		SIZE ybounds)
{
	COORD min_y, max_y;
	SIZE lo, hi;
	STAR_DESC *BaseSDPtr;

	if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
	{
		BaseSDPtr = star_array;
		hi = NUM_SOLAR_SYSTEMS - 1;
	}
	else
	{
#define NUM_HYPER_VORTICES 15
		BaseSDPtr = &star_array[NUM_SOLAR_SYSTEMS + 1];
		hi = (NUM_HYPER_VORTICES + 1) - 1;
	}

	if (LastSDPtr == NULL)
		lo = 0;
	else if ((lo = LastSDPtr - BaseSDPtr + 1) > hi)
		return (0);
	else
		hi = lo;

	if (ybounds <= 0)
		min_y = max_y = puniverse->y;
	else
	{
		min_y = puniverse->y - ybounds;
		max_y = puniverse->y + ybounds;
	}

	while (lo < hi)
	{
		SIZE mid;

		mid = (lo + hi) >> 1;
		if (BaseSDPtr[mid].star_pt.y >= min_y)
			hi = mid - 1;
		else
			lo = mid + 1;
	}

	LastSDPtr = &BaseSDPtr[lo];
	if (ybounds < 0 || LastSDPtr->star_pt.y <= max_y)
	{
		COORD min_x, max_x;

		if (xbounds <= 0)
			min_x = max_x = puniverse->x;
		else
		{
			min_x = puniverse->x - xbounds;
			max_x = puniverse->x + xbounds;
		}

		do
		{
			if ((ybounds < 0 || LastSDPtr->star_pt.y >= min_y)
					&& (xbounds < 0
					|| (LastSDPtr->star_pt.x >= min_x
					&& LastSDPtr->star_pt.x <= max_x))
					)
				return (LastSDPtr);
		} while ((++LastSDPtr)->star_pt.y <= max_y);
	}

	return (0);
}

void
GetClusterName (const STAR_DESC *pSD, UNICODE buf[])
{
	UNICODE *pBuf, *pStr;

	pBuf = buf;
	if (pSD->Prefix)
	{
		pStr = GAME_STRING (STAR_NUMBER_BASE + pSD->Prefix - 1);
		if (pStr)
		{
			while ((*pBuf++ = *pStr++))
				;
			pBuf[-1] = ' ';
		}
	}
	if ((pStr = GAME_STRING (pSD->Postfix)) == 0)
		*pBuf = '\0';
	else
	{
		while ((*pBuf++ = *pStr++))
			;
	}
}

