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
} CONVERSATION;

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
	RESOURCE AlienAltSongRes;
	LDAS_FLAGS AlienSongFlags;

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
	ARTIFACT_2_DEVICE,
	ARTIFACT_3_DEVICE,
	LUNAR_BASE_DEVICE,

	NUM_DEVICES
};

#define YEARS_TO_KOHRAH_VICTORY 4

// A structure describing how many bits are used for each game state value.
typedef struct GameStateBitMap GameStateBitMap;
struct GameStateBitMap {
	const char *name;
	BYTE numBits;
};

BOOLEAN serialiseGameState (const GameStateBitMap *bm,
		BYTE **buf, size_t *numBytes);
BOOLEAN deserialiseGameState (const GameStateBitMap *bm,
		const BYTE *buf, size_t numBytes);

#define START_GAME_STATE enum {
#define ADD_GAME_STATE(SName,NumBits) SName, END_##SName = SName + NumBits - 1,
#define END_GAME_STATE NUM_GAME_STATE_BITS };

// This enum is now only used for the symbolic names, and the comments.
// XXX: When all the dialogs are moved to Lua scripts, this will become
// obsolete. Perhaps it would be best to move the comments to
// content/base/script/initgame/initprops.lua then.
START_GAME_STATE
		/* Shofixti states */
	ADD_GAME_STATE (SHOFIXTI_VISITS, 3)
	ADD_GAME_STATE (SHOFIXTI_STACK1, 2)
	ADD_GAME_STATE (SHOFIXTI_STACK2, 3)
	ADD_GAME_STATE (SHOFIXTI_STACK3, 2)
	ADD_GAME_STATE (SHOFIXTI_KIA, 1)
	ADD_GAME_STATE (SHOFIXTI_BRO_KIA, 1)
	ADD_GAME_STATE (SHOFIXTI_RECRUITED, 1)

	ADD_GAME_STATE (SHOFIXTI_MAIDENS, 1) /* Did you find the babes yet? */
	ADD_GAME_STATE (MAIDENS_ON_SHIP, 1)
	ADD_GAME_STATE (BATTLE_SEGUE, 1)
			/* Set to 0 in init_xxx_comm() if communications directly
			 * follows an encounter.
			 * Set to 1 in init_xxx_comm() if the player gets to decide
			 * whether to attack or talk.
			 * Set to 1 in communication when battle follows the
			 * communication. It is still valid when uninit_xxx_comm() gets
			 * called after combat or communication.
			 */
	ADD_GAME_STATE (PLANETARY_LANDING, 1)
	ADD_GAME_STATE (PLANETARY_CHANGE, 1)
			/* Flag set to 1 when the planet information for the current
			 * world is changed since it was last saved to the starinfo.dat
			 * file. Set when picking up bio, mineral, or energy nodes.
			 * When there's no current world, it should be 0.
			 */

		/* Spathi states */
	ADD_GAME_STATE (SPATHI_VISITS, 3)
	ADD_GAME_STATE (SPATHI_HOME_VISITS, 3)
	ADD_GAME_STATE (FOUND_PLUTO_SPATHI, 2)
			/* 0 - Haven't met Fwiffo.
			 * 1 - Met Fwiffo on Pluto, now talking to him.
			 * 2 - Met Fwiffo on Pluto, after dialog.
			 * 3 - Met Fwiffo, and have reported to the Safe Ones on
			 *     the Spathi moon that he was either killed, or that
			 *     you have him on board.
			 */
	ADD_GAME_STATE (SPATHI_SHIELDED_SELVES, 1)
	ADD_GAME_STATE (SPATHI_CREATURES_EXAMINED, 1)
	ADD_GAME_STATE (SPATHI_CREATURES_ELIMINATED, 1)
	ADD_GAME_STATE (UMGAH_BROADCASTERS, 1)
	ADD_GAME_STATE (SPATHI_MANNER, 2)
	ADD_GAME_STATE (SPATHI_QUEST, 1)
	ADD_GAME_STATE (LIED_ABOUT_CREATURES, 2)
	ADD_GAME_STATE (SPATHI_PARTY, 1)
	ADD_GAME_STATE (KNOW_SPATHI_PASSWORD, 1)

	ADD_GAME_STATE (ILWRATH_HOME_VISITS, 3)
	ADD_GAME_STATE (ILWRATH_CHMMR_VISITS, 1)

	ADD_GAME_STATE (ARILOU_SPACE, 1)
			/* 0 if the periodically opening QuasiSpace portal is
			 * closed or closing.
			 * 1 if the periodically opening QuasiSpace portal is
			 * open or opening.
			 */
	ADD_GAME_STATE (ARILOU_SPACE_SIDE, 2)
			/* 0 if in HyperSpace and not just emerged from the periodically
			 * opening QuasiSpace portal.
			 * 1 if in HyperSpace and just emerged from the periodically
			 * QuasiSpace portal (still on the portal).
			 * 2 if in QuasiSpace and just emerged from the periodically
			 * opening portal (still on the portal).
			 * 3 if in QuasiSpace and not just emerged from the
			 * periodically opening portal.
			 */
	ADD_GAME_STATE (ARILOU_SPACE_COUNTER, 4)
			/* Keeps track of how far the periodically opening QuasiSpace
			 * portal is open. (This determines the image)
			 * 0 <= ARILOU_SPACE_COUNTER <= 9
			 * 0 means totally closed.
			 * 9 means completely open.
			 */

	ADD_GAME_STATE (LANDER_SHIELDS, 4)

	ADD_GAME_STATE (MET_MELNORME, 1)
	ADD_GAME_STATE (MELNORME_RESCUE_REFUSED, 1)
	ADD_GAME_STATE (MELNORME_RESCUE_COUNT, 3)
	ADD_GAME_STATE (TRADED_WITH_MELNORME, 1)
	ADD_GAME_STATE (WHY_MELNORME_PURPLE, 1)
	ADD_GAME_STATE (MELNORME_CREDIT0, 8)
	ADD_GAME_STATE (MELNORME_CREDIT1, 8)
	ADD_GAME_STATE (MELNORME_BUSINESS_COUNT, 2)
	ADD_GAME_STATE (MELNORME_YACK_STACK0, 2)
	ADD_GAME_STATE (MELNORME_YACK_STACK1, 2)
	ADD_GAME_STATE (MELNORME_YACK_STACK2, 4)
	ADD_GAME_STATE (MELNORME_YACK_STACK3, 3)
	ADD_GAME_STATE (MELNORME_YACK_STACK4, 2)
	ADD_GAME_STATE (WHY_MELNORME_BLUE, 1)
	ADD_GAME_STATE (MELNORME_ANGER, 2)
	ADD_GAME_STATE (MELNORME_MIFFED_COUNT, 2)
	ADD_GAME_STATE (MELNORME_PISSED_COUNT, 2)
	ADD_GAME_STATE (MELNORME_HATE_COUNT, 2)

	ADD_GAME_STATE (PROBE_MESSAGE_DELIVERED, 1)
	ADD_GAME_STATE (PROBE_ILWRATH_ENCOUNTER, 1)

	ADD_GAME_STATE (STARBASE_AVAILABLE, 1)
	ADD_GAME_STATE (STARBASE_VISITED, 1)
	ADD_GAME_STATE (RADIOACTIVES_PROVIDED, 1)
	ADD_GAME_STATE (LANDERS_LOST, 1)
	ADD_GAME_STATE (GIVEN_FUEL_BEFORE, 1)

	ADD_GAME_STATE (AWARE_OF_SAMATRA, 1)
	ADD_GAME_STATE (YEHAT_CAVALRY_ARRIVED, 1)
	ADD_GAME_STATE (URQUAN_MESSED_UP, 1)

	ADD_GAME_STATE (MOONBASE_DESTROYED, 1)
	ADD_GAME_STATE (WILL_DESTROY_BASE, 1)

	ADD_GAME_STATE (WIMBLIS_TRIDENT_ON_SHIP, 1)
	ADD_GAME_STATE (GLOWING_ROD_ON_SHIP, 1)

	ADD_GAME_STATE (KOHR_AH_KILLED_ALL, 1)

	ADD_GAME_STATE (STARBASE_YACK_STACK1, 1)

	ADD_GAME_STATE (DISCUSSED_PORTAL_SPAWNER, 1)
	ADD_GAME_STATE (DISCUSSED_TALKING_PET, 1)
	ADD_GAME_STATE (DISCUSSED_UTWIG_BOMB, 1)
	ADD_GAME_STATE (DISCUSSED_SUN_EFFICIENCY, 1)
	ADD_GAME_STATE (DISCUSSED_ROSY_SPHERE, 1)
	ADD_GAME_STATE (DISCUSSED_AQUA_HELIX, 1)
	ADD_GAME_STATE (DISCUSSED_CLEAR_SPINDLE, 1)
	ADD_GAME_STATE (DISCUSSED_ULTRON, 1)
	ADD_GAME_STATE (DISCUSSED_MAIDENS, 1)
	ADD_GAME_STATE (DISCUSSED_UMGAH_HYPERWAVE, 1)
	ADD_GAME_STATE (DISCUSSED_BURVIX_HYPERWAVE, 1)
	ADD_GAME_STATE (SYREEN_WANT_PROOF, 1)
	ADD_GAME_STATE (PLAYER_HAVING_SEX, 1)
	ADD_GAME_STATE (MET_ARILOU, 1)
	ADD_GAME_STATE (DISCUSSED_TAALO_PROTECTOR, 1)
	ADD_GAME_STATE (DISCUSSED_EGG_CASING0, 1)
	ADD_GAME_STATE (DISCUSSED_EGG_CASING1, 1)
	ADD_GAME_STATE (DISCUSSED_EGG_CASING2, 1)
	ADD_GAME_STATE (DISCUSSED_SYREEN_SHUTTLE, 1)
	ADD_GAME_STATE (DISCUSSED_VUX_BEAST, 1)
	ADD_GAME_STATE (DISCUSSED_DESTRUCT_CODE, 1)
	ADD_GAME_STATE (DISCUSSED_URQUAN_WARP, 1)
	ADD_GAME_STATE (DISCUSSED_ARTIFACT_2, 1)
	ADD_GAME_STATE (DISCUSSED_ARTIFACT_3, 1)

	ADD_GAME_STATE (ATTACKED_DRUUGE, 1)

	ADD_GAME_STATE (NEW_ALLIANCE_NAME, 2)

	ADD_GAME_STATE (PORTAL_COUNTER, 4)
			/* Set to 1 when the player opens a QuasiSpace portal.
			 * It will then be increased to 10, at which time
			 * the portal is completely open. (This determines the image).
			 */

	ADD_GAME_STATE (BURVIXESE_BROADCASTERS, 1)
	ADD_GAME_STATE (BURV_BROADCASTERS_ON_SHIP, 1)

	ADD_GAME_STATE (UTWIG_BOMB, 1)
	ADD_GAME_STATE (UTWIG_BOMB_ON_SHIP, 1)

	ADD_GAME_STATE (AQUA_HELIX, 1)
	ADD_GAME_STATE (AQUA_HELIX_ON_SHIP, 1)

	ADD_GAME_STATE (SUN_DEVICE, 1)
	ADD_GAME_STATE (SUN_DEVICE_ON_SHIP, 1)

	ADD_GAME_STATE (TAALO_PROTECTOR, 1)
	ADD_GAME_STATE (TAALO_PROTECTOR_ON_SHIP, 1)

	ADD_GAME_STATE (SHIP_VAULT_UNLOCKED, 1)
	ADD_GAME_STATE (SYREEN_SHUTTLE, 1)

	ADD_GAME_STATE (PORTAL_KEY, 1)
	ADD_GAME_STATE (PORTAL_KEY_ON_SHIP, 1)

	ADD_GAME_STATE (VUX_BEAST, 1)
	ADD_GAME_STATE (VUX_BEAST_ON_SHIP, 1)

	ADD_GAME_STATE (TALKING_PET, 1)
	ADD_GAME_STATE (TALKING_PET_ON_SHIP, 1)

	ADD_GAME_STATE (MOONBASE_ON_SHIP, 1)

	ADD_GAME_STATE (KOHR_AH_FRENZY, 1)
	ADD_GAME_STATE (KOHR_AH_VISITS, 2)
	ADD_GAME_STATE (KOHR_AH_BYES, 1)

	ADD_GAME_STATE (SLYLANDRO_HOME_VISITS, 3)
	ADD_GAME_STATE (DESTRUCT_CODE_ON_SHIP, 1)

	ADD_GAME_STATE (ILWRATH_VISITS, 3)
	ADD_GAME_STATE (ILWRATH_DECEIVED, 1)
	ADD_GAME_STATE (FLAGSHIP_CLOAKED, 1)

	ADD_GAME_STATE (MYCON_VISITS, 3)
	ADD_GAME_STATE (MYCON_HOME_VISITS, 3)
	ADD_GAME_STATE (MYCON_AMBUSH, 1)
	ADD_GAME_STATE (MYCON_FELL_FOR_AMBUSH, 1)
			/* Set to 1 when the Mycon have been told about Organon
			 * and are moving towards it.
			 */

	ADD_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 8)
			/* This state seems to be used to distinguish between different
			 * places where one may have an conversation with an alien.
			 * Like home world, other world, space.
			 * Why this needs 8 bits I don't know. Only specific
			 * combinations of bits seem to be used (0, 1, or all bits).
			 * A closer investigation is desirable. - SvdB
			 * Bit 4 is set when initiating communication with the Ilwrath
			 * 		homeworld by means of a HyperWave Broadcaster.
			 * Bit 5 is set when initiating communication with an Ilwrath
			 * 		ship by means of a HyperWave Broadcaster.
			 * All bits are cleared when communication is over.
			 */

	ADD_GAME_STATE (ORZ_VISITS, 3)
	ADD_GAME_STATE (TAALO_VISITS, 3)
	ADD_GAME_STATE (ORZ_MANNER, 2)

	ADD_GAME_STATE (PROBE_EXHIBITED_BUG, 1)
	ADD_GAME_STATE (CLEAR_SPINDLE_ON_SHIP, 1)

	ADD_GAME_STATE (URQUAN_VISITS, 3)
	ADD_GAME_STATE (PLAYER_HYPNOTIZED, 1)

	ADD_GAME_STATE (VUX_VISITS, 3)
	ADD_GAME_STATE (VUX_HOME_VISITS, 3)
	ADD_GAME_STATE (ZEX_VISITS, 3)
	ADD_GAME_STATE (ZEX_IS_DEAD, 1)
	ADD_GAME_STATE (KNOW_ZEX_WANTS_MONSTER, 1)

	ADD_GAME_STATE (UTWIG_VISITS, 3)
	ADD_GAME_STATE (UTWIG_HOME_VISITS, 3)
	ADD_GAME_STATE (BOMB_VISITS, 3)
	ADD_GAME_STATE (ULTRON_CONDITION, 3)
			/* 0 if the Supox still have the Ultron
			 * 1 if the Captain has the Ultron, completely broken
			 * 2 if the Captain has the Ultron, with 1 fix
			 * 3 if the Captain has the Ultron, with 2 fixes
			 * 4 if the Captain has the Ultron, completely restored
			 * 5 if the Ultron has been returned to the Utwig
			 */
	ADD_GAME_STATE (UTWIG_HAVE_ULTRON, 1)
	ADD_GAME_STATE (BOMB_UNPROTECTED, 1)

	ADD_GAME_STATE (TAALO_UNPROTECTED, 1)

	ADD_GAME_STATE (TALKING_PET_VISITS, 3)
	ADD_GAME_STATE (TALKING_PET_HOME_VISITS, 3)
	ADD_GAME_STATE (UMGAH_ZOMBIE_BLOBBIES, 1)
			/* The Umgah have come under the influence of the Talking Pet */
	ADD_GAME_STATE (KNOW_UMGAH_ZOMBIES, 1)
			/* The Captain is aware that something is up with the Umgah */

	ADD_GAME_STATE (ARILOU_VISITS, 3)
	ADD_GAME_STATE (ARILOU_HOME_VISITS, 3)
	ADD_GAME_STATE (KNOW_ARILOU_WANT_WRECK, 1)
	ADD_GAME_STATE (ARILOU_CHECKED_UMGAH, 2)
	ADD_GAME_STATE (PORTAL_SPAWNER, 1)
	ADD_GAME_STATE (PORTAL_SPAWNER_ON_SHIP, 1)

	ADD_GAME_STATE (UMGAH_VISITS, 3)
	ADD_GAME_STATE (UMGAH_HOME_VISITS, 3)
	ADD_GAME_STATE (MET_NORMAL_UMGAH, 1)

	ADD_GAME_STATE (SYREEN_HOME_VISITS, 3)
	ADD_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP, 1)
	ADD_GAME_STATE (KNOW_SYREEN_VAULT, 1)

	ADD_GAME_STATE (EGG_CASE0_ON_SHIP, 1)
	ADD_GAME_STATE (SUN_DEVICE_UNGUARDED, 1)

	ADD_GAME_STATE (ROSY_SPHERE_ON_SHIP, 1)
			/* The Rosy Sphere is aboard the flagship, i.e. It has been
			 * acquired from the Druuge, but not yet inserted in the broken
			 * Ultron. cf. ROSY_SPHERE */

	ADD_GAME_STATE (CHMMR_HOME_VISITS, 3)
	ADD_GAME_STATE (CHMMR_EMERGING, 1)
	ADD_GAME_STATE (CHMMR_UNLEASHED, 1)
	ADD_GAME_STATE (CHMMR_BOMB_STATE, 2)
			/* 0 - Nothing is known about the Precursor Bomb.
			 * 1 - The captain knows from the Chmmr that some extremely
			 *     powerful weapon is needed to destroy the Sa-Matra.
			 * 2 - Installation of the precursor bomb has started.
			 * 3 - Left the starbase after installation of the Precursor bomb.
			 */

	ADD_GAME_STATE (DRUUGE_DISCLAIMER, 1)

	ADD_GAME_STATE (YEHAT_VISITS, 3)
	ADD_GAME_STATE (YEHAT_REBEL_VISITS, 3)
	ADD_GAME_STATE (YEHAT_HOME_VISITS, 3)
	ADD_GAME_STATE (YEHAT_CIVIL_WAR, 1)
	ADD_GAME_STATE (YEHAT_ABSORBED_PKUNK, 1)
	ADD_GAME_STATE (YEHAT_SHIP_MONTH, 4)
	ADD_GAME_STATE (YEHAT_SHIP_DAY, 5)
	ADD_GAME_STATE (YEHAT_SHIP_YEAR, 5)

	ADD_GAME_STATE (CLEAR_SPINDLE, 1)
	ADD_GAME_STATE (PKUNK_VISITS, 3)
	ADD_GAME_STATE (PKUNK_HOME_VISITS, 3)
	ADD_GAME_STATE (PKUNK_SHIP_MONTH, 4)
			/* The month in PKUNK_SHIP_YEAR that new ships are available
			 * from the Pkunk. */
	ADD_GAME_STATE (PKUNK_SHIP_DAY, 5)
			/* The day of the month in PKUNK_SHIP_MONTH in PKUNK_SHIP_YEAR
			 * that new ships are available. */
	ADD_GAME_STATE (PKUNK_SHIP_YEAR, 5)
			/* The year that new ships are available from the Pkunk
			 * (stored as an offset from the year the game starts). */
	ADD_GAME_STATE (PKUNK_MISSION, 3)

	ADD_GAME_STATE (SUPOX_VISITS, 3)
	ADD_GAME_STATE (SUPOX_HOME_VISITS, 3)

	ADD_GAME_STATE (THRADD_VISITS, 3)
	ADD_GAME_STATE (THRADD_HOME_VISITS, 3)
	ADD_GAME_STATE (HELIX_VISITS, 3)
	ADD_GAME_STATE (HELIX_UNPROTECTED, 1)
	ADD_GAME_STATE (THRADD_CULTURE, 2)
	ADD_GAME_STATE (THRADD_MISSION, 3)
			/* 0 if the Thraddash fleet hasn't left the Thraddash home world.
			 * 1 if the Thraddash are heading towards Kohr-Ah territory.
			 * 2 if the Thraddash are fighting the Kohr-Ah.
			 * 3 if the Thraddash are returning from Kohr-Ah territory.
			 * 4 if the Thraddash fleet is back at the Thraddash home world.
			 */

	ADD_GAME_STATE (DRUUGE_VISITS, 3)
	ADD_GAME_STATE (DRUUGE_HOME_VISITS, 3)
	ADD_GAME_STATE (ROSY_SPHERE, 1)
			/* The play has or has had the Rosy Sphere.
			 * cf. ROSY_SHERE_ON_SHIP */
	ADD_GAME_STATE (SCANNED_MAIDENS, 1)
	ADD_GAME_STATE (SCANNED_FRAGMENTS, 1)
	ADD_GAME_STATE (SCANNED_CASTER, 1)
	ADD_GAME_STATE (SCANNED_SPAWNER, 1)
	ADD_GAME_STATE (SCANNED_ULTRON, 1)

	ADD_GAME_STATE (ZOQFOT_INFO, 2)
	ADD_GAME_STATE (ZOQFOT_HOSTILE, 1)
	ADD_GAME_STATE (ZOQFOT_HOME_VISITS, 3)
	ADD_GAME_STATE (MET_ZOQFOT, 1)
	ADD_GAME_STATE (ZOQFOT_DISTRESS, 2)
			/* 0 if the Zoq-Fot-Pik aren't in distress
			 * 1 if the Zoq-Fot-Pik are under attack by the Kohr-Ah
			 * 2 if the Zoq-Fot-Pik have been destroyed because of this
			 *   attack (not by the Kohr-Ah final victory cleansing)
			 */

	ADD_GAME_STATE (EGG_CASE1_ON_SHIP, 1)
	ADD_GAME_STATE (EGG_CASE2_ON_SHIP, 1)
	ADD_GAME_STATE (MYCON_SUN_VISITS, 3)
	ADD_GAME_STATE (ORZ_HOME_VISITS, 3)

	ADD_GAME_STATE (MELNORME_FUEL_PROCEDURE, 1)
	ADD_GAME_STATE (MELNORME_TECH_PROCEDURE, 1)
	ADD_GAME_STATE (MELNORME_INFO_PROCEDURE, 1)

	ADD_GAME_STATE (MELNORME_TECH_STACK, 4)
			/* MELNORME_TECH_STACK is now unused */
	ADD_GAME_STATE (MELNORME_EVENTS_INFO_STACK, 5)
	ADD_GAME_STATE (MELNORME_ALIEN_INFO_STACK, 5)
	ADD_GAME_STATE (MELNORME_HISTORY_INFO_STACK, 5)

	ADD_GAME_STATE (RAINBOW_WORLD0, 8)
			/* Low byte of a bit array, one bit per rainbow world.
			 * Each bit is set if the rainbow world has been visited.
			 * The lowest bit is for the first star in the star_array
			 * with RAINBOW_DEFINED, and so on.
			 */
	ADD_GAME_STATE (RAINBOW_WORLD1, 2)
			/* High 2 bits of the bit array of which RAINBOW_WORLD0
			 * is the low byte.
			 */
	ADD_GAME_STATE (MELNORME_RAINBOW_COUNT, 4)
			/* The number of rainbow world locations sold to the Melnorme. */

	ADD_GAME_STATE (USED_BROADCASTER, 1)
	ADD_GAME_STATE (BROADCASTER_RESPONSE, 1)

	ADD_GAME_STATE (IMPROVED_LANDER_SPEED, 1)
	ADD_GAME_STATE (IMPROVED_LANDER_CARGO, 1)
	ADD_GAME_STATE (IMPROVED_LANDER_SHOT, 1)

	ADD_GAME_STATE (MET_ORZ_BEFORE, 1)
	ADD_GAME_STATE (YEHAT_REBEL_TOLD_PKUNK, 1)
	ADD_GAME_STATE (PLAYER_HAD_SEX, 1)
	ADD_GAME_STATE (UMGAH_BROADCASTERS_ON_SHIP, 1)

	ADD_GAME_STATE (LIGHT_MINERAL_LOAD, 3)
			/* Number of times the captain has brought in a light mineral
			 * load (<1000 RU). Max 6. */
	ADD_GAME_STATE (MEDIUM_MINERAL_LOAD, 3)
			/* Number of times the captain has brought in a medium mineral
			 * load (>=1000 RU, <2500 RU). Max 6. */
	ADD_GAME_STATE (HEAVY_MINERAL_LOAD, 3)
			/* Number of times the captain has brought in a heavy mineral
			 * load (>=2500 RU). Max 6. */

	ADD_GAME_STATE (STARBASE_BULLETS, 32)

	ADD_GAME_STATE (STARBASE_MONTH, 4)
	ADD_GAME_STATE (STARBASE_DAY, 5)

	ADD_GAME_STATE (CREW_SOLD_TO_DRUUGE0, 8)
	ADD_GAME_STATE (CREW_PURCHASED0, 8)
	ADD_GAME_STATE (CREW_PURCHASED1, 8)

	ADD_GAME_STATE (URQUAN_PROTECTING_SAMATRA, 1)

