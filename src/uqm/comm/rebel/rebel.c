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
#include "../yehat/resinst.h"
#include "strings.h"

#include "uqm/build.h"


static LOCDATA yehat_desc_orig =
{
	YEHAT_REBEL_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	YEHAT_PMAP_ANIM, /* AlienFrame */
	YEHAT_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* (SIS_TEXT_WIDTH - 16) * 2 / 3, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_MIDDLE, /* AlienTextValign */
	YEHAT_COLOR_MAP, /* AlienColorMap */
	YEHAT_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	REBEL_CONVERSATION_PHRASES, /* PlayerPhrases */
	15, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{ /* right hand-wing tapping keyboard; front guy */
			4, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM
            | WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 10, /* FrameRate */
			ONE_SECOND / 4, ONE_SECOND / 2,/* RestartRate */
			(1 << 6) | (1 << 7),
		},
		{ /* left hand-wing tapping keyboard; front guy */
			7, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM
            | WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 10, /* FrameRate */
			ONE_SECOND / 4, ONE_SECOND / 2,/* RestartRate */
			(1 << 6) | (1 << 7),
		},
		{
			10, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 4) | (1 << 14),
		},
		{
			13, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 5),
		},
		{
			16, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 6, ONE_SECOND * 3,/* RestartRate */
			(1 << 2) | (1 << 14),
		},
		{
			21, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 6, ONE_SECOND * 3,/* RestartRate */
			(1 << 3),
		},
		{ /* right arm-wing rising; front guy */
			26, /* StartIndex */
			2, /* NumFrames */
			YOYO_ANIM | WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 6, ONE_SECOND * 3,/* RestartRate */
			(1 << 0) | (1 << 1),
		},
		{ /* left arm-wing rising; front guy */
			28, /* StartIndex */
			2, /* NumFrames */
			YOYO_ANIM | WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 6, ONE_SECOND * 3,/* RestartRate */
			(1 << 0) | (1 << 1),
		},
		{
			30, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			33, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			36, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			39, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			42, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			45, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			48, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM | WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 2) | (1 << 4),
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

static LOCDATA yehat_desc_hd =
{
	YEHAT_REBEL_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	YEHAT_PMAP_ANIM, /* AlienFrame */
	YEHAT_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* (SIS_TEXT_WIDTH - 16) * 2 / 3, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_MIDDLE, /* AlienTextValign */
	YEHAT_COLOR_MAP, /* AlienColorMap */
	YEHAT_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	REBEL_CONVERSATION_PHRASES, /* PlayerPhrases */
	13, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{ /* right hand-wing tapping keyboard; front guy */
			4, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM
			| WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 10, /* FrameRate */
			ONE_SECOND / 4, ONE_SECOND / 2,/* RestartRate */
			0,
		},
		{ /* left hand-wing tapping keyboard; front guy */
			7, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM
			| WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 10, /* FrameRate */
			ONE_SECOND / 4, ONE_SECOND / 2,/* RestartRate */
			0,
		},
		{
			10, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 5) | (1 << 12),
		},
		{
			13, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 5),
		},
		{
			16, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 6, ONE_SECOND * 3,/* RestartRate */
			0,
		},
		{
			21, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND * 6, ONE_SECOND * 3,/* RestartRate */
			(1 << 2) | (1 << 3) | (1 << 12),
		},
		{
			26, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			29, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			32, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			35, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			38, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			41, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 30, ONE_SECOND / 30, /* FrameRate */
			ONE_SECOND / 30, ONE_SECOND / 30, /* RestartRate */
			0, /* BlockMask */
		},
		{
			44, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM | WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 2) | (1 << 5),
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
PrepareShip (void)
{
	BYTE mi, di, yi;

	mi = (GLOBAL (GameClock.month_index) % 12) + 1;
	SET_GAME_STATE (YEHAT_SHIP_MONTH, mi);
	if ((di = GLOBAL (GameClock.day_index)) > 28)
		di = 28;
	SET_GAME_STATE (YEHAT_SHIP_DAY, di);
	yi = (BYTE)(GLOBAL (GameClock.year_index) - START_YEAR);
	if (mi == 1)
		++yi;
	SET_GAME_STATE (YEHAT_SHIP_YEAR, yi);
}

