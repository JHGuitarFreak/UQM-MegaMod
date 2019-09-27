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

/* Sound file decoder for .wav, .mod, .ogg (to be used with OpenAL)
 * API is heavily influenced by SDL_sound.
 */

#include <string.h>
#include <stdlib.h>
#include "port.h"
#include "libs/memlib.h"
#include "libs/file.h"
#include "libs/log.h"
#include "decoder.h"
#include "wav.h"
#include "dukaud.h"
#include "modaud.h"
#ifndef OVCODEC_NONE
#	include "oggaud.h"
#endif  /* OVCODEC_NONE */
#include "aiffaud.h"


#define MAX_REG_DECODERS 31

#define THIS_PTR TFB_SoundDecoder*

static const char* bufa_GetName (void);
static bool bufa_InitModule (int flags, const TFB_DecoderFormats*);
static void bufa_TermModule (void);
static uint32 bufa_GetStructSize (void);
static int bufa_GetError (THIS_PTR);
static bool bufa_Init (THIS_PTR);
static void bufa_Term (THIS_PTR);
static bool bufa_Open (THIS_PTR, uio_DirHandle *dir, const char *filename);
static void bufa_Close (THIS_PTR);
static int bufa_Decode (THIS_PTR, void* buf, sint32 bufsize);
static uint32 bufa_Seek (THIS_PTR, uint32 pcm_pos);
static uint32 bufa_GetFrame (THIS_PTR);

TFB_SoundDecoderFuncs bufa_DecoderVtbl = 
{
	bufa_GetName,
	bufa_InitModule,
	bufa_TermModule,
	bufa_GetStructSize,
	bufa_GetError,
	bufa_Init,
	bufa_Term,
	bufa_Open,
	bufa_Close,
	bufa_Decode,
	bufa_Seek,
	bufa_GetFrame,
};

typedef struct tfb_bufsounddecoder
{
	// always the first member
	TFB_SoundDecoder decoder;

	// private
	void* data;
	uint32 max_pcm;
	uint32 cur_pcm;

} TFB_BufSoundDecoder;

#define SD_MIN_SIZE   (sizeof (TFB_BufSoundDecoder))

static const char* nula_GetName (void);
static bool nula_InitModule (int flags, const TFB_DecoderFormats*);
static void nula_TermModule (void);
static uint32 nula_GetStructSize (void);
static int nula_GetError (THIS_PTR);
static bool nula_Init (THIS_PTR);
static void nula_Term (THIS_PTR);
static bool nula_Open (THIS_PTR, uio_DirHandle *dir, const char *filename);
static void nula_Close (THIS_PTR);
static int nula_Decode (THIS_PTR, void* buf, sint32 bufsize);
static uint32 nula_Seek (THIS_PTR, uint32 pcm_pos);
static uint32 nula_GetFrame (THIS_PTR);

TFB_SoundDecoderFuncs nula_DecoderVtbl = 
{
	nula_GetName,
	nula_InitModule,
	nula_TermModule,
	nula_GetStructSize,
	nula_GetError,
	nula_Init,
	nula_Term,
	nula_Open,
	nula_Close,
	nula_Decode,
	nula_Seek,
	nula_GetFrame,
};

typedef struct tfb_nullsounddecoder
{
	// always the first member
	TFB_SoundDecoder decoder;

	// private
	uint32 cur_pcm;

} TFB_NullSoundDecoder;

#undef THIS_PTR


struct TFB_RegSoundDecoder
{
	bool builtin;
	bool used;        // ever used indicator
	const char* ext;
	const TFB_SoundDecoderFuncs* funcs;
};
static TFB_RegSoundDecoder sd_decoders[MAX_REG_DECODERS + 1] = 
{
	{true,  true,  "wav", &wava_DecoderVtbl},
	{true,  true,  "mod", &moda_DecoderVtbl},
#ifndef OVCODEC_NONE
	{true,  true,  "ogg", &ova_DecoderVtbl},
#endif  /* OVCODEC_NONE */
	{true,  true,  "duk", &duka_DecoderVtbl},
	{true,  true,  "aif", &aifa_DecoderVtbl},
	{false, false,  NULL, NULL}, // null term
};

static TFB_DecoderFormats decoder_formats;
static int sd_flags = 0;

/* change endianness of 16bit words
 * Only works optimal when 'data' is aligned on a 32 bits boundary.
 */
