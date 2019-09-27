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
#include "uqm/gameev.h"


#define ZOQ_FG_COLOR WHITE_COLOR
#define ZOQ_BG_COLOR BLACK_COLOR
#define ZOQ_BASE_X (TEXT_X_OFFS + ((SIS_TEXT_WIDTH >> 1) >> 1))
#define ZOQ_BASE_Y RES_SIS_SCALE(24)
#define ZOQ_TALK_INDEX 18
#define ZOQ_TALK_FRAMES 5
#define FOT_TO_ZOQ 23

#define PIK_FG_COLOR WHITE_COLOR
#define PIK_BG_COLOR BLACK_COLOR
#define PIK_BASE_X (SIS_SCREEN_WIDTH - (TEXT_X_OFFS + ((SIS_TEXT_WIDTH >> 1) >> 1)))
#define PIK_BASE_Y RES_SIS_SCALE(24)
#define PIK_TALK_INDEX 29
#define PIK_TALK_FRAMES 2
#define FOT_TO_PIK 26

static LOCDATA zoqfot_desc =
{
	ZOQFOTPIK_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	ZOQFOTPIK_PMAP_ANIM, /* AlienFrame */
	ZOQFOTPIK_FONT, /* AlienFont */
	UNDEFINED_COLOR_INIT, /* AlienTextFColor */
	UNDEFINED_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_MIDDLE, /* AlienTextValign */
	ZOQFOTPIK_COLOR_MAP, /* AlienColorMap */
	ZOQFOTPIK_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	ZOQFOTPIK_CONVERSATION_PHRASES, /* PlayerPhrases */
	3, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{ /* Eye blink */
			1, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM /* AnimFlags */
			| WAIT_TALKING,
			ONE_SECOND / 24, 0, /* FrameRate */
			0, ONE_SECOND * 10, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Blow smoke */
			5, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND * 7 / 120, 0, /* FrameRate */
			ONE_SECOND * 2, 0, /* RestartRate */
			0, /* BlockMask */
		},
		{ /* Gulp */
			10, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 15, 0, /* FrameRate */
			0, ONE_SECOND * 10, /* RestartRate */
			0, /* BlockMask */
		},
	},
	{ /* AlienTransitionDesc - Move Eye */
		FOT_TO_ZOQ, /* StartIndex */
		3, /* NumFrames */
		0, /* AnimFlags */
		ONE_SECOND / 30, 0, /* FrameRate */
		0, 0, /* RestartRate */
		0, /* BlockMask */
	},
	{ /* AlienTalkDesc */
		ZOQ_TALK_INDEX, /* StartIndex */
		ZOQ_TALK_FRAMES, /* NumFrames */
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

enum
{
	ZOQ_ALIEN,
	FOT_ALIEN,
	PIK_ALIEN
};

static int LastAlien;

// Queued and executes synchronously on the Starcon2Main thread
static void
SelectAlienZOQ (CallbackArg arg)
{
	if (LastAlien != ZOQ_ALIEN)
	{
		// Transition to neutral state first if Pik was talking
		if (LastAlien != FOT_ALIEN)
			CommData.AlienTransitionDesc.AnimFlags |= TALK_DONE;
		LastAlien = ZOQ_ALIEN;
		CommData.AlienTransitionDesc.AnimFlags |= TALK_INTRO;
		CommData.AlienTransitionDesc.StartIndex = FOT_TO_ZOQ;
		CommData.AlienTalkDesc.StartIndex = ZOQ_TALK_INDEX;
		CommData.AlienTalkDesc.NumFrames = ZOQ_TALK_FRAMES;
		CommData.AlienAmbientArray[1].AnimFlags &= ~WAIT_TALKING;

		CommData.AlienTextBaseline.x = (SWORD)ZOQ_BASE_X;
		CommData.AlienTextBaseline.y = ZOQ_BASE_Y;
		CommData.AlienTextFColor = ZOQ_FG_COLOR;
		CommData.AlienTextBColor = ZOQ_BG_COLOR;
	}

	(void)arg; // ignored
}

// Queued and executes synchronously on the Starcon2Main thread
static void
SelectAlienPIK (CallbackArg arg)
{
	if (LastAlien != PIK_ALIEN)
	{
		// Transition to neutral state first if Zoq was talking
		if (LastAlien != FOT_ALIEN)
			CommData.AlienTransitionDesc.AnimFlags |= TALK_DONE;
		LastAlien = PIK_ALIEN;
		CommData.AlienTransitionDesc.AnimFlags |= TALK_INTRO;
		CommData.AlienTransitionDesc.StartIndex = FOT_TO_PIK;
		CommData.AlienTalkDesc.StartIndex = PIK_TALK_INDEX;
		CommData.AlienTalkDesc.NumFrames = PIK_TALK_FRAMES;
		CommData.AlienAmbientArray[1].AnimFlags |= WAIT_TALKING;

		CommData.AlienTextBaseline.x = (SWORD)PIK_BASE_X;
		CommData.AlienTextBaseline.y = PIK_BASE_Y;
		CommData.AlienTextFColor = PIK_FG_COLOR;
		CommData.AlienTextBColor = PIK_BG_COLOR;
	}

	(void)arg; // ignored
}

static void
ZFPTalkSegue (COUNT wait_track)
{
	LastAlien = FOT_ALIEN;
	SelectAlienZOQ (0);
	AlienTalkSegue (wait_track);
}

static void
ExitConversation (RESPONSE_REF R)
{
	setSegue (Segue_peace);

	if (PLAYER_SAID (R, bye_homeworld))
	{
		NPCPhrase_cb (GOODBYE_HOME0, &SelectAlienZOQ);
		NPCPhrase_cb (GOODBYE_HOME1, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);
	}
	else if (PLAYER_SAID (R, decide_later))
	{
		NPCPhrase_cb (PLEASE_HURRY0, &SelectAlienZOQ);
		NPCPhrase_cb (PLEASE_HURRY1, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);
	}
	else if (PLAYER_SAID (R, valuable_info))
	{
		NPCPhrase_cb (GOODBYE0, &SelectAlienZOQ);
		NPCPhrase_cb (GOODBYE1, &SelectAlienPIK);
		NPCPhrase_cb (GOODBYE2, &SelectAlienZOQ);
		NPCPhrase_cb (GOODBYE3, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);
	}
	else if (PLAYER_SAID (R, how_can_i_help))
	{
		NPCPhrase_cb (EMMISSARIES0, &SelectAlienZOQ);
		NPCPhrase_cb (EMMISSARIES1, &SelectAlienPIK);
		NPCPhrase_cb (EMMISSARIES2, &SelectAlienZOQ);
		NPCPhrase_cb (EMMISSARIES3, &SelectAlienPIK);
		NPCPhrase_cb (EMMISSARIES4, &SelectAlienZOQ);
		NPCPhrase_cb (EMMISSARIES5, &SelectAlienPIK);
		NPCPhrase_cb (EMMISSARIES6, &SelectAlienZOQ);
		NPCPhrase_cb (EMMISSARIES7, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);
	}
	else if (PLAYER_SAID (R, sure))
	{
		NPCPhrase_cb (WE_ALLY0, &SelectAlienZOQ);
		NPCPhrase_cb (WE_ALLY1, &SelectAlienPIK);
		NPCPhrase_cb (WE_ALLY2, &SelectAlienZOQ);
		NPCPhrase_cb (WE_ALLY3, &SelectAlienPIK);
		NPCPhrase_cb (WE_ALLY4, &SelectAlienZOQ);
		NPCPhrase_cb (WE_ALLY5, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);
		SetRaceAllied (ZOQFOTPIK_SHIP, TRUE);
		AddEvent (RELATIVE_EVENT, 3, 0, 0, ZOQFOT_DISTRESS_EVENT);
		SET_GAME_STATE (ZOQFOT_HOME_VISITS, 0);
	}
	else if (PLAYER_SAID (R, all_very_interesting))
	{
		NPCPhrase_cb (SEE_TOLD_YOU0, &SelectAlienZOQ);
		NPCPhrase_cb (SEE_TOLD_YOU1, &SelectAlienPIK);
		NPCPhrase_cb (SEE_TOLD_YOU2, &SelectAlienZOQ);
		NPCPhrase_cb (SEE_TOLD_YOU3, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);

		SET_GAME_STATE (ZOQFOT_HOSTILE, 1);
		SET_GAME_STATE (ZOQFOT_HOME_VISITS, 0);
		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, never))
	{
		NPCPhrase_cb (WE_ENEMIES0, &SelectAlienZOQ);
		NPCPhrase_cb (WE_ENEMIES1, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);

		SET_GAME_STATE (ZOQFOT_HOME_VISITS, 0);
		SET_GAME_STATE (ZOQFOT_HOSTILE, 1);
		setSegue (Segue_hostile);
	}
}

