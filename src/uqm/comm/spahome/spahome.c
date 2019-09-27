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
#include "../spathi/resinst.h"
#include "strings.h"

#include "uqm/lua/luacomm.h"
#include "uqm/build.h"
#include "uqm/gameev.h"


static LOCDATA spahome_desc_orig =
{
	SPATHI_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	SPATHI_HOME_PMAP_ANIM, /* AlienFrame */
	SPATHI_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	SPATHI_HOME_COLOR_MAP, /* AlienColorMap */
	SPATHI_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	SPATHI_HOME_CONVERSATION_PHRASES, /* PlayerPhrases */
	14, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			1, /* StartIndex */
			3, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			4, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			9, /* StartIndex */
			4, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 10) | (1 << 11), /* BlockMask */
		},
		{
			13, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND / 20, 0, /* RestartRate */
			(1 << 4) | (1 << 5) /* BlockMask */
		},
		{
			19, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 3) | (1 << 5), /* BlockMask */
		},
		{
			22, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 10, ONE_SECOND / 30, /* RestartRate */
			(1 << 3) | (1 << 4)
			| (1 << 10), /* BlockMask */
		},
		{
			26, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			(1 << 10), /* BlockMask */
		},
		{
			29, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			32, /* StartIndex */
			7, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND / 20, 0, /* RestartRate */
			(1 << 9) | (1 << 10), /* BlockMask */
		},
		{
			39, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 8) | (1 << 10), /* BlockMask */
		},
		{
			42, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, 0, /* RestartRate */
			(1 << 8) | (1 << 9)
			| (1 << 6) | (1 << 2)
			| (1 << 11) | (1 << 5), /* BlockMask */
		},
		{
			46, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 10, ONE_SECOND / 30, /* RestartRate */
			(1 << 2) | (1 << 10), /* BlockMask */
		},
		{
			50, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND / 20, 0, /* RestartRate */
			(1 << 13), /* BlockMask */
		},
		{
			56, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 12), /* BlockMask */
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

