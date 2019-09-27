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
#include "uqm/shipcont.h"
#include "libs/inplib.h"
#include "libs/mathlib.h"
#include "libs/log.h"
#include "uqm/lua/luacomm.h"
#include "uqm/hyper.h"
			// for SOL_X/SOL_Y
#include "uqm/planets/planets.h"
		// for xxx_DISASTER
#include "uqm/sis.h"
#include "../../gendef.h"
#include "../../starmap.h"


static const NUMBER_SPEECH_DESC melnorme_numbers_english;

static LOCDATA melnorme_desc_orig =
{
	MELNORME_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	MELNORME_PMAP_ANIM, /* AlienFrame */
	MELNORME_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	MELNORME_COLOR_MAP, /* AlienColorMap */
	MELNORME_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	MELNORME_CONVERSATION_PHRASES, /* PlayerPhrases */
	4, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			6, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 12, 0, /* FrameRate */
			ONE_SECOND * 4, ONE_SECOND * 4,/* RestartRate */
			(1 << 1), /* BlockMask */
		},
		{
			11, /* StartIndex */
			9, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND * 4, ONE_SECOND * 4,/* RestartRate */
			(1 << 0), /* BlockMask */
		},
		{
			20, /* StartIndex */
			2, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			22, /* StartIndex */
			2, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 15, /* FrameRate */
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
		5, /* NumFrames */
		0, /* AnimFlags */
		ONE_SECOND / 15, 0, /* FrameRate */
		ONE_SECOND / 12, 0, /* RestartRate */
		0, /* BlockMask */
	},
	&melnorme_numbers_english, /* AlienNumberSpeech - default */
	/* Filler for loaded resources */
	NULL, NULL, NULL,
	NULL,
	NULL,
};

static LOCDATA melnorme_desc_hd =
{
	MELNORME_CONVERSATION, /* AlienConv */
	NULL, /* init_encounter_func */
	NULL, /* post_encounter_func */
	NULL, /* uninit_encounter_func */
	MELNORME_PMAP_ANIM, /* AlienFrame */
	MELNORME_FONT, /* AlienFont */
	WHITE_COLOR_INIT, /* AlienTextFColor */
	BLACK_COLOR_INIT, /* AlienTextBColor */
	{0, 0}, /* AlienTextBaseline */
	0, /* SIS_TEXT_WIDTH - 16, */ /* AlienTextWidth */
	ALIGN_CENTER, /* AlienTextAlign */
	VALIGN_TOP, /* AlienTextValign */
	MELNORME_COLOR_MAP, /* AlienColorMap */
	MELNORME_MUSIC, /* AlienSong */
	NULL_RESOURCE, /* AlienAltSong */
	0, /* AlienSongFlags */
	MELNORME_CONVERSATION_PHRASES, /* PlayerPhrases */
	11, /* NumAnimations */
	{ /* AlienAmbientArray (ambient animations) */
		{
			6, /* StartIndex */
			5, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 12, 0, /* FrameRate */
			ONE_SECOND * 4, ONE_SECOND * 4,/* RestartRate */
			(1 << 1), /* BlockMask */
		},
		{
			11, /* StartIndex */
			9, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 20, 0, /* FrameRate */
			ONE_SECOND * 4, ONE_SECOND * 4,/* RestartRate */
			(1 << 0), /* BlockMask */
		},
		{
			20, /* StartIndex */
			2, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 10), /* BlockMask */
		},
		{
			22, /* StartIndex */
			2, /* NumFrames */
			YOYO_ANIM, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			(1 << 10), /* BlockMask */
		},
		{
			24, /* StartIndex */
			11, /* NumFrames */
			CIRCULAR_ANIM | ONE_SHOT_ANIM | ANIM_DISABLED, /* AnimFlags */
			ONE_SECOND / 11, 0, /* FrameRate */
			0, 0, /* RestartRate */
			0, /* BlockMask */
		},
		{
			35, /* StartIndex */
			2, /* NumFrames */
			YOYO_ANIM | ANIM_DISABLED, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			37, /* StartIndex */
			2, /* NumFrames */
			YOYO_ANIM | ANIM_DISABLED, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			39, /* StartIndex */
			10, /* NumFrames */
			CIRCULAR_ANIM | ONE_SHOT_ANIM | ANIM_DISABLED, /* AnimFlags */
			ONE_SECOND / 10, 0, /* FrameRate */
			0, 0, /* RestartRate */
			0, /* BlockMask */
		},
		{
			49, /* StartIndex */
			2, /* NumFrames */
			YOYO_ANIM | ANIM_DISABLED, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			51, /* StartIndex */
			2, /* NumFrames */
			YOYO_ANIM | ANIM_DISABLED, /* AnimFlags */
			ONE_SECOND / 10, ONE_SECOND / 15, /* FrameRate */
			ONE_SECOND, ONE_SECOND * 3, /* RestartRate */
			0, /* BlockMask */
		},
		{
			53, /* StartIndex */
			11, /* NumFrames */
			CIRCULAR_ANIM | ONE_SHOT_ANIM | ANIM_DISABLED, /* AnimFlags */
			ONE_SECOND / 11, 0, /* FrameRate */
			0, 0, /* RestartRate */
			(1 << 2) | (1 << 3) /* BlockMask */
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
		5, /* NumFrames */
		0, /* AnimFlags */
		ONE_SECOND / 15, 0, /* FrameRate */
		ONE_SECOND / 12, 0, /* RestartRate */
		0, /* BlockMask */
	},
	&melnorme_numbers_english, /* AlienNumberSpeech - default */
	/* Filler for loaded resources */
	NULL, NULL, NULL,
	NULL,
	NULL,
};

static COUNT melnorme_digit_names[] =
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
	ENUMERATE_NINE
};

static COUNT melnorme_teen_names[] =
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
	ENUMERATE_NINETEEN
};

static COUNT melnorme_tens_names[] =
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
	ENUMERATE_NINETY
};

static const NUMBER_SPEECH_DESC melnorme_numbers_english =
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
			melnorme_digit_names, /* StrDigits */
			NULL, /* Names - not used */
			ENUMERATE_HUNDRED /* CommonIndex */
		},
		{ /* 20-99 */
			10, /* Divider */
			0, /* Subtrahend */
			melnorme_tens_names, /* StrDigits */
			NULL, /* Names - not used */
			0 /* CommonIndex - not used */
		},
		{ /* 10-19 */
			1, /* Divider */
			10, /* Subtrahend */
			melnorme_teen_names, /* StrDigits */
			NULL, /* Names - not used */
			0 /* CommonIndex - not used */
		},
		{ /* 0-9 */
			1, /* Divider */
			0, /* Subtrahend */
			melnorme_digit_names, /* StrDigits */
			NULL, /* Names - not used */
			0 /* CommonIndex - not used */
		}
	}
};

#define ARRAY_SIZE(array) (sizeof(array) / sizeof (*array))


//////////////Technology System///////////////////////
// This section deals with enabling and checking for
// various technologies.  It should probably be
// migrated to its own file.

// Identifiers for the various technologies
typedef enum 
{
	TECH_MODULE_BLASTER,
	TECH_LANDER_SPEED,
	TECH_MODULE_ANTIMISSILE,
	TECH_LANDER_SHIELD_BIO,
	TECH_LANDER_CARGO,
	TECH_MODULE_BIGFUELTANK,
	TECH_LANDER_RAPIDFIRE,
	TECH_LANDER_SHIELD_QUAKE,
	TECH_MODULE_TRACKING,
	TECH_LANDER_SHIELD_LIGHTNING,
	TECH_LANDER_SHIELD_HEAT,
	TECH_MODULE_CANNON,
	TECH_MODULE_FURNACE,
} TechId_t;

// Group the technologies into three subtypes
typedef enum
{
	TECH_TYPE_MODULE,         // Flagship modules
	                          //  subtype = moduleId, info = cost
                              //   Cost will be scaled by MODULE_COST_SCALE.
	TECH_TYPE_LANDER_SHIELD,  // Lander shield enhancements
                              //  subtype = disaster type, info = unused
	TECH_TYPE_STATE           // Other game state changes
	                          //  subtype = stateId, info = state value
} TechType_t;


// Define the information specifying a particular technology
typedef struct
{
	TechId_t   id;      // ID of the technology
	TechType_t type;    // Type of the technology
	int        subtype; // Subtype of the technology
	int        info;    // Supplemental information
} TechData;


