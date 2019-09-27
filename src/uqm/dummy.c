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

/*
 * This file seems to be a collection of functions that don't do
 * much.
 */

#include "dummy.h"

#include "coderes.h"
#include "globdata.h"
#include "races.h"

#include "libs/compiler.h"
#include "libs/log.h"
#include "libs/memlib.h"

#include <stdlib.h>
#include <ctype.h>


typedef struct
{
	RACE_DESC data _ALIGNED_ANY;
} CODERES_STRUCT;

typedef enum
{
	ANDROSYN_CODE_RES,
	ARILOU_CODE_RES,
	BLACKURQ_CODE_RES,
	CHENJESU_CODE_RES,
	CHMMR_CODE_RES,
	DRUUGE_CODE_RES,
	HUMAN_CODE_RES,
	ILWRATH_CODE_RES,
	MELNORME_CODE_RES,
	MMRNMHRM_CODE_RES,
	MYCON_CODE_RES,
	ORZ_CODE_RES,
	PKUNK_CODE_RES,
	SHOFIXTI_CODE_RES,
	SLYLANDR_CODE_RES,
	SPATHI_CODE_RES,
	SUPOX_CODE_RES,
	SYREEN_CODE_RES,
	THRADD_CODE_RES,
	UMGAH_CODE_RES,
	URQUAN_CODE_RES,
	UTWIG_CODE_RES,
	VUX_CODE_RES,
	YEHAT_CODE_RES,
	ZOQFOT_CODE_RES,

	SAMATRA_CODE_RES,
	SIS_CODE_RES,
	PROBE_CODE_RES
} ShipCodeRes;

typedef RACE_DESC *(*RaceDescInitFunc)(void);

static RaceDescInitFunc
CodeResToInitFunc(ShipCodeRes res)
{
	switch (res)
	{
		case ANDROSYN_CODE_RES: return &init_androsynth;
		case ARILOU_CODE_RES: return &init_arilou;
		case BLACKURQ_CODE_RES: return &init_black_urquan;
		case CHENJESU_CODE_RES: return &init_chenjesu;
		case CHMMR_CODE_RES: return &init_chmmr;
		case DRUUGE_CODE_RES: return &init_druuge;
		case HUMAN_CODE_RES: return &init_human;
		case ILWRATH_CODE_RES: return &init_ilwrath;
		case MELNORME_CODE_RES: return &init_melnorme;
		case MMRNMHRM_CODE_RES: return &init_mmrnmhrm;
		case MYCON_CODE_RES: return &init_mycon;
		case ORZ_CODE_RES: return &init_orz;
		case PKUNK_CODE_RES: return &init_pkunk;
		case SHOFIXTI_CODE_RES: return &init_shofixti;
		case SLYLANDR_CODE_RES: return &init_slylandro;
		case SPATHI_CODE_RES: return &init_spathi;
		case SUPOX_CODE_RES: return &init_supox;
		case SYREEN_CODE_RES: return &init_syreen;
		case THRADD_CODE_RES: return &init_thraddash;
		case UMGAH_CODE_RES: return &init_umgah;
		case URQUAN_CODE_RES: return &init_urquan;
		case UTWIG_CODE_RES: return &init_utwig;
		case VUX_CODE_RES: return &init_vux;
		case YEHAT_CODE_RES: return &init_yehat;
		case ZOQFOT_CODE_RES: return &init_zoqfotpik;
		case SAMATRA_CODE_RES: return &init_samatra;
		case SIS_CODE_RES: return &init_sis;
		case PROBE_CODE_RES: return &init_probe;
		default:
		{
			log_add (log_Warning, "Unknown SHIP identifier '%d'", res);
			return NULL;
		}
	}
}

static void
GetCodeResData (const char *ship_id, RESOURCE_DATA *resdata)
{
	BYTE which_res;
	void *hData;

	which_res = atoi (ship_id);
	hData = HMalloc (sizeof (CODERES_STRUCT));
	if (hData)
	{
		RaceDescInitFunc initFunc = CodeResToInitFunc (which_res);
		RACE_DESC *RDPtr = (initFunc == NULL) ? NULL : (*initFunc)();
		if (RDPtr == 0)
		{
			HFree (hData);
			hData = 0;
		}
		else
		{
			CODERES_STRUCT *cs;

			cs = (CODERES_STRUCT *) hData;
			cs->data = *RDPtr;  // Structure assignment.
		}
	}
	resdata->ptr = hData;
}

static BOOLEAN
_ReleaseCodeResData (void *data)
{
	HFree (data);
	return TRUE;
}

BOOLEAN
InstallCodeResType ()
{
	return (InstallResTypeVectors ("SHIP",
			GetCodeResData, _ReleaseCodeResData, NULL));
}


void *
LoadCodeResInstance (RESOURCE res)
{
	void *hData;

	hData = res_GetResource (res);
	if (hData)
		res_DetachResource (res);

	return hData;
}


BOOLEAN
DestroyCodeRes (void *hCode)
{
	HFree (hCode);
	return TRUE;
}


void*
CaptureCodeRes (void *hCode, void *pData, void **ppLocData)
{
	CODERES_STRUCT *cs;

	if (hCode == NULL)
	{
		log_add (log_Fatal, "dummy.c::CaptureCodeRes() hCode==NULL! FATAL!");
		return(NULL);
	}

	cs = (CODERES_STRUCT *) hCode;
	*ppLocData = &cs->data;

	(void) pData;  /* Satisfying compiler (unused parameter) */
	return cs;
}


void *
ReleaseCodeRes (void *CodeRef)
{
	return CodeRef;
}

