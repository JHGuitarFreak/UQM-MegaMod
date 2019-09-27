// Copyright 2008 Michael Martin

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

#include <stdlib.h>
#include <string.h>
#include "vidintrn.h"
#include "libs/log.h"
#include "libs/memlib.h"


static BOOLEAN
FreeLegacyVideoData (void *data)
{
	LEGACY_VIDEO pLV;
	if (!data)
		return FALSE;
		
	pLV = (LEGACY_VIDEO) data;
	if (pLV->video)
		HFree (pLV->video);
	if (pLV->audio)
		HFree (pLV->audio);
	if (pLV->speech)
		HFree (pLV->speech);
	HFree (pLV);
	
	return TRUE;
}

static void
GetLegacyVideoData (const char *path, RESOURCE_DATA *resdata)
{
	void *result = NULL;
	char paths[1024], *audio_path, *speech_path, *loop_str;
	uint32 LoopFrame = VID_NO_LOOP;

	/* Parse out the video components. */
	strncpy (paths, path, 1023);
	paths[1023] = '\0';
	audio_path = strchr (paths, ':');
	if (audio_path == NULL)
	{
		speech_path = NULL;
		loop_str = NULL;
	}
	else
	{
		*audio_path = '\0';
		audio_path++;

		speech_path = strchr (audio_path, ':');
		if (speech_path == NULL)
		{
			loop_str = NULL;
		}
		else
		{
			*speech_path = '\0';
			speech_path++;
			
			loop_str = strchr (speech_path, ':');
			if (loop_str != NULL) {
				*loop_str = '\0';
				loop_str++;
			}
		}
	}

	log_add (log_Info, "\t'%s' -- video", paths);
	if (audio_path)
		log_add (log_Info, "\t'%s' -- audio", audio_path);
	else
		log_add (log_Info, "\tNo associated audio");
	if (speech_path)
		log_add (log_Info, "\t'%s' -- speech path", speech_path);
	else
		log_add (log_Info, "\tNo associated speech");
	if (loop_str)
	{
		char *end;
		LoopFrame = strtol (loop_str, &end, 10);
		// We allow whitespace at the end, but nothing printable.
		if (*end > 32) {
			log_add (log_Warning, "Warning: Unparsable loop frame '%s'. Disabling loop.", loop_str);
			LoopFrame = VID_NO_LOOP;
		}
		log_add (log_Info, "\tLoop frame is %u", LoopFrame);
	} 
	else
		log_add (log_Info, "\tNo specified loop frame");
		
	result = HMalloc (sizeof (LEGACY_VIDEO_DESC));
	if (result)
	{
		LEGACY_VIDEO pLV = (LEGACY_VIDEO) result;
		int len;
		pLV->video = NULL;
		pLV->audio = NULL;
		pLV->speech = NULL;
		pLV->loop = LoopFrame;
		
		len = strlen(paths)+1;
		pLV->video = (char *)HMalloc (len);
		if (!pLV->video)
		{
			log_add (log_Warning, "Warning: Couldn't allocate space for '%s'", paths);
			goto err;
		}
		strncpy(pLV->video, paths, len);

		if (audio_path)
		{
			len = strlen(audio_path)+1;
			pLV->audio = (char *)HMalloc (len);
			if (!pLV->audio)
			{
				log_add (log_Warning, "Warning: Couldn't allocate space for '%s'", audio_path);
				goto err;
			}
			strncpy(pLV->audio, audio_path, len);
		}
		
		if (speech_path)
		{
			len = strlen(speech_path)+1;
			pLV->speech = (char *)HMalloc (len);
			if (!pLV->speech)
			{
				log_add (log_Warning, "Warning: Couldn't allocate space for '%s'", speech_path);
				goto err;
			}
			strncpy(pLV->speech, speech_path, len);
		}
		
		resdata->ptr = result;
	}
	return;
err:
	if (result)
		FreeLegacyVideoData ((LEGACY_VIDEO)result);

	resdata->ptr = NULL;
	return;
}

BOOLEAN
InstallVideoResType (void)
{
	InstallResTypeVectors ("3DOVID", GetLegacyVideoData, FreeLegacyVideoData, NULL);
	return TRUE;
}

LEGACY_VIDEO
LoadLegacyVideoInstance (RESOURCE res)
{
	void *data;
	
	data = res_GetResource (res);
	if (data)
	{
		res_DetachResource (res);
	}
	
	return (LEGACY_VIDEO)data;
}

BOOLEAN
DestroyLegacyVideo (LEGACY_VIDEO vid)
{
	return FreeLegacyVideoData (vid);
}
