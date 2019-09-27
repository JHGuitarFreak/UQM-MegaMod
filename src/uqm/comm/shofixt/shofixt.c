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


static LOCDATA shofixti_desc_orig =
{
	SHOFIXTI_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	SHOFIXTI_PMAP_ANIM, /* AlienFrame */
	SHOFIXTI_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	SHOFIXTI_COLOR_MAP, /* AlienColorMap */
	SHOFIXTI_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	SHOFIXTI_CONVERSATION_PHRASES, /* PlayerPhrases */
	11, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			5, /* StartIndex */
			15, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND / 30, 0, /* RestartRate */
			0, /* BlockMask */
		},
		{
			20, /* StartIndex */
			3, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			(ONE_SECOND >> 1), (ONE_SECOND >> 1) * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			23, /* StartIndex */
			3, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			(ONE_SECOND >> 1), (ONE_SECOND >> 1) * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			26, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			29, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},

		{
			33, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 20, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			39, /* StartIndex */
			7, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 20, ONE_SECOND / 30, /* RestartRate */
			(1 << 7), /* BlockMask */
		},
		{
			46, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 20, ONE_SECOND / 30, /* RestartRate */
			(1 << 6), /* BlockMask */
		},
		{
			52, /* StartIndex */
			4, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 20, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			56, /* StartIndex */
			7, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 20, ONE_SECOND / 30, /* RestartRate */
			(1 << 10), /* BlockMask */
		},
		{
			63, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 20, ONE_SECOND / 30, /* RestartRate */
			(1 << 9), /* BlockMask */
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
		ONE_SECOND / 20, 0, /* FrameRate */
		ONE_SECOND / 15, 0, /* RestartRate */
		0, /* BlockMask */
	},
	NULL, /* AlienNumberSpeech - none */
	/* Filler for loaded resources */
	NULL, NULL, NULL,
	NULL,
	NULL,
};

static LOCDATA shofixti_desc_hd =
{
	SHOFIXTI_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	SHOFIXTI_PMAP_ANIM, /* AlienFrame */
	SHOFIXTI_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	SHOFIXTI_COLOR_MAP, /* AlienColorMap */
	SHOFIXTI_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	SHOFIXTI_CONVERSATION_PHRASES, /* PlayerPhrases */
	8, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{ /* 0 bottom left star */
			1, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 20, ONE_SECOND / 30, /* RestartRate */
			(1 << 2), /* BlockMask */
		},
		{ /* 1 bottom right star */
			7, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 20, ONE_SECOND / 30, /* RestartRate */
			(1 << 3), /* BlockMask */
		},
		{ /* 2 top left star */
			13, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 20, ONE_SECOND / 30, /* RestartRate */
			(1 << 0), /* BlockMask */
		},
		{ /* 3 top right star */
			18, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 20, ONE_SECOND / 30, /* RestartRate */
			(1 << 1), /* BlockMask */
		},
		{ /* 4 eye blink */
			23, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* 5 right hand */
			26, /* StartIndex */
			3, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			(ONE_SECOND >> 1), (ONE_SECOND >> 1) * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* 6 radar */
			34, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND / 10, 0, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* 7 left hand */
			42, /* StartIndex */
			3, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			(ONE_SECOND >> 1), (ONE_SECOND >> 1) * 3, /* RestartRate */
			0, /* BlockMask */
		},
#ifdef WHEN_GRAPHICS_ARE_DONE
		{ /* 8 upper-middle left star */
			45, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 20, ONE_SECOND / 30, /* RestartRate */
			(1 << 10), /* BlockMask */
		},
#endif
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
		29, /* StartIndex */
		5, /* NumFrames */
		0, /* AnimFlags */
		ONE_SECOND / 20, 0, /* FrameRate */
		ONE_SECOND / 15, 0, /* RestartRate */
		0, /* BlockMask */
	},
	NULL, /* AlienNumberSpeech - none */
	/* Filler for loaded resources */
	NULL, NULL, NULL,
	NULL,
	NULL,
};

static RESPONSE_REF shofixti_name;

static void
GetShofixtiName (void)
{
	if (GET_GAME_STATE (SHOFIXTI_KIA))
		shofixti_name = katana;
	else
		shofixti_name = tanaka;
}

