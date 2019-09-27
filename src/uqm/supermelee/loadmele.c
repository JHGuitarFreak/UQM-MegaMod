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

// This file handles loading of teams, but the UI and the actual loading.

#define MELEESETUP_INTERNAL
#include "melee.h"

#include "../controls.h"
#include "../gameopt.h"
#include "../gamestr.h"
#include "../globdata.h"
#include "../master.h"
#include "meleesetup.h"
#include "../save.h"
#include "../setup.h"
#include "../sounds.h"
#include "options.h"
#include "libs/log.h"
#include "libs/memlib.h"


#define LOAD_TEAM_NAME_TEXT_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x0F, 0x10, 0x1B), 0x00)
#define LOAD_TEAM_NAME_TEXT_COLOR_HILITE \
		BUILD_COLOR (MAKE_RGB15 (0x17, 0x18, 0x1D), 0x00)


#define LOAD_MELEE_BOX_WIDTH RES_SCALE(34) // JMS_GFX
#define LOAD_MELEE_BOX_HEIGHT RES_SCALE(34) // JMS_GFX
#define LOAD_MELEE_BOX_SPACE RES_SCALE(1) // JMS_GFX


static void DrawFileStrings (MELEE_STATE *pMS);
static bool FillFileView (MELEE_STATE *pMS);


static bool
LoadTeamImage (DIRENTRY DirEntry, MeleeTeam *team)
{
	const char *fileName;
	uio_Stream *stream;

	fileName = GetDirEntryAddress (DirEntry);

	stream = uio_fopen (meleeDir, fileName, "rb");
	if (stream == NULL)
		return false;

	if (MeleeTeam_deserialize (team, stream) == -1)
		return false;

	uio_fclose (stream);

	return true;
}

#if 0  /* Not used */
static void
UnindexFleet (MELEE_STATE *pMS, COUNT index)
{
	assert (index < pMS->load.numIndices);
	pMS->load.numIndices--;
	memmove (&pMS->load.entryIndices[index],
			&pMS->load.entryIndices[index + 1],
			(pMS->load.numIndices - index) * sizeof pMS->load.entryIndices[0]);
}
#endif

static void
UnindexFleets (MELEE_STATE *pMS, COUNT index, COUNT count)
{
	assert (index + count <= pMS->load.numIndices);

	pMS->load.numIndices -= count;
	memmove (&pMS->load.entryIndices[index],
			&pMS->load.entryIndices[index + count],
			(pMS->load.numIndices - index) * sizeof pMS->load.entryIndices[0]);
}

static bool
GetFleetByIndex (MELEE_STATE *pMS, COUNT index, MeleeTeam *result)
{
	COUNT firstIndex;

	if (index < pMS->load.preBuiltCount)
	{
		MeleeTeam_copy (result, pMS->load.preBuiltList[index]);
		return true;
	}

	index -= pMS->load.preBuiltCount;
	firstIndex = index;

	for ( ; index < pMS->load.numIndices; index++)
	{
		DIRENTRY entry = SetAbsDirEntryTableIndex (pMS->load.dirEntries,
				pMS->load.entryIndices[index]);
		if (LoadTeamImage (entry, result))
			break;  // Success

		{
			const char *fileName;
			fileName = GetDirEntryAddress (entry);
			log_add (log_Warning, "Warning: File '%s' is not a valid "
					"SuperMelee team.", fileName);
		}
	}

	if (index != firstIndex)
		UnindexFleets (pMS, firstIndex, index - firstIndex);

	return index < pMS->load.numIndices;
}

// returns (COUNT) -1 if not found
static COUNT
GetFleetIndexByFileName (MELEE_STATE *pMS, const char *fileName)
{
	COUNT index;
	
	for (index = 0; index < pMS->load.numIndices; index++)
	{
		DIRENTRY entry = SetAbsDirEntryTableIndex (pMS->load.dirEntries,
				pMS->load.entryIndices[index]);
		const char *entryName = GetDirEntryAddress (entry);

		if (strcasecmp ((const char *) entryName, fileName) == 0)
			return pMS->load.preBuiltCount + index;
	}

	return (COUNT) -1;
}