// A table of the available technologies.
// This should really be an associative map of TechIds to tech data records,
// but implementing that would be excessive.
static const TechData tech_data_table[] = 
{
	// Tech ID                      Tech Type,               Supplemental info
	{ TECH_MODULE_BLASTER,          TECH_TYPE_MODULE,        BLASTER_WEAPON,        4000 },
	{ TECH_LANDER_SPEED,            TECH_TYPE_STATE,         IMPROVED_LANDER_SPEED,    1 },
	{ TECH_MODULE_ANTIMISSILE,      TECH_TYPE_MODULE,        ANTIMISSILE_DEFENSE,   4000 },
	{ TECH_LANDER_SHIELD_BIO,       TECH_TYPE_LANDER_SHIELD, BIOLOGICAL_DISASTER,     -1 },
	{ TECH_LANDER_CARGO,            TECH_TYPE_STATE,         IMPROVED_LANDER_CARGO,    1 },
	{ TECH_MODULE_BIGFUELTANK,      TECH_TYPE_MODULE,        HIGHEFF_FUELSYS,       1000 },
	{ TECH_LANDER_RAPIDFIRE,        TECH_TYPE_STATE,         IMPROVED_LANDER_SHOT,     1 },
	{ TECH_LANDER_SHIELD_QUAKE,     TECH_TYPE_LANDER_SHIELD, EARTHQUAKE_DISASTER,     -1 },
	{ TECH_MODULE_TRACKING,         TECH_TYPE_MODULE,        TRACKING_SYSTEM,       5000 },
	{ TECH_LANDER_SHIELD_LIGHTNING, TECH_TYPE_LANDER_SHIELD, LIGHTNING_DISASTER,      -1 },
	{ TECH_LANDER_SHIELD_HEAT,      TECH_TYPE_LANDER_SHIELD, LAVASPOT_DISASTER,       -1 },
	{ TECH_MODULE_CANNON,           TECH_TYPE_MODULE,        CANNON_WEAPON,         6000 },
	{ TECH_MODULE_FURNACE,          TECH_TYPE_MODULE,        SHIVA_FURNACE,         4000 },
};
const size_t NUM_TECHNOLOGIES = ARRAY_SIZE (tech_data_table);

// Lookup function to get the data for a particular tech
static const TechData* 
GetTechData (TechId_t techId)
{
	size_t i = 0;
	for (i = 0; i < NUM_TECHNOLOGIES; ++i)
	{
		if (tech_data_table[i].id == techId)
			return &tech_data_table[i];
	}
	return NULL;
}


// We have to explicitly switch on the state ID because the xxx_GAME_STATE
// macros use preprocessor stringizing.
static bool
HasStateTech (int stateId)
{
	switch (stateId)
	{
		case IMPROVED_LANDER_SPEED:
			return GET_GAME_STATE (IMPROVED_LANDER_SPEED);
		case IMPROVED_LANDER_CARGO:
			return GET_GAME_STATE (IMPROVED_LANDER_CARGO);
		case IMPROVED_LANDER_SHOT:
			return GET_GAME_STATE (IMPROVED_LANDER_SHOT);
	}
	return false;
}

static void
GrantStateTech (int stateId, BYTE value)
{
	switch (stateId)
	{
		case IMPROVED_LANDER_SPEED:
			SET_GAME_STATE (IMPROVED_LANDER_SPEED, value);
			return;
		case IMPROVED_LANDER_CARGO:
			SET_GAME_STATE (IMPROVED_LANDER_CARGO, value);
			return;
		case IMPROVED_LANDER_SHOT:
			SET_GAME_STATE (IMPROVED_LANDER_SHOT, value);
			return;
	}
}

static bool
HasTech (TechId_t techId)
{
	const TechData* techData = GetTechData (techId);
	if (!techData)
		return false;

	switch (techData->type)
	{
		case TECH_TYPE_MODULE:
			return GLOBAL (ModuleCost[techData->subtype]) != 0;
		case TECH_TYPE_LANDER_SHIELD:
			return (GET_GAME_STATE (LANDER_SHIELDS) & (1 << techData->subtype)) != 0;
		case TECH_TYPE_STATE:
			return HasStateTech (techData->subtype);
	}
	return false;
}

static void
GrantTech (TechId_t techId)
{
	const TechData* techData = GetTechData (techId);
	if (!techData)
		return;

	switch (techData->type)
	{
		case TECH_TYPE_MODULE:
			GLOBAL (ModuleCost[techData->subtype]) = techData->info / MODULE_COST_SCALE;
			return;
		case TECH_TYPE_LANDER_SHIELD:
		{
			COUNT state = GET_GAME_STATE (LANDER_SHIELDS) | (1 << techData->subtype);
			SET_GAME_STATE (LANDER_SHIELDS, state);
			return;
		}
		case TECH_TYPE_STATE:
			GrantStateTech (techData->subtype, techData->info);
			return;
	}
}


////////////Melnorme Sales System///////////
// This section contains code related to Melnorme sales

// Many of the conversation lines in strings.h fall into groups
// of sequential responses.  These structures allow those
// responses to be interated through.
static const int ok_buy_event_lines[] =
{
	OK_BUY_EVENT_1, OK_BUY_EVENT_2, OK_BUY_EVENT_3, OK_BUY_EVENT_4,
	OK_BUY_EVENT_5, OK_BUY_EVENT_6, OK_BUY_EVENT_7, OK_BUY_EVENT_8
};
const size_t NUM_EVENT_ITEMS = ARRAY_SIZE (ok_buy_event_lines);

static const int ok_buy_alien_race_lines[] =
{
	OK_BUY_ALIEN_RACE_1,  OK_BUY_ALIEN_RACE_2,  OK_BUY_ALIEN_RACE_3, 
	OK_BUY_ALIEN_RACE_4,  OK_BUY_ALIEN_RACE_5,  OK_BUY_ALIEN_RACE_6, 
	OK_BUY_ALIEN_RACE_7,  OK_BUY_ALIEN_RACE_8,  OK_BUY_ALIEN_RACE_9, 
	OK_BUY_ALIEN_RACE_10, OK_BUY_ALIEN_RACE_11, OK_BUY_ALIEN_RACE_12,
	OK_BUY_ALIEN_RACE_13, OK_BUY_ALIEN_RACE_14, OK_BUY_ALIEN_RACE_15, 
	OK_BUY_ALIEN_RACE_16
};
const size_t NUM_ALIEN_RACE_ITEMS = ARRAY_SIZE (ok_buy_alien_race_lines);

static const int ok_buy_history_lines[] =
{
	OK_BUY_HISTORY_1, OK_BUY_HISTORY_2, OK_BUY_HISTORY_3, 
	OK_BUY_HISTORY_4, OK_BUY_HISTORY_5, OK_BUY_HISTORY_6, 
	OK_BUY_HISTORY_7, OK_BUY_HISTORY_8, OK_BUY_HISTORY_9
};
const size_t NUM_HISTORY_ITEMS = ARRAY_SIZE (ok_buy_history_lines);

static const int hello_and_down_to_business_lines[] =
{
	HELLO_AND_DOWN_TO_BUSINESS_1, HELLO_AND_DOWN_TO_BUSINESS_2,
	HELLO_AND_DOWN_TO_BUSINESS_3, HELLO_AND_DOWN_TO_BUSINESS_4,
	HELLO_AND_DOWN_TO_BUSINESS_5, HELLO_AND_DOWN_TO_BUSINESS_6,
	HELLO_AND_DOWN_TO_BUSINESS_7, HELLO_AND_DOWN_TO_BUSINESS_8,
	HELLO_AND_DOWN_TO_BUSINESS_9, HELLO_AND_DOWN_TO_BUSINESS_10
};
const size_t NUM_HELLO_LINES = ARRAY_SIZE (hello_and_down_to_business_lines);

static const int rescue_lines[] =
{
	RESCUE_EXPLANATION, RESCUE_AGAIN_1, RESCUE_AGAIN_2,
	RESCUE_AGAIN_3,     RESCUE_AGAIN_4, RESCUE_AGAIN_5
};
const size_t NUM_RESCUE_LINES = ARRAY_SIZE (rescue_lines);

// How many lines are available in the given array?
static size_t
GetNumLines (const int array[])
{
	if (array == ok_buy_event_lines)
		return NUM_EVENT_ITEMS;
	else if (array == ok_buy_alien_race_lines)
		return NUM_ALIEN_RACE_ITEMS;
	else if (array == ok_buy_history_lines)
		return NUM_HISTORY_ITEMS;
	else if (array == hello_and_down_to_business_lines)
		return NUM_HELLO_LINES;
	else if (array == rescue_lines)
		return NUM_RESCUE_LINES;
	return 0;
}

// Get the line, with range checking.
// Returns the last line if the desired one is out of range.
static int 
GetLineSafe (const int array[], size_t linenum)
{
	const size_t array_size = GetNumLines (array);
	assert (array_size > 0);
	if (linenum >= array_size)
		linenum = array_size - 1;
	return array[linenum];
}

// Data structure to hold the Melnorme's info on a technology
typedef struct
{
	TechId_t techId;  // ID of technology
	int price;        // Melnorme's price to sell
	int sale_line;    // Sales pitch line ID
	int sold_line;    // Post-sale line ID
} TechSaleData;

// Right now, all techs have the same price.
#define TECHPRICE (75 * BIO_CREDIT_VALUE)

