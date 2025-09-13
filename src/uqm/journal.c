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

#include "colors.h"
#include "comm.h"
#include "controls.h"
#include "globdata.h"
#include "journal.h"
#include "menustat.h"
#include "nameref.h"
#include "options.h"
#include "races.h"
#include "setup.h"
#include "sis.h"
#include "sounds.h"
#include "starmap.h"
#include "units.h"
#include "uqmdebug.h"
#include "libs/graphics/gfx_common.h"
#include "libs/memlib.h"
#include "libs/strlib.h"
#include <stdlib.h>
#include <stdarg.h>


typedef enum {
	OBJECTIVES_JOURNAL,
	ALIENS_JOURNAL,
	ARTIFACTS_JOURNAL,

	NUM_JOURNALS
} JOURNAL_ID;

typedef enum {
	OPEN_SECTION,
	COMPLETED_SECTION,
	SPATHI_SECTION,

	NUM_SECTIONS
} SECTION_ID;

typedef struct journal_section_struct {
	struct journal_entry_struct *head;
	struct journal_entry_struct *tail;
} JOURNAL_SECTION;

typedef struct journal_entry_struct {
	char *string;
	BOOLEAN shared;
	struct journal_entry_struct *next;
} JOURNAL_ENTRY;

static STRING JournalStrings;
static JOURNAL_ID which_journal;
static int scroll_journal;
static JOURNAL_SECTION journal_section[NUM_SECTIONS];
#define JOURNAL_BUF_SIZE 1024
static char journal_buf[JOURNAL_BUF_SIZE];
static BOOLEAN transition_pending;

#define JOURNAL_STRING(i) (GetStringAddress (SetAbsStringTableIndex (JournalStrings, (i))))

static BOOLEAN
StoreJournalEntry (SECTION_ID sid, JOURNAL_ENTRY *entry)
{
	JOURNAL_SECTION *section = &journal_section[sid];
	if (section->tail)
		section->tail->next = entry;
	else
		section->head = entry;
	section->tail = entry;
	return TRUE;
}


static BOOLEAN
AppendJournalEntry (SECTION_ID sid, char *string)
{
	JOURNAL_ENTRY *entry = HCalloc (sizeof (JOURNAL_ENTRY));
	entry->string = string;
	entry->shared = TRUE;
	return StoreJournalEntry (sid, entry);
}


static BOOLEAN
BuildJournalEntry (SECTION_ID sid, const char *format, ...)
{
	size_t len;
	JOURNAL_ENTRY *entry;

	va_list args;
	va_start (args, format);
	len = vsnprintf (journal_buf, JOURNAL_BUF_SIZE, format, args);
	va_end (args);

	entry = HCalloc (sizeof (JOURNAL_ENTRY));
	entry->string = HMalloc (len + 1);
	strncpy (entry->string, journal_buf, len);
	entry->string[len] = '\0';
	entry->shared = FALSE;
	return StoreJournalEntry (sid, entry);
}


static void
FreeJournals (void)
{
	JOURNAL_ENTRY *curr, *next;
	for (SECTION_ID sid = 0;  sid < NUM_SECTIONS;  sid++)
	{
		JOURNAL_SECTION *section = &journal_section[sid];
		for (curr = section->head;  curr != NULL;  curr = next)
		{
			next = curr->next;
			if (curr->shared == FALSE)
				HFree (curr->string);
			HFree (curr);
		}
		section->head = section->tail = NULL;
	}

	if (JournalStrings != NULL)
		DestroyStringTable (ReleaseStringTable (JournalStrings));
	JournalStrings = NULL;
}


static BOOLEAN
AddJournalObjective (int steps, ...)
{
	int i, s, test, jstring;
	char *str = NULL;

	va_list args;
	va_start (args, steps);
	for (i = 0;  i < steps;  i++)
	{
		test = va_arg (args, int);
		jstring = va_arg (args, int);
		if (test)
		{
			s = i + 1;
			str = (jstring == NO_JOURNAL_ENTRY)
					? NULL : JOURNAL_STRING(jstring);
		}
	}
	va_end (args);

	if (!str || !*str)
		return FALSE;
	return AppendJournalEntry (
			s == steps ? COMPLETED_SECTION : OPEN_SECTION, str);
}