void
SoundDecoder_SwapWords (uint16* data, uint32 size)
{
	uint32 fsize = size & (~3U);

	size -= fsize;
	fsize >>= 2;
	for (; fsize; fsize--, data += 2)
	{
		uint32 v = *(uint32*)data;
		*(uint32*)data = ((v & 0x00ff00ff) << 8)
				| ((v & 0xff00ff00) >> 8);
	}
	if (size)
	{
		/* leftover word */
		*data = ((*data & 0x00ff) << 8) | ((*data & 0xff00) >> 8);
	}
}

const char*
SoundDecoder_GetName (TFB_SoundDecoder *decoder)
{
	if (!decoder || !decoder->funcs)
		return "(Null)";
	return decoder->funcs->GetName ();
}

sint32
SoundDecoder_Init (int flags, TFB_DecoderFormats *formats)
{
	TFB_RegSoundDecoder* info;
	sint32 ret = 0;

	if (!formats)
	{
		log_add (log_Error, "SoundDecoder_Init(): missing decoder formats");
		return 1;		
	}
	decoder_formats = *formats;

	// init built-in decoders
	for (info = sd_decoders; info->ext; info++)
	{
		if (!info->funcs->InitModule (flags, &decoder_formats))
		{
			log_add (log_Error, "SoundDecoder_Init(): "
					"%s audio decoder init failed",
					info->funcs->GetName ());
			ret = 1;
		}
	}

	sd_flags = flags;

	return ret;
}

void
SoundDecoder_Uninit (void)
{
	TFB_RegSoundDecoder* info;

	// uninit all decoders
	// and unregister loaded decoders
	for (info = sd_decoders; info->used; info++)
	{
		if (info->ext) // check if present
			info->funcs->TermModule ();
		
		if (!info->builtin)
		{
			info->used = false;
			info->ext = NULL;
		}
	}
}

TFB_RegSoundDecoder*
SoundDecoder_Register (const char* fileext, TFB_SoundDecoderFuncs* decvtbl)
{
	TFB_RegSoundDecoder* info;
	TFB_RegSoundDecoder* newslot = NULL;

	if (!decvtbl)
	{
		log_add (log_Warning, "SoundDecoder_Register(): Null decoder table");
		return NULL;
	}
	if (!fileext)
	{
		log_add (log_Warning, "SoundDecoder_Register(): Bad file type for %s",
				decvtbl->GetName ());
		return NULL;
	}

	// check if extension already registered
	for (info = sd_decoders; info->used &&
			(!info->ext || strcmp (info->ext, fileext) != 0);
			++info)
	{
		// and pick up an empty slot (where available)
		if (!newslot && !info->ext)
			newslot = info;
	}

	if (info >= sd_decoders + MAX_REG_DECODERS)
	{
		log_add (log_Warning, "SoundDecoder_Register(): Decoders limit reached");
		return NULL;
	}
	else if (info->ext)
	{
		log_add (log_Warning, "SoundDecoder_Register(): "
				"'%s' decoder already registered (%s denied)",
				fileext, decvtbl->GetName ());
		return NULL;
	}
	
	if (!decvtbl->InitModule (sd_flags, &decoder_formats))
	{
		log_add (log_Warning, "SoundDecoder_Register(): %s decoder init failed",
				decvtbl->GetName ());
		return NULL;
	}

	if (!newslot)
	{
		newslot = info;
		newslot->used = true;
		// make next one a term
		info[1].builtin = false;
		info[1].used = false;
		info[1].ext = NULL;
	}

	newslot->ext = fileext;
	newslot->funcs = decvtbl;
	
	return newslot;
}

void
SoundDecoder_Unregister (TFB_RegSoundDecoder* regdec)
{
	if (regdec < sd_decoders || regdec >= sd_decoders + MAX_REG_DECODERS ||
			!regdec->ext || !regdec->funcs)
	{
		log_add (log_Warning, "SoundDecoder_Unregister(): "
				"Invalid or expired decoder passed");
		return;
	}
	
	regdec->funcs->TermModule ();
	regdec->ext = NULL;
	regdec->funcs = NULL;
}

const TFB_SoundDecoderFuncs*
SoundDecoder_Lookup (const char* fileext)
{
	TFB_RegSoundDecoder* info;

	for (info = sd_decoders; info->used &&
			(!info->ext || strcmp (info->ext, fileext) != 0);
			++info)
		;
	return info->ext ? info->funcs : NULL;
}

