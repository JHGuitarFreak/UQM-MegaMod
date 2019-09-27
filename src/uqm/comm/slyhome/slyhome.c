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
#include "resinst.h"
#include "strings.h"

#include "uqm/lua/luacomm.h"
#include "uqm/gameev.h"


static LOCDATA slylandro_desc =
{
	SLYLANDRO_HOME_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	SLYLANDRO_PMAP_ANIM, /* AlienFrame */
	SLYLANDRO_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	SLYLANDRO_COLOR_MAP, /* AlienColorMap */
	SLYLANDRO_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	SLYLANDRO_CONVERSATION_PHRASES, /* PlayerPhrases */
	13, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			0, /* StartIndex */
			5, /* NumFrames */
			RANDOM_ANIM | COLORXFORM_ANIM, /* AnimFlags */
			ONE_SECOND / 8, ONE_SECOND * 5 / 8, /* FrameRate */
			ONE_SECOND / 8, ONE_SECOND * 5 / 8, /* RestartRate */
			0, /* BlockMask */
		},
		{
			1, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			6, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			11, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			16, /* StartIndex */
			6, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			22, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND / 15, 0, /* RestartRate */
			0, /* BlockMask */
		},
		{
			30, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 8) | (1 << 9), /* BlockMask */
		},
		{
			39, /* StartIndex */
			4, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			43, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 6), /* BlockMask */
		},
		{
			48, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 6), /* BlockMask */
		},
		{
			54, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 12), /* BlockMask */
		},
		{
			60, /* StartIndex */
			7, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 12), /* BlockMask */
		},
		{
			67, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 10) | (1 << 11), /* BlockMask */
		},
	},
	{ /* AlienTransitionDesc - empty */
		0, /* StartIndex */
		0, /* NumFrames */
		0, /* AnimFlags */
		0, 0, /* FrameRate */
		0, 0, /* RestartRate */
		0, /* BlockMask */
	},
	{ /* AlienTalkDesc - empty */
		0, /* StartIndex */
		0, /* NumFrames */
		0, /* AnimFlags */
		0, 0, /* FrameRate */
		0, 0, /* RestartRate */
		0, /* BlockMask */
	},
	NULL, /* AlienNumberSpeech - none */
	/* Filler for loaded resources */
	NULL, NULL, NULL,
	NULL,
	NULL,
};

static void
ExitConversation (RESPONSE_REF R)
{
	(void) R;  // ignored
	setSegue (Segue_peace);

	switch (GET_GAME_STATE (SLYLANDRO_HOME_VISITS))
	{
		case 1:
			NPCPhrase (GOODBYE_1);
			break;
		default:
			NPCPhrase (GOODBYE_2);
			break;
	}
}

static void HomeWorld (RESPONSE_REF R);

static void
HumanInfo (RESPONSE_REF R)
{
	BYTE InfoLeft;

	if (PLAYER_SAID (R, happy_to_tell_more))
	{
		NPCPhrase (TELL_MORE);

		SET_GAME_STATE (SLYLANDRO_STACK4, 1);
	}
	else if (PLAYER_SAID (R, would_you_like_to_know_more))
	{
		NPCPhrase (YES_TELL_MORE);
	}
	else if (PLAYER_SAID (R, we_come_from_earth))
	{
		NPCPhrase (OK_EARTH);

		SET_GAME_STATE (SLYLANDRO_KNOW_EARTH, 1);
	}
	else if (PLAYER_SAID (R, we_explore))
	{
		NPCPhrase (OK_EXPLORE);

		SET_GAME_STATE (SLYLANDRO_KNOW_EXPLORE, 1);
	}
	else if (PLAYER_SAID (R, we_fight_urquan))
	{
		NPCPhrase (URQUAN_NICE_GUYS);

		SET_GAME_STATE (SLYLANDRO_KNOW_URQUAN, 1);
	}
	else if (PLAYER_SAID (R, not_same_urquan))
	{
		NPCPhrase (PERSONALITY_CHANGE);

		SET_GAME_STATE (SLYLANDRO_KNOW_URQUAN, 2);
	}
	else if (PLAYER_SAID (R, we_gather))
	{
		NPCPhrase (MAYBE_INTERESTED);

		SET_GAME_STATE (SLYLANDRO_KNOW_GATHER, 1);
	}

	InfoLeft = FALSE;
	if (GET_GAME_STATE (SLYLANDRO_KNOW_URQUAN) == 1)
	{
		InfoLeft = TRUE;
		Response (not_same_urquan, HumanInfo);
	}
	if (!GET_GAME_STATE (SLYLANDRO_KNOW_EARTH))
	{
		InfoLeft = TRUE;
		Response (we_come_from_earth, HumanInfo);
	}
	if (!GET_GAME_STATE (SLYLANDRO_KNOW_EXPLORE))
	{
		InfoLeft = TRUE;
		Response (we_explore, HumanInfo);
	}
	if (!GET_GAME_STATE (SLYLANDRO_KNOW_URQUAN))
	{
		InfoLeft = TRUE;
		Response (we_fight_urquan, HumanInfo);
	}
	if (!GET_GAME_STATE (SLYLANDRO_KNOW_GATHER))
	{
		InfoLeft = TRUE;
		Response (we_gather, HumanInfo);
	}

	Response (enough_about_me, HomeWorld);
	if (!InfoLeft)
	{
		SET_GAME_STATE (SLYLANDRO_STACK4, 2);
	}
}

