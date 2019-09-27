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


static LOCDATA umgah_desc =
{
	UMGAH_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	UMGAH_PMAP_ANIM, /* AlienFrame */
	UMGAH_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	UMGAH_COLOR_MAP, /* AlienColorMap */
	UMGAH_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	UMGAH_CONVERSATION_PHRASES, /* PlayerPhrases */
	16, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			5, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 5),
		},
		{
			8, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 6),
		},
		{
			11, /* StartIndex */
			2, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 7),
		},
		{
			13, /* StartIndex */
			2, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 8),
		},
		{
			15, /* StartIndex */
			2, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 9),
		},
		{
			17, /* StartIndex */
			3, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND * 7 / 60, 0, /* FrameRate */
			ONE_SECOND * 3, ONE_SECOND * 3, /* RestartRate */
			(1 << 0),
		},
		{
			20, /* StartIndex */
			3, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND * 7 / 60, 0, /* FrameRate */
			ONE_SECOND * 3, ONE_SECOND * 3, /* RestartRate */
			(1 << 1),
		},
		{
			23, /* StartIndex */
			2, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND * 7 / 60, 0, /* FrameRate */
			ONE_SECOND * 3, ONE_SECOND * 3, /* RestartRate */
			(1 << 2),
		},
		{
			25, /* StartIndex */
			2, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND * 7 / 60, 0, /* FrameRate */
			ONE_SECOND * 3, ONE_SECOND * 3, /* RestartRate */
			(1 << 3),
		},
		{
			27, /* StartIndex */
			2, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND * 7 / 60, 0, /* FrameRate */
			ONE_SECOND * 3, ONE_SECOND * 3, /* RestartRate */
			(1 << 4),
		},
		{
			29, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			32, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			35, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			40, /* StartIndex */
			6, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND / 10, 0, /* RestartRate */
			0, /* BlockMask */
		},
		{
			46, /* StartIndex */
			2, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 5, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 15), /* BlockMask */
		},
		{
			48, /* StartIndex */
			2, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 5, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 14), /* BlockMask */
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
		4, /* NumFrames */
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
CombatIsInevitable (RESPONSE_REF R)
{
	setSegue (Segue_hostile);

	if (PLAYER_SAID (R, bye_zombie))
	{
		NPCPhrase (GOODBYE_ZOMBIE);

		setSegue (Segue_peace);
	}
	else if (PLAYER_SAID (R, bye_pre_zombie))
		NPCPhrase (GOODBYE_PRE_ZOMBIE);
	else if (PLAYER_SAID (R, can_we_be_friends))
	{
		NPCPhrase (SURE_FRIENDS);

		SET_GAME_STATE (UMGAH_MENTIONED_TRICKS, 1);
	}
	else if (PLAYER_SAID (R, evil_blobbies_give_up))
	{
		NPCPhrase (NOT_EVIL_BLOBBIES);

		SET_GAME_STATE (UMGAH_EVIL_BLOBBIES, 1);
	}
	else if (PLAYER_SAID (R, evil_blobbies_must_die))
		NPCPhrase (OH_NO_WE_WONT);
	else if (PLAYER_SAID (R, threat))
		NPCPhrase (NO_THREAT);
	else if (PLAYER_SAID (R, dont_believe))
	{
		NPCPhrase (THEN_DIE);

		SET_GAME_STATE (KNOW_UMGAH_ZOMBIES, 1);
		SET_GAME_STATE (UMGAH_VISITS, 0);
	}
	else if (PLAYER_SAID (R, bye_unknown))
	{
		NPCPhrase (GOODBYE_UNKNOWN);

		setSegue (Segue_peace);
	}
	else if (PLAYER_SAID (R, bye_post_zombie))
	{
		NPCPhrase (FUNNY_IDEA);

		AlienTalkSegue ((COUNT)~0);
		AddEscortShips (UMGAH_SHIP, 4);
		SET_GAME_STATE (UMGAH_HOSTILE, 1);
	}
}

