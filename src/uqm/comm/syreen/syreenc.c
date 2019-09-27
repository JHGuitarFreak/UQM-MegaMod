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
#include "uqm/lua/luacomm.h"
#include "uqm/build.h"
#include "uqm/setup.h"


static LOCDATA syreen_desc_orig =
{
	SYREEN_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	SYREEN_PMAP_ANIM, /* AlienFrame */
	SYREEN_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	SYREEN_COLOR_MAP, /* AlienColorMap */
	SYREEN_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	SYREEN_CONVERSATION_PHRASES, /* PlayerPhrases */
	15, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			5, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			7, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			9, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			11, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			13, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			15, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 12), /* BlockMask */
		},
		{
			17, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			19, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 13),
		},
		{
			21, /* StartIndex */
			6, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			27, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			(1 << 14), /* BlockMask */
		},
		{
			31, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			37, /* StartIndex */
			4, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND / 15, ONE_SECOND / 15, /* RestartRate */
			0, /* BlockMask */
		},
		{
			41, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			(1 << 5), /* BlockMask */
		},
		{
			44, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM
					| WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 6, 0, /* FrameRate */
			ONE_SECOND * 3, ONE_SECOND, /* RestartRate */
			(1 << 7) | (1 << 14), /* BlockMask */
		},
		{
			48, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM
					| WAIT_TALKING, /* AnimFlags */
			ONE_SECOND * 2 / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND,/* RestartRate */
			(1 << 9) | (1 << 13), /* BlockMask */
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

static LOCDATA syreen_desc_hd =
{
	SYREEN_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	SYREEN_PMAP_ANIM, /* AlienFrame */
	SYREEN_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	SYREEN_COLOR_MAP, /* AlienColorMap */
	SYREEN_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	SYREEN_CONVERSATION_PHRASES, /* PlayerPhrases */
	17, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			5, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			7, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			9, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			11, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			13, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			15, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 12), /* BlockMask */
		},
		{
			17, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			19, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 13),
		},
		{
			21, /* StartIndex */
			6, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			27, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			(1 << 14), /* BlockMask */
		},
		{
			31, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			37, /* StartIndex */
			4, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND / 15, ONE_SECOND / 15, /* RestartRate */
			0, /* BlockMask */
		},
		{
			41, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND * 3, /* RestartRate */
			(1 << 5), /* BlockMask */
		},
		{
			44, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM
					| WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 6, 0, /* FrameRate */
			ONE_SECOND * 3, ONE_SECOND, /* RestartRate */
			(1 << 7) | (1 << 14), /* BlockMask */
		},
		{
			48, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM
					| WAIT_TALKING, /* AnimFlags */
			ONE_SECOND * 2 / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 10, ONE_SECOND,/* RestartRate */
			(1 << 9) | (1 << 13), /* BlockMask */
		},
		{
			51, /* StartIndex */
			13, /* NumFrames */
			CIRCULAR_ANIM | ONE_SHOT_ANIM 
				| WAIT_TALKING | ANIM_DISABLED, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			0, 0,/* RestartRate */
			0, /* BlockMask */
		},
		{
			64, /* StartIndex */
			13, /* NumFrames */
			CIRCULAR_ANIM | ONE_SHOT_ANIM 
				| WAIT_TALKING | ANIM_DISABLED, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
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
FriendlyExit (RESPONSE_REF R)
{
	setSegue (Segue_peace);

	if (PLAYER_SAID (R, bye))
		NPCPhrase (GOODBYE);
	else if (PLAYER_SAID (R, im_on_my_way)
			|| PLAYER_SAID (R, doing_this_for_you)
			|| PLAYER_SAID (R, if_i_die))
		NPCPhrase (GOOD_LUCK);
	else if (PLAYER_SAID (R, bye_before_vault))
		NPCPhrase (GOODBYE_BEFORE_VAULT);
	else if (PLAYER_SAID (R, bye_after_vault))
		NPCPhrase (GOODBYE_AFTER_VAULT);
	else if (PLAYER_SAID (R, bye_before_ambush))
		NPCPhrase (GOODBYE_BEFORE_AMBUSH);
	else if (PLAYER_SAID (R, bye_after_ambush))
		NPCPhrase (GOODBYE_AFTER_AMBUSH);
	else
	{
		if (PLAYER_SAID (R, hands_off))
			NPCPhrase (OK_WONT_USE_HANDS);
		else if (PLAYER_SAID (R, not_much_more_to_say))
			NPCPhrase (THEN_STOP_TALKING);
		NPCPhrase (LATER);
		NPCPhrase (SEX_GOODBYE);

		AlienTalkSegue (2);
		if (!IS_HD) {
			XFormColorMap (GetColorMapAddress (
 				SetAbsColorMapIndex (CommData.AlienColorMap, 0)
 				), ONE_SECOND / 2);
			AlienTalkSegue ((COUNT)~0);
		} else {
			COUNT i = 0;
			COUNT limit = CommData.NumAnimations;
			
			CommData.AlienAmbientArray[limit-1].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienFrame = SetAbsFrameIndex 
				(CommData.AlienFrame, 0);
			
			CommData.AlienTalkDesc.AnimFlags &= ~PAUSE_TALKING;
			
			AlienTalkSegue ((COUNT)~0);
			
			for (i = 0; i < limit; i++)
				CommData.AlienAmbientArray[i].AnimFlags &= ~ANIM_DISABLED;
			
			CommData.AlienAmbientArray[limit-2].AnimFlags |= ANIM_DISABLED;
		}

		SET_GAME_STATE (PLAYER_HAD_SEX, 1);
		SET_GAME_STATE (PLAYER_HAVING_SEX, 0);
	}
}

static void
Sex (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, in_the_spirit))
		NPCPhrase (OK_SPIRIT);
	else if (PLAYER_SAID (R, what_in_mind))
		NPCPhrase (SOMETHING_LIKE_THIS);
	else if (PLAYER_SAID (R, disease))
		NPCPhrase (JUST_RELAX);
	else if (PLAYER_SAID (R, what_happens_if_i_touch_this))
	{
		NPCPhrase (THIS_HAPPENS);

		DISABLE_PHRASE (what_happens_if_i_touch_this);
	}
	else if (PLAYER_SAID (R, are_you_sure_this_is_ok))
	{
		NPCPhrase (YES_SURE);

		DISABLE_PHRASE (are_you_sure_this_is_ok);
	}
	else if (PLAYER_SAID (R, boy_they_never_taught))
	{
		NPCPhrase (THEN_LET_ME_TEACH);

		DISABLE_PHRASE (boy_they_never_taught);
	}

	if (!PHRASE_ENABLED (what_happens_if_i_touch_this)
			&& !PHRASE_ENABLED (are_you_sure_this_is_ok)
			&& !PHRASE_ENABLED (boy_they_never_taught))
		Response (not_much_more_to_say, FriendlyExit);
	else
	{
		if (PHRASE_ENABLED (what_happens_if_i_touch_this))
			Response (what_happens_if_i_touch_this, Sex);
		if (PHRASE_ENABLED (are_you_sure_this_is_ok))
			Response (are_you_sure_this_is_ok, Sex);
		if (PHRASE_ENABLED (boy_they_never_taught))
			Response (boy_they_never_taught, Sex);
	}
}

