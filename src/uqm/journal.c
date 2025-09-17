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
#include "lua/luacomm.h"
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

	// Default values for error checking
	OPEN_SPATHI,
	CLOSED_SPATHI,

	NUM_SECTIONS
} SECTION_ID;

typedef struct journal_section_struct {
	struct journal_entry_struct *head;
	struct journal_entry_struct *tail;
} JOURNAL_SECTION;

typedef struct journal_entry_struct {
	UNICODE *string;
	BOOLEAN shared;
	struct journal_entry_struct *next;
} JOURNAL_ENTRY;

static STRING JournalStrings;
static JOURNAL_ID which_journal;
static int scroll_journal;
static JOURNAL_SECTION journal_section[NUM_SECTIONS];
#define JOURNAL_BUF_SIZE 1024
static UNICODE journal_buf[JOURNAL_BUF_SIZE];
static BOOLEAN transition_pending;
BOOLEAN FwiffoCanJoin = FALSE;

#define JOURNAL_STRING(i) \
		(GetStringAddress (SetAbsStringTableIndex (JournalStrings, (i))))

#define GS(flag)     (GET_GAME_STATE(flag) > 0)
#define GSET(flag,n) (GET_GAME_STATE(flag) == (n))
#define GSLT(flag,n) (GET_GAME_STATE(flag) < (n))
#define GSGE(flag,n) (GET_GAME_STATE(flag) >= (n))

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
AppendJournalEntry (SECTION_ID sid, UNICODE *string)
{
	JOURNAL_ENTRY *entry = HCalloc (sizeof (JOURNAL_ENTRY));
	entry->string = string;
	entry->shared = TRUE;
	return StoreJournalEntry (sid, entry);
}

static BOOLEAN
BuildJournalEntry (SECTION_ID sid, const UNICODE *format, ...)
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
	va_list args;
	int i, test, jstring;
	int s = 0;
	UNICODE *str = NULL;
	SECTION_ID Open = OPEN_SPATHI;
	SECTION_ID Closed = CLOSED_SPATHI;

	if (Objective > ARTIFACTS_JOURNAL || Objective < OBJECTIVES_JOURNAL)
		return FALSE;

	switch (Objective)
	{
		case OBJECTIVES_JOURNAL:
			Open = OPEN_OBJECTIVES;
			Closed = CLOSED_OBJECTIVES;
			break;
		case ALIENS_JOURNAL:
			Open = OPEN_ALIENS;
			Closed = CLOSED_ALIENS;
			break;
		case ARTIFACTS_JOURNAL:
			Open = OPEN_ARTIFACTS;
			Closed = CLOSED_ARTIFACTS;
			break;
		default:
			// Shouldn't happen
			return FALSE;
	}

	if (Open == OPEN_SPATHI || Closed == CLOSED_SPATHI)
		return FALSE;

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
ObjectivesJournal (void)
{	// starbase missions
	BOOLEAN FuelLow = GLOBAL_SIS (FuelOnBoard) < (2 * FUEL_TANK_SCALE);
	BOOLEAN HaveRadios = GLOBAL_SIS (ElementAmounts[RADIOACTIVE]) > 0;
	BOOLEAN FwiffoBullet = GET_GAME_STATE (STARBASE_BULLETS) & (1L << 9);

	int have_radioactives = GS (STARBASE_VISITED) && HaveRadios;
	int need_radioactives = GS (STARBASE_VISITED) && !HaveRadios &&
			!GS (RADIOACTIVES_PROVIDED);
	int need_lander = need_radioactives && GLOBAL_SIS (NumLanders) < 1;
	int need_lander_again = need_lander && GS (LANDERS_LOST);
	int need_fuel = need_radioactives && FuelLow;
	int need_fuel_again = need_fuel && GS (GIVEN_FUEL_BEFORE);
	int reported_moonbase = GS (MOONBASE_DESTROYED) && !GS (MOONBASE_ON_SHIP);
	int signal_uranus = FwiffoBullet && !GS (FOUND_PLUTO_SPATHI);
	int can_fwiffo_join = GSET (FOUND_PLUTO_SPATHI, 1) && FwiffoCanJoin;

	// starbase missions
	AddJournal (OBJECTIVES_JOURNAL, 2,
			1,                            VISIT_EARTH,
			GS (PROBE_MESSAGE_DELIVERED), VISIT_EARTH);
	AddJournal (OBJECTIVES_JOURNAL, 4,
			GS (PROBE_MESSAGE_DELIVERED), CONTACT_EARTH,
			GS (STARBASE_VISITED),        GET_RADIOACTIVES,
			have_radioactives,            GIVE_RADIOACTIVES,
			GS (RADIOACTIVES_PROVIDED),   GIVE_RADIOACTIVES);
	AddJournal (OBJECTIVES_JOURNAL, 2,
			need_lander,                  NEED_LANDER,
			GS (LANDERS_LOST),            NEED_LANDER);
	AddJournal (OBJECTIVES_JOURNAL, 2,
			need_lander_again,            NEED_LANDER_AGAIN,
			!need_lander,                 NO_JOURNAL_ENTRY);
	AddJournal (OBJECTIVES_JOURNAL, 2,
			need_fuel,                    NEED_FUEL,
			GS (GIVEN_FUEL_BEFORE),       NEED_FUEL);
	AddJournal (OBJECTIVES_JOURNAL, 2,
			need_fuel_again,              NEED_FUEL_AGAIN,
			!need_fuel,                   NO_JOURNAL_ENTRY);
	AddJournal (OBJECTIVES_JOURNAL, 3,
			GS (WILL_DESTROY_BASE),       DESTROY_MOONBASE,
			GS (MOONBASE_ON_SHIP),        REPORT_MOONBASE,
			reported_moonbase,            DESTROY_MOONBASE);
	AddJournal (OBJECTIVES_JOURNAL, 2,
			GS (RADIOACTIVES_PROVIDED),   RECRUIT_EARTH,
			GS (STARBASE_AVAILABLE),      RECRUIT_EARTH);

	AddJournal (OBJECTIVES_JOURNAL, 2,
			signal_uranus,                FIND_FWIFFO,
			GS (FOUND_PLUTO_SPATHI),      FIND_FWIFFO);

	AddJournal (OBJECTIVES_JOURNAL, 2,
			can_fwiffo_join,              RECRUIT_FWIFFO,
			GSGE (FOUND_PLUTO_SPATHI, 2), MET_FWIFFO);


	AddJournal (OBJECTIVES_JOURNAL, 4,
			(!GS (AWARE_OF_SAMATRA) || GSLT (CHMMR_BOMB_STATE, 1)) && GS (STARBASE_AVAILABLE), FIND_DEFEAT_URQUAN,
			GS (AWARE_OF_SAMATRA) && !GS (TALKING_PET_ON_SHIP), FIND_ACCESS_SAMATRA,
			GS (AWARE_OF_SAMATRA) || GSET (CHMMR_BOMB_STATE, 1),  FIND_DESTROY_SAMATRA,
			GS (AWARE_OF_SAMATRA) && GSGE (CHMMR_BOMB_STATE, 2), DESTROY_SAMATRA);
}

