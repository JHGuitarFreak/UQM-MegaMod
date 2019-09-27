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

#include "uqm/gameev.h"


static LOCDATA arilou_desc =
{
	ARILOU_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	ARILOU_PMAP_ANIM, /* AlienFrame */
	ARILOU_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	ARILOU_COLOR_MAP, /* AlienColorMap */
	ARILOU_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	ARILOU_CONVERSATION_PHRASES, /* PlayerPhrases */
	20, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			4, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 1) | (1L << 16)
		},
		{
			13, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 0) | (1L << 16)
		},
		{
			22, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1L << 16)
		},
		{
			31, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 4)
		},
		{
			40, /* StartIndex */
			10, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 3)
		},
		{
			50, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 7)
		},
		{
			59, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			67, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 5)
		},
		{
			76, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 9)
		},
		{
			85, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 8)
		},
		{
			94, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			103, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 13)
		},
		{
			112, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			121, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 11)
		},
		{
			129, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1L << 15)
		},
		{
			138, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 14)
		},
		{
			146, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 0) | (1 << 1) | (1 << 2)
		},
		{	/* Hands moving (right up) */
			155, /* StartIndex */
			2, /* NumFrames */
			CIRCULAR_ANIM | WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 19), /* BlockMask */
		},
		{	/* Hands moving (left up) */
			157, /* StartIndex */
			2, /* NumFrames */
			CIRCULAR_ANIM | WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 19), /* BlockMask */
		},
		{	/* Stars flashing next to the head */
			159, /* StartIndex */
			4, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 12, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 17) | (1 << 18), /* BlockMask */
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
		ONE_SECOND / 15, 0, /* FrameRate */
		ONE_SECOND / 12, 0, /* RestartRate */
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

	if (PLAYER_SAID (R, bye_angry_space))
		NPCPhrase (GOODBYE_ANGRY_SPACE);
	else if (PLAYER_SAID (R, bye_friendly_space))
		NPCPhrase (GOODBYE_FRIENDLY_SPACE);
	else if (PLAYER_SAID (R, bye_friendly_homeworld))
		NPCPhrase (GOODBYE_FRDLY_HOMEWORLD);
	else if (PLAYER_SAID (R, lets_fight))
		NPCPhrase (NO_FIGHT);
	else if (PLAYER_SAID (R, bug_eyed_fruitcakes))
	{
		NPCPhrase (WE_NEVER_FRIENDS);

		SET_GAME_STATE (ARILOU_MANNER, 2);
	}
	else if (PLAYER_SAID (R, best_if_i_killed_you))
	{
		NPCPhrase (WICKED_HUMAN);

		SET_GAME_STATE (ARILOU_MANNER, 2);
	}
}

