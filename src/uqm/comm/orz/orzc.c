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
#include "../../nameref.h"
		//JMS_GFX: For LoadGraphic 

#include "uqm/setup.h"
		// for GraphicsLock


static LOCDATA orz_desc =
{
	ORZ_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	ORZ_PMAP_ANIM, /* AlienFrame */
	ORZ_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	ORZ_COLOR_MAP, /* AlienColorMap */
	ORZ_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	ORZ_CONVERSATION_PHRASES, /* PlayerPhrases */
	14, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			4, /* StartIndex */
			6, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			10, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			15, /* StartIndex */
			2, /* NumFrames */
			RANDOM_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 10, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			17, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 10, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			20, /* StartIndex */
			2, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND / 10, ONE_SECOND * 3, /* RestartRate */
			(1 << 7), /* BlockMask */
		},
		{
			22, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND / 10, ONE_SECOND * 3, /* RestartRate */
			(1 << 6), /* BlockMask */
		},
		{
			30, /* StartIndex */
			3, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND / 10, ONE_SECOND * 3, /* RestartRate */
			(1 << 5), /* BlockMask */
		},
		{
			33, /* StartIndex */
			3, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			ONE_SECOND / 10, ONE_SECOND * 3, /* RestartRate */
			(1 << 4), /* BlockMask */
		},
		{
			36, /* StartIndex */
			25, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 60, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			61, /* StartIndex */
			15, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 60, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			76, /* StartIndex */
			17, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 60, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 12), /* BlockMask */
		},
		{
			93, /* StartIndex */
			25, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 60, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			118, /* StartIndex */
			11, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 60, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 10), /* BlockMask */
		},
		{
			129, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM | ONE_SHOT_ANIM | WAIT_TALKING | ANIM_DISABLED, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
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
		3, /* NumFrames */
		0, /* AnimFlags */
		ONE_SECOND / 15, ONE_SECOND / 15, /* FrameRate */
		ONE_SECOND / 12, ONE_SECOND * 3 / 8, /* RestartRate */
		(1 << 13), /* BlockMask */
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

	if (PLAYER_SAID (R, bye_ally))
		NPCPhrase (GOODBYE_ALLY);
	else if (PLAYER_SAID (R, bye_neutral))
		NPCPhrase (GOODBYE_NEUTRAL);
	else if (PLAYER_SAID (R, bye_angry))
		NPCPhrase (GOODBYE_ANGRY);
	else if (PLAYER_SAID (R, bye_taalo))
	{
		if (GET_GAME_STATE (ORZ_MANNER) == 1)
			NPCPhrase (ANGRY_TAALO_GOODBYE);
		else
			NPCPhrase (FRIENDLY_TAALO_GOODBYE);
	}
	else if (PLAYER_SAID (R, hostile_2))
	{
		NPCPhrase (HOSTILITY_IS_BAD_2);
		
		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, may_we_land))
	{
		NPCPhrase (SURE_LAND);

		SET_GAME_STATE (TAALO_UNPROTECTED, 1);
	}
	else if (PLAYER_SAID (R, yes_alliance)
			|| PLAYER_SAID (R, were_sorry))
	{
		if (PLAYER_SAID (R, yes_alliance))
			NPCPhrase (GREAT);
		else
			NPCPhrase (APOLOGY_ACCEPTED);

		SET_GAME_STATE (ORZ_ANDRO_STATE, 0);
		SET_GAME_STATE (ORZ_GENERAL_INFO, 0);
		SET_GAME_STATE (ORZ_PERSONAL_INFO, 0);
		SET_GAME_STATE (ORZ_MANNER, 3);
		SetRaceAllied (ORZ_SHIP, TRUE);
	}
	else if (PLAYER_SAID (R, demand_to_land))
	{
		NPCPhrase (NO_DEMAND);

		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, about_andro_3)
			|| PLAYER_SAID (R, must_know_about_androsyn))
	{
		// JMS_GFX: Use separate graphics in hires instead of colormap transform.
		if (IS_HD)
		{
			int ii;
			for (ii = 0; ii < CommData.NumAnimations - 1; ii++)
				CommData.AlienAmbientArray[ii].AnimFlags |= ANIM_DISABLED;
			
			CommData.AlienAmbientArray[13].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienFrameRes = ORZ_ANGRY_PMAP_ANIM;
			CommData.AlienFrame = CaptureDrawable (LoadGraphic (CommData.AlienFrameRes));
		}

		if (PLAYER_SAID (R, about_andro_3))
			NPCPhrase (BLEW_IT);
		else
			NPCPhrase (KNOW_TOO_MUCH);

		// JMS_GFX: Use separate graphics in hires instead of colormap transform.
		if (IS_HD)
		{
			int ii;
			AlienTalkSegue (1);
			for (ii = 0; ii < CommData.NumAnimations - 1; ii++)
				CommData.AlienAmbientArray[ii].AnimFlags &= ~ANIM_DISABLED;
		}

		SET_GAME_STATE (ORZ_VISITS, 0);
		SET_GAME_STATE (ORZ_MANNER, 2);
		setSegue (Segue_hostile);
		if (PLAYER_SAID (R, about_andro_3))
		{
			SetRaceAllied (ORZ_SHIP, FALSE);
			RemoveEscortShips (ORZ_SHIP);
		}

		XFormColorMap (GetColorMapAddress (
				SetAbsColorMapIndex (CommData.AlienColorMap, 1)
				), ONE_SECOND / 2);
	}
	else /* insults */
	{
		BYTE NumVisits;

		NumVisits = GET_GAME_STATE (ORZ_PERSONAL_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (INSULTED_1);
				break;
			case 1:
				NPCPhrase (INSULTED_2);
				break;
			case 2:
				NPCPhrase (INSULTED_3);
				setSegue (Segue_hostile);
				break;
			case 7:
				--NumVisits;
			default:
				NPCPhrase (INSULTED_4);
				setSegue (Segue_hostile);
				break;
		}
		SET_GAME_STATE (ORZ_PERSONAL_INFO, NumVisits);
	}
}

