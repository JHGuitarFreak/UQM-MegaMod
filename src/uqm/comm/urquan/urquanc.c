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

static LOCDATA urquan_desc =
{
	URQUAN_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	URQUAN_PMAP_ANIM, /* AlienFrame */
	URQUAN_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	URQUAN_COLOR_MAP, /* AlienColorMap */
	URQUAN_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	URQUAN_CONVERSATION_PHRASES, /* PlayerPhrases */
	7, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			7, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND / 10, 0, /* RestartRate */
			0, /* BlockMask */
		},
		{
			13, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			16, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			19, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			22, /* StartIndex */
			7, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			29, /* StartIndex */
			7, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			36, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
	},
	{ /* AlienTransitionDesc */
		1, /* StartIndex */
		2, /* NumFrames */
		0, /* AnimFlags */
		ONE_SECOND / 6, 0, /* FrameRate */
		0, 0, /* RestartRate */
		0, /* BlockMask */
	},
	{ /* AlienTalkDesc */
		2, /* StartIndex */
		5, /* NumFrames */
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

	if (PLAYER_SAID (R, you_must_surrender))
		NPCPhrase (NOPE);
	else if (PLAYER_SAID (R, whats_up_1)
			|| PLAYER_SAID (R, whats_up_2)
			|| PLAYER_SAID (R, whats_up_3)
			|| PLAYER_SAID (R, whats_up_4))
	{
		BYTE NumVisits;

		NumVisits = GET_GAME_STATE (URQUAN_INFO);
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
		SET_GAME_STATE (URQUAN_INFO, NumVisits);
	}
	else if (PLAYER_SAID (R, i_wont_surrender))
		NPCPhrase (BAD_CHOICE);
	else if (PLAYER_SAID (R, i_will_surrender))
	{
		NPCPhrase (GOOD_CHOICE);

		setSegue (Segue_defeat);
	}
	else if (PLAYER_SAID (R, like_to_leave))
		NPCPhrase (INDEPENDENCE_IS_BAD);
	else if (PLAYER_SAID (R, bye_wars_over))
	{
		NPCPhrase (GOODBYE_WARS_OVER);

		setSegue (Segue_peace);
	}
	else if (PLAYER_SAID (R, bye_sub_hypno))
		NPCPhrase (GOODBYE_AND_DIE_SUB_HYPNO);
	else if (PLAYER_SAID (R, bye_init_hypno))
	{
		NPCPhrase (GOODBYE_AND_DIE_INIT_HYPNO);

		SET_GAME_STATE (URQUAN_HYPNO_VISITS, 1);
	}
	else if (PLAYER_SAID (R, terran_amphibian)
			|| PLAYER_SAID (R, talking_pet_on_steroids))
	{
		NPCPhrase (BAD_NEWS);

		setSegue (Segue_peace);
		SET_GAME_STATE (URQUAN_HYPNO_VISITS, 1);
	}
	else if (PLAYER_SAID (R, falling_asleep)
			|| PLAYER_SAID (R, someone_else_controlled))
	{
		NPCPhrase (SOUNDS_FAMILIAR);

		setSegue (Segue_peace);
		SET_GAME_STATE (URQUAN_HYPNO_VISITS, 1);
	}
}

static void
DescribePet (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, wascally_little_guy))
		NPCPhrase (WHAT_IT_LOOK_LIKE);
	else if (PLAYER_SAID (R, turd_and_toad))
	{
		NPCPhrase (WHAT_IS_TURD_AND_TOAD);

		DISABLE_PHRASE (turd_and_toad);
	}

	Response (terran_amphibian, CombatIsInevitable);
	Response (talking_pet_on_steroids, CombatIsInevitable);
	if (PHRASE_ENABLED (turd_and_toad))
	{
		Response (turd_and_toad, DescribePet);
	}
}

static void
DescribeCompulsion (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, alien_mind_control))
		NPCPhrase (WHAT_FELT_LIKE);
	else if (PLAYER_SAID (R, before_coffee))
	{
		NPCPhrase (EXPLAIN);

		DISABLE_PHRASE (before_coffee);
	}

	Response (falling_asleep, CombatIsInevitable);
	Response (someone_else_controlled, CombatIsInevitable);
	if (PHRASE_ENABLED (before_coffee))
	{
		Response (before_coffee, DescribeCompulsion);
	}
}