static void
FormAlliance (RESPONSE_REF R)
{
	(void) R;  // ignored
	NPCPhrase_cb (ALLY_WITH_US0, &SelectAlienZOQ);
	NPCPhrase_cb (ALLY_WITH_US1, &SelectAlienPIK);
	NPCPhrase_cb (ALLY_WITH_US2, &SelectAlienZOQ);
	NPCPhrase_cb (ALLY_WITH_US3, &SelectAlienPIK);
	NPCPhrase_cb (ALLY_WITH_US4, &SelectAlienZOQ);
	NPCPhrase_cb (ALLY_WITH_US5, &SelectAlienPIK);
	ZFPTalkSegue ((COUNT)~0);

	Response (sure, ExitConversation);
	Response (never, ExitConversation);
	Response (decide_later, ExitConversation);
}

static void
ZoqFotIntro (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, we_are_vindicator))
	{
		NPCPhrase_cb (WE_GLAD0, &SelectAlienZOQ);
		NPCPhrase_cb (WE_GLAD1, &SelectAlienPIK);
		NPCPhrase_cb (WE_GLAD2, &SelectAlienZOQ);
		NPCPhrase_cb (WE_GLAD3, &SelectAlienPIK);
		NPCPhrase_cb (WE_GLAD4, &SelectAlienZOQ);
		NPCPhrase_cb (WE_GLAD5, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);
		
		DISABLE_PHRASE (we_are_vindicator);
	}
	else if (PLAYER_SAID (R, your_race))
	{
		NPCPhrase_cb (YEARS_AGO0, &SelectAlienZOQ);
		NPCPhrase_cb (YEARS_AGO1, &SelectAlienPIK);
		NPCPhrase_cb (YEARS_AGO2, &SelectAlienZOQ);
		NPCPhrase_cb (YEARS_AGO3, &SelectAlienPIK);
		NPCPhrase_cb (YEARS_AGO4, &SelectAlienZOQ);
		NPCPhrase_cb (YEARS_AGO5, &SelectAlienPIK);
		NPCPhrase_cb (YEARS_AGO6, &SelectAlienZOQ);
		NPCPhrase_cb (YEARS_AGO7, &SelectAlienPIK);
		NPCPhrase_cb (YEARS_AGO8, &SelectAlienZOQ);
		NPCPhrase_cb (YEARS_AGO9, &SelectAlienPIK);
		NPCPhrase_cb (YEARS_AGO10, &SelectAlienZOQ);
		NPCPhrase_cb (YEARS_AGO11, &SelectAlienPIK);
		NPCPhrase_cb (YEARS_AGO12, &SelectAlienZOQ);
		NPCPhrase_cb (YEARS_AGO13, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);
		
		DISABLE_PHRASE (your_race);
	}
	else if (PLAYER_SAID (R, where_from))
	{
		NPCPhrase_cb (TRAVELED_FAR0, &SelectAlienZOQ);
		NPCPhrase_cb (TRAVELED_FAR1, &SelectAlienPIK);
		NPCPhrase_cb (TRAVELED_FAR2, &SelectAlienZOQ);
		NPCPhrase_cb (TRAVELED_FAR3, &SelectAlienPIK);
		NPCPhrase_cb (TRAVELED_FAR4, &SelectAlienZOQ);
		NPCPhrase_cb (TRAVELED_FAR5, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);

		DISABLE_PHRASE (where_from);
	}
	else if (PLAYER_SAID (R, what_emergency))
	{
		NPCPhrase_cb (UNDER_ATTACK0, &SelectAlienZOQ);
		NPCPhrase_cb (UNDER_ATTACK1, &SelectAlienPIK);
		NPCPhrase_cb (UNDER_ATTACK2, &SelectAlienZOQ);
		NPCPhrase_cb (UNDER_ATTACK3, &SelectAlienPIK);
		NPCPhrase_cb (UNDER_ATTACK4, &SelectAlienZOQ);
		NPCPhrase_cb (UNDER_ATTACK5, &SelectAlienPIK);
		NPCPhrase_cb (UNDER_ATTACK6, &SelectAlienZOQ);
		NPCPhrase_cb (UNDER_ATTACK7, &SelectAlienPIK);
		NPCPhrase_cb (UNDER_ATTACK8, &SelectAlienZOQ);
		NPCPhrase_cb (UNDER_ATTACK9, &SelectAlienPIK);
		NPCPhrase_cb (UNDER_ATTACK10, &SelectAlienZOQ);
		NPCPhrase_cb (UNDER_ATTACK11, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);

		DISABLE_PHRASE (what_emergency);
	}
	else if (PLAYER_SAID (R, tough_luck))
	{
		NPCPhrase_cb (NOT_HELPFUL0, &SelectAlienZOQ);
		NPCPhrase_cb (NOT_HELPFUL1, &SelectAlienPIK);
		NPCPhrase_cb (NOT_HELPFUL2, &SelectAlienZOQ);
		NPCPhrase_cb (NOT_HELPFUL3, &SelectAlienPIK);
		NPCPhrase_cb (NOT_HELPFUL4, &SelectAlienZOQ);
		NPCPhrase_cb (NOT_HELPFUL5, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);

		DISABLE_PHRASE (tough_luck);
	}
	else if (PLAYER_SAID (R, what_look_like))
	{
		NPCPhrase_cb (LOOK_LIKE0, &SelectAlienZOQ);
		NPCPhrase_cb (LOOK_LIKE1, &SelectAlienPIK);
		NPCPhrase_cb (LOOK_LIKE2, &SelectAlienZOQ);
		NPCPhrase_cb (LOOK_LIKE3, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);

		DISABLE_PHRASE (what_look_like);
	}
	
	if (PHRASE_ENABLED (your_race)
			|| PHRASE_ENABLED (where_from)
			|| PHRASE_ENABLED (what_emergency))
	{
		if (PHRASE_ENABLED (your_race))
			 Response (your_race, ZoqFotIntro);
		if (PHRASE_ENABLED (where_from))
			 Response (where_from, ZoqFotIntro);
		if (PHRASE_ENABLED (what_emergency))
			 Response (what_emergency, ZoqFotIntro);
	}
	else
	{
		if (PHRASE_ENABLED (tough_luck))
			 Response (tough_luck, ZoqFotIntro);
		if (PHRASE_ENABLED (what_look_like))
			 Response (what_look_like, ZoqFotIntro);
		Response (all_very_interesting, ExitConversation);
		if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			Response (how_can_i_help, FormAlliance);
		}
		else
		{
			Response (how_can_i_help, ExitConversation);
		}
		Response (valuable_info, ExitConversation);
	}
}

