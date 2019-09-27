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

/* Ogg Vorbis decoder (.ogg adapter)
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "libs/log.h"
#include "port.h"
#include "types.h"
#include "libs/uio.h"
#include "decoder.h"
#ifdef OVCODEC_TREMOR
#	include <tremor/ivorbiscodec.h>
#	include <tremor/ivorbisfile.h>
#else
#	include <vorbis/codec.h>
#	include <vorbis/vorbisfile.h>
#endif  /* OVCODEC_TREMOR */
#include "oggaud.h"


#define THIS_PTR TFB_SoundDecoder* This

static const char* ova_GetName (void);
static bool ova_InitModule (int flags, const TFB_DecoderFormats*);
static void ova_TermModule (void);
static uint32 ova_GetStructSize (void);
static int ova_GetError (THIS_PTR);
static bool ova_Init (THIS_PTR);
static void ova_Term (THIS_PTR);
static bool ova_Open (THIS_PTR, uio_DirHandle *dir, const char *filename);
static void ova_Close (THIS_PTR);
static int ova_Decode (THIS_PTR, void* buf, sint32 bufsize);
static uint32 ova_Seek (THIS_PTR, uint32 pcm_pos);
static uint32 ova_GetFrame (THIS_PTR);

TFB_SoundDecoderFuncs ova_DecoderVtbl = 
{
	ova_GetName,
	ova_InitModule,
	ova_TermModule,
	ova_GetStructSize,
	ova_GetError,
	ova_Init,
	ova_Term,
	ova_Open,
	ova_Close,
	ova_Decode,
	ova_Seek,
	ova_GetFrame,
};

typedef struct tfb_oggsounddecoder
{
	// always the first member
	TFB_SoundDecoder decoder;

	// private
	sint32 last_error;
	OggVorbis_File vf;

} TFB_OggSoundDecoder;

static const TFB_DecoderFormats* ova_formats = NULL;

static size_t
ogg_read (void *ptr, size_t size, size_t nmemb, void *datasource)
{
	return uio_fread (ptr, size, nmemb, (uio_Stream *) datasource);
}

static int
ogg_seek (void *datasource, ogg_int64_t offset, int whence)
{
	long off = (long) offset;
	return uio_fseek ((uio_Stream *) datasource, off, whence);
}

static int
ogg_close (void *datasource)
{
	return uio_fclose ((uio_Stream *) datasource);
}

static long
ogg_tell (void *datasource)
{
	return uio_ftell ((uio_Stream *) datasource);
}

static const ov_callbacks ogg_callbacks = 
{
	ogg_read,
	ogg_seek,
	ogg_close, 
	ogg_tell,
};

static const char*
ova_GetName (void)
{
	return "Ogg Vorbis";
}

static bool
ova_InitModule (int flags, const TFB_DecoderFormats* fmts)
{
	ova_formats = fmts;
	return true;
	
	(void)flags;	// laugh at compiler warning
}

static void
ova_TermModule (void)
{
	// no specific module term
}

static uint32
ova_GetStructSize (void)
{
	return sizeof (TFB_OggSoundDecoder);
}

static int
ova_GetError (THIS_PTR)
{
	TFB_OggSoundDecoder* ova = (TFB_OggSoundDecoder*) This;
	int ret = ova->last_error;
	ova->last_error = 0;
	return ret;
}

static bool
ova_Init (THIS_PTR)
{
	//TFB_OggSoundDecoder* ova = (TFB_OggSoundDecoder*) This;
	This->need_swap = false;
	return true;
}

static void
ova_Term (THIS_PTR)
{
	//TFB_OggSoundDecoder* ova = (TFB_OggSoundDecoder*) This;
	ova_Close (This); // ensure cleanup
}

static bool
ova_Open (THIS_PTR, uio_DirHandle *dir, const char *filename)
{
	TFB_OggSoundDecoder* ova = (TFB_OggSoundDecoder*) This;
	int rc;
	uio_Stream *fp;
	vorbis_info *vinfo;
	
	fp = uio_fopen (dir, filename, "rb");
	if (fp == NULL)
	{
		log_add (log_Warning, "ova_Open(): could not open %s", filename);
		return false;
	}

	rc = ov_open_callbacks (fp, &ova->vf, NULL, 0, ogg_callbacks);
	if (rc != 0)
	{
		log_add (log_Warning, "ova_Open(): "
				"ov_open_callbacks failed for %s, error code %d",
				filename, rc);
		uio_fclose (fp);
		return false;
	}

	vinfo = ov_info (&ova->vf, -1);
	if (!vinfo)
	{
		log_add (log_Warning, "ova_Open(): "
				"failed to retrieve ogg bitstream info for %s",
				filename);
	    ov_clear (&ova->vf);
		return false;
	}
	
	This->frequency = vinfo->rate;
#ifdef OVCODEC_TREMOR
	// With tremor ov_time_total returns an integer, in milliseconds.
	This->length = ((float) ov_time_total (&ova->vf, -1)) / 1000.0f;
#else
	// With libvorbis ov_time_total returns a double, in seconds.
	This->length = (float) ov_time_total (&ova->vf, -1);
#endif  /* OVCODEC_TREMOR */
	
	if (vinfo->channels == 1)
		This->format = ova_formats->mono16;
	else
		This->format = ova_formats->stereo16;

	ova->last_error = 0;

	return true;
}

static void
ova_Close (THIS_PTR)
{
	TFB_OggSoundDecoder* ova = (TFB_OggSoundDecoder*) This;

	ov_clear (&ova->vf);
}

static int
ova_Decode (THIS_PTR, void* buf, sint32 bufsize)
{
	TFB_OggSoundDecoder* ova = (TFB_OggSoundDecoder*) This;
	long rc;
	int bitstream;

#ifdef OVCODEC_TREMOR
	rc = ov_read (&ova->vf, buf, bufsize, &bitstream);
#else
	rc = ov_read (&ova->vf, buf, bufsize, ova_formats->want_big_endian,
			2, 1, &bitstream);
#endif  /* OVCODEC_TREMOR */
	
	if (rc < 0)
		ova->last_error = rc;
	else
		ova->last_error = 0;

	return rc;
}

static uint32
ova_Seek (THIS_PTR, uint32 pcm_pos)
{
	TFB_OggSoundDecoder* ova = (TFB_OggSoundDecoder*) This;
	int ret;

	ret = ov_pcm_seek (&ova->vf, pcm_pos);
	if (ret != 0)
	{
		ova->last_error = ret;
		return (uint32) ov_pcm_tell (&ova->vf);
	}
	else
		return pcm_pos;
}

static uint32
ova_GetFrame (THIS_PTR)
{
	TFB_OggSoundDecoder* ova = (TFB_OggSoundDecoder*) This;
	// this is the closest to a frame there is in ogg vorbis stream
	// doesn't seem to be a func to retrive it
#ifdef OVCODEC_TREMOR
#if defined(ANDROID) || defined(__ANDROID__)
	return ova->vf.os.pageno;
#else
	return ova->vf.os->pageno;
#endif
#else
	return ova->vf.os.pageno;
#endif  /* OVCODEC_TREMOR */
}