static LOCDATA spahome_desc_hd =
{
	SPATHI_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	SPATHI_HOME_PMAP_ANIM, /* AlienFrame */
	SPATHI_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	SPATHI_HOME_COLOR_MAP, /* AlienColorMap */
	SPATHI_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	SPATHI_HOME_CONVERSATION_PHRASES, /* PlayerPhrases */
	15, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			1, /* StartIndex */
			3, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0 | (1 << 14), /* BlockMask */
		},
		{
			4, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0 | (1 << 14), /* BlockMask */
		},
		{
			9, /* StartIndex */
			4, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 10) | (1 << 11) | (1 << 14), /* BlockMask */
		},
		{
			13, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND / 20, 0, /* RestartRate */
			(1 << 4) | (1 << 5) | (1 << 14) /* BlockMask */
		},
		{
			19, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 3) | (1 << 5) | (1 << 14), /* BlockMask */
		},
		{
			22, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 10, ONE_SECOND / 30, /* RestartRate */
			(1 << 3) | (1 << 4)
			| (1 << 10) | (1 << 14), /* BlockMask */
		},
		{
			26, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			(1 << 10) | (1 << 14), /* BlockMask */
		},
		{
			29, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			0 | (1 << 14), /* BlockMask */
		},
		{
			32, /* StartIndex */
			7, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND / 20, 0, /* RestartRate */
			(1 << 9) | (1 << 10) | (1 << 14), /* BlockMask */
		},
		{
			39, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 8) | (1 << 10) | (1 << 14), /* BlockMask */
		},
		{
			42, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, 0, /* RestartRate */
			(1 << 8) | (1 << 9)
			| (1 << 6) | (1 << 2)
			| (1 << 11) | (1 << 5)
			 | (1 << 14), /* BlockMask */
		},
		{
			46, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 10, ONE_SECOND / 30, /* RestartRate */
			(1 << 2) | (1 << 10) | (1 << 14), /* BlockMask */
		},
		{
			50, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND / 20, 0, /* RestartRate */
			(1 << 13) | (1 << 14), /* BlockMask */
		},
		{
			56, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 12) | (1 << 14), /* BlockMask */
		},
		{
			59, /* StartIndex */
			11, /* NumFrames */
			CIRCULAR_ANIM | ONE_SHOT_ANIM | WAIT_TALKING | ANIM_DISABLED, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			0, 0,/* RestartRate */
			(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) 
			| (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) 
			| (1 << 8) | (1 << 9) | (1 << 10) | (1 << 11) 
			| (1 << 12) | (1 << 13) , /* BlockMask */
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
	setSegue (Segue_peace);
	if (PLAYER_SAID (R, we_attack_again))
	{
		NPCPhrase (WE_FIGHT_AGAIN);

		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, surrender_or_die))
	{
		NPCPhrase (DEFEND_OURSELVES);

		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, we_are_vindicator))
	{
		NPCPhrase (NO_PASSWORD);

		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, gort_merenga)
			|| PLAYER_SAID (R, guph_florp)
			|| PLAYER_SAID (R, wagngl_fthagn)
			|| PLAYER_SAID (R, pleeese))
	{
		NPCPhrase (WRONG_PASSWORD);

		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, screw_password))
	{
		NPCPhrase (NO_PASSWORD);

		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, bye_no_ally_offer))
		NPCPhrase (GOODBYE_NO_ALLY_OFFER);
	else if (PLAYER_SAID (R, bye_angry_spathi))
		NPCPhrase (GOODBYE_ANGRY_SPATHI);
	else if (PLAYER_SAID (R, bye_ally))
		NPCPhrase (GOODBYE_ALLY);
	else if (PLAYER_SAID (R, already_got_them))
	{
		NPCPhrase (EARLY_BIRD_CHECK);

		SET_GAME_STATE (SPATHI_HOME_VISITS, 0);
		SET_GAME_STATE (SPATHI_VISITS, 0);
		SET_GAME_STATE (SPATHI_PARTY, 1);
		SET_GAME_STATE (SPATHI_MANNER, 3);
	}
	else if (PLAYER_SAID (R, too_dangerous))
		NPCPhrase (WE_AGREE);
	else if (PLAYER_SAID (R, think_more))
		NPCPhrase (COWARD);
	else if (PLAYER_SAID (R, i_accept))
	{
		NPCPhrase (AWAIT_RETURN);

		SET_GAME_STATE (SPATHI_QUEST, 1);
		SET_GAME_STATE (SPATHI_MANNER, 3);
		SET_GAME_STATE (SPATHI_VISITS, 0);
	}
	else if (PLAYER_SAID (R, do_as_we_say))
	{
		NPCPhrase (DEPART_FOR_EARTH);

		SetRaceAllied (SPATHI_SHIP, TRUE);
		AddEvent (RELATIVE_EVENT, 6, 0, 0, SPATHI_SHIELD_EVENT);
		SET_GAME_STATE (SPATHI_HOME_VISITS, 0);
		SET_GAME_STATE (SPATHI_VISITS, 0);
	}
	else if (PLAYER_SAID (R, killed_them_all_1))
	{
		NPCPhrase (WILL_CHECK_1);

		if (!GET_GAME_STATE (SPATHI_CREATURES_ELIMINATED))
		{
			SET_GAME_STATE (LIED_ABOUT_CREATURES, 1);
		}
		else
		{
			SET_GAME_STATE (SPATHI_HOME_VISITS, 0);
			SET_GAME_STATE (SPATHI_VISITS, 0);
			SET_GAME_STATE (SPATHI_PARTY, 1);
			SET_GAME_STATE (SPATHI_MANNER, 3);
		}
	}
	else if (PLAYER_SAID (R, killed_them_all_2))
	{
		NPCPhrase (WILL_CHECK_2);

		if (!GET_GAME_STATE (SPATHI_CREATURES_ELIMINATED))
		{
			SET_GAME_STATE (LIED_ABOUT_CREATURES, 2);
		}
		else
		{
			SET_GAME_STATE (SPATHI_HOME_VISITS, 0);
			SET_GAME_STATE (SPATHI_VISITS, 0);
			SET_GAME_STATE (SPATHI_PARTY, 1);
			SET_GAME_STATE (SPATHI_MANNER, 3);
		}
	}
	else if (PLAYER_SAID (R, bye_before_party))
	{
		NPCPhrase (GOODBYE_BEFORE_PARTY);
	}
	else if (PLAYER_SAID (R, bye_from_party_1)
		   || PLAYER_SAID (R, bye_from_party_2)
		   || PLAYER_SAID (R, bye_from_party_3))
	{
		NPCPhrase (GOODBYE_FROM_PARTY);
	}
}