static void
AquaintZoqFot (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, which_fot))
	{
		NPCPhrase_cb (HE_IS0, &SelectAlienZOQ);
		NPCPhrase_cb (HE_IS1, &SelectAlienPIK);
		NPCPhrase_cb (HE_IS2, &SelectAlienZOQ);
		NPCPhrase_cb (HE_IS3, &SelectAlienPIK);
		NPCPhrase_cb (HE_IS4, &SelectAlienZOQ);
		NPCPhrase_cb (HE_IS5, &SelectAlienPIK);
		NPCPhrase_cb (HE_IS6, &SelectAlienZOQ);
		NPCPhrase_cb (HE_IS7, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);

		DISABLE_PHRASE (which_fot);
	}
	else if (PLAYER_SAID (R, quiet_toadies))
	{
		NPCPhrase_cb (TOLD_YOU0, &SelectAlienZOQ);
		NPCPhrase_cb (TOLD_YOU1, &SelectAlienPIK);
		NPCPhrase_cb (TOLD_YOU2, &SelectAlienZOQ);
		NPCPhrase_cb (TOLD_YOU3, &SelectAlienPIK);
		NPCPhrase_cb (TOLD_YOU4, &SelectAlienZOQ);
		NPCPhrase_cb (TOLD_YOU5, &SelectAlienPIK);
		NPCPhrase_cb (TOLD_YOU6, &SelectAlienZOQ);
		NPCPhrase_cb (TOLD_YOU7, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);

		DISABLE_PHRASE (quiet_toadies);
	}

	if (PHRASE_ENABLED (which_fot))
		Response (which_fot, AquaintZoqFot);
	if (PHRASE_ENABLED (we_are_vindicator))
		Response (we_are_vindicator, ZoqFotIntro);
	if (PHRASE_ENABLED (quiet_toadies))
		Response (quiet_toadies, AquaintZoqFot);
	Response (all_very_interesting, ExitConversation);
	Response (valuable_info, ExitConversation);
}

