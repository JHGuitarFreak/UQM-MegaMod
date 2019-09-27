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

#include "build.h"
#include "colors.h"
#include "controls.h"
#include "starmap.h"
#include "comm.h"
#include "gamestr.h"
#include "save.h"
#include "starbase.h"
#include "sis.h"
#include "resinst.h"
#include "nameref.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "libs/graphics/gfx_common.h"
#include "libs/tasklib.h"
#include "libs/log.h"

#include "planets/planets.h"
// JMS: For MIN_MOON_RADIUS

#include <math.h>
// JMS: For sin and cos


static void CleanupAfterStarBase (void);

static void
DrawBaseStateStrings (STARBASE_STATE OldState, STARBASE_STATE NewState)
{
	TEXT t;
	RECT r;
	COUNT text_base_y = 106 + 28;
	COUNT text_spacing_y = 23 - 4;
	COUNT HD_Y = 32;
	//STRING locString;

	SetContext (ScreenContext);
	SetContextFont (RES_BOOL(StarConFont, MeleeFont));
	SetContextForeGroundColor (BLACK_COLOR);

	t.baseline.x = RES_SCALE(73 - 4); // JMS_GFX
	t.align = ALIGN_CENTER;

	if (OldState == (STARBASE_STATE)~0)
	{
		t.baseline.y = RES_SCALE(text_base_y + 4) + IF_HD(HD_Y); // JMS_GFX;
		for (OldState = TALK_COMMANDER; OldState < DEPART_BASE; ++OldState)
		{
			if (OldState != NewState)
			{
				t.pStr = GAME_STRING (STARBASE_STRING_BASE + 1 + OldState);
				t.CharCount = (COUNT)~0;
				font_DrawText (&t);
			}
			t.baseline.y += RES_SCALE(text_spacing_y); // JMS_GFX
		}
	}

	t.baseline.y = RES_SCALE(text_base_y + (text_spacing_y * OldState) + 4) + IF_HD(HD_Y); // JMS_GFX
	t.pStr = GAME_STRING (STARBASE_STRING_BASE + 1 + OldState);
	// BW: erase previously selected string before redrawing it
	TextRect(&t, &r, NULL);
	SetContextForeGroundColor (
			   BUILD_COLOR_RGBA (0x88, 0x88, 0x88, 0xff));
	if(!IS_HD)
		DrawFilledRectangle (&r);
	SetContextForeGroundColor (BLACK_COLOR);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E));
	t.baseline.y = RES_SCALE(text_base_y + (text_spacing_y * NewState) + 4) + IF_HD(HD_Y); // JMS_GFX
	t.pStr = GAME_STRING (STARBASE_STRING_BASE + 1 + NewState);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
}

