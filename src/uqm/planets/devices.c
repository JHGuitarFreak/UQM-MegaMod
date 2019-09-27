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

#include "../build.h"
#include "../colors.h"
#include "../gendef.h"
#include "../starmap.h"
#include "../encount.h"
#include "../gamestr.h"
#include "../controls.h"
#include "../save.h"
#include "../settings.h"
#include "../shipcont.h"
#include "../setup.h"
#include "../state.h"
#include "../sis.h"
		// for ClearSISRect()
#include "../grpinfo.h"
#include "../sounds.h"
#include "../util.h"
#include "../hyper.h"
		// for SaveSisHyperState()
#include "planets.h"
		// for SaveSolarSysLocation() and tests
#include "libs/strlib.h"
#include "../../options.h"
#include "libs/graphics/gfx_common.h"
                // for scaling down devices in HD


// If DEBUG_DEVICES is defined, the device list shown in the game will
// include the pictures of all devices defined, regardless of which
// devices the player actually possesses.
//#define DEBUG_DEVICES

#define DEVICE_ICON_WIDTH  RES_STAT_SCALE(16) // JMS_GFX
#define DEVICE_ICON_HEIGHT RES_STAT_SCALE(16) // JMS_GFX

#define DEVICE_ORG_Y       RES_STAT_SCALE(33) // JMS_GFX
#define DEVICE_SPACING_Y   (DEVICE_ICON_HEIGHT + RES_STAT_SCALE(2)) // JMS_GFX

#define DEVICE_COL_0       RES_STAT_SCALE(4) // JMS_GFX
#define DEVICE_COL_1       RES_STAT_SCALE(40) // JMS_GFX

#define DEVICE_SEL_ORG_X  (DEVICE_COL_0 + DEVICE_ICON_WIDTH)
#define DEVICE_SEL_WIDTH  (FIELD_WIDTH + RES_STAT_SCALE(2) - DEVICE_SEL_ORG_X) // JMS_GFX

#define ICON_OFS_Y         RES_BOOL(1, 11) // JMS_GFX
#define NAME_OFS_Y         RES_STAT_SCALE(2) // JMS_GFX
#define TEXT_BASELINE      RES_STAT_SCALE(6) // JMS_GFX
#define TEXT_SPACING_Y     RES_STAT_SCALE(7) // JMS_GFX

#define MAX_VIS_DEVICES    ((RES_STAT_SCALE(129) - DEVICE_ORG_Y) / DEVICE_SPACING_Y) // JMS_GFX


typedef enum
{
	DEVICE_FAILURE = 0,
	DEVICE_SUCCESS,
	DEVICE_SUCCESS_NO_SOUND,
} DeviceStatus;

typedef struct
{
	BYTE list[NUM_DEVICES];
			// List of all devices player has
	COUNT count;
			// Number of devices in the list
	COUNT topIndex;
			// Index of the top device displayed
} DEVICES_STATE;


#if 0
static void
EraseDevicesBackground (void)
{
	RECT r;

	r.corner.x = RES_STAT_SCALE(2 + 1); // JMS_GFX
	r.extent.width = FIELD_WIDTH - RES_STAT_SCALE(1); // JMS_GFX
	r.corner.y = DEVICE_ORG_Y;
	r.extent.height = MAX_VIS_DEVICES * DEVICE_SPACING_Y;
	SetContextForeGroundColor (DEVICES_BACK_COLOR);
	DrawFilledRectangle (&r);
}
#endif

static void
DrawDevice (COUNT device, COUNT pos, bool selected)
{
	RECT r;
	TEXT t;

	t.align = ALIGN_CENTER;
	t.baseline.x = DEVICE_COL_1;

	r.extent.width = DEVICE_SEL_WIDTH;
	r.extent.height = TEXT_SPACING_Y * 2;
	r.corner.x = DEVICE_SEL_ORG_X - IF_HD(8);

	// draw line background
	r.corner.y = DEVICE_ORG_Y + pos * DEVICE_SPACING_Y + NAME_OFS_Y;
	SetContextForeGroundColor (selected ?
			DEVICES_SELECTED_BACK_COLOR : DEVICES_BACK_COLOR);
	DrawFilledRectangle (&r);

	SetContextFont (TinyFont);

	// print device name
	SetContextForeGroundColor (selected ?
			DEVICES_SELECTED_NAME_COLOR : DEVICES_NAME_COLOR);
	t.baseline.y = r.corner.y + TEXT_BASELINE;
	t.pStr = GAME_STRING (device + DEVICE_STRING_BASE + 1);
	t.CharCount = utf8StringPos (t.pStr, ' ');
	font_DrawText (&t);
	t.baseline.y += TEXT_SPACING_Y;
	t.pStr = skipUTF8Chars (t.pStr, t.CharCount + 1);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
}