#define THRADDASH_BODY_THRESHOLD DIF_CASE(25, 15, 35)
	ADD_GAME_STATE (THRADDASH_BODY_COUNT, 5)

	ADD_GAME_STATE (UTWIG_SUPOX_MISSION, 3)
			/* 0 if the Utwig and Supox fleet haven't left their home world.
			 * 1 if the U&S are on their way towards the Kohr-Ah
			 * 2 if the U&S are fighting the Kohr-Ah (first 80 days)
			 * 3 does not occur
             * 4 if the U&S are fighting the Kohr-Ah (second 80 days)
			 * 5 if the U&S are returning home.
			 * 6 if the U&S are back at their home world.
			 */
	ADD_GAME_STATE (SPATHI_INFO, 3)

	ADD_GAME_STATE (ILWRATH_INFO, 2)
	ADD_GAME_STATE (ILWRATH_GODS_SPOKEN, 4)
	ADD_GAME_STATE (ILWRATH_WORSHIP, 2)
	ADD_GAME_STATE (ILWRATH_FIGHT_THRADDASH, 1)

	ADD_GAME_STATE (READY_TO_CONFUSE_URQUAN, 1)
	ADD_GAME_STATE (URQUAN_HYPNO_VISITS, 1)
	ADD_GAME_STATE (MENTIONED_PET_COMPULSION, 1)
	ADD_GAME_STATE (URQUAN_INFO, 2)
	ADD_GAME_STATE (KNOW_URQUAN_STORY, 2)

	ADD_GAME_STATE (MYCON_INFO, 4)
	ADD_GAME_STATE (MYCON_RAMBLE, 5)
	ADD_GAME_STATE (KNOW_ABOUT_SHATTERED, 2)
			/* 0 if the player doesn't known about shattered worlds
			 * 1 if the player has encountered a shattered world
			 * 2 if the player knows that shatterred worlds are caused
			 *   by Mycon deep children.
			 * 3 if the player has told the Syreen that Mycon Deep Children
			 *   cause shattered worlds. Proof doesn't have to be presented
			 *   yet at this time.
			 */
	ADD_GAME_STATE (MYCON_INSULTS, 3)
	ADD_GAME_STATE (MYCON_KNOW_AMBUSH, 1)
			/* Set to 1 when the Mycon have been butchered at Organon,
			 * just before the remaining Mycon head back home.
			 */

	ADD_GAME_STATE (SYREEN_INFO, 2)
	ADD_GAME_STATE (KNOW_SYREEN_WORLD_SHATTERED, 1)
	ADD_GAME_STATE (SYREEN_KNOW_ABOUT_MYCON, 1)

	ADD_GAME_STATE (TALKING_PET_INFO, 3)
	ADD_GAME_STATE (TALKING_PET_SUGGESTIONS, 3)
	ADD_GAME_STATE (LEARNED_TALKING_PET, 1)
	ADD_GAME_STATE (DNYARRI_LIED, 1)
			/* Set when the Talking Pet tells you his version of their
			 * race's history with the Ur-Quan.
			 * Cleared once you confront him about this lie.
			 */
	ADD_GAME_STATE (SHIP_TO_COMPEL, 1)

	ADD_GAME_STATE (ORZ_GENERAL_INFO, 2)
	ADD_GAME_STATE (ORZ_PERSONAL_INFO, 3)
	ADD_GAME_STATE (ORZ_ANDRO_STATE, 2)
	ADD_GAME_STATE (REFUSED_ORZ_ALLIANCE, 1)

	ADD_GAME_STATE (PKUNK_MANNER, 2)
			/* 0 not met the Pkunk
			 * 1 fought the Pkunk, but relations are still salvagable.
			 * 2 hostile relations with the Pkunk, no way back.
			 * 3 friendly relations with the Pkunk
			 */
	ADD_GAME_STATE (PKUNK_ON_THE_MOVE, 1)
	ADD_GAME_STATE (PKUNK_FLEET, 2)
	ADD_GAME_STATE (PKUNK_MIGRATE, 2)
	ADD_GAME_STATE (PKUNK_RETURN, 1)
	ADD_GAME_STATE (PKUNK_WORRY, 2)
	ADD_GAME_STATE (PKUNK_INFO, 3)
	ADD_GAME_STATE (PKUNK_WAR, 2)
	ADD_GAME_STATE (PKUNK_FORTUNE, 3)
	ADD_GAME_STATE (PKUNK_MIGRATE_VISITS, 3)
	ADD_GAME_STATE (PKUNK_REASONS, 4)
	ADD_GAME_STATE (PKUNK_SWITCH, 1)
	ADD_GAME_STATE (PKUNK_SENSE_VICTOR, 1)

	ADD_GAME_STATE (KOHR_AH_REASONS, 2)
	ADD_GAME_STATE (KOHR_AH_PLEAD, 2)
	ADD_GAME_STATE (KOHR_AH_INFO, 2)
	ADD_GAME_STATE (KNOW_KOHR_AH_STORY, 2)
	ADD_GAME_STATE (KOHR_AH_SENSES_EVIL, 1)
	ADD_GAME_STATE (URQUAN_SENSES_EVIL, 1)

	ADD_GAME_STATE (SLYLANDRO_PROBE_VISITS, 3)
	ADD_GAME_STATE (SLYLANDRO_PROBE_THREAT, 2)
	ADD_GAME_STATE (SLYLANDRO_PROBE_WRONG, 2)
	ADD_GAME_STATE (SLYLANDRO_PROBE_ID, 2)
	ADD_GAME_STATE (SLYLANDRO_PROBE_INFO, 2)
	ADD_GAME_STATE (SLYLANDRO_PROBE_EXIT, 2)

	ADD_GAME_STATE (UMGAH_HOSTILE, 1)
	ADD_GAME_STATE (UMGAH_EVIL_BLOBBIES, 1)
	ADD_GAME_STATE (UMGAH_MENTIONED_TRICKS, 2)

	ADD_GAME_STATE (BOMB_CARRIER, 1)
			/* 0 when the flagship is not in battle, or it doesn't have the
			 *   enhanced precursor bomb installed.
			 * 1 when the flagship is in battle and the bomb is installed.
			 * This determines whether you can flee (if the warp escape unit
			 * is installed at all), and whether taking the ship into the
			 * Sa-Matra defense structure will trigger the end of the game.
			 */
	
	ADD_GAME_STATE (THRADD_MANNER, 1)
	ADD_GAME_STATE (THRADD_INTRO, 2)
	ADD_GAME_STATE (THRADD_DEMEANOR, 3)
	ADD_GAME_STATE (THRADD_INFO, 2)
	ADD_GAME_STATE (THRADD_BODY_LEVEL, 2)
	ADD_GAME_STATE (THRADD_MISSION_VISITS, 1)
	ADD_GAME_STATE (THRADD_STACK_1, 3)
	ADD_GAME_STATE (THRADD_HOSTILE_STACK_2, 1)
	ADD_GAME_STATE (THRADD_HOSTILE_STACK_3, 1)
	ADD_GAME_STATE (THRADD_HOSTILE_STACK_4, 1)
	ADD_GAME_STATE (THRADD_HOSTILE_STACK_5, 1)

	ADD_GAME_STATE (CHMMR_STACK, 2)

	ADD_GAME_STATE (ARILOU_MANNER, 2)
	ADD_GAME_STATE (NO_PORTAL_VISITS, 1)
	ADD_GAME_STATE (ARILOU_STACK_1, 2)
	ADD_GAME_STATE (ARILOU_STACK_2, 1)
	ADD_GAME_STATE (ARILOU_STACK_3, 2)
	ADD_GAME_STATE (ARILOU_STACK_4, 1)
	ADD_GAME_STATE (ARILOU_STACK_5, 2)
	ADD_GAME_STATE (ARILOU_INFO, 2)
	ADD_GAME_STATE (ARILOU_HINTS, 2)

	ADD_GAME_STATE (DRUUGE_MANNER, 1)
	ADD_GAME_STATE (DRUUGE_SPACE_INFO, 2)
	ADD_GAME_STATE (DRUUGE_HOME_INFO, 2)
	ADD_GAME_STATE (DRUUGE_SALVAGE, 1)
	ADD_GAME_STATE (KNOW_DRUUGE_SLAVERS, 2)
	ADD_GAME_STATE (FRAGMENTS_BOUGHT, 2)

	ADD_GAME_STATE (ZEX_STACK_1, 2)
	ADD_GAME_STATE (ZEX_STACK_2, 2)
	ADD_GAME_STATE (ZEX_STACK_3, 2)

	ADD_GAME_STATE (VUX_INFO, 2)
	ADD_GAME_STATE (VUX_STACK_1, 4)
	ADD_GAME_STATE (VUX_STACK_2, 2)
	ADD_GAME_STATE (VUX_STACK_3, 2)
	ADD_GAME_STATE (VUX_STACK_4, 2)

	ADD_GAME_STATE (SHOFIXTI_STACK4, 2)

	ADD_GAME_STATE (YEHAT_REBEL_INFO, 3)
	ADD_GAME_STATE (YEHAT_ROYALIST_INFO, 1)
	ADD_GAME_STATE (YEHAT_ROYALIST_TOLD_PKUNK, 1)
	ADD_GAME_STATE (NO_YEHAT_ALLY_HOME, 1)
	ADD_GAME_STATE (NO_YEHAT_HELP_HOME, 1)
	ADD_GAME_STATE (NO_YEHAT_INFO, 1)
	ADD_GAME_STATE (NO_YEHAT_ALLY_SPACE, 2)
	ADD_GAME_STATE (NO_YEHAT_HELP_SPACE, 2)

	ADD_GAME_STATE (ZOQFOT_KNOW_MASK, 4)

	ADD_GAME_STATE (SUPOX_HOSTILE, 1)
	ADD_GAME_STATE (SUPOX_INFO, 1)
	ADD_GAME_STATE (SUPOX_WAR_NEWS, 2)
	ADD_GAME_STATE (SUPOX_ULTRON_HELP, 1)
	ADD_GAME_STATE (SUPOX_STACK1, 3)
	ADD_GAME_STATE (SUPOX_STACK2, 2)

	ADD_GAME_STATE (UTWIG_HOSTILE, 1)
	ADD_GAME_STATE (UTWIG_INFO, 1)
	ADD_GAME_STATE (UTWIG_WAR_NEWS, 2)
	ADD_GAME_STATE (UTWIG_STACK1, 3)
	ADD_GAME_STATE (UTWIG_STACK2, 2)
	ADD_GAME_STATE (BOMB_INFO, 1)
	ADD_GAME_STATE (BOMB_STACK1, 2)
	ADD_GAME_STATE (BOMB_STACK2, 2)

	ADD_GAME_STATE (SLYLANDRO_KNOW_BROKEN, 1)
	ADD_GAME_STATE (PLAYER_KNOWS_PROBE, 1)
	ADD_GAME_STATE (PLAYER_KNOWS_PROGRAM, 1)
	ADD_GAME_STATE (PLAYER_KNOWS_EFFECTS, 1)
	ADD_GAME_STATE (PLAYER_KNOWS_PRIORITY, 1)
	ADD_GAME_STATE (SLYLANDRO_STACK1, 3)
	ADD_GAME_STATE (SLYLANDRO_STACK2, 1)
	ADD_GAME_STATE (SLYLANDRO_STACK3, 2)
	ADD_GAME_STATE (SLYLANDRO_STACK4, 2)
	ADD_GAME_STATE (SLYLANDRO_STACK5, 1)
	ADD_GAME_STATE (SLYLANDRO_STACK6, 1)
	ADD_GAME_STATE (SLYLANDRO_STACK7, 2)
	ADD_GAME_STATE (SLYLANDRO_STACK8, 2)
	ADD_GAME_STATE (SLYLANDRO_STACK9, 2)
	ADD_GAME_STATE (SLYLANDRO_KNOW_EARTH, 1)
	ADD_GAME_STATE (SLYLANDRO_KNOW_EXPLORE, 1)
	ADD_GAME_STATE (SLYLANDRO_KNOW_GATHER, 1)
	ADD_GAME_STATE (SLYLANDRO_KNOW_URQUAN, 2)
	ADD_GAME_STATE (RECALL_VISITS, 2)

	ADD_GAME_STATE (SLYLANDRO_MULTIPLIER, 3)
	ADD_GAME_STATE (KNOW_SPATHI_QUEST, 1)
	ADD_GAME_STATE (KNOW_SPATHI_EVIL, 1)

	ADD_GAME_STATE (BATTLE_PLANET, 8)
	ADD_GAME_STATE (ESCAPE_COUNTER, 8)

	ADD_GAME_STATE (CREW_SOLD_TO_DRUUGE1, 8)
	ADD_GAME_STATE (PKUNK_DONE_WAR, 1)

	ADD_GAME_STATE (SYREEN_STACK0, 2)
	ADD_GAME_STATE (SYREEN_STACK1, 2)
	ADD_GAME_STATE (SYREEN_STACK2, 2)

	ADD_GAME_STATE (REFUSED_ULTRON_AT_BOMB, 1)
	ADD_GAME_STATE (NO_TRICK_AT_SUN, 1)

	ADD_GAME_STATE (SPATHI_STACK0, 2)
	ADD_GAME_STATE (SPATHI_STACK1, 1)
	ADD_GAME_STATE (SPATHI_STACK2, 1)

	ADD_GAME_STATE (ORZ_STACK0, 1)
	ADD_GAME_STATE (ORZ_STACK1, 1)