void
DrawShipPiece (FRAME ModuleFrame, COUNT which_piece, COUNT which_slot,
		BOOLEAN DrawBluePrint)
{
	Color OldColor = UNDEFINED_COLOR;
			// Initialisation is just to keep the compiler silent.
	RECT r;
	STAMP Side, Top;
	SBYTE RepairSlot;
	COUNT ship_piece_offset_scaled = SHIP_PIECE_OFFSET;
 
	// JMS_GFX
	if (IS_HD &&
		which_piece != FUSION_THRUSTER && which_piece != TURNING_JETS
		 && which_piece != EMPTY_SLOT + 0 && which_piece != EMPTY_SLOT + 1)
		ship_piece_offset_scaled += 1;

	RepairSlot = 0;
	switch (which_piece)
	{
		case FUSION_THRUSTER:
		case EMPTY_SLOT + 0:
			Side.origin.x = DRIVE_SIDE_X;
			Side.origin.y = DRIVE_SIDE_Y;
			Top.origin.x = DRIVE_TOP_X;
			Top.origin.y = DRIVE_TOP_Y;
			break;
		case TURNING_JETS:
		case EMPTY_SLOT + 1:
			Side.origin.x = JET_SIDE_X;
			Side.origin.y = JET_SIDE_Y;
			Top.origin.x = JET_TOP_X;
			Top.origin.y = JET_TOP_Y;
			break;
		default:
			if (which_piece < EMPTY_SLOT + 2)
			{
				RepairSlot = 1;
				if (which_piece < EMPTY_SLOT
						&& (which_slot == 0
						|| GLOBAL_SIS (ModuleSlots[
								which_slot - 1
								]) < EMPTY_SLOT))
					++RepairSlot;
			}
			else if (!DrawBluePrint)
			{
				if (which_slot == 0 || which_slot >= NUM_MODULE_SLOTS - 3)
					++which_piece;

				if (which_slot < NUM_MODULE_SLOTS - 1
						&& GLOBAL_SIS (ModuleSlots[
								which_slot + 1
								]) < EMPTY_SLOT)
				{
					RepairSlot = -1;
					if (which_piece == EMPTY_SLOT + 3
							|| which_slot + 1 == NUM_MODULE_SLOTS - 3)
						--RepairSlot;
				}
			}
			Side.origin.x = MODULE_SIDE_X;
			Side.origin.y = MODULE_SIDE_Y;
			Top.origin.x = MODULE_TOP_X;
			Top.origin.y = MODULE_TOP_Y;
			break;
	}

	Side.origin.x += which_slot * ship_piece_offset_scaled;
	Side.frame = NULL;
	if (RepairSlot < 0)
	{
		Side.frame = SetAbsFrameIndex (ModuleFrame,
				((NUM_MODULES - 1) + (6 - 2)) + (NUM_MODULES + 6)
				- (RepairSlot + 1));
		// JMS_GFX:
		if (!IS_HD ||
			(which_piece != FUSION_THRUSTER && which_piece != TURNING_JETS
			 && which_piece != EMPTY_SLOT + 0 && which_piece != EMPTY_SLOT + 1))
			DrawStamp (&Side);
	}
	else if (RepairSlot && !(IS_HD && DrawBluePrint))
	{
		OldColor = SetContextForeGroundColor (BLACK_COLOR);

		r.corner = Side.origin;
		r.corner.y += IF_HD(8);
		r.extent.width = ship_piece_offset_scaled;
		r.extent.height = RES_SCALE(1);

		DrawFilledRectangle (&r);

		r.corner.y += RES_SCALE(23 - 1); // JMS_GFX
		if (IS_HD)
			r.extent.height += IF_HD(30);
		
		if (which_slot == 0 && IS_HD)
			r.corner.x += IF_HD(4); // JMS_GFX
		else if (which_slot == NUM_MODULE_SLOTS - 1 && IS_HD)
			r.extent.width -= IF_HD(9); // JMS_GFX
		DrawFilledRectangle (&r);
		
		r.extent.width = RES_BOOL(1, 12);
		r.extent.height = RES_SCALE(8) + IF_HD(30); // JMS_GFX

		if (RepairSlot == 2)
		{
			r.corner = Side.origin;
			r.corner.y += IF_HD(8);
			DrawFilledRectangle (&r);
			if (IS_HD) {
				r.corner.x += ship_piece_offset_scaled - r.extent.width;
				if (which_slot == NUM_MODULE_SLOTS - 1 && IS_HD) {
					r.extent.height -= IF_HD(16); // JMS_GFX
					r.extent.width += IF_HD(4); // JMS_GFX
					DrawFilledRectangle (&r);
					r.extent.width -= IF_HD(4); // JMS_GFX
				} else
					DrawFilledRectangle (&r);
				
				r.corner.x -= ship_piece_offset_scaled - r.extent.width;
			}
			r.corner.y += RES_SCALE(15); // JMS_GFX
			DrawFilledRectangle (&r);
			if (IS_HD) {
				r.corner.x += ship_piece_offset_scaled - r.extent.width;
				if (which_slot == NUM_MODULE_SLOTS - 1 && IS_HD) {
					r.corner.y += IF_HD(32);
					r.extent.height -= IF_HD(36); // JMS_GFX
					r.extent.width += IF_HD(3); // JMS_GFX
					DrawFilledRectangle (&r);
					r.extent.width -= IF_HD(3); // JMS_GFX
					r.extent.height += IF_HD(36); // JMS_GFX
				} else
					DrawFilledRectangle (&r);
				
				r.corner.x -= ship_piece_offset_scaled - r.extent.width;
			}
		}
		if (which_slot < (NUM_MODULE_SLOTS - 1))
		{
			r.corner = Side.origin;
			r.corner.y += IF_HD(8);
			r.corner.x += ship_piece_offset_scaled;
			DrawFilledRectangle (&r);
			if (IS_HD) {
				r.corner.x += ship_piece_offset_scaled - r.extent.width;
				DrawFilledRectangle (&r);
				r.corner.x -= ship_piece_offset_scaled - r.extent.width;
			}
			r.corner.y += RES_SCALE(15); // JMS_GFX
			DrawFilledRectangle (&r);
		}
	}

	if (DrawBluePrint)
	{
		if (RepairSlot && !(IS_HD && DrawBluePrint))
			SetContextForeGroundColor (OldColor);
		Side.frame = SetAbsFrameIndex (ModuleFrame, which_piece - 1);
		DrawFilledStamp (&Side);
	}
	else
	{
		Top.origin.x += which_slot * ship_piece_offset_scaled;
		if (RepairSlot < 0)
		{
			Top.frame = SetRelFrameIndex (Side.frame, -((NUM_MODULES - 1) + 6));
			DrawStamp (&Top);
		}
		else if (RepairSlot)
		{
			r.corner = Top.origin;
			r.extent.width = ship_piece_offset_scaled;
			r.extent.height = RES_SCALE(1) + RESOLUTION_FACTOR;
			DrawFilledRectangle (&r);
			r.corner.y += RES_SCALE(32 - 1) + IF_HD(16);  // JMS_GFX
			DrawFilledRectangle (&r);

			r.extent.width = RES_SCALE(1) + RESOLUTION_FACTOR;
			r.extent.height = RES_SCALE(12) + IF_HD(17); // JMS_GFX
			if (RepairSlot == 2)
			{
				r.corner = Top.origin;
				DrawFilledRectangle (&r);
				if (IS_HD) {
					r.corner.x += ship_piece_offset_scaled - r.extent.width;
					DrawFilledRectangle (&r);
					r.corner.x -= ship_piece_offset_scaled - r.extent.width;
				}
				r.corner.y += RES_SCALE(20); // JMS_GFX
 				DrawFilledRectangle (&r);
				if (IS_HD) {
					r.corner.x += ship_piece_offset_scaled - r.extent.width;
					DrawFilledRectangle (&r);
					r.corner.x -= ship_piece_offset_scaled - r.extent.width;
				}
			}
			RepairSlot = (which_slot < NUM_MODULE_SLOTS - 1);
			if (RepairSlot)
			{
				r.corner = Top.origin;
				r.corner.x += ship_piece_offset_scaled;
				DrawFilledRectangle (&r);
				r.corner.y += RES_SCALE(20); // JMS_GFX
				DrawFilledRectangle (&r);
			}
		}

		Top.frame = SetAbsFrameIndex (ModuleFrame, which_piece);
		DrawStamp (&Top);

		Side.frame = SetRelFrameIndex (Top.frame, (NUM_MODULES - 1) + 6);
		if (IS_HD
			&& (which_piece == EMPTY_SLOT + 2 
				|| which_piece == EMPTY_SLOT + 3))
		{
			if (which_slot == 0)
				Side.frame = SetRelFrameIndex (Side.frame, 8);
			if (which_slot == 1)
				Side.frame = SetRelFrameIndex (Side.frame, 10);
			if (which_slot == NUM_MODULE_SLOTS - 2)
				Side.frame = SetRelFrameIndex (Side.frame, 10);
			if (which_slot == NUM_MODULE_SLOTS - 1)
				Side.frame = SetRelFrameIndex (Side.frame, 11);
		}
		
		// JMS_GFX:
		if (!IS_HD ||
			(which_piece != FUSION_THRUSTER && which_piece != TURNING_JETS
			 && which_piece != EMPTY_SLOT + 0 && which_piece != EMPTY_SLOT + 1))
			DrawStamp (&Side);

		if (which_slot == 1 && which_piece == EMPTY_SLOT + 2 && !IS_HD)
		{
			STAMP s;

			s.origin = Top.origin;
			s.origin.x -= ship_piece_offset_scaled;
			s.frame = SetAbsFrameIndex (ModuleFrame, NUM_MODULES + 5);
			DrawStamp (&s);
			s.origin = Side.origin;
			s.origin.x -= ship_piece_offset_scaled;
			s.frame = SetRelFrameIndex (s.frame, (NUM_MODULES - 1) + 6);
			DrawStamp (&s);
		}

		if (RepairSlot)
		{
			Top.origin.x += ship_piece_offset_scaled;
			Side.origin.x += ship_piece_offset_scaled;
			which_piece = GLOBAL_SIS (ModuleSlots[++which_slot]);
			if (which_piece == EMPTY_SLOT + 2
					&& which_slot >= NUM_MODULE_SLOTS - 3)
				++which_piece;

			Top.frame = SetAbsFrameIndex (ModuleFrame, which_piece);
			DrawStamp (&Top);

			Side.frame = SetRelFrameIndex (Top.frame, (NUM_MODULES - 1) + 6);
			if (IS_HD)
			{
				if (which_slot == 0 && which_piece == EMPTY_SLOT + 3)
					Side.frame = SetAbsFrameIndex (ModuleFrame, GetFrameCount (ModuleFrame)-4);
				if (which_slot == 1 && which_piece == EMPTY_SLOT + 2)
					Side.frame = SetAbsFrameIndex (ModuleFrame, GetFrameCount (ModuleFrame)-3);
				if (which_slot == NUM_MODULE_SLOTS - 2 
					&& which_piece == EMPTY_SLOT + 3)
					Side.frame = SetAbsFrameIndex (ModuleFrame, GetFrameCount (ModuleFrame)-2);
				if (which_slot == NUM_MODULE_SLOTS - 1 
					&& which_piece == EMPTY_SLOT + 3)
					Side.frame = SetAbsFrameIndex (ModuleFrame, GetFrameCount (ModuleFrame)-1);
			}
			
			// JMS_GFX:
			if (!IS_HD ||
				(which_piece != FUSION_THRUSTER && which_piece != TURNING_JETS
				 && which_piece != EMPTY_SLOT + 0 && which_piece != EMPTY_SLOT + 1))
				DrawStamp (&Side);
		}
	}
}

