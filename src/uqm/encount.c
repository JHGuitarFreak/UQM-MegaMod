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

#include "encount.h"

#include "battle.h"
#include "battlecontrols.h"
#include "build.h"
#include "colors.h"
#include "starmap.h"
#include "cons_res.h"
#include "controls.h"
#include "menustat.h"
#include "gameopt.h"
#include "gamestr.h"
#include "globdata.h"
#include "sis.h"
		// for DrawStatusMessage(), SetStatusMessageMode()
#include "init.h"
#include "pickship.h"
#include "intel.h"
#include "nameref.h"
#include "resinst.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "util.h" // JMS: For SaveContextFrame()
#include "libs/graphics/gfx_common.h"
#include "libs/log.h"
#include "libs/mathlib.h"
#include "libs/inplib.h"
#include "libs/misc.h"


static void DrawFadeText (const UNICODE *str1, const UNICODE *str2,
		BOOLEAN fade_in, RECT *pRect);


static BOOLEAN
DoSelectAction (MENU_STATE *pMS)
{
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		pMS->CurState = ATTACK + 1;
		return (FALSE);
	}
	if (!pMS->Initialized)
	{
		pMS->Initialized = TRUE;
		pMS->InputFunc = DoSelectAction;
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		switch (pMS->CurState)
		{
			case HAIL:
			case ATTACK:
				if (LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
					pMS->CurState = HAIL;
				return (FALSE);
			case ATTACK + 1:
				// Clearing FlashRect is not necessary
				if (!GameOptions ())
					return FALSE;
				DrawMenuStateStrings (PM_CONVERSE, pMS->CurState);
				SetFlashRect (SFR_MENU_3DO);
				break;
			default:
				printf ("Unknown option: %d\n", pMS->CurState);
		}
	}
	DoMenuChooser (pMS, PM_CONVERSE);
	return (TRUE);
}

static QUEUE *
GetShipFragQueueForPlayer (COUNT playerNr)
{
	if (playerNr == RPG_PLAYER_NUM)
		return &GLOBAL (built_ship_q);
	else
		return &GLOBAL (npc_built_ship_q);
}

// Called by comm code to intialize battle fleets during encounter
void
BuildBattle (COUNT which_player)
{
	QUEUE *pQueue;
	HSHIPFRAG hStarShip, hNextShip;
	HSTARSHIP hBuiltShip;
	STARSHIP *BuiltShipPtr;

	EncounterRace = -1;

	if (GetHeadLink (&GLOBAL (npc_built_ship_q)) == 0)
	{
		SET_GAME_STATE (BATTLE_SEGUE, 0);
		return;
	}

	if (which_player != RPG_PLAYER_NUM)
	{	// This function is called first for the NPC character
		// and this is when a centerpiece is loaded
		switch (LOBYTE (GLOBAL (CurrentActivity)))
		{
			case IN_LAST_BATTLE:
				load_gravity_well (NUMBER_OF_PLANET_TYPES);
				break;
			case IN_HYPERSPACE:
				load_gravity_well ((BYTE)((COUNT)TFB_Random ()
						% NUMBER_OF_PLANET_TYPES));
				break;
			default:
				SET_GAME_STATE (ESCAPE_COUNTER, 110);
				load_gravity_well (GET_GAME_STATE (BATTLE_PLANET));
				break;
		}
	}
	pQueue = GetShipFragQueueForPlayer (which_player);

	ReinitQueue (&race_q[which_player]);
	for (hStarShip = GetHeadLink (pQueue);
			hStarShip != 0; hStarShip = hNextShip)
	{
		SHIP_FRAGMENT *FragPtr;

		FragPtr = LockShipFrag (pQueue, hStarShip);
		hNextShip = _GetSuccLink (FragPtr);

		hBuiltShip = Build (&race_q[which_player],
				FragPtr->race_id == SAMATRA_SHIP ?
					SA_MATRA_ID : FragPtr->SpeciesID);
		if (hBuiltShip)
		{
			BuiltShipPtr = LockStarShip (&race_q[which_player], hBuiltShip);
			BuiltShipPtr->captains_name_index = FragPtr->captains_name_index;
			BuiltShipPtr->playerNr = which_player;
			if (FragPtr->crew_level != INFINITE_FLEET)
				BuiltShipPtr->crew_level = FragPtr->crew_level;
			else /* if infinite ships */
				BuiltShipPtr->crew_level = FragPtr->max_crew;
			BuiltShipPtr->max_crew = FragPtr->max_crew;
			BuiltShipPtr->race_strings = FragPtr->race_strings;
			BuiltShipPtr->icons = FragPtr->icons;
			BuiltShipPtr->index = FragPtr->index;
			BuiltShipPtr->ship_cost = 0;
			BuiltShipPtr->RaceDescPtr = 0;

			UnlockStarShip (&race_q[which_player], hBuiltShip);
		}

		UnlockShipFrag (pQueue, hStarShip);
	}

	if (which_player == RPG_PLAYER_NUM
			&& (hBuiltShip = Build (&race_q[0], SIS_SHIP_ID)))
	{
		BuiltShipPtr = LockStarShip (&race_q[0], hBuiltShip);
		BuiltShipPtr->captains_name_index = 0;
		BuiltShipPtr->playerNr = RPG_PLAYER_NUM;
		BuiltShipPtr->crew_level = 0;
		BuiltShipPtr->max_crew = 0;
				// Crew will be copied directly from
				// GLOBAL_SIS (CrewEnlisted) later.
		BuiltShipPtr->race_strings = 0;
		BuiltShipPtr->icons = 0;
		BuiltShipPtr->index = -1;
		BuiltShipPtr->ship_cost = 0;
		BuiltShipPtr->energy_counter = MAX_ENERGY_SIZE;
		BuiltShipPtr->RaceDescPtr = 0;
		UnlockStarShip (&race_q[0], hBuiltShip);
	}
}

