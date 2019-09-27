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

#ifndef UQM_STATUS_H_INCL_
#define UQM_STATUS_H_INCL_

#include "races.h"
#include "libs/compiler.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define CREW_XOFFS RES_STAT_SCALE(4)
#define ENERGY_XOFFS RES_STAT_SCALE(52)
#define GAUGE_YOFFS (SHIP_INFO_HEIGHT - RES_SCALE(10))
#define UNIT_WIDTH RES_SCALE(2)
#define UNIT_HEIGHT RES_SCALE(1)
#define STAT_WIDTH (1 + UNIT_WIDTH + 1 + UNIT_WIDTH + 1)

#define SHIP_INFO_HEIGHT RES_SCALE(65)
#define CAPTAIN_WIDTH RES_STAT_SCALE(55)
#define CAPTAIN_HEIGHT RES_STAT_SCALE(30)
#define CAPTAIN_XOFFS ((STATUS_WIDTH - CAPTAIN_WIDTH) >> 1) 
#define CAPTAIN_YOFFS (SHIP_INFO_HEIGHT + RES_SCALE(4))

#define SHIP_STATUS_HEIGHT (STATUS_HEIGHT >> 1)
#define BAD_GUY_YOFFS 0
#define GOOD_GUY_YOFFS SHIP_STATUS_HEIGHT

#define STARCON_TEXT_HEIGHT RES_SCALE(7)
#define TINY_TEXT_HEIGHT RES_SCALE(9)
#define BATTLE_CREW_X RES_STAT_SCALE(10)
#define BATTLE_CREW_Y (RES_SCALE(64))

extern COORD status_y_offsets[];

extern void InitStatusOffsets (void);

extern void DrawCrewFuelString(COORD y, SIZE state, BOOLEAN InMeleeMenu);
extern void ClearShipStatus (COORD y, COORD w, BOOLEAN inMeleeMenu);
extern void OutlineShipStatus (COORD y, COORD w, BOOLEAN inMeleeMenu); // JMS: now is needed elsewhere
extern void InitShipStatus (SHIP_INFO *ShipInfoPtr, STARSHIP *StarShipPtr, RECT *pClipRect, BOOLEAN inMeleeMenu);
			// StarShipPtr or pClipRect can be NULL
extern void DeltaStatistics (SHIP_INFO *ShipInfoPtr, COORD y_offs,
		SIZE crew_delta, SIZE energy_delta);
extern void DrawBattleCrewAmount (SHIP_INFO *ShipInfoPtr, COORD y_offs);

extern void DrawCaptainsWindow (STARSHIP *StarShipPtr);
extern BOOLEAN DeltaEnergy (ELEMENT *ElementPtr, SIZE energy_delta);
extern BOOLEAN DeltaCrew (ELEMENT *ElementPtr, SIZE crew_delta);

extern void PreProcessStatus (ELEMENT *ShipPtr);
extern void PostProcessStatus (ELEMENT *ShipPtr);

#if defined(__cplusplus)
}
#endif

#endif /* UQM_STATUS_H_INCL_ */
