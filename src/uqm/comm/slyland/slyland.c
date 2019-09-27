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
#include <stdlib.h>
#include "resinst.h"
#include "strings.h"

#include "options.h"
#include "uqm/battle.h"
#include "uqm/setup.h"


static const NUMBER_SPEECH_DESC probe_numbers_english;

static LOCDATA slylandro_desc =
{
	SLYLANDRO_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	SLYLAND_PMAP_ANIM, /* AlienFrame */
	SLYLAND_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	SLYLAND_COLOR_MAP, /* AlienColorMap */
	SLYLAND_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	SLYLAND_CONVERSATION_PHRASES, /* PlayerPhrases */
	6, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			1, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 3, /* RestartRate */
			(1 << 3) /* BlockMask */
		},
		{
			10, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 3, /* RestartRate */
			(1 << 4) /* BlockMask */
		},
		{
			18, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 3, /* RestartRate */
			(1 << 5) /* BlockMask */
		},
		{
			26, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 3, /* RestartRate */
			(1 << 0) /* BlockMask */
		},
		{
			34, /* StartIndex */
			8, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 3, /* RestartRate */
			(1 << 1) /* BlockMask */
		},
		{
			42, /* StartIndex */
			9, /* NumFrames */
			CIRCULAR_ANIM, /* AnimFlags */
			ONE_SECOND / 40, 0, /* FrameRate */
			0, ONE_SECOND * 3, /* RestartRate */
			(1 << 2) /* BlockMask */
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
	{ /* AlienTalkDesc - empty */
		0, /* StartIndex */
		0, /* NumFrames */
		0, /* AnimFlags */
		0, 0, /* FrameRate */
		0, 0, /* RestartRate */
		0, /* BlockMask */
	},
	&probe_numbers_english, /* AlienNumberSpeech - default */
	/* Filler for loaded resources */
	NULL, NULL, NULL,
	NULL,
	NULL,
};

static COUNT probe_digit_names[] =
{
	ENUMERATE_ZERO,
	ENUMERATE_ONE,
	ENUMERATE_TWO,
	ENUMERATE_THREE,
	ENUMERATE_FOUR,
	ENUMERATE_FIVE,
	ENUMERATE_SIX,
	ENUMERATE_SEVEN,
	ENUMERATE_EIGHT,
	ENUMERATE_NINE,
};

static COUNT probe_teen_names[] =
{
	ENUMERATE_TEN,
	ENUMERATE_ELEVEN,
	ENUMERATE_TWELVE,
	ENUMERATE_THIRTEEN,
	ENUMERATE_FOURTEEN,
	ENUMERATE_FIFTEEN,
	ENUMERATE_SIXTEEN,
	ENUMERATE_SEVENTEEN,
	ENUMERATE_EIGHTEEN,
	ENUMERATE_NINETEEN,
};

static COUNT probe_tens_names[] =
{
	0, /* invalid */
	0, /* skip digit */
	ENUMERATE_TWENTY,
	ENUMERATE_THIRTY,
	ENUMERATE_FOURTY,
	ENUMERATE_FIFTY,
	ENUMERATE_SIXTY,
	ENUMERATE_SEVENTY,
	ENUMERATE_EIGHTY,
	ENUMERATE_NINETY,
};

static const NUMBER_SPEECH_DESC probe_numbers_english =
{
	5, /* NumDigits */
	{
		{ /* 1000-999999 */
			1000, /* Divider */
			0, /* Subtrahend */
			NULL, /* StrDigits - recurse */
			NULL, /* Names - not used */
			ENUMERATE_THOUSAND /* CommonIndex */
		},
		{ /* 100-999 */
			100, /* Divider */
			0, /* Subtrahend */
			probe_digit_names, /* StrDigits */
			NULL, /* Names - not used */
			ENUMERATE_HUNDRED /* CommonIndex */
		},
		{ /* 20-99 */
			10, /* Divider */
			0, /* Subtrahend */
			probe_tens_names, /* StrDigits */
			NULL, /* Names - not used */
			0 /* CommonIndex - not used */
		},
		{ /* 10-19 */
			1, /* Divider */
			10, /* Subtrahend */
			probe_teen_names, /* StrDigits */
			NULL, /* Names - not used */
			0 /* CommonIndex - not used */
		},
		{ /* 0-9 */
			1, /* Divider */
			0, /* Subtrahend */
			probe_digit_names, /* StrDigits */
			NULL, /* Names - not used */
			0 /* CommonIndex - not used */
		}
	}
};

