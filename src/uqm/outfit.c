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

#include "options.h"
#include "colors.h"
#include "controls.h"
#include "menustat.h"
#include "gameopt.h"
#include "gamestr.h"
#include "resinst.h"
#include "nameref.h"
#include "settings.h"
#include "starbase.h"
#include "setup.h"
#include "sis.h"
#include "units.h"
#include "sounds.h"
#include "planets/planets.h"
		// for xxx_DISASTER
#include "libs/graphics/gfx_common.h"

// How manyeth .png in the module.ani file is the first lander shield.
#define SHIELD_LOCATION_IN_MODULE_ANI (RES_BOOL(5, 9))

enum
{
	OUTFIT_FUEL,
	OUTFIT_MODULES,
	OUTFIT_SAVELOAD,
	OUTFIT_EXIT,
	OUTFIT_DOFUEL
};


static void
DrawModuleStrings (MENU_STATE *pMS, BYTE NewModule)
{
	RECT r;
	STAMP s;
	CONTEXT OldContext;

	OldContext = SetContext (StatusContext);
	GetContextClipRect (&r);
	s.origin.x = RADAR_X - r.corner.x;
	s.origin.y = RADAR_Y - r.corner.y - 19 * RESOLUTION_FACTOR; // JMS_GFX;
	r.corner.x = s.origin.x - 1;
	r.corner.y = s.origin.y - 11;
	r.extent.width = RADAR_WIDTH + 2;
	r.extent.height = 11 + 20 * RESOLUTION_FACTOR; // JMS_GFX;
	BatchGraphics ();
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08));
	DrawFilledRectangle (&r);
	DrawBorder(8, FALSE);
	if (NewModule >= EMPTY_SLOT)
	{
		r.corner = s.origin;
		r.corner.y += 19 * RESOLUTION_FACTOR; // JMS_GFX
		r.extent.width = RADAR_WIDTH;
		r.extent.height = RADAR_HEIGHT;
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x00), 0x00));
		DrawFilledRectangle (&r);
	}
	else if (pMS->CurFrame)
	{
		TEXT t;
		UNICODE buf[40];
		s.frame = SetAbsFrameIndex (pMS->CurFrame, NewModule);
		DrawStamp (&s);
		t.baseline.x = s.origin.x + RADAR_WIDTH - RES_STAT_SCALE(2) - RESOLUTION_FACTOR;
		t.baseline.y = s.origin.y + RADAR_HEIGHT - RES_STAT_SCALE(2) + 14 * RESOLUTION_FACTOR; // JMS_GFX;
		t.align = ALIGN_RIGHT;
		t.CharCount = (COUNT)~0;
		t.pStr = buf;
		sprintf (buf, "%u",
				GLOBAL (ModuleCost[NewModule]) * MODULE_COST_SCALE);

		if ((GLOBAL_SIS (ResUnits)) > (DWORD)((GLOBAL (ModuleCost[NewModule]) * MODULE_COST_SCALE))) {
			sprintf (buf, "%u", GLOBAL (ModuleCost[NewModule]) * MODULE_COST_SCALE);
			SetContextForeGroundColor (
					BUILD_COLOR (MAKE_RGB15 (0x00, 0x1F, 0x00), 0x02));
		} else {
			sprintf (buf, "(%u)", GLOBAL (ModuleCost[NewModule]) * MODULE_COST_SCALE);
			SetContextForeGroundColor (
					BUILD_COLOR (MAKE_RGB15 (0x1F, 0x00, 0x00), 0x02));
		}

		SetContextFont (TinyFont);
		font_DrawText (&t);
	}
	UnbatchGraphics ();
	SetContext (OldContext);
}