static void
WriteJournals (void)
{
#define TF(val)       ((val) ? 1 : 0)
#define GS(flag)      TF(GET_GAME_STATE(flag) > 0)
#define GSLT(flag,n)  TF(GET_GAME_STATE(flag) < (n))
#define GSGE(flag,n)  TF(GET_GAME_STATE(flag) >= (n))

	JournalStrings = CaptureStringTable (LoadStringTable (JOURNAL_STRTAB));
	if (JournalStrings == 0)
		return;

	// starbase missions

	int have_radioactives = TF(
			GET_GAME_STATE(STARBASE_VISITED) &&
			GLOBAL_SIS (ElementAmounts[RADIOACTIVE]) > 0);
	int need_radioactives = TF(
			GET_GAME_STATE(STARBASE_VISITED) &&
			GLOBAL_SIS (ElementAmounts[RADIOACTIVE]) <= 0 &&
			!GET_GAME_STATE(RADIOACTIVES_PROVIDED));
	int need_lander = TF(
			need_radioactives &&
			GLOBAL_SIS (NumLanders) < 1);
	int need_lander_again = TF(
			need_lander &&
			GS(LANDERS_LOST));
	int need_fuel = TF(
			need_radioactives &&
			GLOBAL_SIS (FuelOnBoard) < 2 * FUEL_TANK_SCALE);
	int need_fuel_again = TF(
			need_fuel &&
			GS(GIVEN_FUEL_BEFORE));
	int reported_moonbase = TF(
			GET_GAME_STATE(MOONBASE_DESTROYED) &&
			!GET_GAME_STATE(MOONBASE_ON_SHIP));

	AddJournalObjective (2,
			1,                            VISIT_EARTH,
			GS(PROBE_MESSAGE_DELIVERED),  VISIT_EARTH);
	AddJournalObjective (4,
			GS(PROBE_MESSAGE_DELIVERED),  CONTACT_EARTH,
			GS(STARBASE_VISITED),         GET_RADIOACTIVES,
			have_radioactives,            GIVE_RADIOACTIVES,
			GS(RADIOACTIVES_PROVIDED),    GIVE_RADIOACTIVES);
	AddJournalObjective (2,
			need_lander,                  NEED_LANDER,
			GS(LANDERS_LOST),             NEED_LANDER);
	AddJournalObjective (2,
			need_lander_again,            NEED_LANDER_AGAIN,
			TF(!need_lander),             NO_JOURNAL_ENTRY);
	AddJournalObjective (2,
			need_fuel,                    NEED_FUEL,
			GS(GIVEN_FUEL_BEFORE),        NEED_FUEL);
	AddJournalObjective (2,
			need_fuel_again,              NEED_FUEL_AGAIN,
			TF(!need_fuel),               NO_JOURNAL_ENTRY);
	AddJournalObjective (3,
			GS(WILL_DESTROY_BASE),        DESTROY_MOONBASE,
			GS(MOONBASE_ON_SHIP),         REPORT_MOONBASE,
			reported_moonbase,            DESTROY_MOONBASE);
	AddJournalObjective (2,
			GS(RADIOACTIVES_PROVIDED),    RECRUIT_EARTH,
			GS(STARBASE_AVAILABLE),       RECRUIT_EARTH);
}