static void
DrawDevicesDisplay (DEVICES_STATE *devState)
{
	TEXT t;
	RECT r;
	STAMP s;
	COORD cy;
	COUNT i;

	r.corner.x = 2; 
	r.corner.y = RES_STAT_SCALE(20);
	r.extent.width = FIELD_WIDTH + 1;
	// XXX: Shouldn't the height be 1 less? This draws the bottom border
	//   1 pixel too low. Or if not, why do we need another box anyway?
	r.extent.height = (RES_STAT_SCALE(129) - r.corner.y) + IF_HD(19);
	DrawStarConBox (&r, 1,
			SHADOWBOX_MEDIUM_COLOR, SHADOWBOX_DARK_COLOR,
			TRUE, DEVICES_BACK_COLOR);

	DrawBorder(12, FALSE);

	// print the "DEVICES" title
	SetContextFont (StarConFont);
	t.baseline.x = (STATUS_WIDTH >> 1) - RES_STAT_SCALE(1);
	t.baseline.y = r.corner.y + RES_STAT_SCALE(7);
	t.align = ALIGN_CENTER;
	t.pStr = GAME_STRING (DEVICE_STRING_BASE);
	t.CharCount = (COUNT)~0;
	SetContextForeGroundColor (DEVICES_SELECTED_NAME_COLOR);
	font_DrawText (&t);

	s.origin.x = DEVICE_COL_0;
	cy = DEVICE_ORG_Y;

	// draw device icons and print names
	for (i = 0; i < MAX_VIS_DEVICES; ++i, cy += DEVICE_SPACING_Y)
	{
		COUNT devIndex = devState->topIndex + i;

		if (devIndex >= devState->count)
			break;

		// draw device icon
		s.origin.y = cy + ICON_OFS_Y;
		s.frame = SetAbsFrameIndex (MiscDataFrame,
				77 + devState->list[devIndex]);
		
		if (!IS_HD) {
			DrawStamp (&s);			
		} else {
			int oldMode, oldScale;
			oldMode = SetGraphicScaleMode (TFB_SCALE_BILINEAR);
			oldScale = SetGraphicScale ((int)(GSCALE_IDENTITY / 2));
			DrawStamp (&s);
			SetGraphicScale (oldScale);
			SetGraphicScaleMode (oldMode);
		}

		DrawDevice (devState->list[devIndex], i, false);
	}
}

static void
DrawDevices (DEVICES_STATE *devState, COUNT OldDevice, COUNT NewDevice)
{
	BatchGraphics ();

	SetContext (StatusContext);

	if (OldDevice > NUM_DEVICES)
	{	// Asked for the initial display or refresh
		DrawDevicesDisplay (devState);

		// do not draw unselected again this time
		OldDevice = NewDevice;
	}

	if (OldDevice != NewDevice)
	{	// unselect the previous element
		DrawDevice (devState->list[OldDevice], OldDevice - devState->topIndex,
				false);
	}

	if (NewDevice < NUM_DEVICES)
	{	// select the new element
		DrawDevice (devState->list[NewDevice], NewDevice - devState->topIndex,
				true);
	}

	UnbatchGraphics ();
}