// Auxiliary function for DrawFileStrings
// If drawShips is set the ships themselves are drawn, in addition to the
// fleet name and value; if not, only the fleet name and value are drawn.
// If highlite is set the text is drawn in the color used for highlighting.
static void
DrawFileString (const MeleeTeam *team, const POINT *origin,
		BOOLEAN drawShips, BOOLEAN highlite)
{
	SetContextForeGroundColor (highlite ?
			LOAD_TEAM_NAME_TEXT_COLOR_HILITE : LOAD_TEAM_NAME_TEXT_COLOR);

	// Print the name of the fleet
	{
		TEXT Text;

		Text.baseline = *origin;
		Text.align = ALIGN_LEFT;
		Text.pStr = MeleeTeam_getTeamName(team);
		Text.CharCount = (COUNT)~0;
		font_DrawText (&Text);
	}

	// Print the value of the fleet
	{
		TEXT Text;
		UNICODE buf[60];

		sprintf (buf, "%u", MeleeTeam_getValue (team));
		Text.baseline = *origin;
		Text.baseline.x += NUM_MELEE_COLUMNS *
				(LOAD_MELEE_BOX_WIDTH + LOAD_MELEE_BOX_SPACE) - 1;
		Text.align = ALIGN_RIGHT;
		Text.pStr = buf;
		Text.CharCount = (COUNT)~0;
		font_DrawText (&Text);
	}

	// Draw the ships for the fleet
	if (drawShips)
	{
		STAMP s;
		FleetShipIndex slotI;

		s.origin.x = origin->x + RES_SCALE(1); // JMS_GFX
		s.origin.y = origin->y + RES_STAT_SCALE(4); // JMS_GFX
		for (slotI = 0; slotI < MELEE_FLEET_SIZE; slotI++)
		{
			BYTE StarShip;
				
			StarShip = team->ships[slotI];
			if (StarShip != MELEE_NONE)
			{
				s.frame = GetShipIconsFromIndex (StarShip);
				DrawStamp (&s);
				s.origin.x += RES_SCALE(17); // JMS_GFX
			}
		}
	}
}

// returns true if there are any entries in the view, in which case
// pMS->load.bot gets set to the index just past the bottom entry in the view.
// returns false if not, in which case, the entire view remains unchanged.
static bool
FillFileView (MELEE_STATE *pMS)
{
	COUNT viewI;

	for (viewI = 0; viewI < LOAD_TEAM_VIEW_SIZE; viewI++)
	{
		bool success = GetFleetByIndex (pMS, pMS->load.top + viewI,
				pMS->load.view[viewI]);
		if (!success)
			break;
	}

	if (viewI == 0)
		return false;

	pMS->load.bot = pMS->load.top + viewI;
	return true;
}

#define FILE_STRING_ORIGIN_X RES_SCALE(5) // JMS_GFX
#define FILE_STRING_ORIGIN_Y  RES_SCALE(34) // JMS_GFX
#define ENTRY_HEIGHT RES_SCALE(32) // JMS_GFX

static void
SelectFileString (MELEE_STATE *pMS, bool hilite)
{
	CONTEXT OldContext;
	POINT origin;
	COUNT viewI;

	viewI = pMS->load.cur - pMS->load.top;

	OldContext = SetContext (SpaceContext);
	SetContextFont (MicroFont);
	BatchGraphics ();

	origin.x = FILE_STRING_ORIGIN_X;
	origin.y = FILE_STRING_ORIGIN_Y + viewI * ENTRY_HEIGHT;
	DrawFileString (pMS->load.view[viewI], &origin, FALSE, hilite);

	UnbatchGraphics ();
	SetContext (OldContext);
}