static void ZoqFotHome (RESPONSE_REF R);

static void
ZoqFotInfo (RESPONSE_REF R)
{
	BYTE InfoLeft;

	if (PLAYER_SAID (R, want_specific_info))
	{
		NPCPhrase_cb (WHAT_SPECIFIC_INFO0, &SelectAlienZOQ);
		NPCPhrase_cb (WHAT_SPECIFIC_INFO1, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);
	}
	else if (PLAYER_SAID (R, what_about_others))
	{
		NPCPhrase_cb (ABOUT_OTHERS0, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_OTHERS1, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_OTHERS2, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_OTHERS3, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_OTHERS4, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_OTHERS5, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_OTHERS6, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_OTHERS7, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_OTHERS8, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_OTHERS9, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_OTHERS10, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_OTHERS11, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_OTHERS12, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_OTHERS13, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);

		DISABLE_PHRASE (what_about_others);
	}
	else if (PLAYER_SAID (R, what_about_zebranky))
	{
		NPCPhrase_cb (ABOUT_ZEBRANKY0, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_ZEBRANKY1, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_ZEBRANKY2, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_ZEBRANKY3, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_ZEBRANKY4, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_ZEBRANKY5, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_ZEBRANKY6, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_ZEBRANKY7, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);

		DISABLE_PHRASE (what_about_zebranky);
	}
	else if (PLAYER_SAID (R, what_about_stinger))
	{
		NPCPhrase_cb (ABOUT_STINGER0, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_STINGER1, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_STINGER2, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_STINGER3, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_STINGER4, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_STINGER5, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);

		DISABLE_PHRASE (what_about_stinger);
	}
	else if (PLAYER_SAID (R, what_about_guy_in_back))
	{
		NPCPhrase_cb (ABOUT_GUY0, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_GUY1, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);

		DISABLE_PHRASE (what_about_guy_in_back);
	}
	else if (PLAYER_SAID (R, what_about_past))
	{
		NPCPhrase_cb (ABOUT_PAST0, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_PAST1, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_PAST2, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_PAST3, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_PAST4, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_PAST5, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_PAST6, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_PAST7, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_PAST8, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_PAST9, &SelectAlienPIK);
		NPCPhrase_cb (ABOUT_PAST10, &SelectAlienZOQ);
		NPCPhrase_cb (ABOUT_PAST11, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);

		DISABLE_PHRASE (what_about_past);
	}

	InfoLeft = FALSE;
	if (PHRASE_ENABLED (what_about_others))
	{
		Response (what_about_others, ZoqFotInfo);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_zebranky))
	{
		Response (what_about_zebranky, ZoqFotInfo);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_stinger))
	{
		Response (what_about_stinger, ZoqFotInfo);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_guy_in_back))
	{
		Response (what_about_guy_in_back, ZoqFotInfo);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_past))
	{
		Response (what_about_past, ZoqFotInfo);
		InfoLeft = TRUE;
	}
	Response (enough_info, ZoqFotHome);

	if (!InfoLeft)
	{
		DISABLE_PHRASE (want_specific_info);
	}
}