static void
RedistributeFuel (void)
{
	const DWORD FuelVolume = GLOBAL_SIS (FuelOnBoard);
	const CONTEXT OldContext = SetContext (SpaceContext);
	RECT r;
	r.extent.height = 1;

	// Loop through all the rows to draw
	BatchGraphics ();
	for (GLOBAL_SIS (FuelOnBoard) = FUEL_RESERVE;
			GLOBAL_SIS (FuelOnBoard) < GetFTankCapacity (&r.corner);
			GLOBAL_SIS (FuelOnBoard) += FUEL_VOLUME_PER_ROW)
	{
		// If we're less than the fuel level, draw fuel.
		if (GLOBAL_SIS (FuelOnBoard) < FuelVolume)
		{
			r.extent.width = RES_SCALE(3) + IF_HD(6); // JMS_GFX
			DrawPoint (&r.corner);
			r.corner.x += r.extent.width + 1;
			DrawPoint (&r.corner);
			r.corner.x -= r.extent.width;
			SetContextForeGroundColor (
					SetContextBackGroundColor (BLACK_COLOR));
		}
		else // Otherwise, draw an empty bar.
		{
			r.extent.width = RES_SCALE(5); // JMS_GFX
			SetContextForeGroundColor (
					BUILD_COLOR (MAKE_RGB15 (0x0B, 0x00, 0x00), 0x2E));
		}
		DrawFilledRectangle (&r);
	}
	UnbatchGraphics ();
	SetContext (OldContext);

	GLOBAL_SIS (FuelOnBoard) = FuelVolume;
}

#define LANDER_X RES_SCALE(24) // JMS_GFX
#define LANDER_Y RES_SCALE(67) // JMS_GFX
#define LANDER_WIDTH RES_SCALE(15) // JMS_GFX

static void
DisplayLanders (MENU_STATE *pMS)
{
	STAMP s;

	s.frame = pMS->ModuleFrame;
	if (GET_GAME_STATE (CHMMR_BOMB_STATE) == 3)
	{
		s.origin.x = s.origin.y = 0;
		s.frame = SetAbsFrameIndex (pMS->ModuleFrame,
			GetFrameCount (pMS->ModuleFrame) - SHIELD_LOCATION_IN_MODULE_ANI + 4);
		DrawStamp (&s);
	}
	else
	{
		COUNT i;

		s.origin.x = LANDER_X;
		s.origin.y = LANDER_Y;
		for (i = 0; i < GLOBAL_SIS (NumLanders); ++i)
		{
			DrawStamp (&s);
			s.origin.x += LANDER_WIDTH;
		}

		SetContextForeGroundColor (BLACK_COLOR);
		for (; i < MAX_LANDERS; ++i)
		{
			DrawFilledStamp (&s);
			s.origin.x += LANDER_WIDTH;
		}
	}
}

