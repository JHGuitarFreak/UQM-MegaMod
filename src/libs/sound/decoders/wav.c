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

/* Wave decoder (.wav adapter)
 * Code is based on Creative's Win32 OpenAL implementation.
 */

#include <stdio.h>
#include <errno.h>
#include "port.h"
#include "types.h"
#include "libs/uio.h"
#include "endian_uqm.h"
#include "libs/log.h"
#include "wav.h"

#define wave_MAKE_ID(x1, x2, x3, x4) \
		(((x4) << 24) | ((x3) << 16) | ((x2) << 8) | (x1))

#define wave_RiffID   wave_MAKE_ID('R', 'I', 'F', 'F')
#define wave_WaveID   wave_MAKE_ID('W', 'A', 'V', 'E')
#define wave_FmtID    wave_MAKE_ID('f', 'm', 't', ' ')
#define wave_DataID   wave_MAKE_ID('d', 'a', 't', 'a')

typedef struct
{
	uint32 id;
	uint32 size;
	uint32 type;
} wave_FileHeader;

typedef struct
{
	uint16 format;
	uint16 channels;
	uint32 samplesPerSec;
	uint32 bytesPerSec;
	uint16 blockAlign;
	uint16 bitsPerSample;
} wave_FormatHeader;

typedef struct
{
	uint32 id;
	uint32 size;
} wave_ChunkHeader;


#define THIS_PTR TFB_SoundDecoder* This

static const char* wava_GetName (void);
static bool wava_InitModule (int flags, const TFB_DecoderFormats*);
static void wava_TermModule (void);
static uint32 wava_GetStructSize (void);
static int wava_GetError (THIS_PTR);
static bool wava_Init (THIS_PTR);
static void wava_Term (THIS_PTR);
static bool wava_Open (THIS_PTR, uio_DirHandle *dir, const char *filename);
static void wava_Close (THIS_PTR);
static int wava_Decode (THIS_PTR, void* buf, sint32 bufsize);
static uint32 wava_Seek (THIS_PTR, uint32 pcm_pos);
static uint32 wava_GetFrame (THIS_PTR);

TFB_SoundDecoderFuncs wava_DecoderVtbl = 
{
	wava_GetName,
	wava_InitModule,
	wava_TermModule,
	wava_GetStructSize,
	wava_GetError,
	wava_Init,
	wava_Term,
	wava_Open,
	wava_Close,
	wava_Decode,
	wava_Seek,
	wava_GetFrame,
};

typedef struct tfb_wavesounddecoder
{
	// always the first member
	TFB_SoundDecoder decoder;

	// private
	sint32 last_error;
	uio_Stream *fp;
	wave_FormatHeader fmtHdr;
	uint32 data_ofs;
	uint32 data_size;
	uint32 max_pcm;
	uint32 cur_pcm;

} TFB_WaveSoundDecoder;

static const TFB_DecoderFormats* wava_formats = NULL;


static const char*
wava_GetName (void)
{
	return "Wave";
}

static bool
wava_InitModule (int flags, const TFB_DecoderFormats* fmts)
{
	wava_formats = fmts;
	return true;
	
	(void)flags;	// laugh at compiler warning
}

static void
wava_TermModule (void)
{
	// no specific module term
}

static uint32
wava_GetStructSize (void)
{
	return sizeof (TFB_WaveSoundDecoder);
}

static int
wava_GetError (THIS_PTR)
{
	TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;
	int ret = wava->last_error;
	wava->last_error = 0;
	return ret;
}

static bool
wava_Init (THIS_PTR)
{
	//TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;
	This->need_swap = wava_formats->want_big_endian;
	return true;
}

static void
wava_Term (THIS_PTR)
{
	//TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;
	wava_Close (This); // ensure cleanup
}

static bool
read_le_16 (uio_Stream *fp, uint16 *v)
{
	if (!uio_fread (v, sizeof(*v), 1, fp))
		return false;
	*v = UQM_SwapLE16 (*v);
	return true;
}

static bool
read_le_32 (uio_Stream *fp, uint32 *v)
{
	if (!uio_fread (v, sizeof(*v), 1, fp))
		return false;
	*v = UQM_SwapLE32 (*v);
	return true;
}

static bool
wava_readFileHeader (TFB_WaveSoundDecoder* wava, wave_FileHeader* hdr)
{
	if (!read_le_32 (wava->fp, &hdr->id) ||
			!read_le_32 (wava->fp, &hdr->size) ||
			!read_le_32 (wava->fp, &hdr->type))
	{
		wava->last_error = errno;
		return false;
	}
	return true;
}

static bool
wava_readChunkHeader (TFB_WaveSoundDecoder* wava, wave_ChunkHeader* chunk)
{
	if (!read_le_32 (wava->fp, &chunk->id) ||
			!read_le_32 (wava->fp, &chunk->size))
	{
		wava->last_error = errno;
		return false;
	}
	return true;
}

static bool
wava_readFormatHeader (TFB_WaveSoundDecoder* wava, wave_FormatHeader* fmt)
{
	if (!read_le_16 (wava->fp, &fmt->format) ||
			!read_le_16 (wava->fp, &fmt->channels) ||
			!read_le_32 (wava->fp, &fmt->samplesPerSec) ||
			!read_le_32 (wava->fp, &fmt->bytesPerSec) ||
			!read_le_16 (wava->fp, &fmt->blockAlign) ||
			!read_le_16 (wava->fp, &fmt->bitsPerSample))
	{
		wava->last_error = errno;
		return false;
	}
	return true;
}