BOOLEAN
FleetIsInfinite (COUNT playerNr)
{
	QUEUE *pQueue;
	HSHIPFRAG hShipFrag;
	SHIP_FRAGMENT *FragPtr;
	BOOLEAN ret;

	pQueue = GetShipFragQueueForPlayer (playerNr);
	hShipFrag = GetHeadLink (pQueue);
	if (!hShipFrag)
	{	// Ship queue is empty in SuperMelee or for RPG player w/o escorts
		return FALSE;
	}

	FragPtr = LockShipFrag (pQueue, hShipFrag);
	ret = (FragPtr->crew_level == INFINITE_FLEET);
	UnlockShipFrag (pQueue, hShipFrag);

	return ret;
}

void
UpdateShipFragCrew (STARSHIP *StarShipPtr)
{
	QUEUE *frag_q;
	HSHIPFRAG hShipFrag, hNextFrag;
	SHIP_FRAGMENT *frag;
	QUEUE *ship_q;
	HSTARSHIP hStarShip, hNextShip;
	STARSHIP *ship;

	frag_q = GetShipFragQueueForPlayer (StarShipPtr->playerNr);
	ship_q = &race_q[StarShipPtr->playerNr];

	// Find a SHIP_FRAGMENT that corresponds to the given STARSHIP
	// The ships and fragments are in the same order in two queues
	// XXX: It would probably be simpler to keep HSHIPFRAG in STARSHIP struct
	for (hShipFrag = GetHeadLink (frag_q), hStarShip = GetHeadLink (ship_q);
			hShipFrag != 0 && hStarShip != 0;
			hShipFrag = hNextFrag, hStarShip = hNextShip)
	{
		ship = LockStarShip (ship_q, hStarShip);
		hNextShip = _GetSuccLink (ship);
		frag = LockShipFrag (frag_q, hShipFrag);
		hNextFrag = _GetSuccLink (frag);
		
		if (ship == StarShipPtr)
		{
			assert (frag->crew_level != INFINITE_FLEET);
			
			// Record crew left after the battle */
			frag->crew_level = ship->crew_level;
			
			UnlockShipFrag (frag_q, hShipFrag);
			UnlockStarShip (ship_q, hStarShip);
			break;
		}
		
		UnlockShipFrag (frag_q, hShipFrag);
		UnlockStarShip (ship_q, hStarShip);
	}
}

