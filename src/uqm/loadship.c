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

#include "build.h"
#include "coderes.h"
#include "corecode.h"
#include "globdata.h"
#include "nameref.h"
#include "races.h"
#include "init.h"

static RESOURCE code_resources[] = {
		NULL_RESOURCE,
		ARILOU_CODE,
		CHMMR_CODE,
		HUMAN_CODE,
		ORZ_CODE,
		PKUNK_CODE,
		SHOFIXTI_CODE,
		SPATHI_CODE,
		SUPOX_CODE,
		THRADDASH_CODE,
		UTWIG_CODE,
		VUX_CODE,
		YEHAT_CODE,
		MELNORME_CODE,
		DRUUGE_CODE,
		ILWRATH_CODE,
		MYCON_CODE,
		SLYLANDRO_CODE,
		UMGAH_CODE,
		URQUAN_CODE,
		ZOQFOTPIK_CODE,
		SYREEN_CODE,
		KOHR_AH_CODE,
		ANDROSYNTH_CODE,
		CHENJESU_CODE,
		MMRNMHRM_CODE,
		SIS_CODE,
		SAMATRA_CODE,
		URQUAN_DRONE_CODE };

RACE_DESC *
load_ship (SPECIES_ID SpeciesID, BOOLEAN LoadBattleData)
{
	RACE_DESC *RDPtr = 0;
	void *CodeRef;
	
	if (SpeciesID >= NUM_SPECIES_ID)
		return NULL;

	CodeRef = CaptureCodeRes (LoadCodeRes (code_resources[SpeciesID]),
			&GlobData, (void **)(&RDPtr));
			
	if (!CodeRef)
		goto BadLoad;
	RDPtr->CodeRef = CodeRef;

	if (RDPtr->ship_info.icons_rsc != NULL_RESOURCE)
	{
		RDPtr->ship_info.icons = CaptureDrawable (LoadGraphic (
				RDPtr->ship_info.icons_rsc));
		if (!RDPtr->ship_info.icons)
		{
			/* goto BadLoad */
		}
	}
	
	if (RDPtr->ship_info.melee_icon_rsc != NULL_RESOURCE)
	{
		RDPtr->ship_info.melee_icon = CaptureDrawable (LoadGraphic (
				RDPtr->ship_info.melee_icon_rsc));
		if (!RDPtr->ship_info.melee_icon)
		{
			/* goto BadLoad */
		}
	}

	if (RDPtr->ship_info.race_strings_rsc != NULL_RESOURCE)
	{
		RDPtr->ship_info.race_strings =	CaptureStringTable (LoadStringTable (
				RDPtr->ship_info.race_strings_rsc));
		if (!RDPtr->ship_info.race_strings)
		{
			/* goto BadLoad */
		}
	}

	if (LoadBattleData)
	{
		DATA_STUFF *RawPtr = &RDPtr->ship_data;
		if (!load_animation (RawPtr->ship,
				RawPtr->ship_rsc[0],
				RawPtr->ship_rsc[1],
				RawPtr->ship_rsc[2]))
			goto BadLoad;

		if (RawPtr->weapon_rsc[0] != NULL_RESOURCE)
		{
			if (!load_animation (RawPtr->weapon,
					RawPtr->weapon_rsc[0],
					RawPtr->weapon_rsc[1],
					RawPtr->weapon_rsc[2]))
				goto BadLoad;
		}

		if (RawPtr->special_rsc[0] != NULL_RESOURCE)
		{
			if (!load_animation (RawPtr->special,
					RawPtr->special_rsc[0],
					RawPtr->special_rsc[1],
					RawPtr->special_rsc[2]))
				goto BadLoad;
		}

		if (RawPtr->captain_control.captain_rsc != NULL_RESOURCE)
		{
			RawPtr->captain_control.background = CaptureDrawable (LoadGraphic (
					RawPtr->captain_control.captain_rsc));
			if (!RawPtr->captain_control.background)
				goto BadLoad;
		}

		if (RawPtr->victory_ditty_rsc != NULL_RESOURCE)
		{
			RawPtr->victory_ditty =
					LoadMusic (RawPtr->victory_ditty_rsc);
			if (!RawPtr->victory_ditty)
				goto BadLoad;
		}

		if (RawPtr->ship_sounds_rsc != NULL_RESOURCE)
		{
			RawPtr->ship_sounds = CaptureSound (
					LoadSound (RawPtr->ship_sounds_rsc));
			if (!RawPtr->ship_sounds)
				goto BadLoad;
		}
	}

ExitFunc:
	return RDPtr;

	// TODO: We should really free the resources that did load here
BadLoad:
	if (CodeRef)
		DestroyCodeRes (ReleaseCodeRes (CodeRef));

	RDPtr = 0; /* failed */

	goto ExitFunc;
}

void
free_ship (RACE_DESC *raceDescPtr, BOOLEAN FreeIconData,
		BOOLEAN FreeBattleData)
{
	if (raceDescPtr->uninit_func != NULL)
		(*raceDescPtr->uninit_func) (raceDescPtr);

	if (FreeBattleData)
	{
		DATA_STUFF *shipData = &raceDescPtr->ship_data;

		free_image (shipData->special);
		free_image (shipData->weapon);
		free_image (shipData->ship);

		DestroyDrawable (
				ReleaseDrawable (shipData->captain_control.background));
		DestroyMusic (shipData->victory_ditty);
		DestroySound (ReleaseSound (shipData->ship_sounds));
	}

	if (FreeIconData)
	{
		SHIP_INFO *shipInfo = &raceDescPtr->ship_info;

		DestroyDrawable (ReleaseDrawable (shipInfo->melee_icon));
		DestroyDrawable (ReleaseDrawable (shipInfo->icons));
		DestroyStringTable (ReleaseStringTable (shipInfo->race_strings));
	}

	DestroyCodeRes (ReleaseCodeRes (raceDescPtr->CodeRef));
}