/* These state bits are actually offsets into defgrp.dat. They really
 * shouldn't be part of the serialized Game State array! --MCM */
	ADD_GAME_STATE (SHOFIXTI_GRPOFFS, 32)
	ADD_GAME_STATE (ZOQFOT_GRPOFFS, 32)
	ADD_GAME_STATE (MELNORME0_GRPOFFS, 32)
	ADD_GAME_STATE (MELNORME1_GRPOFFS, 32)
	ADD_GAME_STATE (MELNORME2_GRPOFFS, 32)
	ADD_GAME_STATE (MELNORME3_GRPOFFS, 32)
	ADD_GAME_STATE (MELNORME4_GRPOFFS, 32)
	ADD_GAME_STATE (MELNORME5_GRPOFFS, 32)
	ADD_GAME_STATE (MELNORME6_GRPOFFS, 32)
	ADD_GAME_STATE (MELNORME7_GRPOFFS, 32)
	ADD_GAME_STATE (MELNORME8_GRPOFFS, 32)
	ADD_GAME_STATE (URQUAN_PROBE_GRPOFFS, 32)
	ADD_GAME_STATE (COLONY_GRPOFFS, 32)
	ADD_GAME_STATE (SAMATRA_GRPOFFS, 32)

	// JMS: It is allowed for the autopilot to engage
	ADD_GAME_STATE (AUTOPILOT_OK, 1)

	// JMS: Quasispace portal name flags
	ADD_GAME_STATE (KNOW_QS_PORTAL_0, 1)
	ADD_GAME_STATE (KNOW_QS_PORTAL_1, 1)
	ADD_GAME_STATE (KNOW_QS_PORTAL_2, 1)
	ADD_GAME_STATE (KNOW_QS_PORTAL_3, 1)
	ADD_GAME_STATE (KNOW_QS_PORTAL_4, 1)
	ADD_GAME_STATE (KNOW_QS_PORTAL_5, 1)
	ADD_GAME_STATE (KNOW_QS_PORTAL_6, 1)
	ADD_GAME_STATE (KNOW_QS_PORTAL_7, 1)
	ADD_GAME_STATE (KNOW_QS_PORTAL_8, 1)
	ADD_GAME_STATE (KNOW_QS_PORTAL_9, 1)
	ADD_GAME_STATE (KNOW_QS_PORTAL_10, 1)
	ADD_GAME_STATE (KNOW_QS_PORTAL_11, 1)
	ADD_GAME_STATE (KNOW_QS_PORTAL_12, 1)
	ADD_GAME_STATE (KNOW_QS_PORTAL_13, 1)
	ADD_GAME_STATE (KNOW_QS_PORTAL_14, 1)
	ADD_GAME_STATE (KNOW_QS_PORTAL_15, 1)