static void
DrawFileStrings (MELEE_STATE *pMS)
{
	POINT origin;
	CONTEXT OldContext;

	origin.x = FILE_STRING_ORIGIN_X;
	origin.y = FILE_STRING_ORIGIN_Y;
		
	OldContext = SetContext (SpaceContext);
	SetContextFont (MicroFont);
	BatchGraphics ();

	DrawMeleeIcon (28);  /* The load team frame */

	if (FillFileView (pMS))
	{
		COUNT i;
		for (i = pMS->load.top; i < pMS->load.bot; i++) {
			DrawFileString (pMS->load.view[i - pMS->load.top], &origin,
					TRUE, FALSE);
			origin.y += ENTRY_HEIGHT;
		}
	}

	UnbatchGraphics ();
	SetContext (OldContext);
}

static void
RefocusView (MELEE_STATE *pMS, COUNT index)
{
	assert (index < pMS->load.preBuiltCount + pMS->load.numIndices);
		
	pMS->load.cur = index;
	if (index <= LOAD_TEAM_VIEW_SIZE / 2)
		pMS->load.top = 0;
	else
		pMS->load.top = index - LOAD_TEAM_VIEW_SIZE / 2;
}

static void
flashSelectedTeam (MELEE_STATE *pMS)
{
#define FLASH_RATE (ONE_SECOND / 9)
	static TimeCount NextTime = 0;
	static int hilite = 0;
	TimeCount Now = GetTimeCounter ();

	if (Now >= NextTime)
	{
		CONTEXT OldContext;

		NextTime = Now + FLASH_RATE;
		hilite ^= 1;

		OldContext = SetContext (SpaceContext);
		SelectFileString (pMS, hilite);
		SetContext (OldContext);
	}
}

BOOLEAN
DoLoadTeam (MELEE_STATE *pMS)
{
	DWORD TimeIn = GetTimeCounter ();

	/* Cancel any presses of the Pause key. */
	GamePaused = FALSE;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return FALSE;

	SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN | MENU_SOUND_PAGE, 
		MENU_SOUND_SELECT);

	if (!pMS->Initialized)
	{
		DrawFileStrings (pMS);
		SelectFileString (pMS, true);
		pMS->Initialized = TRUE;
		pMS->InputFunc = DoLoadTeam;
		return TRUE;
	}

	if (PulsedInputState.menu[KEY_MENU_SELECT] ||
			PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		if (PulsedInputState.menu[KEY_MENU_SELECT])
		{
			// Copy the selected fleet to the player.
			Melee_LocalChange_team (pMS, pMS->side,
					pMS->load.view[pMS->load.cur - pMS->load.top]);
		}

		pMS->InputFunc = DoMelee;
		pMS->LastInputTime = GetTimeCounter ();
		{
			RECT r;
			
			GetFrameRect (SetAbsFrameIndex (MeleeFrame, 28), &r);
			RepairMeleeFrame (&r);
		}
		return TRUE;
	}
	
	{
		COUNT newTop = pMS->load.top;
		COUNT newIndex = pMS->load.cur;

		if (PulsedInputState.menu[KEY_MENU_UP])
		{
			if (newIndex > 0)
			{
				newIndex--;
				if (newIndex < newTop)
					newTop = (newTop < LOAD_TEAM_VIEW_SIZE) ?
							0 : newTop - LOAD_TEAM_VIEW_SIZE;
			}
		}
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
		{
			COUNT numEntries = pMS->load.numIndices + pMS->load.preBuiltCount;
			if (newIndex + 1 < numEntries)
			{
				newIndex++;
				if (newIndex >= pMS->load.bot)
					newTop = pMS->load.bot;
			}
		}
		else if (PulsedInputState.menu[KEY_MENU_PAGE_UP])
		{
			newIndex = (newIndex < LOAD_TEAM_VIEW_SIZE) ?
					0 : newIndex - LOAD_TEAM_VIEW_SIZE;
			newTop = (newTop < LOAD_TEAM_VIEW_SIZE) ?
					0 : newTop - LOAD_TEAM_VIEW_SIZE;
		}
		else if (PulsedInputState.menu[KEY_MENU_PAGE_DOWN])
		{
			COUNT numEntries = pMS->load.numIndices + pMS->load.preBuiltCount;
			if (newIndex + LOAD_TEAM_VIEW_SIZE < numEntries)
			{
				newIndex += LOAD_TEAM_VIEW_SIZE;
				newTop += LOAD_TEAM_VIEW_SIZE;
			}
			else
			{
				newIndex = numEntries - 1;
				if (newTop + LOAD_TEAM_VIEW_SIZE < numEntries &&
						numEntries > LOAD_TEAM_VIEW_SIZE)
					newTop = numEntries - LOAD_TEAM_VIEW_SIZE;
			}
		}

		if (newIndex != pMS->load.cur)
		{
			// The cursor has been moved.
			if (newTop == pMS->load.top)
			{
				// The view itself hasn't changed.
				SelectFileString (pMS, false);
			}
			else
			{
				// The view is changed.
				pMS->load.top = newTop;
				DrawFileStrings (pMS);
			}
			pMS->load.cur = newIndex;
		}
	}

	flashSelectedTeam (pMS);

	SleepThreadUntil (TimeIn + ONE_SECOND / 30);

	return TRUE;
}