static RESPONSE_REF threat,
			something_wrong,
			we_are_us,
			why_attack,
			bye;

static void
sayCoord (int coord)
{
	int ac;
	
	ac = abs (coord);

	NPCPhrase_splice (coord < 0 ? COORD_MINUS : COORD_PLUS);
	NPCNumber (ac / 10, "%03d");
	NPCPhrase_splice (COORD_POINT);
	NPCNumber (ac % 10, NULL);
}

static void
CombatIsInevitable (RESPONSE_REF R)
{
	if (R == 0)
	{
		if (GET_GAME_STATE (DESTRUCT_CODE_ON_SHIP))
			Response (destruct_code, CombatIsInevitable);
		switch (GET_GAME_STATE (SLYLANDRO_PROBE_THREAT))
		{
			case 0:
				threat = threat_1;
				break;
			case 1:
				threat = threat_2;
				break;
			case 2:
				threat = threat_3;
				break;
			default:
				threat = threat_4;
				break;
		}
		Response (threat, CombatIsInevitable);
		switch (GET_GAME_STATE (SLYLANDRO_PROBE_WRONG))
		{
			case 0:
				something_wrong = something_wrong_1;
				break;
			case 1:
				something_wrong = something_wrong_2;
				break;
			case 2:
				something_wrong = something_wrong_3;
				break;
			default:
				something_wrong = something_wrong_4;
				break;
		}
		Response (something_wrong, CombatIsInevitable);
		switch (GET_GAME_STATE (SLYLANDRO_PROBE_ID))
		{
			case 0:
				we_are_us = we_are_us_1;
				break;
			case 1:
				we_are_us = we_are_us_2;
				break;
			case 2:
				we_are_us = we_are_us_3;
				break;
			default:
				we_are_us = we_are_us_4;
				break;
		}
		Response (we_are_us, CombatIsInevitable);
		switch (GET_GAME_STATE (SLYLANDRO_PROBE_INFO))
		{
			case 0:
				why_attack = why_attack_1;
				break;
			case 1:
				why_attack = why_attack_2;
				break;
			case 2:
				why_attack = why_attack_3;
				break;
			default:
				why_attack = why_attack_4;
				break;
		}
		Response (why_attack, CombatIsInevitable);
		switch (GET_GAME_STATE (SLYLANDRO_PROBE_EXIT))
		{
			case 0:
				bye = bye_1;
				break;
			case 1:
				bye = bye_2;
				break;
			case 2:
				bye = bye_3;
				break;
			default:
				bye = bye_4;
				break;
		}
		Response (bye, CombatIsInevitable);
	}
	else if (PLAYER_SAID (R, destruct_code))
	{
		NPCPhrase (DESTRUCT_SEQUENCE);
		setSegue (Segue_victory);
	}
	else
	{
		BYTE NumVisits;

		if (PLAYER_SAID (R, threat))
		{
			NumVisits = GET_GAME_STATE (SLYLANDRO_PROBE_THREAT);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (PROGRAMMED_TO_DEFEND_1);
					break;
				case 1:
					NPCPhrase (PROGRAMMED_TO_DEFEND_2);
					break;
				case 2:
					NPCPhrase (PROGRAMMED_TO_DEFEND_3);
					break;
				case 3:
					NPCPhrase (PROGRAMMED_TO_DEFEND_4);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (SLYLANDRO_PROBE_THREAT, NumVisits);
		}
		else if (PLAYER_SAID (R, something_wrong))
		{
			NumVisits = GET_GAME_STATE (SLYLANDRO_PROBE_WRONG);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (NOMINAL_FUNCTION_1);
					break;
				case 1:
					NPCPhrase (NOMINAL_FUNCTION_2);
					break;
				case 2:
					NPCPhrase (NOMINAL_FUNCTION_3);
					break;
				case 3:
					NPCPhrase (NOMINAL_FUNCTION_4);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (SLYLANDRO_PROBE_WRONG, NumVisits);
		}
		else if (PLAYER_SAID (R, we_are_us))
		{
			NumVisits = GET_GAME_STATE (SLYLANDRO_PROBE_ID);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (THIS_IS_PROBE_1);
					break;
				case 1:
					NPCPhrase (THIS_IS_PROBE_2);
					break;
				case 2:
					NPCPhrase (THIS_IS_PROBE_3);
					break;
				case 3:
				{
					SIZE dx, dy;

					// Probe's coordinate system is {-Y,X} relative to B Corvi
					dx = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x)) - 333;
					dy = 9812 - LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));

					NPCPhrase (THIS_IS_PROBE_40);
					sayCoord (dy);
					NPCPhrase_splice (THIS_IS_PROBE_41);
					sayCoord (dx);
					NPCPhrase (THIS_IS_PROBE_42);

					--NumVisits;
					break;
				}
			}
			SET_GAME_STATE (SLYLANDRO_PROBE_ID, NumVisits);
		}
		else if (PLAYER_SAID (R, why_attack))
		{
			NumVisits = GET_GAME_STATE (SLYLANDRO_PROBE_INFO);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (PEACEFUL_MISSION_1);
					break;
				case 1:
					NPCPhrase (PEACEFUL_MISSION_2);
					break;
				case 2:
					NPCPhrase (PEACEFUL_MISSION_3);
					break;
				case 3:
					NPCPhrase (PEACEFUL_MISSION_4);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (SLYLANDRO_PROBE_INFO, NumVisits);
		}
		else if (PLAYER_SAID (R, bye))
		{
			NumVisits = GET_GAME_STATE (SLYLANDRO_PROBE_EXIT);
			switch (NumVisits++)
			{
				case 0:
					NPCPhrase (GOODBYE_1);
					break;
				case 1:
					NPCPhrase (GOODBYE_2);
					break;
				case 2:
					NPCPhrase (GOODBYE_3);
					break;
				case 3:
					NPCPhrase (GOODBYE_4);
					--NumVisits;
					break;
			}
			SET_GAME_STATE (SLYLANDRO_PROBE_EXIT, NumVisits);
		}

		NPCPhrase (HOSTILE);

		SET_GAME_STATE (PROBE_EXHIBITED_BUG, 1);
		setSegue (Segue_hostile);
	}
}