static void
rotateStarbase (MENU_STATE *pMS, FRAME AniFrame)
{
	static TimeCount NextTime = 0;
	TimeCount Now = GetTimeCounter ();

	if (AniFrame)
	{	// Setup the animation
		pMS->flash_frame0 = AniFrame;
		pMS->flash_rect0.corner.x = 0;
		pMS->flash_rect0.corner.y = 4;
	}
	
	if (Now >= NextTime || AniFrame)
	{
		STAMP s;

		NextTime = Now + (ONE_SECOND / 20);

		s.origin = pMS->flash_rect0.corner;
		s.frame = pMS->flash_frame0;
		DrawStamp (&s);

		s.frame = IncFrameIndex (s.frame);
		if (GetFrameIndex (s.frame) == 0)
		{	// Do not redraw the base frame, animation loops to frame 1
			s.frame = IncFrameIndex (s.frame);
		}
		pMS->flash_frame0 = s.frame;
	}
}

BOOLEAN
DoStarBase (MENU_STATE *pMS)
{
	// XXX: This function is full of hacks and otherwise strange code

	if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
	{
		pMS->CurState = DEPART_BASE;
		goto ExitStarBase;
	}
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

	if (!pMS->Initialized)
	{
		LastActivity &= ~CHECK_LOAD;
		pMS->InputFunc = DoStarBase;

		SetFlashRect (NULL);

		if (pMS->hMusic)
		{
			StopMusic ();
			DestroyMusic (pMS->hMusic);
			pMS->hMusic = 0;
		}

		pMS->Initialized = TRUE;

		pMS->CurFrame = CaptureDrawable (LoadGraphic (STARBASE_ANIM));
		pMS->hMusic = LoadMusic (STARBASE_MUSIC);

		SetContext (ScreenContext);
		SetTransitionSource (NULL);
		BatchGraphics ();
		SetContextBackGroundColor (BLACK_COLOR);
		ClearDrawable ();
		rotateStarbase (pMS, pMS->CurFrame);
		DrawBaseStateStrings ((STARBASE_STATE)~0, pMS->CurState);
		ScreenTransition (3, NULL);
		PlayMusic (pMS->hMusic, TRUE, 1);
		UnbatchGraphics ();
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
ExitStarBase:
		DestroyDrawable (ReleaseDrawable (pMS->CurFrame));
		pMS->CurFrame = 0;
		StopMusic ();
		if (pMS->hMusic)
		{
			DestroyMusic (pMS->hMusic);
			pMS->hMusic = 0;
		}

		if (pMS->CurState == DEPART_BASE)
		{
			if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
			{
				SET_GAME_STATE (STARBASE_VISITED, 0);
			}
			return (FALSE);
		}

		pMS->Initialized = FALSE;
		if (pMS->CurState == TALK_COMMANDER)
		{
			FlushInput ();
			InitCommunication (COMMANDER_CONVERSATION);
			// XXX: InitCommunication() clears these flags, and we need them
			//   This marks that we are in Starbase.
			SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, (BYTE)~0);
		}
		else
		{
			BYTE OldState;

			switch (OldState = pMS->CurState)
			{
				case OUTFIT_STARSHIP:
					pMS->InputFunc = DoOutfit;
					break;
				case SHIPYARD:
					pMS->InputFunc = DoShipyard;
					break;
			}

			SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
			DoInput (pMS, TRUE);

			pMS->Initialized = FALSE;
			pMS->CurState = OldState;
			pMS->InputFunc = DoStarBase;
		}
	}
	else
	{
		STARBASE_STATE NewState;

		NewState = pMS->CurState;
		if (PulsedInputState.menu[KEY_MENU_LEFT] || PulsedInputState.menu[KEY_MENU_UP])
		{
			if (NewState-- == TALK_COMMANDER)
				NewState = DEPART_BASE;
		}
		else if (PulsedInputState.menu[KEY_MENU_RIGHT] || PulsedInputState.menu[KEY_MENU_DOWN])
		{
			if (NewState++ == DEPART_BASE)
				NewState = TALK_COMMANDER;
		}

		BatchGraphics ();
		SetContext (ScreenContext);

		if (NewState != pMS->CurState)
		{
			DrawBaseStateStrings (pMS->CurState, NewState);
			pMS->CurState = NewState;
		}

		rotateStarbase (pMS, NULL);

		UnbatchGraphics ();

		SleepThread (ONE_SECOND / 30);
	}

	return (TRUE);
}