static void
ExitConversation (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, bye_rebel))
		NPCPhrase (GOODBYE_REBEL);
}

static void Rebels (RESPONSE_REF R);

static void
RebelInfo (RESPONSE_REF R)
{
	BYTE InfoLeft;

	InfoLeft = FALSE;
	if (PLAYER_SAID (R, give_info_rebels))
		NPCPhrase (WHAT_INFO);
	else if (PLAYER_SAID (R, what_about_urquan))
	{
		NPCPhrase (ABOUT_URQUAN);

		DISABLE_PHRASE (what_about_urquan);
	}
	else if (PLAYER_SAID (R, what_about_royalty))
	{
		NPCPhrase (ABOUT_ROYALTY);

		DISABLE_PHRASE (what_about_royalty);
	}
	else if (PLAYER_SAID (R, what_about_war))
	{
		NPCPhrase (ABOUT_WAR);

		DISABLE_PHRASE (what_about_war);
	}
	else if (PLAYER_SAID (R, what_about_vux))
	{
		NPCPhrase (ABOUT_VUX);

		DISABLE_PHRASE (what_about_vux);
	}
	else if (PLAYER_SAID (R, what_about_clue))
	{
		NPCPhrase (ABOUT_CLUE);

		DISABLE_PHRASE (what_about_clue);
	}

	if (PHRASE_ENABLED (what_about_urquan))
	{
		Response (what_about_urquan, RebelInfo);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_royalty))
	{
		Response (what_about_royalty, RebelInfo);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_war))
	{
		Response (what_about_war, RebelInfo);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_vux))
	{
		Response (what_about_vux, RebelInfo);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_clue))
	{
		Response (what_about_clue, RebelInfo);
		InfoLeft = TRUE;
	}
	Response (enough_info, Rebels);

	if (!InfoLeft)
	{
		DISABLE_PHRASE (give_info_rebels);
	}
}