static bool
wava_Open (THIS_PTR, uio_DirHandle *dir, const char *filename)
{
	TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;
	wave_FileHeader fileHdr;
	wave_ChunkHeader chunkHdr;
	long dataLeft; 

	wava->fp = uio_fopen (dir, filename, "rb");
	if (!wava->fp)
	{
		wava->last_error = errno;
		return false;
	}

	wava->data_size = 0;
	wava->data_ofs = 0;

	// read wave header
	if (!wava_readFileHeader (wava, &fileHdr))
	{
		wava->last_error = errno;
		wava_Close (This);
		return false;
	}
	if (fileHdr.id != wave_RiffID || fileHdr.type != wave_WaveID)
	{
		log_add (log_Warning, "wava_Open(): "
				"not a wave file, ID 0x%08x, Type 0x%08x",
				fileHdr.id, fileHdr.type);
		wava_Close (This);
		return false;
	}

	for (dataLeft = ((fileHdr.size + 1) & ~1) - 4; dataLeft > 0;
			dataLeft -= (((chunkHdr.size + 1) & ~1) + 8))
	{
		if (!wava_readChunkHeader (wava, &chunkHdr))
		{
			wava_Close (This);
			return false;
		}

		if (chunkHdr.id == wave_FmtID)
		{
			if (!wava_readFormatHeader (wava, &wava->fmtHdr))
			{
				wava_Close (This);
				return false;
			}
			uio_fseek (wava->fp, chunkHdr.size - 16, SEEK_CUR);
		}
		else
		{
			if (chunkHdr.id == wave_DataID)
			{
				wava->data_size = chunkHdr.size;
				wava->data_ofs = uio_ftell (wava->fp);
			}
			uio_fseek (wava->fp, chunkHdr.size, SEEK_CUR);
		}

		// 2-align the file ptr
		// XXX: I do not think this is necessary in WAVE files;
		//   possibly a remnant of ported AIFF reader
		uio_fseek (wava->fp, chunkHdr.size & 1, SEEK_CUR);
	}

	if (!wava->data_size || !wava->data_ofs)
	{
		log_add (log_Warning, "wava_Open(): bad wave file,"
				" no DATA chunk found");
		wava_Close (This);
		return false;
	}

	if (wava->fmtHdr.format != 0x0001)
	{	// not a PCM format
		log_add (log_Warning, "wava_Open(): unsupported format %x",
				wava->fmtHdr.format);
		wava_Close (This);
		return false;
	}
	if (wava->fmtHdr.channels != 1 && wava->fmtHdr.channels != 2)
	{
		log_add (log_Warning, "wava_Open(): unsupported number of channels %u",
				(unsigned)wava->fmtHdr.channels);
		wava_Close (This);
		return false;
	}

	if (dataLeft != 0)
		log_add (log_Warning, "wava_Open(): bad or unsupported wave file, "
				"size in header does not match read chunks");

	This->format = (wava->fmtHdr.channels == 1 ?
			(wava->fmtHdr.bitsPerSample == 8 ?
				wava_formats->mono8 : wava_formats->mono16)
			:
			(wava->fmtHdr.bitsPerSample == 8 ?
				wava_formats->stereo8 : wava_formats->stereo16)
			);
	This->frequency = wava->fmtHdr.samplesPerSec;

	uio_fseek (wava->fp, wava->data_ofs, SEEK_SET);
	wava->max_pcm = wava->data_size / wava->fmtHdr.blockAlign;
	wava->cur_pcm = 0;
	This->length = (float) wava->max_pcm / wava->fmtHdr.samplesPerSec;
	wava->last_error = 0;

	return true;
}

static void
wava_Close (THIS_PTR)
{
	TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;

	if (wava->fp)
	{
		uio_fclose (wava->fp);
		wava->fp = NULL;
	}
}

static int
wava_Decode (THIS_PTR, void* buf, sint32 bufsize)
{
	TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;
	uint32 dec_pcm;

	dec_pcm = bufsize / wava->fmtHdr.blockAlign;
	if (dec_pcm > wava->max_pcm - wava->cur_pcm)
		dec_pcm = wava->max_pcm - wava->cur_pcm;

	dec_pcm = uio_fread (buf, wava->fmtHdr.blockAlign, dec_pcm, wava->fp);
	wava->cur_pcm += dec_pcm;
	
	return dec_pcm * wava->fmtHdr.blockAlign;
}

static uint32
wava_Seek (THIS_PTR, uint32 pcm_pos)
{
	TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;

	if (pcm_pos > wava->max_pcm)
		pcm_pos = wava->max_pcm;
	wava->cur_pcm = pcm_pos;
	uio_fseek (wava->fp,
			wava->data_ofs + pcm_pos * wava->fmtHdr.blockAlign,
			SEEK_SET);

	return pcm_pos;
}

static uint32
wava_GetFrame (THIS_PTR)
{
	//TFB_WaveSoundDecoder* wava = (TFB_WaveSoundDecoder*) This;
	return 0; // only 1 frame for now

	(void)This;	// laugh at compiler warning
}