static void
SelectTeamByFileName (MELEE_STATE *pMS, const char *fileName)
{
	COUNT index = GetFleetIndexByFileName (pMS, fileName);
	if (index == (COUNT) -1)
		return;

	RefocusView (pMS, index);
}

void
LoadTeamList (MELEE_STATE *pMS)
{
	COUNT i;

	DestroyDirEntryTable (ReleaseDirEntryTable (pMS->load.dirEntries));
	pMS->load.dirEntries = CaptureDirEntryTable (
			LoadDirEntryTable (meleeDir, "", ".mle", match_MATCH_SUFFIX));
	
	if (pMS->load.entryIndices != NULL)
		HFree (pMS->load.entryIndices);
	pMS->load.numIndices = GetDirEntryTableCount (pMS->load.dirEntries);
	pMS->load.entryIndices = HMalloc (pMS->load.numIndices *
			sizeof pMS->load.entryIndices[0]);
	for (i = 0; i < pMS->load.numIndices; i++)
		pMS->load.entryIndices[i] = i;
}

BOOLEAN
DoSaveTeam (MELEE_STATE *pMS)
{
	STAMP MsgStamp;
	char file[NAME_MAX];
	uio_Stream *stream;
	CONTEXT OldContext;
	bool saveOk = false;

	snprintf (file, sizeof file, "%s.mle",
			MeleeSetup_getTeamName (pMS->meleeSetup, pMS->side));

	OldContext = SetContext (ScreenContext);
	ConfirmSaveLoad (&MsgStamp);
			// Show the "Saving . . ." message.

	stream = uio_fopen (meleeDir, file, "wb");
	if (stream != NULL)
	{
		saveOk = (MeleeTeam_serialize (&pMS->meleeSetup->teams[pMS->side],
				stream) == 0);
		uio_fclose (stream);

		if (!saveOk)
			uio_unlink (meleeDir, file);
	}

	pMS->load.top = 0;
	pMS->load.cur = 0;

	// Undo the screen damage done by the "Saving . . ." message.
	DrawStamp (&MsgStamp);
	DestroyDrawable (ReleaseDrawable (MsgStamp.frame));
	SetContext (OldContext);

	if (!saveOk)
		SaveProblem ();

	// Update the team list; a previously existing team may have been
	// deleted when save failed.
	LoadTeamList (pMS);
	SelectTeamByFileName (pMS, file);
	
	return (stream != 0);
}

