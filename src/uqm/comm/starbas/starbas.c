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

#include "../commall.h"
#include "../comandr/resinst.h"
#include "strings.h"
#include "../../../options.h"
#include "uqm/lua/luacomm.h"
#include "uqm/build.h"
#include "uqm/setup.h"
#include "uqm/shipcont.h"
#include "uqm/sis.h"
		// for DeltaSISGauges()
#include "libs/graphics/gfx_common.h"
#include "libs/mathlib.h"
#include "libs/inplib.h"


static void TellMission (RESPONSE_REF R);
static void SellMinerals (RESPONSE_REF R);


static LOCDATA commander_desc_orig =
{
	COMMANDER_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	COMMANDER_PMAP_ANIM, /* AlienFrame */
	COMMANDER_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_MIDDLE, /* AlienTextValign */
	COMMANDER_COLOR_MAP, /* AlienColorMap */
	COMMANDER_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	STARBASE_CONVERSATION_PHRASES, /* PlayerPhrases */
	10, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{ /* Blink */
			1, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			0, ONE_SECOND * 8, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Running light */
			10, /* StartIndex */
			30, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			ONE_SECOND * 2, 0, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Arc welder 0 */
			40, /* StartIndex */
			7, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 8, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Arc welder 1 */
			47, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 8, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Arc welder 2 */
			55, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 8, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Arc welder 3 */
			61, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 8, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Arc welder 4 */
			67, /* StartIndex */
			7, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 8, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Arc welder 5 */
			74, /* StartIndex */
			11, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 8, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Arc welder 6 */
			85, /* StartIndex */
			10, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 8, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Flagship picture */
			95, /* StartIndex */
			1, /* NumFrames */
			0, /* AnimFlags */
			0, 0, /* FrameRate */
			0, 0, /* RestartRate */
			0, /* BlockMask */
		},
	},
	{ /* AlienTransitionDesc */
		0, /* StartIndex */
		0, /* NumFrames */
		0, /* AnimFlags */
		0, 0, /* FrameRate */
		0, 0, /* RestartRate */
		0, /* BlockMask */
	},
	{ /* AlienTalkDesc */
		4, /* StartIndex */
		6, /* NumFrames */
		0, /* AnimFlags */
		ONE_SECOND / 10, ONE_SECOND / 15, /* FrameRate */
		ONE_SECOND * 7 / 60, ONE_SECOND / 12, /* RestartRate */
		0, /* BlockMask */
	},
	NULL, /* AlienNumberSpeech - none */
	/* Filler for loaded resources */
	NULL, NULL, NULL,
	NULL,
	NULL,
};

static LOCDATA commander_desc_hd =
{
	COMMANDER_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	COMMANDER_PMAP_ANIM, /* AlienFrame */
	COMMANDER_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_MIDDLE, /* AlienTextValign */
	COMMANDER_COLOR_MAP, /* AlienColorMap */
	COMMANDER_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	STARBASE_CONVERSATION_PHRASES, /* PlayerPhrases */
	9, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{ /* Blink */
			1, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			0, ONE_SECOND * 8, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Running light */
			10, /* StartIndex */
			27, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			ONE_SECOND * 2, 0, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Flagship picture */
			37, /* StartIndex */
			1, /* NumFrames */
			0, /* AnimFlags */
			0, 0, /* FrameRate */
			0, 0, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Flagship side lights */
			38, /* StartIndex */
			2, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND * 2, 0, /* FrameRate */
			0, ONE_SECOND * 12, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Arc welder 1 */
			40, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 8, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Arc welder 2 */
			48, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND, 0, /* FrameRate */
			0, ONE_SECOND * 8, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Arc welder 3 */
			54, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 8, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Arc welder 4 */
			60, /* StartIndex */
			7, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 8, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Arc welder 5 */
			67, /* StartIndex */
			11, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 8, /* RestartRate */
			0, /* BlockMask */
		},
	},
	{ /* AlienTransitionDesc */
		0, /* StartIndex */
		0, /* NumFrames */
		0, /* AnimFlags */
		0, 0, /* FrameRate */
		0, 0, /* RestartRate */
		0, /* BlockMask */
	},
	{ /* AlienTalkDesc */
		4, /* StartIndex */
		6, /* NumFrames */
		0, /* AnimFlags */
		ONE_SECOND / 10, ONE_SECOND / 15, /* FrameRate */
		ONE_SECOND * 7 / 60, ONE_SECOND / 12, /* RestartRate */
		0, /* BlockMask */
	},
	NULL, /* AlienNumberSpeech - none */
	/* Filler for loaded resources */
	NULL, NULL, NULL,
	NULL,
	NULL,
};

static DWORD CurBulletinMask;

static void
ByeBye (RESPONSE_REF R)
{
	(void) R;  // ignored

	CurBulletinMask |= GET_GAME_STATE (STARBASE_BULLETS);
	SET_GAME_STATE (STARBASE_BULLETS, CurBulletinMask);

	/* if (R == goodbye_starbase_commander) */
	if (GET_GAME_STATE (CHMMR_BOMB_STATE) >= 2)
		NPCPhrase (GOOD_LUCK_AGAIN);
	else
	{
		RESPONSE_REF pStr = 0;
		
		switch ((BYTE)TFB_Random () & 7)
		{
			case 0: pStr = NORMAL_GOODBYE_A; break;
			case 1: pStr = NORMAL_GOODBYE_B; break;
			case 2: pStr = NORMAL_GOODBYE_C; break;
			case 3: pStr = NORMAL_GOODBYE_D; break;
			case 4: pStr = NORMAL_GOODBYE_E; break;
			case 5: pStr = NORMAL_GOODBYE_F; break;
			case 6: pStr = NORMAL_GOODBYE_G; break;
			case 7: pStr = NORMAL_GOODBYE_H; break;
		}

		NPCPhrase (pStr);
	}
}

static void NeedInfo (RESPONSE_REF R);
static void TellHistory (RESPONSE_REF R);
static void AlienRaces (RESPONSE_REF R);

static BYTE stack0;
static BYTE stack1;
static BYTE stack2;
static BYTE stack3;

static void
AllianceInfo (RESPONSE_REF R)
{
#define ALLIANCE_SHOFIXTI (1 << 0)
#define ALLIANCE_YEHAT (1 << 1)
#define ALLIANCE_ARILOU (1 << 2)
#define ALLIANCE_CHENJESU (1 << 3)
#define ALLIANCE_MMRNMHRM (1 << 4)
#define ALLIANCE_SYREEN (1 << 5)
	static BYTE AllianceMask = 0;

	if (PLAYER_SAID (R, what_about_alliance))
	{
		NPCPhrase (WHICH_ALLIANCE);
		AllianceMask = 0;
	}
	else if (PLAYER_SAID (R, shofixti))
	{
		NPCPhrase (ABOUT_SHOFIXTI);
		AllianceMask |= ALLIANCE_SHOFIXTI;
	}
	else if (PLAYER_SAID (R, yehat))
	{
		NPCPhrase (ABOUT_YEHAT);
		AllianceMask |= ALLIANCE_YEHAT;
	}
	else if (PLAYER_SAID (R, arilou))
	{
		NPCPhrase (ABOUT_ARILOU);
		AllianceMask |= ALLIANCE_ARILOU;
	}
	else if (PLAYER_SAID (R, chenjesu))
	{
		NPCPhrase (ABOUT_CHENJESU);
		AllianceMask |= ALLIANCE_CHENJESU;
	}
	else if (PLAYER_SAID (R, mmrnmhrm))
	{
		NPCPhrase (ABOUT_MMRNMHRM);
		AllianceMask |= ALLIANCE_MMRNMHRM;
	}
	else if (PLAYER_SAID (R, syreen))
	{
		NPCPhrase (ABOUT_SYREEN);
		AllianceMask |= ALLIANCE_SYREEN;
	}

	if (!(AllianceMask & ALLIANCE_SHOFIXTI))
		Response (shofixti, AllianceInfo);
	if (!(AllianceMask & ALLIANCE_YEHAT))
		Response (yehat, AllianceInfo);
	if (!(AllianceMask & ALLIANCE_ARILOU))
		Response (arilou, AllianceInfo);
	if (!(AllianceMask & ALLIANCE_CHENJESU))
		Response (chenjesu, AllianceInfo);
	if (!(AllianceMask & ALLIANCE_MMRNMHRM))
		Response (mmrnmhrm, AllianceInfo);
	if (!(AllianceMask & ALLIANCE_SYREEN))
		Response (syreen, AllianceInfo);
	Response (enough_alliance, AlienRaces);
}