static void
DrawJournal (void)
{	// TODO: move strings to gamestrings or some other string resource file
	SIZE width, leading;
	TEXT t;
	SECTION_ID sid, sid_end;
	JOURNAL_SECTION *section;
	JOURNAL_ENTRY *entry;

	SetContext (SpaceContext);

	if (transition_pending)
		SetTransitionSource (NULL);

	BatchGraphics ();

	ClearDrawable ();
	SetContextForeGroundColor (LTGRAY_COLOR);
	SetContextBackGroundColor (BLACK_COLOR);
	SetContextFont (PlyrFont);
	GetContextFontLeading (&leading);

	width = SIS_SCREEN_WIDTH - RES_SCALE (10 + 2);
	t.align = ALIGN_LEFT;
	t.baseline.y = leading * (1 - scroll_journal);

	switch (which_journal)
	{
		case OBJECTIVES_JOURNAL:
			DrawSISMessage (JOURNAL_STRING (OBJECTIVES_JOURNAL_HEADER));
			sid = OPEN_SECTION;
			sid_end = COMPLETED_SECTION;
			break;
		case ALIENS_JOURNAL:
			DrawSISMessage (JOURNAL_STRING (ALIENS_JOURNAL_HEADER));
			sid = 1;
			sid_end = 0;
			break;
		case ARTIFACTS_JOURNAL:
			DrawSISMessage (JOURNAL_STRING (ARTIFACTS_JOURNAL_HEADER));
			sid = 1;
			sid_end = 0;
			break;
		default:
			snprintf (journal_buf, JOURNAL_BUF_SIZE, "journal #%d",
					which_journal);
			DrawSISMessage (journal_buf);
			sid = 1;
			sid_end = 0;
			break;
	}

	for (; sid <= sid_end; sid++)
	{
		section = &journal_section[sid];
		const char *nextchar = NULL;

		SetContextForeGroundColor (
				(sid == COMPLETED_SECTION) ? DKGRAY_COLOR : LTGRAY_COLOR);

		for (entry = section->head;  entry != NULL;  entry = entry->next)
		{
			t.pStr = STR_BULLET;
			t.CharCount = (COUNT)~0;
			t.baseline.x = RES_SCALE (3);
			font_DrawText (&t);

			t.pStr = entry->string;
			t.CharCount = (COUNT)~0;
			t.baseline.x = RES_SCALE (10);
			while (!getLineWithinWidth (&t, &nextchar, width, (COUNT)~0))
			{
				font_DrawText (&t);
				t.baseline.y += leading;
				t.CharCount = (COUNT)~0;
				t.pStr = nextchar;
			}
			font_DrawText(&t);

			t.baseline.y += leading;
		}
	}

	if (transition_pending)
	{
		RECT r;
		GetContextClipRect (&r);
		ScreenTransition (optScrTrans, &r);
		transition_pending = FALSE;
	}

	UnbatchGraphics ();
}

static BOOLEAN
DoChangeJournal (MENU_STATE *pMS)
{
	if (!pMS->Initialized)
	{
		pMS->Initialized = TRUE;
		pMS->InputFunc = DoChangeJournal;

		return TRUE;
	}
	else if (PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		return FALSE;
	}
	else
	{
		SBYTE dj = 0;
		if (PulsedInputState.menu[KEY_MENU_LEFT])    dj = -1;
		if (PulsedInputState.menu[KEY_MENU_RIGHT])   dj = 1;
		if (PulsedInputState.menu[KEY_MENU_UP])      --scroll_journal;
		if (PulsedInputState.menu[KEY_MENU_DOWN])    ++scroll_journal;

		if (scroll_journal < 0 || dj != 0)
			scroll_journal = 0;
		which_journal = (which_journal + dj + NUM_JOURNALS) % NUM_JOURNALS;

		DrawJournal ();
	}

	return !(GLOBAL (CurrentActivity) & CHECK_ABORT);
}

BOOLEAN
Journal (void)
{
	MENU_STATE MenuState;
	POINT universe;

	memset (&MenuState, 0, sizeof (MenuState));

	WriteJournals ();
	which_journal = OBJECTIVES_JOURNAL;

	if (!inHQSpace ())
		universe = CurStarDescPtr->star_pt;
	else
	{
		universe.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
		universe.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
	}

	//if (optWhichMenu == OPT_PC)
	//{
	//	/*if (actuallyInOrbit)
	//		DrawMenuStateStrings (PM_ALT_SCAN,
	//				PM_ALT_JOURNAL - PM_ALT_SCAN);
	//	else*/
	//		DrawMenuStateStrings (PM_ALT_STARMAP,
	//				PM_ALT_JOURNAL - PM_ALT_STARMAP);
	//}

	MenuState.InputFunc = DoChangeJournal;
	MenuState.Initialized = FALSE;

	transition_pending = TRUE;
	DrawJournal ();
	transition_pending = FALSE;

	DoInput (&MenuState, FALSE);

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	SetDefaultMenuRepeatDelay ();

	FreeJournals ();

	DrawHyperCoords (universe);
	DrawSISMessage (NULL);
	DrawStatusMessage (NULL);
	
	return TRUE;
}