/*
 * Encountering an alien.
 * Draws the encounter screen, plays the red alert music, and
 * waits for a decision of the player on how to handle the situation.
 * Returns either HAIL or ATTACK.
 */
COUNT
InitEncounter (void)
{
	COUNT i;
	FRAME SegueFrame;
	STAMP s;
	TEXT t;
	extern FRAME planet[];
	MUSIC_REF MR;


	SetContext (SpaceContext);
	SetContextFont (TinyFont);

	MR = LoadMusic (REDALERT_MUSIC);
	PlayMusic (MR, FALSE, 1);
	SegueFrame = CaptureDrawable (LoadGraphic (SEGUE_PMAP_ANIM));
	WaitForSoundEnd (TFBSOUND_WAIT_ALL);
	StopMusic ();
	DestroyMusic (MR);
	s.origin.x = s.origin.y = 0;

	SetTransitionSource (NULL);
	BatchGraphics ();
	
	SetContextBackGroundColor (BLACK_COLOR);
	ClearDrawable ();
	s.frame = SegueFrame;
	DrawStamp (&s);

//    t.baseline.x = SIS_SCREEN_WIDTH >> 1;
	t.baseline.x = (SIS_SCREEN_WIDTH >> 1) + 1;
	t.baseline.y = RES_SCALE(10); // JMS_GFX
	t.align = ALIGN_CENTER;

	SetContextFont (MicroFont);
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x14), 0x01));
	if (inHQSpace ())
	{
		t.pStr = GAME_STRING (ENCOUNTER_STRING_BASE + 0);
				// "ENCOUNTER IN"
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
		t.baseline.y += RES_SCALE(12); // JMS_GFX
		t.pStr = GAME_STRING (ENCOUNTER_STRING_BASE + 1);
				// "DEEP SPACE"
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
	}
	else
	{
		UNICODE buf[256];

		t.pStr = GAME_STRING (ENCOUNTER_STRING_BASE + 2);
				// "ENCOUNTER AT"
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
		t.baseline.y += RES_SCALE(12); // JMS_GFX
		GetClusterName (CurStarDescPtr, buf);
		t.pStr = buf;
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
		t.baseline.y += RES_SCALE(12); // JMS_GFX
		t.pStr = GLOBAL_SIS (PlanetName);
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
	}

	s.origin.x = SIS_SCREEN_WIDTH >> 1;
	s.origin.y = SIS_SCREEN_HEIGHT >> 1;
	s.frame = planet[0];
	DrawStamp (&s);

	if (LOBYTE (GLOBAL (CurrentActivity)) != IN_LAST_BATTLE)
	{
#define NUM_DISPLAY_PTS (sizeof (display_pt) / sizeof (display_pt[0]))
		HSHIPFRAG hStarShip, hNextShip;
		POINT display_pt[] =
		{
			{ 10,  51},
			{-10,  51},
			{ 33,  40},
			{-33,  40},
			{ 49,  18},
			{-49,  18},
			{ 52,  -6},
			{-52,  -6},
			{ 44, -27},
			{-44, -27},
		};

		for (hStarShip = GetHeadLink (&GLOBAL (npc_built_ship_q)), i = 0;
				hStarShip && i < 60; hStarShip = hNextShip, ++i)
		{
			RECT r;
			SHIP_FRAGMENT *FragPtr;

			FragPtr = LockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
			if (FragPtr->crew_level != INFINITE_FLEET)
				hNextShip = _GetSuccLink (FragPtr);
			else /* if infinite ships */
				hNextShip = hStarShip;

			s.origin = display_pt[i % NUM_DISPLAY_PTS];
			if (i >= NUM_DISPLAY_PTS)
			{
				COUNT angle, radius;

				radius = square_root ((long)s.origin.x * s.origin.x
						+ (long)s.origin.y * s.origin.y)
						+ ((i / NUM_DISPLAY_PTS) * 18);

				radius <<= RESOLUTION_FACTOR; // JMS_GFX

				angle = ARCTAN (s.origin.x, s.origin.y);
				s.origin.x = (COSINE (angle, radius));
				s.origin.y = (SINE (angle, radius));
			} else {
				s.origin.x <<= RESOLUTION_FACTOR; // JMS_GFX
				s.origin.y <<= RESOLUTION_FACTOR; // JMS_GFX
			}
			s.frame = SetAbsFrameIndex (FragPtr->icons, 0);
			GetFrameRect (s.frame, &r);
			s.origin.x += (SIS_SCREEN_WIDTH >> 1) - (r.extent.width >> 1);
			s.origin.y += (SIS_SCREEN_HEIGHT >> 1) - (r.extent.height >> 1);
			DrawStamp (&s);

			UnlockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
		}
	}

	UnbatchGraphics ();
	DestroyDrawable (ReleaseDrawable (SegueFrame));
	ScreenTransition (3, NULL);


	{
		MENU_STATE MenuState;

		MenuState.InputFunc = DoSelectAction;
		MenuState.Initialized = FALSE;

		DrawMenuStateStrings (PM_CONVERSE, MenuState.CurState = HAIL);
		SetFlashRect (SFR_MENU_3DO);

		DoInput (&MenuState, TRUE);

		SetFlashRect (NULL);

		return (MenuState.CurState);
	}
}

