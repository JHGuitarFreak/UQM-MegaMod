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

#ifndef LIBS_VIDLIB_H_
#define LIBS_VIDLIB_H_

#include "libs/compiler.h"
#include "libs/sndlib.h"
#include "libs/reslib.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum
{
	NO_FMV = 0,
	HARDWARE_FMV,
	SOFTWARE_FMV
} VIDEO_TYPE;

typedef struct tfb_videoclip *VIDEO_REF;
typedef struct legacy_video_desc *LEGACY_VIDEO;
typedef struct legacy_video_ref *LEGACY_VIDEO_REF;

extern BOOLEAN InstallVideoResType (void);

extern BOOLEAN InitVideoPlayer (BOOLEAN UseCDROM);
extern void UninitVideoPlayer (void);

extern VIDEO_REF LoadVideoFile (const char *pStr);
extern BOOLEAN DestroyVideo (VIDEO_REF VidRef);
extern VIDEO_TYPE VidPlay (VIDEO_REF VidRef);
extern VIDEO_TYPE VidPlayEx (VIDEO_REF VidRef, MUSIC_REF AudRef,
		MUSIC_REF SpeechRef, DWORD LoopFrame);
#define VID_NO_LOOP (0U-1)
extern void VidStop (void);
extern VIDEO_REF VidPlaying (void);
extern BOOLEAN VidProcessFrame (void);
extern DWORD VidGetPosition (void);  // position in milliseconds
extern BOOLEAN VidSeek (DWORD pos); // position in milliseconds

extern LEGACY_VIDEO LoadLegacyVideoInstance (RESOURCE res);
extern BOOLEAN DestroyLegacyVideo (LEGACY_VIDEO vid);
extern LEGACY_VIDEO_REF PlayLegacyVideo (LEGACY_VIDEO vid);
extern void StopLegacyVideo (LEGACY_VIDEO_REF ref);
extern BOOLEAN PlayingLegacyVideo (LEGACY_VIDEO_REF ref);

#if defined(__cplusplus)
}
#endif

#endif /* LIBS_VIDLIB_H_ */