static void
SlylandroInfo (RESPONSE_REF R)
{
	BYTE InfoLeft;

	if (PLAYER_SAID (R, like_more_about_you))
	{
		NPCPhrase (SURE_KNOW_WHAT);
	}
	else if (PLAYER_SAID (R, what_about_home))
	{
		NPCPhrase (ABOUT_HOME);

		DISABLE_PHRASE (what_about_home);
	}
	else if (PLAYER_SAID (R, what_about_culture))
	{
		NPCPhrase (ABOUT_CULTURE);

		DISABLE_PHRASE (what_about_culture);
	}
	else if (PLAYER_SAID (R, what_about_history))
	{
		NPCPhrase (ABOUT_HISTORY);

		DISABLE_PHRASE (what_about_history);
	}
	else if (PLAYER_SAID (R, what_about_biology))
	{
		NPCPhrase (ABOUT_BIOLOGY);

		DISABLE_PHRASE (what_about_biology);
	}

	InfoLeft = FALSE;
	if (PHRASE_ENABLED (what_about_home))
	{
		InfoLeft = TRUE;
		Response (what_about_home, SlylandroInfo);
	}
	if (PHRASE_ENABLED (what_about_culture))
	{
		InfoLeft = TRUE;
		Response (what_about_culture, SlylandroInfo);
	}
	if (PHRASE_ENABLED (what_about_history))
	{
		InfoLeft = TRUE;
		Response (what_about_history, SlylandroInfo);
	}
	if (PHRASE_ENABLED (what_about_biology))
	{
		InfoLeft = TRUE;
		Response (what_about_biology, SlylandroInfo);
	}

	Response (enough_info, HomeWorld);
	if (!InfoLeft)
	{
		DISABLE_PHRASE (like_more_about_you);
	}
}

static void
FixBug (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, think_about_rep_priorities))
		NPCPhrase (UH_OH);
	else if (PLAYER_SAID (R, hunt_them_down))
	{
		NPCPhrase (GROW_TOO_FAST);

		DISABLE_PHRASE (hunt_them_down);
	}
	else if (PLAYER_SAID (R, sue_melnorme))
	{
		NPCPhrase (SIGNED_WAIVER);

		DISABLE_PHRASE (sue_melnorme);
	}
	else if (PLAYER_SAID (R, recall_signal))
	{
		NPCPhrase (NOT_THIS_MODEL);

		DISABLE_PHRASE (recall_signal);
	}

	if (PHRASE_ENABLED (hunt_them_down))
		Response (hunt_them_down, FixBug);
	if (PHRASE_ENABLED (sue_melnorme))
		Response (sue_melnorme, FixBug);
	if (PHRASE_ENABLED (recall_signal))
		Response (recall_signal, FixBug);
	Response (mega_self_destruct, HomeWorld);
}