TFB_SoundDecoder*
SoundDecoder_Load (uio_DirHandle *dir, char *filename,
		uint32 buffer_size, uint32 startTime, sint32 runTime)
			// runTime < 0 specifies a default length for a nul decoder
{	
	const char* pext;
	TFB_RegSoundDecoder* info;
	const TFB_SoundDecoderFuncs* funcs;
	TFB_SoundDecoder* decoder;
	uint32 struct_size;

	pext = strrchr (filename, '.');
	if (!pext)
	{
		log_add (log_Warning, "SoundDecoder_Load(): Unknown file type (%s)",
				filename);
		return NULL;
	}
	++pext;

	for (info = sd_decoders; info->used &&
			(!info->ext || strcmp (info->ext, pext) != 0);
			++info)
		;
	if (!info->ext)
	{
		log_add (log_Warning, "SoundDecoder_Load(): Unsupported file type (%s)",
			filename);			

		if (runTime)
		{
			runTime = abs (runTime);
			startTime = 0;
			funcs = &nula_DecoderVtbl;
		}
		else
		{		
			return NULL;
		}
	}
	else
	{
		funcs = info->funcs;
	}

	if (!fileExists2 (dir, filename))
	{
		if (runTime)
		{
			runTime = abs (runTime);
			startTime = 0;
			funcs = &nula_DecoderVtbl;
		}
		else
		{
			log_add (log_Warning, "SoundDecoder_Load(): %s does not exist",
					filename);
			return NULL;
		}
	}

	struct_size = funcs->GetStructSize ();
	if (struct_size < SD_MIN_SIZE)
		struct_size = SD_MIN_SIZE;

	decoder = (TFB_SoundDecoder*) HCalloc (struct_size);
	decoder->funcs = funcs;
	if (!decoder->funcs->Init (decoder))
	{
		log_add (log_Warning, "SoundDecoder_Load(): "
				"%s decoder instance failed init",
				decoder->funcs->GetName ());
		HFree (decoder);
		return NULL;
	}

	if (!decoder->funcs->Open (decoder, dir, filename))
	{
		log_add (log_Warning, "SoundDecoder_Load(): "
				"%s decoder could not load %s",
				decoder->funcs->GetName (), filename);
		decoder->funcs->Term (decoder);
		HFree (decoder);
		return NULL;
	}

	decoder->buffer = HMalloc (buffer_size);
	decoder->buffer_size = buffer_size;
	decoder->looping = false;
	decoder->error = SOUNDDECODER_OK;
	decoder->dir = dir;
	decoder->filename = (char *) HMalloc (strlen (filename) + 1);
	strcpy (decoder->filename, filename);

	if (decoder->is_null)
	{	// fake decoder, keeps voiceovers and etc. going
		decoder->length = (float) (runTime / 1000.0);
	}

	decoder->length -= startTime / 1000.0f;
	if (decoder->length < 0)
		decoder->length = 0;
	else if (runTime > 0 && runTime / 1000.0 < decoder->length)
		decoder->length = (float)(runTime / 1000.0);

	decoder->start_sample = (uint32)(startTime / 1000.0f * decoder->frequency);
	decoder->end_sample = decoder->start_sample + 
			(unsigned long)(decoder->length * decoder->frequency);
	if (decoder->start_sample != 0)
		decoder->funcs->Seek (decoder, decoder->start_sample);

	if (decoder->format == decoder_formats.mono8)
		decoder->bytes_per_samp = 1;
	else if (decoder->format == decoder_formats.mono16)
		decoder->bytes_per_samp = 2;
	else if (decoder->format == decoder_formats.stereo8)
		decoder->bytes_per_samp = 2;
	else if (decoder->format == decoder_formats.stereo16)
		decoder->bytes_per_samp = 4;

	decoder->pos = decoder->start_sample * decoder->bytes_per_samp;

	return decoder;
}

