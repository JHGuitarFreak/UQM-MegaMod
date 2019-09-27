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
#include "uqm/hyper.h"
			// for SOL_X/SOL_Y
#include "../../nameref.h"
			// JMS_GFX: For LoadGraphic 
#include "../../planets/planets.h"
			// For MIN_MOON_RADIUS


static LOCDATA chmmr_desc =
{
	CHMMR_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	CHMMR_PMAP_ANIM, /* AlienFrame */
	CHMMR_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	CHMMR_COLOR_MAP, /* AlienColorMap */
	CHMMR_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	CHMMR_CONVERSATION_PHRASES, /* PlayerPhrases */
	6, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			12, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			17, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			22, /* StartIndex */
			5, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			27, /* StartIndex */
			20, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			47, /* StartIndex */
			14, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			61, /* StartIndex */
			24, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
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
		ONE_SECOND / 60, 0, /* FrameRate */
		ONE_SECOND / 60, 0, /* RestartRate */
		0, /* BlockMask */
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

	if (PLAYER_SAID (R, bye))
		NPCPhrase (GOODBYE);
	else if (PLAYER_SAID (R, bye_shielded))
		NPCPhrase (GOODBYE_SHIELDED);
	else if (PLAYER_SAID (R, bye_after_bomb))
		NPCPhrase (GOODBYE_AFTER_BOMB);
	else if (PLAYER_SAID (R, proceed))
	{
		int i;

		NPCPhrase (TAKE_2_WEEKS);

		SetRaceAllied (CHMMR_SHIP, TRUE);

		SET_GAME_STATE (CHMMR_HOME_VISITS, 0);
		SET_GAME_STATE (CHMMR_STACK, 0);
		SET_GAME_STATE (CHMMR_BOMB_STATE, 2);
		SET_GAME_STATE (UTWIG_BOMB_ON_SHIP, 0);

		if(!optInfiniteRU)
			GLOBAL_SIS (ResUnits) = 1000000L;
		else
			oldRU = 1000000L;

		GLOBAL_SIS (NumLanders) = 0;
		GLOBAL (ModuleCost[PLANET_LANDER]) = 0;

#define EARTH_INDEX 2 /* earth is 3rd planet --> 3 - 1 = 2 */

		/* transport player to Earth */
		GLOBAL_SIS (log_x) = UNIVERSE_TO_LOGX (SOL_X);
		GLOBAL_SIS (log_y) = UNIVERSE_TO_LOGY (SOL_Y);
		GLOBAL (ShipFacing) = 1;
		/* At Earth or at Starbase */
		GLOBAL (ip_planet) = EARTH_INDEX + 1;
		GLOBAL (in_orbit) = 0;
		/* XXX : this should be unhardcoded eventually */
		GLOBAL (ip_location.x) = EARTH_OUTER_X;
		GLOBAL (ip_location.y) = EARTH_OUTER_Y;

		if (GET_GAME_STATE (STARBASE_AVAILABLE))
		{	/* Normal game mode - you are transported to Starbase */
			GLOBAL_SIS (FuelOnBoard) = FUEL_RESERVE;
			GLOBAL_SIS (CrewEnlisted) = 0;
			GLOBAL_SIS (TotalElementMass) = 0;
			GLOBAL (ModuleCost[STORAGE_BAY]) = 0; /* disable Storage Bay */
			for (i = 0; i < NUM_ELEMENT_CATEGORIES; ++i)
				GLOBAL_SIS (ElementAmounts[i]) = 0;
			for (i = NUM_BOMB_MODULES; i < NUM_MODULE_SLOTS; ++i)
				GLOBAL_SIS (ModuleSlots[i]) = EMPTY_SLOT + 2;

			/* XXX : this should be unhardcoded eventually */
			/* transport to Starbase */
			/* note: if optOrbitingPlanets is enabled, this will be corrected in DoTimePassage */
			GLOBAL (ShipStamp.origin.x) = (SIS_SCREEN_WIDTH >> 1) + COSINE(HALF_CIRCLE + QUADRANT, MIN_MOON_RADIUS);
			GLOBAL (ShipStamp.origin.y) = (SIS_SCREEN_HEIGHT >> 1) + SINE(HALF_CIRCLE + QUADRANT, MIN_MOON_RADIUS >> 1);
		}
		else
		{	/* 'Beating Game Differently' mode - never visited Starbase,
			 * so you are transported to Earth */
			/* compress the layout -- move all to front */
			for (i = NUM_MODULE_SLOTS - 1; i > 0; --i)
			{
				int m;

				/* find next unused slot */
				for (; i > 0
						&& GLOBAL_SIS (ModuleSlots[i]) != EMPTY_SLOT + 2;
						--i)
					;
				if (i == 0)
					break;
				/* find next module to move */
				for (m = i - 1; m >= 0
						&& GLOBAL_SIS (ModuleSlots[m]) == EMPTY_SLOT + 2;
						--m)
					;
				if (m < 0)
					break;
								
				/* move the module */
				GLOBAL_SIS (ModuleSlots[i]) = GLOBAL_SIS (ModuleSlots[m]);
				GLOBAL_SIS (ModuleSlots[m]) = EMPTY_SLOT + 2;
			}
		}

		/* install Chmmr-supplied modules */
		for (i = 0; i < NUM_DRIVE_SLOTS; ++i)
			GLOBAL_SIS (DriveSlots[i]) = FUSION_THRUSTER;
		for (i = 0; i < NUM_JET_SLOTS; ++i)
			GLOBAL_SIS (JetSlots[i]) = TURNING_JETS;
		GLOBAL_SIS (ModuleSlots[0]) = BOMB_MODULE_4;
		GLOBAL_SIS (ModuleSlots[1]) = BOMB_MODULE_5;
		GLOBAL_SIS (ModuleSlots[2]) = BOMB_MODULE_3;
		GLOBAL_SIS (ModuleSlots[3]) = BOMB_MODULE_1;
		GLOBAL_SIS (ModuleSlots[4]) = BOMB_MODULE_0;
		GLOBAL_SIS (ModuleSlots[5]) = BOMB_MODULE_1;
		GLOBAL_SIS (ModuleSlots[6]) = BOMB_MODULE_3;
		GLOBAL_SIS (ModuleSlots[7]) = BOMB_MODULE_4;
		GLOBAL_SIS (ModuleSlots[8]) = BOMB_MODULE_5;
		GLOBAL_SIS (ModuleSlots[9]) = BOMB_MODULE_2;
	}
}

