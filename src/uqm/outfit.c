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
#include "menustat.h"
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
#include "util.h"
#include "shipcont.h"

// How manyeth .png in the module.ani file is the first lander shield.
#define SHIELD_LOCATION_IN_MODULE_ANI 51

enum
{
	OUTFIT_FUEL,
	OUTFIT_MODULES,
	OUTFIT_SAVELOAD,
	OUTFIT_EXIT,
	OUTFIT_DOFUEL
};

POINT lander_pos[MAX_LANDERS];
FONT ModuleFont;

static void
InitializeDOSLanderPos (void)
{	// Initialize the DOS lander icon positions
	int i;
	POINT temp[MAX_LANDERS] = { LANDER_DOS_PTS };

	if (!IS_DOS)
		return;

	if (lander_pos[0].x != RES_SCALE (temp[0].x))
	{
		for (i = 0; i < MAX_LANDERS; i++)
		{
			lander_pos[i].x = RES_SCALE (temp[i].x);
			lander_pos[i].y = RES_SCALE (temp[i].y);
		}
	}
}

// This is all for drawing the DOS version modules menu
#define MODULE_ORG_Y       RES_SCALE (36)
#define MODULE_SPACING_Y   RES_SCALE (16)

#define MODULE_COL_0       RES_SCALE (5)
#define MODULE_COL_1       RES_SCALE (61)

#define MODULE_SEL_ORG_X  (MODULE_COL_0 - RES_SCALE (1))
#define MODULE_SEL_WIDTH  (FIELD_WIDTH - RES_SCALE (3))

#define NAME_OFS_Y         RES_SCALE (2)
#define TEXT_BASELINE      RES_SCALE (6)
#define TEXT_SPACING_Y     RES_SCALE (7)

#define MAX_VIS_MODULES  5

typedef struct
{
	BYTE list[NUM_PURCHASE_MODULES];
	// List of all modules player has
	COUNT count;
	// Number of modules in the list
	COUNT topIndex;
	// Index of the top module displayed
} MODULES_STATE;

MODULES_STATE ModuleState;

static void
DrawModuleStatus (COUNT index, COUNT pos, bool selected)
{
	RECT r;
	TEXT t;
	UNICODE buf[10];

	t.align = ALIGN_LEFT;
	t.baseline.x = MODULE_COL_0;

	r.extent.width = MODULE_SEL_WIDTH;
	r.extent.height = TEXT_SPACING_Y * 2;
	r.corner.x = MODULE_SEL_ORG_X;

	// draw line background
	r.corner.y = MODULE_ORG_Y + pos * MODULE_SPACING_Y + NAME_OFS_Y;
	SetContextForeGroundColor (selected ?
			MODULE_SELECTED_BACK_COLOR : MODULE_BACK_COLOR);
	DrawFilledRectangle (&r);
	SetContextFont (TinyFont);


	if (GLOBAL (ModuleCost[index]))
	{	// print module name
		SetContextForeGroundColor (selected ?
				MODULE_SELECTED_COLOR : MODULE_NAME_COLOR);
		t.baseline.y = r.corner.y + TEXT_BASELINE;
		t.pStr = GAME_STRING (index + STARBASE_STRING_BASE + 12);
		t.CharCount = utf8StringPos (t.pStr, ' ');
		font_DrawText (&t);
		t.baseline.y += TEXT_SPACING_Y;
		t.pStr = skipUTF8Chars (t.pStr, t.CharCount + 1);
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
		
		// print module cost
		SetContextForeGroundColor (selected ?
				MODULE_SELECTED_COLOR : MODULE_PRICE_COLOR);
		t.align = ALIGN_RIGHT;
		t.baseline.x = MODULE_COL_1 - RES_SCALE (2);
		t.baseline.y -= RES_SCALE (3);
		snprintf (buf, sizeof (buf), "%u",
				GLOBAL (ModuleCost[index]) * MODULE_COST_SCALE);
		t.pStr = buf;
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
	}
	else
	{
		SetContextForeGroundColor (MODULE_PRICE_COLOR);
		r.corner.x += RES_SCALE (21);
		r.corner.y += RES_SCALE (6);
		r.extent.width >>= 2;
		r.extent.height = RES_SCALE (2);
		DrawFilledRectangle (&r);
	}
}