static void
HierarchyInfo (RESPONSE_REF R)
{
#define HIERARCHY_MYCON (1 << 0)
#define HIERARCHY_SPATHI (1 << 1)
#define HIERARCHY_UMGAH (1 << 2)
#define HIERARCHY_ANDROSYNTH (1 << 3)
#define HIERARCHY_ILWRATH (1 << 4)
#define HIERARCHY_VUX (1 << 5)
#define HIERARCHY_URQUAN (1 << 6)
	static BYTE HierarchyMask = 0;

	if (PLAYER_SAID (R, what_about_hierarchy))
	{
		NPCPhrase (WHICH_HIERARCHY);
		HierarchyMask = 0;
	}
	else if (PLAYER_SAID (R, urquan))
	{
		NPCPhrase (ABOUT_URQUAN);
		HierarchyMask |= HIERARCHY_URQUAN;
	}
	else if (PLAYER_SAID (R, mycon))
	{
		NPCPhrase (ABOUT_MYCON);
		HierarchyMask |= HIERARCHY_MYCON;
	}
	else if (PLAYER_SAID (R, spathi))
	{
		NPCPhrase (ABOUT_SPATHI);
		HierarchyMask |= HIERARCHY_SPATHI;
	}
	else if (PLAYER_SAID (R, umgah))
	{
		NPCPhrase (ABOUT_UMGAH);
		HierarchyMask |= HIERARCHY_UMGAH;
	}
	else if (PLAYER_SAID (R, androsynth))
	{
		NPCPhrase (ABOUT_ANDROSYNTH);
		HierarchyMask |= HIERARCHY_ANDROSYNTH;
	}
	else if (PLAYER_SAID (R, ilwrath))
	{
		NPCPhrase (ABOUT_ILWRATH);
		HierarchyMask |= HIERARCHY_ILWRATH;
	}
	else if (PLAYER_SAID (R, vux))
	{
		NPCPhrase (ABOUT_VUX);
		HierarchyMask |= HIERARCHY_VUX;
	}

	if (!(HierarchyMask & HIERARCHY_URQUAN))
		Response (urquan, HierarchyInfo);
	if (!(HierarchyMask & HIERARCHY_MYCON))
		Response (mycon, HierarchyInfo);
	if (!(HierarchyMask & HIERARCHY_SPATHI))
		Response (spathi, HierarchyInfo);
	if (!(HierarchyMask & HIERARCHY_UMGAH))
		Response (umgah, HierarchyInfo);
	if (!(HierarchyMask & HIERARCHY_ANDROSYNTH))
		Response (androsynth, HierarchyInfo);
	if (!(HierarchyMask & HIERARCHY_ILWRATH))
		Response (ilwrath, HierarchyInfo);
	if (!(HierarchyMask & HIERARCHY_VUX))
		Response (vux, HierarchyInfo);
	Response (enough_hierarchy, AlienRaces);
}

static void
AlienRaces (RESPONSE_REF R)
{
#define RACES_ALLIANCE (1 << 0)
#define RACES_HIERARCHY (1 << 1)
#define RACES_OTHER (1 << 2)
	static BYTE RacesMask = 0;

	if (PLAYER_SAID (R, alien_races))
	{
		NPCPhrase (WHICH_ALIEN);
		RacesMask = 0;
	}
	else if (PLAYER_SAID (R, enough_alliance))
	{
		NPCPhrase (OK_ENOUGH_ALLIANCE);
		RacesMask |= RACES_ALLIANCE;
	}
	else if (PLAYER_SAID (R, enough_hierarchy))
	{
		NPCPhrase (OK_ENOUGH_HIERARCHY);
		RacesMask |= RACES_HIERARCHY;
	}
	else if (PLAYER_SAID (R, what_about_other))
	{
		NPCPhrase (ABOUT_OTHER);
		RacesMask |= RACES_OTHER;
	}

	if (!(RacesMask & RACES_ALLIANCE))
	{
		Response (what_about_alliance, AllianceInfo);
	}
	if (!(RacesMask & RACES_HIERARCHY))
	{
		Response (what_about_hierarchy, HierarchyInfo);
	}
	if (!(RacesMask & RACES_OTHER))
	{
		Response (what_about_other, AlienRaces);
	}
	Response (enough_aliens, TellHistory);
}

static void
WarInfo (RESPONSE_REF R)
{
#define WAR_STARTED (1 << 0)
#define WAR_WAS_LIKE (1 << 1)
#define WAR_LOST (1 << 2)
#define WAR_AFTERMATH (1 << 3)
	static BYTE WarMask = 0;

	if (PLAYER_SAID (R, the_war))
	{
		NPCPhrase (WHICH_WAR);
		WarMask = 0;
	}
	else if (PLAYER_SAID (R, what_started_war))
	{
		NPCPhrase (URQUAN_STARTED_WAR);
		WarMask |= WAR_STARTED;
	}
	else if (PLAYER_SAID (R, what_was_war_like))
	{
		NPCPhrase (WAR_WAS_LIKE_SO);
		WarMask |= WAR_WAS_LIKE;
	}
	else if (PLAYER_SAID (R, why_lose_war))
	{
		NPCPhrase (LOST_WAR_BECAUSE);
		WarMask |= WAR_LOST;
	}
	else if (PLAYER_SAID (R, what_after_war))
	{
		NPCPhrase (AFTER_WAR);
		WarMask |= WAR_AFTERMATH;
	}

	if (!(WarMask & WAR_STARTED))
		Response (what_started_war, WarInfo);
	if (!(WarMask & WAR_WAS_LIKE))
		Response (what_was_war_like, WarInfo);
	if (!(WarMask & WAR_LOST))
		Response (why_lose_war, WarInfo);
	if (!(WarMask & WAR_AFTERMATH))
		Response (what_after_war, WarInfo);
	Response (enough_war, TellHistory);
}

