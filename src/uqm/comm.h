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

#ifndef UQM_COMM_H_
#define UQM_COMM_H_

#include "commglue.h"
#include "globdata.h"
#include "units.h"
#include "libs/compiler.h"
#include "libs/gfxlib.h"
		// for CONVERSATION

#ifdef COMM_INTERNAL

#define SLIDER_Y (RES_SCALE(107) + IF_HD(46)) // JMS_GFX
#define SLIDER_HEIGHT RES_BOOL(15, 30) // JMS_GFX

#include "commanim.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern LOCDATA CommData;

static inline BOOLEAN
haveTalkingAnim (void)
{
	return CommData.AlienTalkDesc.NumFrames > 0;
}

static inline BOOLEAN
haveTransitionAnim (void)
{
	return CommData.AlienTransitionDesc.NumFrames > 0;
}

static inline BOOLEAN
wantTalkingAnim (void)
{
	return !(CommData.AlienTalkDesc.AnimFlags & PAUSE_TALKING);
}

static inline void
setRunTalkingAnim (void)
{
	CommData.AlienTalkDesc.AnimFlags |= WAIT_TALKING;
}

static inline void
clearRunTalkingAnim (void)
{
	CommData.AlienTalkDesc.AnimFlags &= ~WAIT_TALKING;
}

static inline BOOLEAN
runningTalkingAnim (void)
{
	return (CommData.AlienTalkDesc.AnimFlags & WAIT_TALKING);
}

static inline void
setRunIntroAnim (void)
{
	CommData.AlienTransitionDesc.AnimFlags |= TALK_INTRO;
}

static inline BOOLEAN
runningIntroAnim (void)
{
	return (CommData.AlienTransitionDesc.AnimFlags & TALK_INTRO);
}

static inline void
setStopTalkingAnim (void)
{
	CommData.AlienTalkDesc.AnimFlags |= TALK_DONE;
}

static inline void
clearStopTalkingAnim (void)
{
	CommData.AlienTalkDesc.AnimFlags &= ~TALK_DONE;
}

static inline BOOLEAN
signaledStopTalkingAnim (void)
{
	return CommData.AlienTalkDesc.AnimFlags & TALK_DONE;
}

#endif

#define TEXT_X_OFFS RES_SCALE(1) // JMS_GFX
#define TEXT_Y_OFFS RES_SCALE(1) // JMS_GFX
#define SIS_TEXT_WIDTH (SIS_SCREEN_WIDTH - (TEXT_X_OFFS << 1))

extern void init_communication (void);
extern void uninit_communication (void);

extern COUNT InitCommunication (CONVERSATION which_comm);
extern void RaceCommunication (void);

#define WAIT_TRACK_ALL  ((COUNT)~0)
extern void AlienTalkSegue (COUNT wait_track);
BOOLEAN getLineWithinWidth(TEXT *pText, const char **startNext,
		SIZE maxWidth, COUNT maxChars);

extern RECT CommWndRect; /* comm window rect */

typedef enum
{
	CIM_CROSSFADE_SPACE,
	CIM_CROSSFADE_WINDOW,
	CIM_CROSSFADE_SCREEN,
	CIM_FADE_IN_SCREEN,

	CIM_DEFAULT = CIM_CROSSFADE_SPACE,
} CommIntroMode;
extern void SetCommIntroMode (CommIntroMode, TimeCount howLong);

extern void EnableTalkingAnim (BOOLEAN enable);

extern BOOLEAN IsProbe;
extern BOOLEAN IsAltSong;

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_COMM_H_ */