static const TechSaleData tech_sale_catalog[] =
{
	{ TECH_MODULE_BLASTER,          TECHPRICE, NEW_TECH_1,  OK_BUY_NEW_TECH_1 },
	{ TECH_LANDER_SPEED,            TECHPRICE, NEW_TECH_2,  OK_BUY_NEW_TECH_2 },
	{ TECH_MODULE_ANTIMISSILE,      TECHPRICE, NEW_TECH_3,  OK_BUY_NEW_TECH_3 },
	{ TECH_LANDER_SHIELD_BIO,       TECHPRICE, NEW_TECH_4,  OK_BUY_NEW_TECH_4 },
	{ TECH_LANDER_CARGO,            TECHPRICE, NEW_TECH_5,  OK_BUY_NEW_TECH_5 },
	{ TECH_MODULE_BIGFUELTANK,      TECHPRICE, NEW_TECH_6,  OK_BUY_NEW_TECH_6 },
	{ TECH_LANDER_RAPIDFIRE,        TECHPRICE, NEW_TECH_7,  OK_BUY_NEW_TECH_7 },
	{ TECH_LANDER_SHIELD_QUAKE,     TECHPRICE, NEW_TECH_8,  OK_BUY_NEW_TECH_8 },
	{ TECH_MODULE_TRACKING,         TECHPRICE, NEW_TECH_9,  OK_BUY_NEW_TECH_9 },
	{ TECH_LANDER_SHIELD_LIGHTNING, TECHPRICE, NEW_TECH_10, OK_BUY_NEW_TECH_10 },
	{ TECH_LANDER_SHIELD_HEAT,      TECHPRICE, NEW_TECH_11, OK_BUY_NEW_TECH_11 },
	{ TECH_MODULE_CANNON,           TECHPRICE, NEW_TECH_12, OK_BUY_NEW_TECH_12 },
	{ TECH_MODULE_FURNACE,          TECHPRICE, NEW_TECH_13, OK_BUY_NEW_TECH_13 },
};
const size_t NUM_TECH_ITEMS = ARRAY_SIZE (tech_sale_catalog);

// Return the next tech for sale that the player doesn't already have.
// Returns NULL if the player has all the techs.
static const TechSaleData*
GetNextTechForSale (void) {
	size_t i = 0;
	BYTE j = 0;

	if(DIF_HARD && CurStarDescPtr){
		switch (CurStarDescPtr->Index) {
			case MELNORME0_DEFINED:
				i = TECH_MODULE_CANNON;	 
				j = i + 1;
				break;
			case MELNORME1_DEFINED:
				i = TECH_MODULE_BLASTER; 
				j = i + 1;
				break;
			case MELNORME2_DEFINED:
				i = TECH_LANDER_SHIELD_BIO;	
				j = i + 2;
				break;
			case MELNORME3_DEFINED:
				i = TECH_LANDER_RAPIDFIRE;
				j = i + 2;
				break;
			case MELNORME4_DEFINED:
				i = TECH_MODULE_BIGFUELTANK;	
				j = i + 1;
				break;
			case MELNORME5_DEFINED:
				i = TECH_LANDER_SPEED;
				j = i + 1;
				break;
			case MELNORME6_DEFINED:
				i = TECH_MODULE_TRACKING;	
				j = i + 1;
				break;
			case MELNORME7_DEFINED:
				i = TECH_LANDER_SHIELD_LIGHTNING;	
				j = i + 2;
				break;
			case MELNORME8_DEFINED:
				i = TECH_MODULE_FURNACE;
				j = i + 1;
				break;
			default:
				i = 0; j = i;
		}

		for (i = i; i < j; ++i) {
			if (!HasTech (tech_sale_catalog[i].techId))
				return &tech_sale_catalog[i];
		}
	} else if (DIF_HARD && !CurStarDescPtr) {
		return NULL;
	} else {
		for (i = 0; i < NUM_TECH_ITEMS; ++i) {
			if (!HasTech (tech_sale_catalog[i].techId))
				return &tech_sale_catalog[i];
		}
	}
	return NULL;
}

///////////End Melnorme Sales Section//////////////////

static StatMsgMode prevMsgMode;

static void DoFirstMeeting (RESPONSE_REF R);

static COUNT
ShipWorth (void)
{
	BYTE i;
	SBYTE crew_pods;
	COUNT worth;

	worth = GLOBAL_SIS (NumLanders)
			* GLOBAL (ModuleCost[PLANET_LANDER]);
	for (i = 0; i < NUM_DRIVE_SLOTS; ++i)
	{
		if (GLOBAL_SIS (DriveSlots[i]) < EMPTY_SLOT)
			worth += GLOBAL (ModuleCost[FUSION_THRUSTER]);
	}
	for (i = 0; i < NUM_JET_SLOTS; ++i)
	{
		if (GLOBAL_SIS (JetSlots[i]) < EMPTY_SLOT)
			worth += GLOBAL (ModuleCost[TURNING_JETS]);
	}

	crew_pods = -(SBYTE)(
			(GLOBAL_SIS (CrewEnlisted) + CREW_POD_CAPACITY - 1)
			/ CREW_POD_CAPACITY
			);
	for (i = 0; i < NUM_MODULE_SLOTS; ++i)
	{
		BYTE which_module;

		which_module = GLOBAL_SIS (ModuleSlots[i]);
		if (which_module < BOMB_MODULE_0
				&& (which_module != CREW_POD || ++crew_pods > 0))
		{
			worth += GLOBAL (ModuleCost[which_module]);
		}
	}

	return (worth);
}

static COUNT rescue_fuel;
static SIS_STATE SIS_copy;

// Extract method to return the response string index
// for stripping a given module.
static int 
GetStripModuleRef (int moduleID)
{
	switch (moduleID)
	{
		case PLANET_LANDER:       return LANDERS;
		case FUSION_THRUSTER:     return THRUSTERS;
		case TURNING_JETS:        return JETS;
		case CREW_POD:            return PODS;
		case STORAGE_BAY:         return BAYS;
		case DYNAMO_UNIT:         return DYNAMOS;
		case SHIVA_FURNACE:       return FURNACES;
		case GUN_WEAPON:          return GUNS;
		case BLASTER_WEAPON:      return BLASTERS;
		case CANNON_WEAPON:       return CANNONS;
		case TRACKING_SYSTEM:     return TRACKERS;
		case ANTIMISSILE_DEFENSE: return DEFENSES;
		// If a modder has added new modules, should it really
		// be a fatal error if the Melnorme don't know about
		// them?
		default:
			assert (0 && "Unknown module");
	}
	return 0;
}

static DWORD
getStripRandomSeed (void)
{
	DWORD x, y;
	// We truncate the location because encounters move the ship slightly in
	// HSpace, and throw some other relatively immutable values in the mix to
	// vary the deal when stuck at the same general location again.
	// It is still possible but unlikely for encounters to move the ship into
	// another truncation sector so the player could choose from 2 deals.
	x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x)) / 100;
	y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y)) / 100;
	// prime numbers help randomness
	return y * 1013 + x + GLOBAL_SIS (NumLanders)
			+ GLOBAL_SIS (ModuleSlots[1]) + GLOBAL_SIS (ModuleSlots[4])
			+ GLOBAL_SIS (ModuleSlots[7]) + GLOBAL_SIS (ModuleSlots[10]);
}

