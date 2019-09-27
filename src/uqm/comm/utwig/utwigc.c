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
#include "uqm/build.h"
#include "uqm/gameev.h"


static LOCDATA utwig_desc =
{
	UTWIG_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	UTWIG_PMAP_ANIM, /* AlienFrame */
	UTWIG_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_MIDDLE, /* AlienTextValign */
	UTWIG_COLOR_MAP, /* AlienColorMap */
	UTWIG_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	UTWIG_CONVERSATION_PHRASES, /* PlayerPhrases */
	16, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			4, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			7, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			(1 << 2), /* BlockMask */
		},
		{
			11, /* StartIndex */
			2, /* NumFrames */
			YOYO_ANIM
					| WAIT_TALKING, /* AnimFlags */
			ONE_SECOND * 2 / 15, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			(1 << 1), /* BlockMask */
		},
		{
			13, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 12, 0, /* FrameRate */
			ONE_SECOND / 12, 0, /* RestartRate */
			0, /* BlockMask */
		},
		{
			18, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND * 2 / 15, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			20, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND * 2 / 15, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			22, /* StartIndex */
			3, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND * 2 / 15, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			25, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND * 2 / 15, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			27, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND * 2 / 15, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			30, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND * 2 / 15, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			32, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND * 2 / 15, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			34, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND * 2 / 15, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			36, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND * 2 / 15, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			38, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND * 2 / 15, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			40, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND * 2 / 15, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			42, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND * 2 / 15, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
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
		1, /* StartIndex */
		3, /* NumFrames */
		0, /* AnimFlags */
		ONE_SECOND / 20, ONE_SECOND / 20, /* FrameRate */
		ONE_SECOND * 7 / 60, ONE_SECOND / 2, /* RestartRate */
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
	setSegue (Segue_peace);

	if (PLAYER_SAID (R, bye_neutral))
		NPCPhrase (GOODBYE_NEUTRAL);
	else if (PLAYER_SAID (R, bye_after_space))
		NPCPhrase (GOODBYE_AFTER_SPACE);
	else if (PLAYER_SAID (R, bye_before_space))
		NPCPhrase (GOODBYE_BEFORE_SPACE);
	else if (PLAYER_SAID (R, bye_allied_homeworld))
		NPCPhrase (GOODBYE_ALLIED_HOMEWORLD);
	else if (PLAYER_SAID (R, bye_bomb))
		NPCPhrase (GOODBYE_BOMB);
	else if (PLAYER_SAID (R, demand_bomb))
	{
		NPCPhrase (GUARDS_FIGHT);

		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, got_ultron)
			|| PLAYER_SAID (R, hey_wait_got_ultron))
	{
		if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 6))
		{
			NPCPhrase (NO_ULTRON_AT_BOMB);

			SET_GAME_STATE (REFUSED_ULTRON_AT_BOMB, 1);
		}
		else
		{
			if (PLAYER_SAID (R, got_ultron))
				NPCPhrase (DONT_WANT_TO_LOOK);
			else
				NPCPhrase (TAUNT_US_BUT_WE_LOOK);
			if (GET_GAME_STATE (ULTRON_CONDITION) < 4)
			{
				switch (GET_GAME_STATE (UTWIG_INFO))
				{
					case 0:
						if (PLAYER_SAID (R, got_ultron))
							NPCPhrase (SICK_TRICK_1);
						else
						{
							NPCPhrase (TRICKED_US_1);

							setSegue (Segue_hostile);
						}
						break;
					case 1:
						if (PLAYER_SAID (R, got_ultron))
							NPCPhrase (SICK_TRICK_2);
						else
						{
							NPCPhrase (TRICKED_US_2);

							setSegue (Segue_hostile);
						}
						break;
				}
				SET_GAME_STATE (UTWIG_INFO, 1);
			}
			else
			{
				NPCPhrase (HAPPY_DAYS);
				if (GET_GAME_STATE (KOHR_AH_FRENZY))
					NPCPhrase (TOO_LATE);
				else
				{
					NPCPhrase (OK_ATTACK_KOHRAH);

					AddEvent (RELATIVE_EVENT, 0, 0, 0, ADVANCE_UTWIG_SUPOX_MISSION);
				}

				SET_GAME_STATE (UTWIG_HAVE_ULTRON, 1);
				SET_GAME_STATE (ULTRON_CONDITION, 5);

				SET_GAME_STATE (UTWIG_VISITS, 0);
				SET_GAME_STATE (SUPOX_VISITS, 0);
				SET_GAME_STATE (UTWIG_HOME_VISITS, 0);
				SET_GAME_STATE (SUPOX_HOME_VISITS, 0);
				SET_GAME_STATE (BOMB_VISITS, 0);

				SET_GAME_STATE (SUPOX_INFO, 0);
				SET_GAME_STATE (UTWIG_INFO, 0);
				SET_GAME_STATE (SUPOX_WAR_NEWS, 0);
				SET_GAME_STATE (UTWIG_WAR_NEWS, 0);
				SET_GAME_STATE (SUPOX_HOSTILE, 0);
				SET_GAME_STATE (UTWIG_HOSTILE, 0);

				SetRaceAllied (UTWIG_SHIP, TRUE);
				SetRaceAllied (SUPOX_SHIP, TRUE);
			}
		}
	}
	else if (PLAYER_SAID (R, can_you_help))
	{
		NPCPhrase (HOW_HELP);
		if (EscortFeasibilityStudy (UTWIG_SHIP) == 0)
			NPCPhrase (DONT_NEED);
		else
		{
			NPCPhrase (HAVE_4_SHIPS);

			AlienTalkSegue ((COUNT)~0);
			AddEscortShips (UTWIG_SHIP, 4);
		}
	}
}