static void
TaaloWorld (RESPONSE_REF R)
{
	// We can only get here when ORZ_MANNER != HOSTILE (2)
	BYTE Manner;

	Manner = GET_GAME_STATE (ORZ_MANNER);
	if (PLAYER_SAID (R, demand_to_land))
	{
		NPCPhrase (ASK_NICELY);

		DISABLE_PHRASE (demand_to_land);
	}
	else if (PLAYER_SAID (R, why_you_here))
	{
		if (Manner != 1)
			NPCPhrase (FRIENDLY_EXPLANATION);
		else
			NPCPhrase (ANGRY_EXPLANATION);

		DISABLE_PHRASE (why_you_here);
	}
	else if (PLAYER_SAID (R, what_is_this_place))
	{
		if (Manner != 1)
			NPCPhrase (FRIENDLY_PLACE);
		else
			NPCPhrase (ANGRY_PLACE);

		DISABLE_PHRASE (what_is_this_place);
	}
	else if (PLAYER_SAID (R, may_we_land))
	{
		NPCPhrase (ALLIES_CAN_VISIT);

		DISABLE_PHRASE (may_we_land);
	}
	else if (PLAYER_SAID (R, make_alliance))
	{
		NPCPhrase (CANT_ALLY_HERE);

		DISABLE_PHRASE (make_alliance);
	}
	else if (PLAYER_SAID (R, why_busy))
	{
		NPCPhrase (BUSY_BECAUSE);

		DISABLE_PHRASE (why_busy);
	}

	if (PHRASE_ENABLED (may_we_land))
	{
		if (Manner == 3 && CheckAlliance (ORZ_SHIP) == GOOD_GUY)
			Response (may_we_land, ExitConversation);
		else
			Response (may_we_land, TaaloWorld);
	}
	else if (PHRASE_ENABLED (make_alliance))
		Response (make_alliance, TaaloWorld);
	else if (PHRASE_ENABLED (why_busy))
		Response (why_busy, TaaloWorld);
	if (PHRASE_ENABLED (demand_to_land))
	{
		if (Manner == 1)
			Response (demand_to_land, ExitConversation);
		else
			Response (demand_to_land, TaaloWorld);
	}
	if (PHRASE_ENABLED (why_you_here))
		Response (why_you_here, TaaloWorld);
	if (PHRASE_ENABLED (what_is_this_place))
		Response (what_is_this_place, TaaloWorld);
	Response (bye_taalo, ExitConversation);
}

