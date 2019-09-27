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


static LOCDATA pkunk_desc =
{
	PKUNK_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	PKUNK_PMAP_ANIM, /* AlienFrame */
	PKUNK_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	PKUNK_COLOR_MAP, /* AlienColorMap */
	PKUNK_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	PKUNK_CONVERSATION_PHRASES, /* PlayerPhrases */
	3, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			3, /* StartIndex */
			4, /* NumFrames */
			CIRCULAR_ANIM
					| WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			7, /* StartIndex */
			4, /* NumFrames */
			YOYO_ANIM
					| WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 2), /* BlockMask */
		},
		{
			11, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM
					| WAIT_TALKING, /* AnimFlags */
			ONE_SECOND / 30, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 1), /* BlockMask */
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
		2, /* NumFrames */
		0, /* AnimFlags */
		ONE_SECOND / 15, ONE_SECOND / 6, /* FrameRate */
		ONE_SECOND / 12, ONE_SECOND / 2, /* RestartRate */
		0, /* BlockMask */
	},
	NULL, /* AlienNumberSpeech - none */
	/* Filler for loaded resources */
	NULL, NULL, NULL,
	NULL,
	NULL,
};

static BOOLEAN
ShipsReady (void)
{
	SIZE i;

	return (GET_GAME_STATE (PKUNK_MANNER) == 3
		&& !((i = (GLOBAL (GameClock.year_index) - START_YEAR) - GET_GAME_STATE (PKUNK_SHIP_YEAR)) < 0
		     || ((i == 0 && (i = GLOBAL (GameClock.month_index) - GET_GAME_STATE (PKUNK_SHIP_MONTH)) < 0)
			 || (i == 0 && GLOBAL (GameClock.day_index) < GET_GAME_STATE (PKUNK_SHIP_DAY)))));
}

static void
PrepareShip (void)
{
#define MAX_PKUNK_SHIPS 4
	if (AddEscortShips (PKUNK_SHIP, MAX_PKUNK_SHIPS))
	{
		BYTE mi, di, yi;

		mi = GLOBAL (GameClock.month_index);
		SET_GAME_STATE (PKUNK_SHIP_MONTH, mi);
		if ((di = GLOBAL (GameClock.day_index)) > 28)
			di = 28;
		SET_GAME_STATE (PKUNK_SHIP_DAY, di);
		yi = (BYTE)(GLOBAL (GameClock.year_index) - START_YEAR) + 1;
		SET_GAME_STATE (PKUNK_SHIP_YEAR, yi);
	}
}

#define GOOD_REASON_1 (1 << 0)
#define GOOD_REASON_2 (1 << 1)
#define BAD_REASON_1 (1 << 2)
#define BAD_REASON_2 (1 << 3)

