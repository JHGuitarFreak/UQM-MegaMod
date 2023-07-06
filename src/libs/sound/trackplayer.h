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

#ifndef TRACKPLAYER_H
#define TRACKPLAYER_H

#include "libs/compiler.h"
#include "libs/callback.h"

#define ACCEL_SCROLL_SPEED 300

#define VERY_SLOW 0
#define SLOW 12
#define MODERATE_SPEED 16
#define FAST 24
#define VERY_FAST 32

static const BYTE speed_array[] =
{
	VERY_SLOW,
	SLOW,
	MODERATE_SPEED,
	FAST,
	VERY_FAST
};

extern void PlayTrack (void);
extern void StopTrack (void);
extern void JumpTrack (void);
extern void PauseTrack (void);
extern void ResumeTrack (void);
extern COUNT PlayingTrack (void);
extern COUNT GetSubtitleNumber (const UNICODE *sub);

extern void FastReverse_Smooth (void);
extern void FastForward_Smooth (void);
extern void FastReverse_Page (void);
extern void FastForward_Page (void);

extern void SpliceTrack (UNICODE *filespec, UNICODE *textspec, UNICODE *TimeStamp, CallbackFunction cb);
extern void SpliceMultiTrack (UNICODE *TrackNames[], UNICODE *TrackText);

extern int GetTrackPosition (int in_units);

typedef struct tfb_soundchunk *SUBTITLE_REF;

extern SUBTITLE_REF GetFirstTrackSubtitle (void);
extern SUBTITLE_REF GetNextTrackSubtitle (SUBTITLE_REF LastRef);
extern const UNICODE *GetTrackSubtitleText (SUBTITLE_REF SubRef);

extern const UNICODE *GetTrackSubtitle (void);
extern COUNT GetSubtitleNumberByTrack (COUNT track);
extern DWORD RecalculateDelay (DWORD numChars, BOOLEAN talk);

#endif