uint32
SoundDecoder_Decode (TFB_SoundDecoder *decoder)
{
	long decoded_bytes;
	long rc;
	long buffer_size;
	uint32 max_bytes = UINT32_MAX;
	uint8 *buffer;

	if (!decoder || !decoder->funcs)
	{
		log_add (log_Warning, "SoundDecoder_Decode(): null or bad decoder");
		return 0;
	}

	buffer = (uint8*) decoder->buffer;
	buffer_size = decoder->buffer_size;
	if (!decoder->looping && decoder->end_sample > 0)
	{
		max_bytes = decoder->end_sample * decoder->bytes_per_samp;
		if (max_bytes - decoder->pos < decoder->buffer_size)
			buffer_size = max_bytes - decoder->pos;
	}

	if (buffer_size == 0)
	{	// nothing more to decode
		decoder->error = SOUNDDECODER_EOF;
		return 0;
	}

	for (decoded_bytes = 0, rc = 1; rc > 0 && decoded_bytes < buffer_size; )
	{	
		rc = decoder->funcs->Decode (decoder, buffer + decoded_bytes,
					buffer_size - decoded_bytes);
		if (rc < 0)
		{
			log_add (log_Warning, "SoundDecoder_Decode(): "
					"error decoding %s, code %ld",
					decoder->filename, rc);
		}
		else if (rc == 0)
		{	// probably EOF
			if (decoder->looping)
			{
				SoundDecoder_Rewind (decoder);
				if (decoder->error)
				{
					log_add (log_Warning, "SoundDecoder_Decode(): "
							"tried to loop %s but couldn't rewind, "
							"error code %d",
							decoder->filename, decoder->error);
				}
				else
				{
					log_add (log_Info, "SoundDecoder_Decode(): "
							"looping %s", decoder->filename);
					rc = 1;	// prime the loop again
				}
			}
			else
			{
				log_add (log_Info, "SoundDecoder_Decode(): eof for %s",
						decoder->filename);
			}
		}
		else
		{	// some bytes decoded
			decoded_bytes += rc;
		}
	}
	decoder->pos += decoded_bytes;
	if (rc < 0)
		decoder->error = SOUNDDECODER_ERROR;
	else if (rc == 0 || decoder->pos >= max_bytes)
		decoder->error = SOUNDDECODER_EOF;
	else
		decoder->error = SOUNDDECODER_OK;

	if (decoder->need_swap && decoded_bytes > 0 &&
			(decoder->format == decoder_formats.stereo16 ||
			decoder->format == decoder_formats.mono16))
	{
		SoundDecoder_SwapWords (
				decoder->buffer, decoded_bytes);
	}

	return decoded_bytes;
}

uint32
SoundDecoder_DecodeAll (TFB_SoundDecoder *decoder)
{
	uint32 decoded_bytes;
	long rc;
	uint32 reqbufsize;

	if (!decoder || !decoder->funcs)
	{
		log_add (log_Warning, "SoundDecoder_DecodeAll(): null or bad decoder");
		return 0;
	}

	reqbufsize = decoder->buffer_size;

	if (decoder->looping)
	{
		log_add (log_Warning, "SoundDecoder_DecodeAll(): "
				"called for %s with looping", decoder->filename);
		return 0;
	}

	if (reqbufsize < 4096)
		reqbufsize = 4096;

	for (decoded_bytes = 0, rc = 1; rc > 0; )
	{	
		if (decoded_bytes >= decoder->buffer_size)
		{	// need to grow buffer
			decoder->buffer_size += reqbufsize;
			decoder->buffer = HRealloc (
					decoder->buffer, decoder->buffer_size);
		}

		rc = decoder->funcs->Decode (decoder,
				(uint8*) decoder->buffer + decoded_bytes,
				decoder->buffer_size - decoded_bytes);
		
		if (rc > 0)
			decoded_bytes += rc;
	}
	decoder->buffer_size = decoded_bytes;
	decoder->pos += decoded_bytes;
	// Free up some unused memory
	decoder->buffer = HRealloc (decoder->buffer, decoded_bytes);

	if (decoder->need_swap && decoded_bytes > 0 &&
			(decoder->format == decoder_formats.stereo16 ||
			decoder->format == decoder_formats.mono16))
	{
		SoundDecoder_SwapWords (
				decoder->buffer, decoded_bytes);
	}

	if (rc < 0)
	{
		decoder->error = SOUNDDECODER_ERROR;
		log_add (log_Warning, "SoundDecoder_DecodeAll(): "
				"error decoding %s, code %ld",
				decoder->filename, rc);
		return decoded_bytes;
	}

	// switch to Buffer decoder
	decoder->funcs->Close (decoder);
	decoder->funcs->Term (decoder);

	decoder->funcs = &bufa_DecoderVtbl;
	decoder->funcs->Init (decoder);
	decoder->pos = 0;
	decoder->start_sample = 0;
	decoder->error = SOUNDDECODER_OK;

	return decoded_bytes;
}

