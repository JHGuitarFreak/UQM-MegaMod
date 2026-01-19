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

#ifndef UQM_GLOBDATA_H_
#define UQM_GLOBDATA_H_

#include "clock.h"
#include "libs/gfxlib.h"
#include "libs/reslib.h"
#include "libs/sndlib.h"
#include "sis.h"
#include "velocity.h"
#include "commanim.h"
#include "lua/luastate.h"

#if defined(__cplusplus)
extern "C" {
#endif


// general numbers-speech generator info
// should accomodate most common base-10 languages
// many languages require various plural forms
// for digit names like "hundred"
// possibly needs reworking for others
typedef struct
{
	// an array of these structs must be in ascending remainder order
	// terminate the array with Divider == 0

	// digit divider, i.e. 1, 10, 100, etc.
	int Divider;
	// maximum remainder for this name
	// name will be used if Number % Divider <= MaxRemainder
	int MaxRemainder;
	// string table index for this name
	// i.e. "hundred" in English
	COUNT StrIndex;
} SPEECH_DIGITNAME;

typedef struct
{
	// digit divider, i.e. 1, 10, 100, etc.
	int Divider;
	// digit sub, i.e. 10 for teens
	// subtracted from the value to get an index into StrDigits
	int Subtrahend;
	// ptr to 10 indices for this digit
	// index is string table ptr when > 0
	//       is invalid (should not happen) or
	//       is a a 'skip digit' indicator when == 0 
	// StrDigits can be NULL, in which case
	// the value is interpreted recursively
	COUNT *StrDigits;
	// digit Names, can be NULL, in which case
	// CommonNameIndex is used
	SPEECH_DIGITNAME *Names;
	// common digit name string table index
	// i.e. "hundred" in English
	COUNT CommonNameIndex;
} SPEECH_DIGIT;

// this accomodates up to "billions" in english
#define MAX_SPEECH_DIGITS 7

typedef struct
{
	// slots used in Digits array
	COUNT NumDigits;
	// slots for each digit in numbers
	// teens is exception
	// 0-9, 10-19, ..20-90, ..100-900, etc.
	SPEECH_DIGIT Digits[MAX_SPEECH_DIGITS];
} NUMBER_SPEECH_DESC;
typedef const NUMBER_SPEECH_DESC *NUMBER_SPEECH;

typedef DWORD LDAS_FLAGS;
#define LDASF_NONE           ((LDAS_FLAGS)      0 )
#define LDASF_USE_ALTERNATE  ((LDAS_FLAGS)(1 << 0))

// BW: had to move that from commglue.h to here because now LOCDATA features it
typedef enum {
	ARILOU_CONVERSATION,
	CHMMR_CONVERSATION,
	COMMANDER_CONVERSATION,
	ORZ_CONVERSATION,
	PKUNK_CONVERSATION,
	SHOFIXTI_CONVERSATION,
	SPATHI_CONVERSATION,
	SUPOX_CONVERSATION,
	THRADD_CONVERSATION,
	UTWIG_CONVERSATION,
	VUX_CONVERSATION,
	YEHAT_CONVERSATION,
	MELNORME_CONVERSATION,
	DRUUGE_CONVERSATION,
	ILWRATH_CONVERSATION,
	MYCON_CONVERSATION,
	SLYLANDRO_CONVERSATION,
	UMGAH_CONVERSATION,
	URQUAN_CONVERSATION,
	ZOQFOTPIK_CONVERSATION,
	SYREEN_CONVERSATION,
	BLACKURQ_CONVERSATION,
	TALKING_PET_CONVERSATION,
	SLYLANDRO_HOME_CONVERSATION,
	URQUAN_DRONE_CONVERSATION,
	YEHAT_REBEL_CONVERSATION,
	INVALID_CONVERSATION,

	NUM_CONVERSATIONS,
} CONVERSATION;

typedef struct
{
	RESOURCE AlienFrameRes;
	RESOURCE AlienColorMapRes;
	RESOURCE AlienSongRes;
} ALT_RESOURCE;

typedef struct
{
	CONVERSATION AlienConv;
	void (*init_encounter_func) (void);
			/* Called when entering communications */
	void (*post_encounter_func) (void);
			/* Called when leaving communications or combat normally */
	COUNT (*uninit_encounter_func) (void);
			/* Called when encounter is done for cleanup */

	RESOURCE AlienFrameRes;
	RESOURCE AlienFontRes;
	Color AlienTextFColor, AlienTextBColor;
	POINT AlienTextBaseline;
	COUNT AlienTextWidth;
	TEXT_ALIGN AlienTextAlign;
	TEXT_VALIGN AlienTextValign;
	RESOURCE AlienColorMapRes;
	RESOURCE AlienSongRes;
	ALT_RESOURCE AltRes;

	RESOURCE ConversationPhrasesRes;

	COUNT NumAnimations;
	ANIMATION_DESC AlienAmbientArray[MAX_ANIMATIONS];

	// Transition animation to/from talking state;
	// the first frame is neutral (sort of like YOYO_ANIM)
	ANIMATION_DESC AlienTransitionDesc;
	// Talking animation, like RANDOM_ANIM, except random frames
	// always alternate with a neutral frame;
	// the first frame is neutral
	ANIMATION_DESC AlienTalkDesc;

	NUMBER_SPEECH AlienNumberSpeech;

	FRAME AlienFrame;
	FONT AlienFont;
	COLORMAP AlienColorMap;
	MUSIC_REF AlienSong;
	STRING ConversationPhrases;
} LOCDATA;

enum
{
	PORTAL_SPAWNER_DEVICE = 0,
	TALKING_PET_DEVICE,
	UTWIG_BOMB_DEVICE,
	SUN_EFFICIENCY_DEVICE,
	ROSY_SPHERE_DEVICE,
	AQUA_HELIX_DEVICE,
	CLEAR_SPINDLE_DEVICE,
	ULTRON_0_DEVICE,
	ULTRON_1_DEVICE,
	ULTRON_2_DEVICE,
	ULTRON_3_DEVICE,
	MAIDENS_DEVICE,
	UMGAH_HYPERWAVE_DEVICE,
	BURVIX_HYPERWAVE_DEVICE,
	DATA_PLATE_1_DEVICE,
	DATA_PLATE_2_DEVICE,
	DATA_PLATE_3_DEVICE,
	TAALO_PROTECTOR_DEVICE,
	EGG_CASING0_DEVICE,
	EGG_CASING1_DEVICE,
	EGG_CASING2_DEVICE,
	SYREEN_SHUTTLE_DEVICE,
	VUX_BEAST_DEVICE,
	DESTRUCT_CODE_DEVICE,
	URQUAN_WARP_DEVICE,
	WIMBLIS_TRIDENT_DEVICE,
	GLOWING_ROD_DEVICE,
	LUNAR_BASE_DEVICE,

	NUM_DEVICES
};

#define YEARS_TO_KOHRAH_VICTORY (optDeCleansing ? 100 : 4)
#define THRADDASH_BODY_THRESHOLD DIF_CASE(25, 15, 30)

// A structure describing how many bits are used for each game state value.
typedef struct GameStateBitMap GameStateBitMap;
struct GameStateBitMap {
	const char *name;
	BYTE numBits;
};

size_t totalBitsForGameState (const GameStateBitMap *bm, int rev);
int getGameStateRevByBytes (const GameStateBitMap *bm, int bytes);
BOOLEAN serialiseGameState (const GameStateBitMap *bm,
		BYTE **buf, size_t *numBytes);
BOOLEAN deserialiseGameState (const GameStateBitMap *bm,
		const BYTE *buf, size_t numBytes, int rev);

// Full dev comments preserved at the bottom of this file.
#define CORE_GAME_STATES(X) \
	X(SHOFIXTI_VISITS, 3) \
	X(SHOFIXTI_STACK1, 2) \
	X(SHOFIXTI_STACK2, 3) \
	X(SHOFIXTI_STACK3, 2) \
	X(SHOFIXTI_KIA, 1) \
	X(SHOFIXTI_BRO_KIA, 1) \
	X(SHOFIXTI_RECRUITED, 1) \
	X(SHOFIXTI_MAIDENS, 1) \
	X(MAIDENS_ON_SHIP, 1) \
	X(BATTLE_SEGUE, 1) \
	X(PLANETARY_LANDING, 1) \
	X(PLANETARY_CHANGE, 1) \
	X(SPATHI_VISITS, 3) \
	X(SPATHI_HOME_VISITS, 3) \
	X(FOUND_PLUTO_SPATHI, 2) \
	X(SPATHI_SHIELDED_SELVES, 1) \
	X(SPATHI_CREATURES_EXAMINED, 1) \
	X(SPATHI_CREATURES_ELIMINATED, 1) \
	X(UMGAH_BROADCASTERS, 1) \
	X(SPATHI_MANNER, 2) \
	X(SPATHI_QUEST, 1) \
	X(LIED_ABOUT_CREATURES, 2) \
	X(SPATHI_PARTY, 1) \
	X(KNOW_SPATHI_PASSWORD, 1) \
	X(ILWRATH_HOME_VISITS, 3) \
	X(ILWRATH_CHMMR_VISITS, 1) \
	X(ARILOU_SPACE, 1) \
	X(ARILOU_SPACE_SIDE, 2) \
	X(ARILOU_SPACE_COUNTER, 4) \
	X(LANDER_SHIELDS, 4) \
	X(MET_MELNORME, 1) \
	X(MELNORME_RESCUE_REFUSED, 1) \
	X(MELNORME_RESCUE_COUNT, 3) \
	X(TRADED_WITH_MELNORME, 1) \
	X(WHY_MELNORME_PURPLE, 1) \
	X(MELNORME_CREDIT0, 8) \
	X(MELNORME_CREDIT1, 8) \
	X(MELNORME_BUSINESS_COUNT, 2) \
	X(MELNORME_YACK_STACK0, 2) \
	X(MELNORME_YACK_STACK1, 2) \
	X(MELNORME_YACK_STACK2, 4) \
	X(MELNORME_YACK_STACK3, 3) \
	X(MELNORME_YACK_STACK4, 2) \
	X(WHY_MELNORME_BLUE, 1) \
	X(MELNORME_ANGER, 2) \
	X(MELNORME_MIFFED_COUNT, 2) \
	X(MELNORME_PISSED_COUNT, 2) \
	X(MELNORME_HATE_COUNT, 2) \
	X(PROBE_MESSAGE_DELIVERED, 1) \
	X(PROBE_ILWRATH_ENCOUNTER, 1) \
	X(STARBASE_AVAILABLE, 1) \
	X(STARBASE_VISITED, 1) \
	X(RADIOACTIVES_PROVIDED, 1) \
	X(LANDERS_LOST, 1) \
	X(GIVEN_FUEL_BEFORE, 1) \
	X(AWARE_OF_SAMATRA, 1) \
	X(YEHAT_CAVALRY_ARRIVED, 1) \
	X(URQUAN_MESSED_UP, 1) \
	X(MOONBASE_DESTROYED, 1) \
	X(WILL_DESTROY_BASE, 1) \
	X(WIMBLIS_TRIDENT_ON_SHIP, 1) \
	X(GLOWING_ROD_ON_SHIP, 1) \
	X(KOHR_AH_KILLED_ALL, 1) \
	X(STARBASE_YACK_STACK1, 1) \
	X(DISCUSSED_PORTAL_SPAWNER, 1) \
	X(DISCUSSED_TALKING_PET, 1) \
	X(DISCUSSED_UTWIG_BOMB, 1) \
	X(DISCUSSED_SUN_EFFICIENCY, 1) \
	X(DISCUSSED_ROSY_SPHERE, 1) \
	X(DISCUSSED_AQUA_HELIX, 1) \
	X(DISCUSSED_CLEAR_SPINDLE, 1) \
	X(DISCUSSED_ULTRON, 1) \
	X(DISCUSSED_MAIDENS, 1) \
	X(DISCUSSED_UMGAH_HYPERWAVE, 1) \
	X(DISCUSSED_BURVIX_HYPERWAVE, 1) \
	X(SYREEN_WANT_PROOF, 1) \
	X(PLAYER_HAVING_SEX, 1) \
	X(MET_ARILOU, 1) \
	X(DISCUSSED_TAALO_PROTECTOR, 1) \
	X(DISCUSSED_EGG_CASING0, 1) \
	X(DISCUSSED_EGG_CASING1, 1) \
	X(DISCUSSED_EGG_CASING2, 1) \
	X(DISCUSSED_SYREEN_SHUTTLE, 1) \
	X(DISCUSSED_VUX_BEAST, 1) \
	X(DISCUSSED_DESTRUCT_CODE, 1) \
	X(DISCUSSED_URQUAN_WARP, 1) \
	X(DISCUSSED_WIMBLIS_TRIDENT, 1) \
	X(DISCUSSED_GLOWING_ROD, 1) \
	X(ATTACKED_DRUUGE, 1) \
	X(NEW_ALLIANCE_NAME, 2) \
	X(PORTAL_COUNTER, 4) \
	X(BURVIXESE_BROADCASTERS, 1) \
	X(BURV_BROADCASTERS_ON_SHIP, 1) \
	X(UTWIG_BOMB, 1) \
	X(UTWIG_BOMB_ON_SHIP, 1) \
	X(AQUA_HELIX, 1) \
	X(AQUA_HELIX_ON_SHIP, 1) \
	X(SUN_DEVICE, 1) \
	X(SUN_DEVICE_ON_SHIP, 1) \
	X(TAALO_PROTECTOR, 1) \
	X(TAALO_PROTECTOR_ON_SHIP, 1) \
	X(SHIP_VAULT_UNLOCKED, 1) \
	X(SYREEN_SHUTTLE, 1) \
	X(PORTAL_KEY, 1) \
	X(PORTAL_KEY_ON_SHIP, 1) \
	X(VUX_BEAST, 1) \
	X(VUX_BEAST_ON_SHIP, 1) \
	X(TALKING_PET, 1) \
	X(TALKING_PET_ON_SHIP, 1) \
	X(MOONBASE_ON_SHIP, 1) \
	X(KOHR_AH_FRENZY, 1) \
	X(KOHR_AH_VISITS, 2) \
	X(KOHR_AH_BYES, 1) \
	X(SLYLANDRO_HOME_VISITS, 3) \
	X(DESTRUCT_CODE_ON_SHIP, 1) \
	X(ILWRATH_VISITS, 3) \
	X(ILWRATH_DECEIVED, 1) \
	X(FLAGSHIP_CLOAKED, 1) \
	X(MYCON_VISITS, 3) \
	X(MYCON_HOME_VISITS, 3) \
	X(MYCON_AMBUSH, 1) \
	X(MYCON_FELL_FOR_AMBUSH, 1) \
	X(GLOBAL_FLAGS_AND_DATA, 8) \
	X(ORZ_VISITS, 3) \
	X(TAALO_VISITS, 3) \
	X(ORZ_MANNER, 2) \
	X(PROBE_EXHIBITED_BUG, 1) \
	X(CLEAR_SPINDLE_ON_SHIP, 1) \
	X(URQUAN_VISITS, 3) \
	X(PLAYER_HYPNOTIZED, 1) \
	X(VUX_VISITS, 3) \
	X(VUX_HOME_VISITS, 3) \
	X(ZEX_VISITS, 3) \
	X(ZEX_IS_DEAD, 1) \
	X(KNOW_ZEX_WANTS_MONSTER, 1) \
	X(UTWIG_VISITS, 3) \
	X(UTWIG_HOME_VISITS, 3) \
	X(BOMB_VISITS, 3) \
	X(ULTRON_CONDITION, 3) \
	X(UTWIG_HAVE_ULTRON, 1) \
	X(BOMB_UNPROTECTED, 1) \
	X(TAALO_UNPROTECTED, 1) \
	X(TALKING_PET_VISITS, 3) \
	X(TALKING_PET_HOME_VISITS, 3) \
	X(UMGAH_ZOMBIE_BLOBBIES, 1) \
	X(KNOW_UMGAH_ZOMBIES, 1) \
	X(ARILOU_VISITS, 3) \
	X(ARILOU_HOME_VISITS, 3) \
	X(KNOW_ARILOU_WANT_WRECK, 1) \
	X(ARILOU_CHECKED_UMGAH, 2) \
	X(PORTAL_SPAWNER, 1) \
	X(PORTAL_SPAWNER_ON_SHIP, 1) \
	X(UMGAH_VISITS, 3) \
	X(UMGAH_HOME_VISITS, 3) \
	X(MET_NORMAL_UMGAH, 1) \
	X(SYREEN_HOME_VISITS, 3) \
	X(SYREEN_SHUTTLE_ON_SHIP, 1) \
	X(KNOW_SYREEN_VAULT, 1) \
	X(EGG_CASE0_ON_SHIP, 1) \
	X(SUN_DEVICE_UNGUARDED, 1) \
	X(ROSY_SPHERE_ON_SHIP, 1) \
	X(CHMMR_HOME_VISITS, 3) \
	X(CHMMR_EMERGING, 1) \
	X(CHMMR_UNLEASHED, 1) \
	X(CHMMR_BOMB_STATE, 2) \
	X(DRUUGE_DISCLAIMER, 1) \
	X(YEHAT_VISITS, 3) \
	X(YEHAT_REBEL_VISITS, 3) \
	X(YEHAT_HOME_VISITS, 3) \
	X(YEHAT_CIVIL_WAR, 1) \
	X(YEHAT_ABSORBED_PKUNK, 1) \
	X(YEHAT_SHIP_MONTH, 4) \
	X(YEHAT_SHIP_DAY, 5) \
	X(YEHAT_SHIP_YEAR, 5) \
	X(CLEAR_SPINDLE, 1) \
	X(PKUNK_VISITS, 3) \
	X(PKUNK_HOME_VISITS, 3) \
	X(PKUNK_SHIP_MONTH, 4) \
	X(PKUNK_SHIP_DAY, 5) \
	X(PKUNK_SHIP_YEAR, 5) \
	X(PKUNK_MISSION, 3) \
	X(SUPOX_VISITS, 3) \
	X(SUPOX_HOME_VISITS, 3) \
	X(THRADD_VISITS, 3) \
	X(THRADD_HOME_VISITS, 3) \
	X(HELIX_VISITS, 3) \
	X(HELIX_UNPROTECTED, 1) \
	X(THRADD_CULTURE, 2) \
	X(THRADD_MISSION, 3) \
	X(DRUUGE_VISITS, 3) \
	X(DRUUGE_HOME_VISITS, 3) \
	X(ROSY_SPHERE, 1) \
	X(SCANNED_MAIDENS, 1) \
	X(SCANNED_FRAGMENTS, 1) \
	X(SCANNED_CASTER, 1) \
	X(SCANNED_SPAWNER, 1) \
	X(SCANNED_ULTRON, 1) \
	X(ZOQFOT_INFO, 2) \
	X(ZOQFOT_HOSTILE, 1) \
	X(ZOQFOT_HOME_VISITS, 3) \
	X(MET_ZOQFOT, 1) \
	X(ZOQFOT_DISTRESS, 2) \
	X(EGG_CASE1_ON_SHIP, 1) \
	X(EGG_CASE2_ON_SHIP, 1) \
	X(MYCON_SUN_VISITS, 3) \
	X(ORZ_HOME_VISITS, 3) \
	X(MELNORME_FUEL_PROCEDURE, 1) \
	X(MELNORME_TECH_PROCEDURE, 1) \
	X(MELNORME_INFO_PROCEDURE, 1) \
	X(MELNORME_TECH_STACK, 4) \
	X(MELNORME_EVENTS_INFO_STACK, 5) \
	X(MELNORME_ALIEN_INFO_STACK, 5) \
	X(MELNORME_HISTORY_INFO_STACK, 5) \
	X(RAINBOW_WORLD0, 8) \
	X(RAINBOW_WORLD1, 2) \
	X(MELNORME_RAINBOW_COUNT, 4) \
	X(USED_BROADCASTER, 1) \
	X(BROADCASTER_RESPONSE, 1) \
	X(IMPROVED_LANDER_SPEED, 1) \
	X(IMPROVED_LANDER_CARGO, 1) \
	X(IMPROVED_LANDER_SHOT, 1) \
	X(MET_ORZ_BEFORE, 1) \
	X(YEHAT_REBEL_TOLD_PKUNK, 1) \
	X(PLAYER_HAD_SEX, 1) \
	X(UMGAH_BROADCASTERS_ON_SHIP, 1) \
	X(LIGHT_MINERAL_LOAD, 3) \
	X(MEDIUM_MINERAL_LOAD, 3) \
	X(HEAVY_MINERAL_LOAD, 3) \
	X(STARBASE_BULLETS, 32) \
	X(STARBASE_MONTH, 4) \
	X(STARBASE_DAY, 5) \
	X(CREW_SOLD_TO_DRUUGE0, 8) \
	X(CREW_PURCHASED0, 8) \
	X(CREW_PURCHASED1, 8) \
	X(URQUAN_PROTECTING_SAMATRA, 1) \
	X(THRADDASH_BODY_COUNT, 5) \
	X(UTWIG_SUPOX_MISSION, 3) \
	X(SPATHI_INFO, 3) \
	X(ILWRATH_INFO, 2) \
	X(ILWRATH_GODS_SPOKEN, 4) \
	X(ILWRATH_WORSHIP, 2) \
	X(ILWRATH_FIGHT_THRADDASH, 1) \
	X(READY_TO_CONFUSE_URQUAN, 1) \
	X(URQUAN_HYPNO_VISITS, 1) \
	X(MENTIONED_PET_COMPULSION, 1) \
	X(URQUAN_INFO, 2) \
	X(KNOW_URQUAN_STORY, 2) \
	X(MYCON_INFO, 4) \
	X(MYCON_RAMBLE, 5) \
	X(KNOW_ABOUT_SHATTERED, 2) \
	X(MYCON_INSULTS, 3) \
	X(MYCON_KNOW_AMBUSH, 1) \
	X(SYREEN_INFO, 2) \
	X(KNOW_SYREEN_WORLD_SHATTERED, 1) \
	X(SYREEN_KNOW_ABOUT_MYCON, 1) \
	X(TALKING_PET_INFO, 3) \
	X(TALKING_PET_SUGGESTIONS, 3) \
	X(LEARNED_TALKING_PET, 1) \
	X(DNYARRI_LIED, 1) \
	X(SHIP_TO_COMPEL, 1) \
	X(ORZ_GENERAL_INFO, 2) \
	X(ORZ_PERSONAL_INFO, 3) \
	X(ORZ_ANDRO_STATE, 2) \
	X(REFUSED_ORZ_ALLIANCE, 1) \
	X(PKUNK_MANNER, 2) \
	X(PKUNK_ON_THE_MOVE, 1) \
	X(PKUNK_FLEET, 2) \
	X(PKUNK_MIGRATE, 2) \
	X(PKUNK_RETURN, 1) \
	X(PKUNK_WORRY, 2) \
	X(PKUNK_INFO, 3) \
	X(PKUNK_WAR, 2) \
	X(PKUNK_FORTUNE, 3) \
	X(PKUNK_MIGRATE_VISITS, 3) \
	X(PKUNK_REASONS, 4) \
	X(PKUNK_SWITCH, 1) \
	X(PKUNK_SENSE_VICTOR, 1) \
	X(KOHR_AH_REASONS, 2) \
	X(KOHR_AH_PLEAD, 2) \
	X(KOHR_AH_INFO, 2) \
	X(KNOW_KOHR_AH_STORY, 2) \
	X(KOHR_AH_SENSES_EVIL, 1) \
	X(URQUAN_SENSES_EVIL, 1) \
	X(SLYLANDRO_PROBE_VISITS, 3) \
	X(SLYLANDRO_PROBE_THREAT, 2) \
	X(SLYLANDRO_PROBE_WRONG, 2) \
	X(SLYLANDRO_PROBE_ID, 2) \
	X(SLYLANDRO_PROBE_INFO, 2) \
	X(SLYLANDRO_PROBE_EXIT, 2) \
	X(UMGAH_HOSTILE, 1) \
	X(UMGAH_EVIL_BLOBBIES, 1) \
	X(UMGAH_MENTIONED_TRICKS, 2) \
	X(BOMB_CARRIER, 1) \
	X(THRADD_MANNER, 1) \
	X(THRADD_INTRO, 2) \
	X(THRADD_DEMEANOR, 3) \
	X(THRADD_INFO, 2) \
	X(THRADD_BODY_LEVEL, 2) \
	X(THRADD_MISSION_VISITS, 1) \
	X(THRADD_STACK_1, 3) \
	X(THRADD_HOSTILE_STACK_2, 1) \
	X(THRADD_HOSTILE_STACK_3, 1) \
	X(THRADD_HOSTILE_STACK_4, 1) \
	X(THRADD_HOSTILE_STACK_5, 1) \
	X(CHMMR_STACK, 2) \
	X(ARILOU_MANNER, 2) \
	X(NO_PORTAL_VISITS, 1) \
	X(ARILOU_STACK_1, 2) \
	X(ARILOU_STACK_2, 1) \
	X(ARILOU_STACK_3, 2) \
	X(ARILOU_STACK_4, 1) \
	X(ARILOU_STACK_5, 2) \
	X(ARILOU_INFO, 2) \
	X(ARILOU_HINTS, 2) \
	X(DRUUGE_MANNER, 1) \
	X(DRUUGE_SPACE_INFO, 2) \
	X(DRUUGE_HOME_INFO, 2) \
	X(DRUUGE_SALVAGE, 1) \
	X(KNOW_DRUUGE_SLAVERS, 2) \
	X(FRAGMENTS_BOUGHT, 2) \
	X(ZEX_STACK_1, 2) \
	X(ZEX_STACK_2, 2) \
	X(ZEX_STACK_3, 2) \
	X(VUX_INFO, 2) \
	X(VUX_STACK_1, 4) \
	X(VUX_STACK_2, 2) \
	X(VUX_STACK_3, 2) \
	X(VUX_STACK_4, 2) \
	X(SHOFIXTI_STACK4, 2) \
	X(YEHAT_REBEL_INFO, 3) \
	X(YEHAT_ROYALIST_INFO, 1) \
	X(YEHAT_ROYALIST_TOLD_PKUNK, 1) \
	X(NO_YEHAT_ALLY_HOME, 1) \
	X(NO_YEHAT_HELP_HOME, 1) \
	X(NO_YEHAT_INFO, 1) \
	X(NO_YEHAT_ALLY_SPACE, 2) \
	X(NO_YEHAT_HELP_SPACE, 2) \
	X(ZOQFOT_KNOW_MASK, 4) \
	X(SUPOX_HOSTILE, 1) \
	X(SUPOX_INFO, 1) \
	X(SUPOX_WAR_NEWS, 2) \
	X(SUPOX_ULTRON_HELP, 1) \
	X(SUPOX_STACK1, 3) \
	X(SUPOX_STACK2, 2) \
	X(UTWIG_HOSTILE, 1) \
	X(UTWIG_INFO, 1) \
	X(UTWIG_WAR_NEWS, 2) \
	X(UTWIG_STACK1, 3) \
	X(UTWIG_STACK2, 2) \
	X(BOMB_INFO, 1) \
	X(BOMB_STACK1, 2) \
	X(BOMB_STACK2, 2) \
	X(SLYLANDRO_KNOW_BROKEN, 1) \
	X(PLAYER_KNOWS_PROBE, 1) \
	X(PLAYER_KNOWS_PROGRAM, 1) \
	X(PLAYER_KNOWS_EFFECTS, 1) \
	X(PLAYER_KNOWS_PRIORITY, 1) \
	X(SLYLANDRO_STACK1, 3) \
	X(SLYLANDRO_STACK2, 1) \
	X(SLYLANDRO_STACK3, 2) \
	X(SLYLANDRO_STACK4, 2) \
	X(SLYLANDRO_STACK5, 1) \
	X(SLYLANDRO_STACK6, 1) \
	X(SLYLANDRO_STACK7, 2) \
	X(SLYLANDRO_STACK8, 2) \
	X(SLYLANDRO_STACK9, 2) \
	X(SLYLANDRO_KNOW_EARTH, 1) \
	X(SLYLANDRO_KNOW_EXPLORE, 1) \
	X(SLYLANDRO_KNOW_GATHER, 1) \
	X(SLYLANDRO_KNOW_URQUAN, 2) \
	X(RECALL_VISITS, 2) \
	X(SLYLANDRO_MULTIPLIER, 3) \
	X(KNOW_SPATHI_QUEST, 1) \
	X(KNOW_SPATHI_EVIL, 1) \
	X(BATTLE_PLANET, 8) \
	X(ESCAPE_COUNTER, 8) \
	X(CREW_SOLD_TO_DRUUGE1, 8) \
	X(PKUNK_DONE_WAR, 1) \
	X(SYREEN_STACK0, 2) \
	X(SYREEN_STACK1, 2) \
	X(SYREEN_STACK2, 2) \
	X(REFUSED_ULTRON_AT_BOMB, 1) \
	X(NO_TRICK_AT_SUN, 1) \
	X(SPATHI_STACK0, 2) \
	X(SPATHI_STACK1, 1) \
	X(SPATHI_STACK2, 1) \
	X(ORZ_STACK0, 1) \
	X(ORZ_STACK1, 1) \
	X(SHOFIXTI_GRPOFFS, 32) \
	X(ZOQFOT_GRPOFFS, 32) \
	X(MELNORME0_GRPOFFS, 32) \
	X(MELNORME1_GRPOFFS, 32) \
	X(MELNORME2_GRPOFFS, 32) \
	X(MELNORME3_GRPOFFS, 32) \
	X(MELNORME4_GRPOFFS, 32) \
	X(MELNORME5_GRPOFFS, 32) \
	X(MELNORME6_GRPOFFS, 32) \
	X(MELNORME7_GRPOFFS, 32) \
	X(MELNORME8_GRPOFFS, 32) \
	X(URQUAN_PROBE_GRPOFFS, 32) \
	X(COLONY_GRPOFFS, 32) \
	X(SAMATRA_GRPOFFS, 32)

// Revision 1: MegaMod v0.8.0.85
#define REV1_GAME_STATES(X) \
	X(AUTOPILOT_OK, 1) \
	X(KNOW_QS_PORTAL, 16)

// Revision 2: MegaMod v0.8.1
#define REV2_GAME_STATES(X) \
	X(SYS_VISITED_00, 32) \
	X(SYS_VISITED_01, 32) \
	X(SYS_VISITED_02, 32) \
	X(SYS_VISITED_03, 32) \
	X(SYS_VISITED_04, 32) \
	X(SYS_VISITED_05, 32) \
	X(SYS_VISITED_06, 32) \
	X(SYS_VISITED_07, 32) \
	X(SYS_VISITED_08, 32) \
	X(SYS_VISITED_09, 32) \
	X(SYS_VISITED_10, 32) \
	X(SYS_VISITED_11, 32) \
	X(SYS_VISITED_12, 32) \
	X(SYS_VISITED_13, 32) \
	X(SYS_VISITED_14, 32) \
	X(SYS_VISITED_15, 32) \
	X(KNOW_HOMEWORLD, 18) \
	X(HM_ENCOUNTERS, 9) \
	X(RESERVED, 32)

// Revision 3: MegaMod v0.8.2
#define REV3_GAME_STATES(X) \
	X(SYS_PLYR_MARKER_00, 32) \
	X(SYS_PLYR_MARKER_01, 32) \
	X(SYS_PLYR_MARKER_02, 32) \
	X(SYS_PLYR_MARKER_03, 32) \
	X(SYS_PLYR_MARKER_04, 32) \
	X(SYS_PLYR_MARKER_05, 32) \
	X(SYS_PLYR_MARKER_06, 32) \
	X(SYS_PLYR_MARKER_07, 32) \
	X(SYS_PLYR_MARKER_08, 32) \
	X(SYS_PLYR_MARKER_09, 32) \
	X(SYS_PLYR_MARKER_10, 32) \
	X(SYS_PLYR_MARKER_11, 32) \
	X(SYS_PLYR_MARKER_12, 32) \
	X(SYS_PLYR_MARKER_13, 32) \
	X(SYS_PLYR_MARKER_14, 32) \
	X(SYS_PLYR_MARKER_15, 32) \
	X(LAST_LOCATION_X, 16) \
	X(LAST_LOCATION_Y, 16)

// Revision 4: MegaMod v0.8.3
#define REV4_GAME_STATES(X) \
	X(ADV_AUTOPILOT_SAVE_X, 16) \
	X(ADV_AUTOPILOT_SAVE_Y, 16) \
	X(ADV_AUTOPILOT_QUASI_X, 16) \
	X(ADV_AUTOPILOT_QUASI_Y, 16)

// Revision 5: MegaMod v0.8.4
#define REV5_GAME_STATES(X) \
	X(SEED_TYPE, 2) \
	X(SUPOX_SHIP_MONTH, 4) \
	X(SUPOX_SHIP_DAY, 5) \
	X(SUPOX_SHIP_YEAR, 5) \
	X(UTWIG_SHIP_MONTH, 4) \
	X(UTWIG_SHIP_DAY, 5) \
	X(UTWIG_SHIP_YEAR, 5) \
	X(ALLIANCE_MASK, 6) \
	X(HIERARCHY_MASK, 7) \
	X(HEARD_PKUNK_ILWRATH, 1) \
	X(HAYES_OTHER_ALIENS, 1) \
	X(INVESTIGATE_THRADD, 1) \
	X(INVESTIGATE_UMGAH, 1) \
	X(INVESTIGATE_UMGAH_ZFP, 1) \
	X(INVESTIGATE_PROBES, 2) \
	X(INVESTIGATE_PORTAL, 3) \
	X(INVESTIGATE_ORZ, 1) \
	X(INVESTIGATE_ZFP, 1) \
	X(MET_ZFP_HOME, 1)

#define GAME_STATE_ENUM_ENTRY(name, bits) name, END_##name = name + bits - 1,
enum
{
	CORE_GAME_STATES (GAME_STATE_ENUM_ENTRY) // Core v0.8.0
	REV1_GAME_STATES (GAME_STATE_ENUM_ENTRY) // MegaMod v0.8.0.85
	REV2_GAME_STATES (GAME_STATE_ENUM_ENTRY) // MegaMod v0.8.1
	REV3_GAME_STATES (GAME_STATE_ENUM_ENTRY) // MegaMod v0.8.2
	REV4_GAME_STATES (GAME_STATE_ENUM_ENTRY) // MegaMod v0.8.3
	REV5_GAME_STATES (GAME_STATE_ENUM_ENTRY) // MegaMod v0.8.4

	NUM_GAME_STATE_BITS
};
#undef GAME_STATE_ENUM_ENTRY

// Values for GAME_STATE.glob_flags:
#define READ_SPEED_MASK ((1 << 3) - 1)
#define NUM_READ_SPEEDS 5
#define COMBAT_SPEED_SHIFT 6
#define COMBAT_SPEED_MASK (((1 << 2) - 1) << COMBAT_SPEED_SHIFT)
#define NUM_COMBAT_SPEEDS 4

#define MUSIC_DISABLED (1 << 3)
#define SOUND_DISABLED (1 << 4)
#define CYBORG_ENABLED (1 << 5)

enum
{
	SUPER_MELEE = 0, /* Is also used while in the main menu */
	IN_LAST_BATTLE,
	IN_ENCOUNTER,
	IN_HYPERSPACE /* in HyperSpace or QuasiSpace */,
	IN_INTERPLANETARY,
	WON_LAST_BATTLE,

	/* The following three are only used when displaying save game
	 * summaries */
	IN_QUASISPACE,
	IN_PLANET_ORBIT,
	IN_STARBASE,

	CHECK_PAUSE = MAKE_WORD (0, (1 << 0)),
	IN_BATTLE = MAKE_WORD (0, (1 << 1)),
			/* Is also set while in HyperSpace/QuasiSpace */
	START_ENCOUNTER = MAKE_WORD (0, (1 << 2)),
	START_INTERPLANETARY = MAKE_WORD (0, (1 << 3)),
	CHECK_LOAD = MAKE_WORD (0, (1 << 4)),
	CHECK_RESTART = MAKE_WORD (0, (1 << 5)),
	CHECK_ABORT = MAKE_WORD (0, (1 << 6)),
};
typedef UWORD ACTIVITY;

typedef struct
{
	BYTE glob_flags;
			// See above for the meaning of the bits.

	BYTE CrewCost, FuelCost;
	BYTE ModuleCost[NUM_MODULES];
	BYTE ElementWorth[NUM_ELEMENT_CATEGORIES];

	PRIMITIVE *DisplayArray;
	ACTIVITY CurrentActivity;

	CLOCK_STATE GameClock;

	POINT autopilot;
	POINT ip_location;
	STAMP ShipStamp;
	UWORD ShipFacing;
	BYTE ip_planet;
	BYTE in_orbit;
	VELOCITY_DESC velocity;

	DWORD BattleGroupRef;
	QUEUE avail_race_q;
			/* List of all the races in the game with information
			 * about their ships, and what player knows about their
			 * fleet, center of SoI, status, etc.
			 * queue element is FLEET_INFO */
	QUEUE npc_built_ship_q;
			/* Non-player-character list of ships (during encounter)
			 * queue element is SHIP_FRAGMENT */
	QUEUE ip_group_q;
			/* List of groups present in solarsys (during IP);
			 * queue element is IP_GROUP */
	QUEUE encounter_q;
			/* List of HyperSpace encounters (black globes);
			 * queue element is ENCOUNTER */
	QUEUE built_ship_q;
			/* List of SIS escort ships;
			 * queue element is SHIP_FRAGMENT */
	QUEUE stowed_ship_q;
			/* List of escort ships available at starbase;
			 * queue element is SHIP_FRAGMENT */
} GAME_STATE;

typedef struct
{
	SIS_STATE SIS_state;
	GAME_STATE Game_state;
} GLOBDATA;

extern GLOBDATA GlobData;
#define GLOBAL(f) GlobData.Game_state.f
#define GLOBAL_SIS(f) GlobData.SIS_state.f

#define MAX_ENCOUNTERS  16
#define MAX_BATTLE_GROUPS 32

/* DEFGRP enumeration. These identify scripted TrueSpace encounters
 * more consistently than offsets into the DEFGRPINFO_FILE state
 * file. */
enum {
	DEFGRP_NONE,
	DEFGRP_SHOFIXTI,
	DEFGRP_ZOQFOT,
	DEFGRP_MELNORME0,
	DEFGRP_MELNORME1,
	DEFGRP_MELNORME2,
	DEFGRP_MELNORME3,
	DEFGRP_MELNORME4,
	DEFGRP_MELNORME5,
	DEFGRP_MELNORME6,
	DEFGRP_MELNORME7,
	DEFGRP_MELNORME8,
	DEFGRP_URQUAN_PROBE,
	DEFGRP_COLONY,
	DEFGRP_SAMATRA,
	NUM_DEFGRPS
};

//#define STATE_DEBUG

#define SET_GAME_STATE(SName, val) \
		setGameStateUint (#SName, (val))
#define GET_GAME_STATE(SName) \
		getGameStateUint (#SName)

// For dynamic variable names
#define D_SET_GAME_STATE(SName, val) \
		setGameStateUint (SName, (val))
#define D_GET_GAME_STATE(SName) \
		getGameStateUint (SName)

static inline void
BitMaskGameState (const char *state_name, int bit_to_shift)
{
	BYTE gs = D_GET_GAME_STATE (state_name);
	gs |= (1 << bit_to_shift);
	D_SET_GAME_STATE (state_name, gs);
}
#define BM_GAME_STATE(SName, val) \
		BitMaskGameState (#SName, (val))

extern CONTEXT RadarContext;

extern void FreeSC2Data (void);
extern BOOLEAN LoadSC2Data (void);

extern void InitGlobData (void);
extern BOOLEAN InitStarseed (BOOLEAN newgame);

BOOLEAN inFullGame (void);
BOOLEAN inEncounter (void);
BOOLEAN inSuperMelee (void);
//BOOLEAN inBattle (void);
//BOOLEAN inInterPlanetary (void);
//BOOLEAN inSolarSystem (void);
//BOOLEAN inOrbit (void);
BOOLEAN inHQSpace (void);
BOOLEAN inHyperSpace (void);
BOOLEAN inQuasiSpace (void);
OPT_CONSOLETYPE isPC (int optWhich);
OPT_CONSOLETYPE is3DO (int optWhich);
extern int replaceChar (char *pStr, const char find, const char replace);

extern void LoadFleetInfo (void);
extern BOOLEAN InitGameStructures (void);
extern void UninitGameStructures (void);

// Difficulty
#define NORM 0
#define EASY 1
#define HARD 2
#define DIFFICULTY (GLOBAL_SIS (Difficulty) ? GLOBAL_SIS (Difficulty) : NORM)
#define DIF_CASE(a,b,c) (DIFFICULTY == NORM ? (a) : (DIFFICULTY == EASY ? (b) : (c)))
#define DIF_NORM (DIFFICULTY == NORM ? true : false)
#define DIF_EASY (DIFFICULTY == EASY ? true : false)
#define DIF_HARD (DIFFICULTY == HARD ? true : false)
#define IF_NORM(a,b) (!DIF_NORM ? (a) : (b))
#define IF_EASY(a,b) (!DIF_EASY ? (a) : (b))
#define IF_HARD(a,b) (!DIF_HARD ? (a) : (b))
#define DIF_STR(a) ((a) == NORM ? "Normal" : ((a) == EASY ? "Easy" : (a) == HARD ? "Hard" : "CYO"))

// Extended
#define EXTENDED (GLOBAL_SIS (Extended) ? TRUE : FALSE)
#define EXT_CASE(a,b) (!EXTENDED ? (a) : (b))

// Nomad
#define NOMAD (GLOBAL_SIS (Nomad) ? TRUE : FALSE)
#define NOMAD_DIF(a) (GLOBAL_SIS (Nomad) == (a) ? TRUE : FALSE)
#define NOMAD_STR(a) ((a) == 2 ? "Normal" : ((a) == 1 ? "Easy" : "Off"))

// Storage Queue
#define STORAGE_Q (optShipStore || DIF_HARD || optFleetPointSys)

static inline POINT
LoadLastLoc (void)
{
	return (POINT) { GET_GAME_STATE (LAST_LOCATION_X),
			GET_GAME_STATE (LAST_LOCATION_Y) };
}

static inline void
SaveLastLoc (POINT pt)
{
	SET_GAME_STATE (LAST_LOCATION_X, pt.x);
	SET_GAME_STATE (LAST_LOCATION_Y, pt.y);
}

static inline void
ZeroLastLoc (void)
{
	SET_GAME_STATE (LAST_LOCATION_X, ~0);
	SET_GAME_STATE (LAST_LOCATION_Y, ~0);
}

static inline POINT
LoadAdvancedQuasiPilot (void)
{
	return (POINT) { GET_GAME_STATE (ADV_AUTOPILOT_QUASI_X),
			GET_GAME_STATE (ADV_AUTOPILOT_QUASI_Y) };
}

static inline POINT
LoadAdvancedAutoPilot (void)
{
	return (POINT) { GET_GAME_STATE (ADV_AUTOPILOT_SAVE_X),
			GET_GAME_STATE (ADV_AUTOPILOT_SAVE_Y) };
}

static inline void
SaveAdvancedQuasiPilot (POINT pt)
{
	SET_GAME_STATE (ADV_AUTOPILOT_QUASI_X, pt.x);
	SET_GAME_STATE (ADV_AUTOPILOT_QUASI_Y, pt.y);
}

static inline void
SaveAdvancedAutoPilot (POINT pt)
{
	SET_GAME_STATE (ADV_AUTOPILOT_SAVE_X, pt.x);
	SET_GAME_STATE (ADV_AUTOPILOT_SAVE_Y, pt.y);
}

static inline void
ZeroAdvancedQuasiPilot (void)
{
	SET_GAME_STATE (ADV_AUTOPILOT_QUASI_X, ~0);
	SET_GAME_STATE (ADV_AUTOPILOT_QUASI_Y, ~0);
}

static inline void
ZeroAdvancedAutoPilot (void)
{
	SET_GAME_STATE (ADV_AUTOPILOT_SAVE_X, ~0);
	SET_GAME_STATE (ADV_AUTOPILOT_SAVE_Y, ~0);
}

#define BOOL_STR(a) ((a) ? "True" : "False")

// Earth Coordinates
#define EARTH_OUTER_X -725
#define EARTH_OUTER_Y 597

// Druuge Crew Values
#define MIN_SOLD DIF_CASE(100, 200, 10)
#define MAX_SOLD DIF_CASE(250, 500, 25)

static inline BOOLEAN
IsHomeworldKnown (DWORD homeworld)
{
	if (homeworld > 18)
		return FALSE;

	return (GET_GAME_STATE (KNOW_HOMEWORLD) & (1 << homeworld)) != 0;
}

static inline void
SetHomeworldKnown (DWORD homeworld)
{
	DWORD current;

	if (IsHomeworldKnown (homeworld))
		return;

	current = GET_GAME_STATE (KNOW_HOMEWORLD);
	current |= 1 << homeworld;
	SET_GAME_STATE (KNOW_HOMEWORLD, current);
}

#if defined(__cplusplus)
}
#endif

#endif /* UQM_GLOBDATA_H_ */

/*
 * GAME STATE COMMENTS DOCUMENTATION
 * ==================================
 * These game state comments are preserved from the original globdata.h and
 * save.c files for reference purposes only.
 *
 * Core UQM v0.8.0 states:
 * ------------------------------------
 * SHOFIXTI_MAIDENS:
 *     Did you find the babes yet?
 *
 * BATTLE_SEGUE:
 *     Set to 0 in init_xxx_comm() if communications directly
 *     follows an encounter. Set to 1 in init_xxx_comm() if the
 *     player gets to decide whether to attack or talk. Set to 1
 *     in communication when battle follows the communication. It
 *     is still valid when uninit_xxx_comm() gets called after
 *     combat or communication.
 *
 * PLANETARY_CHANGE:
 *     Flag set to 1 when the planet information for the current
 *     world is changed since it was last saved to the starinfo.dat
 *     file. Set when picking up bio, mineral, or energy nodes.
 *     When there's no current world, it should be 0.
 *
 * FOUND_PLUTO_SPATHI:
 *     0 - Haven't met Fwiffo.
 *     1 - Met Fwiffo on Pluto, now talking to him.
 *     2 - Met Fwiffo on Pluto, after dialog.
 *     3 - Met Fwiffo, and have reported to the Safe Ones on the
 *         Spathi moon that he was either killed, or that you have
 *         him on board.
 *
 * ARILOU_SPACE:
 *     0 if the periodically opening QuasiSpace portal is closed
 *        or closing.
 *     1 if the periodically opening QuasiSpace portal is open or
 *        opening.
 *
 * ARILOU_SPACE_SIDE:
 *     0 if in HyperSpace and not just emerged from the
 *        periodically opening QuasiSpace portal.
 *     1 if in HyperSpace and just emerged from the periodically
 *        QuasiSpace portal (still on portal).
 *     2 if in QuasiSpace and just emerged from the periodically
 *        opening portal (still on portal).
 *     3 if in QuasiSpace and not just emerged from the
 *        periodically opening portal.
 *
 * ARILOU_SPACE_COUNTER:
 *     Keeps track of how far the periodically opening QuasiSpace
 *     portal is open. (This determines the image)
 *     0 <= ARILOU_SPACE_COUNTER <= 9
 *     0 means totally closed.
 *     9 means completely open.
 *
 * PORTAL_COUNTER:
 *     Set to 1 when the player opens a QuasiSpace portal. It will
 *     then be increased to 10, at which time the portal is
 *     completely open. (This determines the image).
 *
 * ULTRON_CONDITION:
 *     0 if the Supox still have the Ultron
 *     1 if the Captain has the Ultron, completely
 *         broken
 *     2 if the Captain has the Ultron, with 1 fix
 *     3 if the Captain has the Ultron, with 2 fixes
 *     4 if the Captain has the Ultron, completely
 *         restored
 *     5 if the Ultron has been returned to the
 *         Utwig
 *
 * ROSY_SPHERE_ON_SHIP:
 *     The Rosy Sphere is aboard the flagship, i.e. It has been
 *     acquired from the Druuge, but not yet inserted in the
 *     broken Ultron. cf. ROSY_SPHERE
 *
 * CHMMR_BOMB_STATE:
 *     0 - Nothing is known about the Precursor Bomb.
 *     1 - The captain knows from the Chmmr that some extremely
 *         powerful weapon is needed to destroy the Sa-Matra.
 *     2 - Installation of the precursor bomb has started.
 *     3 - Left the starbase after installation of the Precursor
 *         bomb.
 *
 * ROSY_SPHERE:
 *     The play has or has had the Rosy Sphere.
 *     cf. ROSY_SHERE_ON_SHIP
 *
 * ZOQFOT_DISTRESS:
 *     0 if the Zoq-Fot-Pik aren't in distress
 *     1 if the Zoq-Fot-Pik are under attack by the Kohr-Ah
 *     2 if the Zoq-Fot-Pik have been destroyed because of this
 *        attack (not by the Kohr-Ah final victory cleansing)
 *
 * MELNORME_TECH_STACK:
 *     MELNORME_TECH_STACK is now unused
 *
 * RAINBOW_WORLD0:
 *     Low byte of a bit array, one bit per rainbow world. Each
 *     bit is set if the rainbow world has been visited. The
 *     lowest bit is for the first star in the star_array with
 *     RAINBOW_DEFINED, and so on.
 *
 * RAINBOW_WORLD1:
 *     High 2 bits of the bit array of which RAINBOW_WORLD0 is the
 *     low byte.
 *
 * MELNORME_RAINBOW_COUNT:
 *     The number of rainbow world locations sold to the Melnorme.
 *
 * LIGHT_MINERAL_LOAD:
 *     Number of times the captain has brought in a light mineral
 *     load (<1000 RU). Max 6.
 *
 * MEDIUM_MINERAL_LOAD:
 *     Number of times the captain has brought in a medium mineral
 *     load (>=1000 RU, <2500 RU). Max 6.
 *
 * HEAVY_MINERAL_LOAD:
 *     Number of times the captain has brought in a heavy mineral
 *     load (>=2500 RU). Max 6.
 *
 * UTWIG_SUPOX_MISSION:
 *     0 if the Utwig and Supox fleet haven't left their home
 *        world.
 *     1 if the U&S are on their way towards the Kohr-Ah
 *     2 if the U&S are fighting the Kohr-Ah (first 80 days)
 *     3 does not occur
 *     4 if the U&S are fighting the Kohr-Ah (second 80 days)
 *     5 if the U&S are returning home.
 *     6 if the U&S are back at their home world.
 *
 * KNOW_ABOUT_SHATTERED:
 *     0 if the player doesn't known about shattered worlds
 *     1 if the player has encountered a shattered world
 *     2 if the player knows that shatterred worlds are caused by
 *        Mycon deep children.
 *     3 if the player has told the Syreen that Mycon Deep
 *        Children cause shattered worlds. Proof doesn't have to be
 *        presented yet at this time.
 *
 * MYCON_KNOW_AMBUSH:
 *     Set to 1 when the Mycon have been butchered at Organon,
 *     just before the remaining Mycon head back home.
 *
 * DNYARRI_LIED:
 *     Set when the Talking Pet tells you his version of their
 *     race's history with the Ur-Quan. Cleared once you confront
 *     him about this lie.
 *
 * PKUNK_MANNER:
 *     0 not met the Pkunk
 *     1 fought the Pkunk, but relations are still salvagable.
 *     2 hostile relations with the Pkunk, no way back.
 *     3 friendly relations with the Pkunk
 *
 * BOMB_CARRIER:
 *     0 when the flagship is not in battle, or it doesn't have
 *        the enhanced precursor bomb installed.
 *     1 when the flagship is in battle and the bomb is installed.
 *        This determines whether you can flee (if the warp escape
 *        unit is installed at all), and whether taking the ship
 *        into the Sa-Matra defense structure will trigger the end
 *        of the game.
 *
 * THRADD_MISSION:
 *     0 if the Thraddash fleet hasn't left the Thraddash home
 *        world.
 *     1 if the Thraddash are heading towards Kohr-Ah territory.
 *     2 if the Thraddash are fighting the Kohr-Ah.
 *     3 if the Thraddash are returning from Kohr-Ah territory.
 *     4 if the Thraddash fleet is back at the Thraddash home
 *        world.
 *
 * MYCON_FELL_FOR_AMBUSH:
 *     Set to 1 when the Mycon have been told about Organon and
 *     are moving towards it.
 *
 * UMGAH_ZOMBIE_BLOBBIES:
 *     The Umgah have come under the influence of the Talking Pet
 *
 * KNOW_UMGAH_ZOMBIES:
 *     The Captain is aware that something is up with the Umgah
 *
 * GLOBAL_FLAGS_AND_DATA:
 *     This state seems to be used to distinguish between different
 *     places where one may have an conversation with an alien.
 *     Like home world, other world, space. Why this needs 8 bits I
 *     don't know. Only specific combinations of bits seem to be
 *     used (0, 1, or all bits). A closer investigation is
 *     desirable. - SvdB
 *     Bit 4 is set when initiating communication with the Ilwrath
 *         homeworld by means of a HyperWave Broadcaster.
 *     Bit 5 is set when initiating communication with an Ilwrath
 *         ship by means of a HyperWave Broadcaster.
 *     All bits are cleared when communication is over.
 *
 * Revision-specific notes:
 * -----------------------
 * REV1: MegaMod v0.8.0.85
 *   AUTOPILOT_OK:
 *       It is allowed for the autopilot to engage
 *
 *   KNOW_QS_PORTAL:
 *       Quasispace portal name flags
 *
 * REV5: MegaMod v0.8.4
 *   SUPOX_SHIP_MONTH:
 *       The month that new ships are available from the Supox.
 *
 *   SUPOX_SHIP_DAY:
 *       The day of the month in that new ships are available.
 *
 *   SUPOX_SHIP_YEAR:
 *       The year that new ships are available from the Supox
 *       (stored as an offset from the year the game starts).
 *
 *   UTWIG_SHIP_MONTH:
 *       The month that new ships are available from the Utwig.
 *
 *   UTWIG_SHIP_DAY:
 *       The day of the month in that new ships are available.
 *
 *   UTWIG_SHIP_YEAR:
 *       The year that new ships are available from the Utwig
 *       (stored as an offset from the year the game starts).
 *
 *   INVESTIGATE_PROBES:
 *       0 no info about slylandro probes.
 *       1 heard info about probes from the ZFP
 *       2 heard info about probes from the Thraddash
 *       3 unused
 *
 *   INVESTIGATE_PORTAL:
 *       Bit-0 Heard of the portal from the Spathi
 *       Bit-1 Heard of the portal from the Arilou
 *       Bit-2 Traveled into QuasiSpace
 *
 * Additional notes:
 * -----------------
 * The following state bits are actually offsets into defgrp.dat.
 * They really shouldn't be part of the serialized Game State
 * array! --MCM
 *   SHOFIXTI_GRPOFFS, ZOQFOT_GRPOFFS, MELNORME0_GRPOFFS through
 *   MELNORME8_GRPOFFS, URQUAN_PROBE_GRPOFFS, COLONY_GRPOFFS,
 *   SAMATRA_GRPOFFS
 *
 * These states are defined as macros but have special meanings:
 *   YEARS_TO_KOHRAH_VICTORY (optDeCleansing ? 100 : 4) [macro,
 *   not a state]
 */