static void
ProbeBug (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, probe_has_bug))
		NPCPhrase (NO_IT_DOESNT);
	else if (PLAYER_SAID (R, tell_me_about_rep_2))
	{
		NPCPhrase (REP_NO_PROBLEM);

		DISABLE_PHRASE (tell_me_about_rep_2);
	}
	else if (PLAYER_SAID (R, what_about_rep_priorities))
	{
		NPCPhrase (MAXIMUM_SO_WHAT);

		DISABLE_PHRASE (what_about_rep_priorities);
	}
	else if (PLAYER_SAID (R, tell_me_about_attack))
	{
		NPCPhrase (ATTACK_NO_PROBLEM);

		DISABLE_PHRASE (tell_me_about_attack);
	}

	if (PHRASE_ENABLED (tell_me_about_rep_2))
		Response (tell_me_about_rep_2, ProbeBug);
	else if (PHRASE_ENABLED (what_about_rep_priorities))
		Response (what_about_rep_priorities, ProbeBug);
	else
	{
		Response (think_about_rep_priorities, FixBug);
	}
	if (PHRASE_ENABLED (tell_me_about_attack))
		Response (tell_me_about_attack, ProbeBug);
}

static void ProbeInfo (RESPONSE_REF R);

static void
ProbeFunction (RESPONSE_REF R)
{
	BYTE LastStack;
	RESPONSE_REF pStr[2];

	LastStack = 0;
	pStr[0] = pStr[1] = 0;
	if (PLAYER_SAID (R, talk_more_probe_attack))
	{
		NPCPhrase (NO_PROBLEM_BUT_SURE);
	}
	else if (PLAYER_SAID (R, tell_me_about_basics))
	{
		NPCPhrase (BASIC_COMMANDS);

		SET_GAME_STATE (PLAYER_KNOWS_PROGRAM, 1);
		DISABLE_PHRASE (tell_basics_again);
	}
	else if (PLAYER_SAID (R, tell_basics_again))
	{
		NPCPhrase (OK_BASICS_AGAIN);

		DISABLE_PHRASE (tell_basics_again);
	}
	else if (PLAYER_SAID (R, what_effect))
	{
		NPCPhrase (AFFECTS_BEHAVIOR);

		SET_GAME_STATE (PLAYER_KNOWS_EFFECTS, 1);
		DISABLE_PHRASE (what_effect);
	}
	else if (PLAYER_SAID (R, tell_me_about_rep_1))
	{
		NPCPhrase (ABOUT_REP);

		LastStack = 2;
		SET_GAME_STATE (SLYLANDRO_STACK8, 3);
	}
	else if (PLAYER_SAID (R, what_set_priority))
	{
		NPCPhrase (MAXIMUM);

		SET_GAME_STATE (PLAYER_KNOWS_PRIORITY, 1);
		DISABLE_PHRASE (what_set_priority);
	}
	else if (PLAYER_SAID (R, how_does_probe_defend))
	{
		NPCPhrase (ONLY_SELF_DEFENSE);

		LastStack = 1;
		SET_GAME_STATE (SLYLANDRO_STACK9, 1);
	}
	else if (PLAYER_SAID (R, combat_behavior))
	{
		NPCPhrase (MISSILE_BATTERIES);

		LastStack = 1;
		SET_GAME_STATE (SLYLANDRO_STACK9, 2);
	}
	else if (PLAYER_SAID (R, what_missile_batteries))
	{
		NPCPhrase (LIGHTNING_ONLY_FOR_HARVESTING);

		SET_GAME_STATE (SLYLANDRO_STACK9, 3);
	}

	switch (GET_GAME_STATE (SLYLANDRO_STACK9))
	{
		case 0:
			pStr[0] = how_does_probe_defend;
			break;
		case 1:
			pStr[0] = combat_behavior;
			break;
		case 2:
			pStr[0] = what_missile_batteries;
			break;
	}
	switch (GET_GAME_STATE (SLYLANDRO_STACK8))
	{
		case 2:
			pStr[1] = tell_me_about_rep_1;
			break;
		case 3:
			if (PHRASE_ENABLED (what_set_priority))
				pStr[1] = what_set_priority;
			break;
	}

	if (LastStack && pStr[LastStack - 1])
		Response (pStr[LastStack - 1], ProbeFunction);
	if (!GET_GAME_STATE (PLAYER_KNOWS_PROGRAM))
		Response (tell_me_about_basics, ProbeFunction);
	else
	{
		if (GET_GAME_STATE (PLAYER_KNOWS_PRIORITY))
		{
			if (GET_GAME_STATE (PLAYER_KNOWS_EFFECTS))
			{
				Response (probe_has_bug, ProbeBug);
			}
			if (PHRASE_ENABLED (what_effect))
				Response (what_effect, ProbeFunction);
		}
		if (PHRASE_ENABLED (tell_basics_again))
			Response (tell_basics_again, ProbeFunction);
	}
	if (LastStack == 0)
	{
		do
		{
			if (pStr[LastStack])
				Response (pStr[LastStack], ProbeFunction);
		} while (++LastStack < 2);
	}
	else
	{
		LastStack = (LastStack - 1) ^ 1;
		if (pStr[LastStack])
			Response (pStr[LastStack], ProbeFunction);
	}

	Response (enough_problem, ProbeInfo);
}