static void
ExitConversation (RESPONSE_REF R)
{
	setSegue (Segue_hostile);

	if (PLAYER_SAID (R, bye0))
	{
		NPCPhrase (GOODBYE);

		setSegue (Segue_peace);
	}
	else if (PLAYER_SAID (R, go_ahead))
	{
		NPCPhrase (ON_SECOND_THOUGHT);

		setSegue (Segue_peace);
	}
	else if (PLAYER_SAID (R, need_you_for_duty))
	{
		NPCPhrase (OK_WILL_BE_SENTRY);

		setSegue (Segue_peace);
	}
	else if (PLAYER_SAID (R, females)
			|| PLAYER_SAID (R, nubiles)
			|| PLAYER_SAID (R, rat_babes))
	{
		NPCPhrase (LEAPING_HAPPINESS);

		SET_GAME_STATE (SHOFIXTI_RECRUITED, 1);
		SET_GAME_STATE (MAIDENS_ON_SHIP, 0);
		setSegue (Segue_peace);

		AddEvent (RELATIVE_EVENT, 2, 0, 0, SHOFIXTI_RETURN_EVENT);
	}
	else if (PLAYER_SAID (R, dont_attack))
	{
		NPCPhrase (TYPICAL_PLOY);

		SET_GAME_STATE (SHOFIXTI_STACK1, 1);
	}
	else if (PLAYER_SAID (R, hey_stop))
	{
		NPCPhrase (ONLY_STOP);

		SET_GAME_STATE (SHOFIXTI_STACK1, 2);
	}
	else if (PLAYER_SAID (R, look_you_are))
	{
		NPCPhrase (TOO_BAD);

		SET_GAME_STATE (SHOFIXTI_STACK1, 3);
	}
	else if (PLAYER_SAID (R, no_one_insults))
	{
		NPCPhrase (YOU_LIMP);

		SET_GAME_STATE (SHOFIXTI_STACK2, 1);
	}
	else if (PLAYER_SAID (R, mighty_words))
	{
		NPCPhrase (HANG_YOUR);

		SET_GAME_STATE (SHOFIXTI_STACK2, 2);
	}
	else if (PLAYER_SAID (R, dont_know))
	{
		NPCPhrase (NEVER);

		SET_GAME_STATE (SHOFIXTI_STACK3, 1);
	}
	else if (PLAYER_SAID (R, look0))
	{
		NPCPhrase (FOR_YOU);

		SET_GAME_STATE (SHOFIXTI_STACK3, 2);
	}
	else if (PLAYER_SAID (R, no_bloodshed))
	{
		NPCPhrase (YES_BLOODSHED);

		SET_GAME_STATE (SHOFIXTI_STACK3, 3);
	}
	else if (PLAYER_SAID (R, dont_want_to_fight))
	{
		BYTE NumVisits;

		NumVisits = GET_GAME_STATE (SHOFIXTI_STACK4);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (MUST_FIGHT_YOU_URQUAN_1);
				break;
			case 1:
				NPCPhrase (MUST_FIGHT_YOU_URQUAN_2);
				break;
			case 2:
				NPCPhrase (MUST_FIGHT_YOU_URQUAN_3);
				break;
			case 3:
				NPCPhrase (MUST_FIGHT_YOU_URQUAN_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (SHOFIXTI_STACK4, NumVisits);
	}
}

static void
GiveMaidens (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, important_duty))
	{
		NPCPhrase (WHAT_DUTY);

		Response (procreating_wildly, GiveMaidens);
		Response (replenishing_your_species, GiveMaidens);
		Response (hope_you_have, GiveMaidens);
	}
	else
	{
		NPCPhrase (SOUNDS_GREAT_BUT_HOW);

		Response (females, ExitConversation);
		Response (nubiles, ExitConversation);
		Response (rat_babes, ExitConversation);
	}
}

static void
ConsoleShofixti (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, dont_do_it))
	{
		NPCPhrase (YES_I_DO_IT);
		DISABLE_PHRASE (dont_do_it);
	}
	else
		NPCPhrase (VERY_SAD_KILL_SELF);

	if (GET_GAME_STATE (MAIDENS_ON_SHIP))
	{
		Response (important_duty, GiveMaidens);
	}
	if (PHRASE_ENABLED (dont_do_it))
	{
		Response (dont_do_it, ConsoleShofixti);
	}
	Response (need_you_for_duty, ExitConversation);
	Response (go_ahead, ExitConversation);
}