static void
Zombies (RESPONSE_REF R)
{
	if (GET_GAME_STATE (MET_NORMAL_UMGAH))
	{
		if (PLAYER_SAID (R, whats_up_zombies))
		{
			NPCPhrase (GENERAL_INFO_ZOMBIE);

			DISABLE_PHRASE (whats_up_zombies);
		}
		else if (PLAYER_SAID (R, how_goes_tpet))
		{
			NPCPhrase (WHAT_TPET);

			DISABLE_PHRASE (how_goes_tpet);
		}
		else if (PLAYER_SAID (R, you_told_us))
		{
			NPCPhrase (SADLY_IT_DIED);

			DISABLE_PHRASE (you_told_us);
		}

		if (PHRASE_ENABLED (whats_up_zombies) && PHRASE_ENABLED (how_goes_tpet))
			Response (whats_up_zombies, Zombies);
		if (PHRASE_ENABLED (how_goes_tpet))
			Response (how_goes_tpet, Zombies);
		else if (PHRASE_ENABLED (you_told_us))
			Response (you_told_us, Zombies);
		else
		{
			Response (dont_believe, CombatIsInevitable);
		}
		if (PHRASE_ENABLED (whats_up_zombies) && !PHRASE_ENABLED (how_goes_tpet))
			Response (whats_up_zombies, Zombies);
		Response (threat, CombatIsInevitable);
		Response (bye_unknown, CombatIsInevitable);
	}
	else
	{
		BYTE i, LastStack;
		RESPONSE_REF pStr[4];

		LastStack = 0;
		pStr[0] = pStr[1] = pStr[2] = pStr[3] = 0;
		if (PLAYER_SAID (R, evil_blobbies))
		{
			NPCPhrase (YES_VERY_EVIL);

			DISABLE_PHRASE (evil_blobbies);
			LastStack = 0;
		}
		else if (PLAYER_SAID (R, we_vindicator))
		{
			NPCPhrase (GOOD_FOR_YOU_1);

			DISABLE_PHRASE (we_vindicator);
			LastStack = 1;
		}
		else if (PLAYER_SAID (R, come_in_peace))
		{
			NPCPhrase (GOOD_FOR_YOU_2);

			DISABLE_PHRASE (come_in_peace);
			LastStack = 1;
		}
		else if (PLAYER_SAID (R, know_any_jokes))
		{
			NPCPhrase (JOKE_1);

			DISABLE_PHRASE (know_any_jokes);
			LastStack = 2;
		}
		else if (PLAYER_SAID (R, better_joke))
		{
			NPCPhrase (JOKE_2);

			DISABLE_PHRASE (better_joke);
			LastStack = 2;
		}
		else if (PLAYER_SAID (R, not_very_funny))
		{
			NPCPhrase (YES_WE_ARE);

			DISABLE_PHRASE (not_very_funny);
			LastStack = 2;
		}
		else if (PLAYER_SAID (R, what_about_tpet))
		{
			NPCPhrase (WHAT_TPET);

			DISABLE_PHRASE (what_about_tpet);
			LastStack = 3;
		}
		else if (PLAYER_SAID (R, give_up_or_die))
		{
			NPCPhrase (NOT_GIVE_UP);

			setSegue (Segue_hostile);
			return;
		}
		else if (PLAYER_SAID (R, arilou_told_us))
		{
			NPCPhrase (THEN_DIE);

			setSegue (Segue_hostile);
			SET_GAME_STATE (KNOW_UMGAH_ZOMBIES, 1);
			SET_GAME_STATE (UMGAH_VISITS, 0);
			return;
		}

		if (PHRASE_ENABLED (evil_blobbies))
			pStr[0] = evil_blobbies;
		else
			pStr[0] = give_up_or_die;

		if (PHRASE_ENABLED (we_vindicator))
			pStr[1] = we_vindicator;
		else if (PHRASE_ENABLED (come_in_peace))
			pStr[1] = come_in_peace;

		if (PHRASE_ENABLED (know_any_jokes))
			pStr[2] = know_any_jokes;
		else if (PHRASE_ENABLED (better_joke))
			pStr[2] = better_joke;
		else if (PHRASE_ENABLED (not_very_funny))
			pStr[2] = not_very_funny;

		if (PHRASE_ENABLED (what_about_tpet))
			pStr[3] = what_about_tpet;
		else
			pStr[3] = arilou_told_us;

		if (pStr[LastStack])
			Response (pStr[LastStack], Zombies);
		for (i = 0; i < 4; ++i)
		{
			if (i != LastStack && pStr[i])
				Response (pStr[i], Zombies);
		}
		Response (bye_zombie, CombatIsInevitable);
	}
}