static void
Foreplay (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, whats_my_reward)
			|| PLAYER_SAID (R, what_about_us))
	{
		if (PLAYER_SAID (R, whats_my_reward))
			NPCPhrase (HERES_REWARD);
		else
			NPCPhrase (ABOUT_US);
		NPCPhrase (MORE_COMFORTABLE);
		AlienTalkSegue (1);

		if (!IS_HD) {
			XFormColorMap (GetColorMapAddress (
					SetAbsColorMapIndex (CommData.AlienColorMap, 1)
					), ONE_SECOND);
		} else {
			COUNT i = 0;
			COUNT limit = CommData.NumAnimations - 2;
			
			for (i = 0; i < limit; i++)
				CommData.AlienAmbientArray[i].AnimFlags |= ANIM_DISABLED;
				
			CommData.AlienAmbientArray[limit].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienFrame = SetAbsFrameIndex 
				(CommData.AlienFrame, 63);
				
			CommData.AlienTalkDesc.AnimFlags |= PAUSE_TALKING;
		}

		AlienTalkSegue ((COUNT)~0);

		SET_GAME_STATE (PLAYER_HAVING_SEX, 1);
	}
	else if (PLAYER_SAID (R, why_lights_off))
	{
		NPCPhrase (LIGHTS_OFF_BECAUSE);

		DISABLE_PHRASE (why_lights_off);
	}
	else if (PLAYER_SAID (R, evil_monster))
	{
		NPCPhrase (NOT_EVIL_MONSTER);

		DISABLE_PHRASE (evil_monster);
	}

	if (PHRASE_ENABLED (why_lights_off))
		Response (why_lights_off, Foreplay);
	else if (PHRASE_ENABLED (evil_monster))
		Response (evil_monster, Foreplay);
	else
		Response (disease, Sex);
	Response (in_the_spirit, Sex);
	Response (what_in_mind, Sex);
	Response (hands_off, FriendlyExit);
}

