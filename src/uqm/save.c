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

#include <assert.h>

#include "save.h"

#include "build.h"
#include "controls.h"
#include "starmap.h"
#include "encount.h"
#include "libs/file.h"
#include "gamestr.h"
#include "globdata.h"
#include "options.h"
#include "races.h"
#include "shipcont.h"
#include "setup.h"
#include "state.h"
#include "grpintrn.h"
#include "util.h"
#include "hyper.h"
		// for SaveSisHyperState()
#include "planets/planets.h"
		// for SaveSolarSysLocation() and tests
#include "libs/inplib.h"
#include "libs/log.h"
#include "libs/memlib.h"

// Status boolean. If for some insane reason you need to
// save games in different threads, you'll need to
// protect your calls to SaveGame with a mutex.

// It's arguably over-paranoid to check for error on
// every single write, but this preserves the older
// behavior.

static BOOLEAN io_ok = TRUE;

// This defines the order and the number of bits in which the game state
// properties are saved.
const GameStateBitMap gameStateBitMap[] = {
		/* Shofixti states */
	{ "SHOFIXTI_VISITS", 3 },
	{ "SHOFIXTI_STACK1", 2 },
	{ "SHOFIXTI_STACK2", 3 },
	{ "SHOFIXTI_STACK3", 2 },
	{ "SHOFIXTI_KIA", 1 },
	{ "SHOFIXTI_BRO_KIA", 1 },
	{ "SHOFIXTI_RECRUITED", 1 },

	{ "SHOFIXTI_MAIDENS", 1 }, /* Did you find the babes yet? */
	{ "MAIDENS_ON_SHIP", 1 },
	{ "BATTLE_SEGUE", 1 },
			/* Set to 0 in init_xxx_comm() if communications directly
			 * follows an encounter.
			 * Set to 1 in init_xxx_comm() if the player gets to decide
			 * whether to attack or talk.
			 * Set to 1 in communication when battle follows the
			 * communication. It is still valid when uninit_xxx_comm() gets
			 * called after combat or communication.
			 */
	{ "PLANETARY_LANDING", 1 },
	{ "PLANETARY_CHANGE", 1 },
			/* Flag set to 1 when the planet information for the current
			 * world is changed since it was last saved to the starinfo.dat
			 * file. Set when picking up bio, mineral, or energy nodes.
			 * When there's no current world, it should be 0.
			 */

		/* Spathi states */
	{ "SPATHI_VISITS", 3 },
	{ "SPATHI_HOME_VISITS", 3 },
	{ "FOUND_PLUTO_SPATHI", 2 },
			/* 0 - Haven't met Fwiffo.
			 * 1 - Met Fwiffo on Pluto, now talking to him.
			 * 2 - Met Fwiffo on Pluto, after dialog.
			 * 3 - Met Fwiffo, and have reported to the Safe Ones on
			 *     the Spathi moon that he was either killed, or that
			 *     you have him on board.
			 */
	{ "SPATHI_SHIELDED_SELVES", 1 },
	{ "SPATHI_CREATURES_EXAMINED", 1 },
	{ "SPATHI_CREATURES_ELIMINATED", 1 },
	{ "UMGAH_BROADCASTERS", 1 },
	{ "SPATHI_MANNER", 2 },
	{ "SPATHI_QUEST", 1 },
	{ "LIED_ABOUT_CREATURES", 2 },
	{ "SPATHI_PARTY", 1 },
	{ "KNOW_SPATHI_PASSWORD", 1 },

	{ "ILWRATH_HOME_VISITS", 3 },
	{ "ILWRATH_CHMMR_VISITS", 1 },

	{ "ARILOU_SPACE", 1 },
			/* 0 if the periodically opening QuasiSpace portal is
			 * closed or closing.
			 * 1 if the periodically opening QuasiSpace portal is
			 * open or opening.
			 */
	{ "ARILOU_SPACE_SIDE", 2 },
			/* 0 if in HyperSpace and not just emerged from the periodically
			 * opening QuasiSpace portal.
			 * 1 if in HyperSpace and just emerged from the periodically
			 * QuasiSpace portal (still on the portal).
			 * 2 if in QuasiSpace and just emerged from the periodically
			 * opening portal (still on the portal).
			 * 3 if in QuasiSpace and not just emerged from the
			 * periodically opening portal.
			 */
	{ "ARILOU_SPACE_COUNTER", 4 },
			/* Keeps track of how far the periodically opening QuasiSpace
			 * portal is open. (This determines the image)
			 * 0 <= ARILOU_SPACE_COUNTER <= 9
			 * 0 means totally closed.
			 * 9 means completely open.
			 */

	{ "LANDER_SHIELDS", 4 },

	{ "MET_MELNORME", 1 },
	{ "MELNORME_RESCUE_REFUSED", 1 },
	{ "MELNORME_RESCUE_COUNT", 3 },
	{ "TRADED_WITH_MELNORME", 1 },
	{ "WHY_MELNORME_PURPLE", 1 },
	{ "MELNORME_CREDIT0", 8 },
	{ "MELNORME_CREDIT1", 8 },
	{ "MELNORME_BUSINESS_COUNT", 2 },
	{ "MELNORME_YACK_STACK0", 2 },
	{ "MELNORME_YACK_STACK1", 2 },
	{ "MELNORME_YACK_STACK2", 4 },
	{ "MELNORME_YACK_STACK3", 3 },
	{ "MELNORME_YACK_STACK4", 2 },
	{ "WHY_MELNORME_BLUE", 1 },
	{ "MELNORME_ANGER", 2 },
	{ "MELNORME_MIFFED_COUNT", 2 },
	{ "MELNORME_PISSED_COUNT", 2 },
	{ "MELNORME_HATE_COUNT", 2 },

	{ "PROBE_MESSAGE_DELIVERED", 1 },
	{ "PROBE_ILWRATH_ENCOUNTER", 1 },

	{ "STARBASE_AVAILABLE", 1 },
	{ "STARBASE_VISITED", 1 },
	{ "RADIOACTIVES_PROVIDED", 1 },
	{ "LANDERS_LOST", 1 },
	{ "GIVEN_FUEL_BEFORE", 1 },

	{ "AWARE_OF_SAMATRA", 1 },
	{ "YEHAT_CAVALRY_ARRIVED", 1 },
	{ "URQUAN_MESSED_UP", 1 },

	{ "MOONBASE_DESTROYED", 1 },
	{ "WILL_DESTROY_BASE", 1 },

	{ "WIMBLIS_TRIDENT_ON_SHIP", 1 },
	{ "GLOWING_ROD_ON_SHIP", 1 },

	{ "KOHR_AH_KILLED_ALL", 1 },

	{ "STARBASE_YACK_STACK1", 1 },

	{ "DISCUSSED_PORTAL_SPAWNER", 1 },
	{ "DISCUSSED_TALKING_PET", 1 },
	{ "DISCUSSED_UTWIG_BOMB", 1 },
	{ "DISCUSSED_SUN_EFFICIENCY", 1 },
	{ "DISCUSSED_ROSY_SPHERE", 1 },
	{ "DISCUSSED_AQUA_HELIX", 1 },
	{ "DISCUSSED_CLEAR_SPINDLE", 1 },
	{ "DISCUSSED_ULTRON", 1 },
	{ "DISCUSSED_MAIDENS", 1 },
	{ "DISCUSSED_UMGAH_HYPERWAVE", 1 },
	{ "DISCUSSED_BURVIX_HYPERWAVE", 1 },
	{ "SYREEN_WANT_PROOF", 1 },
	{ "PLAYER_HAVING_SEX", 1 },
	{ "MET_ARILOU", 1 },
	{ "DISCUSSED_TAALO_PROTECTOR", 1 },
	{ "DISCUSSED_EGG_CASING0", 1 },
	{ "DISCUSSED_EGG_CASING1", 1 },
	{ "DISCUSSED_EGG_CASING2", 1 },
	{ "DISCUSSED_SYREEN_SHUTTLE", 1 },
	{ "DISCUSSED_VUX_BEAST", 1 },
	{ "DISCUSSED_DESTRUCT_CODE", 1 },
	{ "DISCUSSED_URQUAN_WARP", 1 },
	{ "DISCUSSED_ARTIFACT_2", 1 },
	{ "DISCUSSED_ARTIFACT_3", 1 },

	{ "ATTACKED_DRUUGE", 1 },

	{ "NEW_ALLIANCE_NAME", 2 },

	{ "PORTAL_COUNTER", 4 },
			/* Set to 1 when the player opens a QuasiSpace portal.
			 * It will then be increased to 10, at which time
			 * the portal is completely open. (This determines the image).
			 */

	{ "BURVIXESE_BROADCASTERS", 1 },
	{ "BURV_BROADCASTERS_ON_SHIP", 1 },

	{ "UTWIG_BOMB", 1 },
	{ "UTWIG_BOMB_ON_SHIP", 1 },

	{ "AQUA_HELIX", 1 },
	{ "AQUA_HELIX_ON_SHIP", 1 },

	{ "SUN_DEVICE", 1 },
	{ "SUN_DEVICE_ON_SHIP", 1 },

	{ "TAALO_PROTECTOR", 1 },
	{ "TAALO_PROTECTOR_ON_SHIP", 1 },

	{ "SHIP_VAULT_UNLOCKED", 1 },
	{ "SYREEN_SHUTTLE", 1 },

	{ "PORTAL_KEY", 1 },
	{ "PORTAL_KEY_ON_SHIP", 1 },

	{ "VUX_BEAST", 1 },
	{ "VUX_BEAST_ON_SHIP", 1 },

	{ "TALKING_PET", 1 },
	{ "TALKING_PET_ON_SHIP", 1 },

	{ "MOONBASE_ON_SHIP", 1 },

	{ "KOHR_AH_FRENZY", 1 },
	{ "KOHR_AH_VISITS", 2 },
	{ "KOHR_AH_BYES", 1 },

	{ "SLYLANDRO_HOME_VISITS", 3 },
	{ "DESTRUCT_CODE_ON_SHIP", 1 },

	{ "ILWRATH_VISITS", 3 },
	{ "ILWRATH_DECEIVED", 1 },
	{ "FLAGSHIP_CLOAKED", 1 },

	{ "MYCON_VISITS", 3 },
	{ "MYCON_HOME_VISITS", 3 },
	{ "MYCON_AMBUSH", 1 },
	{ "MYCON_FELL_FOR_AMBUSH", 1 },
			/* Set to 1 when the Mycon have been told about Organon
			 * and are moving towards it.
			 */

	{ "GLOBAL_FLAGS_AND_DATA", 8 },
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

	{ "ORZ_VISITS", 3 },
	{ "TAALO_VISITS", 3 },
	{ "ORZ_MANNER", 2 },

	{ "PROBE_EXHIBITED_BUG", 1 },
	{ "CLEAR_SPINDLE_ON_SHIP", 1 },

	{ "URQUAN_VISITS", 3 },
	{ "PLAYER_HYPNOTIZED", 1 },

	{ "VUX_VISITS", 3 },
	{ "VUX_HOME_VISITS", 3 },
	{ "ZEX_VISITS", 3 },
	{ "ZEX_IS_DEAD", 1 },
	{ "KNOW_ZEX_WANTS_MONSTER", 1 },

	{ "UTWIG_VISITS", 3 },
	{ "UTWIG_HOME_VISITS", 3 },
	{ "BOMB_VISITS", 3 },
	{ "ULTRON_CONDITION", 3 },
			/* 0 if the Supox still have the Ultron
			 * 1 if the Captain has the Ultron, completely broken
			 * 2 if the Captain has the Ultron, with 1 fix
			 * 3 if the Captain has the Ultron, with 2 fixes
			 * 4 if the Captain has the Ultron, completely restored
			 * 5 if the Ultron has been returned to the Utwig
			 */
	{ "UTWIG_HAVE_ULTRON", 1 },
	{ "BOMB_UNPROTECTED", 1 },

	{ "TAALO_UNPROTECTED", 1 },

	{ "TALKING_PET_VISITS", 3 },
	{ "TALKING_PET_HOME_VISITS", 3 },
	{ "UMGAH_ZOMBIE_BLOBBIES", 1 },
			/* The Umgah have come under the influence of the Talking Pet */
	{ "KNOW_UMGAH_ZOMBIES", 1 },
			/* The Captain is aware that something is up with the Umgah */

	{ "ARILOU_VISITS", 3 },
	{ "ARILOU_HOME_VISITS", 3 },
	{ "KNOW_ARILOU_WANT_WRECK", 1 },
	{ "ARILOU_CHECKED_UMGAH", 2 },
	{ "PORTAL_SPAWNER", 1 },
	{ "PORTAL_SPAWNER_ON_SHIP", 1 },

	{ "UMGAH_VISITS", 3 },
	{ "UMGAH_HOME_VISITS", 3 },
	{ "MET_NORMAL_UMGAH", 1 },

	{ "SYREEN_HOME_VISITS", 3 },
	{ "SYREEN_SHUTTLE_ON_SHIP", 1 },
	{ "KNOW_SYREEN_VAULT", 1 },

	{ "EGG_CASE0_ON_SHIP", 1 },
	{ "SUN_DEVICE_UNGUARDED", 1 },

	{ "ROSY_SPHERE_ON_SHIP", 1 },
			/* The Rosy Sphere is aboard the flagship, i.e. It has been
			 * acquired from the Druuge, but not yet inserted in the broken
			 * Ultron. cf. ROSY_SPHERE */

	{ "CHMMR_HOME_VISITS", 3 },
	{ "CHMMR_EMERGING", 1 },
	{ "CHMMR_UNLEASHED", 1 },
	{ "CHMMR_BOMB_STATE", 2 },
			/* 0 - Nothing is known about the Precursor Bomb.
			 * 1 - The captain knows from the Chmmr that some extremely
			 *     powerful weapon is needed to destroy the Sa-Matra.
			 * 2 - Installation of the precursor bomb has started.
			 * 3 - Left the starbase after installation of the Precursor bomb.
			 */

	{ "DRUUGE_DISCLAIMER", 1 },

	{ "YEHAT_VISITS", 3 },
	{ "YEHAT_REBEL_VISITS", 3 },
	{ "YEHAT_HOME_VISITS", 3 },
	{ "YEHAT_CIVIL_WAR", 1 },
	{ "YEHAT_ABSORBED_PKUNK", 1 },
	{ "YEHAT_SHIP_MONTH", 4 },
	{ "YEHAT_SHIP_DAY", 5 },
	{ "YEHAT_SHIP_YEAR", 5 },

	{ "CLEAR_SPINDLE", 1 },
	{ "PKUNK_VISITS", 3 },
	{ "PKUNK_HOME_VISITS", 3 },
	{ "PKUNK_SHIP_MONTH", 4 },
			/* The month in PKUNK_SHIP_YEAR that new ships are available
			 * from the Pkunk. */
	{ "PKUNK_SHIP_DAY", 5 },
			/* The day of the month in PKUNK_SHIP_MONTH in PKUNK_SHIP_YEAR
			 * that new ships are available. */
	{ "PKUNK_SHIP_YEAR", 5 },
			/* The year that new ships are available from the Pkunk
			 * (stored as an offset from the year the game starts). */
	{ "PKUNK_MISSION", 3 },

	{ "SUPOX_VISITS", 3 },
	{ "SUPOX_HOME_VISITS", 3 },

	{ "THRADD_VISITS", 3 },
	{ "THRADD_HOME_VISITS", 3 },
	{ "HELIX_VISITS", 3 },
	{ "HELIX_UNPROTECTED", 1 },
	{ "THRADD_CULTURE", 2 },
	{ "THRADD_MISSION", 3 },
			/* 0 if the Thraddash fleet hasn't left the Thraddash home world.
			 * 1 if the Thraddash are heading towards Kohr-Ah territory.
			 * 2 if the Thraddash are fighting the Kohr-Ah.
			 * 3 if the Thraddash are returning from Kohr-Ah territory.
			 * 4 if the Thraddash fleet is back at the Thraddash home world.
			 */

	{ "DRUUGE_VISITS", 3 },
	{ "DRUUGE_HOME_VISITS", 3 },
	{ "ROSY_SPHERE", 1 },
			/* The play has or has had the Rosy Sphere.
			 * cf. ROSY_SHERE_ON_SHIP */
	{ "SCANNED_MAIDENS", 1 },
	{ "SCANNED_FRAGMENTS", 1 },
	{ "SCANNED_CASTER", 1 },
	{ "SCANNED_SPAWNER", 1 },
	{ "SCANNED_ULTRON", 1 },

	{ "ZOQFOT_INFO", 2 },
	{ "ZOQFOT_HOSTILE", 1 },
	{ "ZOQFOT_HOME_VISITS", 3 },
	{ "MET_ZOQFOT", 1 },
	{ "ZOQFOT_DISTRESS", 2 },
			/* 0 if the Zoq-Fot-Pik aren't in distress
			 * 1 if the Zoq-Fot-Pik are under attack by the Kohr-Ah
			 * 2 if the Zoq-Fot-Pik have been destroyed because of this
			 *   attack (not by the Kohr-Ah final victory cleansing)
			 */

	{ "EGG_CASE1_ON_SHIP", 1 },
	{ "EGG_CASE2_ON_SHIP", 1 },
	{ "MYCON_SUN_VISITS", 3 },
	{ "ORZ_HOME_VISITS", 3 },

	{ "MELNORME_FUEL_PROCEDURE", 1 },
	{ "MELNORME_TECH_PROCEDURE", 1 },
	{ "MELNORME_INFO_PROCEDURE", 1 },

	{ "MELNORME_TECH_STACK", 4 },
			/* MELNORME_TECH_STACK is now unused */
	{ "MELNORME_EVENTS_INFO_STACK", 5 },
	{ "MELNORME_ALIEN_INFO_STACK", 5 },
	{ "MELNORME_HISTORY_INFO_STACK", 5 },

	{ "RAINBOW_WORLD0", 8 },
			/* Low byte of a bit array, one bit per rainbow world.
			 * Each bit is set if the rainbow world has been visited.
			 * The lowest bit is for the first star in the star_array
			 * with RAINBOW_DEFINED, and so on.
			 */
	{ "RAINBOW_WORLD1", 2 },
			/* High 2 bits of the bit array of which RAINBOW_WORLD0
			 * is the low byte.
			 */
	{ "MELNORME_RAINBOW_COUNT", 4 },
			/* The number of rainbow world locations sold to the Melnorme. */

	{ "USED_BROADCASTER", 1 },
	{ "BROADCASTER_RESPONSE", 1 },

	{ "IMPROVED_LANDER_SPEED", 1 },
	{ "IMPROVED_LANDER_CARGO", 1 },
	{ "IMPROVED_LANDER_SHOT", 1 },

	{ "MET_ORZ_BEFORE", 1 },
	{ "YEHAT_REBEL_TOLD_PKUNK", 1 },
	{ "PLAYER_HAD_SEX", 1 },
	{ "UMGAH_BROADCASTERS_ON_SHIP", 1 },

	{ "LIGHT_MINERAL_LOAD", 3 },
	{ "MEDIUM_MINERAL_LOAD", 3 },
	{ "HEAVY_MINERAL_LOAD", 3 },

	{ "STARBASE_BULLETS", 32 },

	{ "STARBASE_MONTH", 4 },
	{ "STARBASE_DAY", 5 },

	{ "CREW_SOLD_TO_DRUUGE0", 8 },
	{ "CREW_PURCHASED0", 8 },
	{ "CREW_PURCHASED1", 8 },

	{ "URQUAN_PROTECTING_SAMATRA", 1 },

	{ "THRADDASH_BODY_COUNT", 5 },

	{ "UTWIG_SUPOX_MISSION", 3 },
			/* 0 if the Utwig and Supox fleet haven't left their home world.
			 * 1 if the U&S are on their way towards the Kohr-Ah
			 * 2 if the U&S are fighting the Kohr-Ah (first 80 days)
			 * 3 does not occur
             * 4 if the U&S are fighting the Kohr-Ah (second 80 days)
			 * 5 if the U&S are returning home.
			 * 6 if the U&S are back at their home world.
			 */
	{ "SPATHI_INFO", 3 },

	{ "ILWRATH_INFO", 2 },
	{ "ILWRATH_GODS_SPOKEN", 4 },
	{ "ILWRATH_WORSHIP", 2 },
	{ "ILWRATH_FIGHT_THRADDASH", 1 },

	{ "READY_TO_CONFUSE_URQUAN", 1 },
	{ "URQUAN_HYPNO_VISITS", 1 },
	{ "MENTIONED_PET_COMPULSION", 1 },
	{ "URQUAN_INFO", 2 },
	{ "KNOW_URQUAN_STORY", 2 },

	{ "MYCON_INFO", 4 },
	{ "MYCON_RAMBLE", 5 },
	{ "KNOW_ABOUT_SHATTERED", 2 },
			/* 0 if the player doesn't known about shattered worlds
			 * 1 if the player has encountered a shattered world
			 * 2 if the player knows that shatterred worlds are caused
			 *   by Mycon deep children.
			 * 3 if the player has told the Syreen that Mycon Deep Children
			 *   cause shattered worlds. Proof doesn't have to be presented
			 *   yet at this time.
			 */
	{ "MYCON_INSULTS", 3 },
	{ "MYCON_KNOW_AMBUSH", 1 },
			/* Set to 1 when the Mycon have been butchered at Organon,
			 * just before the remaining Mycon head back home.
			 */

	{ "SYREEN_INFO", 2 },
	{ "KNOW_SYREEN_WORLD_SHATTERED", 1 },
	{ "SYREEN_KNOW_ABOUT_MYCON", 1 },

	{ "TALKING_PET_INFO", 3 },
	{ "TALKING_PET_SUGGESTIONS", 3 },
	{ "LEARNED_TALKING_PET", 1 },
	{ "DNYARRI_LIED", 1 },
			/* Set when the Talking Pet tells you his version of their
			 * race's history with the Ur-Quan.
			 * Cleared once you confront him about this lie.
			 */
	{ "SHIP_TO_COMPEL", 1 },

	{ "ORZ_GENERAL_INFO", 2 },
	{ "ORZ_PERSONAL_INFO", 3 },
	{ "ORZ_ANDRO_STATE", 2 },
	{ "REFUSED_ORZ_ALLIANCE", 1 },

	{ "PKUNK_MANNER", 2 },
			/* 0 not met the Pkunk
			 * 1 fought the Pkunk, but relations are still salvagable.
			 * 2 hostile relations with the Pkunk, no way back.
			 * 3 friendly relations with the Pkunk
			 */
	{ "PKUNK_ON_THE_MOVE", 1 },
	{ "PKUNK_FLEET", 2 },
	{ "PKUNK_MIGRATE", 2 },
	{ "PKUNK_RETURN", 1 },
	{ "PKUNK_WORRY", 2 },
	{ "PKUNK_INFO", 3 },
	{ "PKUNK_WAR", 2 },
	{ "PKUNK_FORTUNE", 3 },
	{ "PKUNK_MIGRATE_VISITS", 3 },
	{ "PKUNK_REASONS", 4 },
	{ "PKUNK_SWITCH", 1 },
	{ "PKUNK_SENSE_VICTOR", 1 },

	{ "KOHR_AH_REASONS", 2 },
	{ "KOHR_AH_PLEAD", 2 },
	{ "KOHR_AH_INFO", 2 },
	{ "KNOW_KOHR_AH_STORY", 2 },
	{ "KOHR_AH_SENSES_EVIL", 1 },
	{ "URQUAN_SENSES_EVIL", 1 },

	{ "SLYLANDRO_PROBE_VISITS", 3 },
	{ "SLYLANDRO_PROBE_THREAT", 2 },
	{ "SLYLANDRO_PROBE_WRONG", 2 },
	{ "SLYLANDRO_PROBE_ID", 2 },
	{ "SLYLANDRO_PROBE_INFO", 2 },
	{ "SLYLANDRO_PROBE_EXIT", 2 },

	{ "UMGAH_HOSTILE", 1 },
	{ "UMGAH_EVIL_BLOBBIES", 1 },
	{ "UMGAH_MENTIONED_TRICKS", 2 },

	{ "BOMB_CARRIER", 1 },
			/* 0 when the flagship is not in battle, or it doesn't have the
			 *   enhanced precursor bomb installed.
			 * 1 when the flagship is in battle and the bomb is installed.
			 * This determines whether you can flee (if the warp escape unit
			 * is installed at all), and whether taking the ship into the
			 * Sa-Matra defense structure will trigger the end of the game.
			 */
	
	{ "THRADD_MANNER", 1 },
	{ "THRADD_INTRO", 2 },
	{ "THRADD_DEMEANOR", 3 },
	{ "THRADD_INFO", 2 },
	{ "THRADD_BODY_LEVEL", 2 },
	{ "THRADD_MISSION_VISITS", 1 },
	{ "THRADD_STACK_1", 3 },
	{ "THRADD_HOSTILE_STACK_2", 1 },
	{ "THRADD_HOSTILE_STACK_3", 1 },
	{ "THRADD_HOSTILE_STACK_4", 1 },
	{ "THRADD_HOSTILE_STACK_5", 1 },

	{ "CHMMR_STACK", 2 },

	{ "ARILOU_MANNER", 2 },
	{ "NO_PORTAL_VISITS", 1 },
	{ "ARILOU_STACK_1", 2 },
	{ "ARILOU_STACK_2", 1 },
	{ "ARILOU_STACK_3", 2 },
	{ "ARILOU_STACK_4", 1 },
	{ "ARILOU_STACK_5", 2 },
	{ "ARILOU_INFO", 2 },
	{ "ARILOU_HINTS", 2 },

	{ "DRUUGE_MANNER", 1 },
	{ "DRUUGE_SPACE_INFO", 2 },
	{ "DRUUGE_HOME_INFO", 2 },
	{ "DRUUGE_SALVAGE", 1 },
	{ "KNOW_DRUUGE_SLAVERS", 2 },
	{ "FRAGMENTS_BOUGHT", 2 },

	{ "ZEX_STACK_1", 2 },
	{ "ZEX_STACK_2", 2 },
	{ "ZEX_STACK_3", 2 },

	{ "VUX_INFO", 2 },
	{ "VUX_STACK_1", 4 },
	{ "VUX_STACK_2", 2 },
	{ "VUX_STACK_3", 2 },
	{ "VUX_STACK_4", 2 },

	{ "SHOFIXTI_STACK4", 2 },

	{ "YEHAT_REBEL_INFO", 3 },
	{ "YEHAT_ROYALIST_INFO", 1 },
	{ "YEHAT_ROYALIST_TOLD_PKUNK", 1 },
	{ "NO_YEHAT_ALLY_HOME", 1 },
	{ "NO_YEHAT_HELP_HOME", 1 },
	{ "NO_YEHAT_INFO", 1 },
	{ "NO_YEHAT_ALLY_SPACE", 2 },
	{ "NO_YEHAT_HELP_SPACE", 2 },

	{ "ZOQFOT_KNOW_MASK", 4 },

	{ "SUPOX_HOSTILE", 1 },
	{ "SUPOX_INFO", 1 },
	{ "SUPOX_WAR_NEWS", 2 },
	{ "SUPOX_ULTRON_HELP", 1 },
	{ "SUPOX_STACK1", 3 },
	{ "SUPOX_STACK2", 2 },

	{ "UTWIG_HOSTILE", 1 },
	{ "UTWIG_INFO", 1 },
	{ "UTWIG_WAR_NEWS", 2 },
	{ "UTWIG_STACK1", 3 },
	{ "UTWIG_STACK2", 2 },
	{ "BOMB_INFO", 1 },
	{ "BOMB_STACK1", 2 },
	{ "BOMB_STACK2", 2 },

	{ "SLYLANDRO_KNOW_BROKEN", 1 },
	{ "PLAYER_KNOWS_PROBE", 1 },
	{ "PLAYER_KNOWS_PROGRAM", 1 },
	{ "PLAYER_KNOWS_EFFECTS", 1 },
	{ "PLAYER_KNOWS_PRIORITY", 1 },
	{ "SLYLANDRO_STACK1", 3 },
	{ "SLYLANDRO_STACK2", 1 },
	{ "SLYLANDRO_STACK3", 2 },
	{ "SLYLANDRO_STACK4", 2 },
	{ "SLYLANDRO_STACK5", 1 },
	{ "SLYLANDRO_STACK6", 1 },
	{ "SLYLANDRO_STACK7", 2 },
	{ "SLYLANDRO_STACK8", 2 },
	{ "SLYLANDRO_STACK9", 2 },
	{ "SLYLANDRO_KNOW_EARTH", 1 },
	{ "SLYLANDRO_KNOW_EXPLORE", 1 },
	{ "SLYLANDRO_KNOW_GATHER", 1 },
	{ "SLYLANDRO_KNOW_URQUAN", 2 },
	{ "RECALL_VISITS", 2 },

	{ "SLYLANDRO_MULTIPLIER", 3 },
	{ "KNOW_SPATHI_QUEST", 1 },
	{ "KNOW_SPATHI_EVIL", 1 },

	{ "BATTLE_PLANET", 8 },
	{ "ESCAPE_COUNTER", 8 },

	{ "CREW_SOLD_TO_DRUUGE1", 8 },
	{ "PKUNK_DONE_WAR", 1 },

	{ "SYREEN_STACK0", 2 },
	{ "SYREEN_STACK1", 2 },
	{ "SYREEN_STACK2", 2 },

	{ "REFUSED_ULTRON_AT_BOMB", 1 },
	{ "NO_TRICK_AT_SUN", 1 },

	{ "SPATHI_STACK0", 2 },
	{ "SPATHI_STACK1", 1 },
	{ "SPATHI_STACK2", 1 },

	{ "ORZ_STACK0", 1 },
	{ "ORZ_STACK1", 1 },

	{ "SHOFIXTI_GRPOFFS", 32 },
	{ "ZOQFOT_GRPOFFS", 32 },
	{ "MELNORME0_GRPOFFS", 32 },
	{ "MELNORME1_GRPOFFS", 32 },
	{ "MELNORME2_GRPOFFS", 32 },
	{ "MELNORME3_GRPOFFS", 32 },
	{ "MELNORME4_GRPOFFS", 32 },
	{ "MELNORME5_GRPOFFS", 32 },
	{ "MELNORME6_GRPOFFS", 32 },
	{ "MELNORME7_GRPOFFS", 32 },
	{ "MELNORME8_GRPOFFS", 32 },
	{ "URQUAN_PROBE_GRPOFFS", 32 },
	{ "COLONY_GRPOFFS", 32 },
	{ "SAMATRA_GRPOFFS", 32 },
	
	{ "AUTOPILOT_OK", 1 },
	
	{ "KNOW_QS_PORTAL_0", 1 },
	{ "KNOW_QS_PORTAL_1", 1 },
	{ "KNOW_QS_PORTAL_2", 1 },
	{ "KNOW_QS_PORTAL_3", 1 },
	{ "KNOW_QS_PORTAL_4", 1 },
	{ "KNOW_QS_PORTAL_5", 1 },
	{ "KNOW_QS_PORTAL_6", 1 },
	{ "KNOW_QS_PORTAL_7", 1 },
	{ "KNOW_QS_PORTAL_8", 1 },
	{ "KNOW_QS_PORTAL_9", 1 },
	{ "KNOW_QS_PORTAL_10", 1 },
	{ "KNOW_QS_PORTAL_11", 1 },
	{ "KNOW_QS_PORTAL_12", 1 },
	{ "KNOW_QS_PORTAL_13", 1 },
	{ "KNOW_QS_PORTAL_14", 1 },
	{ "KNOW_QS_PORTAL_15", 1 },
	
	{ NULL, 0 },
};