// Returns TRUE if the broadcaster has been successfully activated,
// and FALSE otherwise.
static BOOLEAN
UseCaster (void)
{
	if (inHQSpace ())
	{
		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
		{
			SET_GAME_STATE (USED_BROADCASTER, 1);
			return TRUE;
		}
		return FALSE;
	}

	if (LOBYTE (GLOBAL (CurrentActivity)) != IN_INTERPLANETARY
			|| !playerInSolarSystem ())
		return FALSE;

	if (playerInPlanetOrbit ()
			&& matchWorld (pSolarSysState, pSolarSysState->pOrbitalDesc,
				pSolarSysState->SunDesc[0].PlanetByte, MATCH_PLANET)
			&& CurStarDescPtr->Index == CHMMR_DEFINED
			&& !GET_GAME_STATE (CHMMR_UNLEASHED))
	{
		// In orbit around the Chenjesu/Mmrnmhrm home planet.
		NextActivity |= CHECK_LOAD;  /* fake a load game */
		GLOBAL (CurrentActivity) |= START_ENCOUNTER;

		EncounterGroup = 0;
		PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
		ReinitQueue (&GLOBAL (ip_group_q));
		assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

		SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
		SaveSolarSysLocation ();
		return TRUE;
	}

	{
		BOOLEAN FoundIlwrath;
		HIPGROUP hGroup;

		FoundIlwrath = (CurStarDescPtr->Index == ILWRATH_DEFINED)
				&& StartSphereTracking (ILWRATH_SHIP);
				// In the Ilwrath home system and they are alive?

		if (!FoundIlwrath &&
				(hGroup = GetHeadLink (&GLOBAL (ip_group_q))))
		{
			// Is an Ilwrath ship in the system?
			IP_GROUP *GroupPtr;

			GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
			FoundIlwrath = (GroupPtr->race_id == ILWRATH_SHIP);
			UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);
		}

		if (FoundIlwrath)
		{
			NextActivity |= CHECK_LOAD; /* fake a load game */
			GLOBAL (CurrentActivity) |= START_ENCOUNTER;

			EncounterGroup = 0;
			PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
			ReinitQueue (&GLOBAL (ip_group_q));
			assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

			if (CurStarDescPtr->Index == ILWRATH_DEFINED)
			{
				// Ilwrath home system.
				SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 4);
			}
			else
			{
				// Ilwrath ship.
				SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 5);
			}
			
			if (playerInPlanetOrbit ())
				SaveSolarSysLocation ();
			return TRUE;
		}
	}

	return FALSE;
}