static void
ArilouHome (RESPONSE_REF R)
{
	BYTE i, LastStack;
	RESPONSE_REF pStr[4];

	LastStack = 0;
	pStr[0] = pStr[1] = pStr[2] = pStr[3] = 0;
	if (PLAYER_SAID (R, confused_by_hello))
		NPCPhrase (CONFUSED_RESPONSE);
	else if (PLAYER_SAID (R, happy_by_hello))
		NPCPhrase (HAPPY_RESPONSE);
	else if (PLAYER_SAID (R, miffed_by_hello))
		NPCPhrase (MIFFED_RESPONSE);
	else if (PLAYER_SAID (R, ok_lets_be_friends))
		NPCPhrase (NO_ALLY_BUT_MUCH_GIVE);
	else if (PLAYER_SAID (R, what_about_war))
	{
		NPCPhrase (ABOUT_WAR);

		SET_GAME_STATE (ARILOU_STACK_1, 1);
	}
	else if (PLAYER_SAID (R, what_about_urquan))
	{
		NPCPhrase (ABOUT_URQUAN);

		SET_GAME_STATE (ARILOU_STACK_1, 2);
	}
	else if (PLAYER_SAID (R, tell_arilou_about_tpet))
	{
		NPCPhrase (BAD_NEWS_ABOUT_TPET);

		LastStack = 1;
		SET_GAME_STATE (ARILOU_STACK_2, 1);
	}
	else if (PLAYER_SAID (R, what_do_about_tpet))
	{
		NPCPhrase (DANGEROUS_BUT_USEFUL);

		LastStack = 1;
		SET_GAME_STATE (ARILOU_STACK_2, 2);
	}
	else if (PLAYER_SAID (R, learned_about_umgah))
	{
		if (GET_GAME_STATE (ARILOU_CHECKED_UMGAH) != 2)
			NPCPhrase (NO_NEWS_YET);
		else
		{
			NPCPhrase (UMGAH_UNDER_COMPULSION);

			LastStack = 1;
		}

		DISABLE_PHRASE (learned_about_umgah);
	}
	else if (PLAYER_SAID (R, umgah_acting_weird))
	{
		NPCPhrase (WELL_GO_CHECK);

		SET_GAME_STATE (ARILOU_CHECKED_UMGAH, 1);
		AddEvent (RELATIVE_EVENT, 0, 10, 0, ARILOU_UMGAH_CHECK);
		DISABLE_PHRASE (umgah_acting_weird);
	}
	else if (PLAYER_SAID (R, what_do_now))
	{
		NPCPhrase (GO_FIND_OUT);

		SET_GAME_STATE (ARILOU_CHECKED_UMGAH, 3);
	}
	else if (PLAYER_SAID (R, what_did_on_earth))
	{
		NPCPhrase (DID_THIS);

		LastStack = 2;
		SET_GAME_STATE (ARILOU_STACK_3, 1);
	}
	else if (PLAYER_SAID (R, why_did_this))
	{
		NPCPhrase (IDF_PARASITES);

		LastStack = 2;
		SET_GAME_STATE (ARILOU_STACK_3, 2);
	}
	else if (PLAYER_SAID (R, tell_more))
	{
		NPCPhrase (NOT_NOW);

		LastStack = 2;
		SET_GAME_STATE (ARILOU_STACK_3, 3);
	}
	else if (PLAYER_SAID (R, what_give_me))
	{
		NPCPhrase (ABOUT_PORTAL);

		LastStack = 3;
		SET_GAME_STATE (KNOW_ARILOU_WANT_WRECK, 1);

		R = about_portal_again;
		DISABLE_PHRASE (what_give_me);
	}
	else if (PLAYER_SAID (R, what_about_tpet))
	{
		NPCPhrase (ABOUT_TPET);

		SET_GAME_STATE (ARILOU_STACK_4, 1);
	}
	else if (PLAYER_SAID (R, about_portal_again))
	{
		NPCPhrase (PORTAL_AGAIN);

		DISABLE_PHRASE (about_portal_again);
	}
	else if (PLAYER_SAID (R, got_it))
	{
		if (GET_GAME_STATE (ARILOU_HOME_VISITS) == 1)
			NPCPhrase (CLEVER_HUMAN);
		NPCPhrase (GIVE_PORTAL);

		SET_GAME_STATE (PORTAL_KEY_ON_SHIP, 0);
		SET_GAME_STATE (PORTAL_SPAWNER, 1);
		SET_GAME_STATE (PORTAL_SPAWNER_ON_SHIP, 1);
	}
#ifdef NEVER
	else if (PLAYER_SAID (R, got_tpet))
	{
		NPCPhrase (OK_GOT_TPET);

		SET_GAME_STATE (ARILOU_STACK_2, 1);
	}
#endif /* NEVER */

	switch (GET_GAME_STATE (ARILOU_STACK_1))
	{
		case 0:
			pStr[0] = what_about_war;
			break;
		case 1:
			pStr[0] = what_about_urquan;
			break;
	}
	if (GET_GAME_STATE (TALKING_PET))
	{
#ifdef NEVER
		if (GET_GAME_STATE (ARILOU_STACK_2) == 0)
			pStr[1] = got_tpet;
#endif /* NEVER */
	}
	else
	{
		if (GET_GAME_STATE (TALKING_PET_VISITS))
		{
			switch (GET_GAME_STATE (ARILOU_STACK_2))
			{
				case 0:
					pStr[1] = tell_arilou_about_tpet;
					break;
				case 1:
					pStr[1] = what_do_about_tpet;
					break;
			}
		}
		else if (GET_GAME_STATE (KNOW_UMGAH_ZOMBIES))
		{
			if (!GET_GAME_STATE (ARILOU_CHECKED_UMGAH))
				pStr[1] = umgah_acting_weird;
			else if (PHRASE_ENABLED (learned_about_umgah) && PHRASE_ENABLED (umgah_acting_weird))
				pStr[1] = learned_about_umgah;
			else if (GET_GAME_STATE (ARILOU_CHECKED_UMGAH) == 2)
				pStr[1] = what_do_now;
		}
	}
	switch (GET_GAME_STATE (ARILOU_STACK_3))
	{
		case 0:
			pStr[2] = what_did_on_earth;
			break;
		case 1:
			pStr[2] = why_did_this;
			break;
		case 2:
			pStr[2] = tell_more;
			break;
	}
	if (!GET_GAME_STATE (KNOW_ARILOU_WANT_WRECK))
		pStr[3] = what_give_me;
	else if (!GET_GAME_STATE (ARILOU_STACK_4))
		pStr[3] = what_about_tpet;

	if (pStr[LastStack])
		Response (pStr[LastStack], ArilouHome);
	for (i = 0; i < 4; ++i)
	{
		if (i != LastStack && pStr[i])
			Response (pStr[i], ArilouHome);
	}

	if (GET_GAME_STATE (KNOW_ARILOU_WANT_WRECK))
	{
		if (GET_GAME_STATE (PORTAL_KEY_ON_SHIP))
			Response (got_it, ArilouHome);
		else if (PHRASE_ENABLED (about_portal_again) && !GET_GAME_STATE (PORTAL_SPAWNER))
			Response (about_portal_again, ArilouHome);
	}
	if (GET_GAME_STATE (ARILOU_MANNER) != 3)
		Response (best_if_i_killed_you, ExitConversation);
	Response (bye_friendly_homeworld, ExitConversation);
}