static void
AncientHistory (RESPONSE_REF R)
{
#define ANCIENT_PRECURSORS (1 << 0)
#define ANCIENT_RACES (1 << 1)
#define ANCIENT_EARTH (1 << 2)
	static BYTE AncientMask = 0;

	if (PLAYER_SAID (R, ancient_history))
	{
		NPCPhrase (WHICH_ANCIENT);
		AncientMask = 0;
	}
	else if (PLAYER_SAID (R, precursors))
	{
		NPCPhrase (ABOUT_PRECURSORS);
		AncientMask |= ANCIENT_PRECURSORS;
	}
	else if (PLAYER_SAID (R, old_races))
	{
		NPCPhrase (ABOUT_OLD_RACES);
		AncientMask |= ANCIENT_RACES;
	}
	else if (PLAYER_SAID (R, aliens_on_earth))
	{
		NPCPhrase (ABOUT_ALIENS_ON_EARTH);
		AncientMask |= ANCIENT_EARTH;
	}

	if (!(AncientMask & ANCIENT_PRECURSORS))
		Response (precursors, AncientHistory);
	if (!(AncientMask & ANCIENT_RACES))
		Response (old_races, AncientHistory);
	if (!(AncientMask & ANCIENT_EARTH))
		Response (aliens_on_earth, AncientHistory);
	Response (enough_ancient, TellHistory);
}

static void
TellHistory (RESPONSE_REF R)
{
	RESPONSE_REF pstack[3];

	if (PLAYER_SAID (R, history))
	{
		NPCPhrase (WHICH_HISTORY);
		stack0 = 0;
		stack1 = 0;
		stack2 = 0;
	}
	else if (PLAYER_SAID (R, enough_aliens))
	{
		NPCPhrase (OK_ENOUGH_ALIENS);

		stack0 = 1;
	}
	else if (PLAYER_SAID (R, enough_war))
	{
		NPCPhrase (OK_ENOUGH_WAR);

		stack1 = 1;
	}
	else if (PLAYER_SAID (R, enough_ancient))
	{
		NPCPhrase (OK_ENOUGH_ANCIENT);

		stack2 = 1;
	}

	switch (stack0)
	{
		case 0:
			pstack[0] = alien_races;
			break;
		default:
			pstack[0] = 0;
			break;
	}
	switch (stack1)
	{
		case 0:
			pstack[1] = the_war;
			break;
		default:
			pstack[1] = 0;
			break;
	}
	switch (stack2)
	{
		case 0:
			pstack[2] = ancient_history;
			break;
		default:
			pstack[2] = 0;
			break;
	}

	if (pstack[0])
	{
		Response (pstack[0], AlienRaces);
	}
	if (pstack[1])
	{
		Response (pstack[1], WarInfo);
	}
	if (pstack[2])
	{
		Response (pstack[2], AncientHistory);
	}
	Response (enough_history, NeedInfo);
}

static void
DefeatUrquan (RESPONSE_REF R)
{
#define HOW_FIND_URQUAN (1 << 0)
#define HOW_FIGHT_URQUAN (1 << 1)
#define HOW_ALLY_AGAINST_URQUAN (1 << 2)
#define HOW_STRONG_AGAINST_URQUAN (1 << 3)
	static BYTE DefeatMask = 0;

	if (PLAYER_SAID (R, how_defeat))
	{
		NPCPhrase (DEFEAT_LIKE_SO);
		DefeatMask = 0;
	}
	else if (PLAYER_SAID (R, how_find_urquan))
	{
		NPCPhrase (FIND_URQUAN);
		DefeatMask |= HOW_FIND_URQUAN;
	}
	else if (PLAYER_SAID (R, how_fight_urquan))
	{
		NPCPhrase (FIGHT_URQUAN);
		DefeatMask |= HOW_FIGHT_URQUAN;
	}
	else if (PLAYER_SAID (R, how_ally))
	{
		NPCPhrase (ALLY_LIKE_SO);
		DefeatMask |= HOW_ALLY_AGAINST_URQUAN;
	}
	else if (PLAYER_SAID (R, how_get_strong))
	{
		NPCPhrase (STRONG_LIKE_SO);
		DefeatMask |= HOW_STRONG_AGAINST_URQUAN;
	}

	if (!(DefeatMask & HOW_FIND_URQUAN))
		Response (how_find_urquan, DefeatUrquan);
	if (!(DefeatMask & HOW_FIGHT_URQUAN))
		Response (how_fight_urquan, DefeatUrquan);
	if (!(DefeatMask & HOW_ALLY_AGAINST_URQUAN))
		Response (how_ally, DefeatUrquan);
	if (!(DefeatMask & HOW_STRONG_AGAINST_URQUAN))
		Response (how_get_strong, DefeatUrquan);
	Response (enough_defeat, TellMission);
}