static void
DoTimePassage (void)
{
#define LOST_DAYS 14
	SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND * 2));
	MoveGameClockDays (LOST_DAYS);

	// JMS: Calculate flagship location in IP.
	if (optOrbitingPlanets)
	{
		double newAngle;
		POINT starbase_coords;
		
		// Calculate the starbase position on a circle with the help of sin and cos.
		newAngle = ((double)(10) + daysElapsed() * (FULL_CIRCLE / 11.46)) * M_PI / 32 - M_PI/2 ; // JMS: Starbase orbit values copied from gensol.c
		starbase_coords.x = (COORD)(cos(newAngle) * MIN_MOON_RADIUS);
		starbase_coords.y = (COORD)(sin(newAngle) * MIN_MOON_RADIUS);
		
		//log_add (log_Debug, "startangle:%d angle:%f, radius:%d, speed:%f, days:%f X:%d, y:%d", 10, newAngle, MIN_MOON_RADIUS, FULL_CIRCLE / 11.46, daysElapsed(), starbase_coords.x, starbase_coords.y);
		
		// Translate the coordinates on a circle to an ellipse and update the ship's graphics' coordinates on the screen.
		GLOBAL (ShipStamp.origin.x) = (SIS_SCREEN_WIDTH >> 1) + starbase_coords.x;
		GLOBAL (ShipStamp.origin.y) = (SIS_SCREEN_HEIGHT >> 1) + (starbase_coords.y >> 1);
	}
}

