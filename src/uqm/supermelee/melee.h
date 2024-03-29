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

#ifndef UQM_SUPERMELEE_MELEE_H_
#define UQM_SUPERMELEE_MELEE_H_

#include "../init.h"
#include "libs/gfxlib.h"
#include "libs/mathlib.h"
#include "libs/sndlib.h"
#include "libs/timelib.h"
#include "libs/reslib.h"
#include "netplay/packet.h"
		// for NetplayAbortReason and NetplayResetReason.

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct melee_state MELEE_STATE;

#define NUM_MELEE_ROWS 2
#define NUM_MELEE_COLUMNS SAFE_BOOL (7, 6)
//#define NUM_MELEE_COLUMNS 7
#define TRUE_MELEE_FLEET_SIZE (NUM_MELEE_ROWS * NUM_MELEE_COLUMNS)
#define MELEE_FLEET_SIZE (14)
#define ICON_WIDTH RES_SCALE (16)
#define ICON_HEIGHT RES_SCALE (16)

extern FRAME PickMeleeFrame;

#define PICK_BG_COLOR    BUILD_COLOR (MAKE_RGB15 (0x00, 0x01, 0x0F), 0x01)
#define PICK_VALUE_COLOR BUILD_COLOR (MAKE_RGB15 (0x13, 0x00, 0x00), 0x2C)
		// Used for the current fleet value in the next ship selection
		// in SuperMelee.

#define MAX_TEAM_CHARS 30
#define NUM_PICK_COLS 5
#define NUM_PICK_ROWS 5

typedef BYTE MELEE_OPTIONS;

#if defined(__cplusplus)
}
#endif

#include "loadmele.h"
#include "meleesetup.h"
#include "meleeship.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct melee_state
{
	BOOLEAN (*InputFunc) (struct melee_state *pInputState);

	BOOLEAN Initialized;
	BOOLEAN meleeStarted;
	MELEE_OPTIONS MeleeOption;
	COUNT side;
	COUNT row;
	COUNT col;
	MeleeSetup *meleeSetup;
	struct melee_load_state load;
	MeleeShip currentShip;
			// The ship currently displayed. Not really needed.
			// Also the current ship position when selecting a ship.
	COUNT CurIndex;
#define MELEE_STATE_INDEX_DONE ((COUNT) -1)
			// Current position in the team string when editing it.
			// Set to MELEE_STATE_INDEX_DONE when done.
	BOOLEAN buildPickConfirmed;
			// Used by DoPickShip () to communicate to the calling
			// function BuildPickShip() whether a ship has been selected
			// to add to the fleet, or whether the operation has been
			// cancelled. If a ship was selected, it is set in
			// currentShip.
	RandomContext *randomContext;
			/* RNG state for all local random decisions, i.e. those
			 * decisions that are not shared among network parties. */
	TimeCount LastInputTime;

	MUSIC_REF hMusic;
};

extern void Melee (void);

// Some prototypes for use by loadmele.c:
BOOLEAN DoMelee (MELEE_STATE *pMS);
void DrawMeleeIcon (COUNT which_icon, BOOLEAN HiLite);
void GetShipBox (RECT *pRect, COUNT side, COUNT row, COUNT col);
void RepairMeleeFrame (const RECT *pRect);
void DrawMeleeShipStrings (MELEE_STATE *pMS, MeleeShip NewStarShip);
extern FRAME MeleeFrame;
void Melee_flashSelection (MELEE_STATE *pMS);
extern void QuickRepair (COUNT whichFrame, RECT *pRect);

COUNT GetShipValue (MeleeShip StarShip);

void updateRandomSeed (MELEE_STATE *pMS, COUNT side, DWORD seed);
void confirmationCancelled (MELEE_STATE *pMS, COUNT side);
void connectedFeedback (NetConnection *conn);
void abortFeedback (NetConnection *conn, NetplayAbortReason reason);
void resetFeedback (NetConnection *conn, NetplayResetReason reason,
		bool byRemote);
void errorFeedback (NetConnection *conn);
void closeFeedback (NetConnection *conn);

bool Melee_LocalChange_ship (MELEE_STATE *pMS, COUNT side,
		FleetShipIndex index, MeleeShip ship);
bool Melee_LocalChange_teamName (MELEE_STATE *pMS, COUNT side,
		const char *name);
bool Melee_LocalChange_fleet (MELEE_STATE *pMS, size_t teamNr,
		const MeleeShip *fleet);
bool Melee_LocalChange_team (MELEE_STATE *pMS, size_t teamNr,
		const MeleeTeam *team);

void Melee_bootstrapSyncTeam (MELEE_STATE *pMS, size_t teamNr);

void Melee_RemoteChange_ship (MELEE_STATE *pMS, NetConnection *conn,
		COUNT side, FleetShipIndex index, MeleeShip ship);
void Melee_RemoteChange_teamName (MELEE_STATE *pMS, NetConnection *conn,
		COUNT side, const char *name);

void DrawShipPickerText (STAMP stamp);

enum
{	// meleemenu.ani enumerator
	MELEE_BACKGROUND = 0,

	HUMAN_CON_TOP,
	WEAK_CYBORG_TOP,
	GOOD_CYBORG_TOP,
	AWES_CYBORG_TOP,

	HUMAN_CON_TOP_HL,
	WEAK_CYBORG_TOP_HL,
	GOOD_CYBORG_TOP_HL,
	AWES_CYBORG_TOP_HL,

	HUMAN_CON_BOTT,
	WEAK_CYBORG_BOTT,
	GOOD_CYBORG_BOTT,
	AWES_CYBORG_BOTT,

	HUMAN_CON_BOTT_HL,
	WEAK_CYBORG_BOTT_HL,
	GOOD_CYBORG_BOTT_HL,
	AWES_CYBORG_BOTT_HL,

	LOAD_BUTTON_TOP,
	SAVE_BUTTON_TOP,
	LOAD_BUTTON_TOP_HL,
	SAVE_BUTTON_TOP_HL,

	SAVE_BUTTON_BOTT,
	LOAD_BUTTON_BOTT,
	SAVE_BUTTON_BOTT_HL,
	LOAD_BUTTON_BOTT_HL,

	BATTLE_BUTTON,
	BATTLE_BUTTON_HL,

	SHIP_PICK_KB,
	SHIP_PICK_XB,
	SHIP_PICK_PS,

	TEAM_PICK_KB,
	TEAM_PICK_XB,
	TEAM_PICK_PS,

	QUIT_BUTTON,
	QUIT_BUTTON_HL,

	NETWORK_CON_TOP,
	NETWORK_CON_TOP_HL,

	NETWORK_CON_BOTT,
	NETWORK_CON_BOTT_HL,

	NET_BUTTON_TOP,
	NET_BUTTON_TOP_HL,
	NET_BUTTON_BOTT,
	NET_BUTTON_BOTT_HL,

	SHIP_DESC,

	CONFIRM_PC,
	CANCEL_PC,
	UP_DOWN_PC,
	SPECIAL_PC,

	CONFIRM_XB,
	CANCEL_XB,
	UP_DOWN_XB,
	SPECIAL_XB,

	CONFIRM_PS,
	CANCEL_PS,
	UP_DOWN_PS,
	SPECIAL_PS,
};

#if defined(__cplusplus)
}
#endif

#endif /* UQM_SUPERMELEE_MELEE_H_ */