static void
AnalyzeCondition (void)
{
	BYTE i;
	BYTE num_thrusters = 0,
				num_jets = 0,
				num_guns = 0,
				num_bays = 0,
				num_batts = 0,
				num_track = 0,
				num_defense = 0;
	BOOLEAN HasMinimum;

	for (i = 0; i < NUM_DRIVE_SLOTS; ++i)
	{
		if (GLOBAL_SIS (DriveSlots[i]) < EMPTY_SLOT)
			++num_thrusters;
	}
	for (i = 0; i < NUM_JET_SLOTS; ++i)
	{
		if (GLOBAL_SIS (JetSlots[i]) < EMPTY_SLOT)
			++num_jets;
	}
	for (i = 0; i < NUM_MODULE_SLOTS; ++i)
	{
		BYTE which_piece;

		switch (which_piece = GLOBAL_SIS (ModuleSlots[i]))
		{
			case STORAGE_BAY:
				++num_bays;
				break;
			case DYNAMO_UNIT:
			case SHIVA_FURNACE:
				num_batts += 1 + (which_piece - DYNAMO_UNIT);
				break;
			case GUN_WEAPON:
			case BLASTER_WEAPON:
			case CANNON_WEAPON:
				num_guns += 1 + (which_piece - GUN_WEAPON);
				break;
			case TRACKING_SYSTEM:
				++num_track;
				break;
			case ANTIMISSILE_DEFENSE:
				++num_defense;
				break;
		}
	}
	if (num_track && num_guns)
		num_guns += 2;

	HasMinimum = (num_thrusters >= 7 && num_jets >= 5
			&& GLOBAL_SIS (CrewEnlisted) >= CREW_POD_CAPACITY
			&& GLOBAL_SIS (FuelOnBoard) >= FUEL_TANK_CAPACITY
			&& num_bays >= 1 && GLOBAL_SIS (NumLanders)
			&& num_batts >= 1 && num_guns >= 2);
	NPCPhrase (LETS_SEE);
	if (!HasMinimum && GET_GAME_STATE (CHMMR_BOMB_STATE) < 2)
	{
		NPCPhrase (IMPROVE_1);
		if (num_thrusters < 7)
			NPCPhrase (NEED_THRUSTERS_1);
		if (num_jets < 5)
			NPCPhrase (NEED_TURN_1);
		if (num_guns < 2)
			NPCPhrase (NEED_GUNS_1);
		if (GLOBAL_SIS (CrewEnlisted) < CREW_POD_CAPACITY)
			NPCPhrase (NEED_CREW_1);
		if (GLOBAL_SIS (FuelOnBoard) < FUEL_TANK_CAPACITY)
			NPCPhrase (NEED_FUEL_1);
		if (num_bays < 1)
			NPCPhrase (NEED_STORAGE_1);
		if (GLOBAL_SIS (NumLanders) == 0)
			NPCPhrase (NEED_LANDERS_2);
		if (num_batts < 1)
			NPCPhrase (NEED_DYNAMOS_1);

		if (GLOBAL_SIS (ResUnits) >= 3000)
			NPCPhrase (IMPROVE_FLAGSHIP_WITH_RU);
		else
			NPCPhrase (GO_GET_MINERALS);
	}
	else
	{
		BYTE num_aliens = 0;
		COUNT FleetStrength;
		BOOLEAN HasMaximum;

		FleetStrength = CalculateEscortsWorth ();
		for (i = 0; i < NUM_AVAILABLE_RACES; ++i)
		{
			if (i != HUMAN_SHIP && CheckAlliance (i) == GOOD_GUY)
				++num_aliens;
		}

		HasMaximum = (num_thrusters == NUM_DRIVE_SLOTS
				&& num_jets == NUM_JET_SLOTS
				&& GLOBAL_SIS (CrewEnlisted) >= CREW_POD_CAPACITY * 3
				&& GLOBAL_SIS (FuelOnBoard) >= FUEL_TANK_CAPACITY * 3
				&& GLOBAL_SIS (NumLanders) >= 3
				&& num_batts >= 4 && num_guns >= 7 && num_defense >= 2);
		if (!HasMaximum && GET_GAME_STATE (CHMMR_BOMB_STATE) < 2)
			NPCPhrase (GOT_OK_FLAGSHIP);
		else
			NPCPhrase (GOT_AWESOME_FLAGSHIP);

		if (GET_GAME_STATE (CHMMR_BOMB_STATE) >= 2)
		{
			NPCPhrase (CHMMR_IMPROVED_BOMB);
			if (FleetStrength < 20000)
				NPCPhrase (MUST_ACQUIRE_AWESOME_FLEET);
			else
			{
				NPCPhrase (GOT_AWESOME_FLEET);
				if (!GET_GAME_STATE (TALKING_PET_ON_SHIP))
					NPCPhrase (MUST_ELIMINATE_URQUAN_GUARDS);
				else
					NPCPhrase (GO_DESTROY_SAMATRA);
			}
		}
		else if (num_aliens < 2)
			NPCPhrase (GO_ALLY_WITH_ALIENS);
		else
		{
			NPCPhrase (MADE_SOME_ALLIES);
			if (FleetStrength < 6000)
			{
				if (GLOBAL_SIS (ResUnits) >= 3000)
					NPCPhrase (BUY_COMBAT_SHIPS);
				else
					NPCPhrase (GET_SHIPS_BY_MINING_OR_ALLIANCE);
			}
			else
			{
				NPCPhrase (GOT_OK_FLEET);
				if (!HasMaximum)
				{
					NPCPhrase (IMPROVE_2);
					if (num_thrusters < NUM_DRIVE_SLOTS)
						NPCPhrase (NEED_THRUSTERS_2);
					if (num_jets < NUM_JET_SLOTS)
						NPCPhrase (NEED_TURN_2);
					if (num_guns < 7)
						NPCPhrase (NEED_GUNS_2);
					if (GLOBAL_SIS (CrewEnlisted) < CREW_POD_CAPACITY * 3)
						NPCPhrase (NEED_CREW_2);
					if (GLOBAL_SIS (FuelOnBoard) < FUEL_TANK_CAPACITY * 3)
						NPCPhrase (NEED_FUEL_2);
					if (GLOBAL_SIS (NumLanders) < 3)
						NPCPhrase (NEED_LANDERS_1);
					if (num_batts < 4)
						NPCPhrase (NEED_DYNAMOS_2);
					if (num_defense < 2)
						NPCPhrase (NEED_POINT);
				}
				else if (!GET_GAME_STATE (AWARE_OF_SAMATRA))
					NPCPhrase (GO_LEARN_ABOUT_URQUAN);
				else
				{
					NPCPhrase (KNOW_ABOUT_SAMATRA);
					if (!GET_GAME_STATE (UTWIG_BOMB))
						NPCPhrase (FIND_WAY_TO_DESTROY_SAMATRA);
					else if (GET_GAME_STATE (UTWIG_BOMB_ON_SHIP))
						NPCPhrase (MUST_INCREASE_BOMB_STRENGTH);
				}
			}
		}
	}
}

static void
TellMission (RESPONSE_REF R)
{
	RESPONSE_REF pstack[4];

	if (PLAYER_SAID (R, our_mission))
	{
		NPCPhrase (WHICH_MISSION);
		stack0 = 0;
		stack1 = 0;
		stack2 = 0;
		stack3 = 0;
	}
	else if (PLAYER_SAID (R, where_get_minerals))
	{
		NPCPhrase (GET_MINERALS);

		stack0 = 1;
	}
	else if (PLAYER_SAID (R, what_about_aliens))
	{
		NPCPhrase (ABOUT_ALIENS);

		stack1 = 1;
	}
	else if (PLAYER_SAID (R, what_do_now))
	{
		AnalyzeCondition ();

		stack2 = 1;
	}
	else if (PLAYER_SAID (R, what_about_urquan))
	{
		NPCPhrase (MUST_DEFEAT);

		stack3 = 1;
	}
	else if (PLAYER_SAID (R, enough_defeat))
	{
		NPCPhrase (OK_ENOUGH_DEFEAT);

		stack3 = 2;
	}

	switch (stack0)
	{
		case 0:
			pstack[0] = where_get_minerals;
			break;
		default:
			pstack[0] = 0;
			break;
	}
	switch (stack1)
	{
		case 0:
			pstack[1] = what_about_aliens;
			break;
		default:
			pstack[1] = 0;
			break;
	}
	switch (stack2)
	{
		case 0:
			pstack[2] = what_do_now;
			break;
		default:
			pstack[2] = 0;
			break;
	}
	switch (stack3)
	{
		case 0:
			pstack[3] = what_about_urquan;
			break;
		case 1:
			pstack[3] = how_defeat;
			break;
		default:
			pstack[3] = 0;
			break;
	}

	if (pstack[0])
		Response (pstack[0], TellMission);
	if (pstack[1])
		Response (pstack[1], TellMission);
	if (pstack[2])
		Response (pstack[2], TellMission);
	if (pstack[3])
	{
		if (stack3 == 1)
			Response (pstack[3], DefeatUrquan);
		else
			Response (pstack[3], TellMission);
	}

	Response (enough_mission, NeedInfo);
}

static void
TellStarBase (RESPONSE_REF R)
{
	RESPONSE_REF pstack[4];

	if (PLAYER_SAID (R, starbase_functions))
	{
		NPCPhrase (WHICH_FUNCTION);
		stack0 = 0;
		stack1 = 0;
		stack2 = 0;
		stack3 = 0;
	}
	else if (PLAYER_SAID (R, tell_me_about_fuel))
	{
		NPCPhrase (ABOUT_FUEL);

		stack1 = 1;
	}
	else if (PLAYER_SAID (R, tell_me_about_crew))
	{
		NPCPhrase (ABOUT_CREW);

		stack2 = 2;
	}
	else if (PLAYER_SAID (R, tell_me_about_modules))
	{
		NPCPhrase (ABOUT_MODULES);

		stack0 = 1;
	}
	else if (PLAYER_SAID (R, tell_me_about_ships))
	{
		NPCPhrase (ABOUT_SHIPS);

		stack2 = 1;
	}
	else if (PLAYER_SAID (R, tell_me_about_ru))
	{
		NPCPhrase (ABOUT_RU);

		stack3 = 1;
	}
	else if (PLAYER_SAID (R, tell_me_about_minerals))
	{
		NPCPhrase (ABOUT_MINERALS);

		stack3 = 2;
	}
	else if (PLAYER_SAID (R, tell_me_about_life))
	{
		NPCPhrase (ABOUT_LIFE);

		stack3 = 3;
	}

	switch (stack0)
	{
		case 0:
			pstack[0] = tell_me_about_modules;
			break;
		default:
			pstack[0] = 0;
			break;
	}
	switch (stack1)
	{
		case 0:
			pstack[1] = tell_me_about_fuel;
			break;
		default:
			pstack[1] = 0;
			break;
	}
	switch (stack2)
	{
		case 0:
			pstack[2] = tell_me_about_ships;
			break;
		case 1:
			pstack[2] = tell_me_about_crew;
			break;
		default:
			pstack[2] = 0;
			break;
	}
	switch (stack3)
	{
		case 0:
			pstack[3] = tell_me_about_ru;
			break;
		case 1:
			pstack[3] = tell_me_about_minerals;
			break;
		case 2:
			pstack[3] = tell_me_about_life;
			break;
		default:
			pstack[3] = 0;
			break;
	}

	if (pstack[0])
		Response (pstack[0], TellStarBase);
	if (pstack[1])
		Response (pstack[1], TellStarBase);
	if (pstack[2])
		Response (pstack[2], TellStarBase);
	if (pstack[3])
		Response (pstack[3], TellStarBase);

	Response (enough_starbase, NeedInfo);
}