static void
DrawFadeText (const UNICODE *str1, const UNICODE *str2, BOOLEAN fade_in,
		RECT *pRect)
{
	SIZE i;
	DWORD TimeIn;
	TEXT t1, t2;
	RECT r1, r2;
	static const Color fade_cycle[] =
	{
		BUILD_COLOR (MAKE_RGB15_INIT (0x0A, 0x0A, 0x0A), 0x1D),
		BUILD_COLOR (MAKE_RGB15_INIT (0x09, 0x09, 0x09), 0x1E),
		BUILD_COLOR (MAKE_RGB15_INIT (0x08, 0x08, 0x08), 0x1F),
		BUILD_COLOR (MAKE_RGB15_INIT (0x06, 0x06, 0x06), 0x20),
		BUILD_COLOR (MAKE_RGB15_INIT (0x05, 0x05, 0x05), 0x21),
		BUILD_COLOR (MAKE_RGB15_INIT (0x04, 0x04, 0x04), 0x22),
		BUILD_COLOR (MAKE_RGB15_INIT (0x03, 0x03, 0x03), 0x23),
	};
#define NUM_FADES (sizeof (fade_cycle) / sizeof (fade_cycle[0]))

	t1.baseline.x = pRect->corner.x + RES_SCALE(100); // JMS_GFX
	t1.baseline.y = pRect->corner.y + RES_SCALE(45); // JMS_GFX
	t1.align = ALIGN_CENTER;
	t1.pStr = str1;
	t1.CharCount = (COUNT)~0;
	t2 = t1;
	t2.baseline.y += RES_SCALE(11); // JMS_GFX
	t2.pStr = str2;

	FlushInput ();
	TimeIn = GetTimeCounter ();
	if (fade_in)
	{
		for (i = 0; i < (SIZE) NUM_FADES; ++i)
		{
			if (AnyButtonPress (TRUE))
				i = NUM_FADES - 1;

			SetContextForeGroundColor (fade_cycle[i]);
			font_DrawText (&t1);
			font_DrawText (&t2);
			SleepThreadUntil (TimeIn + (ONE_SECOND / 20));
			TimeIn = GetTimeCounter ();
		}
	}
	else
	{
		for (i = NUM_FADES - 1; i >= 0; --i)
		{
			if (AnyButtonPress (TRUE))
				i = 0;

			SetContextForeGroundColor (fade_cycle[i]);
			font_DrawText (&t1);
			font_DrawText (&t2);
			SleepThreadUntil (TimeIn + (ONE_SECOND / 20));
			TimeIn = GetTimeCounter ();
		}
		SetContextForeGroundColor (
				BUILD_COLOR_RGBA (0x50, 0x50, 0x50, 0xff));
		TextRect(&t1, &r1, NULL);
		TextRect(&t2, &r2, NULL);
		DrawFilledRectangle (&r1);
		DrawFilledRectangle (&r2);
	}
}