static void
NotReady (RESPONSE_REF R)
{
	if (R == 0)
		NPCPhrase (RETURN_WHEN_READY);
	else if (PLAYER_SAID (R, further_assistance))
	{
		NPCPhrase (NO_FURTHER_ASSISTANCE);

		DISABLE_PHRASE (further_assistance);
	}
	else if (PLAYER_SAID (R, tech_help))
	{
		NPCPhrase (USE_OUR_SHIPS_BEFORE);

		SetRaceAllied (CHMMR_SHIP, TRUE);
	}
	else if (PLAYER_SAID (R, where_weapon))
	{
		NPCPhrase (PRECURSOR_WEAPON);

		DISABLE_PHRASE (where_weapon);
	}
	else if (PLAYER_SAID (R, where_distraction))
	{
		NPCPhrase (PSYCHIC_WEAPONRY);

		DISABLE_PHRASE (where_distraction);
	}

	if (CheckAlliance (CHMMR_SHIP) != GOOD_GUY)
		Response (tech_help, NotReady);
	else if (PHRASE_ENABLED (further_assistance))
		Response (further_assistance, NotReady);
	if (PHRASE_ENABLED (where_weapon) && !GET_GAME_STATE (UTWIG_BOMB_ON_SHIP))
		Response (where_weapon, NotReady);
	if (PHRASE_ENABLED (where_distraction) && !GET_GAME_STATE (TALKING_PET_ON_SHIP))
		Response (where_distraction, NotReady);
	Response (bye, ExitConversation);
}