static void SpathiAllies (RESPONSE_REF R);

static void
SpathiInfo (RESPONSE_REF R)
{
	BYTE InfoLeft;
	
	InfoLeft = FALSE;
	if (PLAYER_SAID (R, like_some_info))
		NPCPhrase (WHAT_ABOUT);
	else if (PLAYER_SAID (R, what_about_hierarchy))
	{
		NPCPhrase (ABOUT_HIERARCHY);

		DISABLE_PHRASE (what_about_hierarchy);
	}
	else if (PLAYER_SAID (R, what_about_history))
	{
		NPCPhrase (ABOUT_HISTORY);

		DISABLE_PHRASE (what_about_history);
	}
	else if (PLAYER_SAID (R, what_about_alliance))
	{
		NPCPhrase (ABOUT_ALLIANCE);

		DISABLE_PHRASE (what_about_alliance);
	}
	else if (PLAYER_SAID (R, what_about_other))
	{
		NPCPhrase (ABOUT_OTHER);

		DISABLE_PHRASE (what_about_other);
	}
	else if (PLAYER_SAID (R, what_about_precursors))
	{
		NPCPhrase (ABOUT_PRECURSORS);

		DISABLE_PHRASE (what_about_precursors);
	}

	if (PHRASE_ENABLED (what_about_hierarchy))
	{
		InfoLeft = TRUE;
		Response (what_about_hierarchy, SpathiInfo);
	}
	if (PHRASE_ENABLED (what_about_history))
	{
		InfoLeft = TRUE;
		Response (what_about_history, SpathiInfo);
	}
	if (PHRASE_ENABLED (what_about_alliance))
	{
		InfoLeft = TRUE;
		Response (what_about_alliance, SpathiInfo);
	}
	if (PHRASE_ENABLED (what_about_other))
	{
		InfoLeft = TRUE;
		Response (what_about_other, SpathiInfo);
	}
	if (PHRASE_ENABLED (what_about_precursors))
	{
		InfoLeft = TRUE;
		Response (what_about_precursors, SpathiInfo);
	}
	Response (enough_info, SpathiAllies);

	if (!InfoLeft)
	{
		DISABLE_PHRASE (like_some_info);
	}
}