void
VisitStarBase (void)
{
	MENU_STATE MenuState;
	CONTEXT OldContext;
	StatMsgMode prevMsgMode = SMM_UNDEFINED;

	// XXX: This should probably be moved out to Starcon2Main()
	if (GET_GAME_STATE (CHMMR_BOMB_STATE) == 2)
	{	// We were just transported by Chmmr to the Starbase
		// Force a reload of the SolarSys
		CurStarDescPtr = NULL;
		// This marks that we are in Starbase.
		SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, (BYTE)~0);
	}

	if (!GET_GAME_STATE (STARBASE_AVAILABLE))
	{
		HSHIPFRAG hStarShip;
		SHIP_FRAGMENT *FragPtr;

		// Unallied Starbase conversation
		SetCommIntroMode (CIM_CROSSFADE_SCREEN, 0);
		InitCommunication (COMMANDER_CONVERSATION);
		if (!GET_GAME_STATE (PROBE_ILWRATH_ENCOUNTER)
				|| (GLOBAL (CurrentActivity) & CHECK_ABORT))
		{
			CleanupAfterStarBase ();
			return;
		}

		/* Create an Ilwrath ship responding to the Ur-Quan
		 * probe's broadcast */
		hStarShip = CloneShipFragment (ILWRATH_SHIP,
				&GLOBAL (npc_built_ship_q), 7);
		FragPtr = LockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
		/* Hack (sort of): Suppress the tally and salvage info
		 * after the battle */
		FragPtr->race_id = (BYTE)~0;
		UnlockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);

		InitCommunication (ILWRATH_CONVERSATION);
		if (GLOBAL_SIS (CrewEnlisted) == (COUNT)~0
				|| (GLOBAL (CurrentActivity) & CHECK_ABORT))
			return; // Killed by Ilwrath
		
		// After Ilwrath battle, about-to-ally Starbase conversation
		SetCommIntroMode (CIM_CROSSFADE_SCREEN, 0);
		InitCommunication (COMMANDER_CONVERSATION);
		if (GLOBAL (CurrentActivity) & CHECK_ABORT)
			return;
		// XXX: InitCommunication() clears these flags, and we need them
		//   This marks that we are in Starbase.
		SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, (BYTE)~0);
	}

	prevMsgMode = SetStatusMessageMode (SMM_RES_UNITS);

	if (GET_GAME_STATE (MOONBASE_ON_SHIP)
			|| GET_GAME_STATE (CHMMR_BOMB_STATE) == 2)
	{	// Go immediately into a conversation with the Commander when the
		// Starbase becomes available for the first time, or after Chmmr
		// install the bomb.
		DoTimePassage ();
		if (GLOBAL_SIS (CrewEnlisted) == (COUNT)~0)
			return; // You are now dead! Thank you! (killed by Kohr-Ah)

		SetCommIntroMode (CIM_FADE_IN_SCREEN, ONE_SECOND * 2);
		InitCommunication (COMMANDER_CONVERSATION);
		if (GLOBAL (CurrentActivity) & CHECK_ABORT)
			return;
		// XXX: InitCommunication() clears these flags, and we need them
		//   This marks that we are in Starbase.
		SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, (BYTE)~0);
	}

	memset (&MenuState, 0, sizeof (MenuState));
	MenuState.InputFunc = DoStarBase;
	
	OldContext = SetContext (ScreenContext);
	DoInput (&MenuState, TRUE);
	SetContext (OldContext);

	SetStatusMessageMode (prevMsgMode);
	CleanupAfterStarBase ();
}