COUNT
UninitEncounter (void)
{
	COUNT ships_killed;

	ships_killed = 0;

	free_gravity_well ();

	if ((GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
			|| GLOBAL_SIS (CrewEnlisted) == (COUNT)~0
			|| LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE
			|| LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
		goto ExitUninitEncounter;

	if (GET_GAME_STATE (BATTLE_SEGUE) == 0)
	{
		ReinitQueue (&race_q[0]);
		ReinitQueue (&race_q[1]);
	}
	else
	{
		BOOLEAN Sleepy;
		SIZE VictoryState, i;
		COUNT RecycleAmount = 0;
		RECT r, save_r;
		RECT scavenge_r = {{0, 0}, {0, 0}};
		TEXT t;
		STAMP ship_s, saveMetallicFrame;
		const UNICODE *str1 = NULL;
		const UNICODE *str2 = NULL;
		StatMsgMode prevMsgMode;
		UNICODE buf[80];
		HSHIPFRAG hStarShip;
		SHIP_FRAGMENT *FragPtr;

		static const Color fade_ship_cycle[] =
		{
			BUILD_COLOR (MAKE_RGB15_INIT (0x07, 0x00, 0x00), 0x2F),
			BUILD_COLOR (MAKE_RGB15_INIT (0x0F, 0x00, 0x00), 0x2D),
			BUILD_COLOR (MAKE_RGB15_INIT (0x17, 0x00, 0x00), 0x2B),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0A, 0x0A), 0x27),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x14, 0x14), 0x25),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x1F, 0x1F), 0x0F),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x14, 0x14), 0x25),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0A, 0x0A), 0x27),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1B, 0x00, 0x00), 0x2A),
			BUILD_COLOR (MAKE_RGB15_INIT (0x17, 0x00, 0x00), 0x2B),
		};
