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

#ifndef SIS_H_INCL__
#define SIS_H_INCL__

#include "libs/compiler.h"
#include "libs/gfxlib.h"
#include "planets/elemdata.h"
		// for NUM_ELEMENT_CATEGORIES
#include "units.h"
                // for RESOLUTION_FACTOR

#if defined(__cplusplus)
extern "C" {
#endif

#define CLEAR_SIS_RADAR (1 << 2)
#define DRAW_SIS_DISPLAY (1 << 3)

#define UNDEFINED_DELTA 0x7FFF

#define NUM_DRIVE_SLOTS 11
#define NUM_JET_SLOTS 8
#define NUM_MODULE_SLOTS 16

#define CREW_POD_CAPACITY 50
#define STORAGE_BAY_CAPACITY 500 /* km cubed */
#define FUEL_TANK_SCALE 100
#define FUEL_TANK_CAPACITY (50 * FUEL_TANK_SCALE)
#define HEFUEL_TANK_CAPACITY (100 * FUEL_TANK_SCALE)
#define MODULE_COST_SCALE 50

#define CREW_EXPENSE_THRESHOLD 1000

#define CREW_PER_ROW 5
#define SBAY_MASS_PER_ROW 50

#define MAX_FUEL_BARS 10
#define FUEL_VOLUME_PER_ROW (HEFUEL_TANK_CAPACITY / MAX_FUEL_BARS)
#define FUEL_RESERVE (10 * FUEL_TANK_SCALE)
#define FUEL_COST_RU 20

#define IP_SHIP_THRUST_INCREMENT 8
#define IP_SHIP_TURN_WAIT 17
#define IP_SHIP_TURN_DECREMENT 2

#define BIO_CREDIT_VALUE 2

enum
{
	PLANET_LANDER = 0,
		/* thruster types */
	FUSION_THRUSTER,
		/* jet types */
	TURNING_JETS,
		/* module types */
	CREW_POD,
	STORAGE_BAY,
	FUEL_TANK,
	HIGHEFF_FUELSYS,
	DYNAMO_UNIT,
	SHIVA_FURNACE,
	GUN_WEAPON,
	BLASTER_WEAPON,
	CANNON_WEAPON,
	TRACKING_SYSTEM,
	ANTIMISSILE_DEFENSE,

	NUM_PURCHASE_MODULES,

	BOMB_MODULE_0 = NUM_PURCHASE_MODULES,
	BOMB_MODULE_1,
	BOMB_MODULE_2,
	BOMB_MODULE_3,
	BOMB_MODULE_4,
	BOMB_MODULE_5,

	NUM_MODULES /* must be last entry */
};

#define EMPTY_SLOT NUM_MODULES
#define NUM_BOMB_MODULES 10

#define SAFE_PAD SAFE_NEG (3)

#define DRIVE_SIDE_X (RES_SCALE (31) - SAFE_PAD)
#define DRIVE_SIDE_Y RES_SCALE (56)
#define DRIVE_TOP_X (RES_SCALE (33) - SAFE_PAD)
#define DRIVE_TOP_Y (RES_SCALE (86) - DOS_NUM_SCL (21))

#define JET_SIDE_X (RES_SCALE (71) - SAFE_PAD)
#define JET_SIDE_Y RES_SCALE (48)
#define JET_TOP_X (RES_SCALE (70) - SAFE_PAD)
#define JET_TOP_Y (RES_SCALE (94) - DOS_NUM_SCL (21))

#define MODULE_SIDE_X (RES_SCALE (17) - SAFE_PAD)
#define MODULE_SIDE_Y RES_SCALE (14)
#define MODULE_TOP_X (RES_SCALE (17) - SAFE_PAD)
#define MODULE_TOP_Y (RES_SCALE (117) - DOS_NUM_SCL (21))

#define SHIP_PIECE_OFFSET RES_SCALE (12)

#define MAX_BUILT_SHIPS 12
		/* Maximum number of ships escorting the SIS */
#define MAX_LANDERS 10

#define SUPPORT_SHIP_PTS \
	{3 +  0, 30 + (2 * 16)}, \
	{3 + 42, 30 + (2 * 16)}, \
	{3 +  0, 30 + (3 * 16)}, \
	{3 + 42, 30 + (3 * 16)}, \
	{3 +  0, 30 + (1 * 16)}, \
	{3 + 42, 30 + (1 * 16)}, \
	{3 +  0, 30 + (4 * 16)}, \
	{3 + 42, 30 + (4 * 16)}, \
	{3 +  0, 30 + (0 * 16)}, \
	{3 + 42, 30 + (0 * 16)}, \
	{3 +  0, 30 + (5 * 16)}, \
	{3 + 42, 30 + (5 * 16)},

#define LANDER_DOS_X 206
#define LANDER_DOS_Y 59

#define LANDER_DOS_PTS \
	{LANDER_DOS_X,      LANDER_DOS_Y     }, \
	{LANDER_DOS_X + 14, LANDER_DOS_Y -  6}, \
	{LANDER_DOS_X + 14, LANDER_DOS_Y +  9}, \
	{LANDER_DOS_X,      LANDER_DOS_Y - 15}, \
	{LANDER_DOS_X,      LANDER_DOS_Y + 15}, \
	{LANDER_DOS_X - 14, LANDER_DOS_Y -  9}, \
	{LANDER_DOS_X - 14, LANDER_DOS_Y +  6}, \
	{LANDER_DOS_X + 14, LANDER_DOS_Y - 21}, \
	{LANDER_DOS_X + 14, LANDER_DOS_Y + 24}, \
	{LANDER_DOS_X - 28, LANDER_DOS_Y -  3},

#define SIS_NAME_SIZE 32
#define LEGACY_SIS_NAME_SIZE 16
#define LEGACY_SAVE_NAME_SIZE 32
#define LEGACY_SAVE_NAME_CHECKER "superbutcherX !"
#define SAVE_CHECKER_SIZE SIS_NAME_SIZE
#define LEGACY_SAVE_CHECKER_SIZE LEGACY_SIS_NAME_SIZE

typedef struct
{
	SDWORD log_x, log_y;

	DWORD ResUnits;

	DWORD FuelOnBoard;
	COUNT CrewEnlisted;
			// Number of crew on board, not counting the captain.
			// Set to (COUNT) ~0 to indicate game over.
	COUNT TotalElementMass, TotalBioMass;

	BYTE ModuleSlots[NUM_MODULE_SLOTS];
	BYTE DriveSlots[NUM_DRIVE_SLOTS];
	BYTE JetSlots[NUM_JET_SLOTS];

	BYTE NumLanders;

	COUNT ElementAmounts[NUM_ELEMENT_CATEGORIES];

	UNICODE ShipName[SIS_NAME_SIZE];
	UNICODE CommanderName[SIS_NAME_SIZE];
	UNICODE PlanetName[SIS_NAME_SIZE];
	BYTE Difficulty, Extended, Nomad;
	SDWORD Seed;
} SIS_STATE;

#define OVERRIDE_LANDER_FLAGS (1 << 7)
#define AFTER_BOMB_INSTALLED (1 << 7)

extern void RepairSISBorder (void);
extern void InitSISContexts (void);
extern void DrawSISFrame (void);
extern void ClearSISRect (BYTE ClearFlags);
extern void SetFlashRect (const RECT *pRect, BOOLEAN purple);
extern void SetAdditionalRect (const RECT *pRect, COUNT number);
extern void DumpAdditionalRect (void);
extern void PreUpdateFlashRect (void);
extern void PostUpdateFlashRect (void);
extern void PauseFlash (void);
extern void ContinueFlash (void);

#define SFR_MENU_3DO ((RECT*)~0L)
#define SFR_MENU_ANY ((RECT*)~1L)
#define SFR_MENU_NON ((RECT*)~2L)
extern void DrawHyperCoords (POINT puniverse);
extern void DrawSaveInfo (SIS_STATE SisState);
extern void DrawSISTitle (UNICODE *pStr);

// Flags for DrawSISMessageEx (may be OR'ed):
#define DSME_NONE     0
#define DSME_SETFR    (1 << 0)
		// Set the flash rectangle to the message area.
#define DSME_CLEARFR  (1 << 1)
		// Disable the flash rectangle.
#define DSME_BLOCKCUR (1 << 2)
		// Use a block cursor instead of an insertion point cursor,
		// when editing in the message field.
#define DSME_MYCOLOR  (1 << 3)
		// Use the current foreground color, instead of the default.
extern BOOLEAN DrawSISMessageEx (const UNICODE *pStr, SIZE CurPos,
		SIZE ExPos, COUNT flags);

extern void DrawSISMessage (const UNICODE *pStr);
extern void DateToString (char *buf, size_t bufLen,
		BYTE month_index, BYTE day_index, COUNT year_index);

// Returned RECT is relative to the StatusContext
extern void GetStatusMessageRect (RECT *r);
extern void DrawStatusMessage (const UNICODE *pStr);
typedef enum
{
	SMM_UNDEFINED = 0,
	SMM_DATE,
	SMM_RES_UNITS,
	SMM_CREDITS,
	SMM_WARNING,
	SMM_ALERT,

	SMM_DEFAULT = SMM_DATE,
} StatMsgMode;
// Sets the new mode and return the previous
extern StatMsgMode SetStatusMessageMode (StatMsgMode);

extern void DrawLanders (void);
extern void DrawStorageBays (BOOLEAN Refresh);
extern void GetGaugeRect (RECT *pRect, BOOLEAN IsCrewRect);
extern void DrawFlagshipStats (void);
void DrawAutoPilotMessage (BOOLEAN Reset);

extern void DeltaSISGauges (SIZE crew_delta, SDWORD fuel_delta, int
		resunit_delta);

extern COUNT GetCrewCount (void);
extern COUNT GetModuleCrewCapacity (BYTE moduleType);

extern COUNT GetCrewPodCapacity (void);
extern COUNT GetCPodCapacity (POINT *ppt);

extern COUNT GetModuleStorageCapacity (BYTE moduleType);
extern COUNT GetStorageBayCapacity (void);
extern COUNT GetSBayCapacity (POINT *ppt);

extern DWORD GetModuleFuelCapacity (BYTE moduleType);
extern DWORD GetFuelTankCapacity (void);

extern COUNT CountSISPieces (BYTE piece_type);

extern void DrawFlagshipName (BOOLEAN InStatusArea, bool NewGame);
extern void DrawCaptainsName (bool NewGame);

extern void DrawFuelInFTanks (BOOLEAN isOutfit);
extern FRAME hdFuelFrame;// Frame for HD fuel with smooth gradient

typedef enum
{
	SIS_MAIN_FRAME = 0,
	SIS_STAT_FRAME,
	DEVICE_CARGO_FRAME,
	SIS_STAT_REPAIR_FRAME,
	SIS_STAT_SUMM_FRAME,
	SIS_MSG_FRAME,
	SIS_TITLE_FRAME,
	CAPTAIN_FRAME,
	CAP_NAME_FRAME,
	STAT_MSG_FRAME,
	SIS_RADAR_FRAME,
	TEXT_LABEL_FRAME,
	SIS_REPAIR_FRAME,
	LANDER_3DO_FRAME,
	LANDER_DOS_FRAME,
	SIS_COMM_FRAME,
} BORDER_ORDER;

typedef enum
{
	DOS_MENU_HILITE = 0,
	SPECIAL_BEVEL,
	THIN_INNER_BEVEL = SPECIAL_BEVEL + 8,
	THICK_INNER_BEVEL = THIN_INNER_BEVEL + 8,
	THIN_OUTER_BEVEL = THICK_INNER_BEVEL + 8,
	THICK_OUTER_BEVEL = THIN_OUTER_BEVEL + 8,
	THICK_OUT_INV_BEVEL = THICK_OUTER_BEVEL + 8,
} BEVEL_LEVEL;

#if defined(__cplusplus)
}
#endif

#endif /* SIS_H_INCL__ */
