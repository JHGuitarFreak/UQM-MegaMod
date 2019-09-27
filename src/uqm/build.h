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

#ifndef UQM_BUILD_H_
#define UQM_BUILD_H_

#include "races.h"
#include "displist.h"
#include "libs/compiler.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define NAME_OFFSET 5
#define NUM_CAPTAINS_NAMES 16

#define PickCaptainName() (((COUNT)TFB_Random () \
								& (NUM_CAPTAINS_NAMES - 1)) \
								+ NAME_OFFSET)

extern HLINK Build (QUEUE *pQueue, SPECIES_ID SpeciesID);
extern HSHIPFRAG CloneShipFragment (RACE_ID shipIndex, QUEUE *pDstQueue,
		COUNT crew_level);
extern HLINK GetStarShipFromIndex (QUEUE *pShipQ, COUNT Index);
extern HSHIPFRAG GetEscortByStarShipIndex (COUNT index);
extern BYTE NameCaptain (QUEUE *pQueue, SPECIES_ID SpeciesID);

extern COUNT GetIndexFromStarShip (QUEUE *pShipQ, HLINK hStarShip);
extern int SetEscortCrewComplement (RACE_ID which_ship, COUNT crew_level,
		BYTE captain);

extern SPECIES_ID ShipIdStrToIndex (const char *shipIdStr);
extern RACE_ID RaceIdStrToIndex (const char *raceIdStr);
extern COUNT AddEscortShips (RACE_ID race, SIZE count);
extern COUNT CalculateEscortsWorth (void);
extern BOOLEAN SetRaceAllowBuild (RACE_ID race);
extern BOOLEAN SetRaceAllied (RACE_ID race, BOOLEAN flag);
extern COUNT StartSphereTracking (RACE_ID race);
extern BOOLEAN CheckSphereTracking (RACE_ID race);
extern BOOLEAN KillRace (RACE_ID race);
extern COUNT CountEscortShips (RACE_ID race);
extern BOOLEAN HaveEscortShip (RACE_ID race);
extern COUNT EscortFeasibilityStudy (RACE_ID race);
extern COUNT CheckAlliance (RACE_ID race);
extern COUNT RemoveSomeEscortShips (RACE_ID race, COUNT count);
extern COUNT RemoveEscortShips (RACE_ID race);

extern RACE_DESC *load_ship (SPECIES_ID SpeciesID, BOOLEAN LoadBattleData);
extern void free_ship (RACE_DESC *RaceDescPtr, BOOLEAN FreeIconData,
		BOOLEAN FreeBattleData);
extern void loadGameCheats (void);

#if defined(__cplusplus)
}
#endif

#endif /* UQM_BUILD_H_ */