static void
Intro (void)
{
	BYTE  NumVisits;

	NumVisits = GET_GAME_STATE (SLYLANDRO_PROBE_VISITS);
	switch (NumVisits++)
	{
		case 0:
			NPCPhrase (WE_COME_IN_PEACE_1);
			break;
		case 1:
			NPCPhrase (WE_COME_IN_PEACE_2);
			break;
		case 2:
			NPCPhrase (WE_COME_IN_PEACE_3);
			break;
		case 3:
			NPCPhrase (WE_COME_IN_PEACE_4);
			break;
		case 4:
			NPCPhrase (WE_COME_IN_PEACE_5);
			break;
		case 5:
			NPCPhrase (WE_COME_IN_PEACE_6);
			break;
		case 6:
			NPCPhrase (WE_COME_IN_PEACE_7);
			break;
		case 7:
			NPCPhrase (WE_COME_IN_PEACE_8);
			--NumVisits;
			break;
	}
	SET_GAME_STATE (SLYLANDRO_PROBE_VISITS, NumVisits);

	CombatIsInevitable ((RESPONSE_REF)0);
}

static COUNT
uninit_slyland (void)
{
	return (0);
}

static void
post_slyland_enc (void)
{
	// nothing defined so far
}

LOCDATA*
init_slyland_comm (void)
{
	LOCDATA *retval;

	slylandro_desc.init_encounter_func = Intro;
	slylandro_desc.post_encounter_func = post_slyland_enc;
	slylandro_desc.uninit_encounter_func = uninit_slyland;

	slylandro_desc.AlienTextBaseline.x = TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	slylandro_desc.AlienTextBaseline.y = 0;
	slylandro_desc.AlienTextWidth = SIS_TEXT_WIDTH - 16;

	setSegue (Segue_hostile);
	retval = &slylandro_desc;

	return (retval);
}