// XXX: these should handle endian conversions later
static inline void
write_8 (void *fp, BYTE v)
{
	if (io_ok)
		if (WriteResFile (&v, 1, 1, fp) != 1)
			io_ok = FALSE;
}

static inline void
write_16 (void *fp, UWORD v)
{
	write_8 (fp, (BYTE)( v        & 0xff));
	write_8 (fp, (BYTE)((v >>  8) & 0xff));
}

static inline void
write_32 (void *fp, DWORD v)
{
	write_8 (fp, (BYTE)( v        & 0xff));
	write_8 (fp, (BYTE)((v >>  8) & 0xff));
	write_8 (fp, (BYTE)((v >> 16) & 0xff));
	write_8 (fp, (BYTE)((v >> 24) & 0xff));
}

static inline void
write_a8 (void *fp, const BYTE *ar, COUNT count)
{
	if (io_ok)
		if (WriteResFile (ar, 1, count, fp) != count)
			io_ok = FALSE;
}

static inline void
write_str (void *fp, const char *str, COUNT count)
{
	// no type conversion needed for strings
	write_a8 (fp, (const BYTE *)str, count);
}

static inline void
write_a16 (void *fp, const UWORD *ar, COUNT count)
{
	for ( ; count > 0; --count, ++ar)
	{
		if (!io_ok)
			break;
		write_16 (fp, *ar);
	}
}

