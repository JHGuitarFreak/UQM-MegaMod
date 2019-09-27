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

#define STROBE_RATE   10
#define STROBE_LENGTH (ONE_SECOND * 3 / 2)
#define NUM_STROBES   (STROBE_LENGTH * STROBE_RATE / ONE_SECOND)

static LOCDATA talkpet_desc =
{
	TALKING_PET_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	TALKING_PET_PMAP_ANIM, /* AlienFrame */
	TALKING_PET_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	TALKING_PET_COLOR_MAP, /* AlienColorMap */
	TALKING_PET_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	TALKING_PET_CONVERSATION_PHRASES, /* PlayerPhrases */
	17, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			7, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM
					| WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			10, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM
					| WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			13, /* StartIndex */
			3, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			16, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{	/* Blink right eye */
			18, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 13) /* BlockMask */
		},
		{	/* Blink left eye */
			21, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 12) | (1 << 14), /* BlockMask */
		},
		{
			24, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			26, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			28, /* StartIndex */
			4, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			32, /* StartIndex */
			3, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			35, /* StartIndex */
			5, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			40, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			42, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM
					| WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 5), /* BlockMask */
		},
		{	/* Right eyebrow */
			48, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 4), /* BlockMask */
		},
		{	/* Left eyebrow */
			50, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 5), /* BlockMask */
		},
		{
			52, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM | WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{	/* Mind control strobe (on-demand) */
			1, /* StartIndex */
			NUM_STROBES * 2, /* NumFrames */
			CIRCULAR_ANIM | COLORXFORM_ANIM | ONE_SHOT_ANIM
					| ANIM_DISABLED, /* AnimFlags */
			ONE_SECOND / (STROBE_RATE * 2), 0, /* FrameRate */
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
		1, /* StartIndex */
		6, /* NumFrames */
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
	SET_GAME_STATE (SHIP_TO_COMPEL, 0);

	if (PLAYER_SAID (R, compel_urquan))
	{
		NPCPhrase (HERE_WE_GO);

		SET_GAME_STATE (URQUAN_MESSED_UP, 1);
	}
	else if (PLAYER_SAID (R, wish_to_go_now))
		NPCPhrase (EXCELLENT_IDEA);
	else if (PLAYER_SAID (R, bye_onboard))
		NPCPhrase (GOODBYE_ONBOARD);
	else if (PLAYER_SAID (R, compel_that_ship))
		NPCPhrase (SAVING_MY_POWER);
	else if (PLAYER_SAID (R, ok_lets_do_it)
			|| PLAYER_SAID (R, want_kill_1)
			|| PLAYER_SAID (R, want_kill_2)
			|| PLAYER_SAID (R, want_kill_3))
	{
		if (PLAYER_SAID (R, ok_lets_do_it))
			NPCPhrase (COMING_ABOARD);
		else
			NPCPhrase (GLAD_YOU_WONT_KILL);

		SET_GAME_STATE (TALKING_PET, 1);
		SET_GAME_STATE (TALKING_PET_ON_SHIP, 1);
		SET_GAME_STATE (UMGAH_ZOMBIE_BLOBBIES, 0);
		SET_GAME_STATE (UMGAH_VISITS, 0);
		SET_GAME_STATE (UMGAH_HOME_VISITS, 0);
		SET_GAME_STATE (ARILOU_STACK_2, 0);
	}
}

static void
MindFuckUrquan (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, what_now))
	{
		NPCPhrase (DO_THIS);

		DISABLE_PHRASE (what_now);
	}
	else if (PLAYER_SAID (R, im_scared))
	{
		NPCPhrase (STUPID_FOP);

		DISABLE_PHRASE (im_scared);
	}

	if (PHRASE_ENABLED (what_now))
		Response (what_now, MindFuckUrquan);
	if (PHRASE_ENABLED (im_scared))
		Response (im_scared, MindFuckUrquan);
	Response (compel_urquan, ExitConversation);
}

static void PetDevice (RESPONSE_REF R);

static void
MindControlStrobe (void)
{
	// Enable the one-shot strobe animation
	CommData.AlienAmbientArray[16].AnimFlags &= ~ANIM_DISABLED;
}