static void
InitPreBuilt (MELEE_STATE *pMS)
{
	MeleeTeam **list;

#define PREBUILT_COUNT 15
	pMS->load.preBuiltList =
			HMalloc (PREBUILT_COUNT * sizeof (MeleeTeam *));
	pMS->load.preBuiltCount = PREBUILT_COUNT;
#undef PREBUILT_COUNT

	{
		size_t fleetI;

		for (fleetI = 0; fleetI < pMS->load.preBuiltCount; fleetI++)
			pMS->load.preBuiltList[fleetI] = MeleeTeam_new ();
	}

	list = pMS->load.preBuiltList;

	{
		/* "Balanced Team 1" */
		FleetShipIndex i = 0;
		MeleeTeam_setName (*list, GAME_STRING (MELEE_STRING_BASE + 4));
		MeleeTeam_setShip (*list, i++, MELEE_ANDROSYNTH);
		MeleeTeam_setShip (*list, i++, MELEE_CHMMR);
		MeleeTeam_setShip (*list, i++, MELEE_DRUUGE);
		MeleeTeam_setShip (*list, i++, MELEE_URQUAN);
		MeleeTeam_setShip (*list, i++, MELEE_MELNORME);
		MeleeTeam_setShip (*list, i++, MELEE_ORZ);
		MeleeTeam_setShip (*list, i++, MELEE_SPATHI);
		MeleeTeam_setShip (*list, i++, MELEE_SYREEN);
		MeleeTeam_setShip (*list, i++, MELEE_UTWIG);
		list++;
	}

	{
		/* "Balanced Team 2" */
		FleetShipIndex i = 0;
		MeleeTeam_setName (*list, GAME_STRING (MELEE_STRING_BASE + 5));
		MeleeTeam_setShip (*list, i++, MELEE_ARILOU);
		MeleeTeam_setShip (*list, i++, MELEE_CHENJESU);
		MeleeTeam_setShip (*list, i++, MELEE_EARTHLING);
		MeleeTeam_setShip (*list, i++, MELEE_KOHR_AH);
		MeleeTeam_setShip (*list, i++, MELEE_MYCON);
		MeleeTeam_setShip (*list, i++, MELEE_YEHAT);
		MeleeTeam_setShip (*list, i++, MELEE_PKUNK);
		MeleeTeam_setShip (*list, i++, MELEE_SUPOX);
		MeleeTeam_setShip (*list, i++, MELEE_THRADDASH);
		MeleeTeam_setShip (*list, i++, MELEE_ZOQFOTPIK);
		MeleeTeam_setShip (*list, i++, MELEE_SHOFIXTI);
		list++;
	}

	{
		/* "200 points" */
		FleetShipIndex i = 0;
		MeleeTeam_setName (*list, GAME_STRING (MELEE_STRING_BASE + 6));
		MeleeTeam_setShip (*list, i++, MELEE_ANDROSYNTH);
		MeleeTeam_setShip (*list, i++, MELEE_CHMMR);
		MeleeTeam_setShip (*list, i++, MELEE_DRUUGE);
		MeleeTeam_setShip (*list, i++, MELEE_MELNORME);
		MeleeTeam_setShip (*list, i++, MELEE_EARTHLING);
		MeleeTeam_setShip (*list, i++, MELEE_KOHR_AH);
		MeleeTeam_setShip (*list, i++, MELEE_SUPOX);
		MeleeTeam_setShip (*list, i++, MELEE_ORZ);
		MeleeTeam_setShip (*list, i++, MELEE_SPATHI);
		MeleeTeam_setShip (*list, i++, MELEE_ILWRATH);
		MeleeTeam_setShip (*list, i++, MELEE_VUX);
		list++;
	}

	{
		/* "Behemoth Zenith" */
		FleetShipIndex i = 0;
		MeleeTeam_setName (*list, GAME_STRING (MELEE_STRING_BASE + 7));
		MeleeTeam_setShip (*list, i++, MELEE_CHENJESU);
		MeleeTeam_setShip (*list, i++, MELEE_CHENJESU);
		MeleeTeam_setShip (*list, i++, MELEE_CHMMR);
		MeleeTeam_setShip (*list, i++, MELEE_CHMMR);
		MeleeTeam_setShip (*list, i++, MELEE_KOHR_AH);
		MeleeTeam_setShip (*list, i++, MELEE_KOHR_AH);
		MeleeTeam_setShip (*list, i++, MELEE_URQUAN);
		MeleeTeam_setShip (*list, i++, MELEE_URQUAN);
		MeleeTeam_setShip (*list, i++, MELEE_UTWIG);
		MeleeTeam_setShip (*list, i++, MELEE_UTWIG);
		list++;
	}

	{
		/* "The Peeled Eyes" */
		FleetShipIndex i = 0;
		MeleeTeam_setName (*list, GAME_STRING (MELEE_STRING_BASE + 8));
		MeleeTeam_setShip (*list, i++, MELEE_URQUAN);
		MeleeTeam_setShip (*list, i++, MELEE_CHENJESU);
		MeleeTeam_setShip (*list, i++, MELEE_MYCON);
		MeleeTeam_setShip (*list, i++, MELEE_SYREEN);
		MeleeTeam_setShip (*list, i++, MELEE_ZOQFOTPIK);
		MeleeTeam_setShip (*list, i++, MELEE_SHOFIXTI);
		MeleeTeam_setShip (*list, i++, MELEE_EARTHLING);
		MeleeTeam_setShip (*list, i++, MELEE_KOHR_AH);
		MeleeTeam_setShip (*list, i++, MELEE_MELNORME);
		MeleeTeam_setShip (*list, i++, MELEE_DRUUGE);
		MeleeTeam_setShip (*list, i++, MELEE_PKUNK);
		MeleeTeam_setShip (*list, i++, MELEE_ORZ);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName (*list, "Ford's Fighters");
		MeleeTeam_setShip (*list, i++, MELEE_CHMMR);
		MeleeTeam_setShip (*list, i++, MELEE_ZOQFOTPIK);
		MeleeTeam_setShip (*list, i++, MELEE_MELNORME);
		MeleeTeam_setShip (*list, i++, MELEE_SUPOX);
		MeleeTeam_setShip (*list, i++, MELEE_UTWIG);
		MeleeTeam_setShip (*list, i++, MELEE_UMGAH);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName (*list, "Leyland's Lashers");
		MeleeTeam_setShip (*list, i++, MELEE_ANDROSYNTH);
		MeleeTeam_setShip (*list, i++, MELEE_EARTHLING);
		MeleeTeam_setShip (*list, i++, MELEE_MYCON);
		MeleeTeam_setShip (*list, i++, MELEE_ORZ);
		MeleeTeam_setShip (*list, i++, MELEE_URQUAN);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName (*list, "The Gregorizers 200");
		MeleeTeam_setShip (*list, i++, MELEE_ANDROSYNTH);
		MeleeTeam_setShip (*list, i++, MELEE_CHMMR);
		MeleeTeam_setShip (*list, i++, MELEE_DRUUGE);
		MeleeTeam_setShip (*list, i++, MELEE_MELNORME);
		MeleeTeam_setShip (*list, i++, MELEE_EARTHLING);
		MeleeTeam_setShip (*list, i++, MELEE_KOHR_AH);
		MeleeTeam_setShip (*list, i++, MELEE_SUPOX);
		MeleeTeam_setShip (*list, i++, MELEE_ORZ);
		MeleeTeam_setShip (*list, i++, MELEE_PKUNK);
		MeleeTeam_setShip (*list, i++, MELEE_SPATHI);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName (*list, "300 point Armada!");
		MeleeTeam_setShip (*list, i++, MELEE_ANDROSYNTH);
		MeleeTeam_setShip (*list, i++, MELEE_CHMMR);
		MeleeTeam_setShip (*list, i++, MELEE_CHENJESU);
		MeleeTeam_setShip (*list, i++, MELEE_DRUUGE);
		MeleeTeam_setShip (*list, i++, MELEE_EARTHLING);
		MeleeTeam_setShip (*list, i++, MELEE_KOHR_AH);
		MeleeTeam_setShip (*list, i++, MELEE_MELNORME);
		MeleeTeam_setShip (*list, i++, MELEE_MYCON);
		MeleeTeam_setShip (*list, i++, MELEE_ORZ);
		MeleeTeam_setShip (*list, i++, MELEE_PKUNK);
		MeleeTeam_setShip (*list, i++, MELEE_SPATHI);
		MeleeTeam_setShip (*list, i++, MELEE_SUPOX);
		MeleeTeam_setShip (*list, i++, MELEE_URQUAN);
		MeleeTeam_setShip (*list, i++, MELEE_YEHAT);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName (*list, "Little Dudes with Attitudes");
		MeleeTeam_setShip (*list, i++, MELEE_UMGAH);
		MeleeTeam_setShip (*list, i++, MELEE_THRADDASH);
		MeleeTeam_setShip (*list, i++, MELEE_SHOFIXTI);
		MeleeTeam_setShip (*list, i++, MELEE_EARTHLING);
		MeleeTeam_setShip (*list, i++, MELEE_VUX);
		MeleeTeam_setShip (*list, i++, MELEE_ZOQFOTPIK);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName (*list, "New Alliance Ships");
		MeleeTeam_setShip (*list, i++, MELEE_ARILOU);
		MeleeTeam_setShip (*list, i++, MELEE_CHMMR);
		MeleeTeam_setShip (*list, i++, MELEE_EARTHLING);
		MeleeTeam_setShip (*list, i++, MELEE_ORZ);
		MeleeTeam_setShip (*list, i++, MELEE_PKUNK);
		MeleeTeam_setShip (*list, i++, MELEE_SHOFIXTI);
		MeleeTeam_setShip (*list, i++, MELEE_SUPOX);
		MeleeTeam_setShip (*list, i++, MELEE_SYREEN);
		MeleeTeam_setShip (*list, i++, MELEE_UTWIG);
		MeleeTeam_setShip (*list, i++, MELEE_ZOQFOTPIK);
		MeleeTeam_setShip (*list, i++, MELEE_YEHAT);
		MeleeTeam_setShip (*list, i++, MELEE_DRUUGE);
		MeleeTeam_setShip (*list, i++, MELEE_THRADDASH);
		MeleeTeam_setShip (*list, i++, MELEE_SPATHI);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName (*list, "Old Alliance Ships");
		MeleeTeam_setShip (*list, i++, MELEE_ARILOU);
		MeleeTeam_setShip (*list, i++, MELEE_CHENJESU);
		MeleeTeam_setShip (*list, i++, MELEE_EARTHLING);
		MeleeTeam_setShip (*list, i++, MELEE_MMRNMHRM);
		MeleeTeam_setShip (*list, i++, MELEE_SHOFIXTI);
		MeleeTeam_setShip (*list, i++, MELEE_SYREEN);
		MeleeTeam_setShip (*list, i++, MELEE_YEHAT);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName (*list, "Old Hierarchy Ships");
		MeleeTeam_setShip (*list, i++, MELEE_ANDROSYNTH);
		MeleeTeam_setShip (*list, i++, MELEE_ILWRATH);
		MeleeTeam_setShip (*list, i++, MELEE_MYCON);
		MeleeTeam_setShip (*list, i++, MELEE_SPATHI);
		MeleeTeam_setShip (*list, i++, MELEE_UMGAH);
		MeleeTeam_setShip (*list, i++, MELEE_URQUAN);
		MeleeTeam_setShip (*list, i++, MELEE_VUX);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName (*list, "Star Control 1");
		MeleeTeam_setShip (*list, i++, MELEE_ANDROSYNTH);
		MeleeTeam_setShip (*list, i++, MELEE_ARILOU);
		MeleeTeam_setShip (*list, i++, MELEE_CHENJESU);
		MeleeTeam_setShip (*list, i++, MELEE_EARTHLING);
		MeleeTeam_setShip (*list, i++, MELEE_ILWRATH);
		MeleeTeam_setShip (*list, i++, MELEE_MMRNMHRM);
		MeleeTeam_setShip (*list, i++, MELEE_MYCON);
		MeleeTeam_setShip (*list, i++, MELEE_SHOFIXTI);
		MeleeTeam_setShip (*list, i++, MELEE_SPATHI);
		MeleeTeam_setShip (*list, i++, MELEE_SYREEN);
		MeleeTeam_setShip (*list, i++, MELEE_UMGAH);
		MeleeTeam_setShip (*list, i++, MELEE_URQUAN);
		MeleeTeam_setShip (*list, i++, MELEE_VUX);
		MeleeTeam_setShip (*list, i++, MELEE_YEHAT);
		list++;
	}

	{
		FleetShipIndex i = 0;
		MeleeTeam_setName (*list, "Star Control 2");
		MeleeTeam_setShip (*list, i++, MELEE_CHMMR);
		MeleeTeam_setShip (*list, i++, MELEE_DRUUGE);
		MeleeTeam_setShip (*list, i++, MELEE_KOHR_AH);
		MeleeTeam_setShip (*list, i++, MELEE_MELNORME);
		MeleeTeam_setShip (*list, i++, MELEE_ORZ);
		MeleeTeam_setShip (*list, i++, MELEE_PKUNK);
		MeleeTeam_setShip (*list, i++, MELEE_SLYLANDRO);
		MeleeTeam_setShip (*list, i++, MELEE_SUPOX);
		MeleeTeam_setShip (*list, i++, MELEE_THRADDASH);
		MeleeTeam_setShip (*list, i++, MELEE_UTWIG);
		MeleeTeam_setShip (*list, i++, MELEE_ZOQFOTPIK);
		MeleeTeam_setShip (*list, i++, MELEE_ZOQFOTPIK);
		MeleeTeam_setShip (*list, i++, MELEE_ZOQFOTPIK);
		MeleeTeam_setShip (*list, i++, MELEE_ZOQFOTPIK);
		list++;
	}

	assert (list == pMS->load.preBuiltList + pMS->load.preBuiltCount);
}