static void
ExitConversation (RESPONSE_REF R)
{
	setSegue (Segue_peace);

	if (PLAYER_SAID (R, friendly_bye_space))
		NPCPhrase (FRIENDLY_GOODBYE_SPACE);
	else if (PLAYER_SAID (R, neutral_bye_space))
		NPCPhrase (NEUTRAL_GOODBYE_SPACE);
	else if (PLAYER_SAID (R, bye_angry))
		NPCPhrase (GOODBYE_ANGRY);
	else if (PLAYER_SAID (R, bye_friendly))
		NPCPhrase (GOODBYE_FRIENDLY);
	else if (PLAYER_SAID (R, we_here_to_help)
			|| PLAYER_SAID (R, we_need_help))
	{
		if (PLAYER_SAID (R, we_here_to_help))
			NPCPhrase (NEED_HELP);
		else
			NPCPhrase (GIVE_HELP);
		NPCPhrase (ALMOST_ALLIANCE);

		SET_GAME_STATE (PKUNK_MANNER, 3);
		SET_GAME_STATE (PKUNK_VISITS, 0);
		SET_GAME_STATE (PKUNK_HOME_VISITS, 0);
		SET_GAME_STATE (PKUNK_INFO, 0);

		AddEvent (RELATIVE_EVENT, 6, 0, 0, ADVANCE_PKUNK_MISSION);
		if (EscortFeasibilityStudy (PKUNK_SHIP) == 0)
			NPCPhrase (INIT_NO_ROOM);
		else
		{
			NPCPhrase (INIT_SHIP_GIFT);
			AlienTalkSegue ((COUNT)~0);
			PrepareShip ();
		}
	}
	else if (PLAYER_SAID (R, try_to_be_nicer))
	{
		NPCPhrase (CANT_ASK_FOR_MORE);
		NPCPhrase (VISIT_OUR_HOMEWORLD);

		SET_GAME_STATE (PKUNK_MANNER, 3);
		SET_GAME_STATE (PKUNK_VISITS, 0);
		SET_GAME_STATE (PKUNK_HOME_VISITS, 0);
		SET_GAME_STATE (PKUNK_INFO, 0);
	}
	else if (PLAYER_SAID (R, must_conquer)
			|| PLAYER_SAID (R, obey))
	{
		if (PLAYER_SAID (R, obey))
			NPCPhrase (NO_OBEY);
		else
		{
			NPCPhrase (BAD_IDEA);

			SET_GAME_STATE (PKUNK_MANNER, 2);
		}
		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, die_idiot_fools))
	{
		NPCPhrase (VERY_WELL);

		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, suit_yourself))
		NPCPhrase (GOODBYE_MIGRATION);
	else
	{
		BYTE ReasonMask;

		ReasonMask = GET_GAME_STATE (PKUNK_REASONS);
		if (PLAYER_SAID (R, good_reason_1))
		{
			NPCPhrase (WE_GO_HOME_1);
			ReasonMask |= GOOD_REASON_1;
			AddEvent (RELATIVE_EVENT, 0, 0, 0, ADVANCE_PKUNK_MISSION);
		}
		else if (PLAYER_SAID (R, good_reason_2))
		{
			NPCPhrase (WE_GO_HOME_2);
			ReasonMask |= GOOD_REASON_2;
			AddEvent (RELATIVE_EVENT, 0, 0, 0, ADVANCE_PKUNK_MISSION);
		}
		else if (PLAYER_SAID (R, bad_reason_1))
		{
			NPCPhrase (NO_GO_HOME_1);
			ReasonMask |= BAD_REASON_1;
		}
		else if (PLAYER_SAID (R, bad_reason_2))
		{
			NPCPhrase (NO_GO_HOME_2);
			ReasonMask |= BAD_REASON_2;
		}
		SET_GAME_STATE (PKUNK_REASONS, ReasonMask);
	}
}

static void PkunkHome (RESPONSE_REF R);

static void
PkunkAngry (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, why_insults))
	{
		NPCPhrase (RELEASE_TENSION);

		DISABLE_PHRASE (why_insults);
	}
	else if (PLAYER_SAID (R, what_about_you))
	{
		NPCPhrase (ABOUT_US);

		DISABLE_PHRASE (what_about_you);
	}
	else if (PLAYER_SAID (R, should_be_friends))
	{
		NPCPhrase (YES_FRIENDS);

		DISABLE_PHRASE (should_be_friends);
	}

	if (PHRASE_ENABLED (should_be_friends))
	{
		Response (should_be_friends, PkunkAngry);
	}
	else
	{
		if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7))
			Response (try_to_be_nicer, PkunkHome);
		else
			Response (try_to_be_nicer, ExitConversation);
	}
	Response (die_idiot_fools, ExitConversation);
	if (PHRASE_ENABLED (why_insults))
		Response (why_insults, PkunkAngry);
	if (PHRASE_ENABLED (what_about_you))
		Response (what_about_you, PkunkAngry);
	Response (bye_angry, ExitConversation);
}