static void
ImproveBomb (RESPONSE_REF R)
{
	if (R == 0)
		NPCPhrase (WE_WILL_IMPROVE_BOMB);
	else if (PLAYER_SAID (R, what_now))
	{
		NPCPhrase (MODIFY_VESSEL);

		DISABLE_PHRASE (what_now);
	}
	else if (PLAYER_SAID (R, wont_hurt_my_ship))
	{
		NPCPhrase (WILL_DESTROY_IT);

		DISABLE_PHRASE (wont_hurt_my_ship);
	}
	else if (PLAYER_SAID (R, bummer_about_my_ship))
	{
		NPCPhrase (DEAD_SILENCE);

		DISABLE_PHRASE (bummer_about_my_ship);
	}
	else if (PLAYER_SAID (R, other_assistance))
	{
		NPCPhrase (USE_OUR_SHIPS_AFTER);

		SetRaceAllied (CHMMR_SHIP, TRUE);
	}

	if (PHRASE_ENABLED (what_now))
		Response (what_now, ImproveBomb);
	else if (PHRASE_ENABLED (wont_hurt_my_ship))
		Response (wont_hurt_my_ship, ImproveBomb);
	else if (PHRASE_ENABLED (bummer_about_my_ship))
		Response (bummer_about_my_ship, ImproveBomb);
	if (CheckAlliance (CHMMR_SHIP) != GOOD_GUY)
		Response (other_assistance, ImproveBomb);
	Response (proceed, ExitConversation);
}

static void
ChmmrFree (RESPONSE_REF R)
{
	if (R == 0
			|| PLAYER_SAID (R, i_am_captain)
			|| PLAYER_SAID (R, i_am_savior)
			|| PLAYER_SAID (R, i_am_silly))
	{
		NPCPhrase (WHY_HAVE_YOU_FREED_US);
		AlienTalkSegue ((COUNT)~0);
		SET_GAME_STATE (CHMMR_EMERGING, 0);

		Response (serious_1, ChmmrFree);
		Response (serious_2, ChmmrFree);
		Response (silly, ChmmrFree);
	}
	else
	{
		NPCPhrase (WILL_HELP_ANALYZE_LOGS);

		if (GET_GAME_STATE (AWARE_OF_SAMATRA))
			NPCPhrase (YOU_KNOW_SAMATRA);
		else
		{
			NPCPhrase (DONT_KNOW_ABOUT_SAMATRA);

			SET_GAME_STATE (AWARE_OF_SAMATRA, 1);
		}

		if (GET_GAME_STATE (TALKING_PET_ON_SHIP))
			NPCPhrase (HAVE_TALKING_PET);
		else
			NPCPhrase (NEED_DISTRACTION);

		if (GET_GAME_STATE (UTWIG_BOMB_ON_SHIP))
			NPCPhrase (HAVE_BOMB);
		else
			NPCPhrase (NEED_WEAPON);

		if (!GET_GAME_STATE (TALKING_PET_ON_SHIP)
				|| !GET_GAME_STATE (UTWIG_BOMB_ON_SHIP))
			NotReady ((RESPONSE_REF)0);
		else
			ImproveBomb ((RESPONSE_REF)0);
	}
}

static void ChmmrShielded (RESPONSE_REF R);

static void
ChmmrAdvice (RESPONSE_REF R)
{
	BYTE AdviceLeft;

	if (PLAYER_SAID (R, need_advice))
		NPCPhrase (WHAT_ADVICE);
	else if (PLAYER_SAID (R, how_defeat_urquan))
	{
		NPCPhrase (DEFEAT_LIKE_SO);

		SET_GAME_STATE (CHMMR_BOMB_STATE, 1);
		DISABLE_PHRASE (how_defeat_urquan);
	}
	else if (PLAYER_SAID (R, what_about_tpet))
	{
		NPCPhrase (SCARY_BUT_USEFUL);

		DISABLE_PHRASE (what_about_tpet);
	}
	else if (PLAYER_SAID (R, what_about_bomb))
	{
		NPCPhrase (ABOUT_BOMB);

		DISABLE_PHRASE (what_about_bomb);
	}
	else if (PLAYER_SAID (R, what_about_sun_device))
	{
		NPCPhrase (ABOUT_SUN_DEVICE);

		DISABLE_PHRASE (what_about_sun_device);
	}
	else if (PLAYER_SAID (R, what_about_samatra))
	{
		NPCPhrase (ABOUT_SAMATRA);

		DISABLE_PHRASE (what_about_samatra);
	}

	AdviceLeft = 0;
	if (PHRASE_ENABLED (how_defeat_urquan))
	{
		Response (how_defeat_urquan, ChmmrAdvice);
		AdviceLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_tpet) && GET_GAME_STATE (TALKING_PET_ON_SHIP))
	{
		Response (what_about_tpet, ChmmrAdvice);
		AdviceLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_bomb) && GET_GAME_STATE (UTWIG_BOMB_ON_SHIP))
	{
		Response (what_about_bomb, ChmmrAdvice);
		AdviceLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_sun_device) && GET_GAME_STATE (SUN_DEVICE_ON_SHIP))
	{
		Response (what_about_sun_device, ChmmrAdvice);
		AdviceLeft = TRUE;
	}
	if (PHRASE_ENABLED (what_about_samatra) && GET_GAME_STATE (AWARE_OF_SAMATRA))
	{
		Response (what_about_samatra, ChmmrAdvice);
		AdviceLeft = TRUE;
	}
	Response (enough_advice, ChmmrShielded);

	if (!AdviceLeft)
		DISABLE_PHRASE (need_advice);
}