void
SoundDecoder_Rewind (TFB_SoundDecoder *decoder)
{
	SoundDecoder_Seek (decoder, 0);
}

// seekTime is specified in mili-seconds
void
SoundDecoder_Seek (TFB_SoundDecoder *decoder, uint32 seekTime)
{
	uint32 pcm_pos;

	if (!decoder)
		return;
	if (!decoder->funcs)
	{
		log_add (log_Warning, "SoundDecoder_Seek(): bad decoder passed");
		return;
	}

	pcm_pos = (uint32) (seekTime / 1000.0f * decoder->frequency);
	pcm_pos = decoder->funcs->Seek (decoder,
			decoder->start_sample + pcm_pos);
	decoder->pos = pcm_pos * decoder->bytes_per_samp;
	decoder->error = SOUNDDECODER_OK;
}

void
SoundDecoder_Free (TFB_SoundDecoder *decoder)
{
	if (!decoder)
		return;
	if (!decoder->funcs)
	{
		log_add (log_Warning, "SoundDecoder_Free(): bad decoder passed");
		return;
	}

	decoder->funcs->Close (decoder);
	decoder->funcs->Term (decoder);

	HFree (decoder->buffer);
	HFree (decoder->filename);
	HFree (decoder);
}

float
SoundDecoder_GetTime (TFB_SoundDecoder *decoder)
{
	if (!decoder)
		return 0.0f;
	if (!decoder->funcs)
	{
		log_add (log_Warning, "SoundDecoder_GetTime(): bad decoder passed");
		return 0.0f;
	}

	return (float) 
			((decoder->pos / decoder->bytes_per_samp)
			 - decoder->start_sample
			) / decoder->frequency;
}

uint32
SoundDecoder_GetFrame (TFB_SoundDecoder *decoder)
{
	if (!decoder)
		return 0;
	if (!decoder->funcs)
	{
		log_add (log_Warning, "SoundDecoder_GetFrame(): bad decoder passed");
		return 0;
	}

	return decoder->funcs->GetFrame (decoder);
}


#define THIS_PTR TFB_SoundDecoder* This

static const char*
bufa_GetName (void)
{
	return "Buffer";
}

static bool
bufa_InitModule (int flags, const TFB_DecoderFormats* fmts)
{
	// this should never be called
	log_add (log_Debug, "bufa_InitModule(): dead function called");
	return false;
	
	(void)flags; (void)fmts; // laugh at compiler warning
}

static void
bufa_TermModule (void)
{
	// this should never be called
	log_add (log_Debug, "bufa_TermModule(): dead function called");
}

static uint32
bufa_GetStructSize (void)
{
	return sizeof (TFB_BufSoundDecoder);
}

static int
bufa_GetError (THIS_PTR)
{
	return 0; // error? what error?!

	(void)This;	// laugh at compiler warning
}

static bool
bufa_Init (THIS_PTR)
{
	TFB_BufSoundDecoder* bufa = (TFB_BufSoundDecoder*) This;
	
	This->need_swap = false;
	// hijack the buffer
	bufa->data = This->buffer;
	bufa->max_pcm = This->buffer_size / This->bytes_per_samp;
	bufa->cur_pcm = bufa->max_pcm;

	return true;
}

static void
bufa_Term (THIS_PTR)
{
	//TFB_BufSoundDecoder* bufa = (TFB_BufSoundDecoder*) This;
	bufa_Close (This); // ensure cleanup
}

static bool
bufa_Open (THIS_PTR, uio_DirHandle *dir, const char *filename)
{
	// this should never be called
	log_add (log_Debug, "bufa_Open(): dead function called");
	return false;

	// laugh at compiler warnings
	(void)This; (void)dir; (void)filename;
}

static void
bufa_Close (THIS_PTR)
{
	TFB_BufSoundDecoder* bufa = (TFB_BufSoundDecoder*) This;

	// restore the status quo
	if (bufa->data)
	{
		This->buffer = bufa->data;
		bufa->data = NULL;
	}
	bufa->cur_pcm = 0;
}