static void
AliensJournal (void)
{
	
}

static void
ArtifactsJournal (void)
{
	
}

static void
DrawJournal (void)
{
	SIZE width, leading;
	TEXT t;
	SECTION_ID sid, sid_end;
	JOURNAL_SECTION *section;
	JOURNAL_ENTRY *entry;
	Color OldBGColor, OldFGColor;
	FONT OldFont;

	if (transition_pending)
		SetTransitionSource (NULL);

	OldBGColor = SetContextBackGroundColor (COMM_HISTORY_BACKGROUND_COLOR);
	OldFGColor = SetContextForeGroundColor (COMM_HISTORY_TEXT_COLOR);
	OldFont = SetContextFont (TinyFont);

	GetContextFontLeading (&leading);

	width = SIS_SCREEN_WIDTH - RES_SCALE (10 + 2);
	t.align = ALIGN_LEFT;
	t.baseline.y = leading * (1 - scroll_journal);

	BatchGraphics ();

		ClearDrawable ();
		RepairSISBorder ();

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
			sid = OPEN_SPATHI;
			sid_end = CLOSED_SPATHI;
			break;
	}

	for (; sid <= sid_end; sid++)
	{
		const UNICODE *nextchar = NULL;
		BOOLEAN sid_open = sid == OPEN_OBJECTIVES
				|| sid == OPEN_ALIENS || sid == OPEN_ARTIFACTS;

		section = &journal_section[sid];

		if (section->head != NULL)
		{
			const UNICODE *section_header = sid_open ?
					JOURNAL_STRING (OPEN_OBJECTIVE)      // OPEN:
					: JOURNAL_STRING (CLOSED_OBJECTIVE); // CLOSED:

			SetContextForeGroundColor (COMM_HISTORY_TEXT_COLOR);

			// Add gap between last open objective and the CLOSED: header
			if (!sid_open)
				t.baseline.y += leading;

			// Draw section header
			t.pStr = section_header;
			t.CharCount = (COUNT)~0;
			t.baseline.x = RES_SCALE (2);
			font_DrawText (&t);
			t.baseline.y += leading + RES_SCALE (2);
		}

		SetContextForeGroundColor (sid_open ? LTGRAY_COLOR : DKGRAY_COLOR);

		for (entry = section->head;  entry != NULL;  entry = entry->next)
		{
			BOOLEAN comm_init = FALSE;
			RECT r;

			t.pStr = " - ";
			t.CharCount = (COUNT)~0;
			t.baseline.x = 0;
			font_DrawText (&t);

			r = font_GetTextRect (&t);

			if (luaUqm_commState == NULL)
			{
				luaUqm_comm_init (NULL, NULL_RESOURCE);
				comm_init = TRUE;
			}

			if (luaUqm_comm_stringNeedsInterpolate (entry->string))
				entry->string = luaUqm_comm_stringInterpolate (entry->string);
			
			if (comm_init)
				luaUqm_comm_uninit ();

			t.pStr = entry->string;
			t.CharCount = (COUNT)~0;
			t.baseline.x = r.extent.width;
			while (!getLineWithinWidth (&t, &nextchar, width, (COUNT)~0))
			{
				font_DrawText (&t);
				t.baseline.y += leading;
				t.CharCount = (COUNT)~0;
				t.pStr = nextchar;
			}
			font_DrawText (&t);

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

	SetContextFont (OldFont);
	SetContextForeGroundColor (OldFGColor);
	SetContextBackGroundColor (OldBGColor);
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
	else if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		return FALSE; // bail out
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

		return TRUE;
	}
}

BOOLEAN
Journal (void)
{
	MENU_STATE MenuState = { 0 };
	POINT universe;
	CONTEXT OldContext;
	RECT r, old_r;
	STAMP s;

	JournalStrings = CaptureStringTable (LoadStringTable (JOURNAL_STRTAB));
	if (JournalStrings == 0)
		return FALSE;

	OldContext = SetContext (SpaceContext);

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

	PauseFlash ();

	// Write journals
	ObjectivesJournal ();
	AliensJournal ();
	ArtifactsJournal ();

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

	ContinueFlash ();
	
	return TRUE;
}