static void NormalStarbase (RESPONSE_REF R);

static void
NeedInfo (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, need_info))
		NPCPhrase (WHAT_KIND_OF_INFO);
	else if (PLAYER_SAID (R, enough_starbase))
		NPCPhrase (OK_ENOUGH_STARBASE);
	else if (PLAYER_SAID (R, enough_history))
		NPCPhrase (OK_ENOUGH_HISTORY);
	else if (PLAYER_SAID (R, enough_mission))
		NPCPhrase (OK_ENOUGH_MISSION);

	Response (starbase_functions, TellStarBase);
	Response (history, TellHistory);
	Response (our_mission, TellMission);
	Response (no_need_info, NormalStarbase);
}

static BOOLEAN
DiscussDevices (BOOLEAN TalkAbout)
{
	COUNT i, VuxBeastIndex, PhraseIndex;
	BOOLEAN Undiscussed;

	if (TalkAbout)
		NPCPhrase (DEVICE_HEAD);
	PhraseIndex = 2;

	VuxBeastIndex = 0;
	Undiscussed = FALSE;
	for (i = 0; i < NUM_DEVICES; ++i)
	{
		RESPONSE_REF pStr;

		pStr = 0;
		switch (i)
		{
			case ROSY_SPHERE_DEVICE:
				if (GET_GAME_STATE (ROSY_SPHERE_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_ROSY_SPHERE))
				{
					pStr = ABOUT_SPHERE;
					SET_GAME_STATE (DISCUSSED_ROSY_SPHERE, TalkAbout);
				}
				break;
			case ARTIFACT_2_DEVICE:
				if (GET_GAME_STATE (WIMBLIS_TRIDENT_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_ARTIFACT_2))
				{
					pStr = ABOUT_ARTIFACT_2;
					SET_GAME_STATE (DISCUSSED_ARTIFACT_2, TalkAbout);
				}
				break;
			case ARTIFACT_3_DEVICE:
				if (GET_GAME_STATE (GLOWING_ROD_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_ARTIFACT_3))
				{
					pStr = ABOUT_ARTIFACT_3;
					SET_GAME_STATE (DISCUSSED_ARTIFACT_3, TalkAbout);
				}
				break;
			case SUN_EFFICIENCY_DEVICE:
				if (GET_GAME_STATE (SUN_DEVICE_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_SUN_EFFICIENCY))
				{
					pStr = ABOUT_SUN;
					SET_GAME_STATE (DISCUSSED_SUN_EFFICIENCY, TalkAbout);
				}
				break;
			case UTWIG_BOMB_DEVICE:
				if (GET_GAME_STATE (UTWIG_BOMB_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_UTWIG_BOMB))
				{
					pStr = ABOUT_BOMB;
					SET_GAME_STATE (DISCUSSED_UTWIG_BOMB, TalkAbout);
				}
				break;
			case ULTRON_0_DEVICE:
				if (GET_GAME_STATE (ULTRON_CONDITION) == 1
						&& !GET_GAME_STATE (DISCUSSED_ULTRON))
				{
					pStr = ABOUT_ULTRON_0;
					SET_GAME_STATE (DISCUSSED_ULTRON, TalkAbout);
				}
				break;
			case ULTRON_1_DEVICE:
				if (GET_GAME_STATE (ULTRON_CONDITION) == 2
						&& !GET_GAME_STATE (DISCUSSED_ULTRON))
				{
					pStr = ABOUT_ULTRON_1;
					SET_GAME_STATE (DISCUSSED_ULTRON, TalkAbout);
				}
				break;
			case ULTRON_2_DEVICE:
				if (GET_GAME_STATE (ULTRON_CONDITION) == 3
						&& !GET_GAME_STATE (DISCUSSED_ULTRON))
				{
					pStr = ABOUT_ULTRON_2;
					SET_GAME_STATE (DISCUSSED_ULTRON, TalkAbout);
				}
				break;
			case ULTRON_3_DEVICE:
				if (GET_GAME_STATE (ULTRON_CONDITION) == 4
						&& !GET_GAME_STATE (DISCUSSED_ULTRON))
				{
					pStr = ABOUT_ULTRON_3;
					SET_GAME_STATE (DISCUSSED_ULTRON, TalkAbout);
				}
				break;
			case MAIDENS_DEVICE:
				if (GET_GAME_STATE (MAIDENS_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_MAIDENS))
				{
					pStr = ABOUT_MAIDENS;
					SET_GAME_STATE (DISCUSSED_MAIDENS, TalkAbout);
				}
				break;
			case TALKING_PET_DEVICE:
				if (GET_GAME_STATE (TALKING_PET_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_TALKING_PET))
				{
					pStr = ABOUT_TALKPET;
					SET_GAME_STATE (DISCUSSED_TALKING_PET, TalkAbout);
				}
				break;
			case AQUA_HELIX_DEVICE:
				if (GET_GAME_STATE (AQUA_HELIX_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_AQUA_HELIX))
				{
					pStr = ABOUT_HELIX;
					SET_GAME_STATE (DISCUSSED_AQUA_HELIX, TalkAbout);
				}
				break;
			case CLEAR_SPINDLE_DEVICE:
				if (GET_GAME_STATE (CLEAR_SPINDLE_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_CLEAR_SPINDLE))
				{
					pStr = ABOUT_SPINDLE;
					SET_GAME_STATE (DISCUSSED_CLEAR_SPINDLE, TalkAbout);
				}
				break;
			case UMGAH_HYPERWAVE_DEVICE:
				if (GET_GAME_STATE (UMGAH_BROADCASTERS_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_UMGAH_HYPERWAVE))
				{
					pStr = ABOUT_UCASTER;
					SET_GAME_STATE (DISCUSSED_UMGAH_HYPERWAVE, TalkAbout);
				}
				break;
#ifdef NEVER
			case DATA_PLATE_1_DEVICE:
				if (GET_GAME_STATE (DATA_PLATE_1_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_DATA_PLATE_1))
				{
					pStr = ABOUT_DATAPLATE_1;
					SET_GAME_STATE (DISCUSSED_DATA_PLATE_1, TalkAbout);
				}
				break;
			case DATA_PLATE_2_DEVICE:
				if (GET_GAME_STATE (DATA_PLATE_2_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_DATA_PLATE_2))
				{
					pStr = ABOUT_DATAPLATE_2;
					SET_GAME_STATE (DISCUSSED_DATA_PLATE_2, TalkAbout);
				}
				break;
			case DATA_PLATE_3_DEVICE:
				if (GET_GAME_STATE (DATA_PLATE_3_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_DATA_PLATE_3))
				{
					pStr = ABOUT_DATAPLATE_3;
					SET_GAME_STATE (DISCUSSED_DATA_PLATE_3, TalkAbout);
				}
				break;
#endif /* NEVER */
			case TAALO_PROTECTOR_DEVICE:
				if (GET_GAME_STATE (TAALO_PROTECTOR_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_TAALO_PROTECTOR))
				{
					pStr = ABOUT_SHIELD;
					SET_GAME_STATE (DISCUSSED_TAALO_PROTECTOR, TalkAbout);
				}
				break;
			case EGG_CASING0_DEVICE:
				if (GET_GAME_STATE (EGG_CASE0_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_EGG_CASING0))
				{
					pStr = ABOUT_EGGCASE_0;
					SET_GAME_STATE (DISCUSSED_EGG_CASING0, TalkAbout);
					SET_GAME_STATE (DISCUSSED_EGG_CASING1, TalkAbout);
					SET_GAME_STATE (DISCUSSED_EGG_CASING2, TalkAbout);
				}
				break;
			case EGG_CASING1_DEVICE:
				if (GET_GAME_STATE (EGG_CASE1_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_EGG_CASING1))
				{
					pStr = ABOUT_EGGCASE_0;
					SET_GAME_STATE (DISCUSSED_EGG_CASING0, TalkAbout);
					SET_GAME_STATE (DISCUSSED_EGG_CASING1, TalkAbout);
					SET_GAME_STATE (DISCUSSED_EGG_CASING2, TalkAbout);
				}
				break;
			case EGG_CASING2_DEVICE:
				if (GET_GAME_STATE (EGG_CASE2_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_EGG_CASING2))
				{
					pStr = ABOUT_EGGCASE_0;
					SET_GAME_STATE (DISCUSSED_EGG_CASING0, TalkAbout);
					SET_GAME_STATE (DISCUSSED_EGG_CASING1, TalkAbout);
					SET_GAME_STATE (DISCUSSED_EGG_CASING2, TalkAbout);
				}
				break;
			case SYREEN_SHUTTLE_DEVICE:
				if (GET_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_SYREEN_SHUTTLE))
				{
					pStr = ABOUT_SHUTTLE;
					SET_GAME_STATE (DISCUSSED_SYREEN_SHUTTLE, TalkAbout);
				}
				break;
			case VUX_BEAST_DEVICE:
				if (GET_GAME_STATE (VUX_BEAST_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_VUX_BEAST))
				{
					pStr = ABOUT_VUXBEAST0;
					SET_GAME_STATE (DISCUSSED_VUX_BEAST, TalkAbout);
				}
				break;
			case DESTRUCT_CODE_DEVICE:
				if (GET_GAME_STATE (DESTRUCT_CODE_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_DESTRUCT_CODE))
				{
					pStr = ABOUT_DESTRUCT;
					SET_GAME_STATE (DISCUSSED_DESTRUCT_CODE, TalkAbout);
				}
				break;
			case PORTAL_SPAWNER_DEVICE:
				if (GET_GAME_STATE (PORTAL_SPAWNER_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_PORTAL_SPAWNER))
				{
					pStr = ABOUT_PORTAL;
					SET_GAME_STATE (DISCUSSED_PORTAL_SPAWNER, TalkAbout);
				}
				break;
			case URQUAN_WARP_DEVICE:
				if (GET_GAME_STATE (PORTAL_KEY_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_URQUAN_WARP))
				{
					pStr = ABOUT_WARPPOD;
					SET_GAME_STATE (DISCUSSED_URQUAN_WARP, TalkAbout);
				}
				break;
			case BURVIX_HYPERWAVE_DEVICE:
				if (GET_GAME_STATE (BURV_BROADCASTERS_ON_SHIP)
						&& !GET_GAME_STATE (DISCUSSED_BURVIX_HYPERWAVE))
				{
					pStr = ABOUT_BCASTER;
					SET_GAME_STATE (DISCUSSED_BURVIX_HYPERWAVE, TalkAbout);
				}
				break;
		}

		if (pStr)
		{
			if (TalkAbout)
			{
				if (PhraseIndex > 2)
					NPCPhrase (BETWEEN_DEVICES);
				NPCPhrase (pStr);
				if (pStr == ABOUT_VUXBEAST0)
				{
					VuxBeastIndex = ++PhraseIndex;
					NPCPhrase (ABOUT_VUXBEAST1);
				}
			}
			PhraseIndex += 2;
		}
	}

	if (TalkAbout)
	{
		NPCPhrase (DEVICE_TAIL);

		if (VuxBeastIndex)
		{
			// Run all tracks upto the Vux Beast scientist's report
			AlienTalkSegue (VuxBeastIndex - 1);
			// Disable Commander's speech animation and run the report
			EnableTalkingAnim (FALSE);
			AlienTalkSegue (VuxBeastIndex);
			// Enable Commander's speech animation and run the rest
			EnableTalkingAnim (TRUE);
			AlienTalkSegue ((COUNT)~0);
		}
	}

	return (PhraseIndex > 2);
}