static void
NormalUmgah (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, whats_up_pre_zombie))
	{
		NPCPhrase (GENERAL_INFO_PRE_ZOMBIE);

		DISABLE_PHRASE (whats_up_pre_zombie);
	}
	else if (PLAYER_SAID (R, want_to_defeat_urquan))
	{
		NPCPhrase (FINE_BY_US);

		DISABLE_PHRASE (want_to_defeat_urquan);
	}

	if (!GET_GAME_STATE (UMGAH_EVIL_BLOBBIES))
		Response (evil_blobbies_give_up, CombatIsInevitable);
	else
		Response (evil_blobbies_must_die, CombatIsInevitable);
	if (PHRASE_ENABLED (whats_up_pre_zombie))
		Response (whats_up_pre_zombie, NormalUmgah);
	if (PHRASE_ENABLED (want_to_defeat_urquan))
		Response (want_to_defeat_urquan, NormalUmgah);
	switch (GET_GAME_STATE (UMGAH_MENTIONED_TRICKS))
	{
		case 0:
			Response (can_we_be_friends, CombatIsInevitable);
			break;
	}
	Response (bye_pre_zombie, CombatIsInevitable);
}

static void
UmgahReward (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, what_before_tpet))
	{
		NPCPhrase (TRKD_SPATHI_AND_ILWRATH);

		DISABLE_PHRASE (what_before_tpet);
	}
	else if (PLAYER_SAID (R, where_caster))
	{
		NPCPhrase (SPATHI_TOOK_THEM);

		DISABLE_PHRASE (where_caster);
	}
	else if (PLAYER_SAID (R, owe_me_big_time))
	{
		NPCPhrase (THANKS);

		GLOBAL_SIS (TotalBioMass) += 1000 / BIO_CREDIT_VALUE;
		DISABLE_PHRASE (owe_me_big_time);
		DISABLE_PHRASE (our_largesse);
	}
	else if (PLAYER_SAID (R, our_largesse))
	{
		NPCPhrase (GIVE_LIFEDATA);

		GLOBAL_SIS (TotalBioMass) += 1000 / BIO_CREDIT_VALUE;
		DISABLE_PHRASE (our_largesse);
		DISABLE_PHRASE (owe_me_big_time);
	}
	else if (PLAYER_SAID (R, what_do_with_tpet))
	{
		NPCPhrase (TRICK_URQUAN);

		DISABLE_PHRASE (what_do_with_tpet);
	}
	else if (PLAYER_SAID (R, any_jokes))
	{
		NPCPhrase (SURE);

		DISABLE_PHRASE (any_jokes);
	}
	else if (PLAYER_SAID (R, so_what_for_now))
	{
		NPCPhrase (DO_THIS_NOW);

		DISABLE_PHRASE (so_what_for_now);
	}

	if (!GET_GAME_STATE (MET_NORMAL_UMGAH))
	{
		if (PHRASE_ENABLED (what_before_tpet))
			Response (what_before_tpet, UmgahReward);
		else if (PHRASE_ENABLED (where_caster))
			Response (where_caster, UmgahReward);
	}
	if (PHRASE_ENABLED (owe_me_big_time))
	{
		Response (owe_me_big_time, UmgahReward);
		Response (our_largesse, UmgahReward);
	}
	if (PHRASE_ENABLED (what_do_with_tpet))
		Response (what_do_with_tpet, UmgahReward);
	else if (PHRASE_ENABLED (any_jokes) && GET_GAME_STATE (UMGAH_MENTIONED_TRICKS) < 2)
		Response (any_jokes, UmgahReward);
	if (PHRASE_ENABLED (so_what_for_now))
		Response (so_what_for_now, UmgahReward);
	Response (bye_post_zombie, CombatIsInevitable);
}