static void
UninitPreBuilt (MELEE_STATE *pMS)
{
	size_t fleetI;
	for (fleetI = 0; fleetI < pMS->load.preBuiltCount; fleetI++)
		MeleeTeam_delete (pMS->load.preBuiltList[fleetI]);
	HFree (pMS->load.preBuiltList);
	pMS->load.preBuiltCount = 0;
}

static void
InitLoadView (MELEE_STATE *pMS)
{
	size_t viewI;
	MeleeTeam **view = pMS->load.view;

	for (viewI = 0; viewI < LOAD_TEAM_VIEW_SIZE; viewI++)
		view[viewI] = MeleeTeam_new ();
}

static void
UninitLoadView (MELEE_STATE *pMS)
{
	size_t viewI;
	MeleeTeam **view = pMS->load.view;

	for (viewI = 0; viewI < LOAD_TEAM_VIEW_SIZE; viewI++)
		MeleeTeam_delete(view[viewI]);
}

void
InitMeleeLoadState (MELEE_STATE *pMS)
{
	pMS->load.entryIndices = NULL;
	InitPreBuilt (pMS);
	InitLoadView (pMS);
}

void
UninitMeleeLoadState (MELEE_STATE *pMS)
{
	UninitLoadView (pMS);
	UninitPreBuilt (pMS);
	if (pMS->load.entryIndices != NULL)
		HFree (pMS->load.entryIndices);
}


