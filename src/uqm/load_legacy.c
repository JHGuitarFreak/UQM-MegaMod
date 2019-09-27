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

#include "build.h"
#include "libs/declib.h"
#include "encount.h"
#include "gameev.h"
#include "starmap.h"
#include "libs/file.h"
#include "globdata.h"
#include "options.h"
#include "save.h"
#include "setup.h"
#include "state.h"
#include "grpinfo.h"

#include "libs/tasklib.h"
#include "libs/log.h"
#include "libs/misc.h"
#include "libs/scriptlib.h"

//#define DEBUG_LOAD


// This defines the order and the number of bits in which the game state
// properties are saved.
static const GameStateBitMap legacyGameStateBitMap[] = {
	{ "SHOFIXTI_VISITS", 3 },
	{ "SHOFIXTI_STACK1", 2 },
	{ "SHOFIXTI_STACK2", 3 },
	{ "SHOFIXTI_STACK3", 2 },
	{ "SHOFIXTI_KIA", 1 },
	{ "SHOFIXTI_BRO_KIA", 1 },
	{ "SHOFIXTI_RECRUITED", 1 },
	{ "SHOFIXTI_MAIDENS", 1 },
	{ "MAIDENS_ON_SHIP", 1 },
	{ "BATTLE_SEGUE", 1 },
	{ "PLANETARY_LANDING", 1 },
	{ "PLANETARY_CHANGE", 1 },
	{ "SPATHI_VISITS", 3 },
	{ "SPATHI_HOME_VISITS", 3 },
	{ "FOUND_PLUTO_SPATHI", 2 },
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
	{ "ARILOU_SPACE_SIDE", 2 },
	{ "ARILOU_SPACE_COUNTER", 4 },
	{ "LANDER_SHIELDS", 4 },
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
	{ "URQUAN_PROBE_GRPOFFS", 32 },
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
	{ "GLOBAL_FLAGS_AND_DATA", 8 },
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
	{ "UTWIG_HAVE_ULTRON", 1 },
	{ "BOMB_UNPROTECTED", 1 },
	{ "TAALO_UNPROTECTED", 1 },
	{ "TALKING_PET_VISITS", 3 },
	{ "TALKING_PET_HOME_VISITS", 3 },
	{ "UMGAH_ZOMBIE_BLOBBIES", 1 },
	{ "KNOW_UMGAH_ZOMBIES", 1 },
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
	{ "CHMMR_HOME_VISITS", 3 },
	{ "CHMMR_EMERGING", 1 },
	{ "CHMMR_UNLEASHED", 1 },
	{ "CHMMR_BOMB_STATE", 2 },
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
	{ "PKUNK_SHIP_DAY", 5 },
	{ "PKUNK_SHIP_YEAR", 5 },
	{ "PKUNK_MISSION", 3 },
	{ "SUPOX_VISITS", 3 },
	{ "SUPOX_HOME_VISITS", 3 },
	{ "THRADD_VISITS", 3 },
	{ "THRADD_HOME_VISITS", 3 },
	{ "HELIX_VISITS", 3 },
	{ "HELIX_UNPROTECTED", 1 },
	{ "THRADD_CULTURE", 2 },
	{ "THRADD_MISSION", 3 },
	{ "DRUUGE_VISITS", 3 },
	{ "DRUUGE_HOME_VISITS", 3 },
	{ "ROSY_SPHERE", 1 },
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
	{ "EGG_CASE1_ON_SHIP", 1 },
	{ "EGG_CASE2_ON_SHIP", 1 },
	{ "MYCON_SUN_VISITS", 3 },
	{ "ORZ_HOME_VISITS", 3 },
	{ "MELNORME_FUEL_PROCEDURE", 1 },
	{ "MELNORME_TECH_PROCEDURE", 1 },
	{ "MELNORME_INFO_PROCEDURE", 1 },
	{ "MELNORME_TECH_STACK", 4 },
	{ "MELNORME_EVENTS_INFO_STACK", 5 },
	{ "MELNORME_ALIEN_INFO_STACK", 5 },
	{ "MELNORME_HISTORY_INFO_STACK", 5 },
	{ "RAINBOW_WORLD0", 8 },
	{ "RAINBOW_WORLD1", 2 },
	{ "MELNORME_RAINBOW_COUNT", 4 },
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
	{ "COLONY_GRPOFFS", 32 },
	{ "THRADDASH_BODY_COUNT", 5 },
	{ "UTWIG_SUPOX_MISSION", 3 },
	{ "SPATHI_INFO", 3 },
	{ "ILWRATH_INFO", 2 },
	{ "ILWRATH_GODS_SPOKEN", 4 },
	{ "ILWRATH_WORSHIP", 2 },
	{ "ILWRATH_FIGHT_THRADDASH", 1 },
	{ "SAMATRA_GRPOFFS", 32 },
	{ "READY_TO_CONFUSE_URQUAN", 1 },
	{ "URQUAN_HYPNO_VISITS", 1 },
	{ "MENTIONED_PET_COMPULSION", 1 },
	{ "URQUAN_INFO", 2 },
	{ "KNOW_URQUAN_STORY", 2 },
	{ "MYCON_INFO", 4 },
	{ "MYCON_RAMBLE", 5 },
	{ "KNOW_ABOUT_SHATTERED", 2 },
	{ "MYCON_INSULTS", 3 },
	{ "MYCON_KNOW_AMBUSH", 1 },
	{ "SYREEN_INFO", 2 },
	{ "KNOW_SYREEN_WORLD_SHATTERED", 1 },
	{ "SYREEN_KNOW_ABOUT_MYCON", 1 },
	{ "TALKING_PET_INFO", 3 },
	{ "TALKING_PET_SUGGESTIONS", 3 },
	{ "LEARNED_TALKING_PET", 1 },
	{ "DNYARRI_LIED", 1 },
	{ "SHIP_TO_COMPEL", 1 },
	{ "ORZ_GENERAL_INFO", 2 },
	{ "ORZ_PERSONAL_INFO", 3 },
	{ "ORZ_ANDRO_STATE", 2 },
	{ "REFUSED_ORZ_ALLIANCE", 1 },
	{ "PKUNK_MANNER", 2 },
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
	{ NULL, 0 },
};