static void
DiscussConquer (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, we_conquer))
	{
		NPCPhrase (WHY_CONQUER);

		DISABLE_PHRASE (we_conquer);
	}
	else if (PLAYER_SAID (R, conquer_because_1))
	{
#if 0
		NPCPhrase (NOT_CONQUER_10);
		NPCPhrase (GLOBAL_ALLIANCE_NAME + name_1);
		NPCPhrase (NOT_CONQUER_11);
		NPCPhrase (GLOBAL_ALLIANCE_NAME + name_1);
		NPCPhrase (NOT_CONQUER_12);
#endif
		NPCPhrase (NOT_CONQUER_1);

		DISABLE_PHRASE (conquer_because_1);
	}
	else if (PLAYER_SAID (R, conquer_because_2))
	{
		NPCPhrase (NOT_CONQUER_2);

		DISABLE_PHRASE (conquer_because_2);
	}

	if (PHRASE_ENABLED (conquer_because_1))
	{
#if 0
		UNICODE buf[ALLIANCE_NAME_BUFSIZE];

		GetAllianceName (buf, name_1);
		construct_response (
				shared_phrase_buf,
				conquer_because_1,
				buf,
				(RESPONSE_REF)-1);
		DoResponsePhrase (conquer_because_1, DiscussConquer, shared_phrase_buf);
#endif
		Response(conquer_because_1, DiscussConquer);
	}
	if (PHRASE_ENABLED (conquer_because_2))
		Response (conquer_because_2, DiscussConquer);
	Response (must_conquer, ExitConversation);
	Response (no_conquest, PkunkHome);
}

static void
OfferAlliance (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, we_are_vindicator))
		NPCPhrase (WHY_YOU_HERE);
	else if (PLAYER_SAID (R, exploring_universe))
	{
		NPCPhrase (SENSE_DEEPER_CONFLICT);

		DISABLE_PHRASE (exploring_universe);
	}
	else if (PLAYER_SAID (R, fun_cruise))
	{
		NPCPhrase (REPRESS);

		DISABLE_PHRASE (fun_cruise);
	}

	Response (we_here_to_help, ExitConversation);
	Response (we_need_help, ExitConversation);
	if (PHRASE_ENABLED (exploring_universe))
		Response (exploring_universe, OfferAlliance);
	if (PHRASE_ENABLED (fun_cruise))
		Response (fun_cruise, OfferAlliance);
}

static void
AboutPkunk (RESPONSE_REF R)
{
	BYTE InfoLeft;

	InfoLeft = FALSE;
	if (PLAYER_SAID (R, what_about_you))
		NPCPhrase (ABOUT_US);
	else if (PLAYER_SAID (R, what_about_history))
	{
		NPCPhrase (ABOUT_HISTORY);

		DISABLE_PHRASE (what_about_history);
	}
	else if (PLAYER_SAID (R, what_about_yehat))
	{
		NPCPhrase (ABOUT_YEHAT);

		DISABLE_PHRASE (what_about_yehat);
	}
	else if (PLAYER_SAID (R, what_about_culture))
	{
		NPCPhrase (ABOUT_CULTURE);

		DISABLE_PHRASE (what_about_culture);
	}
	else if (PLAYER_SAID (R, elaborate_culture))
	{
		NPCPhrase (OK_ELABORATE_CULTURE);

		DISABLE_PHRASE (elaborate_culture);
	}
	else if (PLAYER_SAID (R, what_about_future))
	{
		NPCPhrase (ABOUT_FUTURE);

		DISABLE_PHRASE (what_about_future);
	}

	if (PHRASE_ENABLED (what_about_history))
	{
		Response (what_about_history, AboutPkunk);
		InfoLeft = TRUE;
	}
	else if (PHRASE_ENABLED (what_about_yehat))
	{
		Response (what_about_yehat, AboutPkunk);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_culture))
	{
		Response (what_about_culture, AboutPkunk);
		InfoLeft = TRUE;
	}
	else if (PHRASE_ENABLED (elaborate_culture))
	{
		Response (elaborate_culture, AboutPkunk);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_future))
	{
		Response (what_about_future, AboutPkunk);
		InfoLeft = TRUE;
	}
	Response (enough_about_you, PkunkHome);

	if (!InfoLeft)
	{
		DISABLE_PHRASE (what_about_you);
	}
}