static void
SpathiAllies (RESPONSE_REF R)
{
	BYTE NumVisits;

	if (R == 0)
	{
		NumVisits = GET_GAME_STATE (SPATHI_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (HELLO_ALLIES_1);
				break;
			case 1:
				NPCPhrase (HELLO_ALLIES_2);
				break;
			case 2:
				NPCPhrase (HELLO_ALLIES_3);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (SPATHI_HOME_VISITS, NumVisits);
	}
	else if (PLAYER_SAID (R, whats_up))
	{
		NumVisits = GET_GAME_STATE (SPATHI_INFO);
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
				SET_GAME_STATE (KNOW_URQUAN_STORY, 1);
				SET_GAME_STATE (KNOW_KOHR_AH_STORY, 1);
				break;
			case 3:
				NPCPhrase (GENERAL_INFO_4);
				break;
			case 4:
				NPCPhrase (GENERAL_INFO_5);
				--NumVisits;
				break;
			case 5:
				NPCPhrase (GENERAL_INFO_5);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (SPATHI_INFO, NumVisits);

		DISABLE_PHRASE (whats_up);
	}
	else if (PLAYER_SAID (R, resources_please))
	{
		NPCPhrase (SORRY_NO_RESOURCES);

		DISABLE_PHRASE (resources_please);
	}
	else if (PLAYER_SAID (R, something_fishy))
	{
		NPCPhrase (NOTHING_FISHY);

		SET_GAME_STATE (SPATHI_INFO, 5);
	}
	else if (PLAYER_SAID (R, enough_info))
		NPCPhrase (OK_ENOUGH_INFO);

	if (GET_GAME_STATE (SPATHI_INFO) == 4)
		Response (something_fishy, SpathiAllies);
	if (PHRASE_ENABLED (whats_up))
		Response (whats_up, SpathiAllies);
	if (PHRASE_ENABLED (resources_please))
		Response (resources_please, SpathiAllies);
	if (PHRASE_ENABLED (like_some_info))
		Response (like_some_info, SpathiInfo);
	Response (bye_ally, ExitConversation);
}

static void
SpathiQuest (RESPONSE_REF R)
{
	if (R == 0)
	{
		if (!GET_GAME_STATE (LIED_ABOUT_CREATURES))
			NPCPhrase (HOW_GO_EFFORTS);
		else
			NPCPhrase (YOU_LIED_1);
	}
	else if (PLAYER_SAID (R, little_mistake))
	{
		NPCPhrase (BIG_MISTAKE);

		DISABLE_PHRASE (little_mistake);
	}
	else if (PLAYER_SAID (R, talk_test))
	{
		NPCPhrase (TEST_AGAIN);

		DISABLE_PHRASE (talk_test);
	}
	else if (PLAYER_SAID (R, zapped_a_few))
	{
		NPCPhrase (YOU_FORTUNATE);

		DISABLE_PHRASE (zapped_a_few);
	}

	if (!GET_GAME_STATE (LIED_ABOUT_CREATURES))
		Response (killed_them_all_1, ExitConversation);
	else
	{
		if (PHRASE_ENABLED (little_mistake))
		{
			Response (little_mistake, SpathiQuest);
		}
		Response (killed_them_all_2, ExitConversation);
	}
	if (!GET_GAME_STATE (SPATHI_CREATURES_ELIMINATED))
	{
		if (PHRASE_ENABLED (talk_test))
		{
			Response (talk_test, SpathiQuest);
		}
		if (PHRASE_ENABLED (zapped_a_few)
				&& GET_GAME_STATE (SPATHI_CREATURES_EXAMINED))
		{
			Response (zapped_a_few, SpathiQuest);
		}
		Response (bye_before_party, ExitConversation);
	}
}

static void
LearnQuest (RESPONSE_REF R)
{
	if (R == 0)
	{
		NPCPhrase (QUEST_AGAIN);

		DISABLE_PHRASE (what_test);
		if (GET_GAME_STATE (KNOW_SPATHI_EVIL))
		{
			DISABLE_PHRASE (tell_evil);
		}
	}
	else if (PLAYER_SAID (R, how_prove))
		NPCPhrase (BETTER_IDEA);
	else if (PLAYER_SAID (R, what_test))
	{
		NPCPhrase (WIPE_EVIL);

		SET_GAME_STATE (KNOW_SPATHI_QUEST, 1);
		DISABLE_PHRASE (what_test);
	}
	else if (PLAYER_SAID (R, tell_evil))
	{
		NPCPhrase (BEFORE_ACCEPT);

		SET_GAME_STATE (KNOW_SPATHI_EVIL, 1);
		DISABLE_PHRASE (tell_evil);
		DISABLE_PHRASE (prove_strength);
	}
	else if (PLAYER_SAID (R, prove_strength))
	{
		NPCPhrase (YOUR_BEHAVIOR);

		DISABLE_PHRASE (prove_strength);
	}
	else if (PLAYER_SAID (R, why_dont_you_do_it))
	{
		NPCPhrase (WE_WONT_BECAUSE);

		DISABLE_PHRASE (why_dont_you_do_it);
	}

	if (PHRASE_ENABLED (what_test))
		Response (what_test, LearnQuest);
	else if (GET_GAME_STATE (SPATHI_CREATURES_ELIMINATED))
	{
		Response (already_got_them, ExitConversation);
	}
	else if (PHRASE_ENABLED (tell_evil))
	{
		Response (too_dangerous, ExitConversation);
		Response (tell_evil, LearnQuest);
	}
	else
	{
		Response (too_dangerous, ExitConversation);
		Response (think_more, ExitConversation);
		Response (i_accept, ExitConversation);
	}
	if (PHRASE_ENABLED (prove_strength) && !GET_GAME_STATE (KNOW_SPATHI_QUEST))
		Response (prove_strength, LearnQuest);
	if (PHRASE_ENABLED (why_dont_you_do_it))
		Response (why_dont_you_do_it, LearnQuest);
}

static void
AllianceOffer (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, misunderstanding))
	{
		NPCPhrase (JUST_MISUNDERSTANDING);
		if (!IS_HD){
			XFormColorMap (GetColorMapAddress (
					SetAbsColorMapIndex (CommData.AlienColorMap, 1)
					), ONE_SECOND / 4);
		} else {
			COUNT i = 0;
			COUNT limit = CommData.NumAnimations;
			
			for (i = 0; i < limit; i++)
				CommData.AlienAmbientArray[i].AnimFlags &= ~ANIM_DISABLED;
				
			CommData.AlienFrame = SetAbsFrameIndex 
				(CommData.AlienFrame, 0);
				
			CommData.AlienTalkDesc.AnimFlags &= ~PAUSE_TALKING;
		}

		SET_GAME_STATE (SPATHI_MANNER, 3);
		SET_GAME_STATE (SPATHI_VISITS, 0);
	}
	else if (PLAYER_SAID (R, we_come_in_peace))
		NPCPhrase (OF_COURSE);
	else if (PLAYER_SAID (R, hand_in_friendship))
	{
		NPCPhrase (TOO_AFRAID);

		DISABLE_PHRASE (hand_in_friendship);
	}
	else if (PLAYER_SAID (R, stronger))
	{
		NPCPhrase (YOURE_NOT);

		DISABLE_PHRASE (stronger);
	}
	else if (PLAYER_SAID (R, yes_we_are))
	{
		NPCPhrase (NO_YOURE_NOT);

		DISABLE_PHRASE (yes_we_are);
	}
	else if (PLAYER_SAID (R, share_info))
	{
		NPCPhrase (NO_INFO);

		DISABLE_PHRASE (share_info);
	}
	else if (PLAYER_SAID (R, give_us_resources))
	{
		NPCPhrase (NO_RESOURCES);

		DISABLE_PHRASE (give_us_resources);
	}

	if (PHRASE_ENABLED (hand_in_friendship))
		Response (hand_in_friendship, AllianceOffer);
	else if (PHRASE_ENABLED (stronger))
		Response (stronger, AllianceOffer);
	else if (PHRASE_ENABLED (yes_we_are))
		Response (yes_we_are, AllianceOffer);
	else
	{
		Response (how_prove, LearnQuest);
	}
	if (PHRASE_ENABLED (share_info))
		Response (share_info, AllianceOffer);
	if (PHRASE_ENABLED (give_us_resources))
		Response (give_us_resources, AllianceOffer);
}