static BOOLEAN
DoInstallModule (MENU_STATE *pMS)
{
	BYTE NewState, new_slot_piece, old_slot_piece;
	SIZE FirstItem, LastItem;
	BOOLEAN select, cancel, motion;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		pMS->InputFunc = DoOutfit;
		return (TRUE);
	}

	select = PulsedInputState.menu[KEY_MENU_SELECT];
	cancel = PulsedInputState.menu[KEY_MENU_CANCEL];
	motion = PulsedInputState.menu[KEY_MENU_LEFT] ||
			PulsedInputState.menu[KEY_MENU_RIGHT] ||
			PulsedInputState.menu[KEY_MENU_UP] ||
			PulsedInputState.menu[KEY_MENU_DOWN];

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

	FirstItem = 0;
	NewState = pMS->CurState;
	switch (NewState)
	{
		case PLANET_LANDER:
		case EMPTY_SLOT + 3:
			old_slot_piece = pMS->delta_item < GLOBAL_SIS (NumLanders)
					? PLANET_LANDER : (EMPTY_SLOT + 3);
			LastItem = MAX_LANDERS - 1;
			break;
		case FUSION_THRUSTER:
		case EMPTY_SLOT + 0:
			old_slot_piece = GLOBAL_SIS (DriveSlots[pMS->delta_item]);
			LastItem = NUM_DRIVE_SLOTS - 1;
			break;
		case TURNING_JETS:
		case EMPTY_SLOT + 1:
			old_slot_piece = GLOBAL_SIS (JetSlots[pMS->delta_item]);
			LastItem = NUM_JET_SLOTS - 1;
			break;
		default:
			old_slot_piece = GLOBAL_SIS (ModuleSlots[pMS->delta_item]);
			if (GET_GAME_STATE (CHMMR_BOMB_STATE) == 3)
				FirstItem = NUM_BOMB_MODULES;
			LastItem = NUM_MODULE_SLOTS - 1;
			break;
	}
	
	if (NewState < CREW_POD)
		FirstItem = LastItem = NewState;
	else if (NewState < EMPTY_SLOT)
		FirstItem = CREW_POD, LastItem = NUM_PURCHASE_MODULES - 1;

	if (!pMS->Initialized)
	{
		new_slot_piece = old_slot_piece;
		pMS->Initialized = TRUE;

		pMS->InputFunc = DoInstallModule;


		SetContext (SpaceContext);
		ClearSISRect (CLEAR_SIS_RADAR);
		SetFlashRect (NULL);
		goto InitFlash;
	}
	else if (select || cancel)
	{
		new_slot_piece = pMS->CurState;

		if (select)
		{
			if (new_slot_piece < EMPTY_SLOT)
			{

				if (GLOBAL_SIS (ResUnits) < (DWORD)(GLOBAL (ModuleCost[new_slot_piece]) * MODULE_COST_SCALE))
				{	// not enough RUs to build
					PlayMenuSound (MENU_SOUND_FAILURE);
					return (TRUE);
				}
			}
			else if (new_slot_piece == EMPTY_SLOT + 2)
			{
				if (old_slot_piece == CREW_POD)
				{
					if (GLOBAL_SIS (CrewEnlisted) > CREW_POD_CAPACITY
							* (CountSISPieces (CREW_POD) - 1))
					{	// crew pod still needed for crew recruited
						PlayMenuSound (MENU_SOUND_FAILURE);
						return (TRUE);
					}
				}
				else if (old_slot_piece == FUEL_TANK
						|| old_slot_piece == HIGHEFF_FUELSYS)
				{
					DWORD volume;

					volume = (DWORD)CountSISPieces (FUEL_TANK)
							* FUEL_TANK_CAPACITY
							+ (DWORD)CountSISPieces (HIGHEFF_FUELSYS)
							* HEFUEL_TANK_CAPACITY;
					volume -= (old_slot_piece == FUEL_TANK
							? FUEL_TANK_CAPACITY : HEFUEL_TANK_CAPACITY);
					if (GLOBAL_SIS (FuelOnBoard) > volume + FUEL_RESERVE)
					{	// fuel tank still needed for the fuel on board
						if(!optInfiniteFuel){
							PlayMenuSound (MENU_SOUND_FAILURE);
							return (TRUE);
						} else {
							if (old_slot_piece == FUEL_TANK)
								DeltaSISGauges(0,-FUEL_TANK_CAPACITY,0);
							else
								DeltaSISGauges(0,-HEFUEL_TANK_CAPACITY,0);
						}
					}
				}
				else if (old_slot_piece == STORAGE_BAY)
				{
					if (GLOBAL_SIS (TotalElementMass) > STORAGE_BAY_CAPACITY
							* (CountSISPieces (STORAGE_BAY) - 1))
					{	// storage bay still needed for the cargo
						PlayMenuSound (MENU_SOUND_FAILURE);
						return (TRUE);
					}
				}
			}
		}

		SetContext (SpaceContext);

		SetFlashRect (NULL);

		if (select)
		{
			if (new_slot_piece >= EMPTY_SLOT && old_slot_piece >= EMPTY_SLOT)
			{
				new_slot_piece -= EMPTY_SLOT - 1;
				if (new_slot_piece > CREW_POD)
					new_slot_piece = PLANET_LANDER;
			}
			else
			{
				switch (pMS->CurState)
				{
					case PLANET_LANDER:
						++GLOBAL_SIS (NumLanders);
						break;
					case EMPTY_SLOT + 3:
						--GLOBAL_SIS (NumLanders);
						break;
					case FUSION_THRUSTER:
					case EMPTY_SLOT + 0:
						GLOBAL_SIS (DriveSlots[pMS->delta_item]) =
								new_slot_piece;
						break;
					case TURNING_JETS:
					case EMPTY_SLOT + 1:
						GLOBAL_SIS (JetSlots[pMS->delta_item]) =
								new_slot_piece;
						break;
					default:
						GLOBAL_SIS (ModuleSlots[pMS->delta_item]) =
								new_slot_piece;
						break;
				}

				if (new_slot_piece < EMPTY_SLOT)
					DeltaSISGauges (UNDEFINED_DELTA, UNDEFINED_DELTA,
							-(GLOBAL (ModuleCost[new_slot_piece])
							* MODULE_COST_SCALE));
				else /* if (old_slot_piece < EMPTY_SLOT) */
					DeltaSISGauges (UNDEFINED_DELTA, UNDEFINED_DELTA,
							GLOBAL (ModuleCost[old_slot_piece])
							* MODULE_COST_SCALE);

				if (pMS->CurState == PLANET_LANDER ||
						pMS->CurState == EMPTY_SLOT + 3)
					DisplayLanders (pMS);
				else
				{
					DrawShipPiece (pMS->ModuleFrame, new_slot_piece,
							pMS->delta_item, FALSE);

					if (new_slot_piece > TURNING_JETS
							&& old_slot_piece > TURNING_JETS)
						RedistributeFuel ();
					if (optWhichFonts == OPT_PC)
						DrawFlagshipStats ();
				}
			}

			cancel = FALSE;
		}

		if (pMS->CurState < EMPTY_SLOT)
		{
			pMS->CurState += EMPTY_SLOT - 1;
			if (pMS->CurState < EMPTY_SLOT)
				pMS->CurState = EMPTY_SLOT + 3;
			else if (pMS->CurState > EMPTY_SLOT + 2)
				pMS->CurState = EMPTY_SLOT + 2;
			if (cancel)
				new_slot_piece = pMS->CurState;
			goto InitFlash;
		}
		else if (!cancel)
		{
			pMS->CurState = new_slot_piece;
			goto InitFlash;
		}
		else
		{
			SetContext (StatusContext);
			DrawMenuStateStrings (PM_FUEL, pMS->CurState = OUTFIT_MODULES);
			SetFlashRect (SFR_MENU_3DO);

			pMS->InputFunc = DoOutfit;
			ClearSISRect (DRAW_SIS_DISPLAY);
		}
	}
	else if (motion)
	{
		SIZE NewItem;

		NewItem = NewState < EMPTY_SLOT ? pMS->CurState : pMS->delta_item;
		do
		{
			if (NewState >= EMPTY_SLOT && (PulsedInputState.menu[KEY_MENU_UP]
					|| PulsedInputState.menu[KEY_MENU_DOWN]))
			{
				if (PulsedInputState.menu[KEY_MENU_UP])
				{
					if (NewState-- == EMPTY_SLOT)
						NewState = EMPTY_SLOT + 3;
				}
				else
				{
					if (NewState++ == EMPTY_SLOT + 3)
						NewState = EMPTY_SLOT;
				}
				NewItem = 0;
				if (GET_GAME_STATE (CHMMR_BOMB_STATE) == 3)
				{
					if (NewState == EMPTY_SLOT + 3)
						NewState = PulsedInputState.menu[KEY_MENU_UP] ?
								EMPTY_SLOT + 2 : EMPTY_SLOT;
					if (NewState == EMPTY_SLOT + 2)
						NewItem = NUM_BOMB_MODULES;
				}
				pMS->delta_item = NewItem;
			}
			else if (PulsedInputState.menu[KEY_MENU_LEFT] ||
					PulsedInputState.menu[KEY_MENU_UP])
			{
				if (NewItem-- == FirstItem)
					NewItem = LastItem;
			}
			else if (PulsedInputState.menu[KEY_MENU_RIGHT] ||
					PulsedInputState.menu[KEY_MENU_DOWN])
			{
				if (NewItem++ == LastItem)
					NewItem = FirstItem;
			}
		} while (NewState < EMPTY_SLOT
				&& (GLOBAL (ModuleCost[NewItem]) == 0
				|| (NewItem >= GUN_WEAPON && NewItem <= CANNON_WEAPON
				&& pMS->delta_item > 0 && pMS->delta_item < 13)));

		if (NewState < EMPTY_SLOT)
		{
			if (NewItem != pMS->CurState)
			{
				pMS->CurState = NewItem;
				PreUpdateFlashRect ();
				DrawModuleStrings (pMS, NewItem);
				PostUpdateFlashRect ();
			}
		}
		else if (NewItem != pMS->delta_item || NewState != pMS->CurState)
		{
			SIZE w;
				
			switch (NewState)
			{
				case PLANET_LANDER:
				case EMPTY_SLOT + 3:
					new_slot_piece = NewItem < GLOBAL_SIS (NumLanders)
							? PLANET_LANDER : (EMPTY_SLOT + 3);
					break;
				case FUSION_THRUSTER:
				case EMPTY_SLOT + 0:
					new_slot_piece = GLOBAL_SIS (DriveSlots[NewItem]);
					break;
				case TURNING_JETS:
				case EMPTY_SLOT + 1:
					new_slot_piece = GLOBAL_SIS (JetSlots[NewItem]);
					break;
				default:
					new_slot_piece = GLOBAL_SIS (ModuleSlots[NewItem]);
					break;
			}

			SetContext (SpaceContext);

			if (NewState == pMS->CurState)
			{
				if (NewState == PLANET_LANDER || NewState == EMPTY_SLOT + 3)
					w = LANDER_WIDTH;
				else
					w = SHIP_PIECE_OFFSET;

				// JMS_GFX
				if (NewState != PLANET_LANDER && NewState != FUSION_THRUSTER 
					&& NewState != TURNING_JETS && NewState != EMPTY_SLOT + 0
					 && NewState != EMPTY_SLOT + 1 && NewState != EMPTY_SLOT + 3)
					w += IF_HD(1);

				w *= (NewItem - pMS->delta_item);
				pMS->flash_rect0.corner.x += w;
				pMS->delta_item = NewItem;
			}
			else
			{
				pMS->CurState = NewState;
InitFlash:
				w = SHIP_PIECE_OFFSET;
				switch (pMS->CurState)
				{
					case PLANET_LANDER:
					case EMPTY_SLOT + 3:
						pMS->flash_rect0.corner.x = LANDER_X - 1 + IF_HD(114); // JMS_GFX
						pMS->flash_rect0.corner.y = LANDER_Y - 1 + IF_HD(65); // JMS_GFX
						pMS->flash_rect0.extent.width = RES_SCALE(11 + 2); // JMS_GFX
						pMS->flash_rect0.extent.height = RES_SCALE(13 + 2); // JMS_GFX;

						w = LANDER_WIDTH;
						break;
					case FUSION_THRUSTER:
					case EMPTY_SLOT + 0:
						pMS->flash_rect0.corner.x = DRIVE_TOP_X - 1 - IF_HD(5);
						pMS->flash_rect0.corner.y = DRIVE_TOP_Y - 1 + IF_HD(146);
						pMS->flash_rect0.extent.width = RES_SCALE(8); // JMS_GFX;
						pMS->flash_rect0.extent.height = RES_SCALE(6) - IF_HD(2); // JMS_GFX;

						break;
					case TURNING_JETS:
					case EMPTY_SLOT + 1:
						pMS->flash_rect0.corner.x = JET_TOP_X - 1 - IF_HD(3);
						pMS->flash_rect0.corner.y = JET_TOP_Y - 1 + IF_HD(185);
						pMS->flash_rect0.extent.width = RES_SCALE(9); // JMS_GFX;
						pMS->flash_rect0.extent.height = RES_SCALE(10) + IF_HD(4); // JMS_GFX;

						break;
					default:
						pMS->flash_rect0.corner.x = MODULE_TOP_X - 1 + IF_HD(2);
						pMS->flash_rect0.corner.y = MODULE_TOP_Y - 1;
						pMS->flash_rect0.extent.width = SHIP_PIECE_OFFSET + 2 - IF_HD(1);
						pMS->flash_rect0.extent.height = RES_SCALE(34) + IF_HD(9); // JMS_GFX;
						w += IF_HD(1);
						break;
				}

				w *= pMS->delta_item;
				pMS->flash_rect0.corner.x += w;
			}

			DrawModuleStrings (pMS, new_slot_piece);
			if (pMS->CurState < EMPTY_SLOT)
				// flash with PC menus too
				SetFlashRect (SFR_MENU_ANY);
			else
				SetFlashRect (&pMS->flash_rect0);
		}
	}

	return (TRUE);
}