END_GAME_STATE

// JMS: For making array of Quasispace portal name flags
#define QS_PORTALS_KNOWN \
	(GET_GAME_STATE (KNOW_QS_PORTAL_0)), \
	(GET_GAME_STATE (KNOW_QS_PORTAL_1)), \
	(GET_GAME_STATE (KNOW_QS_PORTAL_2)), \
	(GET_GAME_STATE (KNOW_QS_PORTAL_3)), \
	(GET_GAME_STATE (KNOW_QS_PORTAL_4)), \
	(GET_GAME_STATE (KNOW_QS_PORTAL_5)), \
	(GET_GAME_STATE (KNOW_QS_PORTAL_6)), \
	(GET_GAME_STATE (KNOW_QS_PORTAL_7)), \
	(GET_GAME_STATE (KNOW_QS_PORTAL_8)), \
	(GET_GAME_STATE (KNOW_QS_PORTAL_9)), \
	(GET_GAME_STATE (KNOW_QS_PORTAL_10)), \
	(GET_GAME_STATE (KNOW_QS_PORTAL_11)), \
	(GET_GAME_STATE (KNOW_QS_PORTAL_12)), \
	(GET_GAME_STATE (KNOW_QS_PORTAL_13)), \
	(GET_GAME_STATE (KNOW_QS_PORTAL_14)), \
	(GET_GAME_STATE (KNOW_QS_PORTAL_15)),