static void
ProbeInfo (RESPONSE_REF R)
{
	BYTE i, LastStack, InfoLeft;
	RESPONSE_REF pStr[3];

	LastStack = 0;
	pStr[0] = pStr[1] = pStr[2] = 0;
	if (PLAYER_SAID (R, what_are_probes))
	{
		NPCPhrase (PROBES_ARE);

		SET_GAME_STATE (SLYLANDRO_STACK5, 1);
	}
	else if (PLAYER_SAID (R, know_more_probe))
		NPCPhrase (OK_WHAT_MORE_PROBE);
	else if (PLAYER_SAID (R, why_probe_always_attack))
	{
		NPCPhrase (ONLY_DEFEND);

		SET_GAME_STATE (SLYLANDRO_STACK6, 1);
	}
	else if (PLAYER_SAID (R, talk_more_probe_attack))
	{
		ProbeFunction (R);
		return;
	}
	else if (PLAYER_SAID (R, where_probes_from))
	{
		NPCPhrase (PROBES_FROM_MELNORME);

		LastStack = 1;
		SET_GAME_STATE (SLYLANDRO_STACK7, 1);
	}
	else if (PLAYER_SAID (R, why_sell))
	{
		NPCPhrase (SELL_FOR_INFO);

		LastStack = 1;
		SET_GAME_STATE (SLYLANDRO_STACK7, 2);
	}
	else if (PLAYER_SAID (R, how_long_ago))
	{
		NPCPhrase (FIFTY_THOUSAND_ROTATIONS);

		SET_GAME_STATE (SLYLANDRO_STACK7, 3);
	}
	else if (PLAYER_SAID (R, whats_probes_mission))
	{
		NPCPhrase (SEEK_OUT_NEW_LIFE);

		LastStack = 2;
		SET_GAME_STATE (SLYLANDRO_STACK8, 1);
	}
	else if (PLAYER_SAID (R, if_only_one))
	{
		NPCPhrase (THEY_REPLICATE);

		SET_GAME_STATE (SLYLANDRO_STACK8, 2);
	}
	else if (PLAYER_SAID (R, enough_problem))
		NPCPhrase (OK_ENOUGH_PROBLEM);

	if (!GET_GAME_STATE (SLYLANDRO_KNOW_BROKEN)
			&& GET_GAME_STATE (PROBE_EXHIBITED_BUG))
	{
		switch (GET_GAME_STATE (SLYLANDRO_STACK6))
		{
			case 0:
				pStr[0] = why_probe_always_attack;
				break;
			case 1:
				pStr[0] = talk_more_probe_attack;
				break;
		}
	}
	switch (GET_GAME_STATE (SLYLANDRO_STACK7))
	{
		case 0:
			pStr[1] = where_probes_from;
			break;
		case 1:
			pStr[1] = why_sell;
			break;
		case 2:
			pStr[1] = how_long_ago;
			break;
	}
	switch (GET_GAME_STATE (SLYLANDRO_STACK8))
	{
		case 0:
			pStr[2] = whats_probes_mission;
			break;
		case 1:
			pStr[2] = if_only_one;
			break;
	}

	InfoLeft = FALSE;
	if (pStr[LastStack])
	{
		InfoLeft = TRUE;
		Response (pStr[LastStack], ProbeInfo);
	}
	for (i = 0; i < 3; ++i)
	{
		if (i != LastStack && pStr[i])
		{
			InfoLeft = TRUE;
			Response (pStr[i], ProbeInfo);
		}
	}

	Response (enough_probe, HomeWorld);
	if (!InfoLeft)
	{
		DISABLE_PHRASE (know_more_probe);
	}
}

