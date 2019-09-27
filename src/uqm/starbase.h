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

#ifndef UQM_STARBASE_H_
#define UQM_STARBASE_H_

#include "menustat.h"

#if defined(__cplusplus)
extern "C" {
#endif

enum
{
	TALK_COMMANDER = 0,
	OUTFIT_STARSHIP,
	SHIPYARD,
	DEPART_BASE
};
typedef BYTE STARBASE_STATE;


extern void InstallBombAtEarth (void);
extern void VisitStarBase (void);
extern BOOLEAN DoStarBase (MENU_STATE *pMS);
extern BOOLEAN DoOutfit (MENU_STATE *pMS);
extern BOOLEAN DoShipyard (MENU_STATE *pMS);

extern void DrawShipPiece (FRAME ModuleFrame, COUNT which_piece, COUNT
		which_slot, BOOLEAN DrawBluePrint);

extern COUNT WrapText (const UNICODE *pStr, COUNT len, TEXT *tarray, SIZE
		field_width);
		// XXX: Doesn't really belong in this file.

#if defined(__cplusplus)
}
#endif

#endif /* UQM_STARBASE_H_ */