// JMS: For making array of Quasispace portal name flags
#define SET_QS_PORTAL_KNOWN(val) \
	switch (val)	\
	{				\
	case 0:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_0, 1);\
		break; \
	case 1:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_1, 1);\
		break; \
	case 2:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_2, 1);\
		break; \
	case 3:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_3, 1);\
		break; \
	case 4:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_4, 1);\
		break; \
	case 5:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_5, 1);\
		break; \
	case 6:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_6, 1);\
		break; \
	case 7:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_7, 1);\
		break; \
	case 8:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_8, 1);\
		break; \
	case 9:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_9, 1);\
		break; \
	case 10:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_10, 1);\
		break; \
	case 11:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_11, 1);\
		break; \
	case 12:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_12, 1);\
		break; \
	case 13:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_13, 1);\
		break; \
	case 14:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_14, 1);\
		break; \
	case 15:			\
		SET_GAME_STATE (KNOW_QS_PORTAL_15, 1);\
		break; \
	default: \
	break;\
	}

// Values for GAME_STATE.glob_flags:
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

extern CONTEXT RadarContext;

extern void FreeSC2Data (void);
extern BOOLEAN LoadSC2Data (void);

extern void InitGlobData (void);

BOOLEAN inFullGame (void);
BOOLEAN inSuperMelee (void);
//BOOLEAN inBattle (void);
//BOOLEAN inInterPlanetary (void);
//BOOLEAN inSolarSystem (void);
//BOOLEAN inOrbit (void);
BOOLEAN inHQSpace (void);
BOOLEAN inHyperSpace (void);
BOOLEAN inQuasiSpace (void);