static void
AfterAmbush (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, what_now_after_ambush))
	{
		NPCPhrase (DO_THIS_AFTER_AMBUSH);

		DISABLE_PHRASE (what_now_after_ambush);
	}
	else if (PLAYER_SAID (R, what_about_you))
	{
		NPCPhrase (ABOUT_ME);

		DISABLE_PHRASE (what_about_you);
	}
	else if (PLAYER_SAID (R, whats_up_after_ambush))
	{
		BYTE NumVisits;

		NumVisits = GET_GAME_STATE (SYREEN_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (GENERAL_INFO_AFTER_AMBUSH_1);
				break;
			case 1:
				NPCPhrase (GENERAL_INFO_AFTER_AMBUSH_2);
				break;
			case 2:
				NPCPhrase (GENERAL_INFO_AFTER_AMBUSH_3);
				break;
			case 3:
				NPCPhrase (GENERAL_INFO_AFTER_AMBUSH_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (SYREEN_INFO, NumVisits);

		DISABLE_PHRASE (whats_up_after_ambush);
	}

	if (PHRASE_ENABLED (what_about_you))
		Response (what_about_you, AfterAmbush);
	else if (!GET_GAME_STATE (PLAYER_HAD_SEX))
	{
		Response (what_about_us, Foreplay);
	}
	if (PHRASE_ENABLED (what_now_after_ambush))
		Response (what_now_after_ambush, AfterAmbush);
	if (PHRASE_ENABLED (whats_up_after_ambush))
		Response (whats_up_after_ambush, AfterAmbush);
	Response (bye_after_ambush, FriendlyExit);
}

static void
AmbushReady (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, repeat_plan))
	{
		NPCPhrase (OK_REPEAT_PLAN);

		DISABLE_PHRASE (repeat_plan);
	}

	if (PHRASE_ENABLED (repeat_plan))
		Response (repeat_plan, AmbushReady);
	Response (bye_before_ambush, FriendlyExit);
}

static void
SyreenShuttle (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, whats_next_step))
	{
		NPCPhrase (OPEN_VAULT);

		DISABLE_PHRASE (whats_next_step);
	}
	else if (PLAYER_SAID (R, what_do_i_get_for_this))
	{
		NPCPhrase (GRATITUDE);

		DISABLE_PHRASE (what_do_i_get_for_this);
	}
	else if (PLAYER_SAID (R, not_sure))
	{
		NPCPhrase (PLEASE);

		DISABLE_PHRASE (not_sure);
	}
	else if (PLAYER_SAID (R, where_is_it))
	{
		NPCPhrase (DONT_KNOW_WHERE);
		NPCPhrase (GIVE_SHUTTLE);

		SET_GAME_STATE (SYREEN_SHUTTLE, 1);
		SET_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP, 1);

		DISABLE_PHRASE (where_is_it);
	}
	else if (PLAYER_SAID (R, been_there))
	{
		NPCPhrase (GREAT);
		NPCPhrase (GIVE_SHUTTLE);

		SET_GAME_STATE (SYREEN_SHUTTLE, 1);
		SET_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP, 1);

		DISABLE_PHRASE (been_there);
	}

	if (PHRASE_ENABLED (whats_next_step))
		Response (whats_next_step, SyreenShuttle);
	else
	{
		if (!GET_GAME_STATE (KNOW_SYREEN_VAULT))
		{
			if (PHRASE_ENABLED (where_is_it))
				Response (where_is_it, SyreenShuttle);
		}
		else
		{
			if (PHRASE_ENABLED (been_there))
				Response (been_there, SyreenShuttle);
		}
		if (!PHRASE_ENABLED (where_is_it)
				|| !PHRASE_ENABLED (been_there))
		{
			Response (im_on_my_way, FriendlyExit);
			Response (doing_this_for_you, FriendlyExit);
			Response (if_i_die, FriendlyExit);
		}
	}
	if (PHRASE_ENABLED (what_do_i_get_for_this))
		Response (what_do_i_get_for_this, SyreenShuttle);
	if (PHRASE_ENABLED (not_sure))
		Response (not_sure, SyreenShuttle);
}

