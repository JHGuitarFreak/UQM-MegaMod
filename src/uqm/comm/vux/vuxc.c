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

static LOCDATA vux_desc_orig =
{
	VUX_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	VUX_PMAP_ANIM, /* AlienFrame */
	VUX_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* (SIS_TEXT_WIDTH - 16) >> 1, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	VUX_COLOR_MAP, /* AlienColorMap */
	VUX_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	VUX_CONVERSATION_PHRASES, /* PlayerPhrases */
	17, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			12, /* StartIndex */
			3, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			15, /* StartIndex */
			5, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			20, /* StartIndex */
			14, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND / 30, 0, /* RestartRate */
			0, /* BlockMask */
		},
		{
			34, /* StartIndex */
			7, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			41, /* StartIndex */
			6, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			47, /* StartIndex */
			11, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			58, /* StartIndex */
			3, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			61, /* StartIndex */
			4, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			65, /* StartIndex */
			4, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			69, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			71, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			74, /* StartIndex */
			6, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND / 15, ONE_SECOND / 15, /* RestartRate */
			0, /* BlockMask */
		},
		{
			80, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND / 15, ONE_SECOND / 15, /* RestartRate */
			(1 << 14), /* BlockMask */
		},
		{
			85, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND / 15, ONE_SECOND / 15, /* RestartRate */
			0, /* BlockMask */
		},
		{
			90, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND / 15, ONE_SECOND / 15, /* RestartRate */
			(1 << 12), /* BlockMask */
		},
		{
			95, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 5, ONE_SECOND * 5,/* RestartRate */
			0, /* BlockMask */
		},
		{
			99, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 5, ONE_SECOND * 5,/* RestartRate */
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
		11, /* NumFrames */
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

static LOCDATA vux_desc_hd =
{
	VUX_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	VUX_PMAP_ANIM, /* AlienFrame */
	VUX_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* (SIS_TEXT_WIDTH - 16) >> 1, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	VUX_COLOR_MAP, /* AlienColorMap */
	VUX_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	VUX_CONVERSATION_PHRASES, /* PlayerPhrases */
	18, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			12, /* StartIndex */
			3, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			15, /* StartIndex */
			5, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			20, /* StartIndex */
			14, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND / 30, 0, /* RestartRate */
			0, /* BlockMask */
		},
		{
			34, /* StartIndex */
			7, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			41, /* StartIndex */
			6, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			47, /* StartIndex */
			11, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			58, /* StartIndex */
			3, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			61, /* StartIndex */
			4, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			65, /* StartIndex */
			4, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			69, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			71, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			74, /* StartIndex */
			6, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND / 15, ONE_SECOND / 15, /* RestartRate */
			0, /* BlockMask */
		},
		{
			80, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND / 15, ONE_SECOND / 15, /* RestartRate */
			(1 << 14), /* BlockMask */
		},
		{
			85, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND / 15, ONE_SECOND / 15, /* RestartRate */
			0, /* BlockMask */
		},
		{
			90, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND / 15, ONE_SECOND / 15, /* RestartRate */
			(1 << 12), /* BlockMask */
		},
		{
			95, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 5, ONE_SECOND * 5,/* RestartRate */
			0, /* BlockMask */
		},
		{
			99, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 5, ONE_SECOND * 5,/* RestartRate */
			0, /* BlockMask */
		},
		{
			103, /* StartIndex */
			13, /* NumFrames */
			CIRCULAR_ANIM | ONE_SHOT_ANIM | WAIT_TALKING | ANIM_DISABLED, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			0, 0,/* RestartRate */
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
		11, /* NumFrames */
		0, /* AnimFlags */
		ONE_SECOND / 15, 0, /* FrameRate */
		ONE_SECOND / 12, 0, /* RestartRate */
		(1 << 18), /* BlockMask */
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
	BYTE NumVisits;

	setSegue (Segue_hostile);

	if (PLAYER_SAID (R, ok_take_beast))
	{
		NPCPhrase (FOOL_AIEE0);
		NPCPhrase (FOOL_AIEE1);

		AlienTalkSegue (1);

		if (!IS_HD) {
			XFormColorMap (GetColorMapAddress (
					SetAbsColorMapIndex (CommData.AlienColorMap, 1)
					), ONE_SECOND / 4);
		} else {
			COUNT i = 0;
			COUNT limit = CommData.NumAnimations - 1;
			
			for (i = 0; i < limit; i++)
				CommData.AlienAmbientArray[i].AnimFlags |= ANIM_DISABLED;
				
			CommData.AlienAmbientArray[limit].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienFrame = SetAbsFrameIndex 
				(CommData.AlienFrame, 115);
				
			CommData.AlienTalkDesc.AnimFlags |= PAUSE_TALKING;
		}

		AlienTalkSegue ((COUNT)~0);

		SET_GAME_STATE (VUX_BEAST_ON_SHIP, 0);
		SET_GAME_STATE (ZEX_IS_DEAD, 1);
		setSegue (Segue_peace);
	}
	else if (PLAYER_SAID (R, try_any_way))
	{
		NPCPhrase (NOPE);

		SET_GAME_STATE (VUX_STACK_1, 4);
	}
	else if (PLAYER_SAID (R, kill_you_squids_1)
			|| PLAYER_SAID (R, kill_you_squids_2)
			|| PLAYER_SAID (R, kill_you_squids_3)
			|| PLAYER_SAID (R, kill_you_squids_4))
	{
		NPCPhrase (WE_FIGHT);

		NumVisits = GET_GAME_STATE (VUX_STACK_2) + 1;
		if (NumVisits <= 3)
		{
			SET_GAME_STATE (VUX_STACK_2, NumVisits);
		}
	}
	else if (PLAYER_SAID (R, cant_we_be_friends_1)
			|| PLAYER_SAID (R, cant_we_be_friends_2)
			|| PLAYER_SAID (R, cant_we_be_friends_3)
			|| PLAYER_SAID (R, cant_we_be_friends_4))
	{
		NumVisits = GET_GAME_STATE (VUX_STACK_3);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (NEVER_UGLY_HUMANS_1);
				break;
			case 1:
				NPCPhrase (NEVER_UGLY_HUMANS_2);
				break;
			case 2:
				NPCPhrase (NEVER_UGLY_HUMANS_3);
				break;
			case 3:
				NPCPhrase (NEVER_UGLY_HUMANS_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (VUX_STACK_3, NumVisits);
	}
	else if (PLAYER_SAID (R, bye_hostile_space))
	{
		NumVisits = GET_GAME_STATE (VUX_STACK_4);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (GOODBYE_AND_DIE_HOSTILE_SPACE_1);
				break;
			case 1:
				NPCPhrase (GOODBYE_AND_DIE_HOSTILE_SPACE_2);
				break;
			case 2:
				NPCPhrase (GOODBYE_AND_DIE_HOSTILE_SPACE_3);
				break;
			case 3:
				NPCPhrase (GOODBYE_AND_DIE_HOSTILE_SPACE_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (VUX_STACK_4, NumVisits);
	}
	else if (PLAYER_SAID (R, bye_zex))
	{
		NPCPhrase (GOODBYE_ZEX);

		setSegue (Segue_peace);
	}
	else
	{
		NumVisits = GET_GAME_STATE (VUX_STACK_1);
		switch (NumVisits++)
		{
			case 4:
				NPCPhrase (NOT_ACCEPTED_1);
				break;
			case 5:
				NPCPhrase (NOT_ACCEPTED_2);
				break;
			case 6:
				NPCPhrase (NOT_ACCEPTED_3);
				break;
			case 7:
				NPCPhrase (NOT_ACCEPTED_4);
				break;
			case 8:
				NPCPhrase (NOT_ACCEPTED_5);
				break;
			case 9:
				NPCPhrase (NOT_ACCEPTED_6);
				break;
			case 10:
				NPCPhrase (NOT_ACCEPTED_7);
				break;
			case 11:
				NPCPhrase (NOT_ACCEPTED_8);
				break;
			case 12:
				NPCPhrase (NOT_ACCEPTED_9);
				break;
			case 13:
				NPCPhrase (TRUTH);
				break;
		}
		SET_GAME_STATE (VUX_STACK_1, NumVisits);
	}
}

static void
Menagerie (RESPONSE_REF R)
{
	BYTE i, LastStack;
	RESPONSE_REF pStr[3];

	if (PLAYER_SAID (R, i_have_beast)
			|| PLAYER_SAID (R, why_trust_1)
			|| PLAYER_SAID (R, why_trust_2)
			|| PLAYER_SAID (R, why_trust_3))
	{
		if (PLAYER_SAID (R, i_have_beast))
			NPCPhrase (GIVE_BEAST);
		else if (PLAYER_SAID (R, why_trust_1))
		{
			NPCPhrase (TRUST_1);

			DISABLE_PHRASE (why_trust_1);
		}
		else if (PLAYER_SAID (R, why_trust_2))
		{
			NPCPhrase (TRUST_2);

			DISABLE_PHRASE (why_trust_2);
		}
		else if (PLAYER_SAID (R, why_trust_3))
		{
			NPCPhrase (TRUST_3);

			DISABLE_PHRASE (why_trust_3);
		}

		if (PHRASE_ENABLED (why_trust_1))
			Response (why_trust_1, Menagerie);
		else if (PHRASE_ENABLED (why_trust_2))
			Response (why_trust_2, Menagerie);
		else if (PHRASE_ENABLED (why_trust_3))
			Response (why_trust_3, Menagerie);
		Response (ok_take_beast, CombatIsInevitable);
	}
	else if (PLAYER_SAID (R, kill_you))
	{
		NPCPhrase (FIGHT_AGAIN);

		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, regardless))
	{
		NPCPhrase (THEN_FIGHT);

		setSegue (Segue_hostile);
		SET_GAME_STATE (ZEX_STACK_3, 2);
		SET_GAME_STATE (ZEX_VISITS, 0);
	}
	else
	{
		LastStack = 0;
		pStr[0] = pStr[1] = pStr[2] = 0;
		if (R == 0)
		{
			BYTE NumVisits;

			NumVisits = GET_GAME_STATE (ZEX_VISITS);
			if (GET_GAME_STATE (ZEX_STACK_3) >= 2)
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase (FIGHT_OR_TRADE_1);
						break;
					case 1:
						NPCPhrase (FIGHT_OR_TRADE_2);
						--NumVisits;
						break;
				}
			}
			else
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase (ZEX_HELLO_1);
						break;
					case 1:
						NPCPhrase (ZEX_HELLO_2);
						break;
					case 2:
						NPCPhrase (ZEX_HELLO_3);
						break;
					case 3:
						NPCPhrase (ZEX_HELLO_4);
						--NumVisits;
						break;
				}
			}
			SET_GAME_STATE (ZEX_VISITS, NumVisits);
		}
		else if (PLAYER_SAID (R, what_you_do_here))
		{
			NPCPhrase (MY_MENAGERIE);

			SET_GAME_STATE (ZEX_STACK_1, 1);
		}
		else if (PLAYER_SAID (R, what_about_menagerie))
		{
			NPCPhrase (NEED_NEW_CREATURE);

			SET_GAME_STATE (ZEX_STACK_1, 2);
		}
		else if (PLAYER_SAID (R, what_about_creature))
		{
			NPCPhrase (ABOUT_CREATURE);

			SET_GAME_STATE (KNOW_ZEX_WANTS_MONSTER, 1);
			SET_GAME_STATE (ZEX_STACK_1, 3);

			R = about_creature_again;
			DISABLE_PHRASE (what_about_creature);
		}
		else if (PLAYER_SAID (R, about_creature_again))
		{
			NPCPhrase (CREATURE_AGAIN);

			DISABLE_PHRASE (about_creature_again);
		}
		else if (PLAYER_SAID (R, why_dont_you_attack))
		{
			NPCPhrase (LIKE_YOU);

			LastStack = 1;
			SET_GAME_STATE (ZEX_STACK_2, 1);
		}
		else if (PLAYER_SAID (R, why_like_me))
		{
			NPCPhrase (LIKE_BECAUSE);

			LastStack = 1;
			SET_GAME_STATE (ZEX_STACK_2, 2);
		}
		else if (PLAYER_SAID (R, are_you_a_pervert))
		{
			NPCPhrase (CALL_ME_WHAT_YOU_WISH);

			SET_GAME_STATE (ZEX_STACK_2, 3);
		}
		else if (PLAYER_SAID (R, take_by_force))
		{
			NPCPhrase (PRECURSOR_DEVICE);

			LastStack = 2;
			SET_GAME_STATE (ZEX_STACK_3, 1);
		}
		else if (PLAYER_SAID (R, you_lied))
		{
			NPCPhrase (YUP_LIED);

			LastStack = 2;
			SET_GAME_STATE (ZEX_STACK_3, 3);
		}

		if (GET_GAME_STATE (KNOW_ZEX_WANTS_MONSTER)
				&& GET_GAME_STATE (VUX_BEAST_ON_SHIP))
			pStr[0] = i_have_beast;
		else
		{
			switch (GET_GAME_STATE (ZEX_STACK_1))
			{
				case 0:
					pStr[0] = what_you_do_here;
					break;
				case 1:
					pStr[0] = what_about_menagerie;
					break;
				case 2:
					pStr[0] = what_about_creature;
					break;
				case 3:
					if (PHRASE_ENABLED (about_creature_again))
						pStr[0] = about_creature_again;
					break;
			}
		}
		switch (GET_GAME_STATE (ZEX_STACK_2))
		{
			case 0:
				pStr[1] = why_dont_you_attack;
				break;
			case 1:
				pStr[1] = why_like_me;
				break;
			case 2:
				pStr[1] = are_you_a_pervert;
				break;
		}
		switch (GET_GAME_STATE (ZEX_STACK_3))
		{
			case 0:
				pStr[2] = take_by_force;
				break;
			case 1:
				pStr[2] = regardless;
				break;
			case 2:
				pStr[2] = you_lied;
				break;
			case 3:
				pStr[2] = kill_you;
				break;
		}

		if (pStr[LastStack])
			Response (pStr[LastStack], Menagerie);
		for (i = 0; i < 3; ++i)
		{
			if (i != LastStack && pStr[i])
				Response (pStr[i], Menagerie);
		}
		Response (bye_zex, CombatIsInevitable);
	}
}