static void
Intro (void)
{
	BYTE NumVisits;


	if (LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE) {
		NPCPhrase (OUT_TAKES);
		SET_GAME_STATE (BATTLE_SEGUE, 0);
		return;
	} else if (GET_GAME_STATE (UMGAH_HOSTILE)) {
		NumVisits = GET_GAME_STATE (UMGAH_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (HOSTILE_HELLO_1);
				break;
			case 1:
				NPCPhrase (HOSTILE_HELLO_2);
				break;
			case 2:
				NPCPhrase (HOSTILE_HELLO_3);
				break;
			case 3:
				NPCPhrase (HOSTILE_HELLO_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (UMGAH_VISITS, NumVisits);

		setSegue (Segue_hostile);
	}
	else if (GET_GAME_STATE (UMGAH_ZOMBIE_BLOBBIES))
	{
		NumVisits = GET_GAME_STATE (UMGAH_VISITS);
		if (GET_GAME_STATE (TALKING_PET_VISITS))
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (DESTROY_INTERFERER_1);
					break;
				case 1:
					NPCPhrase (DESTROY_INTERFERER_2);
					break;
				case 2:
					NPCPhrase (DESTROY_INTERFERER_3);
					break;
				case 3:
					NPCPhrase (DESTROY_INTERFERER_4);
					--NumVisits;
					break;
			}

			setSegue (Segue_hostile);
		}
		else if (GET_GAME_STATE (KNOW_UMGAH_ZOMBIES))
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (REVEALED_ZOMBIE_HELLO_1);
					break;
				case 1:
					NPCPhrase (REVEALED_ZOMBIE_HELLO_2);
					break;
				case 2:
					NPCPhrase (REVEALED_ZOMBIE_HELLO_3);
					break;
				case 3:
					NPCPhrase (REVEALED_ZOMBIE_HELLO_4);
					--NumVisits;
					break;
			}

			setSegue (Segue_hostile);
		}
		else
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (UNKNOWN_ZOMBIE_HELLO_1);
					break;
				case 1:
					NPCPhrase (UNKNOWN_ZOMBIE_HELLO_2);
					break;
				case 2:
					NPCPhrase (UNKNOWN_ZOMBIE_HELLO_3);
					break;
				case 3:
					NPCPhrase (UNKNOWN_ZOMBIE_HELLO_4);
					--NumVisits;
					break;
			}

			Zombies ((RESPONSE_REF)0);
		}
		SET_GAME_STATE (UMGAH_VISITS, NumVisits);
	}
	else if (!GET_GAME_STATE (TALKING_PET))
	{
		if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = GET_GAME_STATE (UMGAH_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (HWLD_PRE_ZOMBIE_HELLO_1);
					break;
				case 1:
					NPCPhrase (HWLD_PRE_ZOMBIE_HELLO_2);
					break;
				case 2:
					NPCPhrase (HWLD_PRE_ZOMBIE_HELLO_3);
					break;
				case 3:
					NPCPhrase (HWLD_PRE_ZOMBIE_HELLO_4);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (UMGAH_HOME_VISITS, NumVisits);
		}
		else
		{
			NumVisits = GET_GAME_STATE (UMGAH_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (SPACE_PRE_ZOMBIE_HELLO_1);
					break;
				case 1:
					NPCPhrase (SPACE_PRE_ZOMBIE_HELLO_2);
					break;
				case 2:
					NPCPhrase (SPACE_PRE_ZOMBIE_HELLO_3);
					break;
				case 3:
					NPCPhrase (SPACE_PRE_ZOMBIE_HELLO_4);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (UMGAH_VISITS, NumVisits);
		}

		NormalUmgah ((RESPONSE_REF)0);
	}
	else
	{
		if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NPCPhrase (POST_ZOMBIE_HWLD_HELLO);

			UmgahReward ((RESPONSE_REF)0);
		}
		else
		{
			NumVisits = GET_GAME_STATE (UMGAH_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (REWARD_AT_HOMEWORLD_1);
					break;
				case 1:
					NPCPhrase (REWARD_AT_HOMEWORLD_2);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (UMGAH_VISITS, NumVisits);

			setSegue (Segue_peace);
		}
	}
}

static COUNT
uninit_umgah (void)
{
	luaUqm_comm_uninit ();
	return (0);
}

static void
post_umgah_enc (void)
{
	if (!GET_GAME_STATE (UMGAH_ZOMBIE_BLOBBIES))
	{
		SET_GAME_STATE (MET_NORMAL_UMGAH, 1);
	}
}

LOCDATA*
init_umgah_comm (void)
{
	LOCDATA *retval;

	umgah_desc.init_encounter_func = Intro;
	umgah_desc.post_encounter_func = post_umgah_enc;
	umgah_desc.uninit_encounter_func = uninit_umgah;

	luaUqm_comm_init (NULL, NULL_RESOURCE);
			// Initialise Lua for string interpolation. This will be
			// generalised in the future.

	umgah_desc.AlienTextBaseline.x = TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	umgah_desc.AlienTextBaseline.y = 0;
	umgah_desc.AlienTextWidth = SIS_TEXT_WIDTH - 16;

	if ((GET_GAME_STATE (TALKING_PET) && !GET_GAME_STATE (UMGAH_HOSTILE))
			|| LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
	{
		setSegue (Segue_peace);
	}
	else
	{
		setSegue (Segue_hostile);
	}
	retval = &umgah_desc;

	return (retval);
}