static void
SaveShipQueue (uio_Stream *fh, QUEUE *pQueue, DWORD tag)
{
	COUNT num_links;
	HSHIPFRAG hStarShip;

	num_links = CountLinks (pQueue);
	if (num_links == 0)
		return;
	write_32 (fh, tag);
	write_32 (fh, num_links * 11); // Size of chunk: each entry is 11 bytes long.

	hStarShip = GetHeadLink (pQueue);
	while (num_links--)
	{
		HSHIPFRAG hNextShip;
		SHIP_FRAGMENT *FragPtr;
		COUNT Index;

		FragPtr = LockShipFrag (pQueue, hStarShip);
		hNextShip = _GetSuccLink (FragPtr);

		Index = FragPtr->race_id;
		// Write the number identifying this ship type.
		// See races.h; look for the enum containing NUM_AVAILABLE_RACES.
		write_16 (fh, Index);

		// Write SHIP_FRAGMENT elements
		write_8  (fh, FragPtr->captains_name_index);
		write_8  (fh, FragPtr->race_id);
		write_8  (fh, FragPtr->index);
		write_16 (fh, FragPtr->crew_level);
		write_16 (fh, FragPtr->max_crew);
		write_8  (fh, FragPtr->energy_level);
		write_8  (fh, FragPtr->max_energy);

		UnlockShipFrag (pQueue, hStarShip);
		hStarShip = hNextShip;
	}
}