static void
AboutIlwrath (RESPONSE_REF R)
{
	BYTE InfoLeft;

	InfoLeft = FALSE;
	if (PLAYER_SAID (R, what_about_ilwrath))
		NPCPhrase (ABOUT_ILWRATH);
	else if (PLAYER_SAID (R, why_ilwrath_fight))
	{
		NPCPhrase (ILWRATH_FIGHT_BECAUSE);

		DISABLE_PHRASE (why_ilwrath_fight);
	}
	else if (PLAYER_SAID (R, when_fight_start))
	{
		NPCPhrase (FIGHT_START_WHEN);

		DISABLE_PHRASE (when_fight_start);
	}
	else if (PLAYER_SAID (R, how_goes_fight))
	{
		NPCPhrase (FIGHT_GOES);

		DISABLE_PHRASE (how_goes_fight);
	}
	else if (PLAYER_SAID (R, how_stop_fight))
	{
		NPCPhrase (STOP_FIGHT_LIKE_SO);

		DISABLE_PHRASE (how_stop_fight);
	}

	if (PHRASE_ENABLED (why_ilwrath_fight))
	{
		Response (why_ilwrath_fight, AboutIlwrath);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (when_fight_start))
	{
		Response (when_fight_start, AboutIlwrath);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (how_goes_fight))
	{
		Response (how_goes_fight, AboutIlwrath);
		InfoLeft = TRUE;
	}
	if (PHRASE_ENABLED (how_stop_fight))
	{
		Response (how_stop_fight, AboutIlwrath);
		InfoLeft = TRUE;
	}
	Response (enough_ilwrath, PkunkHome);

	if (!InfoLeft)
	{
		DISABLE_PHRASE (what_about_ilwrath);
	}
}

static void
PkunkHome (RESPONSE_REF R)
{
	BYTE NumVisits;

	if (PLAYER_SAID (R, no_conquest))
		NPCPhrase (GOOD_IDEA);
	else if (PLAYER_SAID (R, enough_ilwrath))
		NPCPhrase (OK_ENOUGH_ILWRATH);
	else if (PLAYER_SAID (R, enough_about_you))
		NPCPhrase (OK_ENOUGH_ABOUT_US);
	else if (PLAYER_SAID (R, where_fleet_1)
			|| PLAYER_SAID (R, where_fleet_2)
			|| PLAYER_SAID (R, where_fleet_3))
	{
		SET_GAME_STATE (PKUNK_SWITCH, 1);
		if (!(GET_GAME_STATE (PKUNK_MISSION) & 1))
		{
			NumVisits = GET_GAME_STATE (PKUNK_RETURN);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (RETURNING_FROM_YEHAT_1);
					break;
				case 1:
					NPCPhrase (RETURNING_FROM_YEHAT_2);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (PKUNK_RETURN, NumVisits);
		}
		else
		{
			NumVisits = GET_GAME_STATE (PKUNK_MIGRATE);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (MIGRATING_HOMEWORLD_1);
					break;
				case 1:
					NPCPhrase (MIGRATING_HOMEWORLD_2);
					break;
				case 2:
					NPCPhrase (MIGRATING_HOMEWORLD_3);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (PKUNK_MIGRATE, NumVisits);
		}

		NumVisits = GET_GAME_STATE (PKUNK_FLEET) + 1;
		SET_GAME_STATE (PKUNK_FLEET, NumVisits);

		DISABLE_PHRASE (where_fleet_1);
	}
	else if (PLAYER_SAID (R, am_worried_1)
			|| PLAYER_SAID (R, am_worried_2)
			|| PLAYER_SAID (R, am_worried_3))
	{
		NumVisits = GET_GAME_STATE (PKUNK_WORRY);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (DONT_WORRY_1);
				break;
			case 1:
				NPCPhrase (DONT_WORRY_2);
				break;
			case 2:
				NPCPhrase (DONT_WORRY_3);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (PKUNK_WORRY, NumVisits);

		DISABLE_PHRASE (am_worried_1);
	}
	else if (PLAYER_SAID (R, try_to_be_nicer))
	{
		NPCPhrase (CANT_ASK_FOR_MORE);
		if (!GET_GAME_STATE (CLEAR_SPINDLE))
		{
			NPCPhrase (GIVE_SPINDLE);

			SET_GAME_STATE (CLEAR_SPINDLE, 1);
			SET_GAME_STATE (CLEAR_SPINDLE_ON_SHIP, 1);
		}
		NPCPhrase (CAN_BE_FRIENDS);

		SET_GAME_STATE (PKUNK_MANNER, 3);
		SET_GAME_STATE (PKUNK_VISITS, 0);
		SET_GAME_STATE (PKUNK_HOME_VISITS, 0);
	}
	else if (PLAYER_SAID (R, what_about_ilwrath))
	{
		NPCPhrase (ABOUT_ILWRATH /* ILWRATH_GONE */);

		DISABLE_PHRASE (what_about_ilwrath);
	}

	if (PHRASE_ENABLED (we_conquer) && GET_GAME_STATE (PKUNK_MANNER) == 0)
	{
		Response (we_conquer, DiscussConquer);
	}
	if (GET_GAME_STATE (PKUNK_ON_THE_MOVE))
	{
		if (PHRASE_ENABLED (where_fleet_1) && !GET_GAME_STATE (PKUNK_SWITCH))
		{
			switch (GET_GAME_STATE (PKUNK_FLEET))
			{
				case 0:
					Response (where_fleet_1, PkunkHome);
					break;
				case 1:
					Response (where_fleet_2, PkunkHome);
					break;
				case 2:
					Response (where_fleet_3, PkunkHome);
					break;
			}
		}
		else if (!PHRASE_ENABLED (where_fleet_1)
				&& PHRASE_ENABLED (am_worried_1)
				&& (GET_GAME_STATE (PKUNK_MISSION) & 1))
		{
			switch (GET_GAME_STATE (PKUNK_WORRY))
			{
				case 0:
					Response (am_worried_1, PkunkHome);
					break;
				case 1:
					Response (am_worried_2, PkunkHome);
					break;
				case 2:
					Response (am_worried_3, PkunkHome);
					break;
			}
		}
	}
	if (!GET_GAME_STATE (PKUNK_SHIP_MONTH))
	{
		Response (we_are_vindicator, OfferAlliance);
	}
	if (PHRASE_ENABLED (what_about_you))
	{
		Response (what_about_you, AboutPkunk);
	}
	if (PHRASE_ENABLED (what_about_ilwrath))
	{
		if (!GET_GAME_STATE (ILWRATH_DECEIVED))
		{
			Response (what_about_ilwrath, AboutIlwrath);
		}
		else
		{
			Response (what_about_ilwrath, PkunkHome);
		}
	}
	Response (bye_friendly, ExitConversation);
}