static void AlliedHome (RESPONSE_REF R);

static void
AlliedHome (RESPONSE_REF R)
{
	BYTE NumVisits, News;

	News = GET_GAME_STATE (UTWIG_WAR_NEWS);
	NumVisits = GET_GAME_STATE (UTWIG_SUPOX_MISSION);
	if (PLAYER_SAID (R, how_went_war))
	{
		NPCPhrase (ABOUT_BATTLE);

		News |= (1 << 0);
	}
	else if (PLAYER_SAID (R, how_goes_war))
	{
		if (NumVisits == 1)
		{
			NPCPhrase (FLEET_ON_WAY);

			SET_GAME_STATE (UTWIG_WAR_NEWS, 1);
		}
		else switch (GET_GAME_STATE (UTWIG_WAR_NEWS))
		{
			case 0:
				NPCPhrase (BATTLE_HAPPENS_1);
				News = 1;
				break;
			case 1:
				NPCPhrase (BATTLE_HAPPENS_2);
				News = 2;
				break;
		}

		DISABLE_PHRASE (how_goes_war);
	}
	else if (PLAYER_SAID (R, learn_new_info))
	{
		if (NumVisits < 5)
			NPCPhrase (NO_NEW_INFO);
		else
		{
			NPCPhrase (SAMATRA);

			News |= (1 << 1);
		}

		DISABLE_PHRASE (learn_new_info);
	}
	else if (PLAYER_SAID (R, what_now_homeworld))
	{
		if (NumVisits < 5)
			NPCPhrase (UP_TO_YOU);
		else
			NPCPhrase (HOPE_KILL_EACH_OTHER);

		DISABLE_PHRASE (what_now_homeworld);
	}
	else if (PLAYER_SAID (R, how_is_ultron))
	{
		NPCPhrase (ULTRON_IS_GREAT);

		DISABLE_PHRASE (how_is_ultron);
	}
	SET_GAME_STATE (UTWIG_WAR_NEWS, News);

	if (NumVisits >= 5)
	{
		if (!(News & (1 << 0)))
			Response (how_went_war, AlliedHome);
	}
	else if (PHRASE_ENABLED (how_goes_war)
			&& ((NumVisits == 1 && News == 0)
			|| (NumVisits && News < 2)))
		Response (how_goes_war, AlliedHome);
	if (PHRASE_ENABLED (learn_new_info))
		Response (learn_new_info, AlliedHome);
	if (PHRASE_ENABLED (what_now_homeworld))
		Response (what_now_homeworld, AlliedHome);
	if (PHRASE_ENABLED (how_is_ultron))
		Response (how_is_ultron, AlliedHome);
	if (NumVisits == 0 && EscortFeasibilityStudy (UTWIG_SHIP) != 0)
		Response (can_you_help, ExitConversation);
	Response (bye_allied_homeworld, ExitConversation);
}

