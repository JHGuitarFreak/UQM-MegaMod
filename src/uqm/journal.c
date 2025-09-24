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

#include <stdlib.h>
#include <stdarg.h>

#include "build.h"
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
#include "comm/starbas/strings.h"


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

#define GGS(flag)    (GET_GAME_STATE(flag))
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
WriteJournals (void)
{	// starbase missions
	BOOLEAN StarbaseAvailable = GGS (STARBASE_AVAILABLE);
	BOOLEAN FuelLow = GLOBAL_SIS (FuelOnBoard) < (2 * FUEL_TANK_SCALE);
	BOOLEAN HaveRadios = GLOBAL_SIS (ElementAmounts[RADIOACTIVE]) > 0;
	BOOLEAN have_radioactives = GS (STARBASE_VISITED) && HaveRadios;
	BOOLEAN need_radioactives = GS (STARBASE_VISITED) && !HaveRadios &&
		!GS (RADIOACTIVES_PROVIDED);
	BOOLEAN need_lander = need_radioactives && GLOBAL_SIS (NumLanders) < 1;
	BOOLEAN need_lander_again = need_lander && GS (LANDERS_LOST);
	BOOLEAN need_fuel = need_radioactives && FuelLow;
	BOOLEAN need_fuel_again = need_fuel && GS (GIVEN_FUEL_BEFORE);
	BOOLEAN reported_moonbase = GS (MOONBASE_DESTROYED)
			&& !GS (MOONBASE_ON_SHIP);

	BOOLEAN AwareOfSAM =
			GS (AWARE_OF_SAMATRA) || GSET (CHMMR_BOMB_STATE, 1);
	BOOLEAN FindDefeatQuan = StarbaseAvailable && !AwareOfSAM;
	BOOLEAN FindDestroySAM = AwareOfSAM;
	BOOLEAN DestroySAM =
			GS (AWARE_OF_SAMATRA) && GSGE (CHMMR_BOMB_STATE, 2);
	BOOLEAN FindAccessSAM = AwareOfSAM && !GS (TALKING_PET_ON_SHIP);


	// Alien missions
	BYTE AllianceMask = GGS (ALLIANCE_MASK);
	BYTE HierarchyMask = GGS (HIERARCHY_MASK);

	BOOLEAN FwiffoBullet = (GGS (STARBASE_BULLETS) & (1L << 9)) != 0;
	BOOLEAN signal_uranus = FwiffoBullet && !GS (FOUND_PLUTO_SPATHI);
	BOOLEAN can_fwiffo_join = GSET (FOUND_PLUTO_SPATHI, 1) && FwiffoCanJoin;

	BOOLEAN ZFPBullet = (GGS (STARBASE_BULLETS) & (1L << 11)) != 0;
	BOOLEAN met_the_zfp = GS (ZOQFOT_HOME_VISITS) || GS (ZOQFOT_GRPOFFS)
			|| GS (MET_ZOQFOT);

	BOOLEAN MelsBullet = (GGS (STARBASE_BULLETS) & (1L << 7)) != 0;
	BOOLEAN met_mels = GS (MET_MELNORME);

	BOOLEAN OrzVisits = GGS (ORZ_VISITS);
	BOOLEAN OrzHomeVisits = GGS (ORZ_HOME_VISITS);

	BOOLEAN PkunkIlwrath = GGS (HEARD_PKUNK_ILWRATH);
	BOOLEAN PkunkMelnorme = GGS (MELNORME_ALIEN_INFO_STACK) >= 2;
	BOOLEAN KnowntPkunkHome = IsHomeworldKnown (PKUNK_HOME)
			&& GGS (PKUNK_VISITS) && !GGS (PKUNK_HOME_VISITS);

	BOOLEAN sb_arilou = (AllianceMask & ALLIANCE_ARILOU) != 0;
	BOOLEAN met_arilou = (GS (ARILOU_VISITS) || GS (ARILOU_HOME_VISITS));

	BOOLEAN sb_chenjesu = (AllianceMask & ALLIANCE_CHENJESU) != 0;
	BOOLEAN sb_mmrnmhrm = (AllianceMask & ALLIANCE_MMRNMHRM) != 0;
	BOOLEAN met_chmmr = GS (CHMMR_HOME_VISITS);

	BOOLEAN sb_andro = (HierarchyMask & HIERARCHY_ANDROSYNTH) != 0;
	BOOLEAN andro_dead = RaceDead (ANDROSYNTH_SHIP);

	BOOLEAN find_shofixti = (AllianceMask & ALLIANCE_SHOFIXTI) != 0
			|| GGS (MELNORME_ALIEN_INFO_STACK) >= 12
			|| (IsHomeworldKnown (SHOFIXTI_HOME) && !GGS (SHOFIXTI_VISITS));
	BOOLEAN shofixti_returned = RaceAllied (SHOFIXTI_SHIP);
	BOOLEAN find_maidens = GGS (MELNORME_ALIEN_INFO_STACK) >= 12;

	BOOLEAN meet_supox = IsHomeworldKnown (SUPOX_HOME)
			|| CheckSphereTracking (SUPOX_SHIP);
	BOOLEAN met_supox = GS (SUPOX_HOSTILE) || GS (SUPOX_HOME_VISITS)
			|| GS (SUPOX_VISITS) || GS (SUPOX_STACK1) || GS (SUPOX_STACK2);

	BOOLEAN SyreenBullet = (AllianceMask & ALLIANCE_SYREEN) != 0;
	BOOLEAN meet_syreen = IsHomeworldKnown (SYREEN_HOME);
	BOOLEAN met_syreen = GS (SYREEN_HOME_VISITS)
			|| GS (KNOW_SYREEN_VAULT) || GS (SHIP_VAULT_UNLOCKED)
			|| RaceAllied (SYREEN_SHIP);

	BOOLEAN sb_others = GGS (HAYES_OTHER_ALIENS);

	BOOLEAN gen_thraddash = GGS (INVESTIGATE_THRADD);
	BOOLEAN mels_thraddash = GGS (MELNORME_ALIEN_INFO_STACK) >= 7;
	BOOLEAN met_thraddash = GGS (THRADD_INFO) || GGS (THRADD_STACK_1)
			|| GGS (THRADD_HOSTILE_STACK_2) || GGS (THRADD_HOSTILE_STACK_3)
			|| GGS (THRADD_HOSTILE_STACK_4) || GGS (THRADD_HOSTILE_STACK_5)
			|| GGS (THRADD_VISITS) || GGS (HELIX_VISITS)
			|| GGS (THRADD_HOME_VISITS) || GGS (THRADDASH_BODY_COUNT);

	BOOLEAN meet_utwig = GGS (MELNORME_EVENTS_INFO_STACK) >= 7
			|| CheckSphereTracking (UTWIG_SHIP);
	BOOLEAN met_utwig = GGS (UTWIG_HOSTILE) || GGS (UTWIG_INFO)
			|| GGS (UTWIG_HOME_VISITS) || GGS (UTWIG_VISITS)
			|| GGS (BOMB_VISITS) || RaceAllied (UTWIG_SHIP);
	
	BOOLEAN meet_umgah = GGS (MELNORME_EVENTS_INFO_STACK) >= 3
			|| GGS (INVESTIGATE_UMGAH) || CheckSphereTracking (UTWIG_SHIP);
	BOOLEAN met_umgah = GGS (MET_NORMAL_UMGAH) || GGS (KNOW_UMGAH_ZOMBIES)
			|| GGS (UMGAH_MENTIONED_TRICKS) || GGS (UMGAH_EVIL_BLOBBIES)
			|| GGS (UMGAH_HOSTILE);

	{	// Objectives Journal
		// Starbase Missions
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
				StarbaseAvailable,            RECRUIT_EARTH);

		// Sa-Matra Missions
		AddJournal (OBJECTIVES_JOURNAL, 2,
				FindAccessSAM,            FIND_ACCESS_SAMATRA,
				GS (TALKING_PET_ON_SHIP), FIND_ACCESS_SAMATRA);

		AddJournal (OBJECTIVES_JOURNAL, 3,
				FindDefeatQuan, FIND_DEFEAT_URQUAN,
				AwareOfSAM,     FIND_DESTROY_SAMATRA,
				DestroySAM,     DESTROY_SAMATRA);
	}

	{	// Aliens Journal
		AddJournal (ALIENS_JOURNAL, 3,
				signal_uranus,                FIND_FWIFFO,
				can_fwiffo_join,              RECRUIT_FWIFFO,
				GSGE (FOUND_PLUTO_SPATHI, 2), MET_FWIFFO);

		AddJournal (ALIENS_JOURNAL, 2,
				ZFPBullet,   INVESTIGATE_RIGEL,
				met_the_zfp, INVESTIGATE_RIGEL);

		AddJournal (ALIENS_JOURNAL, 2,
				sb_arilou,   CONTACT_ARILOU,
				met_arilou,  CONTACT_ARILOU);

		AddJournal (ALIENS_JOURNAL, 2,
				sb_chenjesu, CONTACT_CHENJESU,
				met_chmmr,   CONTACT_CHENJESU);

		AddJournal (ALIENS_JOURNAL, 2,
				sb_mmrnmhrm, CONTACT_MMRNMHRM,
				met_chmmr,   CONTACT_MMRNMHRM);

		AddJournal (ALIENS_JOURNAL, 2,
				MelsBullet && !met_mels, CONTACT_MELNORME,
				MelsBullet && met_mels,  MET_THE_MELNORME);

		AddJournal (ALIENS_JOURNAL, 2,
				sb_andro,                CONTACT_ANDROSYNTH,
				andro_dead || OrzVisits, CONTACT_ANDROSYNTH);

		AddJournal (ALIENS_JOURNAL, 2,
				OrzVisits && !OrzHomeVisits, VISIT_ORZ_HOMEWORLD,
				OrzVisits && OrzHomeVisits,  VISIT_ORZ_HOMEWORLD);

		AddJournal (ALIENS_JOURNAL, 5,
				sb_others,         HAYES_PKUNK,
				PkunkIlwrath,      HEARD_OF_PKUNK_ILWRATH,
				PkunkMelnorme,     HEARD_OF_PKUNK_MELNORME,
				KnowntPkunkHome,   GO_TO_PKUNK_HOMEWORLD,
				GS (PKUNK_HOME_VISITS), MET_THE_PKUNK);

		AddJournal (ALIENS_JOURNAL, 4,
				find_shofixti,            CONTACT_SHOFIXTI,
				GGS (SHOFIXTI_VISITS),    RECRUIT_THE_SHOFIXTI,
				GGS (SHOFIXTI_RECRUITED), SENT_SHOFIXTI,
				shofixti_returned,        SHOFIXTI_REPOPULATED);

		AddJournal (ALIENS_JOURNAL, 2,
				meet_supox, CONTACT_SUPOX,
				met_supox,  MET_THE_SUPOX);

		AddJournal (ALIENS_JOURNAL, 3,
				SyreenBullet, CONTACT_SYREEN,
				meet_syreen,  CONTACT_SYREEN_HW,
				met_syreen,   MET_THE_SYREEN);

		AddJournal (ALIENS_JOURNAL, 4,
				sb_others,      HAYES_THRADDASH,
				gen_thraddash,  GENERAL_THRADDASH,
				mels_thraddash, MELNORME_THRADDASH,
				met_thraddash,  CONTACTED_THRADDASH);

		AddJournal (ALIENS_JOURNAL, 2,
				meet_utwig, INVESTIGATE_UWTIG,
				met_utwig,  CONTACTED_UTWIG);

		AddJournal (ALIENS_JOURNAL, 2,
				meet_umgah, CONTACT_UMGAH,
				met_umgah,  MET_THE_UMGAH);


	}

	{	// Artifacts Journal

	}
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
			if (!sid_open && journal_section[sid - 1].head != NULL)
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

	{
		RECT r;

		SetContextForeGroundColor (COMM_HISTORY_BACKGROUND_COLOR);
		r.corner.y = SIS_SCREEN_HEIGHT - (leading + RES_SCALE (4));
		r.corner.x = 0;
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = leading + RES_SCALE (4);
		DrawFilledRectangle (&r);

		SetContextForeGroundColor (COMM_HISTORY_TEXT_COLOR);
		t.pStr = JOURNAL_STRING (PRESS_LEFT_RIGHT);
		t.align = ALIGN_CENTER;
		t.baseline.y = SIS_SCREEN_HEIGHT - (leading - RES_SCALE (4));
		t.baseline.x = RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1);
		t.CharCount = (COUNT)~0;
		r = font_GetTextRect (&t);
		font_DrawText (&t);
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

	if (NOMAD || DIF_HARD)
		return FALSE;

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