static BOOLEAN
CheckTiming (COUNT month_index, COUNT day_index)
{
	COUNT mi, year_index;
	BYTE days_in_month[12] =
	{
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	};

	mi = GET_GAME_STATE (STARBASE_MONTH);
	year_index = START_YEAR;

	day_index += GET_GAME_STATE (STARBASE_DAY);
	while (day_index > days_in_month[mi - 1])
	{
		day_index -= days_in_month[mi - 1];
		if (++mi > 12)
		{
			mi = 1;
			++year_index;
		}
	}

	month_index += mi;
	year_index += (month_index - 1) / 12;
	month_index = ((month_index - 1) % 12) + 1;

	return (year_index < GLOBAL (GameClock.year_index)
			|| (year_index == GLOBAL (GameClock.year_index)
			&& (month_index < GLOBAL (GameClock.month_index)
			|| (month_index == GLOBAL (GameClock.month_index)
			&& day_index < GLOBAL (GameClock.day_index)))));
}

static void
CheckBulletins (BOOLEAN Repeat)
{
	RESPONSE_REF pIntro;
	BYTE b0;
	DWORD BulletinMask;
	COUNT CrewSold = MAKE_WORD(
		GET_GAME_STATE(CREW_SOLD_TO_DRUUGE0),
		GET_GAME_STATE(CREW_SOLD_TO_DRUUGE1));

	if (Repeat)
		BulletinMask = CurBulletinMask ^ 0xFFFFFFFFL;
	else
		BulletinMask = GET_GAME_STATE (STARBASE_BULLETS);

	pIntro = 0;
	for (b0 = 0; b0 < 32; ++b0)
	{
		if (!(BulletinMask & (1L << b0)))
		{
			RESPONSE_REF pStr;

			pStr = 0;
			switch (b0)
			{
				case 0:
					if (CheckAlliance (SPATHI_SHIP) == GOOD_GUY)
					{
						pStr = STARBASE_BULLETIN_1;
					}
					break;
				case 1:
					if (CheckAlliance (ZOQFOTPIK_SHIP) == GOOD_GUY)
					{
						pStr = STARBASE_BULLETIN_2;
					}
					break;
				case 2:
					if (CheckAlliance (SUPOX_SHIP) == GOOD_GUY)
					{
						pStr = STARBASE_BULLETIN_3;
					}
					break;
				case 3:
					if (CheckAlliance (UTWIG_SHIP) == GOOD_GUY)
					{
						pStr = STARBASE_BULLETIN_4;
					}
					break;
				case 4:
					if (CheckAlliance (ORZ_SHIP) == GOOD_GUY)
					{
						pStr = STARBASE_BULLETIN_5;
					}
					break;
				case 5:
					if (GET_GAME_STATE (ARILOU_MANNER) == 2)
						BulletinMask |= 1L << b0;
					else if (GET_GAME_STATE (PORTAL_SPAWNER)
							&& (Repeat || EscortFeasibilityStudy (
									ARILOU_SHIP)))
					{
#define NUM_GIFT_ARILOUS 3
						pStr = STARBASE_BULLETIN_6;
						if (!Repeat)
							AddEscortShips (ARILOU_SHIP, NUM_GIFT_ARILOUS);
					}
					break;
				case 6:
					if (GET_GAME_STATE (ZOQFOT_DISTRESS) == 1)
					{
						pStr = STARBASE_BULLETIN_7;
					}
					break;
				case 7:
					if (GET_GAME_STATE (MET_MELNORME))
						BulletinMask |= 1L << b0;
					else if (CheckTiming (IF_EASY(3, 1), 0))
					{
						pStr = STARBASE_BULLETIN_8;
					}
					break;
				case 8:
					if (GET_GAME_STATE (MET_MELNORME))
						BulletinMask |= 1L << b0;
					else if (CheckTiming (IF_EASY(6, 3), 0))
					{
						pStr = STARBASE_BULLETIN_9;
					}
					break;
				case 9:
					if (GET_GAME_STATE (FOUND_PLUTO_SPATHI))
						BulletinMask |= 1L << b0;
					else if (CheckTiming (0, 7))
					{
						pStr = STARBASE_BULLETIN_10;
					}
					break;
				case 10:
					if (GET_GAME_STATE (SPATHI_SHIELDED_SELVES))
					{
						pStr = STARBASE_BULLETIN_11;
					}
					break;
				case 11:
					if (GET_GAME_STATE (ZOQFOT_HOME_VISITS)
							|| GET_GAME_STATE (ZOQFOT_GRPOFFS))
						BulletinMask |= 1L << b0;
					else if (CheckTiming (0, IF_EASY(42, 21)))
					{
						pStr = STARBASE_BULLETIN_12;
					}
					break;
				case 12:
					if (CheckAlliance (CHMMR_SHIP) == GOOD_GUY)
					{
						pStr = STARBASE_BULLETIN_13;
					}
					break;
				case 13:
					if (CheckAlliance (SHOFIXTI_SHIP) == GOOD_GUY)
					{
						pStr = STARBASE_BULLETIN_14;
					}
					break;
				case 14:
					if (GET_GAME_STATE (PKUNK_MISSION))
					{
						pStr = STARBASE_BULLETIN_15;
					}
					break;
				case 15:
					if (GET_GAME_STATE (DESTRUCT_CODE_ON_SHIP))
						BulletinMask |= 1L << b0;
					else if (CheckTiming (7, 0))
					{
						pStr = STARBASE_BULLETIN_16;
					}
					break;
				case 16:
					break;
				case 17:
					if (GET_GAME_STATE (YEHAT_ABSORBED_PKUNK))
					{
						pStr = STARBASE_BULLETIN_18;
					}
					break;
				case 18:
					if (GET_GAME_STATE (CHMMR_BOMB_STATE) == 2)
					{
						pStr = STARBASE_BULLETIN_19;
					}
					break;
				case 19:
					break;
				case 20:
					break;
				case 21:
					if (GET_GAME_STATE (ZOQFOT_DISTRESS) == 2)
					{
						pStr = STARBASE_BULLETIN_22;
					}
					break;
				case 22:
					break;
				case 23:
					break;
				case 24:
					break;
				case 25:
					break;
				case 26:
				{
					COUNT crew_sold = CrewSold;

					if (crew_sold > MIN_SOLD)
						BulletinMask |= 1L << b0;
					else if (crew_sold)
					{
						pStr = STARBASE_BULLETIN_27;
					}
					break;
				}
				case 27:
				{
					COUNT crew_sold = CrewSold;

					if (crew_sold > MAX_SOLD)
						BulletinMask |= 1L << b0;
					else if (crew_sold > MIN_SOLD)
					{
						pStr = STARBASE_BULLETIN_28;
					}
					break;
				}
				case 28:
				{
					COUNT crew_bought;

					crew_bought = MAKE_WORD (
							GET_GAME_STATE (CREW_PURCHASED0),
							GET_GAME_STATE (CREW_PURCHASED1)
							);
					if (crew_bought >= CREW_EXPENSE_THRESHOLD)
					{
						pStr = STARBASE_BULLETIN_29;
					}
					break;
				}
				case 29:
					if (CrewSold > MAX_SOLD)
					{
						pStr = STARBASE_BULLETIN_30;
					}
					break;
				case 30:
					break;
				case 31:
					break;
			}

			if (pStr)
			{
				if (pIntro)
					NPCPhrase (BETWEEN_BULLETINS);
				else if (Repeat)
					pIntro = BEFORE_WE_GO_ON_1;
				else
				{
					switch ((BYTE)TFB_Random () % 7)
					{
						case 0:
							pIntro = BEFORE_WE_GO_ON_1;
							break;
						case 1:
							pIntro = BEFORE_WE_GO_ON_2;
							break;
						case 2:
							pIntro = BEFORE_WE_GO_ON_3;
							break;
						case 3:
							pIntro = BEFORE_WE_GO_ON_4;
							break;
						case 4:
							pIntro = BEFORE_WE_GO_ON_5;
							break;
						case 5:
							pIntro = BEFORE_WE_GO_ON_6;
							break;
						default:
							pIntro = BEFORE_WE_GO_ON_7;
							break;
					}

					NPCPhrase (pIntro);
				}

				NPCPhrase (pStr);
				CurBulletinMask |= 1L << b0;
			}
		}
	}

	if (pIntro == 0 && GET_GAME_STATE (STARBASE_VISITED))
		NPCPhrase (RETURN_HELLO);
	else if (!Repeat)
		SET_GAME_STATE (STARBASE_BULLETS, BulletinMask);
}