static void
DrawModuleDisplay (MODULES_STATE *modState)
{
	TEXT t;
	RECT r;
	COUNT i;

	r.corner.x = RES_SCALE (2);
	r.corner.y = RES_SCALE (20);
	r.extent.width = FIELD_WIDTH + RES_SCALE (1);
	r.extent.height = (RES_SCALE (129) - r.corner.y);

	if (!optCustomBorder && !IS_HD)
	{
		DrawStarConBox (&r, RES_SCALE (1),
				SHADOWBOX_MEDIUM_COLOR, SHADOWBOX_DARK_COLOR,
				TRUE, MODULE_BACK_COLOR, FALSE, TRANSPARENT);
	}
	else
		DrawBorder (DEVICE_CARGO_FRAME);

	// print the "MODULES" title
	SetContextFont (StarConFont);
	t.baseline.x = (STATUS_WIDTH >> 1) - RES_SCALE (1);
	t.baseline.y = r.corner.y + RES_SCALE (7);
	t.align = ALIGN_CENTER;
	t.pStr = GAME_STRING (STARBASE_STRING_BASE + 11);
	t.CharCount = (COUNT)~0;
	SetContextForeGroundColor (MODULE_SELECTED_COLOR);
	font_DrawText (&t);

	// print names and costs
	for (i = 0; i < MAX_VIS_MODULES; ++i)
	{
		COUNT modIndex = modState->topIndex + i;

		if (modIndex >= modState->count)
			break;

		DrawModuleStatus (modState->list[modIndex], i, false);
	}
}

static void
DrawModules (MODULES_STATE *modState, COUNT NewItem)
{
	CONTEXT OldContext = SetContext (StatusContext);

	BatchGraphics ();

	DrawModuleDisplay (modState);
	DrawModuleStatus (modState->list[NewItem],
			NewItem - modState->topIndex, true);

	UnbatchGraphics ();

	SetContext (OldContext);
}

static void
ManipulateModules (SIZE NewState)
{
	MODULES_STATE *modState;
	SIZE NewTop;

	if (!IS_DOS)
		return;

	modState = &ModuleState;
	NewTop = modState->topIndex;

	if (NewState > NUM_PURCHASE_MODULES)
	{
		DrawModules (modState, NewState);
		return;
	}

	if (NewState < NewTop || NewState >= NewTop + MAX_VIS_MODULES)
		modState->topIndex = NewState - NewState % MAX_VIS_MODULES;

	DrawModules (modState, NewState);
}

SIZE
InventoryModules (BYTE *pModuleMap, COUNT Size)
{
	BYTE i;
	SIZE ModulesOnBoard;

	ModulesOnBoard = 0;
	for (i = 0; i < NUM_PURCHASE_MODULES && Size > 0; ++i)
	{
		BYTE ActiveModule;

		ActiveModule = GLOBAL (ModuleCost[i]);

		{
			*pModuleMap++ = i;
			++ModulesOnBoard;
			--Size;
		}
	}

	return ModulesOnBoard;
}

static void
DrawModuleMenuText (RECT *r, int Index)
{
	TEXT text;
	SIZE leading;
	RECT block;
	UNICODE buf[256];
	COORD og_baseline_x;

	if (IS_DOS || !strlen (GAME_STRING (STARBASE_STRING_BASE + 26 + Index)))
		return;

	SetContextFont (ModuleFont);

	GetContextFontLeading (&leading);

	SetContextForeGroundColor (MDL_RECT_COLOR);

	if (!optCustomBorder)
	{
		block = *r;
		block.extent.height = (leading << 1) - RES_SCALE (1);
		DrawFilledRectangle (&block);
	}
	else
		DrawBorder (TEXT_LABEL_FRAME);

	text.baseline.x = r->corner.x + (r->extent.width >> 1);
	text.baseline.y = r->corner.y + leading - RES_SCALE (1);
	og_baseline_x = text.baseline.x;

	utf8StringCopy ((char *)buf, sizeof (buf),
			GAME_STRING (STARBASE_STRING_BASE + 26 + Index));

	text.align = ALIGN_CENTER;
	text.pStr = strtok (buf, " ");
	text.CharCount = (COUNT)~0;

	while (text.pStr != NULL)
	{
		text.pStr = AlignText ((const UNICODE *)text.pStr,
				&text.baseline.x);
		text.CharCount = (COUNT)~0;

		font_DrawShadowedText (&text, WEST_SHADOW, MDL_TEXT_COLOR,
				MDL_SHADOW_COLOR);

		text.pStr = strtok (NULL, " ");
		text.CharCount = (COUNT)~0;
		text.baseline.y += leading;
		text.baseline.x = og_baseline_x;
	}
}