// Difficulty
#define NORM 0
#define EASY 1
#define HARD 2
#define DIFFICULTY (GLOBAL_SIS(Difficulty) ? GLOBAL_SIS(Difficulty) : NORM)
#define DIF_CASE(a,b,c) (DIFFICULTY == NORM ? (a) : (DIFFICULTY == EASY ? (b) : (c)))
#define DIF_NORM (DIFFICULTY == NORM ? true : false)
#define DIF_EASY (DIFFICULTY == EASY ? true : false)
#define DIF_HARD (DIFFICULTY == HARD ? true : false)
#define IF_NORM(a,b) (!DIF_NORM ? (a) : (b))
#define IF_EASY(a,b) (!DIF_EASY ? (a) : (b))
#define IF_HARD(a,b) (!DIF_HARD ? (a) : (b))
#define DIF_STR(a) ((a) == NORM ? "Normal" : ((a) == EASY ? "Easy" : "Hard"))

// Extended
#define EXTENDED (GLOBAL_SIS(Extended) ? TRUE : FALSE)
#define EXT_STR(a) ((a) ? "True" : "False")

// Nomad
#define NOMAD (GLOBAL_SIS(Nomad) ? TRUE : FALSE)
#define NOMAD_STR(a) ((a) ? "True" : "False")

extern BOOLEAN InitGameStructures (void);
extern void UninitGameStructures (void);

#if defined(__cplusplus)
}
#endif

#endif /* UQM_GLOBDATA_H_ */