static void
NormalStarbase (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, no_need_info))
		NPCPhrase (OK_NO_NEED_INFO);
	else if (PLAYER_SAID (R, new_devices))
		DiscussDevices (TRUE);
	else if (PLAYER_SAID (R, repeat_bulletins))
		CheckBulletins (TRUE);
	else if (R == 0)
	{
		if (GET_GAME_STATE (MOONBASE_ON_SHIP))
		{
			NPCPhrase (STARBASE_IS_READY);
			DeltaSISGauges (0, 0, 2500);
			if(optInfiniteRU){
				oldRU = 2500;
			}
			SET_GAME_STATE (STARBASE_MONTH,
					GLOBAL (GameClock.month_index));
			SET_GAME_STATE (STARBASE_DAY,
					GLOBAL (GameClock.day_index));
		}
		else if (GET_GAME_STATE (STARBASE_VISITED))
		{
			CheckBulletins (FALSE);
		}
		else
		{
			// XXX TODO: This can be simplified now.
			RESPONSE_REF pStr = 0;

			switch ((BYTE)TFB_Random () & 7)
			{
				case 0: pStr = NORMAL_HELLO_A; break;
				case 1: pStr = NORMAL_HELLO_B; break;
				case 2: pStr = NORMAL_HELLO_C; break;
				case 3: pStr = NORMAL_HELLO_D; break;
				case 4: pStr = NORMAL_HELLO_E; break;
				case 5: pStr = NORMAL_HELLO_F; break;
				case 6: pStr = NORMAL_HELLO_G; break;
				case 7: pStr = NORMAL_HELLO_H; break;
			}
			NPCPhrase (pStr);
			CheckBulletins (FALSE);
		}

		SET_GAME_STATE (STARBASE_VISITED, 1);
	}

	if (GLOBAL_SIS (TotalElementMass))
		Response (have_minerals, SellMinerals);
	if (DiscussDevices (FALSE))
		Response (new_devices, NormalStarbase);
	if (CurBulletinMask)
		Response (repeat_bulletins, NormalStarbase);
	Response (need_info, NeedInfo);
	Response (goodbye_commander, ByeBye);
}