static void
DrawModuleStrings (MENU_STATE *pMS, BYTE NewModule)
{
	RECT r;
	STAMP s;
	CONTEXT OldContext;

	OldContext = SetContext (StatusContext);
	GetContextClipRect (&r);
	s.origin.x = RADAR_X - r.corner.x;
	s.origin.y = RADAR_Y - r.corner.y;

	BatchGraphics ();

	if (!IS_DOS)
	{
		r.corner.x = s.origin.x - RES_SCALE (1);
		r.corner.y = s.origin.y - RES_SCALE (11);
		r.extent.width = RADAR_WIDTH + RES_SCALE (2);
		r.extent.height = RES_SCALE (11);
		//	ClearSISRect (CLEAR_SIS_RADAR); // blinks otherwise
		SetContextForeGroundColor (MENU_FOREGROUND_COLOR);
		if (!optCustomBorder)
			DrawFilledRectangle (&r); // drawn over anyway
	}

	DrawBorder (SIS_RADAR_FRAME);

	if (IS_DOS)
	{
		RECT dosRect;

		dosRect.corner.x = RES_SCALE (2);
		dosRect.corner.y = RADAR_Y - RES_SCALE (1);
		dosRect.extent.width = RADAR_WIDTH + RES_SCALE (4);
		dosRect.extent.height = RADAR_HEIGHT + RES_SCALE (2);

		if (optCustomBorder)
		{
			dosRect.corner.x += RES_SCALE (2);
			dosRect.corner.y += RES_SCALE (2);
			dosRect.extent.width -= RES_SCALE (4);
			dosRect.extent.height -= RES_SCALE (4);
			SetContextForeGroundColor (BLACK_COLOR);
			DrawFilledRectangle (&dosRect);
		}
		else
		{
			if (!IS_HD)
			{
				DrawStarConBox (&dosRect, 1, PCMENU_TOP_LEFT_BORDER_COLOR,
						PCMENU_BOTTOM_RIGHT_BORDER_COLOR, TRUE,
						BLACK_COLOR, FALSE, TRANSPARENT);
			}
			else
			{
				DrawRenderedBox (&dosRect, TRUE, BLACK_COLOR,
						THIN_INNER_BEVEL, optCustomBorder);
			}
		}
	}

	if (NewModule >= EMPTY_SLOT)
	{
		r.corner = s.origin;
		r.extent.width = RADAR_WIDTH;
		r.extent.height = RADAR_HEIGHT;
		SetContextForeGroundColor (BLACK_COLOR);
		DrawFilledRectangle (&r);
	}
	else if (pMS->CurFrame)
	{
		TEXT t;
		UNICODE buf[40];

		// Draw the module image.
		s.frame = SetAbsFrameIndex (pMS->CurFrame, NewModule);
		DrawStamp (&s);

		/// HERE!
		DrawModuleMenuText (&r, NewModule);

		// Print the module cost.
		t.baseline.x = s.origin.x + RADAR_WIDTH - RES_SCALE (2);
		t.baseline.y = s.origin.y + RADAR_HEIGHT - RES_SCALE (2);
		t.align = ALIGN_RIGHT;
		t.CharCount = (COUNT)~0;
		t.pStr = buf;
		sprintf (buf, "%u",
				GLOBAL (ModuleCost[NewModule]) * MODULE_COST_SCALE);
		if (isPC (optWhichFonts))
			SetContextFont (TinyFont);
		else
			SetContextFont (TinyFontBold);

		if ((GLOBAL_SIS (ResUnits)) 
				>= (DWORD)((GLOBAL (ModuleCost[NewModule])
				* MODULE_COST_SCALE))) 
			SetContextForeGroundColor (BRIGHT_GREEN_COLOR);
		else
			SetContextForeGroundColor (BRIGHT_RED_COLOR);

		if (!IS_DOS)
			font_DrawText (&t);
	}
	UnbatchGraphics ();
	SetContext (OldContext);
}

