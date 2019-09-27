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

/* .duk sound track decoder
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "libs/memlib.h"
#include "port.h"
#include "types.h"
#include "libs/uio.h"
#include "dukaud.h"
#include "decoder.h"
#include "endian_uqm.h"

#define DATA_BUF_SIZE	  0x8000
#define DUCK_GENERAL_FPS  14.622f


#define THIS_PTR TFB_SoundDecoder* This

static const char* duka_GetName (void);
static bool duka_InitModule (int flags, const TFB_DecoderFormats*);
static void duka_TermModule (void);
static uint32 duka_GetStructSize (void);
static int duka_GetError (THIS_PTR);
static bool duka_Init (THIS_PTR);
static void duka_Term (THIS_PTR);
static bool duka_Open (THIS_PTR, uio_DirHandle *dir, const char *filename);
static void duka_Close (THIS_PTR);
static int duka_Decode (THIS_PTR, void* buf, sint32 bufsize);
static uint32 duka_Seek (THIS_PTR, uint32 pcm_pos);
static uint32 duka_GetFrame (THIS_PTR);

TFB_SoundDecoderFuncs duka_DecoderVtbl = 
{
	duka_GetName,
	duka_InitModule,
	duka_TermModule,
	duka_GetStructSize,
	duka_GetError,
	duka_Init,
	duka_Term,
	duka_Open,
	duka_Close,
	duka_Decode,
	duka_Seek,
	duka_GetFrame,
};

typedef struct tfb_ducksounddecoder
{
	// always the first member
	TFB_SoundDecoder decoder;

	// public read-only
	uint32 iframe;    // current frame index
	uint32 cframes;   // total count of frames
	uint32 channels;  // number of channels
	uint32 pcm_frame; // samples per frame

	// private
	sint32 last_error;
	uio_Stream* duk;
	uint32* frames;
	// buffer
	void* data;
	uint32 maxdata;
	uint32 cbdata;
	uint32 dataofs;
	// decoder stuff
	sint32 predictors[2];

} TFB_DuckSoundDecoder;


typedef struct
{
	uint32 audsize;
	uint32 vidsize;
} DukAud_FrameHeader;

typedef struct
{
	uint16 magic;  // always 0xf77f
	uint16 numsamples;
	uint16 tag;
	uint16 indices[2];  // initial indices for channels
} DukAud_AudSubframe;

static const TFB_DecoderFormats* duka_formats = NULL;

static sint32
duka_readAudFrameHeader (TFB_DuckSoundDecoder* duka, uint32 iframe,
		DukAud_AudSubframe* aud)
{
	DukAud_FrameHeader hdr;

	uio_fseek (duka->duk, duka->frames[iframe], SEEK_SET);
	if (uio_fread (&hdr, sizeof(hdr), 1, duka->duk) != 1)
	{
		duka->last_error = errno;
		return dukae_BadFile;
	}
	hdr.audsize = UQM_SwapBE32 (hdr.audsize);

	if (uio_fread (aud, sizeof(*aud), 1, duka->duk) != 1)
	{
		duka->last_error = errno;
		return dukae_BadFile;
	}

	aud->magic = UQM_SwapBE16 (aud->magic);
	if (aud->magic != 0xf77f)
		return duka->last_error = dukae_BadFile;
	
	aud->numsamples = UQM_SwapBE16 (aud->numsamples);
	aud->tag = UQM_SwapBE16 (aud->tag);
	aud->indices[0] = UQM_SwapBE16 (aud->indices[0]);
	aud->indices[1] = UQM_SwapBE16 (aud->indices[1]);

	return 0;
}

// This table is from one of the files that came with the original 3do source
// It's slightly different from the data used by MPlayer.
static int adpcm_step[89] = {
		0x7, 0x8, 0x9, 0xA, 0xB, 0xC, 0xD, 0xF,
		0x10, 0x12, 0x13, 0x15, 0x17, 0x1A, 0x1C, 0x1F,
		0x22, 0x26, 0x29, 0x2E, 0x32, 0x37, 0x3D, 0x43,
		0x4A, 0x51, 0x59, 0x62, 0x6C, 0x76, 0x82, 0x8F,
		0x9E, 0xAD, 0xBF, 0xD2, 0xE7, 0xFE, 0x117, 0x133,
		0x152, 0x174, 0x199, 0x1C2, 0x1EF, 0x220, 0x256, 0x292,
		0x2D4, 0x31D, 0x36C, 0x3C4, 0x424, 0x48E, 0x503, 0x583,
		0x610, 0x6AC, 0x756, 0x812, 0x8E1, 0x9C4, 0xABE, 0xBD1,
		0xCFF, 0xE4C, 0xFBA, 0x114D, 0x1308, 0x14EF, 0x1707, 0x1954,
		0x1BDD, 0x1EA6, 0x21B7, 0x2516,
		0x28CB, 0x2CDF, 0x315C, 0x364C,
		0x3BBA, 0x41B2, 0x4844, 0x4F7E,
		0x5771, 0x6030, 0x69CE, 0x7463,
		0x7FFF
		};


// *** BEGIN part copied from MPlayer ***
// (some little changes)

#if 0
// pertinent tables for IMA ADPCM
static int adpcm_step[89] = {
		7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
		19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
		50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
		130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
		337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
		876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
		2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
		5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
		};
#endif

static int adpcm_index[16] = {
		-1, -1, -1, -1, 2, 4, 6, 8,
		-1, -1, -1, -1, 2, 4, 6, 8
		};

// clamp a number between 0 and 88
#define CLAMP_0_TO_88(x) \
		if ((x) < 0) (x) = 0; else if ((x) > 88) (x) = 88;

// clamp a number within a signed 16-bit range
#define CLAMP_S16(x) \
		if ((x) < -32768) \
			(x) = -32768; \
		else if ((x) > 32767) \
			(x) = 32767;

static void
decode_nibbles (sint16 *output, sint32 output_size, sint32 channels,
		sint32* predictors, uint16* indices)
{
	sint32 step[2];
	sint32 index[2];
	sint32 diff;
	sint32 i;
	int sign;
	sint32 delta;
	int channel_number = 0;

	channels -= 1;
	index[0] = indices[0];
	index[1] = indices[1];
	step[0] = adpcm_step[index[0]];
	step[1] = adpcm_step[index[1]];

	for (i = 0; i < output_size; i++)
	{
		delta = output[i];

		index[channel_number] += adpcm_index[delta];
		CLAMP_0_TO_88(index[channel_number]);

		sign = delta & 8;
		delta = delta & 7;

#if 0
		// fast approximation, used in most decoders
		diff = step[channel_number] >> 3;
		if (delta & 4) diff += step[channel_number];
		if (delta & 2) diff += step[channel_number] >> 1;
		if (delta & 1) diff += step[channel_number] >> 2;
#else
		// real thing
//		diff = ((signed)delta + 0.5) * step[channel_number] / 4;
		diff = (((delta << 1) + 1) * step[channel_number]) >> 3;
#endif

		if (sign)
			predictors[channel_number] -= diff;
		else
			predictors[channel_number] += diff;

		CLAMP_S16(predictors[channel_number]);
		output[i] = predictors[channel_number];
		step[channel_number] = adpcm_step[index[channel_number]];

		// toggle channel
		channel_number ^= channels;
	}
}
// *** END part copied from MPlayer ***

static sint32
duka_decodeFrame (TFB_DuckSoundDecoder* duka, DukAud_AudSubframe* header,
		uint8* input)
{
	uint8* inend;
	sint16* output;
	sint16* outptr;
	sint32 outputsize;

	outputsize = header->numsamples * 2 * sizeof (sint16);
	outptr = output = (sint16*) ((uint8*)duka->data + duka->cbdata);
	
	for (inend = input + header->numsamples; input < inend; ++input)
	{
		*(outptr++) = *input >> 4;
		*(outptr++) = *input & 0x0f;
	}
	
	decode_nibbles (output, header->numsamples * 2, duka->channels,
			duka->predictors, header->indices);

	duka->cbdata += outputsize;

	return outputsize;
}


static sint32
duka_readNextFrame (TFB_DuckSoundDecoder* duka)
{
	DukAud_FrameHeader hdr;
	DukAud_AudSubframe* aud;
	uint8* p;

	uio_fseek (duka->duk, duka->frames[duka->iframe], SEEK_SET);
	if (uio_fread (&hdr, sizeof(hdr), 1, duka->duk) != 1)
	{
		duka->last_error = errno;
		return dukae_BadFile;
	}
	hdr.audsize = UQM_SwapBE32 (hdr.audsize);

	// dump encoded data at the end of the buffer aligned on 8-byte
	p = ((uint8*)duka->data + duka->maxdata - ((hdr.audsize + 7) & (-8)));
	if (uio_fread (p, 1, hdr.audsize, duka->duk) != hdr.audsize)
	{
		duka->last_error = errno;
		return dukae_BadFile;
	}
	aud = (DukAud_AudSubframe*) p;
	p += sizeof(DukAud_AudSubframe);

	aud->magic = UQM_SwapBE16 (aud->magic);
	if (aud->magic != 0xf77f)
		return duka->last_error = dukae_BadFile;
	
	aud->numsamples = UQM_SwapBE16 (aud->numsamples);
	aud->tag = UQM_SwapBE16 (aud->tag);
	aud->indices[0] = UQM_SwapBE16 (aud->indices[0]);
	aud->indices[1] = UQM_SwapBE16 (aud->indices[1]);

	duka->iframe++;

	return duka_decodeFrame (duka, aud, p);
}

static sint32
duka_stuffBuffer (TFB_DuckSoundDecoder* duka, void* buf, sint32 bufsize)
{
	sint32 dataleft;

	dataleft = duka->cbdata - duka->dataofs;
	if (dataleft > 0)
	{
		if (dataleft > bufsize)
			dataleft = bufsize & (-4);
		memcpy (buf, (uint8*)duka->data + duka->dataofs, dataleft);
		duka->dataofs += dataleft;
	}

	if (duka->cbdata > 0 && duka->dataofs >= duka->cbdata)
		duka->cbdata = duka->dataofs = 0; // reset for new data

	return dataleft;
}


static const char*
duka_GetName (void)
{
	return "DukAud";
}

static bool
duka_InitModule (int flags, const TFB_DecoderFormats* fmts)
{
	duka_formats = fmts;
	return true;

	(void)flags;	// laugh at compiler warning
}

static void
duka_TermModule (void)
{
	// no specific module term
}

static uint32
duka_GetStructSize (void)
{
	return sizeof (TFB_DuckSoundDecoder);
}

static int
duka_GetError (THIS_PTR)
{
	TFB_DuckSoundDecoder* duka = (TFB_DuckSoundDecoder*) This;
	int ret = duka->last_error;
	duka->last_error = dukae_None;
	return ret;
}

static bool
duka_Init (THIS_PTR)
{
	//TFB_DuckSoundDecoder* duka = (TFB_DuckSoundDecoder*) This;
	This->need_swap =
			duka_formats->big_endian != duka_formats->want_big_endian;
	return true;
}

static void
duka_Term (THIS_PTR)
{
	//TFB_DuckSoundDecoder* duka = (TFB_DuckSoundDecoder*) This;
	duka_Close (This); // ensure cleanup
}

static bool
duka_Open (THIS_PTR, uio_DirHandle *dir, const char *file)
{
	TFB_DuckSoundDecoder* duka = (TFB_DuckSoundDecoder*) This;
	uio_Stream* duk;
	uio_Stream* frm;
	DukAud_AudSubframe aud;
	char filename[256];
	uint32 filelen;
	size_t cread;
	uint32 i;

	filelen = strlen (file);
	if (filelen > sizeof (filename) - 1)
		return false;
	strcpy (filename, file);

	duk = uio_fopen (dir, filename, "rb");
	if (!duk)
	{
		duka->last_error = errno;
		return false;
	}

	strcpy (filename + filelen - 3, "frm");
	frm = uio_fopen (dir, filename, "rb");
	if (!frm)
	{
		duka->last_error = errno;
		uio_fclose (duk);
		return false;
	}

	duka->duk = duk;

	uio_fseek (frm, 0, SEEK_END);
	duka->cframes = uio_ftell (frm) / sizeof (uint32);
	uio_fseek (frm, 0, SEEK_SET);
	if (!duka->cframes)
	{
		duka->last_error = dukae_BadFile;
		uio_fclose (frm);
		duka_Close (This);
		return false;
	}
	
	duka->frames = (uint32*) HMalloc (duka->cframes * sizeof (uint32));
	cread = uio_fread (duka->frames, sizeof (uint32), duka->cframes, frm);
	uio_fclose (frm);
	if (cread != duka->cframes)
	{
		duka->last_error = dukae_BadFile;
		duka_Close (This);
		return false;
	}

	for (i = 0; i < duka->cframes; ++i)
		duka->frames[i] = UQM_SwapBE32 (duka->frames[i]);

	if (duka_readAudFrameHeader (duka, 0, &aud) < 0)
	{
		duka_Close (This);
		return false;
	}

	This->frequency = 22050;
	This->format = duka_formats->stereo16;
	duka->channels = 2;
	duka->pcm_frame = aud.numsamples;
	duka->data = HMalloc (DATA_BUF_SIZE);
	duka->maxdata = DATA_BUF_SIZE;

	// estimate
	This->length = (float) duka->cframes / DUCK_GENERAL_FPS;

	duka->last_error = 0;

	return true;
}

static void
duka_Close (THIS_PTR)
{
	TFB_DuckSoundDecoder* duka = (TFB_DuckSoundDecoder*) This;

	if (duka->data)
	{
		HFree (duka->data);
		duka->data = NULL;
	}
	if (duka->frames)
	{
		HFree (duka->frames);
		duka->frames = NULL;
	}
	if (duka->duk)
	{
		uio_fclose (duka->duk);
		duka->duk = NULL;
	}
	duka->last_error = 0;
}

static int
duka_Decode (THIS_PTR, void* buf, sint32 bufsize)
{
	TFB_DuckSoundDecoder* duka = (TFB_DuckSoundDecoder*) This;
	sint32 stuffed;
	sint32 total = 0;

	if (bufsize <= 0)
		return duka->last_error = dukae_BadArg;

	do
	{
		stuffed = duka_stuffBuffer (duka, buf, bufsize);
		buf = (uint8*)buf + stuffed;
		bufsize -= stuffed;
		total += stuffed;
	
		if (bufsize > 0 && duka->iframe < duka->cframes)
		{
			stuffed = duka_readNextFrame (duka);
			if (stuffed <= 0)
				return stuffed;
		}
	} while (bufsize > 0 && duka->iframe < duka->cframes);

	return total;
}

static uint32
duka_Seek (THIS_PTR, uint32 pcm_pos)
{
	TFB_DuckSoundDecoder* duka = (TFB_DuckSoundDecoder*) This;
	uint32 iframe;

	iframe = pcm_pos / duka->pcm_frame;
	if (iframe < duka->cframes)
	{
		duka->iframe = iframe;
		duka->cbdata = 0;
		duka->dataofs = 0;
		duka->predictors[0] = 0;
		duka->predictors[1] = 0;
	}
	return duka->iframe * duka->pcm_frame;
}

static uint32
duka_GetFrame (THIS_PTR)
{
	TFB_DuckSoundDecoder* duka = (TFB_DuckSoundDecoder*) This;

	// if there is nothing buffered return the actual current frame
	//  otherwise return previous
	return duka->dataofs == duka->cbdata ?
			duka->iframe : duka->iframe - 1;
}