static void
BeforeKohrAh (RESPONSE_REF R)
{
	BYTE NumVisits;

	if (PLAYER_SAID (R, whats_up_before_space))
	{
		NumVisits = GET_GAME_STATE (UTWIG_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (GENERAL_INFO_BEFORE_SPACE_1);
				break;
			case 1:
				NPCPhrase (GENERAL_INFO_BEFORE_SPACE_2);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (UTWIG_INFO, NumVisits);

		DISABLE_PHRASE (whats_up_before_space);
	}
	else if (PLAYER_SAID (R, what_now_before_space))
	{
		NPCPhrase (DO_THIS_BEFORE_SPACE);

		DISABLE_PHRASE (what_now_before_space);
	}

	if (PHRASE_ENABLED (whats_up_before_space))
		Response (whats_up_before_space, BeforeKohrAh);
	if (PHRASE_ENABLED (what_now_before_space))
		Response (what_now_before_space, BeforeKohrAh);
	Response (bye_before_space, ExitConversation);
}

static void
AfterKohrAh (RESPONSE_REF R)
{
	BYTE NumVisits;

	if (PLAYER_SAID (R, whats_up_after_space))
	{
		NumVisits = GET_GAME_STATE (UTWIG_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (GENERAL_INFO_AFTER_SPACE_1);
				break;
			case 1:
				NPCPhrase (GENERAL_INFO_AFTER_SPACE_2);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (UTWIG_INFO, NumVisits);

		DISABLE_PHRASE (whats_up_after_space);
	}
	else if (PLAYER_SAID (R, what_now_after_space))
	{
		NPCPhrase (DO_THIS_AFTER_SPACE);

		DISABLE_PHRASE (what_now_after_space);
	}

	if (PHRASE_ENABLED (whats_up_after_space))
		Response (whats_up_after_space, AfterKohrAh);
	if (PHRASE_ENABLED (what_now_after_space))
		Response (what_now_after_space, AfterKohrAh);
	Response (bye_after_space, ExitConversation);
}

static void
NeutralUtwig (RESPONSE_REF R)
{
	BYTE i, LastStack;
	RESPONSE_REF pStr[4];

	LastStack = 0;
	pStr[0] = pStr[1] = pStr[2] = pStr[3] = 0;
	if (PLAYER_SAID (R, we_are_vindicator))
	{
		NPCPhrase (WOULD_BE_HAPPY_BUT);

		SET_GAME_STATE (UTWIG_STACK1, 1);
	}
	else if (PLAYER_SAID (R, why_sad))
	{
		NPCPhrase (ULTRON_BROKE);

		SET_GAME_STATE (UTWIG_STACK1, 2);
	}
	else if (PLAYER_SAID (R, what_ultron))
	{
		NPCPhrase (GLORIOUS_ULTRON);

		SET_GAME_STATE (UTWIG_STACK1, 3);
	}
	else if (PLAYER_SAID (R, dont_be_babies))
	{
		NPCPhrase (MOCK_OUR_PAIN);

		setSegue (Segue_hostile);
		SET_GAME_STATE (UTWIG_STACK1, 4);
		SET_GAME_STATE (UTWIG_HOSTILE, 1);
		SET_GAME_STATE (UTWIG_INFO, 0);
		SET_GAME_STATE (UTWIG_HOME_VISITS, 0);
		SET_GAME_STATE (UTWIG_VISITS, 0);
		SET_GAME_STATE (BOMB_VISITS, 0);
		return;
	}
	else if (PLAYER_SAID (R, real_sorry_about_ultron))
	{
		NPCPhrase (APPRECIATE_SYMPATHY);

		SET_GAME_STATE (UTWIG_STACK1, 4);
		return;
	}
	else if (PLAYER_SAID (R, what_about_you_1))
	{
		NPCPhrase (ABOUT_US_1);

		LastStack = 2;
		SET_GAME_STATE (UTWIG_WAR_NEWS, 1);
	}
	else if (PLAYER_SAID (R, what_about_you_2))
	{
		NPCPhrase (ABOUT_US_2);

		LastStack = 2;
		StartSphereTracking (SUPOX_SHIP);
		SET_GAME_STATE (UTWIG_WAR_NEWS, 2);
	}
	else if (PLAYER_SAID (R, what_about_you_3))
	{
		NPCPhrase (ABOUT_US_3);
		
		SET_GAME_STATE (UTWIG_WAR_NEWS, 3);
	}
	else if (PLAYER_SAID (R, what_about_urquan_1))
	{
		NPCPhrase (ABOUT_URQUAN_1);

		LastStack = 3;
		SET_GAME_STATE (UTWIG_STACK2, 1);
	}
	else if (PLAYER_SAID (R, what_about_urquan_2))
	{
		NPCPhrase (ABOUT_URQUAN_2);

		SET_GAME_STATE (UTWIG_STACK2, 2);
	}

	switch (GET_GAME_STATE (UTWIG_STACK1))
	{
		case 0:
			pStr[0] = we_are_vindicator;
			break;
		case 1:
			pStr[0] = why_sad;
			break;
		case 2:
			pStr[0] = what_ultron;
			break;
		case 3:
			pStr[0] = dont_be_babies;
			pStr[1] = real_sorry_about_ultron;
			break;
	}
	switch (GET_GAME_STATE (UTWIG_WAR_NEWS))
	{
		case 0:
			pStr[2] = what_about_you_1;
			break;
		case 1:
			pStr[2] = what_about_you_2;
			break;
		case 2:
			pStr[2] = what_about_you_3;
			break;
	}
	switch (GET_GAME_STATE (UTWIG_STACK2))
	{
		case 0:
			pStr[2] = what_about_urquan_1;
			break;
		case 1:
			pStr[2] = what_about_urquan_2;
			break;
	}

	if (pStr[LastStack])
		Response (pStr[LastStack], NeutralUtwig);
	for (i = 0; i < 4; ++i)
	{
		if (i != LastStack && pStr[i])
			Response (pStr[i], NeutralUtwig);
	}
	if (GET_GAME_STATE (ULTRON_CONDITION))
		Response (got_ultron, ExitConversation);
	Response (bye_neutral, ExitConversation);
}

static void
BombWorld (RESPONSE_REF R)
{
	BYTE LastStack;
	RESPONSE_REF pStr[2];

	LastStack = 0;
	pStr[0] = pStr[1] = 0;
	if (PLAYER_SAID (R, why_you_here))
	{
		NPCPhrase (WE_GUARD_BOMB);

		SET_GAME_STATE (BOMB_STACK1, 1);
	}
	else if (PLAYER_SAID (R, what_about_bomb))
	{
		NPCPhrase (ABOUT_BOMB);

		SET_GAME_STATE (BOMB_STACK1, 2);
	}
	else if (PLAYER_SAID (R, give_us_bomb_or_die))
	{
		NPCPhrase (GUARDS_WARN);

		SET_GAME_STATE (BOMB_STACK1, 3);
	}
	else if (PLAYER_SAID (R, demand_bomb))
	{
		NPCPhrase (GUARDS_FIGHT);

		setSegue (Segue_hostile);
		SET_GAME_STATE (UTWIG_HOSTILE, 1);
		SET_GAME_STATE (UTWIG_INFO, 0);
		SET_GAME_STATE (UTWIG_HOME_VISITS, 0);
		SET_GAME_STATE (UTWIG_VISITS, 0);
		SET_GAME_STATE (BOMB_VISITS, 0);
		return;
	}
	else if (PLAYER_SAID (R, may_we_have_bomb))
	{
		NPCPhrase (NO_BOMB);

		LastStack = 1;
		SET_GAME_STATE (BOMB_STACK2, 1);
	}
	else if (PLAYER_SAID (R, please))
	{
		NPCPhrase (SORRY_NO_BOMB);

		SET_GAME_STATE (BOMB_STACK2, 2);
	}
	else if (PLAYER_SAID (R, whats_up_bomb))
	{
		if (GET_GAME_STATE (BOMB_INFO))
			NPCPhrase (GENERAL_INFO_BOMB_2);
		else
		{
			NPCPhrase (GENERAL_INFO_BOMB_1);

			SET_GAME_STATE (BOMB_INFO, 1);
		}

		DISABLE_PHRASE (whats_up_bomb);
	}

	switch (GET_GAME_STATE (BOMB_STACK2))
	{
		case 0:
			pStr[1] = may_we_have_bomb;
			break;
		case 1:
			pStr[1] = please;
			break;
	}
	switch (GET_GAME_STATE (BOMB_STACK1))
	{
		case 0:
			pStr[0] = why_you_here;
			pStr[1] = 0;
			break;
		case 1:
			pStr[0] = what_about_bomb;
			pStr[1] = 0;
			break;
		case 2:
			pStr[0] = give_us_bomb_or_die;
			break;
		case 3:
			pStr[0] = demand_bomb;
			break;
	}

	if (pStr[LastStack])
		Response (pStr[LastStack], BombWorld);
	LastStack ^= 1;
	if (pStr[LastStack])
		Response (pStr[LastStack], BombWorld);

	if (PHRASE_ENABLED (whats_up_bomb) && (GET_GAME_STATE (BOMB_STACK1) > 1))
		Response (whats_up_bomb, BombWorld);

	if (GET_GAME_STATE (ULTRON_CONDITION)
			&& !GET_GAME_STATE (REFUSED_ULTRON_AT_BOMB))
		Response (got_ultron, ExitConversation);

	if (GET_GAME_STATE (BOMB_INFO))
	{
		Response (bye_bomb, ExitConversation);
	}
	else
	{
		Response (bye_neutral, ExitConversation);
	}
}

static void
Intro (void)
{
	BYTE NumVisits;

	if (LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
	{
		NPCPhrase (OUT_TAKES);

		setSegue (Segue_peace);
		return;
	}

	if (GET_GAME_STATE (UTWIG_HOSTILE))
	{
		if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 6))
		{
			NumVisits = GET_GAME_STATE (BOMB_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (HOSTILE_BOMB_HELLO_1);
					break;
				case 1:
					NPCPhrase (HOSTILE_BOMB_HELLO_2);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (BOMB_VISITS, NumVisits);
		}
		else if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = GET_GAME_STATE (UTWIG_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (HOSTILE_HOMEWORLD_HELLO_1);
					break;
				case 1:
					NPCPhrase (HOSTILE_HOMEWORLD_HELLO_2);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (UTWIG_HOME_VISITS, NumVisits);
		}
		else
		{
			NumVisits = GET_GAME_STATE (UTWIG_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (HOSTILE_SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase (HOSTILE_SPACE_HELLO_2);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (UTWIG_VISITS, NumVisits);
		}

		if (!GET_GAME_STATE (ULTRON_CONDITION)
				|| (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 6)))
		{
			setSegue (Segue_hostile);
		}
		else
		{
			Response (hey_wait_got_ultron, ExitConversation);
		}
	}
	else if (CheckAlliance (UTWIG_SHIP) == GOOD_GUY)
	{
		if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = GET_GAME_STATE (UTWIG_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (ALLIED_HOMEWORLD_HELLO_1);
					break;
				case 1:
					NPCPhrase (ALLIED_HOMEWORLD_HELLO_2);
					break;
				case 2:
					NPCPhrase (ALLIED_HOMEWORLD_HELLO_3);
					break;
				case 3:
					NPCPhrase (ALLIED_HOMEWORLD_HELLO_4);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (UTWIG_HOME_VISITS, NumVisits);

			AlliedHome ((RESPONSE_REF)0);
		}
		else
		{
			NumVisits = GET_GAME_STATE (UTWIG_SUPOX_MISSION);
			if (NumVisits == 1)
			{
				NumVisits = GET_GAME_STATE (UTWIG_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase (HELLO_BEFORE_KOHRAH_SPACE_1);
						break;
					case 1:
						NPCPhrase (HELLO_BEFORE_KOHRAH_SPACE_2);
						--NumVisits;
						break;
				}
				SET_GAME_STATE (UTWIG_VISITS, NumVisits);

				BeforeKohrAh ((RESPONSE_REF)0);
			}
			else if (NumVisits < 5)
			{
				NumVisits = GET_GAME_STATE (UTWIG_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase (HELLO_DURING_KOHRAH_SPACE_1);
						break;
					case 1:
						NPCPhrase (HELLO_DURING_KOHRAH_SPACE_2);
						--NumVisits;
						break;
				}
				SET_GAME_STATE (UTWIG_VISITS, NumVisits);
			}
			else
			{
				NumVisits = GET_GAME_STATE (UTWIG_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase (HELLO_AFTER_KOHRAH_SPACE_1);
						break;
					case 1:
						NPCPhrase (HELLO_AFTER_KOHRAH_SPACE_2);
						--NumVisits;
						break;
				}
				SET_GAME_STATE (UTWIG_VISITS, NumVisits);

				AfterKohrAh ((RESPONSE_REF)0);
			}
		}
	}
	else if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		NumVisits = GET_GAME_STATE (BOMB_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (BOMB_WORLD_HELLO_1);
				break;
			case 1:
				NPCPhrase (BOMB_WORLD_HELLO_2);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (BOMB_VISITS, NumVisits);

		BombWorld ((RESPONSE_REF)0);
	}
	else
	{
		if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = GET_GAME_STATE (UTWIG_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (NEUTRAL_HOMEWORLD_HELLO_1);
					break;
				case 1:
					NPCPhrase (NEUTRAL_HOMEWORLD_HELLO_2);
					break;
				case 2:
					NPCPhrase (NEUTRAL_HOMEWORLD_HELLO_3);
					break;
				case 3:
					NPCPhrase (NEUTRAL_HOMEWORLD_HELLO_4);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (UTWIG_HOME_VISITS, NumVisits);
		}
		else
		{
			NumVisits = GET_GAME_STATE (UTWIG_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (NEUTRAL_SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase (NEUTRAL_SPACE_HELLO_2);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (UTWIG_VISITS, NumVisits);
		}

		NeutralUtwig ((RESPONSE_REF)0);
	}
}

static COUNT
uninit_utwig (void)
{
	luaUqm_comm_uninit ();
	return (0);
}

static void
post_utwig_enc (void)
{
	// nothing defined so far
}

LOCDATA*
init_utwig_comm (void)
{
	LOCDATA *retval;

	utwig_desc.init_encounter_func = Intro;
	utwig_desc.post_encounter_func = post_utwig_enc;
	utwig_desc.uninit_encounter_func = uninit_utwig;

	luaUqm_comm_init (NULL, NULL_RESOURCE);
			// Initialise Lua for string interpolation. This will be
			// generalised in the future.

	utwig_desc.AlienTextBaseline.x = TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	utwig_desc.AlienTextBaseline.y = RES_SIS_SCALE(70);
	utwig_desc.AlienTextWidth = SIS_TEXT_WIDTH - 16;

	if (GET_GAME_STATE (UTWIG_HAVE_ULTRON))
	{	// use alternate 'Happy Utwig!' track
		utwig_desc.AlienAltSongRes = UTWIG_ULTRON_MUSIC;
		utwig_desc.AlienSongFlags |= LDASF_USE_ALTERNATE;
	}
	else
	{	// regular track -- let's make sure
		utwig_desc.AlienSongFlags &= ~LDASF_USE_ALTERNATE;
	}

	if (GET_GAME_STATE (UTWIG_HAVE_ULTRON)
			|| LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
	{
		setSegue (Segue_peace);
	}
	else
	{
		setSegue (Segue_hostile);
	}
	retval = &utwig_desc;

	return (retval);
}