static void
RedistributeFuel (void)
{
	const CONTEXT OldContext = SetContext (SpaceContext);
	
	BatchGraphics ();
	
	DrawFuelInFTanks (TRUE);

	UnbatchGraphics ();
	SetContext (OldContext);
}

static void
DrawEscapePodText (RECT rect )
{
	TEXT text;
	FONT OldFont;
	Color OldColor;
	SIZE leading;
	RECT block;
	UNICODE buf[256];
	COORD og_baseline_x;

	if (!strlen (GAME_STRING (STARBASE_STRING_BASE + 41)))
		return;

	OldFont = SetContextFont (SquareFont);
	OldColor = SetContextForeGroundColor (BLACK_COLOR);
	
	block = rect;
	block.corner.x += DOS_BOOL_SCL (171, 9);
	block.corner.y += RES_SCALE (38);
	block.extent.width = RES_SCALE (36);
	block.extent.height = RES_SCALE (6);
	DrawFilledRectangle (&block);

	block.corner.x += RES_SCALE (9);
	block.corner.y += block.extent.height;
	block.extent.width = RES_SCALE (19);
	DrawFilledRectangle (&block);

	GetContextFontLeading (&leading);

	text.baseline = rect.corner;
	text.baseline.x += DOS_BOOL_SCL (189, 27);
	text.baseline.y += RES_SCALE (39);
	text.align = ALIGN_CENTER;

	og_baseline_x = text.baseline.x;

	utf8StringCopy ((char *)buf, sizeof (buf),
			GAME_STRING (STARBASE_STRING_BASE + 41));

	text.align = ALIGN_CENTER;
	text.pStr = strtok (buf, " ");
	text.CharCount = (COUNT)~0;

	SetContextForeGroundColor (LANDER_POD_TEXT_COLOR);

	while (text.pStr != NULL)
	{
		text.pStr = AlignText ((const UNICODE *)text.pStr,
				&text.baseline.x);
		text.CharCount = (COUNT)~0;

		font_DrawText (&text);

		text.pStr = strtok (NULL, " ");
		text.CharCount = (COUNT)~0;
		text.baseline.y += leading;
		text.baseline.x = og_baseline_x;
	}

	SetContextFont (OldFont);
	SetContextForeGroundColor (OldColor);
}

static void
DrawNoLandersText (RECT rect)
{
	TEXT text;
	FONT OldFont;
	Color OldColor;
	RECT block;

	if (IS_DOS || !strlen (GAME_STRING (STARBASE_STRING_BASE + 40)))
		return;

	OldFont = SetContextFont (SquareFont);
	OldColor = SetContextForeGroundColor (BLACK_COLOR);

	block = rect;
	block.corner.y += RES_SCALE (9);
	block.extent.width = RES_SCALE (162) - IF_HD (3);
	block.extent.height = RES_SCALE (20);
	DrawFilledRectangle (&block);

	text.baseline = rect.corner;
	text.baseline.x += RES_SCALE (81);
	text.baseline.y += RES_SCALE (17);
	text.align = ALIGN_CENTER;
	text.pStr = AlignText (
			(const UNICODE *)GAME_STRING (STARBASE_STRING_BASE + 40),
			&text.baseline.x);
	text.CharCount = (COUNT)~0;

	SetContextForeGroundColor (LANDER_POD_TEXT_COLOR);
	font_DrawText (&text);

	SetContextFont (OldFont);
	SetContextForeGroundColor (OldColor);
}

#define LANDER_X (RES_SCALE (24) - SAFE_PAD)
#define LANDER_Y RES_SCALE (67)
#define LANDER_WIDTH RES_SCALE (15)