static void
NormalVux (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, why_so_mean))
	{
		NPCPhrase (URQUAN_SLAVES);

		SET_GAME_STATE (VUX_STACK_1, 1);
	}
	else if (PLAYER_SAID (R, deeper_reason))
	{
		NPCPhrase (OLD_INSULT);

		SET_GAME_STATE (VUX_STACK_1, 2);
	}
	else if (PLAYER_SAID (R, if_we_apologize))
	{
		NPCPhrase (PROBABLY_NOT);

		SET_GAME_STATE (VUX_STACK_1, 3);
	}
	else if (PLAYER_SAID (R, whats_up_hostile))
	{
		BYTE NumVisits;

		NumVisits = GET_GAME_STATE (VUX_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (GENERAL_INFO_HOSTILE_1);
				break;
			case 1:
				NPCPhrase (GENERAL_INFO_HOSTILE_2);
				break;
			case 2:
				NPCPhrase (GENERAL_INFO_HOSTILE_3);
				break;
			case 3:
				NPCPhrase (GENERAL_INFO_HOSTILE_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (VUX_INFO, NumVisits);

		DISABLE_PHRASE (whats_up_hostile);
	}

	switch (GET_GAME_STATE (VUX_STACK_1))
	{
		case 0:
			Response (why_so_mean, NormalVux);
			break;
		case 1:
			Response (deeper_reason, NormalVux);
			break;
		case 2:
			Response (if_we_apologize, NormalVux);
			break;
		case 3:
			Response (try_any_way, CombatIsInevitable);
			break;
		case 4:
			Response (apology_1, CombatIsInevitable);
			break;
		case 5:
			Response (apology_2, CombatIsInevitable);
			break;
		case 6:
			Response (apology_3, CombatIsInevitable);
			break;
		case 7:
			Response (apology_4, CombatIsInevitable);
			break;
		case 8:
			Response (apology_5, CombatIsInevitable);
			break;
		case 9:
			Response (apology_6, CombatIsInevitable);
			break;
		case 10:
			Response (apology_7, CombatIsInevitable);
			break;
		case 11:
			Response (apology_8, CombatIsInevitable);
			break;
		case 12:
			Response (apology_9, CombatIsInevitable);
			break;
		case 13:
			Response (apology_10, CombatIsInevitable);
			break;
	}

	switch (GET_GAME_STATE (VUX_STACK_2))
	{
		case 0:
			Response (kill_you_squids_1, CombatIsInevitable);
			break;
		case 1:
			Response (kill_you_squids_2, CombatIsInevitable);
			break;
		case 2:
			Response (kill_you_squids_3, CombatIsInevitable);
			break;
		case 3:
			Response (kill_you_squids_4, CombatIsInevitable);
			break;
	}

	if (PHRASE_ENABLED (whats_up_hostile))
	{
		Response (whats_up_hostile, NormalVux);
	}

	if (GET_GAME_STATE (VUX_STACK_1) > 13)
	{
		switch (GET_GAME_STATE (VUX_STACK_3))
		{
			case 0:
				Response (cant_we_be_friends_1, CombatIsInevitable);
				break;
			case 1:
				Response (cant_we_be_friends_2, CombatIsInevitable);
				break;
			case 2:
				Response (cant_we_be_friends_3, CombatIsInevitable);
				break;
			case 3:
				Response (cant_we_be_friends_4, CombatIsInevitable);
				break;
		}
	}

	Response (bye_hostile_space, CombatIsInevitable);
}

static void
Intro (void)
{
	if (LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
	{
		NPCPhrase (OUT_TAKES);

		setSegue (Segue_peace);
		return;
	}

	if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		Menagerie ((RESPONSE_REF)0);
	}
	else
	{
		BYTE NumVisits;

		if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = GET_GAME_STATE (VUX_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (HOMEWORLD_HELLO_1);
					break;
				case 1:
					NPCPhrase (HOMEWORLD_HELLO_2);
					break;
				case 2:
					NPCPhrase (HOMEWORLD_HELLO_3);
					break;
				case 3:
					NPCPhrase (HOMEWORLD_HELLO_4);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (VUX_HOME_VISITS, NumVisits);
		}
		else
		{
			NumVisits = GET_GAME_STATE (VUX_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase (SPACE_HELLO_2);
					break;
				case 2:
					NPCPhrase (SPACE_HELLO_3);
					break;
				case 3:
					NPCPhrase (SPACE_HELLO_4);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (VUX_VISITS, NumVisits);
		}

		NormalVux ((RESPONSE_REF)0);
	}
}

static COUNT
uninit_vux (void)
{
	return (0);
}

static void
post_vux_enc (void)
{
	// nothing defined so far
}

LOCDATA*
init_vux_comm (void)
{
	static LOCDATA vux_desc;
 	LOCDATA *retval;
	
	vux_desc = RES_BOOL(vux_desc_orig, vux_desc_hd);

	if(GET_GAME_STATE(GLOBAL_FLAGS_AND_DATA) & (1 << 6)){
		// use alternate "ZEX" track if available
		vux_desc.AlienAltSongRes = VUX_ZEX_MUSIC;
		vux_desc.AlienSongFlags |= LDASF_USE_ALTERNATE;
	} else {
		// regular track -- let's make sure
		vux_desc.AlienSongFlags &= ~LDASF_USE_ALTERNATE;
	}

	vux_desc.init_encounter_func = Intro;
	vux_desc.post_encounter_func = post_vux_enc;
	vux_desc.uninit_encounter_func = uninit_vux;

	vux_desc.AlienTextBaseline.x = TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1)
			+ (SIS_TEXT_WIDTH >> 2);
	vux_desc.AlienTextBaseline.y = 0;
	vux_desc.AlienTextWidth = (SIS_TEXT_WIDTH - 16) >> 1;

	if ((GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 6))
			|| LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
	{
		setSegue (Segue_peace);
	}
	else
	{
		setSegue (Segue_hostile);
	}
	retval = &vux_desc;

	return (retval);
}