#define NUM_SHIP_FADES (sizeof (fade_ship_cycle) / \
		sizeof (fade_ship_cycle[0]))

		COUNT race_bounty[] =
		{
			RACE_SHIP_COST
		};

		SET_GAME_STATE (BATTLE_SEGUE, 0);
		SET_GAME_STATE (BOMB_CARRIER, 0);

		VictoryState = (
				battle_counter[1] || !battle_counter[0]
				|| GET_GAME_STATE (URQUAN_PROTECTING_SAMATRA)
				) ? 0 : 1;

		hStarShip = GetHeadLink (&GLOBAL (npc_built_ship_q));
		FragPtr = LockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
		EncounterRace = FragPtr->race_id;
		if (GetStarShipFromIndex (&GLOBAL (avail_race_q), EncounterRace) == 0)
		{
			/* Suppress the final tally and salvage info */
			VictoryState = -1;
			InitSISContexts ();
		}
		UnlockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);

		prevMsgMode = SetStatusMessageMode (SMM_RES_UNITS);
		ship_s.origin.x = 0;
		ship_s.origin.y = 0;
		Sleepy = TRUE;
		for (i = 0; i < NUM_SIDES; ++i)
		{
			QUEUE *pQueue;
			HSHIPFRAG hNextShip;

			if (i == 0)
				pQueue = &GLOBAL (built_ship_q);
			else
			{
				if (VictoryState < 0)
					VictoryState = 0;
				else
				{
					DrawSISFrame ();
					DrawSISMessage (NULL);
					if (inHQSpace ())
						DrawHyperCoords (GLOBAL (ShipStamp.origin));
					else if (GLOBAL (ip_planet) == 0)
						DrawHyperCoords (CurStarDescPtr->star_pt);
					else
						DrawSISTitle(GLOBAL_SIS (PlanetName));

					SetContext (SpaceContext);
					if (VictoryState)
						DrawArmadaPickShip (TRUE, &scavenge_r);
				}
				pQueue = &GLOBAL (npc_built_ship_q);
			}
				
			ReinitQueue (&race_q[(NUM_SIDES - 1) - i]);

			for (hStarShip = GetHeadLink (pQueue); hStarShip;
					hStarShip = hNextShip)
			{
				FragPtr = LockShipFrag (pQueue, hStarShip);
				hNextShip = _GetSuccLink (FragPtr);

				if (FragPtr->crew_level == 0
						|| (VictoryState && i == NUM_SIDES - 1))
				{
					if (i == NUM_SIDES - 1)
					{
						++ships_killed;
						if (VictoryState)
						{
#define MAX_DEAD_DISPLAYED 5
							COUNT j;

							if (ships_killed == 1)
							{
								RecycleAmount = 0;

								DrawStatusMessage (NULL);
								
								ship_s.origin.x = scavenge_r.corner.x + RES_SCALE(32); // JMS_GFX
								ship_s.origin.y = scavenge_r.corner.y + RES_SCALE(56); // JMS_GFX
								ship_s.frame = IncFrameIndex (FragPtr->icons);
								DrawStamp (&ship_s);
								SetContextForeGroundColor (
										BUILD_COLOR (MAKE_RGB15 (0x08, 0x08, 0x08), 0x1F));
								SetContextFont (TinyFont);

								utf8StringCopy (buf, sizeof buf,
										GetStringAddress (FragPtr->race_strings));
								// XXX: this will not work with UTF-8 strings
								strupr (buf);

								// JMS: Handling the a-umlaut and o-umlaut characters
								{
									unsigned char *ptr;
									ptr = (unsigned char*)buf;
									while (*ptr) {
										if (*ptr == 0xc3) {
											ptr++;
											if (*ptr == 0xb6 || *ptr == 0xa4) {
												*ptr += 'A' - 'a';
											}
										}
										ptr++;
									}
								}

								t.baseline.x = scavenge_r.corner.x + RES_SCALE(100); // JMS_GFX
								t.baseline.y = scavenge_r.corner.y + RES_SCALE(68); // JMS_GFX
								t.align = ALIGN_CENTER;
								t.pStr = buf;
								t.CharCount = (COUNT)~0;
								font_DrawText (&t);
								t.baseline.y += RES_SCALE(6); // JMS_GFX
								t.pStr = GAME_STRING (
										ENCOUNTER_STRING_BASE + 3);
										// "BATTLE GROUP"
								t.CharCount = (COUNT)~0;
								font_DrawText (&t);

								ship_s.frame = FragPtr->icons;

								SetContextFont (MicroFont);

								// JMS: Let's store the rectangle behind "Enemy ships destroyed" (before drawing the text on it).
								if (IS_HD)
								{
									// These values are inferred from DrawFadeText.
									// However, they're not the same (100 and 45) because the text there is centered,
									// but these rect coords are for the upper-left corner, not center.
									save_r.corner.x = scavenge_r.corner.x + RES_SCALE(70); // JMS_GFX
									save_r.corner.y = scavenge_r.corner.y + (RES_SCALE(35)); // JMS_GFX
									
									// These are wild-assed guesses.
									save_r.extent.width  = RES_SCALE(60);
									save_r.extent.height = RES_SCALE(30); 
									
									// Now that we have the size and placement of the rectangle, let's store it.
									saveMetallicFrame = SaveContextFrame (&save_r);
								}

								str1 = GAME_STRING (
										ENCOUNTER_STRING_BASE + 4);
										// "Enemy Ships"
								str2 = GAME_STRING (
										ENCOUNTER_STRING_BASE + 5),
										// "Destroyed"
								DrawFadeText (str1, str2, TRUE, &scavenge_r);
							}

							r.corner.y = scavenge_r.corner.y + RES_SCALE(9); // JMS_GFX
							r.extent.height = RES_SCALE(22); // JMS_GFX

							SetContextForeGroundColor (BLACK_COLOR);

							r.extent.width = RES_SCALE(34); // JMS_GFX
							r.corner.x = scavenge_r.corner.x +
									scavenge_r.extent.width
									- (RES_SCALE(10) + r.extent.width); // JMS_GFX
							DrawFilledRectangle (&r);

							/* collect bounty ResUnits */
							j = race_bounty[EncounterRace] >> 3;
							RecycleAmount += j;
							sprintf (buf, "%u", RecycleAmount);
							t.baseline.x = r.corner.x + r.extent.width - 1 - 5 * RESOLUTION_FACTOR; // JMS_GFX;
							t.baseline.y = r.corner.y + RES_SCALE(14); // JMS_GFX
							t.align = ALIGN_RIGHT;
							t.pStr = buf;
							t.CharCount = (COUNT)~0;
							SetContextForeGroundColor (
									BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x18), 0x50));
							font_DrawText (&t);
							DeltaSISGauges (0, 0, j);

							if ((VictoryState++ - 1) % MAX_DEAD_DISPLAYED)
								ship_s.origin.x += RES_SCALE(17); // JMS_GFX
							else
							{
								SetContextForeGroundColor (BLACK_COLOR);

								r.corner.x = scavenge_r.corner.x + RES_SCALE(10); // JMS_GFX
								r.extent.width = RES_SCALE(104); // JMS_GFX
								DrawFilledRectangle (&r);

								ship_s.origin.x = r.corner.x + RES_SCALE(2); // JMS_GFX
								ship_s.origin.y = scavenge_r.corner.y + RES_SCALE(12); // JMS_GFX
							}

							if (Sleepy)
							{
								TimeCount Time = GetTimeCounter ();
								for (j = 0; j < NUM_SHIP_FADES; ++j)
								{
									Sleepy = (BOOLEAN)!AnyButtonPress (TRUE) &&
											!(GLOBAL (CurrentActivity) & CHECK_ABORT);
									if (!Sleepy)
										break;

									SetContextForeGroundColor (fade_ship_cycle[j]);
									DrawFilledStamp (&ship_s);

									SleepThreadUntil (Time + (ONE_SECOND / 15));
									Time = GetTimeCounter ();
								}
							}

							if (IS_HD) {
								SetContextForeGroundColor (BLACK_COLOR);
								DrawFilledStamp (&ship_s);
								DrawFilledStamp (&ship_s);
								DrawFilledStamp (&ship_s);
								DrawFilledStamp (&ship_s);
								DrawFilledStamp (&ship_s);
								DrawFilledStamp (&ship_s);
								DrawFilledStamp (&ship_s);
								DrawFilledStamp (&ship_s);
								DrawFilledStamp (&ship_s);
							}

							DrawStamp (&ship_s);
						}
					}

					UnlockShipFrag (pQueue, hStarShip);
					RemoveQueue (pQueue, hStarShip);
					FreeShipFrag (pQueue, hStarShip);

					continue;
				}

				UnlockShipFrag (pQueue, hStarShip);
			}
		}
		SetStatusMessageMode (prevMsgMode);

		if (VictoryState)
		{
#ifdef NEVER
			DestroyDrawable (ReleaseDrawable (s.frame));
#endif /* NEVER */

			WaitForAnyButton (TRUE, ONE_SECOND * 3, FALSE);
			if (!CurrentInputState.key[PlayerControls[0]][KEY_ESCAPE])
			{
				DrawFadeText (str1, str2, FALSE, &scavenge_r);
				if (!CurrentInputState.key[PlayerControls[0]][KEY_ESCAPE])
				{
					SetContextForeGroundColor (BLACK_COLOR);
					r.corner.x = scavenge_r.corner.x + RES_SCALE(10); // JMS_GFX
					r.extent.width = RES_SCALE(132); // JMS_GFX
					DrawFilledRectangle (&r);
					sprintf (buf, "%u %s", RecycleAmount,
							GAME_STRING (STATUS_STRING_BASE + 1)); // "RU"
					t.baseline.x = r.corner.x + (r.extent.width >> 1);
					t.baseline.y = r.corner.y + RES_SCALE(14); // JMS_GFX
					t.align = ALIGN_CENTER;
					t.pStr = buf;
					t.CharCount = (COUNT)~0;
					SetContextForeGroundColor (
							BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x18), 0x50));
					font_DrawText (&t);

					str1 = GAME_STRING (ENCOUNTER_STRING_BASE + 6);
							// "Debris"
					str2 = GAME_STRING (ENCOUNTER_STRING_BASE + 7);
							// "Scavenged"

					// JMS: Now we draw the clean metallic frame to erase the "Enemy ships destroyed"
					// text before drawing "debris scavenged."
					if(IS_HD)
						DrawStamp (&saveMetallicFrame);

					DrawFadeText (str1, str2, TRUE, &scavenge_r);
					WaitForAnyButton (TRUE, ONE_SECOND * 2, FALSE);
					if (!CurrentInputState.key[PlayerControls[0]][KEY_ESCAPE])
						DrawFadeText (str1, str2, FALSE, &scavenge_r);

					// JMS: The final cleanup of the "Debris scavenged". Without this, an ugly grey ghost-text would remain.
					if(IS_HD)
						DrawStamp (&saveMetallicFrame);
				}
			}

			DrawStatusMessage (NULL);
		}

		if (ships_killed && EncounterRace == THRADDASH_SHIP
				&& !GET_GAME_STATE (THRADD_MANNER))
		{
			if ((ships_killed += GET_GAME_STATE (THRADDASH_BODY_COUNT)) >
					THRADDASH_BODY_THRESHOLD)
				ships_killed = THRADDASH_BODY_THRESHOLD;
			SET_GAME_STATE (THRADDASH_BODY_COUNT, ships_killed);
		}
	}