static void
OrzAllied (RESPONSE_REF R)
{
	BYTE NumVisits;

	if (PLAYER_SAID (R, whats_up_ally))
	{
		NumVisits = GET_GAME_STATE (ORZ_GENERAL_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (GENERAL_INFO_ALLY_1);
				break;
			case 1:
				NPCPhrase (GENERAL_INFO_ALLY_2);
				break;
			case 2:
				NPCPhrase (GENERAL_INFO_ALLY_3);
				break;
			case 3:
				NPCPhrase (GENERAL_INFO_ALLY_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (ORZ_GENERAL_INFO, NumVisits);

		DISABLE_PHRASE (whats_up_ally);
	}
	else if (PLAYER_SAID (R, more_about_you))
	{
		NumVisits = GET_GAME_STATE (ORZ_PERSONAL_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (ABOUT_US_1);
				break;
			case 1:
				NPCPhrase (ABOUT_US_2);
				break;
			case 2:
				NPCPhrase (ABOUT_US_3);
				break;
			case 3:
				NPCPhrase (ABOUT_US_4);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (ORZ_PERSONAL_INFO, NumVisits);

		DISABLE_PHRASE (more_about_you);
	}
	else if (PLAYER_SAID (R, about_andro_1))
	{
		NPCPhrase (FORGET_ANDRO_1);

		SET_GAME_STATE (ORZ_ANDRO_STATE, 1);
	}
	else if (PLAYER_SAID (R, about_andro_2))
	{
		NPCPhrase (FORGET_ANDRO_2);

		SET_GAME_STATE (ORZ_ANDRO_STATE, 2);
	}

	if (GET_GAME_STATE (ORZ_ANDRO_STATE) == 0)
		Response (about_andro_1, OrzAllied);
	else if (GET_GAME_STATE (ORZ_ANDRO_STATE) == 1)
		Response (about_andro_2, OrzAllied);
	else
	{
		Response (about_andro_3, ExitConversation);
	}
	if (PHRASE_ENABLED (whats_up_ally))
		Response (whats_up_ally, OrzAllied);
	if (PHRASE_ENABLED (more_about_you))
		Response (more_about_you, OrzAllied);
	Response (bye_ally, ExitConversation);
}

static void OrzNeutral (RESPONSE_REF R);

static void
WhereAndrosyn (RESPONSE_REF R)
{
	(void) R;  // ignored
	NPCPhrase (DISEMBLE_ABOUT_ANDROSYN);
	DISABLE_PHRASE (where_androsyn);

	Response (must_know_about_androsyn, ExitConversation);
	Response (dont_really_care, OrzNeutral);
}

static void
OfferAlliance (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, seem_like_nice_guys))
		NPCPhrase (ARE_NICE_WANT_ALLY);
	else if (PLAYER_SAID (R, talk_about_alliance))
		NPCPhrase (OK_TALK_ALLIANCE);
	else if (PLAYER_SAID (R, why_so_trusting))
	{
		NPCPhrase (TRUSTING_BECAUSE);

		SET_GAME_STATE (ORZ_STACK1, 1);
	}

	Response (no_alliance, OrzNeutral);
	Response (decide_later, OrzNeutral);
	if (GET_GAME_STATE (ORZ_STACK1) == 0)
	{
		Response (why_so_trusting, OfferAlliance);
	}
	Response (yes_alliance, ExitConversation);
}

static void
OrzNeutral (RESPONSE_REF R)
{
	BYTE i, LastStack;
	RESPONSE_REF pStr[3];

	LastStack = 0;
	pStr[0] = pStr[1] = pStr[2] = 0;
	if (PLAYER_SAID (R, hostile_1))
	{
		NPCPhrase (HOSTILITY_IS_BAD_1);

		DISABLE_PHRASE (hostile_1);
		LastStack = 2;
	}
	else if (PLAYER_SAID (R, we_are_vindicator))
	{
		NPCPhrase (NICE_TO_MEET_YOU);

		SET_GAME_STATE (ORZ_STACK0, 1);
		LastStack = 1;
	}
	else if (PLAYER_SAID (R, who_you))
	{
		NPCPhrase (WE_ARE_ORZ);

		SET_GAME_STATE (ORZ_ANDRO_STATE, 1);
	}
	else if (PLAYER_SAID (R, why_here))
	{
		NPCPhrase (HERE_BECAUSE);

		SET_GAME_STATE (ORZ_ANDRO_STATE, 2);
	}
	else if (PLAYER_SAID (R, no_alliance))
	{
		NPCPhrase (MAYBE_LATER);

		DISABLE_PHRASE (talk_about_alliance);
		SET_GAME_STATE (REFUSED_ORZ_ALLIANCE, 1);
	}
	else if (PLAYER_SAID (R, decide_later))
	{
		NPCPhrase (OK_LATER);

		DISABLE_PHRASE (talk_about_alliance);
		SET_GAME_STATE (REFUSED_ORZ_ALLIANCE, 1);
	}
	else if (PLAYER_SAID (R, dont_really_care))
		NPCPhrase (YOU_ARE_OUR_FRIENDS);
	else if (PLAYER_SAID (R, where_androsyn))
	{
		WhereAndrosyn (R);
		return;
	}
	else if (PLAYER_SAID (R, talk_about_alliance)
			|| PLAYER_SAID (R, seem_like_nice_guys))
	{
		OfferAlliance (R);
		return;
	}
	else if (PLAYER_SAID (R, hostile_2))
	{
		ExitConversation (R);
		return;
	}

	if (GET_GAME_STATE (ORZ_ANDRO_STATE) == 0)
		pStr[0] = who_you;
	else if (GET_GAME_STATE (ORZ_ANDRO_STATE) == 1)
		pStr[0] = why_here;
	else if (PHRASE_ENABLED (where_androsyn) && GET_GAME_STATE (ORZ_ANDRO_STATE) == 2)
		pStr[0] = where_androsyn;
	if (GET_GAME_STATE (REFUSED_ORZ_ALLIANCE))
	{
		if (PHRASE_ENABLED (talk_about_alliance))
			pStr[1] = talk_about_alliance;
	}
	else if (GET_GAME_STATE (ORZ_STACK0) == 0)
	{
		pStr[1] = we_are_vindicator;
	}
	else
		pStr[1] = seem_like_nice_guys;
	if (PHRASE_ENABLED (hostile_1))
		pStr[2] = hostile_1;
	else
		pStr[2] = hostile_2;

	if (pStr[LastStack])
		Response (pStr[LastStack], OrzNeutral);

	for (i = 0; i < 3; ++i)
	{
		if (i != LastStack && pStr[i])
			Response (pStr[i], OrzNeutral);
	}
	Response (bye_neutral, ExitConversation);
}

static void
OrzAngry (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, whats_up_angry))
	{
		BYTE NumVisits;

		NumVisits = GET_GAME_STATE (ORZ_GENERAL_INFO);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (GENERAL_INFO_ANGRY_1);
				break;
			case 1:
				NPCPhrase (GENERAL_INFO_ANGRY_2);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (ORZ_GENERAL_INFO, NumVisits);

		DISABLE_PHRASE (whats_up_angry);
	}

	if (PHRASE_ENABLED (whats_up_angry))
	{
		Response (whats_up_angry, OrzAngry);
	}
	Response (were_sorry, ExitConversation);
	switch (GET_GAME_STATE (ORZ_PERSONAL_INFO))
	{
		case 0:
			Response (insult_1, ExitConversation);
			break;
		case 1:
			Response (insult_2, ExitConversation);
			break;
		case 2:
			Response (insult_3, ExitConversation);
			break;
		case 3:
			Response (insult_4, ExitConversation);
			break;
		case 4:
			Response (insult_5, ExitConversation);
			break;
		case 5:
			Response (insult_6, ExitConversation);
			break;
		case 6:
			Response (insult_7, ExitConversation);
			break;
		case 7:
			Response (insult_8, ExitConversation);
			break;
	}
	Response (bye_angry, ExitConversation);
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

	if (!GET_GAME_STATE (MET_ORZ_BEFORE))
		NPCPhrase (INIT_HELLO);

	Manner = GET_GAME_STATE (ORZ_MANNER);
	if (Manner == 2)
	{
		CommData.AlienColorMap =
				SetAbsColorMapIndex (CommData.AlienColorMap, 1);

		// JMS_GFX: Use separate red angry graphics in hires instead of colormap transform.
		if (IS_HD) {
			CommData.AlienFrameRes = ORZ_ANGRY_PMAP_ANIM;
			CommData.AlienFrame = CaptureDrawable (
				LoadGraphic (CommData.AlienFrameRes));
		}

		NumVisits = GET_GAME_STATE (ORZ_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (HOSTILE_HELLO_1);
				break;
			case 1:
				NPCPhrase (HOSTILE_HELLO_2);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (ORZ_VISITS, NumVisits);

		setSegue (Segue_hostile);
	}
	else if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 6))
	{
		NumVisits = GET_GAME_STATE (TAALO_VISITS);
		if (Manner != 1)
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (FRIENDLY_ALLIED_TAALO_HELLO_1);
					break;
				case 1:
					NPCPhrase (FRIENDLY_ALLIED_TAALO_HELLO_2);
					--NumVisits;
					break;
			}
		}
		else
		{
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (ANGRY_TAALO_HELLO_1);
					break;
				case 1:
					NPCPhrase (ANGRY_TAALO_HELLO_2);
					--NumVisits;
					break;
			}
		}
		SET_GAME_STATE (TAALO_VISITS, NumVisits);

		TaaloWorld ((RESPONSE_REF)0);
	}
	else if (Manner == 3 && CheckAlliance (ORZ_SHIP) == GOOD_GUY)
	{
		if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = GET_GAME_STATE (ORZ_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (ALLIED_HOMEWORLD_HELLO_1);
					break;
				case 1:
					NPCPhrase (ALLIED_HOMEWORLD_HELLO_2);
					break;
				case 2:
					NPCPhrase (ALLIED_HOMEWORLD_HELLO_3);
					break;
				case 3:
					NPCPhrase (ALLIED_HOMEWORLD_HELLO_4);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (ORZ_HOME_VISITS, NumVisits);
		}
		else
		{
			NumVisits = GET_GAME_STATE (ORZ_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (ALLIED_SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase (ALLIED_SPACE_HELLO_2);
					break;
				case 2:
					NPCPhrase (ALLIED_SPACE_HELLO_3);
					break;
				case 3:
					NPCPhrase (ALLIED_SPACE_HELLO_4);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (ORZ_VISITS, NumVisits);
		}

		OrzAllied ((RESPONSE_REF)0);
	}
	else if (Manner != 1)
	{
		if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = GET_GAME_STATE (ORZ_HOME_VISITS);
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
			SET_GAME_STATE (ORZ_HOME_VISITS, NumVisits);
		}
		else
		{
			NumVisits = GET_GAME_STATE (ORZ_VISITS);
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
			SET_GAME_STATE (ORZ_VISITS, NumVisits);
		}

		OrzNeutral ((RESPONSE_REF)0);
	}
	else
	{
		if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) & (1 << 7))
		{
			NumVisits = GET_GAME_STATE (ORZ_HOME_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (ANGRY_HOMEWORLD_HELLO_1);
					break;
				case 1:
					NPCPhrase (ANGRY_HOMEWORLD_HELLO_2);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (ORZ_HOME_VISITS, NumVisits);
		}
		else
		{
			NumVisits = GET_GAME_STATE (ORZ_VISITS);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (ANGRY_SPACE_HELLO_1);
					break;
				case 1:
					NPCPhrase (ANGRY_SPACE_HELLO_2);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (ORZ_VISITS, NumVisits);
		}

		OrzAngry ((RESPONSE_REF)0);
	}

	if (!GET_GAME_STATE (MET_ORZ_BEFORE))
	{
		SET_GAME_STATE (MET_ORZ_BEFORE, 1);

		// Disable talking anim and run the computer report
		EnableTalkingAnim (FALSE);
		AlienTalkSegue (1);
		// Run whatever is left with talking anim
		EnableTalkingAnim (TRUE);
	}
}