static BOOLEAN
StripShip (COUNT fuel_required)
{
	BYTE i, which_module;
	SBYTE crew_pods;

	SET_GAME_STATE (MELNORME_RESCUE_REFUSED, 0);

	crew_pods = -(SBYTE)(
			(GLOBAL_SIS (CrewEnlisted) + CREW_POD_CAPACITY - 1)
			/ CREW_POD_CAPACITY
			);
	if (fuel_required == 0)
	{
		GlobData.SIS_state = SIS_copy;
		DeltaSISGauges (UNDEFINED_DELTA, rescue_fuel, UNDEFINED_DELTA);
	}
	else if (fuel_required == (COUNT)~0)
	{
		GLOBAL_SIS (NumLanders) = 0;
		for (i = 0; i < NUM_DRIVE_SLOTS; ++i)
			GLOBAL_SIS (DriveSlots[i]) = EMPTY_SLOT + 0;
		for (i = 0; i < NUM_JET_SLOTS; ++i)
			GLOBAL_SIS (JetSlots[i]) = EMPTY_SLOT + 1;
		if (GLOBAL_SIS (FuelOnBoard) > FUEL_RESERVE)
			GLOBAL_SIS (FuelOnBoard) = FUEL_RESERVE;
		GLOBAL_SIS (TotalBioMass) = 0;
		GLOBAL_SIS (TotalElementMass) = 0;
		for (i = 0; i < NUM_ELEMENT_CATEGORIES; ++i)
			GLOBAL_SIS (ElementAmounts[i]) = 0;
		for (i = 0; i < NUM_MODULE_SLOTS; ++i)
		{
			which_module = GLOBAL_SIS (ModuleSlots[i]);
			if (which_module < BOMB_MODULE_0
					&& (which_module != CREW_POD
					|| ++crew_pods > 0))
				GLOBAL_SIS (ModuleSlots[i]) = EMPTY_SLOT + 2;
		}

		DeltaSISGauges (UNDEFINED_DELTA, UNDEFINED_DELTA, UNDEFINED_DELTA);
	}
	else if (fuel_required)
	{
		SBYTE bays;
		BYTE num_searches, beg_mod, end_mod;
		COUNT worth, total;
		BYTE module_count[BOMB_MODULE_0];
		BYTE slot;
		DWORD capacity;
		RandomContext *rc;
		
		// Bug #567
		// In order to offer the same deal each time if it is refused, we seed
		// the random number generator with our location, thus making the deal
		// a repeatable pseudo-random function of where we got stuck and what,
		// exactly, is on our ship.
		rc = RandomContext_New();
		RandomContext_SeedRandom (rc, getStripRandomSeed ());

		SIS_copy = GlobData.SIS_state;
		for (i = PLANET_LANDER; i < BOMB_MODULE_0; ++i)
			module_count[i] = 0;

		capacity = FUEL_RESERVE;
		slot = NUM_MODULE_SLOTS - 1;
		do
		{
			if (SIS_copy.ModuleSlots[slot] == FUEL_TANK
					|| SIS_copy.ModuleSlots[slot] == HIGHEFF_FUELSYS)
			{
				COUNT volume;

				volume = SIS_copy.ModuleSlots[slot] == FUEL_TANK
						? FUEL_TANK_CAPACITY : HEFUEL_TANK_CAPACITY;
				capacity += volume;
			}
		} while (slot--);
		if (fuel_required > capacity)
			fuel_required = capacity;

		bays = -(SBYTE)(
				(SIS_copy.TotalElementMass + STORAGE_BAY_CAPACITY - 1)
				/ STORAGE_BAY_CAPACITY
				);
		for (i = 0; i < NUM_MODULE_SLOTS; ++i)
		{
			which_module = SIS_copy.ModuleSlots[i];
			if (which_module == CREW_POD)
				++crew_pods;
			else if (which_module == STORAGE_BAY)
				++bays;
		}

		worth = fuel_required / FUEL_TANK_SCALE;
		total = 0;
		num_searches = 0;
		beg_mod = end_mod = (BYTE)~0;
		while (total < worth && ShipWorth () && ++num_searches)
		{
			DWORD rand_val;

			rand_val = RandomContext_Random (rc);
			switch (which_module = LOBYTE (LOWORD (rand_val)) % (CREW_POD + 1))
			{
				case PLANET_LANDER:
					if (SIS_copy.NumLanders == 0)
						continue;
					--SIS_copy.NumLanders;
					break;
				case FUSION_THRUSTER:
					for (i = 0; i < NUM_DRIVE_SLOTS; ++i)
					{
						if (SIS_copy.DriveSlots[i] < EMPTY_SLOT)
							break;
					}
					if (i == NUM_DRIVE_SLOTS)
						continue;
					SIS_copy.DriveSlots[i] = EMPTY_SLOT + 0;
					break;
				case TURNING_JETS:
					for (i = 0; i < NUM_JET_SLOTS; ++i)
					{
						if (SIS_copy.JetSlots[i] < EMPTY_SLOT)
							break;
					}
					if (i == NUM_JET_SLOTS)
						continue;
					SIS_copy.JetSlots[i] = EMPTY_SLOT + 1;
					break;
				case CREW_POD:
					i = HIBYTE (LOWORD (rand_val)) % NUM_MODULE_SLOTS;
					which_module = SIS_copy.ModuleSlots[i];
					if (which_module >= BOMB_MODULE_0
							|| which_module == FUEL_TANK
							|| which_module == HIGHEFF_FUELSYS
							|| (which_module == STORAGE_BAY
							&& module_count[STORAGE_BAY] >= bays)
							|| (which_module == CREW_POD
							&& module_count[CREW_POD] >= crew_pods))
						continue;
					SIS_copy.ModuleSlots[i] = EMPTY_SLOT + 2;
					break;
			}

			if (beg_mod == (BYTE)~0)
				beg_mod = end_mod = which_module;
			else if (which_module > end_mod)
				end_mod = which_module;
			++module_count[which_module];
			total += GLOBAL (ModuleCost[which_module]);
		}
		RandomContext_Delete (rc);

		if (total == 0)
		{
			NPCPhrase (CHARITY);
			DeltaSISGauges (0, fuel_required, 0);
			return (FALSE);
		}
		else
		{
			NPCPhrase (RESCUE_OFFER);
			rescue_fuel = fuel_required;
			if (rescue_fuel == capacity)
				NPCPhrase (RESCUE_TANKS);
			else
				NPCPhrase (RESCUE_HOME);
			for (i = PLANET_LANDER; i < BOMB_MODULE_0; ++i)
			{
				if (module_count[i])
				{
					if (i == end_mod && i != beg_mod)
						NPCPhrase (END_LIST_WITH_AND);
					NPCPhrase (ENUMERATE_ONE + (module_count[i] - 1));
					NPCPhrase (GetStripModuleRef (i));
				}
			}
		}
	}

	return (TRUE);
}

static void
ExitConversation (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, no_trade_now))
		NPCPhrase (OK_NO_TRADE_NOW_BYE);
	else if (PLAYER_SAID (R, youre_on))
	{
		NPCPhrase (YOU_GIVE_US_NO_CHOICE);

		SET_GAME_STATE (MELNORME_ANGER, 1);
		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, so_we_can_attack))
	{
		NPCPhrase (DECEITFUL_HUMAN);

		SET_GAME_STATE (MELNORME_ANGER, 2);
		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, bye_melnorme_slightly_angry))
		NPCPhrase (MELNORME_SLIGHTLY_ANGRY_GOODBYE);
	else if (PLAYER_SAID (R, ok_strip_me))
	{
		if (ShipWorth () < 4000 / MODULE_COST_SCALE)
				/* is ship worth stripping */
			NPCPhrase (NOT_WORTH_STRIPPING);
		else
		{
			SET_GAME_STATE (MELNORME_ANGER, 0);

			StripShip ((COUNT)~0);
			NPCPhrase (FAIR_JUSTICE);
		}
	}
	else if (PLAYER_SAID (R, fight_some_more))
	{
		NPCPhrase (OK_FIGHT_SOME_MORE);

		SET_GAME_STATE (MELNORME_ANGER, 3);
		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, bye_melnorme_pissed_off))
		NPCPhrase (MELNORME_PISSED_OFF_GOODBYE);
	else if (PLAYER_SAID (R, well_if_thats_the_way_you_feel))
	{
		NPCPhrase (WE_FIGHT_AGAIN);

		setSegue (Segue_hostile);
	}
	else if (PLAYER_SAID (R, you_hate_us_so_we_go_away))
		NPCPhrase (HATE_YOU_GOODBYE);
	else if (PLAYER_SAID (R, take_it))
	{
		StripShip (0);
		NPCPhrase (HAPPY_TO_HAVE_RESCUED);
	}
	else if (PLAYER_SAID (R, leave_it))
	{
		SET_GAME_STATE (MELNORME_RESCUE_REFUSED, 1);
		NPCPhrase (MAYBE_SEE_YOU_LATER);
	}
	else if (PLAYER_SAID (R, no_help))
	{
		SET_GAME_STATE (MELNORME_RESCUE_REFUSED, 1);
		NPCPhrase (GOODBYE_AND_GOODLUCK);
	}
	else if (PLAYER_SAID (R, no_changed_mind))
	{
		NPCPhrase (GOODBYE_AND_GOODLUCK_AGAIN);
	}
	else if (PLAYER_SAID (R, be_leaving_now)
			|| PLAYER_SAID (R, goodbye))
	{
		NPCPhrase (FRIENDLY_GOODBYE);
	}
}

static void
DoRescue (RESPONSE_REF R)
{
	SIZE dx, dy;
	COUNT fuel_required;

	(void) R;  // ignored
	dx = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x))
			- SOL_X;
	dy = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y))
			- SOL_Y;
	fuel_required = square_root (
			(DWORD)((long)dx * dx + (long)dy * dy)
			) + (2 * FUEL_TANK_SCALE);

	if (StripShip (fuel_required))
	{
		Response (take_it, ExitConversation);
		Response (leave_it, ExitConversation);
	}
}

// Extract method for getting the player's current credits.
static COUNT 
GetAvailableCredits (void)
{
	return MAKE_WORD (GET_GAME_STATE (MELNORME_CREDIT0),
			GET_GAME_STATE (MELNORME_CREDIT1));
}

// Extract method for setting the player's current credits.
static void
SetAvailableCredits (COUNT credits)
{
	SET_GAME_STATE (MELNORME_CREDIT0, LOBYTE (credits));
	SET_GAME_STATE (MELNORME_CREDIT1, HIBYTE (credits));
}

// Now returns whether the purchase succeeded instead of the remaining
// credit balance.  Use GetAvailableCredits() to get the latter.
static bool
DeltaCredit (SIZE delta_credit)
{
	COUNT Credit = GetAvailableCredits ();

	// Can they afford it?
	if ((int)delta_credit >= 0 || ((int)(-delta_credit) <= (int)(Credit)))
	{
		Credit += delta_credit;
		SetAvailableCredits (Credit);
		DrawStatusMessage (NULL);
		return true;
	}

	// Fail
	NPCPhrase (NEED_MORE_CREDIT0);
	NPCNumber (-delta_credit - Credit, NULL);
	NPCPhrase (NEED_MORE_CREDIT1);
	
	return false;
}