static void
HomeWorld (RESPONSE_REF R)
{
	BYTE i, LastStack;
	RESPONSE_REF pStr[3];

	LastStack = 0;
	pStr[0] = pStr[1] = pStr[2] = 0;
	if (PLAYER_SAID (R, we_are_us))
	{
		NPCPhrase (TERRIBLY_EXCITING);

		SET_GAME_STATE (SLYLANDRO_STACK1, 1);
		DISABLE_PHRASE (we_are_us);
	}
	else if (PLAYER_SAID (R, what_other_visitors))
	{
		NPCPhrase (VISITORS);

		SET_GAME_STATE (PLAYER_KNOWS_PROBE, 1);
		SET_GAME_STATE (SLYLANDRO_STACK1, 2);
	}
	else if (PLAYER_SAID (R, any_other_visitors))
	{
		NPCPhrase (LONG_AGO);

		SET_GAME_STATE (SLYLANDRO_STACK1, 3);
	}
	else if (PLAYER_SAID (R, what_about_sentient_milieu))
	{
		NPCPhrase (MET_TAALO_THEY_ARE_FROM);

		SET_GAME_STATE (SLYLANDRO_STACK1, 4);
	}
	else if (PLAYER_SAID (R, who_else))
	{
		NPCPhrase (PRECURSORS);

		SET_GAME_STATE (SLYLANDRO_STACK1, 5);
	}
	else if (PLAYER_SAID (R, precursors_yow))
	{
		NPCPhrase (ABOUT_PRECURSORS);

		SET_GAME_STATE (SLYLANDRO_STACK1, 6);
	}
	else if (PLAYER_SAID (R, must_know_more))
	{
		NPCPhrase (ALL_WE_KNOW);

		SET_GAME_STATE (SLYLANDRO_STACK1, 7);
	}
	else if (PLAYER_SAID (R, who_are_you))
	{
		NPCPhrase (WE_ARE_SLY);

		LastStack = 1;
		SET_GAME_STATE (SLYLANDRO_STACK2, 1);
	}
	else if (PLAYER_SAID (R, where_are_you))
	{
		NPCPhrase (DOWN_HERE);

		LastStack = 2;
		SET_GAME_STATE (SLYLANDRO_STACK3, 1);
	}
	else if (PLAYER_SAID (R, thats_impossible_1))
	{
		NPCPhrase (NO_ITS_NOT_1);

		LastStack = 2;
		SET_GAME_STATE (SLYLANDRO_STACK3, 2);
	}
	else if (PLAYER_SAID (R, thats_impossible_2))
	{
		NPCPhrase (NO_ITS_NOT_2);

		LastStack = 2;
		SET_GAME_STATE (SLYLANDRO_STACK3, 3);
	}
	else if (PLAYER_SAID (R, like_more_about_you))
	{
		SlylandroInfo (R);
		return;
	}
	else if (PLAYER_SAID (R, enough_about_me))
		NPCPhrase (OK_ENOUGH_YOU);
	else if (PLAYER_SAID (R, enough_info))
		NPCPhrase (OK_ENOUGH_INFO);
	else if (PLAYER_SAID (R, enough_probe))
		NPCPhrase (OK_ENOUGH_PROBE);
	else if (PLAYER_SAID (R, mega_self_destruct))
	{
		NPCPhrase (WHY_YES_THERE_IS);

		SET_GAME_STATE (SLYLANDRO_KNOW_BROKEN, 1);
		SET_GAME_STATE (DESTRUCT_CODE_ON_SHIP, 1);
		i = GET_GAME_STATE (SLYLANDRO_MULTIPLIER) + 1;
		SET_GAME_STATE (SLYLANDRO_MULTIPLIER, i);
		AddEvent (RELATIVE_EVENT, 0, 0, 0, SLYLANDRO_RAMP_DOWN);
	}

	switch (GET_GAME_STATE (SLYLANDRO_STACK1))
	{
		case 0:
			pStr[0] = we_are_us;
			break;
		case 1:
			pStr[0] = what_other_visitors;
			break;
		case 2:
			pStr[0] = any_other_visitors;
			break;
		case 3:
			pStr[0] = what_about_sentient_milieu;
			break;
		case 4:
			pStr[0] = who_else;
			break;
		case 5:
			pStr[0] = precursors_yow;
			break;
		case 6:
			pStr[0] = must_know_more;
			break;
	}
	switch (GET_GAME_STATE (SLYLANDRO_STACK2))
	{
		case 0:
			pStr[1] = who_are_you;
			break;
		case 1:
			if (PHRASE_ENABLED (like_more_about_you))
				pStr[1] = like_more_about_you;
			break;
	}
	switch (GET_GAME_STATE (SLYLANDRO_STACK3))
	{
		case 0:
			pStr[2] = where_are_you;
			break;
		case 1:
			pStr[2] = thats_impossible_1;
			break;
		case 2:
			pStr[2] = thats_impossible_2;
			break;
	}

	if (pStr[LastStack])
		Response (pStr[LastStack], HomeWorld);
	for (i = 0; i < 3; ++i)
	{
		if (i != LastStack && pStr[i])
			Response (pStr[i], HomeWorld);
	}
	if (GET_GAME_STATE (SLYLANDRO_STACK1))
	{
		switch (GET_GAME_STATE (SLYLANDRO_STACK4))
		{
			case 0:
				Response (happy_to_tell_more, HumanInfo);
				break;
			case 1:
				Response (would_you_like_to_know_more, HumanInfo);
				break;
		}
	}
	if (GET_GAME_STATE (PLAYER_KNOWS_PROBE)
			&& !GET_GAME_STATE (SLYLANDRO_KNOW_BROKEN))
	{
		switch (GET_GAME_STATE (SLYLANDRO_STACK5))
		{
			case 0:
				Response (what_are_probes, ProbeInfo);
				break;
			case 1:
				if (PHRASE_ENABLED (know_more_probe))
					Response (know_more_probe, ProbeInfo);
				break;
		}
	}
	Response (bye, ExitConversation);
}