static void
ExplainDefeat (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, i_am_nice))
		NPCPhrase (MUST_UNDERSTAND);
	else if (PLAYER_SAID (R, i_am_guy))
		NPCPhrase (NICE_BUT_WHAT_IS_DONKEY);
	else /* if (PLAYER_SAID (R, i_am_captain)) */
		NPCPhrase (SO_SORRY);
	NPCPhrase (IS_DEFEAT_TRUE);

	Response (yes_and_no, ConsoleShofixti);
	Response (clobbered, ConsoleShofixti);
	Response (butt_blasted, ConsoleShofixti);
}

static void
RealizeMistake (RESPONSE_REF R)
{
	(void) R;  // ignored
	NPCPhrase (DGRUNTI);
	SET_GAME_STATE (SHOFIXTI_STACK1, 0);
	SET_GAME_STATE (SHOFIXTI_STACK3, 0);
	SET_GAME_STATE (SHOFIXTI_STACK2, 3);

	Response (i_am_captain, ExplainDefeat);
	Response (i_am_nice, ExplainDefeat);
	Response (i_am_guy, ExplainDefeat);
}

static void
Hostile (RESPONSE_REF R)
{
	(void) R;  // ignored
	switch (GET_GAME_STATE (SHOFIXTI_STACK1))
	{
		case 0:
			Response (dont_attack, ExitConversation);
			break;
		case 1:
			Response (hey_stop, ExitConversation);
			break;
		case 2:
			Response (look_you_are, ExitConversation);
			break;
	}
	switch (GET_GAME_STATE (SHOFIXTI_STACK2))
	{
		case 0:
			Response (no_one_insults, ExitConversation);
			break;
		case 1:
			Response (mighty_words, ExitConversation);
			break;
		case 2:
			Response (donkey_breath, RealizeMistake);
			break;
	}
	switch (GET_GAME_STATE (SHOFIXTI_STACK3))
	{
		case 0:
			Response (dont_know, ExitConversation);
			break;
		case 1:
		{
			construct_response (
					shared_phrase_buf,
					look0,
					"",
					shofixti_name,
					"",
					look1,
					(UNICODE*)NULL);
			DoResponsePhrase (look0, ExitConversation, shared_phrase_buf);
			break;
		}
		case 2:
			Response (look_you_are, ExitConversation);
			break;
	}
	Response (dont_want_to_fight, ExitConversation);
}