static DeviceStatus
InvokeDevice (BYTE which_device)
{
	BYTE val;

	switch (which_device)
	{
		case ROSY_SPHERE_DEVICE:
			val = GET_GAME_STATE (ULTRON_CONDITION);
			if (val)
			{
				SET_GAME_STATE (ULTRON_CONDITION, val + 1);
				SET_GAME_STATE (ROSY_SPHERE_ON_SHIP, 0);
				SET_GAME_STATE (DISCUSSED_ULTRON, 0);
				SET_GAME_STATE (SUPOX_ULTRON_HELP, 0);
				return DEVICE_SUCCESS;
			}
			break;
		case ARTIFACT_2_DEVICE:
			break;
		case ARTIFACT_3_DEVICE:
			break;
		case SUN_EFFICIENCY_DEVICE:
			if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
					&& playerInPlanetOrbit ())
			{
				PlayMenuSound (MENU_SOUND_INVOKED);
				SleepThreadUntil (FadeScreen (FadeAllToWhite, ONE_SECOND * 1)
						+ (ONE_SECOND * 2));
				if (CurStarDescPtr->Index != CHMMR_DEFINED
						|| !matchWorld (pSolarSysState,
								pSolarSysState->pOrbitalDesc,
								pSolarSysState->SunDesc[0].PlanetByte, MATCH_PLANET))
				{
					FadeScreen (FadeAllToColor, ONE_SECOND * 2);
				}
				else
				{
					SET_GAME_STATE (CHMMR_EMERGING, 1);

					EncounterGroup = 0;
					GLOBAL (CurrentActivity) |= START_ENCOUNTER;

					PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
					ReinitQueue (&GLOBAL (ip_group_q));
					assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

					CloneShipFragment (CHMMR_SHIP,
							&GLOBAL (npc_built_ship_q), 0);
				}
				return DEVICE_SUCCESS_NO_SOUND;
			}
			break;
		case UTWIG_BOMB_DEVICE:
			SET_GAME_STATE (UTWIG_BOMB, 0);
			GLOBAL (CurrentActivity) &= ~IN_BATTLE;
			GLOBAL_SIS (CrewEnlisted) = (COUNT)~0;
			DeathBySuicide = TRUE;
			return DEVICE_SUCCESS;
		case ULTRON_0_DEVICE:
			break;
		case ULTRON_1_DEVICE:
			break;
		case ULTRON_2_DEVICE:
			break;
		case ULTRON_3_DEVICE:
			break;
		case MAIDENS_DEVICE:
			break;
		case TALKING_PET_DEVICE:
			NextActivity |= CHECK_LOAD; /* fake a load game */
			GLOBAL (CurrentActivity) |= START_ENCOUNTER;
			SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 0);
			if (inHQSpace ())
			{
				if (GetHeadEncounter ())
				{
					SET_GAME_STATE (SHIP_TO_COMPEL, 1);
				}
				GLOBAL (CurrentActivity) &= ~IN_BATTLE;

				SaveSisHyperState ();
			}
			else
			{
				EncounterGroup = 0;
				if (GetHeadLink (&GLOBAL (ip_group_q)))
				{
					SET_GAME_STATE (SHIP_TO_COMPEL, 1);

					PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
					ReinitQueue (&GLOBAL (ip_group_q));
					assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);
				}

				if (CurStarDescPtr->Index == SAMATRA_DEFINED)
				{
					SET_GAME_STATE (READY_TO_CONFUSE_URQUAN, 1);
				}
				if (playerInPlanetOrbit ())
					SaveSolarSysLocation ();
			}
			return DEVICE_SUCCESS;
		case AQUA_HELIX_DEVICE:
			val = GET_GAME_STATE (ULTRON_CONDITION);
			if (val)
			{
				SET_GAME_STATE (ULTRON_CONDITION, val + 1);
				SET_GAME_STATE (AQUA_HELIX_ON_SHIP, 0);
				SET_GAME_STATE (DISCUSSED_ULTRON, 0);
				SET_GAME_STATE (SUPOX_ULTRON_HELP, 0);
				return DEVICE_SUCCESS;
			}
			break;
		case CLEAR_SPINDLE_DEVICE:
			val = GET_GAME_STATE (ULTRON_CONDITION);
			if (val)
			{
				SET_GAME_STATE (ULTRON_CONDITION, val + 1);
				SET_GAME_STATE (CLEAR_SPINDLE_ON_SHIP, 0);
				SET_GAME_STATE (DISCUSSED_ULTRON, 0);
				SET_GAME_STATE (SUPOX_ULTRON_HELP, 0);
				return DEVICE_SUCCESS;
			}
			break;
		case UMGAH_HYPERWAVE_DEVICE:
		case BURVIX_HYPERWAVE_DEVICE:
			if (UseCaster ())
				return DEVICE_SUCCESS;
			break;
		case TAALO_PROTECTOR_DEVICE:
			break;
		case EGG_CASING0_DEVICE:
		case EGG_CASING1_DEVICE:
		case EGG_CASING2_DEVICE:
			break;
		case SYREEN_SHUTTLE_DEVICE:
			break;
		case VUX_BEAST_DEVICE:
			break;
		case DESTRUCT_CODE_DEVICE:
			break;
		case PORTAL_SPAWNER_DEVICE:
#define PORTAL_FUEL_COST (DIF_CASE(10, 5, 20) * FUEL_TANK_SCALE)
			if (inHyperSpace ()
					&& GLOBAL_SIS (FuelOnBoard) >= PORTAL_FUEL_COST)
			{
				/* No DeltaSISGauges because the flagship picture
				 * is currently obscured.
				 */
				if (!optInfiniteFuel)
					GLOBAL_SIS (FuelOnBoard) -= PORTAL_FUEL_COST;

				SET_GAME_STATE (PORTAL_COUNTER, 1);
				return DEVICE_SUCCESS;
			}
			break;
		case URQUAN_WARP_DEVICE:
			break;
		case LUNAR_BASE_DEVICE:
			break;
	}

	return DEVICE_FAILURE;
}