static void
PkunkFriendlySpace (RESPONSE_REF R)
{
	BYTE NumVisits;

	if (PLAYER_SAID (R, whats_up_space))
	{
		if (ShipsReady ())
			NPCPhrase (SHIPS_AT_HOME);
		else
		{
			NumVisits = GET_GAME_STATE (PKUNK_INFO);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (GENERAL_INFO_SPACE_1);
					break;
				case 1:
					NPCPhrase (GENERAL_INFO_SPACE_2);
					break;
				case 2:
					NPCPhrase (GENERAL_INFO_SPACE_3);
					break;
				case 3:
					NPCPhrase (GENERAL_INFO_SPACE_4);
					break;
				case 4:
					NPCPhrase (GENERAL_INFO_SPACE_5);
					break;
				case 5:
					NPCPhrase (GENERAL_INFO_SPACE_6);
					break;
				case 6:
					NPCPhrase (GENERAL_INFO_SPACE_7);
					break;
				case 7:
					NPCPhrase (GENERAL_INFO_SPACE_8);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (PKUNK_INFO, NumVisits);
		}

		DISABLE_PHRASE (whats_up_space);
	}
	else if (PLAYER_SAID (R, how_goes_war))
	{
		NumVisits = GET_GAME_STATE (PKUNK_WAR);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (WAR_GOES_1);
				SET_GAME_STATE (KNOW_URQUAN_STORY, 1);
				SET_GAME_STATE (KNOW_KOHR_AH_STORY, 1);
				break;
			case 1:
				NPCPhrase (WAR_GOES_2);
				break;
			case 2:
				NPCPhrase (WAR_GOES_3);
				break;
			case 3:
				NPCPhrase (WAR_GOES_4);
				SET_GAME_STATE (PKUNK_DONE_WAR, 1);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (PKUNK_WAR, NumVisits);

		DISABLE_PHRASE (how_goes_war);
	}
	else if (PLAYER_SAID (R, tell_my_fortune))
	{
		NumVisits = GET_GAME_STATE (PKUNK_FORTUNE);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (FORTUNE_IS_1);
				break;
			case 1:
				NPCPhrase (FORTUNE_IS_2);
				break;
			case 2:
				NPCPhrase (FORTUNE_IS_3);
				break;
			case 3:
				NPCPhrase (FORTUNE_IS_4);
				break;
			case 4:
				NPCPhrase (FORTUNE_IS_5);
				break;
			case 5:
				NPCPhrase (FORTUNE_IS_6);
				break;
			case 6:
				NPCPhrase (FORTUNE_IS_7);
				break;
			case 7:
				NPCPhrase (FORTUNE_IS_8);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (PKUNK_FORTUNE, NumVisits);

		DISABLE_PHRASE (tell_my_fortune);
	}

	if (PHRASE_ENABLED (whats_up_space))
		Response (whats_up_space, PkunkFriendlySpace);
	if (!GET_GAME_STATE (PKUNK_DONE_WAR) && PHRASE_ENABLED (how_goes_war))
		Response (how_goes_war, PkunkFriendlySpace);
	if (PHRASE_ENABLED (tell_my_fortune))
		Response (tell_my_fortune, PkunkFriendlySpace);
	Response (friendly_bye_space, ExitConversation);
}