static void
CleanupAfterStarBase (void)
{
	if (!(GLOBAL (CurrentActivity) & (CHECK_LOAD | CHECK_ABORT)))
	{
		// Mark as not in Starbase anymore
		SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 0);
		// Fake a load so Starcon2Main takes us to IP
		GLOBAL (CurrentActivity) = CHECK_LOAD;
		NextActivity = MAKE_WORD (IN_INTERPLANETARY, 0)
				| START_INTERPLANETARY;
	}
}

void
InstallBombAtEarth (void)
{
	DoTimePassage ();

	GLOBAL (ShipStamp.origin.x) = SIS_SCREEN_WIDTH >> 1;
	GLOBAL (ShipStamp.origin.y) = SIS_SCREEN_HEIGHT >> 1;

	SetContext (ScreenContext);
	SetTransitionSource (NULL);
	SetContextBackGroundColor (BLACK_COLOR);
	ClearDrawable ();
	
	SleepThreadUntil (FadeScreen (FadeAllToColor, 0));
	
	SET_GAME_STATE (CHMMR_BOMB_STATE, 3); /* bomb processed */
	GLOBAL (CurrentActivity) = CHECK_LOAD; /* fake a load game */
	NextActivity = MAKE_WORD (IN_INTERPLANETARY, 0) | START_INTERPLANETARY;
	CurStarDescPtr = 0; /* force SolarSys reload */
}

// XXX: Doesn't really belong in this file.
COUNT
WrapText (const UNICODE *pStr, COUNT len, TEXT *tarray, SIZE field_width)
{
	COUNT num_lines;

	num_lines = 0;
	do
	{
		RECT r;
		COUNT OldCount;
		
		tarray->align = ALIGN_LEFT; /* set alignment to something */
		tarray->pStr = pStr;
		tarray->CharCount = 1;
		++num_lines;
		
		do
		{
			OldCount = tarray->CharCount;
			while (*++pStr != ' ' && (COUNT)(pStr - tarray->pStr) < len)
				;
			tarray->CharCount = pStr - tarray->pStr;
			TextRect (tarray, &r, NULL);
		} while (tarray->CharCount < len && r.extent.width < field_width);
	
		if (r.extent.width >= field_width)
		{
			if ((tarray->CharCount = OldCount) == 1)
			{
				do
				{
					++tarray->CharCount;
					TextRect (tarray, &r, NULL);
				} while (r.extent.width < field_width);
				--tarray->CharCount;
			}
		}
	
		pStr = tarray->pStr + tarray->CharCount;
		len -= tarray->CharCount;
		++tarray;
	
		if (len && (r.extent.width < field_width || OldCount > 1))
		{
			++pStr; /* skip white space */
			--len;
		}

	} while (len);

	return (num_lines);
}

