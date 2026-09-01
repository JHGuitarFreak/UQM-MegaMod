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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

 /*
  * This file contains definitions relating to using strings specific to
  * the game. libs/strlib.h is for the string library.
  */

#ifndef UQM_IMGUI_STRINGS_H_
#define UQM_IMGUI_STRINGS_H_

#include "libs/strlib.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern STRING ImGuiStrings;

#define NAV_TAB_STR_COUNT   8
#define TIP_WARN_STR_COUNT 16

// General
#define GEN_SETT_STR_COUNT 11
#define GEN_GFX_STR_COUNT  14
#define GEN_ENG_STR_COUNT  36
#define GEN_AUD_STR_COUNT  18
#define GEN_CON_STR_COUNT  31
#define GEN_ADV_STR_COUNT  14

// Enhancements
#define ENH_QOL_STR_COUNT  10
#define ENH_VIS_STR_COUNT  21
#define ENH_CHT_STR_COUNT  18

// Journal
#define JOURNAL_STR_COUNT   1

// Debug
#define DBG_STS_STR_COUNT  21
#define DBG_DVC_STR_COUNT  4
#define DBG_GMS_STR_COUNT  17
#define DBG_EVT_STR_COUNT  15
#define DBG_STA_STR_COUNT  17

enum
{
	NAV_TAB_STR_BASE  = 0,
	TIP_WARN_STR_BASE = NAV_TAB_STR_BASE  + NAV_TAB_STR_COUNT,

	// General
	GEN_SETT_STR_BASE = TIP_WARN_STR_BASE + TIP_WARN_STR_COUNT,
	GEN_GFX_STR_BASE  = GEN_SETT_STR_BASE + GEN_SETT_STR_COUNT,
	GEN_ENG_STR_BASE  = GEN_GFX_STR_BASE  + GEN_GFX_STR_COUNT,
	GEN_AUD_STR_BASE  = GEN_ENG_STR_BASE  + GEN_ENG_STR_COUNT,
	GEN_CON_STR_BASE  = GEN_AUD_STR_BASE  + GEN_AUD_STR_COUNT,
	GEN_ADV_STR_BASE  = GEN_CON_STR_BASE  + GEN_CON_STR_COUNT,

	// Enhancements
	ENH_QOL_STR_BASE  = GEN_ADV_STR_BASE  + GEN_ADV_STR_COUNT,
	ENH_VIS_STR_BASE  = ENH_QOL_STR_BASE  + ENH_QOL_STR_COUNT,
	ENH_CHT_STR_BASE  = ENH_VIS_STR_BASE  + ENH_VIS_STR_COUNT,

	// Journal
	JOURNAL_STR_BASE  = ENH_CHT_STR_BASE  + ENH_CHT_STR_COUNT,

	// Debug
	DBG_STS_STR_BASE  = JOURNAL_STR_BASE  + JOURNAL_STR_COUNT,
	DBG_DVC_STR_BASE  = DBG_STS_STR_BASE  + DBG_STS_STR_COUNT,
	DBG_GMS_STR_BASE  = DBG_DVC_STR_BASE  + DBG_DVC_STR_COUNT,
	DBG_EVT_STR_BASE  = DBG_GMS_STR_BASE  + DBG_GMS_STR_COUNT,
	DBG_STA_STR_BASE  = DBG_EVT_STR_BASE  + DBG_EVT_STR_COUNT,

	IMGUI_STR_COUNT   = DBG_STA_STR_BASE  + DBG_STA_STR_COUNT
};

#define IMGUI_STRING(i) ((UNICODE *)GetStringAddress (SetAbsStringTableIndex (ImGuiStrings, (i))))
#define ImStr(i) IMGUI_STRING(i)

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_IMGUI_STRINGS_H_ */