static void
PkunkNeutralSpace (RESPONSE_REF R)
{
	BYTE NumVisits;

	if (PLAYER_SAID (R, form_alliance))
	{
		NPCPhrase (GO_TO_HOMEWORLD);

		DISABLE_PHRASE (form_alliance);
	}
	else if (PLAYER_SAID (R, can_you_help))
	{
		NPCPhrase (GO_TO_HOMEWORLD_AGAIN);

		DISABLE_PHRASE (can_you_help);
	}
	else if (PLAYER_SAID (R, hostile_greeting))
	{
		NPCPhrase (DONT_BE_HOSTILE);

		DISABLE_PHRASE (hostile_greeting);
	}
	else if (PLAYER_SAID (R, whats_up_neutral))
	{
		NumVisits = GET_GAME_STATE (PKUNK_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (GENERAL_INFO_SPACE_1);
				break;
			case 1:
				NPCPhrase (GENERAL_INFO_SPACE_2);
				break;
			case 2:
				NPCPhrase (GENERAL_INFO_SPACE_6 /* was 3 */);
				break;
			case 3:
				NPCPhrase (GENERAL_INFO_SPACE_7 /* was 4 */);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (PKUNK_INFO, NumVisits);

		DISABLE_PHRASE (whats_up_neutral);
	}

	if (PHRASE_ENABLED (form_alliance))
		Response (form_alliance, PkunkNeutralSpace);
	else if (PHRASE_ENABLED (can_you_help))
		Response (can_you_help, PkunkNeutralSpace);
	if (PHRASE_ENABLED (hostile_greeting))
		Response (hostile_greeting, PkunkNeutralSpace);
	else
	{
		Response (obey, ExitConversation);
	}
	if (PHRASE_ENABLED (whats_up_neutral))
		Response (whats_up_neutral, PkunkNeutralSpace);
	Response (neutral_bye_space, ExitConversation);
}

static void
PkunkMigrate (RESPONSE_REF R)
{
	BYTE ReasonMask;
	(void) R;  // ignored

	ReasonMask = GET_GAME_STATE (PKUNK_REASONS);
	if (!(ReasonMask & GOOD_REASON_1))
		Response (good_reason_1, ExitConversation);
	if (!(ReasonMask & BAD_REASON_1))
		Response (bad_reason_1, ExitConversation);
	if (!(ReasonMask & GOOD_REASON_2))
		Response (good_reason_2, ExitConversation);
	if (!(ReasonMask & BAD_REASON_2))
		Response (bad_reason_2, ExitConversation);
	Response (suit_yourself, ExitConversation);
}

static void
Intro (void)
{
	BYTE NumVisits, Manner;

	if (LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
	{
		NPCPhrase (OUT_TAKES);

		setSegue (Segue_peace);
		return;
	}

	Manner = GET_GAME_STATE (PKUNK_MANNER);
	if (Manner == 2)
	{
		// Irreparably Pissed off the Pkunk.
		NumVisits = GET_GAME_STATE (PKUNK_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (HATE_YOU_FOREVER_1);
				break;
			case 1:
				NPCPhrase (HATE_YOU_FOREVER_2);
				break;
			case 2:
				NPCPhrase (HATE_YOU_FOREVER_3);
				break;
			case 3:
				NPCPhrase (HATE_YOU_FOREVER_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (PKUNK_VISITS, NumVisits);

		setSegue (Segue_hostile);
	}
	else if (Manner == 1)
	{
		// Bad relations with the Pkunk, but not irreparably.
		NumVisits = GET_GAME_STATE (PKUNK_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (SPIRITUAL_PROBLEMS_1);
				break;
			case 1:
				NPCPhrase (SPIRITUAL_PROBLEMS_2);
				break;
			case 2:
				NPCPhrase (SPIRITUAL_PROBLEMS_3);
				break;
			case 3:
				NPCPhrase (SPIRITUAL_PROBLEMS_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (PKUNK_VISITS, NumVisits);

		PkunkAngry ((RESPONSE_REF)0);
	}
	else if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7))
	{
		// Encountering the Pkunk at their home world.
		if (!GET_GAME_STATE (CLEAR_SPINDLE))
		{
			NPCPhrase (GIVE_SPINDLE);

			SET_GAME_STATE (CLEAR_SPINDLE, 1);
			SET_GAME_STATE (CLEAR_SPINDLE_ON_SHIP, 1);
		}
		else if (!GET_GAME_STATE (PKUNK_SENSE_VICTOR)
				&& GLOBAL (GameClock.year_index) > START_YEAR
				&& !GET_GAME_STATE (KOHR_AH_FRENZY))
		{
			NPCPhrase (SENSE_KOHRAH_VICTORY);

			SET_GAME_STATE (PKUNK_SENSE_VICTOR, 1);
		}

		NumVisits = GET_GAME_STATE (PKUNK_HOME_VISITS);
		if (Manner == 0)
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (NEUTRAL_HOMEWORLD_HELLO_1);
					break;
				case 1:
					NPCPhrase (NEUTRAL_HOMEWORLD_HELLO_2);
					break;
				case 2:
					NPCPhrase (NEUTRAL_HOMEWORLD_HELLO_3);
					break;
				case 3:
					NPCPhrase (NEUTRAL_HOMEWORLD_HELLO_4);
					--NumVisits;
					break;
			}
		}
		else
		{
			if (NumVisits && ShipsReady ())
			{
				if (EscortFeasibilityStudy (PKUNK_SHIP) == 0)
					NPCPhrase (NO_ROOM);
				else
				{
					NPCPhrase (SHIP_GIFT);
					PrepareShip ();
				}
			}
			else switch (NumVisits++)
			{
				case 0:
					NPCPhrase (FRIENDLY_HOMEWORLD_HELLO_1);
					break;
				case 1:
					NPCPhrase (FRIENDLY_HOMEWORLD_HELLO_2);
					break;
				case 2:
					NPCPhrase (FRIENDLY_HOMEWORLD_HELLO_3);
					break;
				case 3:
					NPCPhrase (FRIENDLY_HOMEWORLD_HELLO_4);
					break;
				case 4:
					NPCPhrase (FRIENDLY_HOMEWORLD_HELLO_5);
					break;
				case 5:
					NPCPhrase (FRIENDLY_HOMEWORLD_HELLO_6);
					break;
				case 6:
					NPCPhrase (FRIENDLY_HOMEWORLD_HELLO_7);
					break;
				case 7:
					NPCPhrase (FRIENDLY_HOMEWORLD_HELLO_8);
					--NumVisits;
					break;
			}
		}
		SET_GAME_STATE (PKUNK_HOME_VISITS, NumVisits);

		PkunkHome ((RESPONSE_REF)0);
	}
	else if ((NumVisits = GET_GAME_STATE (PKUNK_MISSION)) == 0
			|| !(NumVisits & 1))
	{
		// Encountering a Pkunk ship in space, while they are not
		// migrating.
		NumVisits = GET_GAME_STATE (PKUNK_VISITS);
		if (Manner == 3)
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (FRIENDLY_SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase (FRIENDLY_SPACE_HELLO_2);
					break;
				case 2:
					NPCPhrase (FRIENDLY_SPACE_HELLO_3);
					break;
				case 3:
					NPCPhrase (FRIENDLY_SPACE_HELLO_4);
					break;
				case 4:
					NPCPhrase (FRIENDLY_SPACE_HELLO_5);
					break;
				case 5:
					NPCPhrase (FRIENDLY_SPACE_HELLO_6);
					break;
				case 6:
					NPCPhrase (FRIENDLY_SPACE_HELLO_7);
					break;
				case 7:
					NPCPhrase (FRIENDLY_SPACE_HELLO_8);
					--NumVisits;
					break;
			}

			PkunkFriendlySpace ((RESPONSE_REF)0);
		}
		else
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (NEUTRAL_SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase (NEUTRAL_SPACE_HELLO_2);
					break;
				case 2:
					NPCPhrase (NEUTRAL_SPACE_HELLO_3);
					break;
				case 3:
					NPCPhrase (NEUTRAL_SPACE_HELLO_4);
					--NumVisits;
					break;
			}

			PkunkNeutralSpace ((RESPONSE_REF)0);
		}
		SET_GAME_STATE (PKUNK_VISITS, NumVisits);

	}
	else
	{
		// Encountering a Pkunk ship in space, while they are
		// migrating.
		NumVisits = GET_GAME_STATE (PKUNK_MIGRATE_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (MIGRATING_SPACE_1);
				break;
			case 1:
				NPCPhrase (MIGRATING_SPACE_2);
				break;
			case 2:
				NPCPhrase (MIGRATING_SPACE_3);
				break;
			case 3:
				NPCPhrase (MIGRATING_SPACE_4);
				break;
			case 4:
				NPCPhrase (MIGRATING_SPACE_5);
				break;
			case 5:
				NPCPhrase (MIGRATING_SPACE_6);
				break;
			case 6:
				NPCPhrase (MIGRATING_SPACE_7);
				break;
			case 7:
				NPCPhrase (MIGRATING_SPACE_8);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (PKUNK_MIGRATE_VISITS, NumVisits);

		PkunkMigrate ((RESPONSE_REF)0);
	}
}

// Called after combat or communications
static COUNT
uninit_pkunk (void)
{
	luaUqm_comm_uninit();
	return (0);
}

static void
post_pkunk_enc (void)
{
	BYTE Manner;

	if (getSegue () == Segue_hostile
			&& (Manner = GET_GAME_STATE (PKUNK_MANNER)) != 2)
	{
		SET_GAME_STATE (PKUNK_MANNER, 1);
		if (Manner != 1)
		{
			SET_GAME_STATE (PKUNK_VISITS, 0);
			SET_GAME_STATE (PKUNK_HOME_VISITS, 0);
		}
	}
}

LOCDATA*
init_pkunk_comm (void)
{
	LOCDATA *retval;

	pkunk_desc.init_encounter_func = Intro;
	pkunk_desc.post_encounter_func = post_pkunk_enc;
	pkunk_desc.uninit_encounter_func = uninit_pkunk;

	luaUqm_comm_init(NULL, NULL_RESOURCE);
			// Initialise Lua for string interpolation. This will be
			// generalised in the future.

	pkunk_desc.AlienTextBaseline.x = TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	pkunk_desc.AlienTextBaseline.y = 0;
	pkunk_desc.AlienTextWidth = SIS_TEXT_WIDTH - 16;

	if (GET_GAME_STATE (PKUNK_MANNER) == 3
			|| LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
	{
		// Enter communications immediately.
		setSegue (Segue_peace);
	}
	else
	{
		// Ask the player whether to attack or talk.
		setSegue (Segue_hostile);
	}
	retval = &pkunk_desc;

	return (retval);
}