static void
SaveRaceQueue (uio_Stream *fh, QUEUE *pQueue)
{
	COUNT num_links;
	HFLEETINFO hFleet;

	num_links = CountLinks (pQueue);
	if (num_links == 0)
		return;
	write_32 (fh, RACE_Q_TAG);
	// Write chunk size: 30 bytes per entry
	write_32 (fh, num_links * 30);

	hFleet = GetHeadLink (pQueue);
	while (num_links--)
	{
		HFLEETINFO hNextFleet;
		FLEET_INFO *FleetPtr;
		COUNT Index;

		FleetPtr = LockFleetInfo (pQueue, hFleet);
		hNextFleet = _GetSuccLink (FleetPtr);

		Index = GetIndexFromStarShip (pQueue, hFleet);
		// The index is the position in the queue.
		write_16 (fh, Index);

		// Write FLEET_INFO elements
		write_16 (fh, FleetPtr->allied_state);
		write_8  (fh, FleetPtr->days_left);
		write_8  (fh, FleetPtr->growth_fract);
		write_16 (fh, FleetPtr->crew_level);
		write_16 (fh, FleetPtr->max_crew);
		write_8  (fh, FleetPtr->growth);
		write_8  (fh, FleetPtr->max_energy);
		write_16 (fh, FleetPtr->loc.x);
		write_16 (fh, FleetPtr->loc.y);

		write_16 (fh, FleetPtr->actual_strength);
		write_16 (fh, FleetPtr->known_strength);
		write_16 (fh, FleetPtr->known_loc.x);
		write_16 (fh, FleetPtr->known_loc.y);
		write_8  (fh, FleetPtr->growth_err_term);
		write_8  (fh, FleetPtr->func_index);
		write_16 (fh, FleetPtr->dest_loc.x);
		write_16 (fh, FleetPtr->dest_loc.y);

		UnlockFleetInfo (pQueue, hFleet);
		hFleet = hNextFleet;
	}
}

