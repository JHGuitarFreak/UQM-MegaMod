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

#include "vidintrn.h"
#include "video.h"
#include "vidplayer.h"
#include "libs/memlib.h"


LEGACY_VIDEO_REF
PlayLegacyVideo (LEGACY_VIDEO vid)
{
	const char *name, *audname, *speechname;
	uint32 loopframe;
	LEGACY_VIDEO_REF ref;
	VIDEO_TYPE type;

	if (!vid)
		return NULL;
	ref = HCalloc (sizeof (*ref));
	if (!ref)
		return NULL;
	name = vid->video;
	audname = vid->audio;
	speechname = vid->speech;
	loopframe = vid->loop;

	ref->vidref = LoadVideoFile (name);
	if (!ref->vidref)
		return NULL;
	if (audname)
		ref->audref = LoadMusicFile (audname);
	if (speechname)
		ref->speechref = LoadMusicFile (speechname);

	type = VidPlayEx (ref->vidref, ref->audref, ref->speechref, loopframe);
	if (type == NO_FMV)
	{	// Video failed to start
		StopLegacyVideo (ref);
		return NULL;
	}
	
	return ref;
}

void
StopLegacyVideo (LEGACY_VIDEO_REF ref)
{
	if (!ref)
		return;
	VidStop ();

	DestroyVideo (ref->vidref);
	if (ref->speechref)
		DestroyMusic (ref->speechref);
	if (ref->audref)
		DestroyMusic (ref->audref);

	HFree (ref);
}

BOOLEAN
PlayingLegacyVideo (LEGACY_VIDEO_REF ref)
{
	if (!ref)
		return FALSE;
	return TFB_VideoPlaying (ref->vidref);
}