static void
SpathiAngry (RESPONSE_REF R)
{
	if (R == 0)
	{
		NPCPhrase (MEAN_GUYS_RETURN);

		Response (we_apologize, SpathiAngry);
	}
	else /* if (R == we_apologize) */
	{
		NPCPhrase (DONT_BELIEVE);

		Response (misunderstanding, AllianceOffer);
	}

	Response (we_attack_again, ExitConversation);
	Response (bye_angry_spathi, ExitConversation);
}

static void
SpathiParty (RESPONSE_REF R)
{
	if (R == 0)
	{
		BYTE NumVisits;

		NumVisits = GET_GAME_STATE (SPATHI_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (MUST_PARTY_1);
				break;
			case 1:
				NPCPhrase (MUST_PARTY_2);
				break;
			case 2:
				NPCPhrase (MUST_PARTY_3);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (SPATHI_HOME_VISITS, NumVisits);
	}
	else if (PLAYER_SAID (R, deals_a_deal))
	{
		NPCPhrase (WAIT_A_WHILE);

		DISABLE_PHRASE (deals_a_deal);
	}
	else if (PLAYER_SAID (R, how_long))
	{
		NPCPhrase (TEN_YEARS);

		DISABLE_PHRASE (how_long);
	}
	else if (PLAYER_SAID (R, reneging))
	{
		NPCPhrase (ADULT_VIEW);

		DISABLE_PHRASE (reneging);
	}
	else if (PLAYER_SAID (R, return_beasts))
	{
		NPCPhrase (WHAT_RELATIONSHIP);

		DISABLE_PHRASE (return_beasts);
	}
	else if (PLAYER_SAID (R, minds_and_might))
	{
		NPCPhrase (HUH);

		DISABLE_PHRASE (minds_and_might);
	}
	else if (PLAYER_SAID (R, fellowship))
	{
		NPCPhrase (WHAT);

		DISABLE_PHRASE (fellowship);
	}

	if (PHRASE_ENABLED (deals_a_deal))
		Response (deals_a_deal, SpathiParty);
	else if (PHRASE_ENABLED (how_long))
		Response (how_long, SpathiParty);
	else if (PHRASE_ENABLED (reneging))
		Response (reneging, SpathiParty);
	else if (PHRASE_ENABLED (return_beasts))
		Response (return_beasts, SpathiParty);
	else
	{
		if (PHRASE_ENABLED (minds_and_might))
			Response (minds_and_might, SpathiParty);
		if (PHRASE_ENABLED (fellowship))
			Response (fellowship, SpathiParty);
		Response (do_as_we_say, ExitConversation);

		return;
	}
	switch (GET_GAME_STATE (SPATHI_HOME_VISITS) - 1)
	{
		case 0:
			Response (bye_from_party_1, ExitConversation);
			break;
		case 1:
			Response (bye_from_party_2, ExitConversation);
			break;
		default:
			Response (bye_from_party_3, ExitConversation);
			break;
	}
}

static void
SpathiCouncil (RESPONSE_REF R)
{
	if (R == 0)
		NPCPhrase (HELLO_AGAIN);
	else if (PLAYER_SAID (R, good_password))
	{
		NPCPhrase (YES_GOOD_PASSWORD);
		if (!IS_HD) {
			XFormColorMap (GetColorMapAddress (
					SetAbsColorMapIndex (CommData.AlienColorMap, 1)
					), ONE_SECOND / 4);
		} else {
			COUNT i = 0;
			COUNT limit = CommData.NumAnimations;
			
			for (i = 0; i < limit; i++)
				CommData.AlienAmbientArray[i].AnimFlags &= ~ANIM_DISABLED;
				
			CommData.AlienFrame = SetAbsFrameIndex 
				(CommData.AlienFrame, 0);
				
			CommData.AlienTalkDesc.AnimFlags &= ~PAUSE_TALKING;
		}

		SET_GAME_STATE (KNOW_SPATHI_PASSWORD, 1);
		SET_GAME_STATE (SPATHI_HOME_VISITS, 0);
	}
	else if (PLAYER_SAID (R, we_come_in_peace))
	{
		NPCPhrase (KILLED_SPATHI);

		DISABLE_PHRASE (we_come_in_peace);
	}
	else if (PLAYER_SAID (R, spathi_on_pluto))
	{
		NPCPhrase (WHERE_SPATHI);

		DISABLE_PHRASE (spathi_on_pluto);
	}
	else if (PLAYER_SAID (R, hostage))
	{
		NPCPhrase (GUN_TO_HEAD);

		SET_GAME_STATE (FOUND_PLUTO_SPATHI, 3);
		DISABLE_PHRASE (hostage);
	}
	else if (PLAYER_SAID (R, killed_fwiffo))
	{
		NPCPhrase (POOR_FWIFFO);

		SET_GAME_STATE (FOUND_PLUTO_SPATHI, 3);
		DISABLE_PHRASE (killed_fwiffo);
	}
	else if (PLAYER_SAID (R, fwiffo_fine))
	{
		NPCPhrase (NOT_LIKELY);

		R = killed_fwiffo;
		DISABLE_PHRASE (fwiffo_fine);
	}
	else if (PLAYER_SAID (R, surrender))
	{
		NPCPhrase (NO_SURRENDER);

		DISABLE_PHRASE (surrender);
	}

	if (GET_GAME_STATE (SPATHI_MANNER) == 0)
	{
		Response (we_come_in_peace, AllianceOffer);
	}
	else if (PHRASE_ENABLED (we_come_in_peace))
	{
		Response (we_come_in_peace, SpathiCouncil);
	}
	else
	{
		Response (misunderstanding, AllianceOffer);
	}
	if (GET_GAME_STATE (FOUND_PLUTO_SPATHI)
			&& GET_GAME_STATE (FOUND_PLUTO_SPATHI) < 3)
	{
		if (PHRASE_ENABLED (spathi_on_pluto))
			Response (spathi_on_pluto, SpathiCouncil);
		else if (HaveEscortShip (SPATHI_SHIP))
		{
			if (PHRASE_ENABLED (hostage))
				Response (hostage, SpathiCouncil);
		}
		else if (PHRASE_ENABLED (killed_fwiffo))
		{
			Response (killed_fwiffo, SpathiCouncil);
			if (PHRASE_ENABLED (fwiffo_fine))
				Response (fwiffo_fine, SpathiCouncil);
		}
	}
	if (PHRASE_ENABLED (surrender))
		Response (surrender, SpathiCouncil);
	else
	{
		Response (surrender_or_die, ExitConversation);
	}
	Response (bye_no_ally_offer, ExitConversation);
}

static void
SpathiPassword (RESPONSE_REF R)
{
	if (R == 0)
	{
		BYTE NumVisits;

		NumVisits = GET_GAME_STATE (SPATHI_HOME_VISITS);
		switch (NumVisits++)
		{
			default:
				NPCPhrase (WHAT_IS_PASSWORD);
				NumVisits = 1;
				break;
			case 1:
				NPCPhrase (WHAT_IS_PASSWORD_AGAIN);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (SPATHI_HOME_VISITS, NumVisits);
	}
	else if (PLAYER_SAID (R, what_do_i_get))
	{
		NPCPhrase (YOU_GET_TO_LIVE);

		DISABLE_PHRASE (what_do_i_get);
	}

	if (GET_GAME_STATE (FOUND_PLUTO_SPATHI)
			|| GET_GAME_STATE (KNOW_SPATHI_PASSWORD))
	{
		Response (good_password, SpathiCouncil);
		if (PHRASE_ENABLED (what_do_i_get))
		{
			Response (what_do_i_get, SpathiPassword);
		}
	}
	else
	{
		Response (we_are_vindicator, ExitConversation);
		Response (gort_merenga, ExitConversation);
		Response (guph_florp, ExitConversation);
		Response (wagngl_fthagn, ExitConversation);
		Response (pleeese, ExitConversation);
		Response (screw_password, ExitConversation);
	}
}

static void
Intro (void)
{
	BYTE Manner;

	if (IS_HD)
		CommData.AlienFrame = SetAbsFrameIndex 
			(CommData.AlienFrame, 59);

	Manner = GET_GAME_STATE (SPATHI_MANNER);
	if (Manner == 2)
	{
		NPCPhrase (HATE_YOU_FOREVER);
		setSegue (Segue_hostile);
	}
	else if (Manner == 1
			&& GET_GAME_STATE (KNOW_SPATHI_PASSWORD)
			&& (GET_GAME_STATE (FOUND_PLUTO_SPATHI)
			|| GET_GAME_STATE (SPATHI_HOME_VISITS) != 7))
	{
		SpathiAngry ((RESPONSE_REF)0);
	}
	else if (CheckAlliance (SPATHI_SHIP) == GOOD_GUY)
	{
		if (!IS_HD) {
			CommData.AlienColorMap =
 				SetAbsColorMapIndex (CommData.AlienColorMap, 1);
		} else {
			COUNT i = 0;
			COUNT limit = CommData.NumAnimations - 1;
			
			for (i = 0; i < limit; i++)
				CommData.AlienAmbientArray[i].AnimFlags &= ~ANIM_DISABLED;
			
			CommData.AlienFrame = SetAbsFrameIndex 
				(CommData.AlienFrame, 0);
				
			CommData.AlienTalkDesc.AnimFlags &= ~PAUSE_TALKING;
		}
		SpathiAllies ((RESPONSE_REF)0);
	}
	else if (GET_GAME_STATE (SPATHI_PARTY))
	{
		if (!IS_HD){
			CommData.AlienColorMap =
 				SetAbsColorMapIndex (CommData.AlienColorMap, 1);
		} else {
			COUNT i = 0;
			COUNT limit = CommData.NumAnimations - 1;
			
			for (i = 0; i < limit; i++)
				CommData.AlienAmbientArray[i].AnimFlags &= ~ANIM_DISABLED;
				
			CommData.AlienFrame = SetAbsFrameIndex 
				(CommData.AlienFrame, 0);
				
			CommData.AlienTalkDesc.AnimFlags &= ~PAUSE_TALKING;
		};
		SpathiParty ((RESPONSE_REF)0);
	}
	else if (GET_GAME_STATE (SPATHI_QUEST))
	{
		if (GET_GAME_STATE (LIED_ABOUT_CREATURES) < 2)
		{
			if (!IS_HD) {
				CommData.AlienColorMap =
 					SetAbsColorMapIndex (CommData.AlienColorMap, 1);
			} else {
				COUNT i = 0;
				COUNT limit = CommData.NumAnimations - 1;
			
				for (i = 0; i < limit; i++)
					CommData.AlienAmbientArray[i].AnimFlags &= ~ANIM_DISABLED;
				
				CommData.AlienFrame = SetAbsFrameIndex 
					(CommData.AlienFrame, 0);
				
				CommData.AlienTalkDesc.AnimFlags &= ~PAUSE_TALKING;
			}
			SpathiQuest ((RESPONSE_REF)0);
		}
		else
		{
			NPCPhrase (YOU_LIED_2);

			SET_GAME_STATE (SPATHI_MANNER, 2);
			setSegue (Segue_hostile);
		}
	}
	else if (GET_GAME_STATE (KNOW_SPATHI_QUEST))
	{
		if (!IS_HD) {
			CommData.AlienColorMap =
 				SetAbsColorMapIndex (CommData.AlienColorMap, 1);
		} else {
			COUNT i = 0;
			COUNT limit = CommData.NumAnimations - 1;
			
			for (i = 0; i < limit; i++)
				CommData.AlienAmbientArray[i].AnimFlags &= ~ANIM_DISABLED;
				
			CommData.AlienFrame = SetAbsFrameIndex 
				(CommData.AlienFrame, 0);
				
			CommData.AlienTalkDesc.AnimFlags &= ~PAUSE_TALKING;
		}
		LearnQuest ((RESPONSE_REF)0);
	}
	else if (GET_GAME_STATE (KNOW_SPATHI_PASSWORD)
			&& (GET_GAME_STATE (FOUND_PLUTO_SPATHI)
			|| GET_GAME_STATE (SPATHI_HOME_VISITS) != 7))
	{
		if (!IS_HD) {
			CommData.AlienColorMap =
 				SetAbsColorMapIndex (CommData.AlienColorMap, 1);
		} else {
			COUNT i = 0;
			COUNT limit = CommData.NumAnimations - 1;
			
			for (i = 0; i < limit; i++)
				CommData.AlienAmbientArray[i].AnimFlags &= ~ANIM_DISABLED;
				
			CommData.AlienFrame = SetAbsFrameIndex 
				(CommData.AlienFrame, 0);
				
			CommData.AlienTalkDesc.AnimFlags &= ~PAUSE_TALKING;
		}
		SpathiCouncil ((RESPONSE_REF)0);
	}
	else
	{
		SpathiPassword ((RESPONSE_REF)0);
	}
}

static COUNT
uninit_spahome (void)
{
	luaUqm_comm_uninit ();
	return (0);
}

static void
post_spahome_enc (void)
{
	BYTE Manner;

	if (getSegue () == Segue_hostile
			&& (Manner = GET_GAME_STATE (SPATHI_MANNER)) != 2)
	{
		SET_GAME_STATE (SPATHI_MANNER, 1);
		if (Manner != 1)
		{
			SET_GAME_STATE (SPATHI_VISITS, 0);
			SET_GAME_STATE (SPATHI_HOME_VISITS, 0);
		}
	}
}

LOCDATA*
init_spahome_comm ()
{
	static LOCDATA spahome_desc;
 	LOCDATA *retval;
	
	spahome_desc = RES_BOOL(spahome_desc_orig, spahome_desc_hd);

	spahome_desc.init_encounter_func = Intro;
	spahome_desc.post_encounter_func = post_spahome_enc;
	spahome_desc.uninit_encounter_func = uninit_spahome;

	luaUqm_comm_init (NULL, NULL_RESOURCE);
			// Initialise Lua for string interpolation. This will be
			// generalised in the future.

	spahome_desc.AlienTextBaseline.x = TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	spahome_desc.AlienTextBaseline.y = 0;
	spahome_desc.AlienTextWidth = SIS_TEXT_WIDTH - 16;

	// use alternate "Safe Ones" track if available
	spahome_desc.AlienAltSongRes = SPAHOME_MUSIC;
	spahome_desc.AlienSongFlags |= LDASF_USE_ALTERNATE;

	if (IS_HD) {
		COUNT i;
		COUNT limit = spahome_desc.NumAnimations;
	
		for (i = 0; i < limit; i++)
			spahome_desc.AlienAmbientArray[i].AnimFlags |= ANIM_DISABLED;
			
		spahome_desc.AlienTalkDesc.AnimFlags |= PAUSE_TALKING;
	}

	if (GET_GAME_STATE (SPATHI_MANNER) == 3)
	{
		setSegue (Segue_peace);
	}
	else
	{
		setSegue (Segue_hostile);
	}

	retval = &spahome_desc;

	return (retval);
}