static BOOLEAN
DoManipulateDevices (MENU_STATE *pMS)
{
	DEVICES_STATE *devState = pMS->privData;
	BOOLEAN select, cancel, back, forward;
	BOOLEAN pagefwd, pageback;
	
	select = PulsedInputState.menu[KEY_MENU_SELECT];
	cancel = PulsedInputState.menu[KEY_MENU_CANCEL];
	back = PulsedInputState.menu[KEY_MENU_UP] ||
			PulsedInputState.menu[KEY_MENU_LEFT];
	forward = PulsedInputState.menu[KEY_MENU_DOWN]
			|| PulsedInputState.menu[KEY_MENU_RIGHT];
	pagefwd = PulsedInputState.menu[KEY_MENU_PAGE_DOWN];
	pageback = PulsedInputState.menu[KEY_MENU_PAGE_UP];

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return FALSE;

	if (cancel)
	{
		return FALSE;
	}
	else if (select)
	{
		DeviceStatus status;

		status = InvokeDevice (devState->list[pMS->CurState]);
		if (status == DEVICE_FAILURE)
			PlayMenuSound (MENU_SOUND_FAILURE);
		else if (status == DEVICE_SUCCESS)
			PlayMenuSound (MENU_SOUND_INVOKED);

		return (status == DEVICE_FAILURE);
	}
	else
	{
		SIZE NewTop;
		SIZE NewState;

		NewTop = devState->topIndex;
		NewState = pMS->CurState;
		
		if (back)
			--NewState;
		else if (forward)
			++NewState;
		else if (pagefwd)
			NewState += MAX_VIS_DEVICES;
		else if (pageback)
			NewState -= MAX_VIS_DEVICES;

		if (NewState < 0)
			NewState = 0;
		else if (NewState >= devState->count)
			NewState = devState->count - 1;

		if (NewState < NewTop || NewState >= NewTop + MAX_VIS_DEVICES)
			NewTop = NewState - NewState % MAX_VIS_DEVICES;

		if (NewState != pMS->CurState)
		{
			if (NewTop != devState->topIndex)
			{	// redraw the display
				devState->topIndex = NewTop;
				DrawDevices (devState, (COUNT)~0, NewState);
			}
			else
			{	// move selection to new device
				DrawDevices (devState, pMS->CurState, NewState);
			}
			pMS->CurState = NewState;
		}

		SleepThread (ONE_SECOND / 30);
	}

	return TRUE;
}