// Extract methods to process the giving of various bits of information to the
// player.  Ideally, we'd want to merge these three into a single parameterized
// function, but the nature of the XXX_GAME_STATE() code makes that tricky.
static void
CurrentEvents (void)
{
	BYTE stack = GET_GAME_STATE (MELNORME_EVENTS_INFO_STACK);
	const int phraseId = GetLineSafe (ok_buy_event_lines, stack);
	NPCPhrase (phraseId);
	SET_GAME_STATE (MELNORME_EVENTS_INFO_STACK, stack + 1);
}

static void
AlienRaces (void)
{
	BYTE stack = GET_GAME_STATE (MELNORME_ALIEN_INFO_STACK);
	const int phraseId = GetLineSafe (ok_buy_alien_race_lines, stack);
	// Two pieces of alien knowledge trigger state changes.
	switch (phraseId)
	{
		case OK_BUY_ALIEN_RACE_14:
			if (!GET_GAME_STATE (FOUND_PLUTO_SPATHI))
			{
				SET_GAME_STATE (KNOW_SPATHI_PASSWORD, 1);
				SET_GAME_STATE (SPATHI_HOME_VISITS, 7);
			}
			break;
		case OK_BUY_ALIEN_RACE_15:
			if (GET_GAME_STATE (KNOW_ABOUT_SHATTERED) < 2)
			{
				SET_GAME_STATE (KNOW_ABOUT_SHATTERED, 2);
			}
			SET_GAME_STATE (KNOW_SYREEN_WORLD_SHATTERED, 1);
			break;
	}
	NPCPhrase (phraseId);
	SET_GAME_STATE (MELNORME_ALIEN_INFO_STACK, stack + 1);
}

static void
History (void)
{
	BYTE stack = GET_GAME_STATE (MELNORME_HISTORY_INFO_STACK);
	const int phraseId = GetLineSafe (ok_buy_history_lines, stack);
	NPCPhrase (phraseId);
	SET_GAME_STATE (MELNORME_HISTORY_INFO_STACK, stack + 1);
}

// extract method to tell if we have any information left to sell to the player.
static bool AnyInfoLeftToSell (void)
{
	return GET_GAME_STATE (MELNORME_EVENTS_INFO_STACK) < NUM_EVENT_ITEMS
			|| GET_GAME_STATE (MELNORME_ALIEN_INFO_STACK) < NUM_ALIEN_RACE_ITEMS
			|| GET_GAME_STATE (MELNORME_HISTORY_INFO_STACK) < NUM_HISTORY_ITEMS;
}

static void NatureOfConversation (RESPONSE_REF R);

static BYTE AskedToBuy;


static void
DoBuy (RESPONSE_REF R)
{
	COUNT credit;
	SIZE needed_credit;
	BYTE slot;
	DWORD capacity; 
	BYTE FuelCost = IF_HARD((BIO_CREDIT_VALUE / 2), (FUEL_COST_RU / 2));

	credit = GetAvailableCredits ();

	capacity = FUEL_RESERVE;
	slot = NUM_MODULE_SLOTS - 1;
	do
	{
		if (GLOBAL_SIS (ModuleSlots[slot]) == FUEL_TANK
				|| GLOBAL_SIS (ModuleSlots[slot]) == HIGHEFF_FUELSYS)
		{
			COUNT volume;

			volume = GLOBAL_SIS (ModuleSlots[slot]) == FUEL_TANK
					? FUEL_TANK_CAPACITY : HEFUEL_TANK_CAPACITY;
			capacity += volume;
		}
	} while (slot--);

	// If they're out of credits, educate them on how commerce works.
	if (credit == 0)
	{
		AskedToBuy = TRUE;
		NPCPhrase (NEED_CREDIT);

		NatureOfConversation (R);
	}
	else if (PLAYER_SAID (R, buy_fuel)
			|| PLAYER_SAID (R, buy_1_fuel)
			|| PLAYER_SAID (R, buy_5_fuel)
			|| PLAYER_SAID (R, buy_10_fuel)
			|| PLAYER_SAID (R, buy_25_fuel)
			|| PLAYER_SAID (R, fill_me_up))
	{
		needed_credit = 0;
		if (PLAYER_SAID (R, buy_1_fuel))
			needed_credit = 1;
		else if (PLAYER_SAID (R, buy_5_fuel))
			needed_credit = 5;
		else if (PLAYER_SAID (R, buy_10_fuel))
			needed_credit = 10;
		else if (PLAYER_SAID (R, buy_25_fuel))
			needed_credit = 25;
		else if (PLAYER_SAID (R, fill_me_up))
			needed_credit = (capacity - GLOBAL_SIS (FuelOnBoard)
					+ FUEL_TANK_SCALE - 1)
				/ FUEL_TANK_SCALE;

		if (needed_credit == 0)
		{
			if (!GET_GAME_STATE (MELNORME_FUEL_PROCEDURE))
			{
				NPCPhrase (BUY_FUEL_INTRO);
				SET_GAME_STATE (MELNORME_FUEL_PROCEDURE, 1);
			}
		}
		else
		{
			if (GLOBAL_SIS (FuelOnBoard) / FUEL_TANK_SCALE
					+ needed_credit > capacity / FUEL_TANK_SCALE)
			{
				NPCPhrase (NO_ROOM_FOR_FUEL);
				goto TryFuelAgain;
			}

			if ((int)(needed_credit * FuelCost) <= (int)credit)
			{
				DWORD f;

				NPCPhrase (GOT_FUEL);

				f = (DWORD)needed_credit * FUEL_TANK_SCALE;
				while (f > 0x3FFFL)
				{
					DeltaSISGauges (0, 0x3FFF, 0);
					f -= 0x3FFF;
				}
				DeltaSISGauges (0, f, 0);
			}
			needed_credit *= FuelCost;
		}
		if (needed_credit)
		{
			DeltaCredit (-needed_credit);
			if (GLOBAL_SIS (FuelOnBoard) >= capacity)
				goto BuyBuyBuy;
		}
TryFuelAgain:
		NPCPhrase (HOW_MUCH_FUEL);

		Response (buy_1_fuel, DoBuy);
		Response (buy_5_fuel, DoBuy);
		Response (buy_10_fuel, DoBuy);
		Response (buy_25_fuel, DoBuy);
		Response (fill_me_up, DoBuy);
		Response (done_buying_fuel, DoBuy);
	}
	else if (PLAYER_SAID (R, buy_technology)
			|| PLAYER_SAID (R, buy_new_tech))
	{
		// Note that this code no longer uses the MELNORME_TECH_STACK state
		// buts, as they're not needed; we can tell what technologies the
		// player has by using the technology API above.  This opens the
		// possibility of the player acquiring tech from someplace other than
		// the Melnorme.
		const TechSaleData* nextTech;

		// If it's our first time, give an introduction.
		if (!GET_GAME_STATE (MELNORME_TECH_PROCEDURE))
		{
			NPCPhrase (BUY_NEW_TECH_INTRO);
			SET_GAME_STATE (MELNORME_TECH_PROCEDURE, 1);
		}

		// Did the player just attempt to buy a tech?
		if (PLAYER_SAID (R, buy_new_tech))
		{
			nextTech = GetNextTechForSale ();
			if (!nextTech)
				goto BuyBuyBuy; // No tech left to buy

			if (!DeltaCredit (-nextTech->price))
				goto BuyBuyBuy;  // Can't afford it

			// Make the sale
			GrantTech (nextTech->techId);
			NPCPhrase (nextTech->sold_line);
		}

		nextTech = GetNextTechForSale ();
		if (!nextTech)
		{
			NPCPhrase (NEW_TECH_ALL_GONE);
			goto BuyBuyBuy; // No tech left to buy
		}

		NPCPhrase (nextTech->sale_line);

		Response (buy_new_tech, DoBuy);
		Response (no_buy_new_tech, DoBuy);
	}
	else if (PLAYER_SAID (R, buy_info)
			|| PLAYER_SAID (R, buy_current_events)
			|| PLAYER_SAID (R, buy_alien_races)
			|| PLAYER_SAID (R, buy_history))
	{
		if (!GET_GAME_STATE (MELNORME_INFO_PROCEDURE))
		{
			NPCPhrase (BUY_INFO_INTRO);
			SET_GAME_STATE (MELNORME_INFO_PROCEDURE, 1);
		}
		else if (PLAYER_SAID (R, buy_info))
		{
			NPCPhrase (OK_BUY_INFO);
		}
		else
		{
#define INFO_COST 75
			if (!DeltaCredit (-INFO_COST))
				goto BuyBuyBuy;

			if (PLAYER_SAID (R, buy_current_events))
				CurrentEvents ();
			else if (PLAYER_SAID (R, buy_alien_races))
				AlienRaces ();
			else if (PLAYER_SAID (R, buy_history))
				History ();
		}

		if (!AnyInfoLeftToSell ())
		{
			NPCPhrase (INFO_ALL_GONE);
			goto BuyBuyBuy;
		}

		if (GET_GAME_STATE (MELNORME_EVENTS_INFO_STACK) < NUM_EVENT_ITEMS)
			Response (buy_current_events, DoBuy);
		if (GET_GAME_STATE (MELNORME_ALIEN_INFO_STACK) < NUM_ALIEN_RACE_ITEMS)
			Response (buy_alien_races, DoBuy);
		if (GET_GAME_STATE (MELNORME_HISTORY_INFO_STACK) < NUM_HISTORY_ITEMS)
			Response (buy_history, DoBuy);
		Response (done_buying_info, DoBuy);
	}
	else
	{
		if (PLAYER_SAID (R, done_buying_fuel))
			NPCPhrase (OK_DONE_BUYING_FUEL);
		else if (PLAYER_SAID (R, no_buy_new_tech))
			NPCPhrase (OK_NO_BUY_NEW_TECH);
		else if (PLAYER_SAID (R, done_buying_info))
			NPCPhrase (OK_DONE_BUYING_INFO);
		else
			NPCPhrase (WHAT_TO_BUY);

BuyBuyBuy:
		if (GLOBAL_SIS (FuelOnBoard) < capacity)
			Response (buy_fuel, DoBuy);
		if (GetNextTechForSale ())
			Response (buy_technology, DoBuy);
		if (AnyInfoLeftToSell ())
			Response (buy_info, DoBuy);

		Response (done_buying, NatureOfConversation);
		Response (be_leaving_now, ExitConversation);
	}
}

