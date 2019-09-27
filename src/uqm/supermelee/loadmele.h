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

#ifndef UQM_SUPERMELEE_LOADMELE_H_
#define UQM_SUPERMELEE_LOADMELE_H_

#define LOAD_TEAM_VIEW_SIZE 5

struct melee_load_state;

#include "melee.h"
#include "meleesetup.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct melee_load_state
{
	MeleeTeam **preBuiltList;
	COUNT preBuiltCount;
	
	DIRENTRY dirEntries;
	COUNT *entryIndices;
	COUNT numIndices;

	MeleeTeam *view[LOAD_TEAM_VIEW_SIZE];
	COUNT top;
			// Index of the first entry for the view.
	COUNT bot;
			// Index of the first entry past the end of the view.

	COUNT cur;
			// Index of the current position in the view.
	COUNT viewSize;
			// Number of entries in the view.
};

void InitMeleeLoadState (MELEE_STATE *pMS);
void UninitMeleeLoadState (MELEE_STATE *pMS);

BOOLEAN DoLoadTeam (MELEE_STATE *pMS);
BOOLEAN DoSaveTeam (MELEE_STATE *pMS);
bool ReadTeamImage (MeleeTeam *pTI, uio_Stream *load_fp);
int WriteTeamImage (const MeleeTeam *pTI, uio_Stream *save_fp);
void LoadTeamList (MELEE_STATE *pMS);

#if defined(__cplusplus)
}
#endif

#endif /* UQM_SUPERMELEE_LOADMELE_H_ */


