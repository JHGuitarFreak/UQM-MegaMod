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
#include "util.h"
#include <stdlib.h>
#include <stdarg.h>


typedef enum {
	OBJECTIVES_JOURNAL,
	ALIENS_JOURNAL,
	ARTIFACTS_JOURNAL,

	NUM_JOURNALS
} JOURNAL_ID;

typedef enum {
	OPEN_OBJECTIVES,
	CLOSED_OBJECTIVES,
	OPEN_ALIENS,
	CLOSED_ALIENS,
	OPEN_ARTIFACTS,
	CLOSED_ARTIFACTS,
	OPEN_SPATHI,
	CLOSED_SPATHI,

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
AddJournal (JOURNAL_ID Objective, int steps, ...)
{
	int i, test, jstring;
	int s = 0;
	char *str = NULL;
	SECTION_ID Open = OPEN_SPATHI;
	SECTION_ID Closed = CLOSED_SPATHI;

	if (Objective > ARTIFACTS_JOURNAL || Objective < OBJECTIVES_JOURNAL)
		return FALSE;

	if (Objective == OBJECTIVES_JOURNAL)
	{
		Open = OPEN_OBJECTIVES;
		Closed = CLOSED_OBJECTIVES;
	}
	else if (Objective == ALIENS_JOURNAL)
	{
		Open = OPEN_ALIENS;
		Closed = CLOSED_ALIENS;
	}
	else
	{
		Open = OPEN_ARTIFACTS;
		Closed = CLOSED_ARTIFACTS;
	}

	if (Open > OPEN_ARTIFACTS || Open < OPEN_OBJECTIVES)
		return FALSE;

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
	return AppendJournalEntry (s == steps ? Closed : Open, str);
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

	AddJournal (ALIENS_JOURNAL, 2,
			1,                            VISIT_EARTH,
			GS(PROBE_MESSAGE_DELIVERED),  VISIT_EARTH);
	AddJournal (OBJECTIVES_JOURNAL, 4,
			GS(PROBE_MESSAGE_DELIVERED),  CONTACT_EARTH,
			GS(STARBASE_VISITED),         GET_RADIOACTIVES,
			have_radioactives,            GIVE_RADIOACTIVES,
			GS(RADIOACTIVES_PROVIDED),    GIVE_RADIOACTIVES);
	AddJournal (OBJECTIVES_JOURNAL, 2,
			need_lander,                  NEED_LANDER,
			GS(LANDERS_LOST),             NEED_LANDER);
	AddJournal (OBJECTIVES_JOURNAL, 2,
			need_lander_again,            NEED_LANDER_AGAIN,
			TF(!need_lander),             NO_JOURNAL_ENTRY);
	AddJournal (OBJECTIVES_JOURNAL, 2,
			need_fuel,                    NEED_FUEL,
			GS(GIVEN_FUEL_BEFORE),        NEED_FUEL);
	AddJournal (OBJECTIVES_JOURNAL, 2,
			need_fuel_again,              NEED_FUEL_AGAIN,
			TF(!need_fuel),               NO_JOURNAL_ENTRY);
	AddJournal (OBJECTIVES_JOURNAL, 3,
			GS(WILL_DESTROY_BASE),        DESTROY_MOONBASE,
			GS(MOONBASE_ON_SHIP),         REPORT_MOONBASE,
			reported_moonbase,            DESTROY_MOONBASE);
	AddJournal (OBJECTIVES_JOURNAL, 2,
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

	if (transition_pending)
		SetTransitionSource (NULL);

	BatchGraphics ();
	ClearDrawable ();
	RepairSISBorder ();
	SetContextFont (PlyrFont);
	GetContextFontLeading (&leading);

	width = SIS_SCREEN_WIDTH - RES_SCALE (10 + 2);
	t.align = ALIGN_LEFT;
	t.baseline.y = leading * (1 - scroll_journal);

	switch (which_journal)
	{
		case OBJECTIVES_JOURNAL:
			DrawSISMessage (JOURNAL_STRING (OBJECTIVES_JOURNAL_HEADER));
			sid = OPEN_OBJECTIVES;
			sid_end = CLOSED_OBJECTIVES;
			break;
		case ALIENS_JOURNAL:
			DrawSISMessage (JOURNAL_STRING (ALIENS_JOURNAL_HEADER));
			sid = OPEN_ALIENS;
			sid_end = CLOSED_ALIENS;
			break;
		case ARTIFACTS_JOURNAL:
			DrawSISMessage (JOURNAL_STRING (ARTIFACTS_JOURNAL_HEADER));
			sid = OPEN_ARTIFACTS;
			sid_end = CLOSED_ARTIFACTS;
			break;
		default:
			snprintf (journal_buf, JOURNAL_BUF_SIZE, "journal #%d",
					which_journal);
			DrawSISMessage (journal_buf);
			sid = OPEN_OBJECTIVES;
			sid_end = CLOSED_OBJECTIVES;
			break;
	}

	for (; sid <= sid_end; sid++)
	{
		section = &journal_section[sid];
		const char *nextchar = NULL;

		SetContextForeGroundColor (
				(sid == CLOSED_OBJECTIVES
				|| sid == CLOSED_ALIENS
				|| sid == CLOSED_ARTIFACTS)
				? VDKGRAY_COLOR : LTGRAY_COLOR);

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
	else if (PulsedInputState.menu[KEY_JOURNAL]
			|| PulsedInputState.menu[KEY_MENU_CANCEL])
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
	MENU_STATE MenuState = { 0 };
	POINT universe;
	Color OldBGColor, OldFGColor;
	CONTEXT OldContext;
	RECT r, old_r;
	STAMP s;

	OldContext = SetContext (SpaceContext);
	OldBGColor = SetContextBackGroundColor (COMM_HISTORY_BACKGROUND_COLOR);
	OldFGColor = SetContextForeGroundColor (COMM_HISTORY_TEXT_COLOR);

	GetContextClipRect (&old_r);

	// For the Orbital and Shipyard transitions
	r = old_r;
	r.corner.x -= RES_SCALE (1);
	r.extent.width += RES_SCALE (2);
	r.extent.height += RES_SCALE (1);
	SetContextClipRect (&r);
	s = SaveContextFrame (NULL);

	SetContextClipRect (&old_r);

	if (!inHQSpace ())
		universe = CurStarDescPtr->star_pt;
	else
	{
		universe.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
		universe.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
	}

	WriteJournals ();
	which_journal = OBJECTIVES_JOURNAL;

	MenuState.InputFunc = DoChangeJournal;
	MenuState.Initialized = FALSE;

	transition_pending = TRUE;
	DrawJournal ();
	transition_pending = FALSE;

	DoInput (&MenuState, FALSE);

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	SetDefaultMenuRepeatDelay ();

	FreeJournals ();

	SetContextClipRect (&r);
	SetTransitionSource (&r);
	BatchGraphics ();

		SetContextBackGroundColor (OldBGColor);
		SetContextForeGroundColor (OldFGColor);
		
		DrawStamp (&s);

		DrawHyperCoords (universe);
		DrawSISMessage (NULL);
		DrawStatusMessage (NULL);

		ScreenTransition (optScrTrans, &r);
	
	UnbatchGraphics ();

	DestroyDrawable (ReleaseDrawable (s.frame));
	FlushInput ();

	SetContextClipRect (&old_r);
	SetContext (OldContext);
	
	return TRUE;
}