static void
DoSell (RESPONSE_REF R)
{
	BYTE num_new_rainbows;
	UWORD rainbow_mask;
	SIZE added_credit;
	int what_to_sell_queued = 0;

	rainbow_mask = MAKE_WORD (
			GET_GAME_STATE (RAINBOW_WORLD0),
			GET_GAME_STATE (RAINBOW_WORLD1)
			);
	num_new_rainbows = (BYTE)(-GET_GAME_STATE (MELNORME_RAINBOW_COUNT));
	while (rainbow_mask)
	{
		if (rainbow_mask & 1)
			++num_new_rainbows;

		rainbow_mask >>= 1;
	}

	if (!PLAYER_SAID (R, sell))
	{
		if (PLAYER_SAID (R, sell_life_data))
		{
			DWORD TimeIn;

			added_credit = GLOBAL_SIS (TotalBioMass) * BIO_CREDIT_VALUE;

			NPCPhrase (SOLD_LIFE_DATA1);
			NPCNumber (GLOBAL_SIS (TotalBioMass), NULL);
			NPCPhrase (SOLD_LIFE_DATA2);
			NPCNumber (added_credit, NULL);
			NPCPhrase (SOLD_LIFE_DATA3);
			// queue WHAT_TO_SELL before talk-segue
			if (num_new_rainbows)
			{
				NPCPhrase (WHAT_TO_SELL);
				what_to_sell_queued = 1;
			}
			AlienTalkSegue (1);

			DrawCargoStrings ((BYTE)~0, (BYTE)~0);
			SleepThread (ONE_SECOND / 2);
			TimeIn = GetTimeCounter ();
			DrawCargoStrings (
					(BYTE)NUM_ELEMENT_CATEGORIES,
					(BYTE)NUM_ELEMENT_CATEGORIES
					);
			do
			{
				TimeIn = GetTimeCounter ();
				if (AnyButtonPress (TRUE))
				{
					DeltaCredit (GLOBAL_SIS (TotalBioMass) * BIO_CREDIT_VALUE);
					GLOBAL_SIS (TotalBioMass) = 0;
				}
				else
				{
					--GLOBAL_SIS (TotalBioMass);
					DeltaCredit (BIO_CREDIT_VALUE);
				}
				DrawCargoStrings (
						(BYTE)NUM_ELEMENT_CATEGORIES,
						(BYTE)NUM_ELEMENT_CATEGORIES
						);
			} while (GLOBAL_SIS (TotalBioMass));
			SleepThread (ONE_SECOND / 2);

			ClearSISRect (DRAW_SIS_DISPLAY);
		}
		else /* if (R == sell_rainbow_locations) */
		{
			added_credit = num_new_rainbows * (DIF_CASE(250, 500, 125) * BIO_CREDIT_VALUE);

			NPCPhrase (SOLD_RAINBOW_LOCATIONS1);
			NPCNumber (num_new_rainbows, NULL);
			NPCPhrase (SOLD_RAINBOW_LOCATIONS2);
			NPCNumber (added_credit, NULL);
			NPCPhrase (SOLD_RAINBOW_LOCATIONS3);

			num_new_rainbows += GET_GAME_STATE (MELNORME_RAINBOW_COUNT);
			SET_GAME_STATE (MELNORME_RAINBOW_COUNT, num_new_rainbows);
			num_new_rainbows = 0;

			DeltaCredit (added_credit);
		}
		
		AskedToBuy = FALSE;
	}

	if (GLOBAL_SIS (TotalBioMass) || num_new_rainbows)
	{
		if (!what_to_sell_queued)
			NPCPhrase (WHAT_TO_SELL);

		if (GLOBAL_SIS (TotalBioMass))
			Response (sell_life_data, DoSell);
		if (num_new_rainbows)
			Response (sell_rainbow_locations, DoSell);
		Response (done_selling, NatureOfConversation);
	}
	else
	{
		if (PLAYER_SAID (R, sell))
			NPCPhrase (NOTHING_TO_SELL);
		DISABLE_PHRASE (sell);

		NatureOfConversation (R);
	}
}


static void
NatureOfConversation (RESPONSE_REF R)
{
	BYTE num_new_rainbows;
	UWORD rainbow_mask;
	COUNT Credit;

	if (PLAYER_SAID (R, get_on_with_business))
	{
		SET_GAME_STATE (MELNORME_YACK_STACK2, 5);
		R = 0;
	}

	// Draw credits display
	DeltaCredit (0);
	Credit = GetAvailableCredits ();
	if (R == 0)
	{
		BYTE stack = GET_GAME_STATE (MELNORME_YACK_STACK2) - 5;
		NPCPhrase (GetLineSafe (hello_and_down_to_business_lines, stack));
		if (stack < (NUM_HELLO_LINES - 1))
			++stack;
		SET_GAME_STATE (MELNORME_YACK_STACK2, stack + 5);
	}

	rainbow_mask = MAKE_WORD (
			GET_GAME_STATE (RAINBOW_WORLD0),
			GET_GAME_STATE (RAINBOW_WORLD1)
			);
	num_new_rainbows = (BYTE)(-GET_GAME_STATE (MELNORME_RAINBOW_COUNT));
	while (rainbow_mask)
	{
		if (rainbow_mask & 1)
			++num_new_rainbows;

		rainbow_mask >>= 1;
	}

	if (GLOBAL_SIS (FuelOnBoard) > 0
			|| GLOBAL_SIS (TotalBioMass)
			|| Credit
			|| num_new_rainbows)
	{
		if (!GET_GAME_STATE (TRADED_WITH_MELNORME))
		{
			SET_GAME_STATE (TRADED_WITH_MELNORME, 1);

			NPCPhrase (TRADING_INFO);
		}

		if (R == 0)
		{
				/* Melnorme reports any news and turns purple */
			NPCPhrase (BUY_OR_SELL);
			AlienTalkSegue (1);

			if (!IS_HD) {
				XFormColorMap (GetColorMapAddress (
					SetAbsColorMapIndex (CommData.AlienColorMap, 1)
					), ONE_SECOND / 2);
			} else {
				CommData.AlienAmbientArray[2].AnimFlags |= ANIM_DISABLED;
				CommData.AlienAmbientArray[3].AnimFlags |= ANIM_DISABLED;
				CommData.AlienAmbientArray[4].AnimFlags &= ~ANIM_DISABLED;
				CommData.AlienAmbientArray[5].AnimFlags &= ~ANIM_DISABLED;
				CommData.AlienAmbientArray[6].AnimFlags &= ~ANIM_DISABLED;
				CommData.AlienAmbientArray[7].AnimFlags |= ANIM_DISABLED;
				CommData.AlienAmbientArray[8].AnimFlags |= ANIM_DISABLED;
				CommData.AlienAmbientArray[9].AnimFlags |= ANIM_DISABLED;
				
				CommData.AlienFrame = SetAbsFrameIndex (CommData.AlienFrame, 33);
			} 

			AlienTalkSegue ((COUNT)~0);
		}
		else if (PLAYER_SAID (R, why_turned_purple))
		{
			SET_GAME_STATE (WHY_MELNORME_PURPLE, 1);

			NPCPhrase (TURNED_PURPLE_BECAUSE);
		}
		else if (PLAYER_SAID (R, done_selling))
		{
			NPCPhrase (OK_DONE_SELLING);
		}
		else if (PLAYER_SAID (R, done_buying))
		{
			NPCPhrase (OK_DONE_BUYING);
		}

		if (!GET_GAME_STATE (WHY_MELNORME_PURPLE))
		{
			Response (why_turned_purple, NatureOfConversation);
		}
		if (!AskedToBuy)
			Response (buy, DoBuy);
		if (PHRASE_ENABLED (sell))
			Response (sell, DoSell);
		Response (goodbye, ExitConversation);
	}
	else /* needs to be rescued */
	{
		if (GET_GAME_STATE (MELNORME_RESCUE_REFUSED))
		{
			NPCPhrase (CHANGED_MIND);

			Response (yes_changed_mind, DoRescue);
			Response (no_changed_mind, ExitConversation);
		}
		else
		{
			BYTE num_rescues = GET_GAME_STATE (MELNORME_RESCUE_COUNT);
			NPCPhrase (GetLineSafe (rescue_lines, num_rescues));

			if (num_rescues < NUM_RESCUE_LINES - 1)
			{
				++num_rescues;
				SET_GAME_STATE (MELNORME_RESCUE_COUNT, num_rescues);
			}

			NPCPhrase (SHOULD_WE_HELP_YOU);

			Response (yes_help, DoRescue);
			Response (no_help, ExitConversation);
		}
	}
}