static void
NormalSyreen (RESPONSE_REF R)
{
	BYTE i, LastStack;
	RESPONSE_REF pStr[4];

	LastStack = 0;
	pStr[0] = pStr[1] = pStr[2] = pStr[3] = 0;
	if (PLAYER_SAID (R, we_here_to_help))
		NPCPhrase (NO_NEED_HELP);
	else if (PLAYER_SAID (R, we_need_help))
		NPCPhrase (CANT_GIVE_HELP);
	else if (PLAYER_SAID (R, know_about_deep_children))
	{
		NPCPhrase (WHAT_ABOUT_DEEP_CHILDREN);

		DISABLE_PHRASE (know_about_deep_children);
	}
	else if (PLAYER_SAID (R, mycons_involved))
	{
		NPCPhrase (WHAT_PROOF);

		SET_GAME_STATE (KNOW_ABOUT_SHATTERED, 3);
	}
	else if (PLAYER_SAID (R, have_no_proof))
	{
		NPCPhrase (NEED_PROOF);

		SET_GAME_STATE (SYREEN_WANT_PROOF, 1);
	}
	else if (PLAYER_SAID (R, have_proof))
	{
		NPCPhrase (SEE_PROOF);

		DISABLE_PHRASE (have_proof);
	}
	else if (PLAYER_SAID (R, what_doing_here))
	{
		NPCPhrase (OUR_NEW_WORLD);

		SET_GAME_STATE (SYREEN_STACK0, 1);
		LastStack = 1;
	}
	else if (PLAYER_SAID (R, what_about_war))
	{
		NPCPhrase (ABOUT_WAR);

		SET_GAME_STATE (SYREEN_STACK0, 2);
		LastStack = 1;
	}
	else if (PLAYER_SAID (R, help_us))
	{
		NPCPhrase (WONT_HELP);

		SET_GAME_STATE (SYREEN_STACK0, 3);
	}
	else if (PLAYER_SAID (R, what_about_history))
	{
		NPCPhrase (BEFORE_WAR);

		SET_GAME_STATE (SYREEN_STACK1, 1);
		LastStack = 2;
	}
	else if (PLAYER_SAID (R, what_about_homeworld))
	{
		NPCPhrase (ABOUT_HOMEWORLD);

		SET_GAME_STATE (SYREEN_STACK1, 2);
		LastStack = 2;
	}
	else if (PLAYER_SAID (R, what_happened))
	{
		NPCPhrase (DONT_KNOW_HOW);

		SET_GAME_STATE (KNOW_SYREEN_WORLD_SHATTERED, 1);
		SET_GAME_STATE (SYREEN_STACK1, 3);
	}
	else if (PLAYER_SAID (R, what_about_outfit))
	{
		NPCPhrase (HOPE_YOU_LIKE_IT);

		SET_GAME_STATE (SYREEN_STACK2, 1);
		LastStack = 3;
	}
	else if (PLAYER_SAID (R, where_mates))
	{
		NPCPhrase (MATES_KILLED);

		SET_GAME_STATE (SYREEN_STACK2, 2);
		LastStack = 3;
	}
	else if (PLAYER_SAID (R, get_lonely))
	{
		NPCPhrase (MAKE_OUT_ALL_RIGHT);

		SET_GAME_STATE (SYREEN_STACK2, 3);
	}
	else if (PLAYER_SAID (R, look_at_egg_sacks))
	{
		NPCPhrase (HORRIBLE_TRUTH);

		setSegue (Segue_peace);
		SET_GAME_STATE (SYREEN_HOME_VISITS, 0);
		SET_GAME_STATE (SYREEN_KNOW_ABOUT_MYCON, 1);

		if(!SyreenVoiceFix && usingSpeech){
			SyreenShuttle ((RESPONSE_REF)0);
		}
		return;
	}

	if (GET_GAME_STATE (KNOW_ABOUT_SHATTERED) < 3)
	{
		if (GET_GAME_STATE (KNOW_ABOUT_SHATTERED) == 2
				&& GET_GAME_STATE (KNOW_SYREEN_WORLD_SHATTERED))
		{
			if (PHRASE_ENABLED (know_about_deep_children))
				pStr[0] = know_about_deep_children;
			else
				pStr[0] = mycons_involved;
		}
	}
	else
	{
		if (GET_GAME_STATE (EGG_CASE0_ON_SHIP)
				|| GET_GAME_STATE (EGG_CASE1_ON_SHIP)
				|| GET_GAME_STATE (EGG_CASE2_ON_SHIP))
		{
			if (PHRASE_ENABLED (have_proof))
				pStr[0] = have_proof;
			else
				pStr[0] = look_at_egg_sacks;
		}
		else if (!GET_GAME_STATE (SYREEN_WANT_PROOF))
		{
			pStr[0] = have_no_proof;
		}
	}
	switch (GET_GAME_STATE (SYREEN_STACK0))
	{
		case 0:
			pStr[1] = what_doing_here;
			break;
		case 1:
			pStr[1] = what_about_war;
			break;
		case 2:
			pStr[1] = help_us;
			break;
	}
	switch (GET_GAME_STATE (SYREEN_STACK1))
	{
		case 0:
			pStr[2] = what_about_history;
			break;
		case 1:
			pStr[2] = what_about_homeworld;
			break;
		case 2:
			pStr[2] = what_happened;
			break;
	}
	switch (GET_GAME_STATE (SYREEN_STACK2))
	{
		case 0:
			pStr[3] = what_about_outfit;
			break;
		case 1:
			pStr[3] = where_mates;
			break;
		case 2:
			pStr[3] = get_lonely;
			break;
	}
	if (pStr[LastStack])
		Response (pStr[LastStack], NormalSyreen);
	for (i = 0; i < 4; ++i)
	{
		if (i != LastStack && pStr[i])
			Response (pStr[i], NormalSyreen);
	}
	Response (bye, FriendlyExit);
}