static void
MindControl (RESPONSE_REF R)
{
	RESPONSE_FUNC RespFunc;

	if (PLAYER_SAID (R, what_about_powers))
	{
		NPCPhrase (NOT_POWERS_BUT_FLOWERS);

		RespFunc = (RESPONSE_FUNC)MindControl;
		R = yes_flowers;
	}
	else /* if (R == yes_flowers) */
	{
		NPCPhrase (GOOD_HUMAN);

		RespFunc = (RESPONSE_FUNC)ExitConversation;
		R = wish_to_go_now;
	}

	AlienTalkSegue ((COUNT)~0);
	MindControlStrobe ();

	Response (R, RespFunc);
}

static void
PetInfo (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, about_your_race))
		NPCPhrase (WHAT_ABOUT_RACE);
	else if (PLAYER_SAID (R, what_about_physiology))
	{
		NPCPhrase (NO_TALK_ABOUT_SELF);

		DISABLE_PHRASE (what_about_physiology);
	}
	else if (PLAYER_SAID (R, what_about_your_history))
	{
		NPCPhrase (ABOUT_HISTORY);

		DISABLE_PHRASE (what_about_your_history);
	}
	else if (PLAYER_SAID (R, sentient_milieu))
	{
		NPCPhrase (ABOUT_SENTIENT_MILIEU);

		DISABLE_PHRASE (sentient_milieu);
	}
	else if (PLAYER_SAID (R, what_about_war))
	{
		NPCPhrase (ABOUT_WAR);

		DISABLE_PHRASE (what_about_war);
	}

	if (PHRASE_ENABLED (what_about_physiology))
	{
		Response (what_about_physiology, PetInfo);
	}
	else
	{
		Response (what_about_powers, MindControl);
	}
	if (PHRASE_ENABLED (what_about_your_history))
		Response (what_about_your_history, PetInfo);
	else if (PHRASE_ENABLED (sentient_milieu))
		Response (sentient_milieu, PetInfo);
	else if (PHRASE_ENABLED (what_about_war))
		Response (what_about_war, PetInfo);
	Response (enough_info, PetDevice);
}