static void
SaveGroupQueue (uio_Stream *fh, QUEUE *pQueue)
{
	HIPGROUP hGroup, hNextGroup;
	COUNT num_links;

	num_links = CountLinks (pQueue);
	if (num_links == 0)
		return;
	write_32 (fh, IP_GRP_Q_TAG);
	write_32 (fh, num_links * 13); // 13 bytes per element right now

	for (hGroup = GetHeadLink (pQueue); hGroup; hGroup = hNextGroup)
	{
		IP_GROUP *GroupPtr;

		GroupPtr = LockIpGroup (pQueue, hGroup);
		hNextGroup = _GetSuccLink (GroupPtr);

		write_16 (fh, GroupPtr->group_counter);
		write_8  (fh, GroupPtr->race_id);
		write_8  (fh, GroupPtr->sys_loc);
		write_8  (fh, GroupPtr->task);
		write_8  (fh, GroupPtr->in_system); /* was crew_level */
		write_8  (fh, GroupPtr->dest_loc);
		write_8  (fh, GroupPtr->orbit_pos);
		write_8  (fh, GroupPtr->group_id); /* was max_energy */
		write_16 (fh, GroupPtr->loc.x);
		write_16 (fh, GroupPtr->loc.y);

		UnlockIpGroup (pQueue, hGroup);
	}
}

