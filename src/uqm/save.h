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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef UQM_SAVE_H_
#define UQM_SAVE_H_

#include "sis.h" // SUMMARY_DESC includes SIS_STATE in it
#include "globdata.h"
#include "libs/compiler.h"

#if defined(__cplusplus)
extern "C" {
#endif

// XXX: Theoretically, a player can have 17 devices on board without
//   cheating. We only provide
//   room for 16 below, which is not really a problem since this
//   is only used for displaying savegame summaries. There is also
//   room for only 16 devices on screen.
#define MAX_EXCLUSIVE_DEVICES 16
#define SAVE_NAME_SIZE 64

// The savefile tag numbers.
#define SAVEFILE_TAG     0x01534d55 // "UMS\x01": UQM Save version 1
#define SUMMARY_TAG      0x6d6d7553 // "Summ": Summary. Must be first!
#define GLOBAL_STATE_TAG 0x74536c47 // "GlSt": Global State. Must be 2nd!
#define GAME_STATE_TAG   0x74536d47 // "GmSt": Game State Bits. Must be 3rd!
#define EVENTS_TAG       0x73747645 // "Evts": Events
#define ENCOUNTERS_TAG   0x74636e45 // "Enct": Encounters
#define RACE_Q_TAG       0x51636152 // "RacQ": avail_race_q
#define IP_GRP_Q_TAG     0x51704749 // "IGpQ": ip_group_q
#define NPC_SHIP_Q_TAG   0x5163704e // "NpcQ": npc_built_ship_q
#define SHIP_Q_TAG       0x51706853 // "ShpQ": built_ship_q
#define STAR_TAG         0x72617453 // "Star": STAR_DESC
#define SCAN_TAG         0x6e616353 // "Scan": Scan Masks (stuff picked up)
#define BATTLE_GROUP_TAG 0x70477442 // "BtGp": Battle Group definition
#define GROUP_LIST_TAG   0x73707247 // "Grps": Group List

typedef struct
{
	SIS_STATE SS;
	BYTE Activity;
	BYTE Flags;
	BYTE day_index, month_index;
	COUNT year_index;
	BYTE MCreditLo, MCreditHi;
	BYTE NumShips, NumDevices;
	BYTE ShipList[MAX_BUILT_SHIPS];
	BYTE DeviceList[MAX_EXCLUSIVE_DEVICES];
	UNICODE SaveName[SAVE_NAME_SIZE], SaveNameChecker[SAVE_CHECKER_SIZE], LegacySaveName[LEGACY_SAVE_NAME_SIZE]; // JMS
	BYTE res_factor;
} SUMMARY_DESC;

extern ACTIVITY NextActivity;

extern BOOLEAN LoadGame (COUNT which_game, SUMMARY_DESC *summary_desc);
extern BOOLEAN LoadLegacyGame  (COUNT which_game, SUMMARY_DESC *SummPtr, BOOLEAN try_vanilla);

extern void SaveProblem (void);
extern BOOLEAN SaveGame (COUNT which_game, SUMMARY_DESC *summary_desc, const char *name);

extern const GameStateBitMap gameStateBitMap[];

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_SAVE_H_ */