static void
PetDevice (RESPONSE_REF R)
{
	BYTE NumVisits;

	if (PLAYER_SAID (R, whats_up_onboard))
	{
		NumVisits = GET_GAME_STATE (TALKING_PET_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (GENERAL_INFO_ONBOARD_1);
				break;
			case 1:
				NPCPhrase (GENERAL_INFO_ONBOARD_2);
				break;
			case 2:
				NPCPhrase (GENERAL_INFO_ONBOARD_3);
				break;
			case 3:
				NPCPhrase (GENERAL_INFO_ONBOARD_4);
				break;
			case 4:
				NPCPhrase (GENERAL_INFO_ONBOARD_5);
				break;
			case 5:
				NPCPhrase (GENERAL_INFO_ONBOARD_6);
				break;
			case 6:
				NPCPhrase (GENERAL_INFO_ONBOARD_7);
				break;
			case 7:
				NPCPhrase (GENERAL_INFO_ONBOARD_8);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (TALKING_PET_INFO, NumVisits);

		DISABLE_PHRASE (whats_up_onboard);
	}
	else if (PLAYER_SAID (R, any_suggestions))
	{
		NumVisits = GET_GAME_STATE (TALKING_PET_SUGGESTIONS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (SUGGESTION_1);
				break;
			case 1:
				NPCPhrase (SUGGESTION_2);
				break;
			case 2:
				NPCPhrase (SUGGESTION_3);
				break;
			case 3:
				NPCPhrase (SUGGESTION_4);
				break;
			case 4:
				NPCPhrase (SUGGESTION_5);
				break;
			case 5:
				NPCPhrase (SUGGESTION_6);
				break;
			case 6:
				NPCPhrase (SUGGESTION_7);
				break;
			case 7:
				NPCPhrase (SUGGESTION_8);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (TALKING_PET_SUGGESTIONS, NumVisits);

		DISABLE_PHRASE (any_suggestions);
	}
	else if (PLAYER_SAID (R, enough_info))
		NPCPhrase (OK_ENOUGH_INFO);
	else if (PLAYER_SAID (R, you_lied))
	{
		NPCPhrase (SO_WHAT);

		SET_GAME_STATE (DNYARRI_LIED, 0);
	}

	if (GET_GAME_STATE (SHIP_TO_COMPEL))
	{
		Response (compel_that_ship, ExitConversation);
	}
	if (PHRASE_ENABLED (whats_up_onboard))
		Response (whats_up_onboard, PetDevice);
	if (PHRASE_ENABLED (any_suggestions))
		Response (any_suggestions, PetDevice);
	Response (about_your_race, PetInfo);
	if (GET_GAME_STATE (DNYARRI_LIED) && GET_GAME_STATE (LEARNED_TALKING_PET))
		Response (you_lied, PetDevice);
	Response (bye_onboard, ExitConversation);
}

static void
CompelPlayer (RESPONSE_REF R)
{
	BYTE i, LastStack;
	RESPONSE_REF pStr[3];

	LastStack = 0;
	pStr[0] = pStr[1] = pStr[2] = 0;
	if (PLAYER_SAID (R, what_are_you))
	{
		NPCPhrase (JUST_TALKING_PET);

		DISABLE_PHRASE (what_are_you);
	}
	else if (PLAYER_SAID (R, what_do_to_umgah))
	{
		NPCPhrase (DID_NOTHING);

		DISABLE_PHRASE (what_do_to_umgah);
		LastStack = 1;
	}
	else if (PLAYER_SAID (R, we_are_vindicator))
	{
		NPCPhrase (GOOD_FOR_YOU);

		DISABLE_PHRASE (we_are_vindicator);
		LastStack = 2;
	}
	else if (R != 0)
	{
		if (PLAYER_SAID (R, bye_at_umgah))
			NPCPhrase (GOODBYE_AT_UMGAH);
		else if (PLAYER_SAID (R, must_explain_presence))
			NPCPhrase (EXPLAIN_NOTHING_MONKEY_BOY);
		else if (PLAYER_SAID (R, umgah_zombies))
			NPCPhrase (WORKS_LIKE_THIS);
		else if (PLAYER_SAID (R, talking_pets_dumb))
			NPCPhrase (OH_NO_YOU_DONT);

		SET_GAME_STATE (KNOW_UMGAH_ZOMBIES, 1);
		if (!GET_GAME_STATE (TAALO_PROTECTOR_ON_SHIP))
		{
			SET_GAME_STATE (PLAYER_HYPNOTIZED, 1);
		}
		else
		{
			NPCPhrase (CANT_COMPEL);

			setSegue (Segue_hostile);
		}

		return;
	}

	if (PHRASE_ENABLED (what_are_you))
		pStr[0] = what_are_you;
	else
		pStr[0] = talking_pets_dumb;
	if (GET_GAME_STATE (KNOW_UMGAH_ZOMBIES))
	{
		if (PHRASE_ENABLED (what_do_to_umgah))
			pStr[1] = what_do_to_umgah;
		else
			pStr[1] = umgah_zombies;
	}
	if (PHRASE_ENABLED (we_are_vindicator))
		pStr[2] = we_are_vindicator;
	else
		pStr[2] = must_explain_presence;

	if (pStr[LastStack])
		Response (pStr[LastStack], CompelPlayer);
	for (i = 0; i < 3; ++i)
	{
		if (i != LastStack && pStr[i])
			Response (pStr[i], CompelPlayer);
	}
	Response (bye_at_umgah, CompelPlayer);
}

static void PetDeal (RESPONSE_REF R);

static void
KillPet (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, must_kill))
	{
		NPCPhrase (DONT_KILL);
		AlienTalkSegue ((COUNT)~0);

		MindControlStrobe ();
	}

	Response (want_kill_1, ExitConversation);
	Response (want_kill_2, ExitConversation);
	Response (want_kill_3, ExitConversation);
}