static void
Intro (void)
{
	BYTE NumVisits;

	if (GET_GAME_STATE (SLYLANDRO_KNOW_BROKEN)
			&& (NumVisits = GET_GAME_STATE (RECALL_VISITS)) == 0)
	{
		NPCPhrase (RECALL_PROGRAM_1);
		++NumVisits;
		SET_GAME_STATE (RECALL_VISITS, NumVisits);
	}
	else
	{
		NumVisits = GET_GAME_STATE (SLYLANDRO_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (HELLO_1);
				break;
			case 1:
				NPCPhrase (HELLO_2);
				break;
			case 2:
				NPCPhrase (HELLO_3);
				break;
			case 3:
				NPCPhrase (HELLO_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (SLYLANDRO_HOME_VISITS, NumVisits);
	}

	HomeWorld ((RESPONSE_REF)0);
}

static COUNT
uninit_slylandro (void)
{
	luaUqm_comm_uninit ();
	return (0);
}

static void
post_slylandro_enc (void)
{
	// nothing defined so far
}

LOCDATA*
init_slylandro_comm (void)
{
	LOCDATA *retval;

	slylandro_desc.init_encounter_func = Intro;
	slylandro_desc.post_encounter_func = post_slylandro_enc;
	slylandro_desc.uninit_encounter_func = uninit_slylandro;

	luaUqm_comm_init (NULL, NULL_RESOURCE);
			// Initialise Lua for string interpolation. This will be
			// generalised in the future.

	slylandro_desc.AlienTextBaseline.x = TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	slylandro_desc.AlienTextBaseline.y = 0;
	slylandro_desc.AlienTextWidth = SIS_TEXT_WIDTH;

	setSegue (Segue_peace);
	retval = &slylandro_desc;

	return (retval);
}