static void
ZoqFotHome (RESPONSE_REF R)
{
	BYTE NumVisits;

	if (PLAYER_SAID (R, whats_up_homeworld))
	{
		NumVisits = GET_GAME_STATE (ZOQFOT_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase_cb (GENERAL_INFO_10, &SelectAlienZOQ);
				NPCPhrase_cb (GENERAL_INFO_11, &SelectAlienPIK);
				NPCPhrase_cb (GENERAL_INFO_12, &SelectAlienZOQ);
				NPCPhrase_cb (GENERAL_INFO_13, &SelectAlienPIK);
				ZFPTalkSegue ((COUNT)~0);
				break;
			case 1:
				NPCPhrase_cb (GENERAL_INFO_20, &SelectAlienZOQ);
				NPCPhrase_cb (GENERAL_INFO_21, &SelectAlienPIK);
				NPCPhrase_cb (GENERAL_INFO_22, &SelectAlienZOQ);
				NPCPhrase_cb (GENERAL_INFO_23, &SelectAlienPIK);
				NPCPhrase_cb (GENERAL_INFO_24, &SelectAlienZOQ);
				NPCPhrase_cb (GENERAL_INFO_25, &SelectAlienPIK);
				NPCPhrase_cb (GENERAL_INFO_26, &SelectAlienZOQ);
				NPCPhrase_cb (GENERAL_INFO_27, &SelectAlienPIK);
				ZFPTalkSegue ((COUNT)~0);
				break;
			case 2:
				NPCPhrase_cb (GENERAL_INFO_30, &SelectAlienZOQ);
				NPCPhrase_cb (GENERAL_INFO_31, &SelectAlienPIK);
				NPCPhrase_cb (GENERAL_INFO_32, &SelectAlienZOQ);
				NPCPhrase_cb (GENERAL_INFO_33, &SelectAlienPIK);
				NPCPhrase_cb (GENERAL_INFO_34, &SelectAlienZOQ);
				NPCPhrase_cb (GENERAL_INFO_35, &SelectAlienPIK);
				ZFPTalkSegue ((COUNT)~0);
				break;
			case 3:
				NPCPhrase_cb (GENERAL_INFO_40, &SelectAlienZOQ);
				NPCPhrase_cb (GENERAL_INFO_41, &SelectAlienPIK);
				NPCPhrase_cb (GENERAL_INFO_42, &SelectAlienZOQ);
				NPCPhrase_cb (GENERAL_INFO_43, &SelectAlienPIK);
				NPCPhrase_cb (GENERAL_INFO_44, &SelectAlienZOQ);
				NPCPhrase_cb (GENERAL_INFO_45, &SelectAlienPIK);
				NPCPhrase_cb (GENERAL_INFO_46, &SelectAlienZOQ);
				NPCPhrase_cb (GENERAL_INFO_47, &SelectAlienPIK);
				NPCPhrase_cb (GENERAL_INFO_48, &SelectAlienZOQ);
				NPCPhrase_cb (GENERAL_INFO_49, &SelectAlienPIK);
				NPCPhrase_cb (GENERAL_INFO_410, &SelectAlienZOQ);
				NPCPhrase_cb (GENERAL_INFO_411, &SelectAlienPIK);
				ZFPTalkSegue ((COUNT)~0);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (ZOQFOT_INFO, NumVisits);

		DISABLE_PHRASE (whats_up_homeworld);
	}
	else if (PLAYER_SAID (R, any_war_news))
	{
#define UTWIG_BUY_TIME (1 << 0)
#define KOHR_AH_WIN (1 << 1)
#define URQUAN_LOSE (1 << 2)
#define KOHR_AH_KILL (1 << 3)
#define KNOW_ALL (UTWIG_BUY_TIME | KOHR_AH_WIN | URQUAN_LOSE | KOHR_AH_KILL)
		BYTE KnowMask;

		NumVisits = GET_GAME_STATE (UTWIG_SUPOX_MISSION);
		KnowMask = GET_GAME_STATE (ZOQFOT_KNOW_MASK);
		if (!(KnowMask & KOHR_AH_KILL) && GET_GAME_STATE (KOHR_AH_FRENZY))
		{
			NPCPhrase_cb (KOHRAH_FRENZY0, &SelectAlienZOQ);
			NPCPhrase_cb (KOHRAH_FRENZY1, &SelectAlienPIK);
			NPCPhrase_cb (KOHRAH_FRENZY2, &SelectAlienZOQ);
			NPCPhrase_cb (KOHRAH_FRENZY3, &SelectAlienPIK);
			NPCPhrase_cb (KOHRAH_FRENZY4, &SelectAlienZOQ);
			NPCPhrase_cb (KOHRAH_FRENZY5, &SelectAlienPIK);
			NPCPhrase_cb (KOHRAH_FRENZY6, &SelectAlienZOQ);
			NPCPhrase_cb (KOHRAH_FRENZY7, &SelectAlienPIK);
			NPCPhrase_cb (KOHRAH_FRENZY8, &SelectAlienZOQ);
			NPCPhrase_cb (KOHRAH_FRENZY9, &SelectAlienPIK);
			NPCPhrase_cb (KOHRAH_FRENZY10, &SelectAlienZOQ);
			NPCPhrase_cb (KOHRAH_FRENZY11, &SelectAlienPIK);
			ZFPTalkSegue ((COUNT)~0);

			KnowMask = KNOW_ALL;
		}
		else if (!(KnowMask & UTWIG_BUY_TIME)
				&& NumVisits > 0 && NumVisits < 5)
		{
			NPCPhrase_cb (UTWIG_DELAY0, &SelectAlienZOQ);
			NPCPhrase_cb (UTWIG_DELAY1, &SelectAlienPIK);
			NPCPhrase_cb (UTWIG_DELAY2, &SelectAlienZOQ);
			NPCPhrase_cb (UTWIG_DELAY3, &SelectAlienPIK);
			NPCPhrase_cb (UTWIG_DELAY4, &SelectAlienZOQ);
			NPCPhrase_cb (UTWIG_DELAY5, &SelectAlienPIK);
			NPCPhrase_cb (UTWIG_DELAY6, &SelectAlienZOQ);
			NPCPhrase_cb (UTWIG_DELAY7, &SelectAlienPIK);
			NPCPhrase_cb (UTWIG_DELAY8, &SelectAlienZOQ);
			NPCPhrase_cb (UTWIG_DELAY9, &SelectAlienPIK);
			NPCPhrase_cb (UTWIG_DELAY10, &SelectAlienZOQ);
			NPCPhrase_cb (UTWIG_DELAY11, &SelectAlienPIK);
			NPCPhrase_cb (UTWIG_DELAY12, &SelectAlienZOQ);
			NPCPhrase_cb (UTWIG_DELAY13, &SelectAlienPIK);
			ZFPTalkSegue ((COUNT)~0);

			KnowMask |= UTWIG_BUY_TIME;
		}
		else
		{
			SIZE i;

			i = START_YEAR + YEARS_TO_KOHRAH_VICTORY;
			if (NumVisits)
				++i;
			if ((i -= GLOBAL (GameClock.year_index)) == 1
					&& GLOBAL (GameClock.month_index) > 2)
				i = 0;
			if (!(KnowMask & URQUAN_LOSE) && i <= 0)
			{
				NPCPhrase_cb (URQUAN_NEARLY_GONE0, &SelectAlienZOQ);
				NPCPhrase_cb (URQUAN_NEARLY_GONE1, &SelectAlienPIK);
				NPCPhrase_cb (URQUAN_NEARLY_GONE2, &SelectAlienZOQ);
				NPCPhrase_cb (URQUAN_NEARLY_GONE3, &SelectAlienPIK);
				NPCPhrase_cb (URQUAN_NEARLY_GONE4, &SelectAlienZOQ);
				NPCPhrase_cb (URQUAN_NEARLY_GONE5, &SelectAlienPIK);
				ZFPTalkSegue ((COUNT)~0);

				KnowMask |= KOHR_AH_WIN | URQUAN_LOSE;
			}
			else if (!(KnowMask & KOHR_AH_WIN) && i == 1)
			{
				NPCPhrase_cb (KOHRAH_WINNING0, &SelectAlienZOQ);
				NPCPhrase_cb (KOHRAH_WINNING1, &SelectAlienPIK);
				NPCPhrase_cb (KOHRAH_WINNING2, &SelectAlienZOQ);
				NPCPhrase_cb (KOHRAH_WINNING3, &SelectAlienPIK);
				NPCPhrase_cb (KOHRAH_WINNING4, &SelectAlienZOQ);
				NPCPhrase_cb (KOHRAH_WINNING5, &SelectAlienPIK);
				NPCPhrase_cb (KOHRAH_WINNING6, &SelectAlienZOQ);
				NPCPhrase_cb (KOHRAH_WINNING7, &SelectAlienPIK);
				NPCPhrase_cb (KOHRAH_WINNING8, &SelectAlienZOQ);
				NPCPhrase_cb (KOHRAH_WINNING9, &SelectAlienPIK);
				ZFPTalkSegue ((COUNT)~0);

				KnowMask |= KOHR_AH_WIN;
			}
			else
			{
				NPCPhrase_cb (NO_WAR_NEWS0, &SelectAlienZOQ);
				NPCPhrase_cb (NO_WAR_NEWS1, &SelectAlienPIK);
				ZFPTalkSegue ((COUNT)~0);
			}
		}
		SET_GAME_STATE (ZOQFOT_KNOW_MASK, KnowMask);

		DISABLE_PHRASE (any_war_news);
	}
	else if (PLAYER_SAID (R, i_want_alliance))
	{
		NPCPhrase_cb (GOOD0, &SelectAlienZOQ);
		NPCPhrase_cb (GOOD1, &SelectAlienPIK);
		NPCPhrase_cb (GOOD2, &SelectAlienZOQ);
		NPCPhrase_cb (GOOD3, &SelectAlienPIK);
		NPCPhrase_cb (GOOD4, &SelectAlienZOQ);
		NPCPhrase_cb (GOOD5, &SelectAlienPIK);
		NPCPhrase_cb (GOOD6, &SelectAlienZOQ);
		NPCPhrase_cb (GOOD7, &SelectAlienPIK);
		NPCPhrase_cb (GOOD8, &SelectAlienZOQ);
		NPCPhrase_cb (GOOD9, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);

		SetRaceAllied (ZOQFOTPIK_SHIP, TRUE);
		AddEvent (RELATIVE_EVENT, 3, 0, 0, ZOQFOT_DISTRESS_EVENT);
	}
	else if (PLAYER_SAID (R, enough_info))
	{
		NPCPhrase_cb (OK_ENOUGH_INFO, &SelectAlienZOQ);
		ZFPTalkSegue ((COUNT)~0);
	}

	if (PHRASE_ENABLED (whats_up_homeworld))
		Response (whats_up_homeworld, ZoqFotHome);
	if (PHRASE_ENABLED (any_war_news))
		Response (any_war_news, ZoqFotHome);
	if (CheckAlliance (ZOQFOTPIK_SHIP) != GOOD_GUY)
		Response (i_want_alliance, ZoqFotHome);
	else if (PHRASE_ENABLED (want_specific_info))
	{
		Response (want_specific_info, ZoqFotInfo);
	}
	Response (bye_homeworld, ExitConversation);
}

static void
Intro (void)
{
	BYTE NumVisits;

	if (LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
	{
		NPCPhrase_cb (OUT_TAKES0, &SelectAlienZOQ);
		NPCPhrase_cb (OUT_TAKES1, &SelectAlienPIK);
		NPCPhrase_cb (OUT_TAKES2, &SelectAlienZOQ);
		NPCPhrase_cb (OUT_TAKES3, &SelectAlienPIK);
		NPCPhrase_cb (OUT_TAKES4, &SelectAlienZOQ);
		NPCPhrase_cb (OUT_TAKES5, &SelectAlienPIK);
		NPCPhrase_cb (OUT_TAKES6, &SelectAlienZOQ);
		NPCPhrase_cb (OUT_TAKES7, &SelectAlienPIK);
		NPCPhrase_cb (OUT_TAKES8, &SelectAlienZOQ);
		NPCPhrase_cb (OUT_TAKES9, &SelectAlienPIK);
		NPCPhrase_cb (OUT_TAKES10, &SelectAlienZOQ);
		NPCPhrase_cb (OUT_TAKES11, &SelectAlienPIK);
		NPCPhrase_cb (OUT_TAKES12, &SelectAlienZOQ);
		NPCPhrase_cb (OUT_TAKES13, &SelectAlienPIK);
		ZFPTalkSegue ((COUNT)~0);
		setSegue (Segue_peace);
		return;
	}

	if (GET_GAME_STATE (ZOQFOT_HOSTILE))
	{
		NumVisits = GET_GAME_STATE (ZOQFOT_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase_cb (HOSTILE_HELLO_10, &SelectAlienZOQ);
				NPCPhrase_cb (HOSTILE_HELLO_11, &SelectAlienPIK);
				ZFPTalkSegue ((COUNT)~0);
				break;
			case 1:
				NPCPhrase_cb (HOSTILE_HELLO_20, &SelectAlienZOQ);
				NPCPhrase_cb (HOSTILE_HELLO_21, &SelectAlienPIK);
				NPCPhrase_cb (HOSTILE_HELLO_22, &SelectAlienZOQ);
				NPCPhrase_cb (HOSTILE_HELLO_23, &SelectAlienPIK);
				NPCPhrase_cb (HOSTILE_HELLO_24, &SelectAlienZOQ);
				NPCPhrase_cb (HOSTILE_HELLO_25, &SelectAlienPIK);
				ZFPTalkSegue ((COUNT)~0);
				break;
			case 2:
				NPCPhrase_cb (HOSTILE_HELLO_30, &SelectAlienZOQ);
				NPCPhrase_cb (HOSTILE_HELLO_31, &SelectAlienPIK);
				ZFPTalkSegue ((COUNT)~0);
				break;
			case 3:
				NPCPhrase_cb (HOSTILE_HELLO_40, &SelectAlienZOQ);
				NPCPhrase_cb (HOSTILE_HELLO_41, &SelectAlienPIK);
				ZFPTalkSegue ((COUNT)~0);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (ZOQFOT_HOME_VISITS, NumVisits);

		setSegue (Segue_hostile);
	}
	else if (!GET_GAME_STATE (MET_ZOQFOT))
	{
		SET_GAME_STATE (MET_ZOQFOT, 1);

		NPCPhrase_cb (WE_ARE0, &SelectAlienZOQ);
		NPCPhrase_cb (WE_ARE1, &SelectAlienPIK);
		NPCPhrase_cb (WE_ARE2, &SelectAlienZOQ);
		NPCPhrase_cb (WE_ARE3, &SelectAlienPIK);
		NPCPhrase_cb (WE_ARE4, &SelectAlienZOQ);
		NPCPhrase_cb (WE_ARE5, &SelectAlienPIK);
		NPCPhrase_cb (WE_ARE6, &SelectAlienZOQ);
		NPCPhrase_cb (WE_ARE7, &SelectAlienPIK);

		if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NPCPhrase_cb (INIT_HOME_HELLO0, &SelectAlienZOQ);
			NPCPhrase_cb (INIT_HOME_HELLO1, &SelectAlienPIK);
			NPCPhrase_cb (INIT_HOME_HELLO2, &SelectAlienZOQ);
			NPCPhrase_cb (INIT_HOME_HELLO3, &SelectAlienPIK);
		}
		else
		{
			NPCPhrase_cb (SCOUT_HELLO0, &SelectAlienZOQ);
			NPCPhrase_cb (SCOUT_HELLO1, &SelectAlienPIK);
			NPCPhrase_cb (SCOUT_HELLO2, &SelectAlienZOQ);
			NPCPhrase_cb (SCOUT_HELLO3, &SelectAlienPIK);
		}

		ZFPTalkSegue ((COUNT)~0);

		AquaintZoqFot (0);
	}
	else
	{
		if (GET_GAME_STATE (ZOQFOT_DISTRESS))
		{
#define MAX_ZFP_SHIPS 4
			NPCPhrase_cb (THANKS_FOR_RESCUE0, &SelectAlienZOQ);
			NPCPhrase_cb (THANKS_FOR_RESCUE1, &SelectAlienPIK);
			NPCPhrase_cb (THANKS_FOR_RESCUE2, &SelectAlienZOQ);
			NPCPhrase_cb (THANKS_FOR_RESCUE3, &SelectAlienPIK);
			NPCPhrase_cb (THANKS_FOR_RESCUE4, &SelectAlienZOQ);
			NPCPhrase_cb (THANKS_FOR_RESCUE5, &SelectAlienPIK);
			NPCPhrase_cb (THANKS_FOR_RESCUE6, &SelectAlienZOQ);
			NPCPhrase_cb (THANKS_FOR_RESCUE7, &SelectAlienPIK);
			NPCPhrase_cb (THANKS_FOR_RESCUE8, &SelectAlienZOQ);
			NPCPhrase_cb (THANKS_FOR_RESCUE9, &SelectAlienPIK);
			NPCPhrase_cb (THANKS_FOR_RESCUE10, &SelectAlienZOQ);
			NPCPhrase_cb (THANKS_FOR_RESCUE11, &SelectAlienPIK);
			ZFPTalkSegue ((COUNT)~0);

			SET_GAME_STATE (ZOQFOT_DISTRESS, 0);
			AddEscortShips (ZOQFOTPIK_SHIP, MAX_ZFP_SHIPS);
		}
		else
		{
			NumVisits = GET_GAME_STATE (ZOQFOT_HOME_VISITS);
			if (CheckAlliance (ZOQFOTPIK_SHIP) != GOOD_GUY)
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase_cb (NEUTRAL_HOME_HELLO_10, &SelectAlienZOQ);
						NPCPhrase_cb (NEUTRAL_HOME_HELLO_11, &SelectAlienPIK);
						NPCPhrase_cb (NEUTRAL_HOME_HELLO_12, &SelectAlienZOQ);
						NPCPhrase_cb (NEUTRAL_HOME_HELLO_13, &SelectAlienPIK);
						break;
					case 1:
						NPCPhrase_cb (NEUTRAL_HOME_HELLO_20, &SelectAlienZOQ);
						NPCPhrase_cb (NEUTRAL_HOME_HELLO_21, &SelectAlienPIK);
						NPCPhrase_cb (NEUTRAL_HOME_HELLO_22, &SelectAlienZOQ);
						NPCPhrase_cb (NEUTRAL_HOME_HELLO_23, &SelectAlienPIK);
						--NumVisits;
						break;
				}
				ZFPTalkSegue ((COUNT)~0);
			}
			else
			{
				switch (NumVisits++)
				{
					case 0:
						NPCPhrase_cb (ALLIED_HOME_HELLO_10, &SelectAlienZOQ);
						NPCPhrase_cb (ALLIED_HOME_HELLO_11, &SelectAlienPIK);
						NPCPhrase_cb (ALLIED_HOME_HELLO_12, &SelectAlienZOQ);
						NPCPhrase_cb (ALLIED_HOME_HELLO_13, &SelectAlienPIK);
						ZFPTalkSegue ((COUNT)~0);
						break;
					case 1:
						NPCPhrase_cb (ALLIED_HOME_HELLO_20, &SelectAlienZOQ);
						NPCPhrase_cb (ALLIED_HOME_HELLO_21, &SelectAlienPIK);
						NPCPhrase_cb (ALLIED_HOME_HELLO_22, &SelectAlienZOQ);
						NPCPhrase_cb (ALLIED_HOME_HELLO_23, &SelectAlienPIK);
						NPCPhrase_cb (ALLIED_HOME_HELLO_24, &SelectAlienZOQ);
						NPCPhrase_cb (ALLIED_HOME_HELLO_25, &SelectAlienPIK);
						NPCPhrase_cb (ALLIED_HOME_HELLO_26, &SelectAlienZOQ);
						NPCPhrase_cb (ALLIED_HOME_HELLO_27, &SelectAlienPIK);
						ZFPTalkSegue ((COUNT)~0);
						break;
					case 2:
						NPCPhrase_cb (ALLIED_HOME_HELLO_30, &SelectAlienZOQ);
						NPCPhrase_cb (ALLIED_HOME_HELLO_31, &SelectAlienPIK);
						ZFPTalkSegue ((COUNT)~0);
						break;
					case 3:
						NPCPhrase_cb (ALLIED_HOME_HELLO_40, &SelectAlienZOQ);
						NPCPhrase_cb (ALLIED_HOME_HELLO_41, &SelectAlienPIK);
						ZFPTalkSegue ((COUNT)~0);
						--NumVisits;
						break;
				}
			}
			SET_GAME_STATE (ZOQFOT_HOME_VISITS, NumVisits);
		}

		ZoqFotHome (0);
	}
}

static COUNT
uninit_zoqfot (void)
{
	luaUqm_comm_uninit ();
	return (0);
}

static void
post_zoqfot_enc (void)
{
	// nothing defined so far
}

LOCDATA*
init_zoqfot_comm (void)
{
	LOCDATA *retval;

	zoqfot_desc.init_encounter_func = Intro;
	zoqfot_desc.post_encounter_func = post_zoqfot_enc;
	zoqfot_desc.uninit_encounter_func = uninit_zoqfot;

	luaUqm_comm_init (NULL, NULL_RESOURCE);
			// Initialise Lua for string interpolation. This will be
			// generalised in the future.

	zoqfot_desc.AlienTextWidth = (SIS_TEXT_WIDTH >> 1) - TEXT_X_OFFS;

	if (CheckAlliance (ZOQFOTPIK_SHIP) == GOOD_GUY
			|| LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
	{
		setSegue (Segue_peace);
	}
	else
	{
		setSegue (Segue_hostile);
	}
	
	retval = &zoqfot_desc;

	return (retval);
}