static void
ChangeFuelQuantity (void)
{
	SDWORD incr = 0; // Fuel increment in fuel points (not units).
	const SDWORD maxFit = GetFuelTankCapacity() - (SDWORD)GLOBAL_SIS(FuelOnBoard);
	const SDWORD minFit = -(SDWORD)GLOBAL_SIS(FuelOnBoard);
	
	if (PulsedInputState.menu[KEY_MENU_UP])
		incr = FUEL_TANK_SCALE;  // +1 Unit
	else if (PulsedInputState.menu[KEY_MENU_DOWN])
		incr = -FUEL_TANK_SCALE; // -1 Unit
	else if (PulsedInputState.menu[KEY_MENU_PAGE_UP])
		incr = (FUEL_TANK_SCALE * 10); // +1 Bar
	else if (PulsedInputState.menu[KEY_MENU_PAGE_DOWN])
		incr = -(FUEL_TANK_SCALE * 10); // -1 Bar
	else if (PulsedInputState.menu[KEY_MENU_HOME])
		incr = maxFit; // Fill to max
	else if (PulsedInputState.menu[KEY_MENU_END])
		incr = minFit; // -1 Bar
	else
		return;



	if(PulsedInputState.menu[KEY_MENU_HOME] || PulsedInputState.menu[KEY_MENU_END])
		PlayMenuSound(MENU_SOUND_INVOKED);

	// Clamp incr to what we can afford/hold/have.
	{
		const SDWORD maxAfford = GLOBAL_SIS(ResUnits) / GLOBAL(FuelCost);

		if (incr > maxFit)
			incr = maxFit; // All we can hold.

		if (incr > maxAfford * FUEL_TANK_SCALE)
			incr = maxAfford * FUEL_TANK_SCALE; // All we can afford.

		if (incr < minFit)
			incr = minFit; // All we have.
	}

	if (!incr)
	{
		// No more room, not enough RUs, or no fuel left to drain.
		PlayMenuSound (MENU_SOUND_FAILURE);
	}
	else
	{
		const int cost = (incr / FUEL_TANK_SCALE) * GLOBAL (FuelCost);
		PreUpdateFlashRect ();
		DeltaSISGauges (0, incr, -cost);
		PostUpdateFlashRect ();
		RedistributeFuel ();
	}

	{   // Make fuel gauge flash.
		RECT r;
		CONTEXT oldContext = SetContext (StatusContext);
		GetGaugeRect (&r, FALSE);
		SetFlashRect (&r);
		SetContext (oldContext);
	}
}