static void
InitialSyreen (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, we_are_vice_squad))
	{
		NPCPhrase (OK_VICE);
		NPCPhrase (HOW_CAN_YOU_BE_HERE);
	}
	else if (PLAYER_SAID (R, we_are_the_one_for_you_baby))
	{
		NPCPhrase (MAYBE_CAPTAIN);
		NPCPhrase (HOW_CAN_YOU_BE_HERE);
	}
	else if (PLAYER_SAID (R, we_are_vindicator))
	{
		NPCPhrase (WELCOME_VINDICATOR);
		NPCPhrase (HOW_CAN_YOU_BE_HERE);
	}
	else if (PLAYER_SAID (R, we_are_impressed))
	{
		NPCPhrase (SO_AM_I_CAPTAIN);
		NPCPhrase (HOW_CAN_YOU_BE_HERE);
	}
	else if (PLAYER_SAID (R, i_need_you))
	{
		NPCPhrase (OK_NEED);

		DISABLE_PHRASE (i_need_you);
		DISABLE_PHRASE (i_need_touch_o_vision);
	}
	else if (PLAYER_SAID (R, i_need_touch_o_vision))
	{
		NPCPhrase (TOUCH_O_VISION);

		DISABLE_PHRASE (i_need_you);
		DISABLE_PHRASE (i_need_touch_o_vision);
	}

	Response (we_here_to_help, NormalSyreen);
	Response (we_need_help, NormalSyreen);
	if (PHRASE_ENABLED (i_need_you))
		Response (i_need_you, InitialSyreen);
	if (PHRASE_ENABLED (i_need_touch_o_vision))
		Response (i_need_touch_o_vision, InitialSyreen);
}

static void
PlanAmbush (RESPONSE_REF R)
{
	HFLEETINFO hSyreen = GetStarShipFromIndex(&GLOBAL(avail_race_q), SYREEN_SHIP);
	FLEET_INFO *SyreenPtr = LockFleetInfo(&GLOBAL(avail_race_q), hSyreen);
	(void) R;  // ignored
	NPCPhrase (OK_FOUND_VAULT);

	SET_GAME_STATE (MYCON_AMBUSH, 1);
	// This is redundant but left here for clarity
	SET_GAME_STATE (SYREEN_HOME_VISITS, 0);

	// Send ambush fleet to Organon.  EncounterPercent for the
	// Syreen is 0, so this is purely decorative.
	if (EXTENDED) {
		if (SyreenPtr) {
			SyreenPtr->actual_strength = 300 / SPHERE_RADIUS_INCREMENT * 2;
			SyreenPtr->loc.x = 4125;
			SyreenPtr->loc.y = 3770;
			StartSphereTracking(SYREEN_SHIP);
			SetRaceDest(SYREEN_SHIP, 6858, 577, 15, (BYTE)~0);
		}
		UnlockFleetInfo(&GLOBAL(avail_race_q), hSyreen);
	}

	Response (whats_my_reward, Foreplay);
	Response (bye_after_vault, FriendlyExit);
}