static void
DisplayLanders (MENU_STATE *pMS)
{
	STAMP s;

	s.frame = pMS->ModuleFrame;
	if (GET_GAME_STATE (CHMMR_BOMB_STATE) == 3)
	{
		RECT rect;
		s.origin.x = -SAFE_X;
		s.origin.y = 0;
		s.frame = SetAbsFrameIndex (pMS->ModuleFrame,
				SHIELD_LOCATION_IN_MODULE_ANI + 4);
		DrawStamp (&s);

		GetFrameRect (s.frame, &rect);
		rect.corner.x += s.origin.x;

		DrawNoLandersText (rect);
		DrawEscapePodText (rect);
	}
	else
	{
		COUNT i;

		if (!IS_DOS)
		{
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
		else
		{
			for (i = 0; i < GLOBAL_SIS (NumLanders); ++i)
			{
				s.origin = lander_pos[i];
				DrawStamp (&s);
			}

			SetContextForeGroundColor (BLACK_COLOR);
			for (; i < MAX_LANDERS; ++i)
			{
				s.origin = lander_pos[i];
				DrawFilledStamp (&s);
			}
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
		SetFlashRect (NULL, FALSE);
		goto InitFlash;
	}
	else if (select || cancel)
	{
		new_slot_piece = pMS->CurState;
		if (select)
		{
			if (new_slot_piece < EMPTY_SLOT)
			{
				if (GLOBAL_SIS (ResUnits) <
						(DWORD)(GLOBAL (ModuleCost[new_slot_piece])
						* MODULE_COST_SCALE))
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
						if(!optInfiniteFuel)
						{
							PlayMenuSound (MENU_SOUND_FAILURE);
							return (TRUE);
						} else {
							if (old_slot_piece == FUEL_TANK)
								DeltaSISGauges (0,-FUEL_TANK_CAPACITY,0);
							else
								DeltaSISGauges (0,-HEFUEL_TANK_CAPACITY,0);
						}
					}
				}
				else if (old_slot_piece == STORAGE_BAY)
				{
					if (GLOBAL_SIS (TotalElementMass)
							> STORAGE_BAY_CAPACITY
							* (CountSISPieces (STORAGE_BAY) - 1))
					{	// storage bay still needed for the cargo
						PlayMenuSound (MENU_SOUND_FAILURE);
						return (TRUE);
					}
				}
			}
		}

		SetContext (SpaceContext);

		SetFlashRect (NULL, FALSE);

		if (select)
		{
			if (new_slot_piece >= EMPTY_SLOT
					&& old_slot_piece >= EMPTY_SLOT)
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

					DrawFlagshipStats ();
				}
			}

			cancel = FALSE;
		}

		if (pMS->CurState < EMPTY_SLOT)
		{
			pMS->CurState += EMPTY_SLOT - 1;
			if ((pMS->CurState == EMPTY_SLOT - 1) &&
					pMS->delta_item + 1 > GLOBAL_SIS (NumLanders))
				new_slot_piece = old_slot_piece;
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
			if ((new_slot_piece == EMPTY_SLOT + 3) &&
					(old_slot_piece == PLANET_LANDER) &&
					(pMS->delta_item < GLOBAL_SIS (NumLanders)))
				new_slot_piece = old_slot_piece;
			else
				pMS->CurState = new_slot_piece;
			goto InitFlash;
		}
		else
		{
			SetContext (StatusContext);
			DrawMenuStateStrings (PM_FUEL, pMS->CurState = OUTFIT_MODULES);
			SetFlashRect (SFR_MENU_3DO, FALSE);

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
			if (NewState >= EMPTY_SLOT
					&& (PulsedInputState.menu[KEY_MENU_UP]
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
				ManipulateModules (NewItem);
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
				SIZE h = 0;
				if (NewState == PLANET_LANDER
						|| NewState == EMPTY_SLOT + 3)
				{
					w = LANDER_WIDTH;
				}
				else
					w = SHIP_PIECE_OFFSET;

				w *= (NewItem - pMS->delta_item);
				if (IS_DOS && (NewState == PLANET_LANDER
					|| NewState == EMPTY_SLOT + 3))
				{
					pMS->flash_rect0.corner.x = lander_pos[NewItem].x
							- RES_SCALE (1);
					pMS->flash_rect0.corner.y = lander_pos[NewItem].y
							- RES_SCALE (1);
				}
				else
					pMS->flash_rect0.corner.x += w;
				pMS->flash_rect1.corner.x += w;
				//pMS->flash_rect2.corner.x += w;
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
						pMS->flash_rect0.corner.x =
								DOS_BOOL (LANDER_X, RES_SCALE (LANDER_DOS_X))
								- RES_SCALE (1);
						pMS->flash_rect0.corner.y =
								DOS_BOOL (LANDER_Y, RES_SCALE (LANDER_DOS_Y))
								- RES_SCALE (1);
						pMS->flash_rect0.extent.width =
								RES_SCALE (11 + 2);
						pMS->flash_rect0.extent.height =
								RES_SCALE (13 + 2);

						w = LANDER_WIDTH;
						break;
					case FUSION_THRUSTER:
					case EMPTY_SLOT + 0:
						pMS->flash_rect0.corner.x =
								DRIVE_TOP_X - RES_SCALE (1);
						pMS->flash_rect0.corner.y =
								DRIVE_TOP_Y - RES_SCALE (1);
						pMS->flash_rect0.extent.width = RES_SCALE (8);
						pMS->flash_rect0.extent.height = RES_SCALE (6);

						pMS->flash_rect1.corner.x =
								DRIVE_SIDE_X - RES_SCALE (1) + IF_HD (5);
						pMS->flash_rect1.corner.y =
								DRIVE_SIDE_Y - RES_SCALE (1) - IF_HD (10);
						pMS->flash_rect1.extent.width =
								RES_SCALE (12) - IF_HD (10);
						pMS->flash_rect1.extent.height =
								RES_SCALE (7) + IF_HD (4);

						//pMS->flash_rect2 = pMS->flash_rect0;
						//pMS->flash_rect2.corner.y += RES_SCALE (90);
						break;
					case TURNING_JETS:
					case EMPTY_SLOT + 1:
						pMS->flash_rect0.corner.x =
								JET_TOP_X - RES_SCALE (1) - IF_HD (6);
						pMS->flash_rect0.corner.y =
								JET_TOP_Y - RES_SCALE (1);
						pMS->flash_rect0.extent.width = RES_SCALE (9);
						pMS->flash_rect0.extent.height = RES_SCALE (10);

						pMS->flash_rect1.corner.x =
								JET_SIDE_X - RES_SCALE (1) - IF_HD (6);
						pMS->flash_rect1.corner.y =
								JET_SIDE_Y - RES_SCALE (1) + IF_HD (4);
						pMS->flash_rect1.extent.width = RES_SCALE (7);
						pMS->flash_rect1.extent.height = RES_SCALE (4);

						//pMS->flash_rect2 = pMS->flash_rect0;
						//pMS->flash_rect2.corner.y += RES_SCALE (70);
						break;
					default:
						pMS->flash_rect0.corner.x =
								MODULE_TOP_X - RES_SCALE (1);
						pMS->flash_rect0.corner.y =
								MODULE_TOP_Y - RES_SCALE (1);
						pMS->flash_rect0.extent.width =
								SHIP_PIECE_OFFSET + RES_SCALE (2)
								+ RES_SCALE (optWhichMenu == OPT_PC);
						pMS->flash_rect0.extent.height = RES_SCALE (34);

						pMS->flash_rect1.corner.x =
								MODULE_SIDE_X - RES_SCALE (1);
						pMS->flash_rect1.corner.y =
								MODULE_SIDE_Y - RES_SCALE (1);
						pMS->flash_rect1.extent.width =
								SHIP_PIECE_OFFSET + RES_SCALE (2)
								+ RES_SCALE (optWhichMenu == OPT_PC);
						pMS->flash_rect1.extent.height = RES_SCALE (25);
						break;
				}

				w *= pMS->delta_item;
				if (IS_DOS && (NewState == PLANET_LANDER
					|| NewState == EMPTY_SLOT + 3))
				{
					pMS->flash_rect0.corner.x =
							lander_pos[pMS->delta_item].x - RES_SCALE (1);
					pMS->flash_rect0.corner.y =
							lander_pos[pMS->delta_item].y - RES_SCALE (1);
				}
				else
					pMS->flash_rect0.corner.x += w;
				pMS->flash_rect1.corner.x += w;
				//pMS->flash_rect2.corner.x += w;
			}

			DrawModuleStrings (pMS, new_slot_piece);
			ManipulateModules (new_slot_piece);
			if (pMS->CurState < EMPTY_SLOT)
			{	// flash with PC menus too
				SetFlashRect (DOS_BOOL (SFR_MENU_ANY, SFR_MENU_NON), FALSE);
			}
			else
			{
				if (optWhichMenu == OPT_PC)
				{
					switch (pMS->CurState)
					{
						case EMPTY_SLOT + 3:
						{	// lander
							DumpAdditionalRect ();
							break;
						}
						case EMPTY_SLOT + 0:
						case EMPTY_SLOT + 1:
						{	// thruster and jets
							SetAdditionalRect (&pMS->flash_rect1, TRUE);
							// SetAdditionalRect (&pMS->flash_rect2, 2);
							break;
						}
						default:
						{	// everything else
							DumpAdditionalRect ();
							SetAdditionalRect (&pMS->flash_rect1, TRUE);
							break;
						}
					}
				}

				if (IS_DOS && (pMS->CurState == EMPTY_SLOT + 3
						|| pMS->CurState == PLANET_LANDER)
						&& is3DO (optWhichMenu))
				{
					SetFlashRect (&pMS->flash_rect0, TRUE);
				}
				else
					SetFlashRect (&pMS->flash_rect0, optWhichMenu == OPT_PC);
			}
		}
	}

	return (TRUE);
}