static void
SaveEncounters (uio_Stream *fh)
{
	COUNT num_links;
	HENCOUNTER hEncounter;
	num_links = CountLinks (&GLOBAL (encounter_q));
	if (num_links == 0)
		return;
	write_32 (fh, ENCOUNTERS_TAG);
	write_32 (fh, 65 * num_links);

	hEncounter = GetHeadLink (&GLOBAL (encounter_q));
	while (num_links--)
	{
		HENCOUNTER hNextEncounter;
		ENCOUNTER *EncounterPtr;
		COUNT i;

		LockEncounter (hEncounter, &EncounterPtr);
		hNextEncounter = GetSuccEncounter (EncounterPtr);

		write_16  (fh, EncounterPtr->transition_state);
		write_16  (fh, EncounterPtr->origin.x);
		write_16  (fh, EncounterPtr->origin.y);
		write_16  (fh, EncounterPtr->radius);
		// former STAR_DESC fields
		write_16  (fh, EncounterPtr->loc_pt.x);
		write_16  (fh, EncounterPtr->loc_pt.y);
		write_8   (fh, EncounterPtr->race_id);
		write_8   (fh, EncounterPtr->num_ships);
		write_8   (fh, EncounterPtr->flags);

		// Save each entry in the BRIEF_SHIP_INFO array
		for (i = 0; i < MAX_HYPER_SHIPS; i++)
		{
			const BRIEF_SHIP_INFO *ShipInfo = &EncounterPtr->ShipList[i];

			write_8   (fh, ShipInfo->race_id);
			write_16  (fh, ShipInfo->crew_level);
			write_16  (fh, ShipInfo->max_crew);
			write_8   (fh, ShipInfo->max_energy);
		}

		// Save the stuff after the BRIEF_SHIP_INFO array
		write_32  (fh, RES_DESCALE(EncounterPtr->log_x));
		write_32  (fh, RES_DESCALE(EncounterPtr->log_y));

		UnlockEncounter (hEncounter);
		hEncounter = hNextEncounter;
	}
}

static void
SaveEvents (uio_Stream *fh)
{
	COUNT num_links;
	HEVENT hEvent;
	num_links = CountLinks (&GLOBAL (GameClock.event_q));
	if (num_links == 0)
		return;
	write_32 (fh, EVENTS_TAG);
	write_32 (fh, num_links * 5); /* Event chunks are five bytes each */

	hEvent = GetHeadLink (&GLOBAL (GameClock.event_q));
	while (num_links--)
	{
		HEVENT hNextEvent;
		EVENT *EventPtr;

		LockEvent (hEvent, &EventPtr);
		hNextEvent = GetSuccEvent (EventPtr);

		write_8   (fh, EventPtr->day_index);
		write_8   (fh, EventPtr->month_index);
		write_16  (fh, EventPtr->year_index);
		write_8   (fh, EventPtr->func_index);

		UnlockEvent (hEvent);
		hEvent = hNextEvent;
	}
}

/* The clock state is folded in with the game state chunk. */
static void
SaveClockState (const CLOCK_STATE *ClockPtr, uio_Stream *fh)
{
	write_8   (fh, ClockPtr->day_index);
	write_8   (fh, ClockPtr->month_index);
	write_16  (fh, ClockPtr->year_index);
	write_16  (fh, ClockPtr->tick_count);
	write_16  (fh, ClockPtr->day_in_ticks);
}

/* Save out the game state chunks. There are two of these; the Global
 * State chunk is fixed size, but the Game State tag can be extended
 * by modders. */
static BOOLEAN
SaveGameState (const GAME_STATE *GSPtr, uio_Stream *fh)
{
	BYTE res_scale; // JMS

	if (LOBYTE (GSPtr->CurrentActivity) != IN_INTERPLANETARY)
		res_scale = RESOLUTION_FACTOR; 
	else
		res_scale = 0;

	write_32  (fh, GLOBAL_STATE_TAG);
	write_32  (fh, 75);
	write_8   (fh, GSPtr->glob_flags);
	write_8   (fh, GSPtr->CrewCost);
	write_8   (fh, GSPtr->FuelCost);
	write_a8  (fh, GSPtr->ModuleCost, NUM_MODULES);
	write_a8  (fh, GSPtr->ElementWorth, NUM_ELEMENT_CATEGORIES);
	write_16  (fh, GSPtr->CurrentActivity);

	SaveClockState (&GSPtr->GameClock, fh);

	write_16  (fh, GSPtr->autopilot.x);
	write_16  (fh, GSPtr->autopilot.y);
	write_16  (fh, GSPtr->ip_location.x);
	write_16  (fh, GSPtr->ip_location.y);
	/* STAMP ShipStamp */
	write_16  (fh, RES_DESCALE(GSPtr->ShipStamp.origin.x));
	write_16  (fh, RES_DESCALE(GSPtr->ShipStamp.origin.y));
	write_16  (fh, GSPtr->ShipFacing);
	write_8   (fh, GSPtr->ip_planet);
	write_8   (fh, GSPtr->in_orbit);

	/* VELOCITY_DESC velocity */
	write_16  (fh, GSPtr->velocity.TravelAngle >> res_scale);
	write_16  (fh, GSPtr->velocity.vector.width >> res_scale);
	write_16  (fh, GSPtr->velocity.vector.height >> res_scale);
	write_16  (fh, GSPtr->velocity.fract.width >> res_scale);
	write_16  (fh, GSPtr->velocity.fract.height >> res_scale);
	write_16  (fh, GSPtr->velocity.error.width >> res_scale);
	write_16  (fh, GSPtr->velocity.error.height >> res_scale);
	write_16  (fh, GSPtr->velocity.incr.width >> res_scale);
	write_16  (fh, GSPtr->velocity.incr.height >> res_scale);

	/* The Game state bits. Vanilla UQM uses 155 bytes here at
	 * present. Only the first 99 bytes are significant, though;
	 * the rest will be overwritten by the BtGp chunks. */
	write_32  (fh, GAME_STATE_TAG);
	{
		uint8 *buf = NULL;
		size_t bufSize;
		if (serialiseGameState (gameStateBitMap, &buf, &bufSize))
		{
			write_32  (fh, bufSize);
			write_a8  (fh, buf, bufSize);
			HFree(buf);
		}
		else
			return FALSE;
	}
	return TRUE;
}

/* This is folded into the Summary chunk */
static void
SaveSisState (const SIS_STATE *SSPtr, void *fp)
{
	write_32  (fp, RES_DESCALE(SSPtr->log_x));
	write_32  (fp, RES_DESCALE(SSPtr->log_y));
	write_32  (fp, SSPtr->ResUnits);
	write_32  (fp, SSPtr->FuelOnBoard);
	write_16  (fp, SSPtr->CrewEnlisted);
	write_16  (fp, SSPtr->TotalElementMass);
	write_16  (fp, SSPtr->TotalBioMass);
	write_a8  (fp, SSPtr->ModuleSlots, NUM_MODULE_SLOTS);
	write_a8  (fp, SSPtr->DriveSlots, NUM_DRIVE_SLOTS);
	write_a8  (fp, SSPtr->JetSlots, NUM_JET_SLOTS);
	write_8   (fp, SSPtr->NumLanders);
	write_a16 (fp, SSPtr->ElementAmounts, NUM_ELEMENT_CATEGORIES);

	write_str (fp, SSPtr->ShipName, SIS_NAME_SIZE);
	write_str (fp, SSPtr->CommanderName, SIS_NAME_SIZE);
	write_str (fp, SSPtr->PlanetName, SIS_NAME_SIZE);
	write_8   (fp, SSPtr->Difficulty);
	write_8   (fp, SSPtr->Extended);
	write_8   (fp, SSPtr->Nomad);
	write_32  (fp, SSPtr->Seed);
}

/* Write out the Summary Chunk. This is variable length because of the
   savegame name */
static void
SaveSummary (const SUMMARY_DESC *SummPtr, void *fp)
{
	write_32 (fp, SUMMARY_TAG);
	write_32 (fp, 160 + strlen(SummPtr->SaveName));
	SaveSisState (&SummPtr->SS, fp);

	write_8  (fp, SummPtr->Activity);
	write_8  (fp, SummPtr->Flags);
	write_8  (fp, SummPtr->day_index);
	write_8  (fp, SummPtr->month_index);
	write_16 (fp, SummPtr->year_index);
	write_8  (fp, SummPtr->MCreditLo);
	write_8  (fp, SummPtr->MCreditHi);
	write_8  (fp, SummPtr->NumShips);
	write_8  (fp, SummPtr->NumDevices);
	write_a8 (fp, SummPtr->ShipList, MAX_BUILT_SHIPS);
	write_a8 (fp, SummPtr->DeviceList, MAX_EXCLUSIVE_DEVICES);
	write_8  (fp, SummPtr->res_factor);
	write_a8 (fp, SummPtr->SaveName, strlen(SummPtr->SaveName));
}