static void
Friendly (RESPONSE_REF R)
{
	BYTE i, LastStack;
	struct
	{
		RESPONSE_REF pStr;
		UNICODE *c_buf;
	} Resp[3];
	static UNICODE buf0[80], buf1[80];
	
	LastStack = 0;
	memset (Resp, 0, sizeof (Resp));
	if (PLAYER_SAID (R, report0))
	{
		NPCPhrase (NOTHING_NEW);

		DISABLE_PHRASE (report0);
	}
	else if (PLAYER_SAID (R, why_here0))
	{
		NPCPhrase (I_GUARD);

		LastStack = 1;
		SET_GAME_STATE (SHOFIXTI_STACK1, 1);
	}
	else if (PLAYER_SAID (R, what_happened))
	{
		NPCPhrase (MET_VUX);

		LastStack = 1;
		SET_GAME_STATE (SHOFIXTI_STACK1, 2);
	}
	else if (PLAYER_SAID (R, glory_device))
	{
		NPCPhrase (SWITCH_BROKE);

		SET_GAME_STATE (SHOFIXTI_STACK1, 3);
	}
	else if (PLAYER_SAID (R, where_world))
	{
		NPCPhrase (BLEW_IT_UP);

		LastStack = 2;
		SET_GAME_STATE (SHOFIXTI_STACK3, 1);
	}
	else if (PLAYER_SAID (R, how_survive))
	{
		NPCPhrase (NOT_HERE);

		SET_GAME_STATE (SHOFIXTI_STACK3, 2);
	}

	if (PHRASE_ENABLED (report0))
	{
		construct_response (
				buf0,
				report0,
				"",
				shofixti_name,
				"",
				report1,
				(UNICODE*)NULL);
		Resp[0].pStr = report0;
		Resp[0].c_buf = buf0;
	}

	switch (GET_GAME_STATE (SHOFIXTI_STACK1))
	{
		case 0:
			construct_response (
					buf1,
					why_here0,
					"",
					shofixti_name,
					"",
					why_here1,
					(UNICODE*)NULL);
			Resp[1].pStr = why_here0;
			Resp[1].c_buf = buf1;
			break;
		case 1:
			Resp[1].pStr = what_happened;
			break;
		case 2:
			Resp[1].pStr = glory_device;
			break;
	}

	switch (GET_GAME_STATE (SHOFIXTI_STACK3))
	{
		case 0:
				Resp[2].pStr = where_world;
			break;
		case 1:
				Resp[2].pStr = how_survive;
			break;
	}

	if (Resp[LastStack].pStr)
		DoResponsePhrase (Resp[LastStack].pStr, Friendly, Resp[LastStack].c_buf);
	for (i = 0; i < 3; ++i)
	{
		if (i != LastStack && Resp[i].pStr)
			DoResponsePhrase (Resp[i].pStr, Friendly, Resp[i].c_buf);
	}
	if (GET_GAME_STATE (MAIDENS_ON_SHIP))
	{
		Response (important_duty, GiveMaidens);
	}

	construct_response (
			shared_phrase_buf,
			bye0,
			"",
			shofixti_name,
			"",
			bye1,
			(UNICODE*)NULL);
	DoResponsePhrase (bye0, ExitConversation, shared_phrase_buf);
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

	GetShofixtiName ();

	if (GET_GAME_STATE (SHOFIXTI_STACK2) > 2)
	{
		NPCPhrase (FRIENDLY_HELLO);

		Friendly ((RESPONSE_REF)0);
	}
	else
	{
		BYTE NumVisits;

		NumVisits = GET_GAME_STATE (SHOFIXTI_VISITS);
		if (GET_GAME_STATE (SHOFIXTI_KIA))
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (HOSTILE_KATANA_1);
					break;
				case 1:
					NPCPhrase (HOSTILE_KATANA_2);
					break;
				case 2:
					NPCPhrase (HOSTILE_KATANA_3);
					break;
				case 3:
					NPCPhrase (HOSTILE_KATANA_4);
					--NumVisits;
					break;
			}
		}
		else
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (HOSTILE_TANAKA_1);
					break;
				case 1:
					NPCPhrase (HOSTILE_TANAKA_2);
					break;
				case 2:
					NPCPhrase (HOSTILE_TANAKA_3);
					break;
				case 3:
					NPCPhrase (HOSTILE_TANAKA_4);
					break;
				case 4:
					NPCPhrase (HOSTILE_TANAKA_5);
					break;
				case 5:
					NPCPhrase (HOSTILE_TANAKA_6);
					break;
				case 6:
					NPCPhrase (HOSTILE_TANAKA_7);
					break;
				case 7:
					NPCPhrase (HOSTILE_TANAKA_8);
					--NumVisits;
					break;
			}
		}
		SET_GAME_STATE (SHOFIXTI_VISITS, NumVisits);

		Hostile ((RESPONSE_REF)0);
	}
}

static COUNT
uninit_shofixti (void)
{
	luaUqm_comm_uninit ();
	return(0);
}

static void
post_shofixti_enc (void)
{
	// nothing defined so far
}

LOCDATA*
init_shofixti_comm (void)
{
	static LOCDATA shofixti_desc;
 	LOCDATA *retval;
	
	shofixti_desc = RES_BOOL(shofixti_desc_orig, shofixti_desc_hd);

	shofixti_desc.init_encounter_func = Intro;
	shofixti_desc.post_encounter_func = post_shofixti_enc;
	shofixti_desc.uninit_encounter_func = uninit_shofixti;

	luaUqm_comm_init (NULL, NULL_RESOURCE);
			// Initialise Lua for string interpolation. This will be
			// generalised in the future.

	shofixti_desc.AlienTextBaseline.x = TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	shofixti_desc.AlienTextBaseline.y = 0;
	shofixti_desc.AlienTextWidth = SIS_TEXT_WIDTH;

	setSegue (Segue_peace);

	retval = &shofixti_desc;

	return (retval);
}
