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
#include "planets/lander.h"
#include "gameopt.h"
#include "gamestr.h"


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

#define GGS(flag)    (GET_GAME_STATE(flag))
#define GS(flag)     (GET_GAME_STATE(flag) > 0)
#define GSET(flag,n) (GET_GAME_STATE(flag) == (n))
#define GSLT(flag,n) (GET_GAME_STATE(flag) < (n))
#define GSGE(flag,n) (GET_GAME_STATE(flag) >= (n))

static BOOLEAN
StarbaseBulletins (BYTE bullet)
{
	return (GGS (STARBASE_BULLETS) & (1L << bullet)) != 0;
}

static BOOLEAN
AllianceInfo (BYTE info)
{
	BYTE AllianceMask = GGS (ALLIANCE_MASK);

	return (AllianceMask & info) != 0;
}

static void
WriteJournals (void)
{
	BOOLEAN sb_available, fuel_low, have_radios, have_radioactives,
			need_radioactives, need_lander, need_lander_again, need_fuel,
			need_fuel_again, reported_moonbase,
			aware_of_sam, find_defeat_quan, destroy_sam, find_access_sam;

	BYTE hierarchy_mask;
	BOOLEAN fwiffo_bullet, signal_uranus, can_fwiffo_join,
			spathi_home, spathi_quest, wiped_evil,
			zfp_bullet, met_the_zfp,
			mels_bullet, met_mels,
			investigate_orz, met_orz, orz_status, orz_frumple,
			pkunk_ilwrath, pkunk_melnorme, knownt_pkunk_home,
			sb_arilou, met_arilou,
			sb_chenjesu, sb_mmrnmhrm, met_chmmr,
			sb_andro, andro_dead,
			find_shofixti, shofixti_returned, find_maidens,
			meet_supox, met_supox,
			syreen_bullet, meet_syreen, met_syreen,
			sb_others,
			gen_thraddash, mels_thraddash, met_thraddash,
			meet_utwig, met_utwig,
			meet_umgah, met_umgah,
			find_yehat, mels_yehat, met_yehat;
	BOOLEAN inv_probes, zfp_sly_probes, thradd_probes_00, thradd_probes_01,
			mels_sly_probe, probe_program, sly_know;
	
	BYTE num_rainbows;
	UWORD rainbow_mask;
	BOOLEAN heard_portal, heard_portal_mels, know_portal, have_spawner;
	BOOLEAN melnorme_ultron, have_ahelix, used_ahelix, have_rsphere,
			used_rsphere, have_spindle, used_spindle;
	BOOLEAN burvixese_mels;
	BYTE rbs;
	BOOLEAN rainbow_shofixti, rainbow_supox, rainbow_0, rainbow_7,
			rainbow_5;

	// Objectives log

	AddJournal (OBJECTIVES_JOURNAL, 2,
			1,                            VISIT_EARTH,
			GS (PROBE_MESSAGE_DELIVERED), VISIT_EARTH);

	have_radios = GLOBAL_SIS (ElementAmounts[RADIOACTIVE]) > 0;
	have_radioactives = GS (STARBASE_VISITED) && have_radios;

	AddJournal (OBJECTIVES_JOURNAL, 4,
			GS (PROBE_MESSAGE_DELIVERED), CONTACT_EARTH,
			GS (STARBASE_VISITED),        GET_RADIOACTIVES,
			have_radioactives,            GIVE_RADIOACTIVES,
			GS (RADIOACTIVES_PROVIDED),   GIVE_RADIOACTIVES);

	need_radioactives = GS (STARBASE_VISITED) && !have_radios
			&& !GS (RADIOACTIVES_PROVIDED);
	need_lander = need_radioactives && GLOBAL_SIS (NumLanders) < 1;

	AddJournal (OBJECTIVES_JOURNAL, 2,
			need_lander,                  NEED_LANDER,
			GS (LANDERS_LOST),            NEED_LANDER);

	need_lander_again = need_lander && GS (LANDERS_LOST);

	AddJournal (OBJECTIVES_JOURNAL, 2,
			need_lander_again,            NEED_LANDER_AGAIN,
			!need_lander,                 NO_JOURNAL_ENTRY);

	fuel_low = GLOBAL_SIS (FuelOnBoard) < (2 * FUEL_TANK_SCALE);
	need_fuel = need_radioactives && fuel_low;

	AddJournal (OBJECTIVES_JOURNAL, 2,
			need_fuel,                    NEED_FUEL,
			GS (GIVEN_FUEL_BEFORE),       NEED_FUEL);

	need_fuel_again = need_fuel && GS (GIVEN_FUEL_BEFORE);

	AddJournal (OBJECTIVES_JOURNAL, 2,
			need_fuel_again,              NEED_FUEL_AGAIN,
			!need_fuel,                   NO_JOURNAL_ENTRY);

	reported_moonbase = GS (MOONBASE_DESTROYED) && !GS (MOONBASE_ON_SHIP);

	AddJournal (OBJECTIVES_JOURNAL, 3,
			GS (WILL_DESTROY_BASE),       DESTROY_MOONBASE,
			GS (MOONBASE_ON_SHIP),        REPORT_MOONBASE,
			reported_moonbase,            DESTROY_MOONBASE);

	sb_available = GGS (STARBASE_AVAILABLE);

	AddJournal (OBJECTIVES_JOURNAL, 2,
			GS (RADIOACTIVES_PROVIDED),   RECRUIT_EARTH,
			sb_available,                 RECRUIT_EARTH);	

	aware_of_sam = GS (AWARE_OF_SAMATRA) || GSET (CHMMR_BOMB_STATE, 1);
	find_access_sam = aware_of_sam && !GS (TALKING_PET_ON_SHIP);

	AddJournal (OBJECTIVES_JOURNAL, 2,
			find_access_sam,          FIND_ACCESS_SAMATRA,
			GS (TALKING_PET_ON_SHIP), FIND_ACCESS_SAMATRA);

	find_defeat_quan = sb_available && !aware_of_sam;
	destroy_sam = GS (AWARE_OF_SAMATRA) && GSGE (CHMMR_BOMB_STATE, 2);

	AddJournal (OBJECTIVES_JOURNAL, 3,
			find_defeat_quan, FIND_DEFEAT_URQUAN,
			aware_of_sam,     FIND_DESTROY_SAMATRA,
			destroy_sam,      DESTROY_SAMATRA);

	// Aliens log

	fwiffo_bullet = StarbaseBulletins (9);
	signal_uranus = fwiffo_bullet && !GS (FOUND_PLUTO_SPATHI);
	can_fwiffo_join = GSET (FOUND_PLUTO_SPATHI, 1) && FwiffoCanJoin;

	AddJournal (ALIENS_JOURNAL, 3,
			signal_uranus,                FIND_FWIFFO,
			can_fwiffo_join,              RECRUIT_FWIFFO,
			GSGE (FOUND_PLUTO_SPATHI, 2), MET_FWIFFO);

	spathi_home = IsHomeworldKnown (SPATHI_HOME);
	spathi_quest = GGS (KNOW_SPATHI_QUEST);
	wiped_evil = GGS (SPATHI_CREATURES_ELIMINATED);

	AddJournal (ALIENS_JOURNAL, 5,
			spathi_home,                  CONTACT_SPATHI,
			spathi_quest,                 ERADICATE_EVIL_ONES,
			spathi_quest && wiped_evil,   INFORM_SPATHI_EVIL_ONES,
			RaceAllied (SPATHI_SHIP),     ALLIED_WITH_SPATHI,
			GGS (SPATHI_SHIELDED_SELVES), NO_JOURNAL_ENTRY);

	zfp_bullet = StarbaseBulletins (11);
	met_the_zfp = GS (ZOQFOT_HOME_VISITS) || GS (ZOQFOT_GRPOFFS)
		|| GS (MET_ZOQFOT);

	AddJournal (ALIENS_JOURNAL, 2,
			zfp_bullet,  INVESTIGATE_RIGEL,
			met_the_zfp, INVESTIGATE_RIGEL);

	sb_arilou = AllianceInfo (ALLIANCE_ARILOU);
	met_arilou = (GS (ARILOU_VISITS) || GS (ARILOU_HOME_VISITS));

	AddJournal (ALIENS_JOURNAL, 2,
			sb_arilou,  CONTACT_ARILOU,
			met_arilou, CONTACT_ARILOU);

	sb_chenjesu = AllianceInfo (ALLIANCE_CHENJESU);
	met_chmmr = GS (CHMMR_HOME_VISITS);

	AddJournal (ALIENS_JOURNAL, 2,
			sb_chenjesu, CONTACT_CHENJESU,
			met_chmmr,   CONTACT_CHENJESU);

	sb_mmrnmhrm = AllianceInfo (ALLIANCE_MMRNMHRM);

	AddJournal (ALIENS_JOURNAL, 2,
			sb_mmrnmhrm, CONTACT_MMRNMHRM,
			met_chmmr,   CONTACT_MMRNMHRM);

	mels_bullet = StarbaseBulletins (7);
	met_mels = GS (MET_MELNORME);

	AddJournal (ALIENS_JOURNAL, 2,
			mels_bullet, CONTACT_MELNORME,
			met_mels,   MET_THE_MELNORME);

	hierarchy_mask = GGS (HIERARCHY_MASK);
	sb_andro = (hierarchy_mask & HIERARCHY_ANDROSYNTH) != 0;
	andro_dead = RaceDead (ANDROSYNTH_SHIP);

	AddJournal (ALIENS_JOURNAL, 2,
			sb_andro,   CONTACT_ANDROSYNTH,
			andro_dead, CONTACT_ANDROSYNTH);

	investigate_orz = GGS (INVESTIGATE_ORZ)
			|| GSGE (MELNORME_ALIEN_INFO_STACK, 10);
	met_orz = GGS (MET_ORZ_BEFORE);
	orz_status = RaceAllied (ORZ_SHIP) || RaceDead (ORZ_SHIP)
			|| GSET (ORZ_MANNER, 2);

	AddJournal (ALIENS_JOURNAL, 3,
			investigate_orz, INVESTIGATE_THE_ORZ,
			met_orz,         MET_THE_ORZ,
			orz_status,      NO_JOURNAL_ENTRY);

	orz_frumple = GSET (ORZ_MANNER, 2);

	AddJournal (ALIENS_JOURNAL, 2,
			RaceAllied (ORZ_SHIP), ALLIED_WITH_ORZ,
			orz_frumple,           MADE_ORZ_FRUMPLE);

	sb_others = GGS (HAYES_OTHER_ALIENS);
	pkunk_ilwrath = GGS (HEARD_PKUNK_ILWRATH);
	pkunk_melnorme = GSGE (MELNORME_ALIEN_INFO_STACK, 2);
	knownt_pkunk_home = IsHomeworldKnown (PKUNK_HOME) && GGS (PKUNK_VISITS)
			&& !GGS (PKUNK_HOME_VISITS);

	AddJournal (ALIENS_JOURNAL, 5,
			sb_others,              HAYES_PKUNK,
			pkunk_ilwrath,          HEARD_OF_PKUNK_ILWRATH,
			pkunk_melnorme,         HEARD_OF_PKUNK_MELNORME,
			knownt_pkunk_home,      GO_TO_PKUNK_HOMEWORLD,
			GS (PKUNK_HOME_VISITS), MET_THE_PKUNK);

	find_shofixti = AllianceInfo (ALLIANCE_SHOFIXTI)
			|| GSGE (MELNORME_ALIEN_INFO_STACK, 12)
			|| (IsHomeworldKnown (SHOFIXTI_HOME) && !GGS (SHOFIXTI_VISITS));
	shofixti_returned = RaceAllied (SHOFIXTI_SHIP);
	find_maidens = GSGE (MELNORME_ALIEN_INFO_STACK, 12);

	AddJournal (ALIENS_JOURNAL, 4,
			find_shofixti,            CONTACT_SHOFIXTI,
			GGS (SHOFIXTI_VISITS),    RECRUIT_THE_SHOFIXTI,
			GGS (SHOFIXTI_RECRUITED), SENT_SHOFIXTI,
			shofixti_returned,        SHOFIXTI_REPOPULATED);

	meet_supox = IsHomeworldKnown (SUPOX_HOME)
			|| CheckSphereTracking (SUPOX_SHIP);
	met_supox = GS (SUPOX_HOSTILE) || GS (SUPOX_HOME_VISITS)
			|| GS (SUPOX_VISITS) || GS (SUPOX_STACK1) || GS (SUPOX_STACK2);

	AddJournal (ALIENS_JOURNAL, 2,
			meet_supox, CONTACT_SUPOX,
			met_supox,  MET_THE_SUPOX);

	syreen_bullet = AllianceInfo (ALLIANCE_SYREEN);
	meet_syreen = IsHomeworldKnown (SYREEN_HOME);
	met_syreen = GS (SYREEN_HOME_VISITS) || GS (KNOW_SYREEN_VAULT)
			|| GS (SHIP_VAULT_UNLOCKED) || RaceAllied (SYREEN_SHIP);

	AddJournal (ALIENS_JOURNAL, 3,
			syreen_bullet, CONTACT_SYREEN,
			meet_syreen,   CONTACT_SYREEN_HW,
			met_syreen,    MET_THE_SYREEN);

	gen_thraddash = GGS (INVESTIGATE_THRADD);
	mels_thraddash = GSGE (MELNORME_ALIEN_INFO_STACK, 7);
	met_thraddash = GGS (THRADD_INFO) || GGS (THRADD_STACK_1)
			|| GGS (THRADD_HOSTILE_STACK_2) || GGS (THRADD_HOSTILE_STACK_3)
			|| GGS (THRADD_HOSTILE_STACK_4) || GGS (THRADD_HOSTILE_STACK_5)
			|| GGS (THRADD_VISITS) || GGS (HELIX_VISITS)
			|| GGS (THRADD_HOME_VISITS) || GGS (THRADDASH_BODY_COUNT);

	AddJournal (ALIENS_JOURNAL, 4,
			sb_others,      HAYES_THRADDASH,
			gen_thraddash,  GENERAL_THRADDASH,
			mels_thraddash, MELNORME_THRADDASH,
			met_thraddash,  CONTACTED_THRADDASH);

	meet_utwig = GSGE (MELNORME_EVENTS_INFO_STACK, 7)
			|| CheckSphereTracking (UTWIG_SHIP);
	met_utwig = GGS (UTWIG_HOSTILE) || GGS (UTWIG_INFO)
			|| GGS (UTWIG_HOME_VISITS) || GGS (UTWIG_VISITS)
			|| GGS (BOMB_VISITS) || RaceAllied (UTWIG_SHIP);

	AddJournal (ALIENS_JOURNAL, 2,
			meet_utwig, INVESTIGATE_UTWIG,
			met_utwig,  CONTACTED_UTWIG);

	meet_umgah = GSGE (MELNORME_EVENTS_INFO_STACK, 3)
			|| GGS (INVESTIGATE_UMGAH) || CheckSphereTracking (UTWIG_SHIP)
			|| (hierarchy_mask & HIERARCHY_UMGAH) != 0;
	met_umgah = GGS (MET_NORMAL_UMGAH) || GGS (KNOW_UMGAH_ZOMBIES)
			|| GGS (UMGAH_MENTIONED_TRICKS) || GGS (UMGAH_EVIL_BLOBBIES)
			|| GGS (UMGAH_HOSTILE);

	AddJournal (ALIENS_JOURNAL, 2,
			meet_umgah, CONTACT_UMGAH,
			met_umgah,  MET_THE_UMGAH);

	find_yehat = AllianceInfo (ALLIANCE_YEHAT);
	mels_yehat = GSGE (MELNORME_ALIEN_INFO_STACK, 16);
	met_yehat = GGS (YEHAT_VISITS) || GGS (YEHAT_REBEL_VISITS)
			|| GGS (YEHAT_HOME_VISITS) || GGS (YEHAT_CIVIL_WAR);

	AddJournal (ALIENS_JOURNAL, 3,
			find_yehat, CONTACT_YEHAT,
			mels_yehat, MELNORME_YEHAT,
			met_yehat,  MET_THE_YEHAT);

	inv_probes = GGS (PROBE_EXHIBITED_BUG) || StarbaseBulletins (15);
	zfp_sly_probes = GSET (INVESTIGATE_PROBES, 1);
	thradd_probes_00 = GSET (INVESTIGATE_PROBES, 2);
	mels_sly_probe = GSGE (MELNORME_EVENTS_INFO_STACK, 6)
			|| GSGE (MELNORME_ALIEN_INFO_STACK, 13);
	probe_program = GGS (PLAYER_KNOWS_PROGRAM) || GGS (PLAYER_KNOWS_PROBE);
	sly_know = GGS (SLYLANDRO_KNOW_BROKEN) && GGS (DESTRUCT_CODE_ON_SHIP);

	AddJournal (ALIENS_JOURNAL, 6,
			inv_probes,       INV_SLYLANDRO_PROBE,
			zfp_sly_probes,   INV_SLYLANDRO_ZFP,
			thradd_probes_00, INV_SLY_THRADD_00,
			mels_sly_probe,   MELNORME_SLYLANDRO,
			probe_program,    SLY_PROBE_FAULTY,
			sly_know,         SLYLANDRO_CONVINCED);

	thradd_probes_01 = GSET (INVESTIGATE_PROBES, 3);
	rainbow_mask = MAKE_WORD (GGS (RAINBOW_WORLD0), GGS (RAINBOW_WORLD1));
	rainbow_5 = (rainbow_mask & (1 << 5)) != 0;

	AddJournal (ALIENS_JOURNAL, 2,
			thradd_probes_01, INV_SLY_THRADD_01,
			rainbow_5,        NO_JOURNAL_ENTRY);

	// Curiosities log

	heard_portal = GGS (INVESTIGATE_PORTAL);
	know_portal = heard_portal & (1 << 2);
	have_spawner = GGS (PORTAL_SPAWNER);

	AddJournal (ARTIFACTS_JOURNAL, 2,
			heard_portal & (1 << 0),     FIND_PORTAL_SPATHI,
			have_spawner || know_portal, NO_JOURNAL_ENTRY);

	heard_portal_mels = GSGE (MELNORME_EVENTS_INFO_STACK, 4);

	AddJournal (ARTIFACTS_JOURNAL, 2,
			heard_portal_mels,           FIND_PORTAL_MELS,
			have_spawner || know_portal, NO_JOURNAL_ENTRY);

	AddJournal (ARTIFACTS_JOURNAL, 3,
			heard_portal & (1 << 1), FIND_PORTAL_ARILOU,
			know_portal,             FOUND_QS_PORTAL,
			have_spawner,            NO_JOURNAL_ENTRY);

	AddJournal (ARTIFACTS_JOURNAL, 1,
			GGS (DISCUSSED_GLOWING_ROD), GLOWING_ROD_ENTRY);

	AddJournal (ARTIFACTS_JOURNAL, 1,
			GGS (DISCUSSED_WIMBLIS_TRIDENT), WIMBLIS_TRIDENT_ENTRY);

	melnorme_ultron = GSGE (MELNORME_EVENTS_INFO_STACK, 8);
	have_ahelix = GGS (AQUA_HELIX) && GGS (AQUA_HELIX_ON_SHIP);
	used_ahelix = GGS (AQUA_HELIX) && !GGS (AQUA_HELIX_ON_SHIP);

	AddJournal (ARTIFACTS_JOURNAL, 5,
			melnorme_ultron,                GET_AQUAHELIX,
			mels_thraddash,                 GET_AQUAHELIX_DRACONIS,
			have_ahelix,                    LEARN_AQUAHELIX,
			have_ahelix && melnorme_ultron, USE_AQUAHELIX,
			used_ahelix,                    USED_AQUAHELIX);

	have_rsphere = GGS (ROSY_SPHERE) && GGS (ROSY_SPHERE_ON_SHIP);
	used_rsphere = GGS (ROSY_SPHERE) && !GGS (ROSY_SPHERE_ON_SHIP);

	AddJournal (ARTIFACTS_JOURNAL, 4,
			melnorme_ultron,                 GET_ROSYSPHERE,
			have_rsphere,                    LEARN_ROSYSPHERE,
			have_rsphere && melnorme_ultron, USE_ROSYSPHERE,
			used_rsphere,                    USED_ROSYSPHERE);

	have_spindle = GGS (CLEAR_SPINDLE) && GGS (CLEAR_SPINDLE_ON_SHIP);
	used_spindle = GGS (CLEAR_SPINDLE) && !GGS (CLEAR_SPINDLE_ON_SHIP);

	AddJournal (ARTIFACTS_JOURNAL, 4,
			melnorme_ultron,                 GET_CLEARSPINDLE,
			have_spindle,                    LEARN_CLEARSPINDLE,
			have_spindle && melnorme_ultron, USE_CLEARSPINDLE,
			used_spindle,                    USED_CLEARSPINDLE);

	burvixese_mels = GSGE (MELNORME_ALIEN_INFO_STACK, 6);

	AddJournal (ARTIFACTS_JOURNAL, 2,
			burvixese_mels,               INVESTIGATE_BURVIXESE,
			GGS (BURVIXESE_BROADCASTERS), NO_JOURNAL_ENTRY);

	rainbow_shofixti = GGS (SHOFIXTI_STACK2) > 2
			&& GSGE (SHOFIXTI_STACK1, 2);
	rainbow_0 = (rainbow_mask & (1 << 0)) != 0;

	AddJournal (ARTIFACTS_JOURNAL, 2,
			rainbow_shofixti, FIND_RAINBOW_SEXTANTIS,
			rainbow_0,        NO_JOURNAL_ENTRY);

	rainbow_supox = GSET (SUPOX_STACK1, 6);
	rainbow_7 = (rainbow_mask & (1 << 7)) != 0;

	AddJournal (ARTIFACTS_JOURNAL, 2,
			rainbow_supox, FIND_RAINBOW_LEPORIS,
			rainbow_7,     NO_JOURNAL_ENTRY);

	num_rainbows = RAINBOW9_DEFINED - RAINBOW0_DEFINED;

	for (rbs = 0; rbs <= num_rainbows; rbs++)
	{
		AddJournal (ARTIFACTS_JOURNAL, 1,
				(rainbow_mask & (1 << rbs)) != 0, FOUND_RAINBOW_0 + rbs);
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
	RECT r;
	BOOLEAN HasEntries = FALSE;

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

		DrawSISMessage (JOURNAL_STRING (CAPTAINS_LOG));

	switch (which_journal)
	{
		case OBJECTIVES_JOURNAL:
			DrawSISTitle (JOURNAL_STRING (OBJECTIVES_JOURNAL_HEADER));
			sid = OPEN_OBJECTIVES;
			sid_end = CLOSED_OBJECTIVES;
			break;
		case ALIENS_JOURNAL:
			DrawSISTitle (JOURNAL_STRING (ALIENS_JOURNAL_HEADER));
			sid = OPEN_ALIENS;
			sid_end = CLOSED_ALIENS;
			break;
		case ARTIFACTS_JOURNAL:
			DrawSISTitle (JOURNAL_STRING (ARTIFACTS_JOURNAL_HEADER));
			sid = OPEN_ARTIFACTS;
			sid_end = CLOSED_ARTIFACTS;
			break;
		default:
			snprintf (journal_buf, JOURNAL_BUF_SIZE, "journal #%d",
					which_journal);
			DrawSISTitle (journal_buf);
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

			HasEntries = TRUE;

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

	if (!HasEntries)
	{
		FONT OldEntryFont = SetContextFont (PlayMenuFont);
		SetContextForeGroundColor (LTGRAY_COLOR);
		t.align = ALIGN_CENTER;
		t.pStr = JOURNAL_STRING (NO_OBJECTIVE); // NO ENTRIES
		t.CharCount = (COUNT)~0;
		t.baseline.x = RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1);
		t.baseline.y = RES_SCALE (ORIG_SIS_SCREEN_HEIGHT >> 1);
		font_DrawText (&t);
		SetContextFont (OldEntryFont);
	}

	if (IS_PAD)
		SetContextFont (TinyFontCond);

	SetContextForeGroundColor (COMM_HISTORY_BACKGROUND_COLOR);
	r.corner.y = SIS_SCREEN_HEIGHT - (leading + RES_SCALE (4));
	r.corner.x = 0;
	r.extent.width = SIS_SCREEN_WIDTH;
	r.extent.height = leading + RES_SCALE (4);
	DrawFilledRectangle (&r);

	SetContextForeGroundColor (COMM_HISTORY_TEXT_COLOR);
	t.pStr = JOURNAL_STRING (PRESS_LEFT_RIGHT); // PRESS LEFT OR RIGHT TO
	t.align = ALIGN_CENTER;                     // SHOW THE OTHER LOGS
	t.baseline.y = SIS_SCREEN_HEIGHT - (leading - RES_SCALE (4));
	t.baseline.x = RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1);
	t.CharCount = (COUNT)~0;
	r = font_GetTextRect (&t);
	font_DrawText (&t);

	if (transition_pending)
	{
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
	JournalRequested = FALSE;

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
		PlayMenuSound (MENU_SOUND_SUCCESS);
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
	MENU_SOUND_FLAGS OldSound0, OldSound1;
	InputFrameCallback *oldCallback;
	MENU_STATE MenuState = { 0 };
	POINT universe;
	CONTEXT OldContext;
	RECT r, old_r;
	STAMP s;

	JournalRequested = FALSE;

	if (NOMAD || DIF_HARD || SaveLoadActive || planetSideDesc != NULL
			|| LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE
			|| !(CommData.AlienFrame != NULL || playerInSolarSystem ()
			|| inHQSpace () || inEncounter ()))
	{
		PlayMenuSound (MENU_SOUND_FAILURE);
		return FALSE;
	}

	PlayMenuSound (MENU_SOUND_SUCCESS);

	JournalStrings = CaptureStringTable (LoadStringTable (JOURNAL_STRTAB));
	if (JournalStrings == 0)
		return FALSE;

	if (PlayingTrack ())
		PauseTrack ();

	OldContext = SetContext (SpaceContext);

	GetContextClipRect (&old_r);

	// For the Orbital, Shipyard, and Comm transitions
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

	GetMenuSounds (&OldSound0, &OldSound1);
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	SetDefaultMenuRepeatDelay ();
	oldCallback = SetInputCallback (NULL);

	DoInput (&MenuState, FALSE);

	SetInputCallback (oldCallback);

	FreeJournals ();

	SetContextClipRect (&r);
	SetTransitionSource (&r);
	BatchGraphics ();
		
		DrawStamp (&s);
		DrawSISMessage (NULL);

		if (inHQSpace ())
			DrawHyperCoords (GLOBAL (ShipStamp.origin));
		else if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) == (BYTE)~0)
			DrawSISTitle (GAME_STRING (STARBASE_STRING_BASE + 0));
		else if (GLOBAL (ip_planet) || playerInInnerSystem ())
			DrawSISTitle (GLOBAL_SIS (PlanetName));
		else
			DrawHyperCoords (CurStarDescPtr->star_pt);

		ScreenTransition (optScrTrans, &r);
	
	UnbatchGraphics ();

	DestroyDrawable (ReleaseDrawable (s.frame));
	FlushInput ();

	SetContextClipRect (&old_r);
	SetContext (OldContext);

	ContinueFlash ();
	SetMenuSounds (OldSound0, OldSound1);

	if (PlayingTrack ())
		ResumeTrack ();
	
	return TRUE;
}