static void
SellMinerals (RESPONSE_REF R)
{
	COUNT i, total;
	BOOLEAN Sleepy;
	RESPONSE_REF pStr1 = 0;
	RESPONSE_REF pStr2 = 0;

	total = 0;
	Sleepy = TRUE;
	for (i = 0; i < NUM_ELEMENT_CATEGORIES; ++i)
	{
		COUNT amount;
		DWORD TimeIn = 0;

		if (i == 0)
		{
			DrawCargoStrings ((BYTE)~0, (BYTE)~0);
			SleepThread (ONE_SECOND / 2);
			TimeIn = GetTimeCounter ();
			DrawCargoStrings ((BYTE)0, (BYTE)0);
		}
		else if (Sleepy)
		{
			DrawCargoStrings ((BYTE)(i - 1), (BYTE)i);
			TimeIn = GetTimeCounter ();
		}

		if ((amount = GLOBAL_SIS (ElementAmounts[i])) != 0)
		{
			total = amount * GLOBAL (ElementWorth[i]);
			do
			{
				if (!Sleepy || AnyButtonPress (TRUE) ||
						(GLOBAL (CurrentActivity) & CHECK_ABORT))
				{
					Sleepy = FALSE;
					GLOBAL_SIS (ElementAmounts[i]) = 0;
					GLOBAL_SIS (TotalElementMass) -= amount;
					DeltaSISGauges (0, 0, total);
					break;
				}
				
				--GLOBAL_SIS (ElementAmounts[i]);
				--GLOBAL_SIS (TotalElementMass);
				TaskSwitch ();
				TimeIn = GetTimeCounter ();
				DrawCargoStrings ((BYTE)i, (BYTE)i);
				ShowRemainingCapacity ();
				DeltaSISGauges (0, 0, GLOBAL (ElementWorth[i]));
			} while (--amount);
		}
		if (Sleepy) {
			SleepThreadUntil (TimeIn + (ONE_SECOND / 4));
			TimeIn = GetTimeCounter ();
		}
	}
	SleepThread (ONE_SECOND / 2);

	ClearSISRect (DRAW_SIS_DISPLAY);
// DrawStorageBays (FALSE);

	if (total < 1000)
	{
		total = GET_GAME_STATE (LIGHT_MINERAL_LOAD);
		switch (total++)
		{
			case 0: pStr1 = LIGHT_LOAD_A; break;
			case 1: pStr1 = LIGHT_LOAD_B; break;
			case 2:
				// There are two separate sound samples in this case.
				pStr1 = LIGHT_LOAD_C0;
				pStr2 = LIGHT_LOAD_C1;
				break;
			case 3: pStr1 = LIGHT_LOAD_D; break;
			case 4: pStr1 = LIGHT_LOAD_E; break;
			case 5: pStr1 = LIGHT_LOAD_F; break;
			case 6: --total;
				pStr1 = LIGHT_LOAD_G;
				break;
		}
		SET_GAME_STATE (LIGHT_MINERAL_LOAD, total);
	}
	else if (total < 2500)
	{
		total = GET_GAME_STATE (MEDIUM_MINERAL_LOAD);
		switch (total++)
		{
			case 0: pStr1 = MEDIUM_LOAD_A; break;
			case 1: pStr1 = MEDIUM_LOAD_B; break;
			case 2: pStr1 = MEDIUM_LOAD_C; break;
			case 3: pStr1 = MEDIUM_LOAD_D; break;
			case 4: pStr1 = MEDIUM_LOAD_E; break;
			case 5: pStr1 = MEDIUM_LOAD_F; break;
			case 6:
				--total;
				pStr1 = MEDIUM_LOAD_G;
				break;
		}
		SET_GAME_STATE (MEDIUM_MINERAL_LOAD, total);
	}
	else
	{
		total = GET_GAME_STATE (HEAVY_MINERAL_LOAD);
		switch (total++)
		{
			case 0: pStr1 = HEAVY_LOAD_A; break;
			case 1: pStr1 = HEAVY_LOAD_B; break;
			case 2: pStr1 = HEAVY_LOAD_C; break;
			case 3: pStr1 = HEAVY_LOAD_D; break;
			case 4: pStr1 = HEAVY_LOAD_E; break;
			case 5: pStr1 = HEAVY_LOAD_F; break;
			case 6:
				--total;
				pStr1 = HEAVY_LOAD_G;
				break;
		}
		SET_GAME_STATE (HEAVY_MINERAL_LOAD, total);
	}

	NPCPhrase (pStr1);
	if (pStr2 != (RESPONSE_REF) 0)
		NPCPhrase (pStr2);

	NormalStarbase (R);
}

static void
Intro (void)
{
	NormalStarbase (0);
}

static COUNT
uninit_starbase (void)
{
	luaUqm_comm_uninit ();
	return (0);
}

static void
post_starbase_enc (void)
{
	SET_GAME_STATE (MOONBASE_ON_SHIP, 0);
	if (GET_GAME_STATE (CHMMR_BOMB_STATE) == 2)
	{
		SET_GAME_STATE (CHMMR_BOMB_STATE, 3);
	}
}

LOCDATA*
init_starbase_comm ()
{
	static LOCDATA commander_desc;
	LOCDATA *retval;

	commander_desc = RES_BOOL(commander_desc_orig, commander_desc_hd);

	commander_desc.init_encounter_func = Intro;
	commander_desc.post_encounter_func = post_starbase_enc;
	commander_desc.uninit_encounter_func = uninit_starbase;

	luaUqm_comm_init (NULL, NULL_RESOURCE);
			// Initialise Lua for string interpolation. This will be
			// generalised in the future.

	commander_desc.AlienTextWidth = RES_SIS_SCALE(143); // JMS_GFX
	commander_desc.AlienTextBaseline.x = RES_SIS_SCALE(164); // JMS_GFX
	commander_desc.AlienTextBaseline.y = RES_SIS_SCALE(20); // JMS_GFX

	// use alternate Starbase track if available
	commander_desc.AlienAltSongRes = STARBASE_ALT_MUSIC;
	commander_desc.AlienSongFlags |= LDASF_USE_ALTERNATE;

	CurBulletinMask = 0;
	setSegue (Segue_peace);
	retval = &commander_desc;

	return (retval);
}