static void
AngryHomeArilou (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, invaders_from_mars))
	{
		NPCPhrase (HAD_OUR_REASONS);

		DISABLE_PHRASE (invaders_from_mars);
	}
	else if (PLAYER_SAID (R, why_should_i_trust))
	{
		NPCPhrase (TRUST_BECAUSE);

		DISABLE_PHRASE (why_should_i_trust);
	}
	else if (PLAYER_SAID (R, what_about_interference))
	{
		NPCPhrase (INTERFERENCE_NECESSARY);

		DISABLE_PHRASE (what_about_interference);
	}
	else if (PLAYER_SAID (R, i_just_like_to_leave))
	{
		NPCPhrase (SORRY_NO_LEAVE);

		DISABLE_PHRASE (i_just_like_to_leave);
	}

	if (PHRASE_ENABLED (invaders_from_mars))
		Response (invaders_from_mars, AngryHomeArilou);
	else
	{
		Response (bug_eyed_fruitcakes, ExitConversation);
	}
	if (PHRASE_ENABLED (why_should_i_trust))
		Response (why_should_i_trust, AngryHomeArilou);
	else if (PHRASE_ENABLED (what_about_interference))
		Response (what_about_interference, AngryHomeArilou);
	Response (ok_lets_be_friends, ArilouHome);
	Response (i_just_like_to_leave, AngryHomeArilou);
}

static void
AngrySpaceArilou (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, im_sorry))
	{
		NPCPhrase (APOLOGIZE_AT_HOMEWORLD);

		DISABLE_PHRASE (im_sorry);
	}

	Response (lets_fight, ExitConversation);
	if (PHRASE_ENABLED (im_sorry))
	{
		Response (im_sorry, AngrySpaceArilou);
	}
	Response (bye_angry_space, ExitConversation);
}