const GameStateBitMap legacyGameStateBitMapHD[] = {
	{ "SHOFIXTI_VISITS", 3 },
	{ "SHOFIXTI_STACK1", 2 },
	{ "SHOFIXTI_STACK2", 3 },
	{ "SHOFIXTI_STACK3", 2 },
	{ "SHOFIXTI_KIA", 1 },
	{ "SHOFIXTI_BRO_KIA", 1 },
	{ "SHOFIXTI_RECRUITED", 1 },
	{ "SHOFIXTI_MAIDENS", 1 },
	{ "MAIDENS_ON_SHIP", 1 },
	{ "BATTLE_SEGUE", 1 },
	{ "PLANETARY_LANDING", 1 },
	{ "PLANETARY_CHANGE", 1 },
	{ "SPATHI_VISITS", 3 },
	{ "SPATHI_HOME_VISITS", 3 },
	{ "FOUND_PLUTO_SPATHI", 2 },
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
	{ "ARILOU_SPACE_SIDE", 2 },
	{ "ARILOU_SPACE_COUNTER", 4 },
	{ "LANDER_SHIELDS", 4 },
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
	{ "URQUAN_PROBE_GRPOFFS", 32 },
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
	{ "GLOBAL_FLAGS_AND_DATA", 8 },
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
	{ "UTWIG_HAVE_ULTRON", 1 },
	{ "BOMB_UNPROTECTED", 1 },
	{ "TAALO_UNPROTECTED", 1 },
	{ "TALKING_PET_VISITS", 3 },
	{ "TALKING_PET_HOME_VISITS", 3 },
	{ "UMGAH_ZOMBIE_BLOBBIES", 1 },
	{ "KNOW_UMGAH_ZOMBIES", 1 },
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
	{ "CHMMR_HOME_VISITS", 3 },
	{ "CHMMR_EMERGING", 1 },
	{ "CHMMR_UNLEASHED", 1 },
	{ "CHMMR_BOMB_STATE", 2 },
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
	{ "PKUNK_SHIP_DAY", 5 },
	{ "PKUNK_SHIP_YEAR", 5 },
	{ "PKUNK_MISSION", 3 },
	{ "SUPOX_VISITS", 3 },
	{ "SUPOX_HOME_VISITS", 3 },
	{ "THRADD_VISITS", 3 },
	{ "THRADD_HOME_VISITS", 3 },
	{ "HELIX_VISITS", 3 },
	{ "HELIX_UNPROTECTED", 1 },
	{ "THRADD_CULTURE", 2 },
	{ "THRADD_MISSION", 3 },
	{ "DRUUGE_VISITS", 3 },
	{ "DRUUGE_HOME_VISITS", 3 },
	{ "ROSY_SPHERE", 1 },
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
	{ "EGG_CASE1_ON_SHIP", 1 },
	{ "EGG_CASE2_ON_SHIP", 1 },
	{ "MYCON_SUN_VISITS", 3 },
	{ "ORZ_HOME_VISITS", 3 },
	{ "MELNORME_FUEL_PROCEDURE", 1 },
	{ "MELNORME_TECH_PROCEDURE", 1 },
	{ "MELNORME_INFO_PROCEDURE", 1 },
	{ "MELNORME_TECH_STACK", 4 },
	{ "MELNORME_EVENTS_INFO_STACK", 5 },
	{ "MELNORME_ALIEN_INFO_STACK", 5 },
	{ "MELNORME_HISTORY_INFO_STACK", 5 },
	{ "RAINBOW_WORLD0", 8 },
	{ "RAINBOW_WORLD1", 2 },
	{ "MELNORME_RAINBOW_COUNT", 4 },
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
	{ "COLONY_GRPOFFS", 32 },
	{ "THRADDASH_BODY_COUNT", 5 },
	{ "UTWIG_SUPOX_MISSION", 3 },
	{ "SPATHI_INFO", 3 },
	{ "ILWRATH_INFO", 2 },
	{ "ILWRATH_GODS_SPOKEN", 4 },
	{ "ILWRATH_WORSHIP", 2 },
	{ "ILWRATH_FIGHT_THRADDASH", 1 },
	{ "SAMATRA_GRPOFFS", 32 },
	{ "READY_TO_CONFUSE_URQUAN", 1 },
	{ "URQUAN_HYPNO_VISITS", 1 },
	{ "MENTIONED_PET_COMPULSION", 1 },
	{ "URQUAN_INFO", 2 },
	{ "KNOW_URQUAN_STORY", 2 },
	{ "MYCON_INFO", 4 },
	{ "MYCON_RAMBLE", 5 },
	{ "KNOW_ABOUT_SHATTERED", 2 },
	{ "MYCON_INSULTS", 3 },
	{ "MYCON_KNOW_AMBUSH", 1 },
	{ "SYREEN_INFO", 2 },
	{ "KNOW_SYREEN_WORLD_SHATTERED", 1 },
	{ "SYREEN_KNOW_ABOUT_MYCON", 1 },
	{ "TALKING_PET_INFO", 3 },
	{ "TALKING_PET_SUGGESTIONS", 3 },
	{ "LEARNED_TALKING_PET", 1 },
	{ "DNYARRI_LIED", 1 },
	{ "SHIP_TO_COMPEL", 1 },
	{ "ORZ_GENERAL_INFO", 2 },
	{ "ORZ_PERSONAL_INFO", 3 },
	{ "ORZ_ANDRO_STATE", 2 },
	{ "REFUSED_ORZ_ALLIANCE", 1 },
	{ "PKUNK_MANNER", 2 },
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
static inline COUNT
cread_8 (DECODE_REF fh, BYTE *v)
{
	BYTE t;
	if (!v) /* read value ignored */
		v = &t;
	return cread (v, 1, 1, fh);
}

static inline COUNT
cread_16 (DECODE_REF fh, UWORD *v)
{
	UWORD t;
	if (!v) /* read value ignored */
		v = &t;
	return cread (v, 2, 1, fh);
}

static inline COUNT
cread_16s (DECODE_REF fh, SWORD *v)
{
	UWORD t;
	COUNT ret;
	// value was converted to unsigned when saved
	ret = cread_16 (fh, &t);
	// unsigned to signed conversion
	if (v)
		*v = t;
	return ret;
}

static inline COUNT
cread_32 (DECODE_REF fh, DWORD *v)
{
	DWORD t;
	if (!v) /* read value ignored */
		v = &t;
	return cread (v, 4, 1, fh);
}

static inline COUNT
cread_32s (DECODE_REF fh, SDWORD *v)
{
	DWORD t;
	COUNT ret;
	// value was converted to unsigned when saved
	ret = cread_32 (fh, &t);
	// unsigned to signed conversion
	if (v)
		*v = t;
	return ret;
}

static inline COUNT
cread_ptr (DECODE_REF fh)
{
	DWORD t;
	return cread_32 (fh, &t); /* ptrs are useless in saves */
}

static inline COUNT
cread_a8 (DECODE_REF fh, BYTE *ar, COUNT count)
{
	assert (ar != NULL);
	return cread (ar, 1, count, fh) == count;
}

static inline size_t
read_8 (void *fp, BYTE *v)
{
	BYTE t;
	if (!v) /* read value ignored */
		v = &t;
	return ReadResFile (v, 1, 1, fp);
}

static inline size_t
read_16 (void *fp, UWORD *v)
{
	UWORD t;
	if (!v) /* read value ignored */
		v = &t;
	return ReadResFile (v, 2, 1, fp);
}

static inline size_t
read_32 (void *fp, DWORD *v)
{
	DWORD t;
	if (!v) /* read value ignored */
		v = &t;
	return ReadResFile (v, 4, 1, fp);
}

static inline size_t
read_32s (void *fp, SDWORD *v)
{
	DWORD t;
	COUNT ret;
	// value was converted to unsigned when saved
	ret = read_32 (fp, &t);
	// unsigned to signed conversion
	if (v)
		*v = t;
	return ret;
}

static inline size_t
read_ptr (void *fp)
{
	DWORD t;
	return read_32 (fp, &t); /* ptrs are useless in saves */
}

static inline size_t
read_a8 (void *fp, BYTE *ar, COUNT count)
{
	assert (ar != NULL);
	return ReadResFile (ar, 1, count, fp) == count;
}

static inline size_t
read_str (void *fp, char *str, COUNT count)
{
	// no type conversion needed for strings
	return read_a8 (fp, (BYTE *)str, count);
}

static inline size_t
read_a16 (void *fp, UWORD *ar, COUNT count)
{
	assert (ar != NULL);

	for ( ; count > 0; --count, ++ar)
	{
		if (read_16 (fp, ar) != 1)
			return 0;
	}
	return 1;
}

static void
LoadEmptyQueue (DECODE_REF fh)
{
	COUNT num_links;

	cread_16 (fh, &num_links);
	if (num_links)
	{
		log_add (log_Error, "LoadEmptyQueue(): BUG: the queue is not empty!");
#ifdef DEBUG
		explode ();
#endif
	}
}

static void
LoadShipQueue (DECODE_REF fh, QUEUE *pQueue)
{
	COUNT num_links;

	cread_16 (fh, &num_links);

	while (num_links--)
	{
		HSHIPFRAG hStarShip;
		SHIP_FRAGMENT *FragPtr;
		COUNT Index;
		BYTE tmpb;

		cread_16 (fh, &Index);

		hStarShip = CloneShipFragment (Index, pQueue, 0);
		FragPtr = LockShipFrag (pQueue, hStarShip);

		// Read SHIP_FRAGMENT elements
		cread_16 (fh, NULL); /* unused: was which_side */
		cread_8  (fh, &FragPtr->captains_name_index);
		cread_8  (fh, NULL); /* padding */
		cread_16 (fh, NULL); /* unused: was ship_flags */
		cread_8  (fh, &FragPtr->race_id);
		cread_8  (fh, &FragPtr->index);
		// XXX: reading crew as BYTE to maintain savegame compatibility
		cread_8  (fh, &tmpb);
		FragPtr->crew_level = tmpb;
		cread_8  (fh, &tmpb);
		FragPtr->max_crew = tmpb;
		cread_8  (fh, &FragPtr->energy_level);
		cread_8  (fh, &FragPtr->max_energy);
		cread_16 (fh, NULL); /* unused; was loc.x */
		cread_16 (fh, NULL); /* unused; was loc.y */

		UnlockShipFrag (pQueue, hStarShip);
	}
}

static void
LoadRaceQueue (DECODE_REF fh, QUEUE *pQueue)
{
	COUNT num_links;

	cread_16 (fh, &num_links);

	while (num_links--)
	{
		HFLEETINFO hStarShip;
		FLEET_INFO *FleetPtr;
		COUNT Index;
		BYTE tmpb;

		cread_16 (fh, &Index);

		hStarShip = GetStarShipFromIndex (pQueue, Index);
		FleetPtr = LockFleetInfo (pQueue, hStarShip);

		// Read FLEET_INFO elements
		cread_16 (fh, &FleetPtr->allied_state);
		cread_8  (fh, &FleetPtr->days_left);
		cread_8  (fh, &FleetPtr->growth_fract);
		cread_8  (fh, &tmpb);
		FleetPtr->crew_level = tmpb;
		cread_8  (fh, &tmpb);
		FleetPtr->max_crew = tmpb;
		cread_8  (fh, &FleetPtr->growth);
		cread_8  (fh, &FleetPtr->max_energy);
		cread_16s(fh, &FleetPtr->loc.x);
		cread_16s(fh, &FleetPtr->loc.y);

		cread_16 (fh, &FleetPtr->actual_strength);
		cread_16 (fh, &FleetPtr->known_strength);
		cread_16s(fh, &FleetPtr->known_loc.x);
		cread_16s(fh, &FleetPtr->known_loc.y);
		cread_8  (fh, &FleetPtr->growth_err_term);
		cread_8  (fh, &FleetPtr->func_index);
		cread_16s(fh, &FleetPtr->dest_loc.x);
		cread_16s(fh, &FleetPtr->dest_loc.y);
		cread_16 (fh, NULL); /* alignment padding */

		UnlockFleetInfo (pQueue, hStarShip);
	}
}

static void
LoadGroupQueue (DECODE_REF fh, QUEUE *pQueue)
{
	COUNT num_links;

	cread_16 (fh, &num_links);

	while (num_links--)
	{
		HIPGROUP hGroup;
		IP_GROUP *GroupPtr;
		BYTE tmpb;

		cread_16 (fh, NULL); /* unused; was race_id */

		hGroup = BuildGroup (pQueue, 0);
		GroupPtr = LockIpGroup (pQueue, hGroup);

		cread_16 (fh, NULL); /* unused; was which_side */
		cread_8  (fh, NULL); /* unused; was captains_name_index */
		cread_8  (fh, NULL); /* padding; for savegame compat */
		cread_16 (fh, &GroupPtr->group_counter);
		cread_8  (fh, &GroupPtr->race_id);
		cread_8  (fh, &tmpb); /* was var2 */
		GroupPtr->sys_loc = LONIBBLE (tmpb);
		GroupPtr->task = HINIBBLE (tmpb);
		cread_8  (fh, &GroupPtr->in_system); /* was crew_level */
		cread_8  (fh, NULL); /* unused; was max_crew */
		cread_8  (fh, &tmpb); /* was energy_level */
		GroupPtr->dest_loc = LONIBBLE (tmpb);
		GroupPtr->orbit_pos = HINIBBLE (tmpb);
		cread_8  (fh, &GroupPtr->group_id); /* was max_energy */
		cread_16s(fh, &GroupPtr->loc.x);
		cread_16s(fh, &GroupPtr->loc.y);

		UnlockIpGroup (pQueue, hGroup);
	}
}

static void
LoadEncounter (ENCOUNTER *EncounterPtr, DECODE_REF fh)
{
	COUNT i;
	BYTE tmpb;

	cread_ptr (fh); /* useless ptr; HENCOUNTER pred */
	EncounterPtr->pred = 0;
	cread_ptr (fh); /* useless ptr; HENCOUNTER succ */
	EncounterPtr->succ = 0;
	cread_ptr (fh); /* useless ptr; HELEMENT hElement */
	EncounterPtr->hElement = 0;
	cread_16s (fh, &EncounterPtr->transition_state);
	cread_16s (fh, &EncounterPtr->origin.x);
	cread_16s (fh, &EncounterPtr->origin.y);
	cread_16  (fh, &EncounterPtr->radius);
	// former STAR_DESC fields
	cread_16s (fh, &EncounterPtr->loc_pt.x);
	cread_16s (fh, &EncounterPtr->loc_pt.y);
	cread_8   (fh, &EncounterPtr->race_id);
	cread_8   (fh, &tmpb);
	EncounterPtr->num_ships = tmpb & ENCOUNTER_SHIPS_MASK;
	EncounterPtr->flags = tmpb & ENCOUNTER_FLAGS_MASK;
	cread_16  (fh, NULL); /* alignment padding */

	// Load each entry in the BRIEF_SHIP_INFO array
	for (i = 0; i < MAX_HYPER_SHIPS; i++)
	{
		BRIEF_SHIP_INFO *ShipInfo = &EncounterPtr->ShipList[i];

		cread_16  (fh, NULL); /* useless; was SHIP_INFO.ship_flags */
		cread_8   (fh, &ShipInfo->race_id);
		cread_8   (fh, NULL); /* useless; was SHIP_INFO.var2 */
		// XXX: reading crew as BYTE to maintain savegame compatibility
		cread_8   (fh, &tmpb);
		ShipInfo->crew_level = tmpb;
		cread_8   (fh, &tmpb);
		ShipInfo->max_crew = tmpb;
		cread_8   (fh, NULL); /* useless; was SHIP_INFO.energy_level */
		cread_8   (fh, &ShipInfo->max_energy);
		cread_16  (fh, NULL); /* useless; was SHIP_INFO.loc.x */
		cread_16  (fh, NULL); /* useless; was SHIP_INFO.loc.y */
		cread_32  (fh, NULL); /* useless val; STRING race_strings */
		cread_ptr (fh); /* useless ptr; FRAME icons */
		cread_ptr (fh); /* useless ptr; FRAME melee_icon */
	}
	
	// Load the stuff after the BRIEF_SHIP_INFO array
	cread_32s (fh, &EncounterPtr->log_x);
	cread_32s (fh, &EncounterPtr->log_y);
	
	// JMS: Let's make savegames work even between different resolution modes.
	EncounterPtr->log_x <<= RESOLUTION_FACTOR;
	EncounterPtr->log_y <<= RESOLUTION_FACTOR;
}

static void
LoadEvent (EVENT *EventPtr, DECODE_REF fh)
{
	cread_ptr (fh); /* useless ptr; HEVENT pred */
	EventPtr->pred = 0;
	cread_ptr (fh); /* useless ptr; HEVENT succ */
	EventPtr->succ = 0;
	cread_8   (fh, &EventPtr->day_index);
	cread_8   (fh, &EventPtr->month_index);
	cread_16  (fh, &EventPtr->year_index);
	cread_8   (fh, &EventPtr->func_index);
	cread_8   (fh, NULL); /* padding */
	cread_16  (fh, NULL); /* padding */
}

static void
DummyLoadQueue (QUEUE *QueuePtr, DECODE_REF fh)
{
	/* QUEUE should never actually be loaded since it contains
	 * purely internal representation and the lists
	 * involved are actually loaded separately */
	(void)QueuePtr; /* silence compiler */

	/* QUEUE format with QUEUE_TABLE defined -- UQM default */
	cread_ptr (fh); /* HLINK head */
	cread_ptr (fh); /* HLINK tail */
	cread_ptr (fh); /* BYTE* pq_tab */
	cread_ptr (fh); /* HLINK free_list */
	cread_16  (fh, NULL); /* MEM_HANDLE hq_tab */
	cread_16  (fh, NULL); /* COUNT object_size */
	cread_8   (fh, NULL); /* BYTE num_objects */
	
	cread_8   (fh, NULL); /* padding */
	cread_16  (fh, NULL); /* padding */
}

static void
LoadClockState (CLOCK_STATE *ClockPtr, DECODE_REF fh)
{
	cread_8   (fh, &ClockPtr->day_index);
	cread_8   (fh, &ClockPtr->month_index);
	cread_16  (fh, &ClockPtr->year_index);
	cread_16s (fh, &ClockPtr->tick_count);
	cread_16s (fh, &ClockPtr->day_in_ticks);
	cread_ptr (fh); /* not loading ptr; Semaphore clock_sem */
	cread_ptr (fh); /* not loading ptr; Task clock_task */
	cread_32  (fh, NULL); /* not loading; DWORD TimeCounter */

	DummyLoadQueue (&ClockPtr->event_q, fh);
}

static BOOLEAN
LoadGameState (GAME_STATE *GSPtr, DECODE_REF fh, BOOLEAN vanilla)
{
	BYTE dummy8;
	BYTE res_scale; // JMS

	cread_8   (fh, &dummy8); /* obsolete */
	cread_8   (fh, &GSPtr->glob_flags);
	cread_8   (fh, &GSPtr->CrewCost);
	cread_8   (fh, &GSPtr->FuelCost);
	
	// JMS: Now that we have read the fuelcost, we can compare it
	// to the correct value. Fuel cost is always FUEL_COST_RU, and if
	// the savefile tells otherwise, we have read it with the wrong method
	// (The savegame is from vanilla UQM and we've been reading it as if it
	// were UQM-HD save.)
	//
	// At this point we must then cease reading the savefile, close it
	// and re-open it again, this time using the vanilla-reading method.
	if (GSPtr->FuelCost != FUEL_COST_RU)
		return FALSE;
	
	cread_a8  (fh, GSPtr->ModuleCost, NUM_MODULES);
	cread_a8  (fh, GSPtr->ElementWorth, NUM_ELEMENT_CATEGORIES);
	cread_ptr (fh); /* not loading ptr; PRIMITIVE *DisplayArray */
	cread_16  (fh, &GSPtr->CurrentActivity);
	
	// JMS
	if (LOBYTE (GSPtr->CurrentActivity) != IN_INTERPLANETARY)
		res_scale = RESOLUTION_FACTOR;
	else
		res_scale = 0;
	
	cread_16  (fh, NULL); /* CLOCK_STATE alignment padding */
	LoadClockState (&GSPtr->GameClock, fh);

	cread_16s (fh, &GSPtr->autopilot.x);
	cread_16s (fh, &GSPtr->autopilot.y);
	cread_16s (fh, &GSPtr->ip_location.x);
	cread_16s (fh, &GSPtr->ip_location.y);
	/* STAMP ShipStamp */
	cread_16s (fh, &GSPtr->ShipStamp.origin.x);
	cread_16s (fh, &GSPtr->ShipStamp.origin.y);
	cread_16  (fh, &GSPtr->ShipFacing);
	cread_8   (fh, &GSPtr->ip_planet);
	cread_8   (fh, &GSPtr->in_orbit);
	
	GSPtr->ShipStamp.origin.x <<= RESOLUTION_FACTOR; // JMS: Let's make savegames work even between different resolution modes.
	GSPtr->ShipStamp.origin.y <<= RESOLUTION_FACTOR; // JMS: Let's make savegames work even between different resolution modes.

	/* VELOCITY_DESC velocity */
	cread_16  (fh, &GSPtr->velocity.TravelAngle);
	cread_16s (fh, &GSPtr->velocity.vector.width);
	cread_16s (fh, &GSPtr->velocity.vector.height);
	cread_16s (fh, &GSPtr->velocity.fract.width);
	cread_16s (fh, &GSPtr->velocity.fract.height);
	cread_16s (fh, &GSPtr->velocity.error.width);
	cread_16s (fh, &GSPtr->velocity.error.height);
	cread_16s (fh, &GSPtr->velocity.incr.width);
	cread_16s (fh, &GSPtr->velocity.incr.height);
	cread_16  (fh, NULL); /* VELOCITY_DESC padding */
	
	GSPtr->velocity.vector.width  <<= res_scale; // JMS: Let's make savegames work even between different resolution modes.
	GSPtr->velocity.vector.height <<= res_scale; // JMS: Let's make savegames work even between different resolution modes.
	GSPtr->velocity.fract.width	  <<= res_scale; // JMS: Let's make savegames work even between different resolution modes.
	GSPtr->velocity.fract.height  <<= res_scale; // JMS: Let's make savegames work even between different resolution modes.
	GSPtr->velocity.error.width	  <<= res_scale; // JMS: Let's make savegames work even between different resolution modes.
	GSPtr->velocity.error.height  <<= res_scale; // JMS: Let's make savegames work even between different resolution modes.
	GSPtr->velocity.incr.width	  <<= res_scale; // JMS: Let's make savegames work even between different resolution modes.
	GSPtr->velocity.incr.height	  <<= res_scale; // JMS: Let's make savegames work even between different resolution modes.

	cread_32  (fh, &GSPtr->BattleGroupRef);
	
	DummyLoadQueue (&GSPtr->avail_race_q, fh);
	DummyLoadQueue (&GSPtr->npc_built_ship_q, fh);
	// Not loading ip_group_q, was not there originally
	DummyLoadQueue (&GSPtr->encounter_q, fh);
	DummyLoadQueue (&GSPtr->built_ship_q, fh);

	// JMS: Let's not read the 'autopilot ok' and QS portal
	// coord bits for vanilla UQM saves.
	{ 
		size_t numBytes = ((NUM_GAME_STATE_BITS + 7) >> 3); 
		BYTE *buf; 
		
		numBytes = (vanilla ? numBytes - 2 : numBytes);

		buf = HMalloc (numBytes); 
		if (buf != NULL) 
		{ 
			cread_a8  (fh, buf, numBytes); 
			deserialiseGameState ((vanilla ? legacyGameStateBitMap : legacyGameStateBitMapHD), buf, numBytes);
			HFree(buf); 
		} 
	} 

	cread_8  (fh, NULL); /* GAME_STATE alignment padding */
	
	return TRUE;
}

static BOOLEAN
LoadSisState (SIS_STATE *SSPtr, void *fp)
{
	if (
			read_32s (fp, &SSPtr->log_x) != 1 ||
			read_32s (fp, &SSPtr->log_y) != 1 ||
			read_32  (fp, &SSPtr->ResUnits) != 1 ||
			read_32  (fp, &SSPtr->FuelOnBoard) != 1 ||
			read_16  (fp, &SSPtr->CrewEnlisted) != 1 ||
			read_16  (fp, &SSPtr->TotalElementMass) != 1 ||
			read_16  (fp, &SSPtr->TotalBioMass) != 1 ||
			read_a8  (fp, SSPtr->ModuleSlots, NUM_MODULE_SLOTS) != 1 ||
			read_a8  (fp, SSPtr->DriveSlots, NUM_DRIVE_SLOTS) != 1 ||
			read_a8  (fp, SSPtr->JetSlots, NUM_JET_SLOTS) != 1 ||
			read_8   (fp, &SSPtr->NumLanders) != 1 ||
			read_a16 (fp, SSPtr->ElementAmounts, NUM_ELEMENT_CATEGORIES) != 1 ||

			read_str (fp, SSPtr->ShipName, SIS_NAME_SIZE) != 1 ||
			read_str (fp, SSPtr->CommanderName, SIS_NAME_SIZE) != 1 ||
			read_str (fp, SSPtr->PlanetName, SIS_NAME_SIZE) != 1 ||

			read_16  (fp, NULL) != 1 /* padding */
		)
		return FALSE;
	else
	{
		// JMS: Let's make savegames work even between different resolution modes.
		SSPtr->log_x <<= RESOLUTION_FACTOR;
		SSPtr->log_y <<= RESOLUTION_FACTOR;
		return TRUE;
	}
}

static BOOLEAN
LoadSummary (SUMMARY_DESC *SummPtr, void *fp, BOOLEAN try_vanilla)
{
	// JMS: New variables required for compatibility between
	// old, unnamed saves and the new, named ones.
	SDWORD  temp_log_x = 0;
	SDWORD  temp_log_y = 0;
	DWORD   temp_ru    = 0;
	DWORD   temp_fuel  = 0;
	BOOLEAN no_savename = FALSE;

	// First we check if there is a savegamename identifier.
	// The identifier tells us whether the name exists at all.
	read_str (fp, SummPtr->SaveNameChecker, SAVE_CHECKER_SIZE);
		
	// If the name doesn't exist (because this most probably
	// is a savegame from an older version), we have to rewind the
	// savefile to be able to read the saved variables into their
	// correct places.
	if (strncmp(SummPtr->SaveNameChecker, LEGACY_SAVE_NAME_CHECKER, SAVE_CHECKER_SIZE))
	{
		COUNT i;
			
		// Apparently the bytes read to SummPtr->SaveNameChecker with
		// read_str are destroyed from fp, so we must copy these bytes
		// to temp variables at this point to preserve them.
		no_savename = TRUE;
		memcpy(&temp_log_x, SummPtr->SaveNameChecker, sizeof(SDWORD));
		memcpy(&temp_log_y, &(SummPtr->SaveNameChecker[sizeof(SDWORD)]), sizeof(SDWORD));
		memcpy(&temp_ru, &(SummPtr->SaveNameChecker[2 * sizeof(SDWORD)]), sizeof(DWORD));
		memcpy(&temp_fuel, &(SummPtr->SaveNameChecker[2 * sizeof(SDWORD)+ sizeof(DWORD)]), sizeof(DWORD));
			
		// Rewind the position in savefile.
		for (i = 0; i < SAVE_CHECKER_SIZE; i++)
			uio_backtrack (1, (uio_Stream *) fp);
			
		// Zero the bogus savenamechecker.
		for (i = 0; i < SAVE_CHECKER_SIZE; i++)
			SummPtr->SaveNameChecker[i] = 0;
			
		// Make sure the save's name is empty.
		for (i = 0; i < LEGACY_SAVE_NAME_SIZE; i++)
			SummPtr->LegacySaveName[i] = 0;
		}
	else
	{
		// If the name identifier exists, let's also read
		// the savegame's actual name, which is situated right
		// after the identifier.
		read_str (fp, SummPtr->LegacySaveName, LEGACY_SAVE_NAME_SIZE);
	}
		
	//log_add (log_Debug, "fp: %d Check:%s Name:%s", fp, SummPtr->SaveNameChecker, SummPtr->SaveName);
	
	if (!LoadSisState (&SummPtr->SS, fp))
		return FALSE;

	// Sanitize seed, difficulty, extended, and nomad variables
	SummPtr->SS.Seed = SummPtr->SS.Difficulty = 0;
	SummPtr->SS.Extended = SummPtr->SS.Nomad = 0;
			
	// JMS: Now we'll put those temp variables into action.
	if (no_savename)
	{
		// Removed the complicated maths in favor of utilizing the Vanilla LOG_X code
		SummPtr->SS.log_x = UNIVERSE_TO_LOGX(oldLogxToUniverse(temp_log_x));
		SummPtr->SS.log_y = UNIVERSE_TO_LOGY(oldLogyToUniverse(temp_log_y));
		SummPtr->SS.ResUnits = temp_ru;
		SummPtr->SS.FuelOnBoard = temp_fuel;
	}
	
	if (
			read_8  (fp, &SummPtr->Activity) != 1 ||
			read_8  (fp, &SummPtr->Flags) != 1 ||
			read_8  (fp, &SummPtr->day_index) != 1 ||
			read_8  (fp, &SummPtr->month_index) != 1 ||
			read_16 (fp, &SummPtr->year_index) != 1 ||
			read_8  (fp, &SummPtr->MCreditLo) != 1 ||
			read_8  (fp, &SummPtr->MCreditHi) != 1 ||
			read_8  (fp, &SummPtr->NumShips) != 1 ||
			read_8  (fp, &SummPtr->NumDevices) != 1 ||
			read_a8 (fp, SummPtr->ShipList, MAX_BUILT_SHIPS) != 1 ||
			read_a8 (fp, SummPtr->DeviceList, MAX_EXCLUSIVE_DEVICES) != 1 ||
			read_8  (fp, &SummPtr->res_factor) != 1 || // JMS: This'll help making saves between different resolutions compatible.
		
			read_8  (fp, NULL) != 1 /* padding */
		)
		return FALSE;
	else
	{
		// JMS: UQM-HD saves have an extra piece of padding to compensate for the
		// added res_factor in SummPtr.
		if (!try_vanilla)
			read_8 (fp, NULL); /* padding */
	
		return TRUE;
	}
}

static void
LoadStarDesc (STAR_DESC *SDPtr, DECODE_REF fh)
{
	cread_16s(fh, &SDPtr->star_pt.x);
	cread_16s(fh, &SDPtr->star_pt.y);
	cread_8  (fh, &SDPtr->Type);
	cread_8  (fh, &SDPtr->Index);
	cread_8  (fh, &SDPtr->Prefix);
	cread_8  (fh, &SDPtr->Postfix);
}

BOOLEAN
LoadLegacyGame (COUNT which_game, SUMMARY_DESC *SummPtr, BOOLEAN try_vanilla)
{
	uio_Stream *in_fp;
	char file[PATH_MAX];
	char buf[256];
	SUMMARY_DESC loc_sd;
	GAME_STATE_FILE *fp;
	DECODE_REF fh;
	COUNT num_links;
	STAR_DESC SD;
	ACTIVITY Activity;

	sprintf (file, "starcon2.%02u", which_game);
	in_fp = res_OpenResFile (saveDir, file, "rb");
	if (!in_fp)
		return FALSE;

	loc_sd.SaveName[0] = '\0';
	if (!LoadSummary (&loc_sd, in_fp, try_vanilla))
	{
		log_add (log_Error, "Warning: Savegame is corrupt");
		res_CloseResFile (in_fp);
		return FALSE;
	}

	if (!SummPtr)
	{
		SummPtr = &loc_sd;
	}
	else
	{	// only need summary for displaying to user
		memcpy (SummPtr, &loc_sd, sizeof (*SummPtr));
		res_CloseResFile (in_fp);
		return TRUE;
	}

	// Crude check for big-endian/little-endian incompatibilities.
	// year_index is suitable as it's a multi-byte value within
	// a specific recognisable range.
	if (SummPtr->year_index < START_YEAR ||
			SummPtr->year_index >= START_YEAR +
			YEARS_TO_KOHRAH_VICTORY + 1 /* Utwig intervention */ +
			1 /* time to destroy all races, plenty */ +
			25 /* for cheaters */)
	{
		log_add (log_Error, "Warning: Savegame corrupt or from "
				"an incompatible platform.");
		res_CloseResFile (in_fp);
		return FALSE;
	}

	GlobData.SIS_state = SummPtr->SS;

	if ((fh = copen (in_fp, FILE_STREAM, STREAM_READ)) == 0)
	{
		res_CloseResFile (in_fp);
		return FALSE;
	}

	ReinitQueue (&GLOBAL (GameClock.event_q));
	ReinitQueue (&GLOBAL (encounter_q));
	ReinitQueue (&GLOBAL (ip_group_q));
	ReinitQueue (&GLOBAL (npc_built_ship_q));
	ReinitQueue (&GLOBAL (built_ship_q));

	uninitEventSystem ();
	luaUqm_uninitState();
	luaUqm_initState();
	initEventSystem ();

	Activity = GLOBAL (CurrentActivity);
	
	// JMS: We can decide whether the current savefile is vanilla UQM or UQM-HD
	// only at this point, when reading the game states. If this turns out to be a 
	// vanilla UQM save, we must close the file and re-open it for reading
	// with the vanilla method.
	if (!(LoadGameState (&GlobData.Game_state, fh, try_vanilla)))
	{
		res_CloseResFile (in_fp);
		
		if (!try_vanilla)
		{			
			LoadLegacyGame (which_game, NULL, TRUE);
			return TRUE;
		}
		else
			return FALSE;
	}
	
	NextActivity = GLOBAL (CurrentActivity);
	GLOBAL (CurrentActivity) = Activity;

	LoadRaceQueue (fh, &GLOBAL (avail_race_q));
	// START_INTERPLANETARY is only set when saving from Homeworld
	//   encounter screen. When the game is loaded, the
	//   GenerateOrbitalFunction for the current star system will
	//   create the encounter anew and populate the npc queue.
	if (!(NextActivity & START_INTERPLANETARY))
	{
		if (NextActivity & START_ENCOUNTER)
			LoadShipQueue (fh, &GLOBAL (npc_built_ship_q));
		else if (LOBYTE (NextActivity) == IN_INTERPLANETARY)
			// XXX: Technically, this queue does not need to be
			//   saved/loaded at all. IP groups will be reloaded
			//   from group state files. But the original code did,
			//   and so will we until we can prove we do not need to.
			LoadGroupQueue (fh, &GLOBAL (ip_group_q));
		else
			// XXX: The empty queue read is only needed to maintain
			//   the savegame compatibility
			LoadEmptyQueue (fh);
	}
	LoadShipQueue (fh, &GLOBAL (built_ship_q));

	// Load the game events (compressed)
	cread_16 (fh, &num_links);
	{
#ifdef DEBUG_LOAD
		log_add (log_Debug, "EVENTS:");
#endif /* DEBUG_LOAD */
		while (num_links--)
		{
			HEVENT hEvent;
			EVENT *EventPtr;

			hEvent = AllocEvent ();
			LockEvent (hEvent, &EventPtr);

			LoadEvent (EventPtr, fh);

#ifdef DEBUG_LOAD
		log_add (log_Debug, "\t%u/%u/%u -- %u",
				EventPtr->month_index,
				EventPtr->day_index,
				EventPtr->year_index,
				EventPtr->func_index);
#endif /* DEBUG_LOAD */
			UnlockEvent (hEvent);
			PutEvent (hEvent);
		}
	}

	// Load the encounters (black globes in HS/QS (compressed))
	cread_16 (fh, &num_links);
	{
		while (num_links--)
		{
			HENCOUNTER hEncounter;
			ENCOUNTER *EncounterPtr;

			hEncounter = AllocEncounter ();
			LockEncounter (hEncounter, &EncounterPtr);

			LoadEncounter (EncounterPtr, fh);

			UnlockEncounter (hEncounter);
			PutEncounter (hEncounter);
		}
	}

	// Copy the star info file from the compressed stream
	fp = OpenStateFile (STARINFO_FILE, "wb");
	if (fp)
	{
		DWORD flen;

		cread_32 (fh, &flen);
		while (flen)
		{
			COUNT num_bytes;

			num_bytes = flen >= sizeof (buf) ? sizeof (buf) : (COUNT)flen;
			cread (buf, num_bytes, 1, fh);
			WriteStateFile (buf, num_bytes, 1, fp);

			flen -= num_bytes;
		}
		CloseStateFile (fp);
	}

	// Copy the defined groupinfo file from the compressed stream
	fp = OpenStateFile (DEFGRPINFO_FILE, "wb");
	if (fp)
	{
		DWORD flen;

		cread_32 (fh, &flen);
		while (flen)
		{
			COUNT num_bytes;

			num_bytes = flen >= sizeof (buf) ? sizeof (buf) : (COUNT)flen;
			cread (buf, num_bytes, 1, fh);
			WriteStateFile (buf, num_bytes, 1, fp);

			flen -= num_bytes;
		}
		CloseStateFile (fp);
	}

	// Copy the random groupinfo file from the compressed stream
	fp = OpenStateFile (RANDGRPINFO_FILE, "wb");
	if (fp)
	{
		DWORD flen;

		cread_32 (fh, &flen);
		while (flen)
		{
			COUNT num_bytes;

			num_bytes = flen >= sizeof (buf) ? sizeof (buf) : (COUNT)flen;
			cread (buf, num_bytes, 1, fh);
			WriteStateFile (buf, num_bytes, 1, fp);

			flen -= num_bytes;
		}
		CloseStateFile (fp);
	}

	LoadStarDesc (&SD, fh);
	loadGameCheats();
	cclose (fh);
	res_CloseResFile (in_fp);

	EncounterGroup = 0;
	EncounterRace = -1;

	ReinitQueue (&race_q[0]);
	ReinitQueue (&race_q[1]);
	CurStarDescPtr = FindStar (NULL, &SD.star_pt, 0, 0);
	if (!(NextActivity & START_ENCOUNTER)
			&& LOBYTE (NextActivity) == IN_INTERPLANETARY)
		NextActivity |= START_INTERPLANETARY;

	return TRUE;
}