static void
Rebels (RESPONSE_REF R)
{
	SBYTE NumVisits;

	if (PLAYER_SAID (R, how_goes_revolution))
	{
		NumVisits = GET_GAME_STATE (YEHAT_REBEL_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (REBEL_REVOLUTION_1);
				break;
			case 1:
				NPCPhrase (REBEL_REVOLUTION_2);
				break;
			case 2:
				NPCPhrase (REBEL_REVOLUTION_3);
				break;
			case 3:
				NPCPhrase (REBEL_REVOLUTION_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (YEHAT_REBEL_INFO, NumVisits);

		DISABLE_PHRASE (how_goes_revolution);
	}
	else if (PLAYER_SAID (R, any_ships))
	{
		if (GET_GAME_STATE (YEHAT_SHIP_MONTH)
		    && ((NumVisits = (GLOBAL (GameClock.year_index) - START_YEAR) - GET_GAME_STATE (YEHAT_SHIP_YEAR)) < 0
			|| ((NumVisits == 0 && (NumVisits = GLOBAL (GameClock.month_index) - GET_GAME_STATE (YEHAT_SHIP_MONTH)) < 0) 
			    || (NumVisits == 0 && GLOBAL (GameClock.day_index) < GET_GAME_STATE (YEHAT_SHIP_DAY)))))
			NPCPhrase (NO_SHIPS_YET);
		else if ((NumVisits = EscortFeasibilityStudy (YEHAT_SHIP)) == 0)
			NPCPhrase (NO_ROOM);
		else
		{
#define NUM_YEHAT_SHIPS 4
			if (NumVisits < NUM_YEHAT_SHIPS)
				NPCPhrase (HAVE_FEW_SHIPS);
			else
			{
				NumVisits = NUM_YEHAT_SHIPS;
				NPCPhrase (HAVE_ALL_SHIPS);
			}

			AlienTalkSegue ((COUNT)~0);
			AddEscortShips (YEHAT_SHIP, NumVisits);
			PrepareShip ();
		}

		DISABLE_PHRASE (any_ships);
	}
	else if (PLAYER_SAID (R, what_about_pkunk_rebel))
	{
		if (GET_GAME_STATE (YEHAT_ABSORBED_PKUNK))
			NPCPhrase (PKUNK_ABSORBED_REBEL);
		else
			NPCPhrase (HATE_PKUNK_REBEL);

		SET_GAME_STATE (YEHAT_REBEL_TOLD_PKUNK, 1);
	}
	else if (PLAYER_SAID (R, enough_info))
		NPCPhrase (OK_ENOUGH_INFO);

	if (PHRASE_ENABLED (how_goes_revolution))
		Response (how_goes_revolution, Rebels);
	if (!GET_GAME_STATE (YEHAT_REBEL_TOLD_PKUNK)
			&& GET_GAME_STATE (PKUNK_VISITS)
			&& GET_GAME_STATE (PKUNK_HOME_VISITS))
		Response (what_about_pkunk_rebel, Rebels);
	if (PHRASE_ENABLED (any_ships))
		Response (any_ships, Rebels);
	if (PHRASE_ENABLED (give_info_rebels))
	{
		Response (give_info_rebels, RebelInfo);
	}
	Response (bye_rebel,  ExitConversation);
}

static void
Intro (void)
{
	BYTE NumVisits;

	setSegue (Segue_peace);
	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
	{
		NPCPhrase (YEHAT_CAVALRY);
		AlienTalkSegue ((COUNT)~0);

		NumVisits = (BYTE) EscortFeasibilityStudy (YEHAT_REBEL_SHIP);
		if (NumVisits > 8)
			NumVisits = 8;
		AddEscortShips (YEHAT_REBEL_SHIP, NumVisits - (NumVisits >> 1));
		AddEscortShips (PKUNK_SHIP, NumVisits >> 1);
	}
	else
	{
		NumVisits = GET_GAME_STATE (YEHAT_REBEL_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (REBEL_HELLO_1);
				break;
			case 1:
				NPCPhrase (REBEL_HELLO_2);
				break;
			case 2:
				NPCPhrase (REBEL_HELLO_3);
				break;
			case 3:
				NPCPhrase (REBEL_HELLO_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (YEHAT_REBEL_VISITS, NumVisits);

		Rebels ((RESPONSE_REF)0);
	}
}

static COUNT
uninit_yehat (void)
{
	return (0);
}

static void
post_yehat_enc (void)
{
	// nothing defined so far
}

LOCDATA*
init_rebel_yehat_comm (void)
{
	static LOCDATA yehat_desc;
	LOCDATA *retval;

	yehat_desc = RES_BOOL(yehat_desc_orig, yehat_desc_hd);

	yehat_desc.init_encounter_func = Intro;
	yehat_desc.post_encounter_func = post_yehat_enc;
	yehat_desc.uninit_encounter_func = uninit_yehat;

	yehat_desc.AlienTextBaseline.x = SIS_SCREEN_WIDTH * 2 / 3;
	yehat_desc.AlienTextBaseline.y = RES_SIS_SCALE(60);
	yehat_desc.AlienTextWidth = (SIS_TEXT_WIDTH - 16) * 2 / 3;

	// use alternate "Rebels" track if available
	yehat_desc.AlienAltSongRes = REBEL_MUSIC;
	yehat_desc.AlienSongFlags |= LDASF_USE_ALTERNATE;

	setSegue (Segue_peace);
	retval = &yehat_desc;

	return (retval);
}