/* Save the Star Description chunk. This is not to be confused with
 * the Star *Info* chunk, which records which planetary features you
 * have exploited with your lander */
static void
SaveStarDesc (const STAR_DESC *SDPtr, uio_Stream *fh)
{
	write_32 (fh, STAR_TAG);
	write_32 (fh, 8);
	write_16 (fh, SDPtr->star_pt.x);
	write_16 (fh, SDPtr->star_pt.y);
	write_8  (fh, SDPtr->Type);
	write_8  (fh, SDPtr->Index);
	write_8  (fh, SDPtr->Prefix);
	write_8  (fh, SDPtr->Postfix);
}

static void
PrepareSummary (SUMMARY_DESC *SummPtr, const char *name)
{
	SummPtr->SS = GlobData.SIS_state;

	SummPtr->Activity = LOBYTE (GLOBAL (CurrentActivity));
	switch (SummPtr->Activity)
	{
		case IN_HYPERSPACE:
			if (inQuasiSpace ())
				SummPtr->Activity = IN_QUASISPACE;
			break;
		case IN_INTERPLANETARY:
			// Get a better planet name for summary
			GetPlanetOrMoonName (SummPtr->SS.PlanetName,
					sizeof (SummPtr->SS.PlanetName));
			if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) == (BYTE)~0)
				SummPtr->Activity = IN_STARBASE;
			else if (playerInPlanetOrbit ())
				SummPtr->Activity = IN_PLANET_ORBIT;
			break;
		case IN_LAST_BATTLE:
			utf8StringCopy (SummPtr->SS.PlanetName,
					sizeof (SummPtr->SS.PlanetName),
					GAME_STRING (PLANET_NUMBER_BASE + 32)); // Sa-Matra
			break;
	}

	SummPtr->MCreditLo = GET_GAME_STATE (MELNORME_CREDIT0);
	SummPtr->MCreditHi = GET_GAME_STATE (MELNORME_CREDIT1);

	{
		HSHIPFRAG hStarShip, hNextShip;

		for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q)), SummPtr->NumShips = 0;
				hStarShip; hStarShip = hNextShip, ++SummPtr->NumShips)
		{
			SHIP_FRAGMENT *StarShipPtr;

			StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);
			hNextShip = _GetSuccLink (StarShipPtr);
			SummPtr->ShipList[SummPtr->NumShips] = StarShipPtr->race_id;
			UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);
		}
	}

	SummPtr->NumDevices = InventoryDevices (SummPtr->DeviceList,
			MAX_EXCLUSIVE_DEVICES);

	SummPtr->Flags = GET_GAME_STATE (LANDER_SHIELDS)
			| (GET_GAME_STATE (IMPROVED_LANDER_SPEED) << (4 + 0))
			| (GET_GAME_STATE (IMPROVED_LANDER_CARGO) << (4 + 1))
			| (GET_GAME_STATE (IMPROVED_LANDER_SHOT) << (4 + 2))
			| ((GET_GAME_STATE (CHMMR_BOMB_STATE) < 2 ? 0 : 1) << (4 + 3));

	SummPtr->day_index = GLOBAL (GameClock.day_index);
	SummPtr->month_index = GLOBAL (GameClock.month_index);
	SummPtr->year_index = GLOBAL (GameClock.year_index);
	SummPtr->SaveName[SAVE_NAME_SIZE-1] = 0;
	strncpy (SummPtr->SaveName, name, SAVE_NAME_SIZE-1);
	SummPtr->res_factor = RESOLUTION_FACTOR;
}

static void
SaveProblemMessage (STAMP *MsgStamp)
{
#define MAX_MSG_LINES 1
	RECT r = {{0, 0}, {0, 0}};
	COUNT i;
	TEXT t;
	UNICODE *ppStr[MAX_MSG_LINES];

	// TODO: This should probably just use DoPopupWindow()

	ppStr[0] = GAME_STRING (SAVEGAME_STRING_BASE + 2);

	SetContextFont (StarConFont);

	t.baseline.x = t.baseline.y = 0;
	t.align = ALIGN_CENTER;
	for (i = 0; i < MAX_MSG_LINES; ++i)
	{
		RECT tr;

		t.pStr = ppStr[i];
		if (*t.pStr == '\0')
			break;
		t.CharCount = (COUNT)~0;
		TextRect (&t, &tr, NULL);
		if (i == 0)
			r = tr;
		else
			BoxUnion (&tr, &r, &r);
		t.baseline.y += 11;
	}
	t.baseline.x = ((SIS_SCREEN_WIDTH >> 1) - (r.extent.width >> 1))
			- r.corner.x;
	t.baseline.y = ((SIS_SCREEN_HEIGHT >> 1) - (r.extent.height >> 1))
			- r.corner.y;
	r.corner.x += t.baseline.x - RES_SCALE(4);
	r.corner.y += t.baseline.y - RES_SCALE(4);
	r.extent.width += RES_SCALE(8);
	r.extent.height += RES_SCALE(8);

	*MsgStamp = SaveContextFrame (&r);

	BatchGraphics ();
	DrawStarConBox (&r, 2,
			BUILD_COLOR (MAKE_RGB15 (0x10, 0x10, 0x10), 0x19),
			BUILD_COLOR (MAKE_RGB15 (0x08, 0x08, 0x08), 0x1F),
			TRUE, BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08));
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x14, 0x14, 0x14), 0x0F));

	for (i = 0; i < MAX_MSG_LINES; ++i)
	{
		t.pStr = ppStr[i];
		if (*t.pStr == '\0')
			break;
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
		t.baseline.y += 11;
	}
	UnbatchGraphics ();
}

void
SaveProblem (void)
{
	STAMP s;
	CONTEXT OldContext;

	OldContext = SetContext (SpaceContext);
	SaveProblemMessage (&s);
	FlushGraphics ();

	WaitForAnyButton (TRUE, WAIT_INFINITE, FALSE);

	// Restore the screen under the message
	DrawStamp (&s);
	SetContext (OldContext);
	DestroyDrawable (ReleaseDrawable (s.frame));
}

static void
SaveFlagshipState (void)
{
	if (inHQSpace ())
	{
		// Player is in HyperSpace or QuasiSpace.
		SaveSisHyperState ();
	}
	else if (playerInSolarSystem ())
	{
		SaveSolarSysLocation ();
	}
}

static void
SaveStarInfo (uio_Stream *fh)
{
	GAME_STATE_FILE *fp;
	fp = OpenStateFile (STARINFO_FILE, "rb");
	if (fp)
	{
		DWORD flen = LengthStateFile (fp);
		if (flen % 4)
		{
			log_add (log_Warning, "Unexpected Star Info length! Expected an integral number of DWORDS.\n");
		}
		else
		{
			write_32 (fh, SCAN_TAG);
			write_32 (fh, flen);
			while (flen)
			{
				DWORD val;
				sread_32 (fp, &val);
				write_32 (fh, val);
				flen -= 4;
			}
		}
		CloseStateFile (fp);
	}
}