static BYTE local_stack0, local_stack1;

static void
DoBluster (RESPONSE_REF R)
{
	if (PLAYER_SAID (R, trade_is_for_the_weak))
	{
		NPCPhrase (WERE_NOT_AFRAID);
		AlienTalkSegue ((COUNT)~0);

		if (!IS_HD) {
			XFormColorMap (GetColorMapAddress (
				SetAbsColorMapIndex (CommData.AlienColorMap, 2)
				), ONE_SECOND / 2);
		} else {
			CommData.AlienAmbientArray[2].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[3].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[4].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienAmbientArray[5].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienAmbientArray[6].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienAmbientArray[7].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[8].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[9].AnimFlags |= ANIM_DISABLED;
				
			CommData.AlienFrame = SetAbsFrameIndex (CommData.AlienFrame, 48);
		}

		SET_GAME_STATE (MELNORME_YACK_STACK2, 4);
	}
	else if (PLAYER_SAID (R, why_blue_light))
	{
		SET_GAME_STATE (WHY_MELNORME_BLUE, 1);

		NPCPhrase (BLUE_IS_MAD);
	}
	else if (PLAYER_SAID (R, we_strong_1))
	{
		local_stack0 = 1;
		NPCPhrase (YOU_NOT_STRONG_1);
	}
	else if (PLAYER_SAID (R, we_strong_2))
	{
		local_stack0 = 2;
		NPCPhrase (YOU_NOT_STRONG_2);
	}
	else if (PLAYER_SAID (R, we_strong_3))
	{
		local_stack0 = 3;
		NPCPhrase (YOU_NOT_STRONG_3);
	}
	else if (PLAYER_SAID (R, just_testing))
	{
		local_stack1 = 1;
		NPCPhrase (REALLY_TESTING);
	}

	if (!GET_GAME_STATE (WHY_MELNORME_BLUE))
		Response (why_blue_light, DoBluster);
	switch (local_stack0)
	{
		case 0:
			Response (we_strong_1, DoBluster);
			break;
		case 1:
			Response (we_strong_2, DoBluster);
			break;
		case 2:
			Response (we_strong_3, DoBluster);
			break;
	}
	switch (local_stack1)
	{
		case 0:
			Response (just_testing, DoBluster);
			break;
		case 1:
		{
			Response (yes_really_testing, DoFirstMeeting);
			break;
		}
	}
	Response (youre_on, ExitConversation);
}

static void
yack0_respond (void)
{

	switch (GET_GAME_STATE (MELNORME_YACK_STACK0))
	{
		case 0:
			Response (we_are_from_alliance, DoFirstMeeting);
			break;
		case 1:
			Response (how_know, DoFirstMeeting);
			break;
	}
}

static void
yack1_respond (void)
{
	switch (GET_GAME_STATE (MELNORME_YACK_STACK1))
	{
		case 0:
			Response (what_about_yourselves, DoFirstMeeting);
			break;
		case 1:
			Response (what_factors, DoFirstMeeting);
		case 2:
			Response (get_on_with_business, NatureOfConversation);
			break;
	}
}

static void
yack2_respond (void)
{
	switch (GET_GAME_STATE (MELNORME_YACK_STACK2))
	{
		case 0:
			Response (what_about_universe, DoFirstMeeting);
			break;
		case 1:
			Response (giving_is_good_1, DoFirstMeeting);
			break;
		case 2:
			Response (giving_is_good_2, DoFirstMeeting);
			break;
		case 3:
			Response (trade_is_for_the_weak, DoBluster);
			break;
	}
}

static void
DoFirstMeeting (RESPONSE_REF R)
{
	BYTE last_stack = 0;
	PVOIDFUNC temp_func, stack_func[] =
	{
		yack0_respond,
		yack1_respond,
		yack2_respond,
	};

	if (R == 0)
	{
		BYTE business_count;

		business_count = GET_GAME_STATE (MELNORME_BUSINESS_COUNT);
		switch (business_count++)
		{
			case 0:
				NPCPhrase (HELLO_NOW_DOWN_TO_BUSINESS_1);
				break;
			case 1:
				NPCPhrase (HELLO_NOW_DOWN_TO_BUSINESS_2);
				break;
			case 2:
				NPCPhrase (HELLO_NOW_DOWN_TO_BUSINESS_3);
				--business_count;
				break;
		}
		SET_GAME_STATE (MELNORME_BUSINESS_COUNT, business_count);
	}
	else if (PLAYER_SAID (R, we_are_from_alliance))
	{
		SET_GAME_STATE (MELNORME_YACK_STACK0, 1);
		NPCPhrase (KNOW_OF_YOU);
	}
	else if (PLAYER_SAID (R, how_know))
	{
		SET_GAME_STATE (MELNORME_YACK_STACK0, 2);
		NPCPhrase (KNOW_BECAUSE);
	}
	else if (PLAYER_SAID (R, what_about_yourselves))
	{
		last_stack = 1;
		SET_GAME_STATE (MELNORME_YACK_STACK1, 1);
		NPCPhrase (NO_TALK_ABOUT_OURSELVES);
	}
	else if (PLAYER_SAID (R, what_factors))
	{
		last_stack = 1;
		SET_GAME_STATE (MELNORME_YACK_STACK1, 2);
		NPCPhrase (FACTORS_ARE);
	}
	else if (PLAYER_SAID (R, what_about_universe))
	{
		last_stack = 2;
		SET_GAME_STATE (MELNORME_YACK_STACK2, 1);
		NPCPhrase (NO_FREE_LUNCH);
	}
	else if (PLAYER_SAID (R, giving_is_good_1))
	{
		last_stack = 2;
		SET_GAME_STATE (MELNORME_YACK_STACK2, 2);
		NPCPhrase (GIVING_IS_BAD_1);
	}
	else if (PLAYER_SAID (R, giving_is_good_2))
	{
		last_stack = 2;
		SET_GAME_STATE (MELNORME_YACK_STACK2, 3);
		NPCPhrase (GIVING_IS_BAD_2);
	}
	else if (PLAYER_SAID (R, yes_really_testing))
	{
		NPCPhrase (TEST_RESULTS);
		AlienTalkSegue ((COUNT)~0);
		
		if (!IS_HD) {
			XFormColorMap (GetColorMapAddress (
				SetAbsColorMapIndex (CommData.AlienColorMap, 0)
				), ONE_SECOND / 2);
		} else {
	
			CommData.AlienAmbientArray[10].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienAmbientArray[2].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienAmbientArray[3].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienAmbientArray[4].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[5].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[6].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[7].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[8].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[9].AnimFlags |= ANIM_DISABLED;
			
			CommData.AlienFrame = SetAbsFrameIndex (CommData.AlienFrame, 0);
		}
	}
	else if (PLAYER_SAID (R, we_apologize))
	{
		SET_GAME_STATE (MELNORME_ANGER, 0);
		NPCPhrase (APOLOGY_ACCEPTED);
		AlienTalkSegue ((COUNT)~0);
		
		if (!IS_HD) {
			XFormColorMap (GetColorMapAddress (
				SetAbsColorMapIndex (CommData.AlienColorMap, 0)
				), ONE_SECOND / 2);
		} else {
			
			CommData.AlienAmbientArray[10].AnimFlags &= ~ANIM_DISABLED;	
			CommData.AlienAmbientArray[2].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienAmbientArray[3].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienAmbientArray[4].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[5].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[6].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[7].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[8].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[9].AnimFlags |= ANIM_DISABLED;
			
			CommData.AlienFrame = SetAbsFrameIndex (CommData.AlienFrame, 0);
			
		}
	}

	temp_func = stack_func[0];
	stack_func[0] = stack_func[last_stack];
	stack_func[last_stack] = temp_func;
	(*stack_func[0]) ();
	(*stack_func[1]) ();
	(*stack_func[2]) ();
	Response (no_trade_now, ExitConversation);
}

