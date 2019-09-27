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

#ifndef UQM_WEAPON_H_
#define UQM_WEAPON_H_

#include "element.h"
#include "libs/gfxlib.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct
{
	COORD cx, cy, ex, ey;
	ELEMENT_FLAGS flags;
	SIZE sender; // player number
	SIZE pixoffs;
	COUNT face;
	Color color;
} LASER_BLOCK;

typedef struct
{
	COORD cx, cy;
	ELEMENT_FLAGS flags;
	SIZE sender; // player number
	SIZE pixoffs, speed, hit_points, damage;
	COUNT face, index, life;
	FRAME *farray;
	void (*preprocess_func) (ELEMENT *ElementPtr);
	SIZE blast_offs;
} MISSILE_BLOCK;

extern HELEMENT initialize_laser (LASER_BLOCK *pLaserBlock);
extern HELEMENT initialize_missile (MISSILE_BLOCK *pMissileBlock);
extern HELEMENT weapon_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1);
extern SIZE TrackShip (ELEMENT *Tracker, COUNT *pfacing);
extern void Untarget (ELEMENT *ElementPtr);

#define MODIFY_IMAGE (1 << 0)
#define MODIFY_SWAP (1 << 1)

extern FRAME ModifySilhouette (ELEMENT *ElementPtr, STAMP *modify_stamp,
		BYTE modify_flags);

#if defined(__cplusplus)
}
#endif

#endif /* UQM_WEAPON_H_ */

