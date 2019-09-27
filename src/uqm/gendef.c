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

#include "gendef.h"
#include "planets/generate.h"


extern GenerateFunctions generateDefaultFunctions;

extern GenerateFunctions generateAndrosynthFunctions;
extern GenerateFunctions generateBurvixeseFunctions;
extern GenerateFunctions generateChmmrFunctions;
extern GenerateFunctions generateColonyFunctions;
extern GenerateFunctions generateDruugeFunctions;
extern GenerateFunctions generateIlwrathFunctions;
extern GenerateFunctions generateMelnormeFunctions;
extern GenerateFunctions generateMyconFunctions;
extern GenerateFunctions generateOrzFunctions;
extern GenerateFunctions generatePkunkFunctions;
extern GenerateFunctions generateRainbowWorldFunctions;
extern GenerateFunctions generateSaMatraFunctions;
extern GenerateFunctions generateShofixtiFunctions;
extern GenerateFunctions generateSlylandroFunctions;
extern GenerateFunctions generateSolFunctions;
extern GenerateFunctions generateSpathiFunctions;
extern GenerateFunctions generateSupoxFunctions;
extern GenerateFunctions generateSyreenFunctions;
extern GenerateFunctions generateTalkingPetFunctions;
extern GenerateFunctions generateThraddashFunctions;
extern GenerateFunctions generateTrapFunctions;
extern GenerateFunctions generateUtwigFunctions;
extern GenerateFunctions generateVaultFunctions;
extern GenerateFunctions generateVuxFunctions;
extern GenerateFunctions generateWreckFunctions;
extern GenerateFunctions generateYehatFunctions;
extern GenerateFunctions generateZoqFotPikFunctions;
extern GenerateFunctions generateZoqFotPikScoutFunctions;


const GenerateFunctions *
getGenerateFunctions (BYTE Index)
{
	switch (Index)
	{
		case SOL_DEFINED:
			return &generateSolFunctions;
		case SHOFIXTI_DEFINED:
			return &generateShofixtiFunctions;
		case START_COLONY_DEFINED:
			return &generateColonyFunctions;
		case ALGOLITES_DEFINED:
		case SPATHI_MONUMENT_DEFINED:
		case SPATHI_DEFINED:
			return &generateSpathiFunctions;
		case MELNORME0_DEFINED:
		case MELNORME1_DEFINED:
		case MELNORME2_DEFINED:
		case MELNORME3_DEFINED:
		case MELNORME4_DEFINED:
		case MELNORME5_DEFINED:
		case MELNORME6_DEFINED:
		case MELNORME7_DEFINED:
		case MELNORME8_DEFINED:
			return &generateMelnormeFunctions;
		case TALKING_PET_DEFINED:
			return &generateTalkingPetFunctions;
		case MOTHER_ARK_DEFINED:
		case CHMMR_DEFINED:
			return &generateChmmrFunctions;
		case SYREEN_DEFINED:
			return &generateSyreenFunctions;
		case MYCON_TRAP_DEFINED:
			return &generateTrapFunctions;
		case BURVIXESE_DEFINED:
			return &generateBurvixeseFunctions;
		case SLYLANDRO_DEFINED:
			return &generateSlylandroFunctions;
		case DRUUGE_DEFINED:
			return &generateDruugeFunctions;
		case BOMB_DEFINED:
		case UTWIG_DEFINED:
			return &generateUtwigFunctions;
		case AQUA_HELIX_DEFINED:
		case THRADD_DEFINED:
			return &generateThraddashFunctions;
		case SUN_DEVICE_DEFINED:
		case MYCON_DEFINED:
		case EGG_CASE0_DEFINED:
		case EGG_CASE1_DEFINED:
		case EGG_CASE2_DEFINED:
			return &generateMyconFunctions;
		case ANDROSYNTH_DEFINED:
			return &generateAndrosynthFunctions;
		case TAALO_PROTECTOR_DEFINED:
		case ORZ_DEFINED:
			return &generateOrzFunctions;
		case SHIP_VAULT_DEFINED:
			return &generateVaultFunctions;
		case URQUAN_WRECK_DEFINED:
			return &generateWreckFunctions;
		case MAIDENS_DEFINED:
		case VUX_BEAST_DEFINED:
		case VUX_DEFINED:
			return &generateVuxFunctions;
		case URQUAN_DEFINED:
		case KOHRAH_DEFINED:
		case DESTROYED_STARBASE_DEFINED:
		case SAMATRA_DEFINED:
			return &generateSaMatraFunctions; 
		case ZOQ_COLONY_DEFINED:
		case ZOQFOT_DEFINED:
			return &generateZoqFotPikFunctions;
		case ZOQ_SCOUT_DEFINED:
			return &generateZoqFotPikScoutFunctions;
		case YEHAT_DEFINED:
			return &generateYehatFunctions;
		case PKUNK_DEFINED:
			return &generatePkunkFunctions;
		case SUPOX_DEFINED:
			return &generateSupoxFunctions;
		case RAINBOW_DEFINED:
			return &generateRainbowWorldFunctions;
		case ILWRATH_DEFINED:
			return &generateIlwrathFunctions;
		default:
			return &generateDefaultFunctions;
	}
}

