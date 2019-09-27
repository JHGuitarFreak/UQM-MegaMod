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

#ifndef UQM_SETUP_H_
#define UQM_SETUP_H_

#include "displist.h"
#include "globdata.h"
#include "libs/reslib.h"
#include "libs/sndlib.h"
#include "libs/gfxlib.h"
#include "libs/threadlib.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern RESOURCE_INDEX hResIndex;

extern FRAME Screen;
extern FRAME ActivityFrame;
extern FRAME StatusFrame;
extern FRAME SubmenuFrame; // JMS
extern FRAME ConstellationsFrame; // JMS
extern FRAME NebulaeFrame;	// JMS
extern FRAME hyperspacesuns;	// BW
extern FRAME FlagStatFrame;
extern FRAME MiscDataFrame;
extern FRAME FontGradFrame;
extern FRAME BorderFrame;

extern CONTEXT OffScreenContext;
		// OffScreenContext can often refer to a deleted ForeGroundFrame
		// Always call SetContextFGFrame() before drawing anything to it
		// Neither is the state of its ClipRect guaranteed.
extern CONTEXT ScreenContext;
extern CONTEXT SpaceContext;
extern CONTEXT StatusContext;

extern SIZE screen_width, screen_height;

extern FONT StarConFont;
extern FONT MicroFont;
extern FONT TinyFont;
extern FONT PlyrFont;
extern FONT MeleeFont;

extern CondVar RenderingCond;

extern QUEUE race_q[];
		/* Array of lists of ships involved in a battle, one queue per side;
		 * queue element is STARSHIP */

// Serosis
extern BOOLEAN solTexturesPresent;
extern BOOLEAN SyreenVoiceFix;
extern BOOLEAN HDPackPresent;
extern BOOLEAN comingFromInit;
extern BOOLEAN EndlessSCLoaded;
extern BOOLEAN VolasPackPresent;
extern BYTE Rando;
extern BOOLEAN DeathBySurrender;
extern BOOLEAN DeathByMelee;
extern BOOLEAN DeathBySuicide;

#define DEATH_MARCH 0
#define DIED_IN_BATTLE 1
#define SUICIDE 2
#define SURRENDERED 3

extern ACTIVITY LastActivity;

extern BYTE PlayerControl[];

extern BOOLEAN usingSpeech;
		// Actual speech presence indicator which decouples reality from
		// the user option, thus the user option remains as pure intent

BOOLEAN InitContexts (void);
void UninitPlayerInput (void);
BOOLEAN InitGameKernel (void);
void UninitGameKernel (void);

extern BOOLEAN LoadKernel (int argc, char *argv[], BOOLEAN ReloadPackages);
extern void FreeKernel (void);

int initIO (void);
void uninitIO (void);

bool SetPlayerInput (COUNT playerI);
bool SetPlayerInputAll (void);
void ClearPlayerInput (COUNT playerI);
void ClearPlayerInputAll (void);


#if defined(__cplusplus)
}
#endif

#endif  /* UQM_SETUP_H_ */