static void
MentionCompulsion (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, compulsion))
	{
		NPCPhrase (WHAT_COMPULSION);

		SET_GAME_STATE (MENTIONED_PET_COMPULSION, 1);
	}
	else if (PLAYER_SAID (R, possessed_by_devil))
	{
		NPCPhrase (STUPID_DEVIL);

		DISABLE_PHRASE (possessed_by_devil);
	}
	else if (PLAYER_SAID (R, why_explain))
	{
		NPCPhrase (MUST_EXPLAIN);

		DISABLE_PHRASE (why_explain);
	}

	Response (wascally_little_guy, DescribePet);
	Response (alien_mind_control, DescribeCompulsion);
	if (PHRASE_ENABLED (possessed_by_devil))
		Response (possessed_by_devil, MentionCompulsion);
	if (PHRASE_ENABLED (why_explain))
		Response (why_explain, MentionCompulsion);
}

static void
UrquanHypno (RESPONSE_REF R)
{
	if (GET_GAME_STATE (URQUAN_HYPNO_VISITS) == 0)
	{
		if (R == 0)
			NPCPhrase (INIT_URQUAN_WAKE_UP);
		else if (PLAYER_SAID (R, where_am_i))
		{
			NPCPhrase (YOU_ARE_HERE);

			DISABLE_PHRASE (where_am_i);
		}
		else if (PLAYER_SAID (R, why_does_my_head_hurt))
		{
			NPCPhrase (HURTS_BECAUSE);

			DISABLE_PHRASE (why_does_my_head_hurt);
		}
		else if (PLAYER_SAID (R, what_about_2_weeks))
		{
			NPCPhrase (ABOUT_2_WEEKS);

			DISABLE_PHRASE (what_about_2_weeks);
		}

		if (PHRASE_ENABLED (where_am_i))
			Response (where_am_i, UrquanHypno);
		if (PHRASE_ENABLED (why_does_my_head_hurt))
			Response (why_does_my_head_hurt, UrquanHypno);
		if (PHRASE_ENABLED (what_about_2_weeks))
			Response (what_about_2_weeks, UrquanHypno);
		Response (compulsion, MentionCompulsion);
		Response (bye_init_hypno, CombatIsInevitable);
	}
	else
	{
		if (R == 0)
			NPCPhrase (SUBSEQUENT_URQUAN_WAKE_UP);
		else if (PLAYER_SAID (R, uh_oh))
		{
			NPCPhrase (NO_UH_OH);

			DISABLE_PHRASE (uh_oh);
		}
		else if (PLAYER_SAID (R, stop_meeting))
		{
			NPCPhrase (NO_STOP_MEETING);

			DISABLE_PHRASE (stop_meeting);
		}

		if (PHRASE_ENABLED (uh_oh))
			Response (uh_oh, UrquanHypno);
		if (PHRASE_ENABLED (stop_meeting))
			Response (stop_meeting, UrquanHypno);
		if (!GET_GAME_STATE (MENTIONED_PET_COMPULSION))
		{
			Response (compulsion, MentionCompulsion);
		}
		Response (bye_sub_hypno, CombatIsInevitable);
	}
}

static void
NormalUrquan (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, i_surrender))
	{
		NPCPhrase (DISOBEDIENCE_PUNISHED);

		Response (i_wont_surrender, CombatIsInevitable);
		Response (i_will_surrender, CombatIsInevitable);
	}
	else
	{
		if (PLAYER_SAID (R, key_phrase))
		{
			NPCPhrase (URQUAN_STORY);

			SET_GAME_STATE (KNOW_URQUAN_STORY, 2);
		}

		Response (you_must_surrender, CombatIsInevitable);
		if (GET_GAME_STATE (KNOW_URQUAN_STORY) == 1)
		{
			Response (key_phrase, NormalUrquan);
		}
		switch (GET_GAME_STATE (URQUAN_INFO))
		{
			case 0:
				Response (whats_up_1, CombatIsInevitable);
				break;
			case 1:
				Response (whats_up_2, CombatIsInevitable);
				break;
			case 2:
				Response (whats_up_3, CombatIsInevitable);
				break;
			case 3:
				Response (whats_up_4, CombatIsInevitable);
				break;
		}
		Response (i_surrender, NormalUrquan);
		Response (like_to_leave, CombatIsInevitable);
	}
}

static void
LoserUrquan (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, why_flee))
	{
		NPCPhrase (FLEE_BECAUSE);

		DISABLE_PHRASE (why_flee);
	}
	else if (PLAYER_SAID (R, what_happens_now))
	{
		NPCPhrase (HAPPENS_NOW);

		DISABLE_PHRASE (what_happens_now);
	}
	else if (PLAYER_SAID (R, what_about_you))
	{
		NPCPhrase (ABOUT_US);

		DISABLE_PHRASE (what_about_you);
	}

	if (PHRASE_ENABLED (why_flee))
		Response (why_flee, LoserUrquan);
	if (PHRASE_ENABLED (what_happens_now))
		Response (what_happens_now, LoserUrquan);
	if (PHRASE_ENABLED (what_about_you))
		Response (what_about_you, LoserUrquan);
	Response (bye_wars_over, CombatIsInevitable);
}

