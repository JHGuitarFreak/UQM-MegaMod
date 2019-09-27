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

#include "video.h"

#include "vidintrn.h"
#include "options.h"
#include "vidplayer.h"
#include "libs/memlib.h"
#include "libs/sndlib.h"
#include "uqm/units.h"


#define NULL_VIDEO_REF	(0)
static VIDEO_REF _cur_video = NULL_VIDEO_REF;
static MUSIC_REF _cur_speech = 0;

BOOLEAN
InitVideoPlayer (BOOLEAN useCDROM)
		//useCDROM doesn't really apply to us
{
	TFB_PixelFormat fmt;
	
	TFB_DrawCanvas_GetScreenFormat (&fmt);
	if (!VideoDecoder_Init (0, fmt.BitsPerPixel, fmt.Rmask,
			fmt.Gmask, fmt.Bmask, 0))
		return FALSE;

	return TFB_InitVideoPlayer ();
	
	(void)useCDROM;  /* dodge compiler warning */
}

void
UninitVideoPlayer (void)
{
	TFB_UninitVideoPlayer ();
	VideoDecoder_Uninit ();
}

void
VidStop (void)
{
	if (_cur_speech)
		snd_StopSpeech ();
	if (_cur_video)
		TFB_StopVideo (_cur_video);
	_cur_speech = 0;
	_cur_video = NULL_VIDEO_REF;
}

VIDEO_REF
VidPlaying (void)
		// this should just probably return BOOLEAN
{
	if (!_cur_video)
		return NULL_VIDEO_REF;
	
	if (TFB_VideoPlaying (_cur_video))
		return _cur_video;

	return NULL_VIDEO_REF;
}

BOOLEAN
VidProcessFrame (void)
{
	if (!_cur_video)
		return FALSE;
	return TFB_ProcessVideoFrame (_cur_video);
}

// return current video position in milliseconds
DWORD
VidGetPosition (void)
{
	if (!VidPlaying ())
		return 0;
	return TFB_GetVideoPosition (_cur_video);
}

BOOLEAN
VidSeek (DWORD pos)
		// pos in milliseconds
{
	if (!VidPlaying ())
		return FALSE;
	return TFB_SeekVideo (_cur_video, pos);
}

VIDEO_TYPE
VidPlayEx (VIDEO_REF vid, MUSIC_REF AudRef, MUSIC_REF SpeechRef,
		DWORD LoopFrame)
{
	VIDEO_TYPE ret;

	if (!vid)
		return NO_FMV;

	if (AudRef)
	{
		if (vid->hAudio)
			DestroyMusic (vid->hAudio);
		vid->hAudio = AudRef;
		vid->decoder->audio_synced = FALSE;
	}

	vid->loop_frame = LoopFrame;
	vid->loop_to = 0;

	if (_cur_speech)
		snd_StopSpeech ();
	if (_cur_video)
		TFB_StopVideo (_cur_video);
	_cur_speech = 0;
	_cur_video = NULL_VIDEO_REF;

	// play video in the center of the screen
	if (TFB_PlayVideo (vid, (ScreenWidth - vid->w) / 2, (ScreenHeight - vid->h) / 2)) {
		_cur_video = vid;
		ret = SOFTWARE_FMV;
		if (SpeechRef) {
			snd_PlaySpeech (SpeechRef);
			_cur_speech = SpeechRef;
		}
	}
	else
	{
		ret = NO_FMV;
	}

	return ret;
}

VIDEO_TYPE
VidPlay (VIDEO_REF VidRef)
{
	return VidPlayEx (VidRef, 0, 0, VID_NO_LOOP);
}

VIDEO_REF
_init_video_file (const char *pStr)
{
	TFB_VideoClip* vid;
	TFB_VideoDecoder* dec;

	dec = VideoDecoder_Load (contentDir, pStr);
	if (!dec)
		return NULL_VIDEO_REF;

	vid = HCalloc (sizeof (*vid));
	vid->decoder = dec;
	vid->length = dec->length;
	vid->w = vid->decoder->w - IF_HD(4);
	vid->h = vid->decoder->h + IF_HD(7);
	vid->guard = CreateMutex ("video guard", SYNC_CLASS_VIDEO);

	return (VIDEO_REF) vid;
}

BOOLEAN
DestroyVideo (VIDEO_REF vid)
{
	if (!vid)
		return FALSE;

	// just some armouring; should already be stopped
	TFB_StopVideo (vid);

	VideoDecoder_Free (vid->decoder);
	DestroyMutex (vid->guard);
	HFree (vid);
	
	return TRUE;
}