static void
PetDeal (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, what_kind_of_deal))
	{
		NPCPhrase (HELP_DEFEAT_URQUAN);

		DISABLE_PHRASE (what_kind_of_deal);
	}
	else if (PLAYER_SAID (R, how_trust))
	{
		NPCPhrase (TRUST);

		DISABLE_PHRASE (how_trust);
	}
	else if (PLAYER_SAID (R, boneless_dweeb))
	{
		NPCPhrase (YOUR_BONELESS_DWEEB);

		DISABLE_PHRASE (boneless_dweeb);
	}
	else if (PLAYER_SAID (R, what_are_you_really))
	{
		NPCPhrase (POOR_DNYARRI);

		DISABLE_PHRASE (what_are_you_really);
	}
	else if (PLAYER_SAID (R, hard_to_believe))
	{
		NPCPhrase (ITS_TRUE);

		SET_GAME_STATE (DNYARRI_LIED, 1);
		DISABLE_PHRASE (hard_to_believe);
	}
	else if (PLAYER_SAID (R, bullshit))
	{
		NPCPhrase (WORTH_A_TRY);

		DISABLE_PHRASE (bullshit);
	}
	else if (PLAYER_SAID (R, kill_you))
	{
		NPCPhrase (PLEASE_DONT);

		DISABLE_PHRASE (kill_you);
	}

	if (PHRASE_ENABLED (what_kind_of_deal))
		Response (what_kind_of_deal, PetDeal);
	else
	{
		if (PHRASE_ENABLED (how_trust))
			Response (how_trust, PetDeal);
		else if (PHRASE_ENABLED (boneless_dweeb))
			Response (boneless_dweeb, PetDeal);
		Response (ok_lets_do_it, ExitConversation);
	}
	if (PHRASE_ENABLED (what_are_you_really))
		Response (what_are_you_really, PetDeal);
	else
	{
		if (PHRASE_ENABLED (hard_to_believe) && !GET_GAME_STATE (LEARNED_TALKING_PET))
			Response (hard_to_believe, PetDeal);
		else if (PHRASE_ENABLED (bullshit) && GET_GAME_STATE (LEARNED_TALKING_PET))
			Response (bullshit, PetDeal);
	}
	if (PHRASE_ENABLED (kill_you))
		Response (kill_you, PetDeal);
	else if (PHRASE_ENABLED (must_kill))
	{
		Response (must_kill, KillPet);
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

	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
	{
		SET_GAME_STATE (SHIP_TO_COMPEL, 0);
		setSegue (Segue_hostile);
		if (!(GLOBAL (glob_flags) & CYBORG_ENABLED))
		{
			NPCPhrase (HUMAN_PEP_TALK);
		}
		else
		{
			NPCPhrase (CYBORG_PEP_TALK);
		}
	}
	else if (GET_GAME_STATE (READY_TO_CONFUSE_URQUAN))
	{
		SET_GAME_STATE (SHIP_TO_COMPEL, 0);
		SET_GAME_STATE (READY_TO_CONFUSE_URQUAN, 0);
		SET_GAME_STATE (AWARE_OF_SAMATRA, 1);
		if (GET_GAME_STATE (CHMMR_BOMB_STATE) != 3)
		{
			NPCPhrase (HAVENT_GOT_EVERYTHING);
			if (!GET_GAME_STATE (UTWIG_BOMB_ON_SHIP))
				NPCPhrase (NEED_BOMB);
			else
				NPCPhrase (SOUP_UP_BOMB);

			setSegue (Segue_peace);
		}
		else if (GET_GAME_STATE (URQUAN_MESSED_UP))
		{
			NPCPhrase (HELLO_AFTER_COMPEL_URQUAN);

			setSegue (Segue_peace);
		}
		else
		{
			NPCPhrase (I_SENSE_MY_SLAVES);

			MindFuckUrquan ((RESPONSE_REF)0);
		}
	}
	else if (GET_GAME_STATE (TALKING_PET_ON_SHIP))
	{
		NumVisits = GET_GAME_STATE (TALKING_PET_VISITS);
		
		// You can acquire the Talking Pet without having Taalo Shield after
		// the Kohr-Ah wipe out the Umgah. In that case, the Pet will join
		// you willingly, but his complaints about the Taalo shield as in
		// HELLO_AS_DEVICE_1 do not make any sense.
		if (!GET_GAME_STATE (TAALO_PROTECTOR_ON_SHIP) && NumVisits == 0)
			++NumVisits; // skip HELLO_AS_DEVICE_1

		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (HELLO_AS_DEVICE_1);
				break;
			case 1:
				NPCPhrase (HELLO_AS_DEVICE_2);
				break;
			case 2:
				NPCPhrase (HELLO_AS_DEVICE_3);
				break;
			case 3:
				NPCPhrase (HELLO_AS_DEVICE_4);
				break;
			case 4:
				NPCPhrase (HELLO_AS_DEVICE_5);
				break;
			case 5:
				NPCPhrase (HELLO_AS_DEVICE_6);
				break;
			case 6:
				NPCPhrase (HELLO_AS_DEVICE_7);
				break;
			case 7:
				NPCPhrase (HELLO_AS_DEVICE_8);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (TALKING_PET_VISITS, NumVisits);

		PetDevice ((RESPONSE_REF)0);
	}
	else if (GetHeadLink (&GLOBAL (npc_built_ship_q)))
	{
		NumVisits = GET_GAME_STATE (TALKING_PET_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				SET_GAME_STATE (UMGAH_VISITS, 0);
				NPCPhrase (HELLO_AT_UMGAH);
				break;
			case 1:
				NPCPhrase (HYPNOTIZE_AGAIN_1);
				break;
			case 2:
				NPCPhrase (HYPNOTIZE_AGAIN_2);
				break;
			case 3:
				NPCPhrase (HYPNOTIZE_AGAIN_3);
				break;
			case 4:
				NPCPhrase (HYPNOTIZE_AGAIN_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (TALKING_PET_HOME_VISITS, NumVisits);

		if (NumVisits == 1)
		{
			CompelPlayer ((RESPONSE_REF)0);
		}
		else if (!GET_GAME_STATE (TAALO_PROTECTOR_ON_SHIP))
		{
			SET_GAME_STATE (PLAYER_HYPNOTIZED, 1);
			setSegue (Segue_peace);
		}
		else
		{
			NPCPhrase (CANT_COMPEL);

			setSegue (Segue_hostile);
		}
	}
	else
	{
		if (StartSphereTracking (UMGAH_SHIP))
		{
			NPCPhrase (LETS_MAKE_A_DEAL);
		}
		else
		{
			NPCPhrase (UMGAH_ALL_GONE);

			if (GET_GAME_STATE (TALKING_PET_HOME_VISITS) == 0
					|| !GET_GAME_STATE (TAALO_PROTECTOR_ON_SHIP))
			{	// The how_trust-TRUST exchange only makes sense when the
				// player visited the Talking Pet before so he tried to
				// kill the player *and* the player has the Taalo shield.
				DISABLE_PHRASE (how_trust);
			}
		}

		PetDeal ((RESPONSE_REF)0);
	}
}

static COUNT
uninit_talkpet (void)
{
	luaUqm_comm_uninit ();
	return (0);
}

static void
post_talkpet_enc (void)
{
	// nothing defined so far
}

LOCDATA*
init_talkpet_comm (void)
{
	LOCDATA *retval;

	talkpet_desc.init_encounter_func = Intro;
	talkpet_desc.post_encounter_func = post_talkpet_enc;
	talkpet_desc.uninit_encounter_func = uninit_talkpet;

	luaUqm_comm_init (NULL, NULL_RESOURCE);
			// Initialise Lua for string interpolation. This will be 
			// generalised in the future.

	talkpet_desc.AlienTextBaseline.x = TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	talkpet_desc.AlienTextBaseline.y = 0;
	talkpet_desc.AlienTextWidth = SIS_TEXT_WIDTH - 16;

	if (LOBYTE (GLOBAL (CurrentActivity)) != IN_LAST_BATTLE)
	{
		setSegue (Segue_peace);
	}
	else
	{
		setSegue (Segue_hostile);
	}

	retval = &talkpet_desc;

	return (retval);
}