static void
DoMelnormeMiffed (RESPONSE_REF R)
{
	if (R == 0)
	{
		BYTE miffed_count;

		miffed_count = GET_GAME_STATE (MELNORME_MIFFED_COUNT);
		switch (miffed_count++)
		{
			case 0:
				NPCPhrase (HELLO_SLIGHTLY_ANGRY_1);
				break;
			case 1:
				NPCPhrase (HELLO_SLIGHTLY_ANGRY_2);
				break;
			default:
				--miffed_count;
				NPCPhrase (HELLO_SLIGHTLY_ANGRY_3);
				break;
		}
		SET_GAME_STATE (MELNORME_MIFFED_COUNT, miffed_count);

		AlienTalkSegue ((COUNT)~0);
		
		if (!IS_HD) {
			XFormColorMap (GetColorMapAddress (
 				SetAbsColorMapIndex (CommData.AlienColorMap, 2)
 				), ONE_SECOND / 2);
		} else {
			
			CommData.AlienAmbientArray[2].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[3].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[4].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[5].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[6].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[7].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienAmbientArray[8].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienAmbientArray[9].AnimFlags &= ~ANIM_DISABLED;
			
			CommData.AlienFrame = SetAbsFrameIndex (CommData.AlienFrame, 48);
		}
	}
	else if (PLAYER_SAID (R, explore_relationship))
	{
		SET_GAME_STATE (MELNORME_YACK_STACK3, 1);

		NPCPhrase (EXAMPLE_OF_RELATIONSHIP);
	}
	else if (PLAYER_SAID (R, excuse_1))
	{
		SET_GAME_STATE (MELNORME_YACK_STACK3, 2);

		NPCPhrase (NO_EXCUSE_1);
	}
	else if (PLAYER_SAID (R, excuse_2))
	{
		SET_GAME_STATE (MELNORME_YACK_STACK3, 3);

		NPCPhrase (NO_EXCUSE_2);
	}
	else if (PLAYER_SAID (R, excuse_3))
	{
		SET_GAME_STATE (MELNORME_YACK_STACK3, 4);

		NPCPhrase (NO_EXCUSE_3);
	}

	switch (GET_GAME_STATE (MELNORME_YACK_STACK3))
	{
		case 0:
			Response (explore_relationship, DoMelnormeMiffed);
			break;
		case 1:
			Response (excuse_1, DoMelnormeMiffed);
			break;
		case 2:
			Response (excuse_2, DoMelnormeMiffed);
			break;
		case 3:
			Response (excuse_3, DoMelnormeMiffed);
			break;
	}
	Response (we_apologize, DoFirstMeeting);
	Response (so_we_can_attack, ExitConversation);
	Response (bye_melnorme_slightly_angry, ExitConversation);
}

static void
DoMelnormePissed (RESPONSE_REF R)
{
	if (R == 0)
	{
		BYTE pissed_count;

		pissed_count = GET_GAME_STATE (MELNORME_PISSED_COUNT);
		switch (pissed_count++)
		{
			case 0:
				NPCPhrase (HELLO_PISSED_OFF_1);
				break;
			case 1:
				NPCPhrase (HELLO_PISSED_OFF_2);
				break;
			default:
				--pissed_count;
				NPCPhrase (HELLO_PISSED_OFF_3);
				break;
		}
		SET_GAME_STATE (MELNORME_PISSED_COUNT, pissed_count);

		AlienTalkSegue ((COUNT)~0);
		
		if (!IS_HD) {
			XFormColorMap (GetColorMapAddress (
 				SetAbsColorMapIndex (CommData.AlienColorMap, 2)
 				), ONE_SECOND / 2);
		} else {
			
			CommData.AlienAmbientArray[2].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[3].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[4].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[5].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[6].AnimFlags |= ANIM_DISABLED;
			CommData.AlienAmbientArray[7].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienAmbientArray[8].AnimFlags &= ~ANIM_DISABLED;
			CommData.AlienAmbientArray[9].AnimFlags &= ~ANIM_DISABLED;
			
			CommData.AlienFrame = SetAbsFrameIndex (CommData.AlienFrame, 48);
		} 
	}
	else if (PLAYER_SAID (R, beg_forgiveness))
	{
		SET_GAME_STATE (MELNORME_YACK_STACK4, 1);

		NPCPhrase (LOTS_TO_MAKE_UP_FOR);
	}
	else if (PLAYER_SAID (R, you_are_so_right))
	{
		SET_GAME_STATE (MELNORME_YACK_STACK4, 2);

		NPCPhrase (ONE_LAST_CHANCE);
	}

	switch (GET_GAME_STATE (MELNORME_YACK_STACK4))
	{
		case 0:
			Response (beg_forgiveness, DoMelnormePissed);
			break;
		case 1:
			Response (you_are_so_right, DoMelnormePissed);
			break;
		case 2:
			Response (ok_strip_me, ExitConversation);
			break;
	}
	Response (fight_some_more, ExitConversation);
	Response (bye_melnorme_pissed_off, ExitConversation);
}

static void
DoMelnormeHate (RESPONSE_REF R)
{
	BYTE hate_count;

	(void) R;  // ignored
	hate_count = GET_GAME_STATE (MELNORME_HATE_COUNT);
	switch (hate_count++)
	{
		case 0:
			NPCPhrase (HELLO_HATE_YOU_1);
			break;
		case 1:
			NPCPhrase (HELLO_HATE_YOU_2);
			break;
		default:
			--hate_count;
			NPCPhrase (HELLO_HATE_YOU_3);
			break;
	}
	SET_GAME_STATE (MELNORME_HATE_COUNT, hate_count);

	AlienTalkSegue ((COUNT)~0);
	
	if (!IS_HD) {
		XFormColorMap (GetColorMapAddress (
 			SetAbsColorMapIndex (CommData.AlienColorMap, 2)
 			), ONE_SECOND / 2);
	} else {
		
		CommData.AlienAmbientArray[2].AnimFlags |= ANIM_DISABLED;
		CommData.AlienAmbientArray[3].AnimFlags |= ANIM_DISABLED;
		CommData.AlienAmbientArray[4].AnimFlags |= ANIM_DISABLED;
		CommData.AlienAmbientArray[5].AnimFlags |= ANIM_DISABLED;
		CommData.AlienAmbientArray[6].AnimFlags |= ANIM_DISABLED;
		CommData.AlienAmbientArray[7].AnimFlags &= ~ANIM_DISABLED;
		CommData.AlienAmbientArray[8].AnimFlags &= ~ANIM_DISABLED;
		CommData.AlienAmbientArray[9].AnimFlags &= ~ANIM_DISABLED;

		CommData.AlienFrame = SetAbsFrameIndex (CommData.AlienFrame, 48);
	} 

	Response (well_if_thats_the_way_you_feel, ExitConversation);
	Response (you_hate_us_so_we_go_away, ExitConversation);
}

static void
Intro (void)
{
	prevMsgMode = SetStatusMessageMode (SMM_CREDITS);

	if (GET_GAME_STATE (MET_MELNORME) == 0)
	{
		SET_GAME_STATE (MET_MELNORME, 1);
		DoFirstMeeting (0);
	}
	else
	{
		switch (GET_GAME_STATE (MELNORME_ANGER))
		{
			case 0:
				if (GET_GAME_STATE (MELNORME_YACK_STACK2) <= 5)
					DoFirstMeeting (0);
				else
					NatureOfConversation (0);
				break;
			case 1:
				DoMelnormeMiffed (0);
				break;
			case 2:
				DoMelnormePissed (0);
				break;
			default:
				DoMelnormeHate (0);
				break;
		}
	}
}

static COUNT
uninit_melnorme (void)
{
	luaUqm_comm_uninit ();
	return 0;
}

static void
post_melnorme_enc (void)
{
	if (prevMsgMode != SMM_UNDEFINED)
		SetStatusMessageMode (prevMsgMode);
	DrawStatusMessage (NULL);
}

LOCDATA*
init_melnorme_comm (void)
{
	static LOCDATA melnorme_desc;
 	LOCDATA *retval;

	melnorme_desc = RES_BOOL(melnorme_desc_orig, melnorme_desc_hd);

	melnorme_desc.init_encounter_func = Intro;
	melnorme_desc.post_encounter_func = post_melnorme_enc;
	melnorme_desc.uninit_encounter_func = uninit_melnorme;

	luaUqm_comm_init (NULL, NULL_RESOURCE);
			// Initialise Lua for string interpolation. This will be
			// generalised in the future.

	melnorme_desc.AlienTextBaseline.x = TEXT_X_OFFS + (SIS_TEXT_WIDTH >> 1);
	melnorme_desc.AlienTextBaseline.y = 0;
	melnorme_desc.AlienTextWidth = SIS_TEXT_WIDTH - RES_SCALE(16);

	if (IS_HD) {
		melnorme_desc.AlienAmbientArray[2].AnimFlags &= ~ANIM_DISABLED;
		melnorme_desc.AlienAmbientArray[3].AnimFlags &= ~ANIM_DISABLED;
		melnorme_desc.AlienAmbientArray[4].AnimFlags |= ANIM_DISABLED;
		melnorme_desc.AlienAmbientArray[5].AnimFlags |= ANIM_DISABLED;
		melnorme_desc.AlienAmbientArray[6].AnimFlags |= ANIM_DISABLED;
		melnorme_desc.AlienAmbientArray[7].AnimFlags |= ANIM_DISABLED;
		melnorme_desc.AlienAmbientArray[8].AnimFlags |= ANIM_DISABLED;
		melnorme_desc.AlienAmbientArray[9].AnimFlags |= ANIM_DISABLED;
		melnorme_desc.AlienAmbientArray[10].AnimFlags |= ANIM_DISABLED;
	}

	local_stack0 = 0;
	local_stack1 = 0;

	prevMsgMode = SMM_UNDEFINED;

	setSegue (Segue_peace);
	AskedToBuy = FALSE;
	retval = &melnorme_desc;

	return (retval);
}