ExitUninitEncounter:

	return (ships_killed);
}

void
EncounterBattle (void)
{
	ACTIVITY OldActivity;
	extern UWORD nth_frame;
	InputContext *savedPlayerInput = NULL;


	SET_GAME_STATE (BATTLE_SEGUE, 1);

	OldActivity = GLOBAL (CurrentActivity);
	if (LOBYTE (OldActivity) == IN_LAST_BATTLE)
		GLOBAL (CurrentActivity) = MAKE_WORD (IN_LAST_BATTLE, 0);
	else
		GLOBAL (CurrentActivity) = MAKE_WORD (IN_ENCOUNTER, 0);

//    FreeSC2Data ();
//    DestroyFont (ReleaseFont (MicroFont));
	WaitForSoundEnd (TFBSOUND_WAIT_ALL);
//    DestroySound (ReleaseSound (MenuSounds));

	if (GLOBAL (glob_flags) & CYBORG_ENABLED)
	{
		BYTE cur_speed;

		cur_speed = (BYTE)(GLOBAL (glob_flags) & COMBAT_SPEED_MASK)
				>> COMBAT_SPEED_SHIFT;
		if (cur_speed == 1)
			cur_speed = 0; /* normal speed */
		else if (cur_speed == 2)
			++cur_speed;   /* 4x speed, 3 of 4 frames skipped */
		else /* if (cur_speed == 3) */
			cur_speed = (BYTE)~0; /* maximum speed - no rendering */
		nth_frame = MAKE_WORD (1, cur_speed);
		PlayerControl[0] = CYBORG_CONTROL | AWESOME_RATING;
		savedPlayerInput = PlayerInput[0];
		PlayerInput[0] = NULL;
		if (!SetPlayerInput (0)) {
			log_add (log_Fatal, "Could not set cyborg player input.");
			explode ();  // Does not return;
		}
	}

	if(DIF_EASY)
		PlayerControl[1] = CYBORG_CONTROL | STANDARD_RATING;

	GameSounds = CaptureSound (LoadSound (GAME_SOUNDS));

	Battle (NULL);

	DestroySound (ReleaseSound (GameSounds));
	GameSounds = 0;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		GLOBAL_SIS (CrewEnlisted) = (COUNT)~0;

	if (GLOBAL (glob_flags) & CYBORG_ENABLED)
	{
		nth_frame = MAKE_WORD (0, 0);
		PlayerControl[0] = HUMAN_CONTROL | STANDARD_RATING;
		ClearPlayerInput (0);
		PlayerInput[0] = savedPlayerInput;
	}

//    MicroFont = CaptureFont (
// LoadFont (MICRO_FONT)
// );
//    MenuSounds = CaptureSound (LoadSound (MENU_SOUNDS));
//    LoadSC2Data ();

	GLOBAL (CurrentActivity) = OldActivity;

}