static void
onNamingDone (void)
{
	// In case player just named a ship, redraw it
	DrawFlagshipName (FALSE, FALSE);
}

BOOLEAN
DoOutfit (MENU_STATE *pMS)
{
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		goto ExitOutfit;

	OutfitOrShipyard = 2;

	if (!pMS->Initialized)
	{
		pMS->InputFunc = DoOutfit;
		pMS->Initialized = TRUE;

		SetNamingCallback (onNamingDone);

		{
			COUNT num_frames;
			STAMP s;

			pMS->CurFrame = CaptureDrawable (
					LoadGraphic (MODULES_PMAP_ANIM));
			pMS->hMusic = LoadMusic (OUTFIT_MUSIC);
			pMS->CurState = OUTFIT_FUEL;
			pMS->ModuleFrame = CaptureDrawable (
					LoadGraphic (SISMODS_MASK_PMAP_ANIM));
			s.origin.x = s.origin.y = 0;
			s.frame = CaptureDrawable (
					LoadGraphic (OUTFIT_PMAP_ANIM));

			SetTransitionSource (NULL);
			BatchGraphics ();
			DrawSISFrame ();
			DrawSISMessage (GAME_STRING (STARBASE_STRING_BASE + 2));
			DrawSISTitle (GAME_STRING (STARBASE_STRING_BASE));
			SetContext (SpaceContext);

			DrawStamp (&s);
			DestroyDrawable (ReleaseDrawable (s.frame));

			for (num_frames = 0; num_frames < NUM_DRIVE_SLOTS; ++num_frames)
			{
				BYTE which_piece;

				which_piece = GLOBAL_SIS (DriveSlots[num_frames]);
				if (which_piece < EMPTY_SLOT)
					DrawShipPiece (pMS->ModuleFrame, which_piece,
							num_frames, FALSE);
			}
			for (num_frames = 0; num_frames < NUM_JET_SLOTS; ++num_frames)
			{
				BYTE which_piece;

				which_piece = GLOBAL_SIS (JetSlots[num_frames]);
				if (which_piece < EMPTY_SLOT)
					DrawShipPiece (pMS->ModuleFrame, which_piece,
						num_frames, FALSE);
			}
			for (num_frames = 0; num_frames < NUM_MODULE_SLOTS; ++num_frames)
			{
				BYTE which_piece;

				which_piece = GLOBAL_SIS (ModuleSlots[num_frames]);
				if (which_piece < EMPTY_SLOT)
					DrawShipPiece (pMS->ModuleFrame, which_piece,
							num_frames, FALSE);
			}
			RedistributeFuel ();
			DisplayLanders (pMS);
			if (GET_GAME_STATE (CHMMR_BOMB_STATE) < 3)
			{
				BYTE ShieldFlags;
				
				ShieldFlags = GET_GAME_STATE (LANDER_SHIELDS);

				s.frame = SetAbsFrameIndex (pMS->ModuleFrame,
						GetFrameCount (pMS->ModuleFrame) - SHIELD_LOCATION_IN_MODULE_ANI);
				if (ShieldFlags & (1 << EARTHQUAKE_DISASTER))
					DrawStamp (&s);
				s.frame = IncFrameIndex (s.frame);
				if (ShieldFlags & (1 << BIOLOGICAL_DISASTER))
					DrawStamp (&s);
				s.frame = IncFrameIndex (s.frame);
				if (ShieldFlags & (1 << LIGHTNING_DISASTER))
					DrawStamp (&s);
				s.frame = IncFrameIndex (s.frame);
				if (ShieldFlags & (1 << LAVASPOT_DISASTER))
					DrawStamp (&s);
			}

			DrawMenuStateStrings (PM_FUEL, pMS->CurState);
			DrawFlagshipName (FALSE, FALSE);
			if (optWhichFonts == OPT_PC)
				DrawFlagshipStats ();

			ScreenTransition (3, NULL);
			PlayMusic (pMS->hMusic, TRUE, 1);
			UnbatchGraphics ();
			
			SetFlashRect (SFR_MENU_3DO);

			GLOBAL_SIS (FuelOnBoard) =
					(GLOBAL_SIS (FuelOnBoard)
					+ (FUEL_TANK_SCALE >> 1)) / FUEL_TANK_SCALE;
			GLOBAL_SIS (FuelOnBoard) *= FUEL_TANK_SCALE;
		}

		SetContext (StatusContext);
	}
	else if (PulsedInputState.menu[KEY_MENU_CANCEL]
			|| (PulsedInputState.menu[KEY_MENU_SELECT]
			&& pMS->CurState == OUTFIT_EXIT))
	{
		if (pMS->CurState == OUTFIT_DOFUEL)
		{
			pMS->CurState = OUTFIT_FUEL;
			SetFlashRect (SFR_MENU_3DO);
		}
		else
		{
ExitOutfit:
			DestroyDrawable (ReleaseDrawable (pMS->CurFrame));
			pMS->CurFrame = 0;
			DestroyDrawable (ReleaseDrawable (pMS->ModuleFrame));
			pMS->ModuleFrame = 0;

			SetNamingCallback (NULL);

			return (FALSE);
		}
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		switch (pMS->CurState)
		{
			case OUTFIT_FUEL:
			{
				RECT r;

				pMS->CurState = OUTFIT_DOFUEL;
				SetContext (StatusContext);
				GetGaugeRect (&r, FALSE);
				SetFlashRect (&r);
				break;
			}
			case OUTFIT_DOFUEL:
				pMS->CurState = OUTFIT_FUEL;
				SetFlashRect (SFR_MENU_3DO);
				break;
			case OUTFIT_MODULES:
				pMS->CurState = EMPTY_SLOT + 2;
				if (GET_GAME_STATE (CHMMR_BOMB_STATE) != 3)
					pMS->delta_item = 0;
				else
					pMS->delta_item = NUM_BOMB_MODULES;
				pMS->first_item.y = 0;
				pMS->Initialized = 0;
				DoInstallModule (pMS);
				break;
			case OUTFIT_SAVELOAD:
				// Clearing FlashRect is not necessary
				if (!GameOptions ())
					goto ExitOutfit;
				DrawMenuStateStrings (PM_FUEL, pMS->CurState);
				SetFlashRect (SFR_MENU_3DO);
				break;
		}
	}
	else
	{
		switch (pMS->CurState)
		{
			case OUTFIT_DOFUEL:
				SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_PAGE | MENU_SOUND_ACTION);
				break;
			default:
				SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
				break;
		}

		if (pMS->CurState == OUTFIT_DOFUEL)
		{
			if(!optInfiniteFuel)
				ChangeFuelQuantity ();

			SleepThread (ONE_SECOND / 30);
		}
		else
			DoMenuChooser (pMS, PM_FUEL);
	}
	

	if(optInfiniteFuel){		
		DeltaSISGauges(0,GetFuelTankCapacity(),0);
		RedistributeFuel ();
	}
	if(optInfiniteRU)		
		GLOBAL_SIS (ResUnits) = 1000000L;

	return (TRUE);
}

