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

static LOCDATA blackurq_desc =
{
	BLACKURQ_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	BLACKURQ_PMAP_ANIM, /* AlienFrame */
	BLACKURQ_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	BLACKURQ_COLOR_MAP, /* AlienColorMap */
	BLACKURQ_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	BLACKURQ_CONVERSATION_PHRASES, /* PlayerPhrases */
	8, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			7, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			13, /* StartIndex */
			7, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			20, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			23, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			26, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			29, /* StartIndex */
			4, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND / 10, 0, /* RestartRate */
			0, /* BlockMask */
		},
		{
			33, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM
					| WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			38, /* StartIndex */
			4, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
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
	BYTE NumVisits;

	setSegue (Segue_hostile);

	if (PLAYER_SAID (R, bye))
	{
		if (GET_GAME_STATE (KOHR_AH_BYES) == 0)
			NPCPhrase (GOODBYE_AND_DIE);
		else
			NPCPhrase (DIE_HUMAN /* GOODBYE_AND_DIE_2 */);

		SET_GAME_STATE (KOHR_AH_BYES, 1);
	}
	else if (PLAYER_SAID (R, guess_thats_all))
		NPCPhrase (THEN_DIE);
	else if (PLAYER_SAID (R, what_are_you_hovering_over))
	{
		NPCPhrase (BONE_PILE);

		SET_GAME_STATE (KOHR_AH_INFO, 1);
	}
	else if (PLAYER_SAID (R, you_sure_are_creepy))
	{
		NPCPhrase (YES_CREEPY);

		SET_GAME_STATE (KOHR_AH_INFO, 2);
	}
	else if (PLAYER_SAID (R, stop_that_gross_blinking))
	{
		NPCPhrase (DIE_HUMAN);

		SET_GAME_STATE (KOHR_AH_INFO, 3);
	}
	else if (PLAYER_SAID (R, threat_1)
			|| PLAYER_SAID (R, threat_2)
			|| PLAYER_SAID (R, threat_3)
			|| PLAYER_SAID (R, threat_4))
	{
		NumVisits = GET_GAME_STATE (KOHR_AH_REASONS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (RESISTANCE_IS_USELESS_1);
				break;
			case 1:
				NPCPhrase (RESISTANCE_IS_USELESS_2);
				break;
			case 2:
				NPCPhrase (RESISTANCE_IS_USELESS_3);
				break;
			case 3:
				NPCPhrase (RESISTANCE_IS_USELESS_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (KOHR_AH_REASONS, NumVisits);
	}
	else if (PLAYER_SAID (R, plead_1)
			|| PLAYER_SAID (R, plead_2)
			|| PLAYER_SAID (R, plead_3)
			|| PLAYER_SAID (R, plead_4))
	{
		NumVisits = GET_GAME_STATE (KOHR_AH_PLEAD);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (PLEADING_IS_USELESS_1);
				break;
			case 1:
				NPCPhrase (PLEADING_IS_USELESS_2);
				break;
			case 2:
				// This response disabled due to lack of a speech file.
				// NPCPhrase (PLEADING_IS_USELESS_3);
				// break;
			case 3:
				NPCPhrase (PLEADING_IS_USELESS_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (KOHR_AH_PLEAD, NumVisits);
	}
	else if (PLAYER_SAID (R, why_kill_all_1)
			|| PLAYER_SAID (R, why_kill_all_2)
			|| PLAYER_SAID (R, why_kill_all_3)
			|| PLAYER_SAID (R, why_kill_all_4))
	{
		NumVisits = GET_GAME_STATE (KOHR_AH_REASONS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (KILL_BECAUSE_1);
				break;
			case 1:
				NPCPhrase (KILL_BECAUSE_2);
				break;
			case 2:
				NPCPhrase (KILL_BECAUSE_3);
				break;
			case 3:
				NPCPhrase (KILL_BECAUSE_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (KOHR_AH_REASONS, NumVisits);
	}
	else if (PLAYER_SAID (R, please_dont_kill_1)
			|| PLAYER_SAID (R, please_dont_kill_2)
			|| PLAYER_SAID (R, please_dont_kill_3)
			|| PLAYER_SAID (R, please_dont_kill_4))
	{
		NumVisits = GET_GAME_STATE (KOHR_AH_PLEAD);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (WILL_KILL_1);
				break;
			case 1:
				NPCPhrase (WILL_KILL_2);
				break;
			case 2:
				NPCPhrase (WILL_KILL_3);
				break;
			case 3:
				NPCPhrase (WILL_KILL_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (KOHR_AH_PLEAD, NumVisits);
	}
	else if (PLAYER_SAID (R, bye_frenzy_1)
			|| PLAYER_SAID (R, bye_frenzy_2)
			|| PLAYER_SAID (R, bye_frenzy_3)
			|| PLAYER_SAID (R, bye_frenzy_4))
	{
		NumVisits = GET_GAME_STATE (KOHR_AH_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (GOODBYE_AND_DIE_FRENZY_1);
				break;
			case 1:
				NPCPhrase (GOODBYE_AND_DIE_FRENZY_2);
				break;
			case 2:
				NPCPhrase (GOODBYE_AND_DIE_FRENZY_3);
				break;
			case 3:
				NPCPhrase (GOODBYE_AND_DIE_FRENZY_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (KOHR_AH_INFO, NumVisits);
	}
}

static void
Frenzy (RESPONSE_REF R)
{
	(void) R;  // ignored
	switch (GET_GAME_STATE (KOHR_AH_REASONS))
	{
		case 0:
			Response (why_kill_all_1, CombatIsInevitable);
			break;
		case 1:
			Response (why_kill_all_2, CombatIsInevitable);
			break;
		case 2:
			Response (why_kill_all_3, CombatIsInevitable);
			break;
		case 3:
			Response (why_kill_all_4, CombatIsInevitable);
			break;
	}
	switch (GET_GAME_STATE (KOHR_AH_PLEAD))
	{
		case 0:
			Response (please_dont_kill_1, CombatIsInevitable);
			break;
		case 1:
			Response (please_dont_kill_2, CombatIsInevitable);
			break;
		case 2:
			Response (please_dont_kill_3, CombatIsInevitable);
			break;
		case 3:
			Response (please_dont_kill_4, CombatIsInevitable);
			break;
	}
	switch (GET_GAME_STATE (KOHR_AH_INFO))
	{
		case 0:
			Response (bye_frenzy_1, CombatIsInevitable);
			break;
		case 1:
			Response (bye_frenzy_2, CombatIsInevitable);
			break;
		case 2:
			Response (bye_frenzy_3, CombatIsInevitable);
			break;
		case 3:
			Response (bye_frenzy_4, CombatIsInevitable);
			break;
	}
}

static void
KohrAhStory (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, key_phrase))
	{
		NPCPhrase (RESPONSE_TO_KEY_PHRASE);

		SET_GAME_STATE (KNOW_KOHR_AH_STORY, 2);
	}
	else if (PLAYER_SAID (R, why_do_you_destroy))
	{
		NPCPhrase (WE_WERE_SLAVES);

		DISABLE_PHRASE (why_do_you_destroy);
	}
	else if (PLAYER_SAID (R, relationship_with_urquan))
	{
		NPCPhrase (WE_ARE_URQUAN_TOO);

		DISABLE_PHRASE (relationship_with_urquan);
	}
	else if (PLAYER_SAID (R, what_about_culture))
	{
		NPCPhrase (BONE_GARDENS);

		DISABLE_PHRASE (what_about_culture);
	}
	else if (PLAYER_SAID (R, how_leave_me_alone))
	{
		NPCPhrase (YOU_DIE);

		DISABLE_PHRASE (how_leave_me_alone);
	}

	if (PHRASE_ENABLED (why_do_you_destroy))
		Response (why_do_you_destroy, KohrAhStory);
	if (PHRASE_ENABLED (relationship_with_urquan))
		Response (relationship_with_urquan, KohrAhStory);
	if (PHRASE_ENABLED (what_about_culture))
		Response (what_about_culture, KohrAhStory);
	if (PHRASE_ENABLED (how_leave_me_alone))
		Response (how_leave_me_alone, KohrAhStory);
	Response (guess_thats_all, CombatIsInevitable);
}

static void
DieHuman (RESPONSE_REF R)
{
	(void) R;  // ignored
	switch (GET_GAME_STATE (KOHR_AH_REASONS))
	{
		case 0:
			Response (threat_1, CombatIsInevitable);
			break;
		case 1:
			Response (threat_2, CombatIsInevitable);
			break;
		case 2:
			Response (threat_3, CombatIsInevitable);
			break;
		case 3:
			Response (threat_4, CombatIsInevitable);
			break;
	}
	if (GET_GAME_STATE (KNOW_KOHR_AH_STORY) == 1)
	{
		Response (key_phrase, KohrAhStory);
	}
	switch (GET_GAME_STATE (KOHR_AH_INFO))
	{
		case 0:
			Response (what_are_you_hovering_over, CombatIsInevitable);
			break;
		case 1:
			Response (you_sure_are_creepy, CombatIsInevitable);
			break;
		case 2:
			Response (stop_that_gross_blinking, CombatIsInevitable);
			break;
	}
	switch (GET_GAME_STATE (KOHR_AH_PLEAD))
	{
		case 0:
			Response (plead_1, CombatIsInevitable);
			break;
		case 1:
			Response (plead_2, CombatIsInevitable);
			break;
		case 2:
			// This response disabled due to lack of a speech file.
			// Response (plead_3, CombatIsInevitable);
			// break;
		case 3:
			Response (plead_4, CombatIsInevitable);
			break;
	}
	Response (bye, CombatIsInevitable);
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

	if (GET_GAME_STATE (KOHR_AH_KILLED_ALL))
	{
		NPCPhrase (GAME_OVER_DUDE);

		setSegue (Segue_peace);
		return;
	}

	if (!GET_GAME_STATE (KOHR_AH_SENSES_EVIL)
			&& GET_GAME_STATE (TALKING_PET_ON_SHIP))
	{
		NPCPhrase (SENSE_EVIL);
		SET_GAME_STATE (KOHR_AH_SENSES_EVIL, 1);
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
		BYTE NumVisits;

		NumVisits = GET_GAME_STATE (KOHR_AH_VISITS);
		if (GET_GAME_STATE (KOHR_AH_FRENZY))
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (WE_KILL_ALL_1);
					break;
				case 1:
					NPCPhrase (WE_KILL_ALL_2);
					break;
				case 2:
					NPCPhrase (WE_KILL_ALL_3);
					break;
				case 3:
					NPCPhrase (WE_KILL_ALL_4);
					--NumVisits;
					break;
			}

			Frenzy ((RESPONSE_REF)0);
		}
		else
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (HELLO_AND_DIE_1);
					break;
				case 1:
					NPCPhrase (HELLO_AND_DIE_2);
					break;
				case 2:
					NPCPhrase (HELLO_AND_DIE_3);
					break;
				case 3:
					NPCPhrase (HELLO_AND_DIE_4);
					--NumVisits;
					break;
			}

			DieHuman ((RESPONSE_REF)0);
		}
		SET_GAME_STATE (KOHR_AH_VISITS, NumVisits);
	}
}

static COUNT
uninit_blackurq (void)
{
	return (0);
}

static void
post_blackurq_enc (void)
{
	// nothing defined so far
}

LOCDATA*
init_blackurq_comm (void)
{
	LOCDATA *retval;

	blackurq_desc.init_encounter_func = Intro;
	blackurq_desc.post_encounter_func = post_blackurq_enc;
	blackurq_desc.uninit_encounter_func = uninit_blackurq;

	blackurq_desc.AlienTextBaseline.x =	TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	blackurq_desc.AlienTextBaseline.y = 0;
	blackurq_desc.AlienTextWidth = SIS_TEXT_WIDTH - 16;

	if (!GET_GAME_STATE (KOHR_AH_KILLED_ALL)
			&& LOBYTE (GLOBAL (CurrentActivity)) != WON_LAST_BATTLE)
	{
		setSegue (Segue_hostile);
	}
	else
	{
		setSegue (Segue_peace);
	}
	retval = &blackurq_desc;

	return (retval);
}