static void
SyreenVault (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, vault_hint))
	{
		NPCPhrase (OK_HINT);

		DISABLE_PHRASE (vault_hint);
	}

	if (PHRASE_ENABLED (vault_hint))
	{
		Response (vault_hint, SyreenVault);
	}
	Response (bye_before_vault, FriendlyExit);
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

	NumVisits = GET_GAME_STATE (SYREEN_HOME_VISITS);
	if (GET_GAME_STATE (MYCON_KNOW_AMBUSH))
	{
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (HELLO_AFTER_AMBUSH_1);
				SetRaceAllied (SYREEN_SHIP, TRUE);
				break;
			case 1:
				NPCPhrase (HELLO_AFTER_AMBUSH_2);
				break;
			case 2:
				NPCPhrase (HELLO_AFTER_AMBUSH_3);
				break;
			case 3:
				NPCPhrase (HELLO_AFTER_AMBUSH_3);
				--NumVisits;
				break;
		}

		AfterAmbush ((RESPONSE_REF)0);
	}
	else if (GET_GAME_STATE (MYCON_AMBUSH))
	{
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (READY_FOR_AMBUSH);
				--NumVisits;
				break;
		}

		AmbushReady ((RESPONSE_REF)0);
	}
	else if (!GET_GAME_STATE (SYREEN_KNOW_ABOUT_MYCON))
	{
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (HELLO_BEFORE_AMBUSH_1);
				break;
			case 1:
				NPCPhrase (HELLO_BEFORE_AMBUSH_2);
				break;
			case 2:
				NPCPhrase (HELLO_BEFORE_AMBUSH_3);
				break;
			case 3:
				NPCPhrase (HELLO_BEFORE_AMBUSH_4);
				--NumVisits;
				break;
		}

		if (NumVisits > 1)
			NormalSyreen ((RESPONSE_REF)0);
		else
		{
			Response (we_are_vice_squad, InitialSyreen);
			Response (we_are_the_one_for_you_baby, InitialSyreen);
			Response (we_are_vindicator, InitialSyreen);
			Response (we_are_impressed, InitialSyreen);
		}
	}

	else if (!GET_GAME_STATE (SYREEN_SHUTTLE) && (SyreenVoiceFix || !usingSpeech))
	{
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (MUST_ACT);
				--NumVisits;
				break;
		}

		SyreenShuttle ((RESPONSE_REF)0);
	}

	else if (GET_GAME_STATE (SHIP_VAULT_UNLOCKED))
	{
		PlanAmbush ((RESPONSE_REF)0);
		// XXX: PlanAmbush() sets SYREEN_HOME_VISITS=0, but then this value
		// is immediately reset to NumVisits just below. The intent was to
		// reset the HELLO stack so that is what we will do here as well.
		// Note that it is completely redundant because genvault.c resets
		// SYREEN_HOME_VISITS when it sets SHIP_VAULT_UNLOCKED=1.
		NumVisits = 0;
	}
	else
	{
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (FOUND_VAULT_YET_1);
				break;
			case 1:
				NPCPhrase (FOUND_VAULT_YET_2);
				--NumVisits;
				break;
		}

		SyreenVault ((RESPONSE_REF)0);
	}
	SET_GAME_STATE (SYREEN_HOME_VISITS, NumVisits);
}

static COUNT
uninit_syreen (void)
{
	luaUqm_comm_uninit ();
	return (0);
}

static void
post_syreen_enc (void)
{
	// nothing defined so far
}

LOCDATA*
init_syreen_comm (void)
{
	static LOCDATA syreen_desc;
 	LOCDATA *retval;
	
	syreen_desc = RES_BOOL(syreen_desc_orig, syreen_desc_hd);

	syreen_desc.init_encounter_func = Intro;
	syreen_desc.post_encounter_func = post_syreen_enc;
	syreen_desc.uninit_encounter_func = uninit_syreen;

	luaUqm_comm_init (NULL, NULL_RESOURCE);
			// Initialise Lua for string interpolation. This will be 
			// generalised in the future.

	syreen_desc.AlienTextBaseline.x = TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	syreen_desc.AlienTextBaseline.y = 0;
	syreen_desc.AlienTextWidth = SIS_TEXT_WIDTH - 16;

	setSegue (Segue_peace);
	retval = &syreen_desc;

	return (retval);
}