static void
FriendlySpaceArilou (RESPONSE_REF R)
{
	BYTE NumVisits;

	if (PLAYER_SAID (R, confused_by_hello))
		NPCPhrase (CONFUSED_RESPONSE);
	else if (PLAYER_SAID (R, happy_by_hello))
		NPCPhrase (HAPPY_RESPONSE);
	else if (PLAYER_SAID (R, miffed_by_hello))
		NPCPhrase (MIFFED_RESPONSE);
	else if (PLAYER_SAID (R, whats_up_1)
			|| PLAYER_SAID (R, whats_up_2))
	{
		NumVisits = GET_GAME_STATE (ARILOU_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (GENERAL_INFO_1);
				break;
			case 1:
				NPCPhrase (GENERAL_INFO_2);
				break;
			case 2:
				NPCPhrase (GENERAL_INFO_3);
				break;
			case 3:
				NPCPhrase (GENERAL_INFO_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (ARILOU_INFO, NumVisits);

		DISABLE_PHRASE (whats_up_2);
	}
	else if (PLAYER_SAID (R, why_you_here))
	{
		NPCPhrase (LEARN_THINGS);

		SET_GAME_STATE (ARILOU_STACK_5, 1);
	}
	else if (PLAYER_SAID (R, what_things))
	{
		NPCPhrase (THESE_THINGS);

		SET_GAME_STATE (ARILOU_STACK_5, 2);
	}
	else if (PLAYER_SAID (R, why_do_it))
	{
		NPCPhrase (DO_IT_BECAUSE);

		SET_GAME_STATE (ARILOU_STACK_5, 3);
	}
	else if (PLAYER_SAID (R, give_me_info_1)
			|| PLAYER_SAID (R, give_me_info_2))
	{
		NumVisits = GET_GAME_STATE (ARILOU_HINTS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (ARILOU_HINTS_1);
				break;
			case 1:
				NPCPhrase (ARILOU_HINTS_2);
				if (GET_GAME_STATE (KNOW_ABOUT_SHATTERED) < 2)
				{
					SET_GAME_STATE (KNOW_ABOUT_SHATTERED, 2);
				}
				break;
			case 2:
				NPCPhrase (ARILOU_HINTS_3);
				SET_GAME_STATE (KNOW_URQUAN_STORY, 1);
				SET_GAME_STATE (KNOW_KOHR_AH_STORY, 1);
				break;
			case 3:
				NPCPhrase (ARILOU_HINTS_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (ARILOU_HINTS, NumVisits);

		DISABLE_PHRASE (give_me_info_2);
	}

	switch (GET_GAME_STATE (ARILOU_STACK_5))
	{
		case 0:
			Response (why_you_here, FriendlySpaceArilou);
			break;
		case 1:
			Response (what_things, FriendlySpaceArilou);
			break;
		case 2:
			Response (why_do_it, FriendlySpaceArilou);
			break;
	}
	if (PHRASE_ENABLED (whats_up_2))
	{
		if (GET_GAME_STATE (ARILOU_INFO) == 0)
			Response (whats_up_1, FriendlySpaceArilou);
		else
			Response (whats_up_2, FriendlySpaceArilou);
	}
	if (PHRASE_ENABLED (give_me_info_2))
	{
		if (GET_GAME_STATE (ARILOU_HINTS) == 0)
			Response (give_me_info_1, FriendlySpaceArilou);
		else
			Response (give_me_info_2, FriendlySpaceArilou);
	}
	Response (bye_friendly_space, ExitConversation);
}

static void
Intro (void)
{
	BYTE NumVisits, Manner;

	if (LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
	{
		NPCPhrase (OUT_TAKES);

		setSegue (Segue_peace);
		return;
	}
	else if (!GET_GAME_STATE (MET_ARILOU))
	{
		RESPONSE_FUNC  RespFunc;

		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
		{
			NPCPhrase (INIT_HELLO);
			RespFunc = (RESPONSE_FUNC)FriendlySpaceArilou;
		}
		else
		{
			NPCPhrase (FRDLY_HOMEWORLD_HELLO_1);
			RespFunc = (RESPONSE_FUNC)ArilouHome;
			SET_GAME_STATE (ARILOU_HOME_VISITS, 1);
		}
		Response (confused_by_hello, RespFunc);
		Response (happy_by_hello, RespFunc);
		Response (miffed_by_hello, RespFunc);
		SET_GAME_STATE (MET_ARILOU, 1);
		return;
	}

	Manner = GET_GAME_STATE (ARILOU_MANNER);
	if (Manner == 2)
	{
		NumVisits = GET_GAME_STATE (ARILOU_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (HOSTILE_GOODBYE_1);
				break;
			case 1:
				NPCPhrase (HOSTILE_GOODBYE_2);
				break;
			case 2:
				NPCPhrase (HOSTILE_GOODBYE_3);
				break;
			case 3:
				NPCPhrase (HOSTILE_GOODBYE_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (ARILOU_VISITS, NumVisits);

		setSegue (Segue_peace);
	}
	else if (Manner == 1)
	{
		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) > 1)
		{
			NPCPhrase (INIT_ANGRY_HWLD_HELLO);
			SET_GAME_STATE (ARILOU_HOME_VISITS, 1);

			AngryHomeArilou ((RESPONSE_REF)0);
		}
		else
		{
			NumVisits = GET_GAME_STATE (ARILOU_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (ANGRY_SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase (ANGRY_SPACE_HELLO_2);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (ARILOU_VISITS, NumVisits);

			AngrySpaceArilou ((RESPONSE_REF)0);
		}
	}
	else
	{
		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
		{
			NumVisits = GET_GAME_STATE (ARILOU_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (FRIENDLY_SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase (FRIENDLY_SPACE_HELLO_2);
					break;
				case 2:
					NPCPhrase (FRIENDLY_SPACE_HELLO_3);
					break;
				case 3:
					NPCPhrase (FRIENDLY_SPACE_HELLO_4);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (ARILOU_VISITS, NumVisits);

			FriendlySpaceArilou ((RESPONSE_REF)0);
		}
		else
		{
			if (!GET_GAME_STATE (PORTAL_SPAWNER)
					&& GET_GAME_STATE (KNOW_ARILOU_WANT_WRECK))
			{
				NumVisits = GET_GAME_STATE (NO_PORTAL_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase (GOT_PART_YET_1);
						break;
					case 1:
						NPCPhrase (GOT_PART_YET_1);
						--NumVisits;
						break;
				}
				SET_GAME_STATE (NO_PORTAL_VISITS, NumVisits);
			}
			else
			{
				NumVisits = GET_GAME_STATE (ARILOU_HOME_VISITS);
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase (FRDLY_HOMEWORLD_HELLO_1);
						break;
					case 1:
						NPCPhrase (FRDLY_HOMEWORLD_HELLO_2);
						break;
					case 2:
						NPCPhrase (FRDLY_HOMEWORLD_HELLO_3);
						break;
					case 3:
						NPCPhrase (FRDLY_HOMEWORLD_HELLO_4);
						--NumVisits;
						break;
				}
				SET_GAME_STATE (ARILOU_HOME_VISITS, NumVisits);
			}

			ArilouHome ((RESPONSE_REF)0);
		}
	}
}

static COUNT
uninit_arilou (void)
{
	return (0);
}

static void
post_arilou_enc (void)
{
	BYTE Manner;

	if (getSegue () == Segue_hostile
			&& (Manner = GET_GAME_STATE (ARILOU_MANNER)) != 2)
	{
		SET_GAME_STATE (ARILOU_MANNER, 1);
		if (Manner != 1)
		{
			SET_GAME_STATE (ARILOU_VISITS, 0);
			SET_GAME_STATE (ARILOU_HOME_VISITS, 0);
		}
	}

	if (GET_GAME_STATE (ARILOU_SPACE_SIDE) > 1
			&& GET_GAME_STATE (ARILOU_HOME_VISITS) <= 1)
	{
		SET_GAME_STATE (UMGAH_ZOMBIE_BLOBBIES, 1);
		SET_GAME_STATE (UMGAH_VISITS, 0);
		SET_GAME_STATE (UMGAH_HOME_VISITS, 0);

		if (GET_GAME_STATE (ARILOU_MANNER) < 2)
		{
			SET_GAME_STATE (ARILOU_MANNER, 3);
		}
	}
}

LOCDATA*
init_arilou_comm (void)
{
	LOCDATA *retval;

	arilou_desc.init_encounter_func = Intro;
	arilou_desc.post_encounter_func = post_arilou_enc;
	arilou_desc.uninit_encounter_func = uninit_arilou;

	arilou_desc.AlienTextBaseline.x = TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	arilou_desc.AlienTextBaseline.y = 0;
	arilou_desc.AlienTextWidth = SIS_TEXT_WIDTH - 16;

	if (GET_GAME_STATE (ARILOU_SPACE_SIDE) > 1
			|| GET_GAME_STATE (ARILOU_MANNER) == 3
			|| LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
	{
		setSegue (Segue_peace);
	}
	else
	{
		setSegue (Segue_hostile);
	}
	retval = &arilou_desc;

	return (retval);
}