static void
Intro (void)
{
	DWORD GrpOffs;

	if (LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
	{
		NPCPhrase (OUT_TAKES);

		setSegue (Segue_peace);
		return;
	}

	GrpOffs = GET_GAME_STATE (URQUAN_PROBE_GRPOFFS);
	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
			&& GLOBAL (BattleGroupRef)
			&& GLOBAL (BattleGroupRef) == GrpOffs)
	{
		NPCPhrase (SEND_MESSAGE);
		SET_GAME_STATE (PROBE_MESSAGE_DELIVERED, 1);
	}
	else if (GET_GAME_STATE (PLAYER_HYPNOTIZED))
	{
		SetCommIntroMode (CIM_FADE_IN_SCREEN, ONE_SECOND * 5);
		UrquanHypno ((RESPONSE_REF)0);
	}
	else
	{
		BYTE NumVisits;

		if (!GET_GAME_STATE (URQUAN_SENSES_EVIL)
				&& GET_GAME_STATE (TALKING_PET_ON_SHIP))
		{
			NPCPhrase (SENSE_EVIL);
			SET_GAME_STATE (URQUAN_SENSES_EVIL, 1);
		}

		GrpOffs = GET_GAME_STATE (COLONY_GRPOFFS);
		if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
				&& GLOBAL (BattleGroupRef)
				&& GLOBAL (BattleGroupRef) == GrpOffs)
		{
			NPCPhrase (CAUGHT_YA);

			setSegue (Segue_hostile);
			return;
		}

		GrpOffs = GET_GAME_STATE (SAMATRA_GRPOFFS);
		if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
				&& GLOBAL (BattleGroupRef)
				&& GLOBAL (BattleGroupRef) == GrpOffs)
		{
			NPCPhrase (HELLO_SAMATRA);

			SET_GAME_STATE (AWARE_OF_SAMATRA, 1);
			setSegue (Segue_hostile);
		}
		else
		{
			NumVisits = GET_GAME_STATE (URQUAN_VISITS);
			if (!GET_GAME_STATE (KOHR_AH_FRENZY))
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase (INIT_HELLO);
						break;
					case 1:
						NPCPhrase (SUBSEQUENT_HELLO_1);
						break;
					case 2:
						NPCPhrase (SUBSEQUENT_HELLO_2);
						break;
					case 3:
						NPCPhrase (SUBSEQUENT_HELLO_3);
						break;
					case 4:
						NPCPhrase (SUBSEQUENT_HELLO_4);
						--NumVisits;
						break;
				}

				NormalUrquan ((RESPONSE_REF)0);
			}
			else
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase (INIT_FLEE_HUMAN);
						LoserUrquan ((RESPONSE_REF)0);
						break;
					case 1:
						NPCPhrase (SUBSEQUENT_FLEE_HUMAN);
						setSegue (Segue_peace);
						--NumVisits;
						break;
				}
			}
			SET_GAME_STATE (URQUAN_VISITS, NumVisits);
		}
	}
}

static COUNT
uninit_urquan (void)
{
	return (0);
}

static void
post_urquan_enc (void)
{
	SET_GAME_STATE (PLAYER_HYPNOTIZED, 0);
}

LOCDATA*
init_urquan_comm (void)
{
	LOCDATA *retval;

	DWORD GrpOffs;

	urquan_desc.init_encounter_func = Intro;
	urquan_desc.post_encounter_func = post_urquan_enc;
	urquan_desc.uninit_encounter_func = uninit_urquan;

	urquan_desc.AlienTextBaseline.x = TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	urquan_desc.AlienTextBaseline.y = 0;
	urquan_desc.AlienTextWidth = SIS_TEXT_WIDTH - 16;

	GrpOffs = GET_GAME_STATE (URQUAN_PROBE_GRPOFFS);
	
	if (IsProbe == TRUE){
		// use alternate "Probe" track if available
		urquan_desc.AlienAltSongRes = URQUAN_PROBE_MUSIC;
		urquan_desc.AlienSongFlags |= LDASF_USE_ALTERNATE;
	} else {
		// regular track -- let's make sure
		urquan_desc.AlienSongFlags &= ~LDASF_USE_ALTERNATE;
	}

	if (GET_GAME_STATE (PLAYER_HYPNOTIZED)
			|| LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE
			|| (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
			&& GLOBAL (BattleGroupRef)
			&& GLOBAL (BattleGroupRef) == GrpOffs))
	{
		setSegue (Segue_peace);
	}
	else
	{
		setSegue (Segue_hostile);
	}
	retval = &urquan_desc;

	return (retval);
}