static void
ChangeFuelQuantity (void)
{
	SDWORD incr = 0; // Fuel increment in fuel points (not units).
	const SDWORD maxFit =
			GetFuelTankCapacity() - (SDWORD)GLOBAL_SIS (FuelOnBoard);
	const SDWORD minFit = -(SDWORD)GLOBAL_SIS (FuelOnBoard);
	
	if (PulsedInputState.menu[KEY_MENU_UP])
		incr = FUEL_TANK_SCALE;  // +1 Unit
	else if (PulsedInputState.menu[KEY_MENU_DOWN])
		incr = -FUEL_TANK_SCALE; // -1 Unit
	else if (PulsedInputState.menu[KEY_MENU_RIGHT])
		incr = (FUEL_TANK_SCALE * 10); // +1 Bar
	else if (PulsedInputState.menu[KEY_MENU_LEFT])
		incr = -(FUEL_TANK_SCALE * 10); // -1 Bar
	else if (PulsedInputState.menu[KEY_MENU_ZOOM_IN])
		incr = maxFit; // Fill to max
	else if (PulsedInputState.menu[KEY_MENU_ZOOM_OUT])
		incr = minFit; // Empty the tanks
	else
		return;

	if (PulsedInputState.menu[KEY_MENU_ZOOM_IN]
			|| PulsedInputState.menu[KEY_MENU_ZOOM_OUT])
		PlayMenuSound (MENU_SOUND_INVOKED);

	// Clamp incr to what we can afford/hold/have.
	{
		const SDWORD maxAfford = GLOBAL_SIS (ResUnits) / GLOBAL (FuelCost);

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
		SetFlashRect (&r, FALSE);
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

		InitializeDOSLanderPos ();

		SetNamingCallback (onNamingDone);

		{
			COUNT num_frames;
			STAMP s;

			if (!IS_DOS)
				ModuleFont = LoadFont (MODULE_FONT);

			pMS->CurFrame = CaptureDrawable (
					LoadGraphic (MODULES_PMAP_ANIM));
			pMS->hMusic = LoadMusic (OUTFIT_MUSIC);
			pMS->CurState = OUTFIT_FUEL;
			pMS->ModuleFrame = CaptureDrawable (
					LoadGraphic (SISMODS_MASK_PMAP_ANIM));
			s.origin.x = SAFE_X ? (-SAFE_X + RES_SCALE (3)) : 0;
			s.origin.y = 0;
			s.frame = CaptureDrawable (
					LoadGraphic (OUTFIT_PMAP_ANIM));

			if (optFlagshipColor == OPT_3DO)
				s.frame = SetAbsFrameIndex (s.frame, 1);
			else
				s.frame = SetAbsFrameIndex (s.frame, 0);

			SetTransitionSource (NULL);
			BatchGraphics ();
			DrawSISFrame ();
			DrawSISMessage (GAME_STRING (STARBASE_STRING_BASE + 2));
			DrawSISTitle (GAME_STRING (STARBASE_STRING_BASE));

			SetContext (SpaceContext);

			DrawStamp (&s);
			DestroyDrawable (ReleaseDrawable (s.frame));

			for (num_frames = 0; num_frames < NUM_DRIVE_SLOTS;
					++num_frames)
			{
				BYTE which_piece;

				which_piece = GLOBAL_SIS (DriveSlots[num_frames]);
				if (which_piece < EMPTY_SLOT)
					DrawShipPiece (pMS->ModuleFrame, which_piece,
							num_frames, FALSE);
			}
			for (num_frames = 0; num_frames < NUM_JET_SLOTS;
					++num_frames)
			{
				BYTE which_piece;

				which_piece = GLOBAL_SIS (JetSlots[num_frames]);
				if (which_piece < EMPTY_SLOT)
					DrawShipPiece (pMS->ModuleFrame, which_piece,
						num_frames, FALSE);
			}
			for (num_frames = 0; num_frames < NUM_MODULE_SLOTS;
					++num_frames)
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
						SHIELD_LOCATION_IN_MODULE_ANI);
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

			DrawFlagshipStats ();

			ScreenTransition (optScrTrans, NULL);

			PlayMusicResume (pMS->hMusic, NORMAL_VOLUME);

			UnbatchGraphics ();
			
			SetFlashRect (SFR_MENU_3DO, FALSE);

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
			DrawMenuStateStrings (PM_FUEL, pMS->CurState);
			SetFlashRect (SFR_MENU_3DO, FALSE);
		}
		else
		{
ExitOutfit:
			if (pMS->CurState < OUTFIT_EXIT)
				DrawMenuStateStrings (PM_FUEL, OUTFIT_EXIT);
			DestroyDrawable (ReleaseDrawable (pMS->CurFrame));
			pMS->CurFrame = 0;
			DestroyDrawable (ReleaseDrawable (pMS->ModuleFrame));
			pMS->ModuleFrame = 0;

			// Release Fonts
			if (!IS_DOS)
				DestroyFont (ModuleFont);

			SetMusicPosition ();

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
				DrawMenuStateStrings (PM_FUEL, pMS->CurState);
				pMS->CurState = OUTFIT_DOFUEL;
				SetContext (StatusContext);
				GetGaugeRect (&r, FALSE);
				SetFlashRect (&r, FALSE);
				break;
			}
			case OUTFIT_DOFUEL:
				pMS->CurState = OUTFIT_FUEL;
				DrawMenuStateStrings(PM_FUEL, pMS->CurState);
				SetFlashRect (SFR_MENU_3DO, FALSE);
				break;
			case OUTFIT_MODULES:

				if (IS_DOS)
				{
					memset (&ModuleState, 0, sizeof ModuleState);
					ModuleState.count = InventoryModules (ModuleState.list,
							NUM_PURCHASE_MODULES);
				}

				DrawMenuStateStrings (PM_FUEL, pMS->CurState);
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
				SetFlashRect (SFR_MENU_3DO, FALSE);
				break;
		}
	}
	else
	{
		switch (pMS->CurState)
		{
			case OUTFIT_DOFUEL:
				SetMenuSounds (MENU_SOUND_ARROWS,
						MENU_SOUND_PAGE | MENU_SOUND_ACTION);
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

	if (optInfiniteFuel)
	{		
		DeltaSISGauges (0, GetFuelTankCapacity (), 0);
		RedistributeFuel ();
	}
	if (optInfiniteRU)
		GLOBAL_SIS (ResUnits) = 1000000L;

	return (TRUE);
}