static void
ChmmrShielded (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, find_out_whats_up))
	{
		NPCPhrase (HYBRID_PROCESS);

		DISABLE_PHRASE (find_out_whats_up);
	}
	else if (PLAYER_SAID (R, need_help))
	{
		NPCPhrase (CANT_HELP);

		SET_GAME_STATE (CHMMR_STACK, 1);
	}
	else if (PLAYER_SAID (R, why_no_help))
	{
		NPCPhrase (LONG_TIME);

		SET_GAME_STATE (CHMMR_STACK, 2);
	}
	else if (PLAYER_SAID (R, what_if_more_energy))
	{
		NPCPhrase (DANGER_TO_US);

		SET_GAME_STATE (CHMMR_STACK, 3);
	}
	else if (PLAYER_SAID (R, enough_advice))
		NPCPhrase (OK_ENOUGH_ADVICE);

	switch (GET_GAME_STATE (CHMMR_STACK))
	{
		case 0:
			Response (need_help, ChmmrShielded);
			break;
		case 1:
			Response (why_no_help, ChmmrShielded);
			break;
		case 2:
			Response (what_if_more_energy, ChmmrShielded);
			break;
	}
	if (PHRASE_ENABLED (find_out_whats_up))
		Response (find_out_whats_up, ChmmrShielded);
	if (PHRASE_ENABLED (need_advice))
	{
		Response (need_advice, ChmmrAdvice);
	}
	Response (bye_shielded, ExitConversation);
}

static void
AfterBomb (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, whats_up_after_bomb))
	{
		if (GET_GAME_STATE (CHMMR_STACK))
			NPCPhrase (GENERAL_INFO_AFTER_BOMB_2);
		else
		{
			NPCPhrase (GENERAL_INFO_AFTER_BOMB_1);

			SET_GAME_STATE (CHMMR_STACK, 1);
		}

		DISABLE_PHRASE (whats_up_after_bomb);
	}
	else if (PLAYER_SAID (R, what_do_after_bomb))
	{
		NPCPhrase (DO_AFTER_BOMB);

		DISABLE_PHRASE (what_do_after_bomb);
	}

	if (PHRASE_ENABLED (whats_up_after_bomb))
		Response (whats_up_after_bomb, AfterBomb);
	if (PHRASE_ENABLED (what_do_after_bomb))
		Response (what_do_after_bomb, AfterBomb);
	Response (bye_after_bomb, ExitConversation);
}