static void
SaveBattleGroup (GAME_STATE_FILE *fp, DWORD encounter_id, DWORD grpoffs, uio_Stream *fh)
{
	GROUP_HEADER h;
	DWORD size = 12;
	int i;
	SeekStateFile (fp, grpoffs, SEEK_SET);
	ReadGroupHeader (fp, &h);
	for (i = 1; i <= h.NumGroups; ++i)
	{
		BYTE NumShips;
		SeekStateFile (fp, h.GroupOffset[i], SEEK_SET);
		sread_8 (fp, NULL);
		sread_8 (fp, &NumShips);
		size += 2 + 10 * NumShips;
	}
	write_32 (fh, BATTLE_GROUP_TAG);
	write_32 (fh, size);
	write_32 (fh, encounter_id);
	write_8  (fh, (grpoffs && (GLOBAL (BattleGroupRef) == grpoffs)) ? 1 : 0); // current
	write_16 (fh, h.star_index);
	write_8  (fh, h.day_index);
	write_8  (fh, h.month_index);
	write_16 (fh, h.year_index);
	write_8  (fh, h.NumGroups);
	for (i = 1; i <= h.NumGroups; ++i)
	{
		int j;
		BYTE b;
		SeekStateFile (fp, h.GroupOffset[i], SEEK_SET);
		sread_8 (fp, &b); // Group race icon
		write_8 (fh, b);
		sread_8 (fp, &b); // NumShips
		write_8 (fh, b);
		for (j = 0; j < b; ++j)
		{
			BYTE race_outer;
			SHIP_FRAGMENT sf;
			sread_8 (fp, &race_outer);
			ReadShipFragment (fp, &sf);
			write_8  (fh, race_outer);
			write_8  (fh, sf.captains_name_index);
			write_8  (fh, sf.race_id);
			write_8  (fh, sf.index);
			write_16 (fh, sf.crew_level);
			write_16 (fh, sf.max_crew);
			write_8  (fh, sf.energy_level);
			write_8  (fh, sf.max_energy);
		}
	}
}

static DWORD
GetBattleGroupOffset (int encounterIndex)
{
	// The reason for this switch, even though the group offsets are
	// successive, is because GET_GAME_STATE is a #define, which stringizes
	// its argument.
	switch (encounterIndex)
	{
		case  1: return GET_GAME_STATE (SHOFIXTI_GRPOFFS);
		case  2: return GET_GAME_STATE (ZOQFOT_GRPOFFS);
		case  3: return GET_GAME_STATE (MELNORME0_GRPOFFS);
		case  4: return GET_GAME_STATE (MELNORME1_GRPOFFS);
		case  5: return GET_GAME_STATE (MELNORME2_GRPOFFS);
		case  6: return GET_GAME_STATE (MELNORME3_GRPOFFS);
		case  7: return GET_GAME_STATE (MELNORME4_GRPOFFS);
		case  8: return GET_GAME_STATE (MELNORME5_GRPOFFS);
		case  9: return GET_GAME_STATE (MELNORME6_GRPOFFS);
		case 10: return GET_GAME_STATE (MELNORME7_GRPOFFS);
		case 11: return GET_GAME_STATE (MELNORME8_GRPOFFS);
		case 12: return GET_GAME_STATE (URQUAN_PROBE_GRPOFFS);
		case 13: return GET_GAME_STATE (COLONY_GRPOFFS);
		case 14: return GET_GAME_STATE (SAMATRA_GRPOFFS);
		default:
			log_add (log_Warning, "SetBattleGroupOffset: invalid encounter "
					"index.\n");
			return 0;
	}
}

static void
SaveGroups (uio_Stream *fh)
{
	GAME_STATE_FILE *fp;
	fp = OpenStateFile (RANDGRPINFO_FILE, "rb");
	if (fp && LengthStateFile (fp) > 0)
	{
		GROUP_HEADER h;
		BYTE lastenc, count;
		int i;
		ReadGroupHeader (fp, &h);
		/* Group List */
		SeekStateFile (fp, h.GroupOffset[0], SEEK_SET);	
		sread_8 (fp, &lastenc);
		sread_8 (fp, &count);
		write_32 (fh, GROUP_LIST_TAG);
		write_32 (fh, 1 + 14 * count); // Chunk size
		write_8 (fh, lastenc);
		for (i = 0; i < count; ++i)
		{
			BYTE race_outer;
			IP_GROUP ip;
			sread_8 (fp, &race_outer);
			ReadIpGroup (fp, &ip);

			write_8  (fh, race_outer);
			write_16 (fh, ip.group_counter);
			write_8  (fh, ip.race_id);
			write_8  (fh, ip.sys_loc);
			write_8  (fh, ip.task);
			write_8  (fh, ip.in_system);
			write_8  (fh, ip.dest_loc);
			write_8  (fh, ip.orbit_pos);
			write_8  (fh, ip.group_id);
			write_16 (fh, ip.loc.x);
			write_16 (fh, ip.loc.y);
		}
		SaveBattleGroup (fp, 0, 0, fh);
		CloseStateFile (fp);
	}
	fp = OpenStateFile (DEFGRPINFO_FILE, "rb");
	if (fp && LengthStateFile (fp) > 0)
	{
		int encounter_index;
		for (encounter_index = 1; encounter_index < 15; encounter_index++)
		{
			DWORD grpoffs = GetBattleGroupOffset (encounter_index);
			if (grpoffs)
			{
				SaveBattleGroup (fp, encounter_index, grpoffs, fh);
			}
		}
		CloseStateFile (fp);
	}
}

// This function first writes to a memory file, and then writes the whole
// lot to the actual save file at once.
BOOLEAN
SaveGame (COUNT which_game, SUMMARY_DESC *SummPtr, const char *name)
{
	uio_Stream *out_fp;
	POINT pt;
	STAR_DESC SD;
	char file[PATH_MAX];
	if (CurStarDescPtr)
		SD = *CurStarDescPtr;
	else
		memset (&SD, 0, sizeof (SD));

	// XXX: Backup: SaveFlagshipState() overwrites ip_location
	pt = GLOBAL (ip_location);
	SaveFlagshipState ();
	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY
			&& !(GLOBAL (CurrentActivity)
			& (START_ENCOUNTER | START_INTERPLANETARY)))
		PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);

	// Write the memory file to the actual savegame file.
	sprintf (file, "uqmsave.%02u", which_game);
	if ((out_fp = res_OpenResFile (saveDir, file, "wb")))
	{
		io_ok = TRUE;
		write_32 (out_fp, SAVEFILE_TAG);

		PrepareSummary (SummPtr, name);
		SaveSummary (SummPtr, out_fp);

		if (!SaveGameState (&GlobData.Game_state, out_fp))
			io_ok = FALSE;

		// XXX: Restore
		GLOBAL (ip_location) = pt;
		// Only relevant when loading a game and must be cleaned
		GLOBAL (in_orbit) = 0;

		SaveRaceQueue (out_fp, &GLOBAL (avail_race_q));
		// START_INTERPLANETARY is only set when saving from Homeworld
		//   encounter screen. When the game is loaded, the
		//   GenerateOrbitalFunction for the current star system
		//   create the encounter anew and populate the npc queue.
		if (!(GLOBAL (CurrentActivity) & START_INTERPLANETARY))
		{
			if (GLOBAL (CurrentActivity) & START_ENCOUNTER)
				SaveShipQueue (out_fp, &GLOBAL (npc_built_ship_q), NPC_SHIP_Q_TAG);
			else if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY)
				// XXX: Technically, this queue does not need to be
				//   saved/loaded at all. IP groups will be reloaded
				//   from group state files. But the original code did,
				//   and so will we until we can prove we do not need to.
				SaveGroupQueue (out_fp, &GLOBAL (ip_group_q));
		}
		SaveShipQueue (out_fp, &GLOBAL (built_ship_q), SHIP_Q_TAG);

		// Save the game event chunk
		SaveEvents (out_fp);

		// Save the encounter chunk (black globes in HS/QS)
		SaveEncounters (out_fp);

		// Save out the data that used to be in state files
		SaveStarInfo (out_fp);
		SaveGroups (out_fp);

		// Save out the Star Descriptor
		SaveStarDesc (&SD, out_fp);
		
		res_CloseResFile (out_fp);
		if (!io_ok)
		{
			DeleteResFile(saveDir, file);
			return FALSE;
		}
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}