SIZE
InventoryDevices (BYTE *pDeviceMap, COUNT Size)
{
	BYTE i;
	SIZE DevicesOnBoard;
	
	DevicesOnBoard = 0;
	for (i = 0; i < NUM_DEVICES && Size > 0; ++i)
	{
		BYTE DeviceState;

		DeviceState = 0;
		switch (i)
		{
			case ROSY_SPHERE_DEVICE:
				DeviceState = GET_GAME_STATE (ROSY_SPHERE_ON_SHIP);
				break;
			case ARTIFACT_2_DEVICE:
				DeviceState = GET_GAME_STATE (WIMBLIS_TRIDENT_ON_SHIP);
				break;
			case ARTIFACT_3_DEVICE:
				DeviceState = GET_GAME_STATE (GLOWING_ROD_ON_SHIP);
				break;
			case SUN_EFFICIENCY_DEVICE:
				DeviceState = GET_GAME_STATE (SUN_DEVICE_ON_SHIP);
				break;
			case UTWIG_BOMB_DEVICE:
				DeviceState = GET_GAME_STATE (UTWIG_BOMB_ON_SHIP);
				break;
			case ULTRON_0_DEVICE:
				DeviceState = (GET_GAME_STATE (ULTRON_CONDITION) == 1);
				break;
			case ULTRON_1_DEVICE:
				DeviceState = (GET_GAME_STATE (ULTRON_CONDITION) == 2);
				break;
			case ULTRON_2_DEVICE:
				DeviceState = (GET_GAME_STATE (ULTRON_CONDITION) == 3);
				break;
			case ULTRON_3_DEVICE:
				DeviceState = (GET_GAME_STATE (ULTRON_CONDITION) == 4);
				break;
			case MAIDENS_DEVICE:
				DeviceState = GET_GAME_STATE (MAIDENS_ON_SHIP);
				break;
			case TALKING_PET_DEVICE:
				DeviceState = GET_GAME_STATE (TALKING_PET_ON_SHIP);
				break;
			case AQUA_HELIX_DEVICE:
				DeviceState = GET_GAME_STATE (AQUA_HELIX_ON_SHIP);
				break;
			case CLEAR_SPINDLE_DEVICE:
				DeviceState = GET_GAME_STATE (CLEAR_SPINDLE_ON_SHIP);
				break;
			case UMGAH_HYPERWAVE_DEVICE:
				DeviceState = GET_GAME_STATE (UMGAH_BROADCASTERS_ON_SHIP);
				break;
			case TAALO_PROTECTOR_DEVICE:
				DeviceState = GET_GAME_STATE (TAALO_PROTECTOR_ON_SHIP);
				break;
			case EGG_CASING0_DEVICE:
				DeviceState = GET_GAME_STATE (EGG_CASE0_ON_SHIP);
				break;
			case EGG_CASING1_DEVICE:
				DeviceState = GET_GAME_STATE (EGG_CASE1_ON_SHIP);
				break;
			case EGG_CASING2_DEVICE:
				DeviceState = GET_GAME_STATE (EGG_CASE2_ON_SHIP);
				break;
			case SYREEN_SHUTTLE_DEVICE:
				DeviceState = GET_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP);
				break;
			case VUX_BEAST_DEVICE:
				DeviceState = GET_GAME_STATE (VUX_BEAST_ON_SHIP);
				break;
			case DESTRUCT_CODE_DEVICE:
#ifdef NEVER
				DeviceState = GET_GAME_STATE (DESTRUCT_CODE_ON_SHIP);
#endif /* NEVER */
				break;
			case PORTAL_SPAWNER_DEVICE:
				DeviceState = GET_GAME_STATE (PORTAL_SPAWNER_ON_SHIP);
				break;
			case URQUAN_WARP_DEVICE:
				DeviceState = GET_GAME_STATE (PORTAL_KEY_ON_SHIP);
				break;
			case BURVIX_HYPERWAVE_DEVICE:
				DeviceState = GET_GAME_STATE (BURV_BROADCASTERS_ON_SHIP);
				break;
			case LUNAR_BASE_DEVICE:
				DeviceState = GET_GAME_STATE (MOONBASE_ON_SHIP);
				break;
		}

#ifndef DEBUG_DEVICES
		if (DeviceState)
#endif /* DEBUG_DEVICES */
		{
			*pDeviceMap++ = i;
			++DevicesOnBoard;
			--Size;
		}
	}
	
	return DevicesOnBoard;
}

BOOLEAN
DevicesMenu (void)
{
	MENU_STATE MenuState;
	DEVICES_STATE DevicesState;

	memset (&MenuState, 0, sizeof MenuState);
	MenuState.privData = &DevicesState;

	memset (&DevicesState, 0, sizeof DevicesState);

	DevicesState.count = InventoryDevices (DevicesState.list, NUM_DEVICES);
	if (!DevicesState.count)
		return FALSE;

	DrawDevices (&DevicesState, (COUNT)~0, MenuState.CurState);

	SetMenuSounds (MENU_SOUND_ARROWS | MENU_SOUND_PAGE,
			MENU_SOUND_SELECT);

	MenuState.InputFunc = DoManipulateDevices;
	DoInput (&MenuState, TRUE);

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

	if (GLOBAL_SIS (CrewEnlisted) != (COUNT)~0
			&& !(GLOBAL (CurrentActivity) & CHECK_ABORT))
	{
		ClearSISRect (DRAW_SIS_DISPLAY);

		if (!GET_GAME_STATE (PORTAL_COUNTER)
				&& !(GLOBAL (CurrentActivity) & START_ENCOUNTER)
				&& GLOBAL_SIS (CrewEnlisted) != (COUNT)~0)
			return TRUE;
	}
	
	return FALSE;
}