static void
Intro (void)
{
	BYTE NumVisits;

	if (GET_GAME_STATE (CHMMR_BOMB_STATE) >= 2)
	{
		NumVisits = GET_GAME_STATE (CHMMR_HOME_VISITS);
		switch (NumVisits++)
		{
			case 0:
				NPCPhrase (HELLO_AFTER_BOMB_1);
				break;
			case 1:
				NPCPhrase (HELLO_AFTER_BOMB_2);
				--NumVisits;
				break;
		}
		SET_GAME_STATE (CHMMR_HOME_VISITS, NumVisits);

		AfterBomb ((RESPONSE_REF)0);
	}
	else if (GET_GAME_STATE (CHMMR_UNLEASHED))
	{
		if (!GET_GAME_STATE (TALKING_PET_ON_SHIP)
				|| !GET_GAME_STATE (UTWIG_BOMB_ON_SHIP))
			NotReady ((RESPONSE_REF)0);
		else
		{
			NPCPhrase (YOU_ARE_READY);

			ImproveBomb ((RESPONSE_REF)0);
		}
	}
	else
	{
		NumVisits = GET_GAME_STATE (CHMMR_HOME_VISITS);
		if (!GET_GAME_STATE (CHMMR_EMERGING))
		{
			CommData.AlienColorMap = SetAbsColorMapIndex (
					CommData.AlienColorMap, 1
					);

			// JMS_GFX: Use separate graphics in hires instead of colormap transform.
			if (IS_HD)
			{
				CommData.AlienFrameRes = CHMMR_RED_PMAP_ANIM;
				CommData.AlienFrame = CaptureDrawable (
					LoadGraphic (CommData.AlienFrameRes));
			}

			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (WHY_YOU_HERE_1);
					break;
				case 1:
					NPCPhrase (WHY_YOU_HERE_2);
					break;
				case 2:
					NPCPhrase (WHY_YOU_HERE_3);
					break;
				case 3:
					NPCPhrase (WHY_YOU_HERE_4);
					--NumVisits;
					break;
			}

			ChmmrShielded ((RESPONSE_REF)0);
		}
		else
		{
			HFLEETINFO hChmmr = GetStarShipFromIndex (&GLOBAL (avail_race_q), CHMMR_SHIP);
			FLEET_INFO *ChmmrPtr = LockFleetInfo (&GLOBAL (avail_race_q), hChmmr);

			SetCommIntroMode (CIM_FADE_IN_SCREEN, ONE_SECOND * 2);
			NPCPhrase (WE_ARE_FREE);

			if (NumVisits)
			{
				ChmmrFree ((RESPONSE_REF)0);
				NumVisits = 0;
			}
			else
			{
				NPCPhrase (WHO_ARE_YOU);

				Response (i_am_captain, ChmmrFree);
				Response (i_am_savior, ChmmrFree);
				Response (i_am_silly, ChmmrFree);
			}

			SET_GAME_STATE (CHMMR_UNLEASHED, 1);

			// Setup a Chmmr sphere-of-influence, now that they're out
			// of their shell.  EncounterPercent for the Chmmr is 0,
			// so this is purely decorative.
			if (EXTENDED) {
				if (ChmmrPtr) {
					ChmmrPtr->actual_strength = 1800 / SPHERE_RADIUS_INCREMENT * 2;
					ChmmrPtr->loc.x = 742;
					ChmmrPtr->loc.y = 2268;
					StartSphereTracking(CHMMR_SHIP);
				}
				UnlockFleetInfo(&GLOBAL(avail_race_q), hChmmr);
			}
		}
		SET_GAME_STATE (CHMMR_HOME_VISITS, NumVisits);
	}
}

static COUNT
uninit_chmmr (void)
{
	luaUqm_comm_uninit ();
	return (0);
}

static void
post_chmmr_enc (void)
{
	// nothing defined so far
}

LOCDATA*
init_chmmr_comm (void)
{
	LOCDATA *retval;

	chmmr_desc.init_encounter_func = Intro;
	chmmr_desc.post_encounter_func = post_chmmr_enc;
	chmmr_desc.uninit_encounter_func = uninit_chmmr;

	luaUqm_comm_init (NULL, NULL_RESOURCE);
			// Initialise Lua for string interpolation. This will be
			// generalised in the future.

	if (GET_GAME_STATE(CHMMR_UNLEASHED) || GET_GAME_STATE(CHMMR_EMERGING)) {
		// use alternate "Process" track if available
		chmmr_desc.AlienAltSongRes = CHMMR_PROCESS_MUSIC;
		chmmr_desc.AlienSongFlags |= LDASF_USE_ALTERNATE;
	} else {
		// regular track -- let's make sure
		chmmr_desc.AlienSongFlags &= ~LDASF_USE_ALTERNATE;
	}

	chmmr_desc.AlienTextBaseline.x = TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	chmmr_desc.AlienTextBaseline.y = 0;
	chmmr_desc.AlienTextWidth = SIS_TEXT_WIDTH - 16;

	setSegue (Segue_peace);
	retval = &chmmr_desc;

	return (retval);
}