static COUNT
uninit_orz (void)
{
	luaUqm_comm_uninit();
	return (0);
}

static void
post_orz_enc (void)
{
	BYTE Manner;

	if (getSegue () == Segue_hostile
			&& (Manner = GET_GAME_STATE (ORZ_MANNER)) != 2)
	{
		SET_GAME_STATE (ORZ_MANNER, 1);
		if (Manner != 1)
		{
			SET_GAME_STATE (ORZ_VISITS, 0);
			SET_GAME_STATE (ORZ_HOME_VISITS, 0);
			SET_GAME_STATE (TAALO_VISITS, 0);
		}
	}
}

LOCDATA*
init_orz_comm (void)
{
	LOCDATA *retval;

	orz_desc.init_encounter_func = Intro;
	orz_desc.post_encounter_func = post_orz_enc;
	orz_desc.uninit_encounter_func = uninit_orz;

	luaUqm_comm_init(NULL, NULL_RESOURCE);
			// Initialise Lua for string interpolation. This will be
			// generalised in the future.

	orz_desc.AlienTextBaseline.x = TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	orz_desc.AlienTextBaseline.y = 0;
	orz_desc.AlienTextWidth = SIS_TEXT_WIDTH - 16;

	if (GET_GAME_STATE (ORZ_MANNER) == 3
			|| LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE)
	{
		setSegue (Segue_peace);
	}
	else
	{
		setSegue (Segue_hostile);
	}
	retval = &orz_desc;

	return (retval);
}