static int
bufa_Decode (THIS_PTR, void* buf, sint32 bufsize)
{
	TFB_BufSoundDecoder* bufa = (TFB_BufSoundDecoder*) This;
	uint32 dec_pcm;
	uint32 dec_bytes;

	dec_pcm = bufsize / This->bytes_per_samp;
	if (dec_pcm > bufa->max_pcm - bufa->cur_pcm)
		dec_pcm = bufa->max_pcm - bufa->cur_pcm;
	dec_bytes = dec_pcm * This->bytes_per_samp;

	// Buffer decode is a hack
	This->buffer = (uint8*) bufa->data
			+ bufa->cur_pcm * This->bytes_per_samp;

	if (dec_pcm > 0)
		bufa->cur_pcm += dec_pcm;
	
	return dec_bytes;

	(void)buf;	// laugh at compiler warning
}

static uint32
bufa_Seek (THIS_PTR, uint32 pcm_pos)
{
	TFB_BufSoundDecoder* bufa = (TFB_BufSoundDecoder*) This;

	if (pcm_pos > bufa->max_pcm)
		pcm_pos = bufa->max_pcm;
	bufa->cur_pcm = pcm_pos;

	return pcm_pos;
}

static uint32
bufa_GetFrame (THIS_PTR)
{
	return 0; // only 1 frame

	(void)This; // laugh at compiler warning
}


static const char*
nula_GetName (void)
{
	return "Null";
}

static bool
nula_InitModule (int flags, const TFB_DecoderFormats* fmts)
{
	// this should never be called
	log_add (log_Debug, "nula_InitModule(): dead function called");
	return false;
	
	(void)flags; (void)fmts; // laugh at compiler warning
}

static void
nula_TermModule (void)
{
	// this should never be called
	log_add (log_Debug, "nula_TermModule(): dead function called");
}

static uint32
nula_GetStructSize (void)
{
	return sizeof (TFB_NullSoundDecoder);
}

static int
nula_GetError (THIS_PTR)
{
	return 0; // error? what error?!

	(void)This;	// laugh at compiler warning
}

static bool
nula_Init (THIS_PTR)
{
	TFB_NullSoundDecoder* nula = (TFB_NullSoundDecoder*) This;
	
	This->need_swap = false;
	nula->cur_pcm = 0;
	return true;
}

static void
nula_Term (THIS_PTR)
{
	//TFB_NullSoundDecoder* nula = (TFB_NullSoundDecoder*) This;
	nula_Close (This); // ensure cleanup
}

static bool
nula_Open (THIS_PTR, uio_DirHandle *dir, const char *filename)
{
	This->frequency = 11025;
	This->format = decoder_formats.mono16;
	This->is_null = true;
	return true;

	// laugh at compiler warnings
	(void)dir; (void)filename;
}

static void
nula_Close (THIS_PTR)
{
	TFB_NullSoundDecoder* nula = (TFB_NullSoundDecoder*) This;

	nula->cur_pcm = 0;
}

static int
nula_Decode (THIS_PTR, void* buf, sint32 bufsize)
{
	TFB_NullSoundDecoder* nula = (TFB_NullSoundDecoder*) This;
	uint32 max_pcm;
	uint32 dec_pcm;
	uint32 dec_bytes;

	max_pcm = (uint32) (This->length * This->frequency);
	dec_pcm = bufsize / This->bytes_per_samp;
	if (dec_pcm > max_pcm - nula->cur_pcm)
		dec_pcm = max_pcm - nula->cur_pcm;
	dec_bytes = dec_pcm * This->bytes_per_samp;

	if (dec_pcm > 0)
	{
		memset (buf, 0, dec_bytes);
		nula->cur_pcm += dec_pcm;
	}
	
	return dec_bytes;
}

static uint32
nula_Seek (THIS_PTR, uint32 pcm_pos)
{
	TFB_NullSoundDecoder* nula = (TFB_NullSoundDecoder*) This;
	uint32 max_pcm;

	max_pcm = (uint32) (This->length * This->frequency);
	if (pcm_pos > max_pcm)
		pcm_pos = max_pcm;
	nula->cur_pcm = pcm_pos;

	return pcm_pos;
}

static uint32
nula_GetFrame (THIS_PTR)
{
	return 0; // only 1 frame

	(void)This; // laugh at compiler warning
}
