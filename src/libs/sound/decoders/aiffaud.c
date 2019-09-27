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

/* Portions (C) Serge van den Boom (svdb at stack.nl) */
/* Portions (C) Alex Volkov (codepro at usa.net) */

/* AIFF decoder (.aif)
 *
 * Doesn't work on *all* aiff files in general, only 8/16 PCM and
 * 16-bit AIFF-C SDX2-compressed.
 */

#include <stdio.h>
#include <stdlib.h>
		// for abs()
#include <errno.h>
#ifndef _WIN32_WCE
#	include <memory.h>
#endif
#include <string.h>
#include "port.h"
#include "types.h"
#include "libs/uio.h"
#include "endian_uqm.h"
#include "libs/log.h"
#include "aiffaud.h"

typedef uint32 aiff_ID;

#define aiff_MAKE_ID(x1, x2, x3, x4) \
		(((x1) << 24) | ((x2) << 16) | ((x3) << 8) | (x4))

#define aiff_FormID         aiff_MAKE_ID('F', 'O', 'R', 'M')
#define aiff_FormVersionID  aiff_MAKE_ID('F', 'V', 'E', 'R')
#define aiff_CommonID       aiff_MAKE_ID('C', 'O', 'M', 'M')
#define aiff_SoundDataID    aiff_MAKE_ID('S', 'S', 'N', 'D')

#define aiff_FormTypeAIFF   aiff_MAKE_ID('A', 'I', 'F', 'F')
#define aiff_FormTypeAIFC   aiff_MAKE_ID('A', 'I', 'F', 'C')

#define aiff_CompressionTypeSDX2  aiff_MAKE_ID('S', 'D', 'X', '2')


typedef struct
{
	aiff_ID id;
	uint32 size;
} aiff_ChunkHeader;

#define AIFF_CHUNK_HDR_SIZE (4+4)

typedef struct
{
	aiff_ChunkHeader chunk;
	aiff_ID type;
} aiff_FileHeader;

typedef struct
{
	uint32 version;  /* format version, in Mac format */
} aiff_FormatVersionChunk;

typedef struct
{
	uint16 channels;       /* number of channels */
	uint32 sampleFrames;   /* number of sample frames */
	uint16 sampleSize;     /* number of bits per sample */
	sint32 sampleRate;     /* number of frames per second */
			/* this is actually stored as IEEE-754 80bit in files */
} aiff_CommonChunk;

#define AIFF_COMM_SIZE (2+4+2+10)

typedef struct
{
	uint16 channels;      /* number of channels */
	uint32 sampleFrames;  /* number of sample frames */
	uint16 sampleSize;    /* number of bits per sample */
	sint32 sampleRate;    /* number of frames per second */
	aiff_ID extTypeID;    /* compression type ID */
	char extName[32];     /* compression type name */
} aiff_ExtCommonChunk;

#define AIFF_EXT_COMM_SIZE (AIFF_COMM_SIZE+4)

typedef struct
{
	uint32 offset;     /* offset to sound data */
	uint32 blockSize;  /* size of alignment blocks */
} aiff_SoundDataChunk;

#define AIFF_SSND_SIZE (4+4)
 
 
#define THIS_PTR TFB_SoundDecoder* This

static const char* aifa_GetName (void);
static bool aifa_InitModule (int flags, const TFB_DecoderFormats*);
static void aifa_TermModule (void);
static uint32 aifa_GetStructSize (void);
static int aifa_GetError (THIS_PTR);
static bool aifa_Init (THIS_PTR);
static void aifa_Term (THIS_PTR);
static bool aifa_Open (THIS_PTR, uio_DirHandle *dir, const char *filename);
static void aifa_Close (THIS_PTR);
static int aifa_Decode (THIS_PTR, void* buf, sint32 bufsize);
static uint32 aifa_Seek (THIS_PTR, uint32 pcm_pos);
static uint32 aifa_GetFrame (THIS_PTR);

TFB_SoundDecoderFuncs aifa_DecoderVtbl = 
{
	aifa_GetName,
	aifa_InitModule,
	aifa_TermModule,
	aifa_GetStructSize,
	aifa_GetError,
	aifa_Init,
	aifa_Term,
	aifa_Open,
	aifa_Close,
	aifa_Decode,
	aifa_Seek,
	aifa_GetFrame,
};


typedef enum
{
	aifc_None,
	aifc_Sdx2,
} aiff_CompressionType;

#define MAX_CHANNELS 4

typedef struct tfb_wavesounddecoder
{
	// always the first member
	TFB_SoundDecoder decoder;

	// private
	sint32 last_error;
	uio_Stream *fp;
	aiff_ExtCommonChunk fmtHdr;
	aiff_CompressionType comp_type;
	unsigned bits_per_sample;
	unsigned block_align;
	unsigned file_block;
	uint32 data_ofs;
	uint32 data_size;
	uint32 max_pcm;
	uint32 cur_pcm;
	sint32 prev_val[MAX_CHANNELS];

} TFB_AiffSoundDecoder;

static const TFB_DecoderFormats* aifa_formats = NULL;

static int aifa_DecodePCM (TFB_AiffSoundDecoder*, void* buf, sint32 bufsize);
static int aifa_DecodeSDX2 (TFB_AiffSoundDecoder*, void* buf, sint32 bufsize);


static const char*
aifa_GetName (void)
{
	return "AIFF";
}

static bool
aifa_InitModule (int flags, const TFB_DecoderFormats* fmts)
{
	aifa_formats = fmts;
	return true;
	
	(void)flags;	// laugh at compiler warning
}

static void
aifa_TermModule (void)
{
	// no specific module term
}

static uint32
aifa_GetStructSize (void)
{
	return sizeof (TFB_AiffSoundDecoder);
}

static int
aifa_GetError (THIS_PTR)
{
	TFB_AiffSoundDecoder* aifa = (TFB_AiffSoundDecoder*) This;
	int ret = aifa->last_error;
	aifa->last_error = 0;
	return ret;
}

static bool
aifa_Init (THIS_PTR)
{
	//TFB_AiffSoundDecoder* aifa = (TFB_AiffSoundDecoder*) This;
	This->need_swap = !aifa_formats->want_big_endian;
	return true;
}

static void
aifa_Term (THIS_PTR)
{
	//TFB_AiffSoundDecoder* aifa = (TFB_AiffSoundDecoder*) This;
	aifa_Close (This); // ensure cleanup
}

static bool
read_be_16 (uio_Stream *fp, uint16 *v)
{
	if (!uio_fread (v, sizeof(*v), 1, fp))
		return false;
	*v = UQM_SwapBE16 (*v);
	return true;
}

static bool
read_be_32 (uio_Stream *fp, uint32 *v)
{
	if (!uio_fread (v, sizeof(*v), 1, fp))
		return false;
	*v = UQM_SwapBE32 (*v);
	return true;
}

// Read 80-bit IEEE 754 floating point number.
// We are only interested in values that we can work with,
//   so using an sint32 here is fine.
static bool
read_be_f80 (uio_Stream *fp, sint32 *v)
{
	int sign, exp;
	int shift;
	uint16 se;
	uint32 mant, mant_low;
	if (!read_be_16 (fp, &se) ||
		!read_be_32 (fp, &mant) || !read_be_32 (fp, &mant_low))
		return false;

	sign = (se >> 15) & 1;        // sign is the highest bit
	exp = (se & ((1 << 15) - 1)); // exponent is next highest 15 bits
#if 0 // XXX: 80bit IEEE 754 used in AIFF uses explicit mantissa MS bit
	// mantissa has an implied leading bit which is typically 1
	mant >>= 1;
	if (exp != 0)
		mant |= 0x80000000;
#endif
	mant >>= 1;            // we also need space for sign
	exp -= (1 << 14) - 1;  // exponent is biased by (2^(e-1) - 1)
	shift = exp - 31 + 1;  // mantissa is already 31 bits before decimal pt.
	if (shift > 0)
		mant = 0x7fffffff; // already too big
	else if (shift < 0)
		mant >>= -shift;

	*v = sign ? -(sint32)mant : (sint32)mant;

	return true;
}

static bool
aifa_readFileHeader (TFB_AiffSoundDecoder* aifa, aiff_FileHeader* hdr)
{
	if (!read_be_32 (aifa->fp, &hdr->chunk.id) ||
			!read_be_32 (aifa->fp, &hdr->chunk.size) ||
			!read_be_32 (aifa->fp, &hdr->type))
	{
		aifa->last_error = errno;
		return false;
	}
	return true;
}

static bool
aifa_readChunkHeader (TFB_AiffSoundDecoder* aifa, aiff_ChunkHeader* hdr)
{
	if (!read_be_32 (aifa->fp, &hdr->id) ||
			!read_be_32 (aifa->fp, &hdr->size))
	{
		aifa->last_error = errno;
		return false;
	}
	return true;
}

static int
aifa_readCommonChunk (TFB_AiffSoundDecoder* aifa, uint32 size,
		aiff_ExtCommonChunk* fmt)
{
	int bytes;

	memset(fmt, 0, sizeof(*fmt));
	if (size < AIFF_COMM_SIZE)
	{
		aifa->last_error = aifae_BadFile;
		return 0;
	}

	if (!read_be_16 (aifa->fp, &fmt->channels) ||
			!read_be_32 (aifa->fp, &fmt->sampleFrames) ||
			!read_be_16 (aifa->fp, &fmt->sampleSize) ||
			!read_be_f80 (aifa->fp, &fmt->sampleRate))
	{
		aifa->last_error = errno;
		return 0;
	}
	bytes = AIFF_COMM_SIZE;

	if (size >= AIFF_EXT_COMM_SIZE)
	{
		if (!read_be_32 (aifa->fp, &fmt->extTypeID))
		{
			aifa->last_error = errno;
			return 0;
		}
		bytes += sizeof(fmt->extTypeID);
	}
	
	return bytes;
}

static bool
aifa_readSoundDataChunk (TFB_AiffSoundDecoder* aifa,
		aiff_SoundDataChunk* data)
{
	if (!read_be_32 (aifa->fp, &data->offset) ||
			!read_be_32 (aifa->fp, &data->blockSize))
	{
		aifa->last_error = errno;
		return false;
	}
	return true;
}

static bool
aifa_Open (THIS_PTR, uio_DirHandle *dir, const char *filename)
{
	TFB_AiffSoundDecoder* aifa = (TFB_AiffSoundDecoder*) This;
	aiff_FileHeader fileHdr;
	aiff_ChunkHeader chunkHdr;
	sint32 remSize;

	aifa->fp = uio_fopen (dir, filename, "rb");
	if (!aifa->fp)
	{
		aifa->last_error = errno;
		return false;
	}

	aifa->data_size = 0;
	aifa->max_pcm = 0;
	aifa->data_ofs = 0;
	memset(&aifa->fmtHdr, 0, sizeof(aifa->fmtHdr));
	memset(aifa->prev_val, 0, sizeof(aifa->prev_val));

	// read wave header
	if (!aifa_readFileHeader (aifa, &fileHdr))
	{
		aifa->last_error = errno;
		aifa_Close (This);
		return false;
	}
	if (fileHdr.chunk.id != aiff_FormID)
	{
		log_add (log_Warning, "aifa_Open(): not an aiff file, ID 0x%08x",
				fileHdr.chunk.id);
		aifa_Close (This);
		return false;
	}
	if (fileHdr.type != aiff_FormTypeAIFF && fileHdr.type != aiff_FormTypeAIFC)
	{
		log_add (log_Warning, "aifa_Open(): unsupported aiff file"
				", Type 0x%08x", fileHdr.type);
		aifa_Close (This);
		return false;
	}

	for (remSize = fileHdr.chunk.size - sizeof(aiff_ID); remSize > 0;
			remSize -= ((chunkHdr.size + 1) & ~1) + AIFF_CHUNK_HDR_SIZE)
	{
		if (!aifa_readChunkHeader (aifa, &chunkHdr))
		{
			aifa_Close (This);
			return false;
		}

		if (chunkHdr.id == aiff_CommonID)
		{
			int read = aifa_readCommonChunk (aifa, chunkHdr.size, &aifa->fmtHdr);
			if (!read)
			{
				aifa_Close (This);
				return false;
			}
			uio_fseek (aifa->fp, chunkHdr.size - read, SEEK_CUR);
		}
		else if (chunkHdr.id == aiff_SoundDataID)
		{
			aiff_SoundDataChunk data;
			if (!aifa_readSoundDataChunk (aifa, &data))
			{
				aifa_Close (This);
				return false;
			}
			aifa->data_ofs = uio_ftell (aifa->fp) + data.offset;
			uio_fseek (aifa->fp, chunkHdr.size - AIFF_SSND_SIZE, SEEK_CUR);
		}
		else
		{	// skip uninteresting chunk
			uio_fseek (aifa->fp, chunkHdr.size, SEEK_CUR);
		}

		// 2-align the file ptr
		uio_fseek (aifa->fp, chunkHdr.size & 1, SEEK_CUR);
	}

	if (aifa->fmtHdr.sampleFrames == 0)
	{
		log_add (log_Warning, "aifa_Open(): aiff file has no sound data");
		aifa_Close (This);
		return false;
	}
	
	// make bits-per-sample a multiple of 8
	aifa->bits_per_sample = (aifa->fmtHdr.sampleSize + 7) & ~7;
	if (aifa->bits_per_sample == 0 || aifa->bits_per_sample > 16)
	{	// XXX: for now we do not support 24 and 32 bps
		log_add (log_Warning, "aifa_Open(): unsupported sample size %u",
				aifa->bits_per_sample);
		aifa_Close (This);
		return false;
	}
	if (aifa->fmtHdr.channels != 1 && aifa->fmtHdr.channels != 2)
	{
		log_add (log_Warning, "aifa_Open(): unsupported number of channels %u",
				(unsigned)aifa->fmtHdr.channels);
		aifa_Close (This);
		return false;
	}
	if (aifa->fmtHdr.sampleRate < 300 || aifa->fmtHdr.sampleRate > 128000)
	{
		log_add (log_Warning, "aifa_Open(): unsupported sampling rate %ld",
				(long)aifa->fmtHdr.sampleRate);
		aifa_Close (This);
		return false;
	}

	aifa->block_align = aifa->bits_per_sample / 8 * aifa->fmtHdr.channels;
	aifa->file_block = aifa->block_align;
	if (!aifa->data_ofs)
	{
		log_add (log_Warning, "aifa_Open(): bad aiff file,"
				" no SSND chunk found");
		aifa_Close (This);
		return false;
	}

	if (fileHdr.type == aiff_FormTypeAIFF)
	{
		if (aifa->fmtHdr.extTypeID != 0)
		{
			log_add (log_Warning, "aifa_Open(): unsupported extension 0x%08x",
					aifa->fmtHdr.extTypeID);
			aifa_Close (This);
			return false;
		}
		aifa->comp_type = aifc_None;
	}
	else if (fileHdr.type == aiff_FormTypeAIFC)
	{
		if (aifa->fmtHdr.extTypeID != aiff_CompressionTypeSDX2)
		{
			log_add (log_Warning, "aifa_Open(): unsupported compression 0x%08x",
					aifa->fmtHdr.extTypeID);
			aifa_Close (This);
			return false;
		}
		aifa->comp_type = aifc_Sdx2;
		aifa->file_block /= 2;
		assert(aifa->fmtHdr.channels <= MAX_CHANNELS);
		// after decompression, we will get samples in machine byte order
		This->need_swap = (aifa_formats->big_endian
				!= aifa_formats->want_big_endian);
	}

	aifa->data_size = aifa->fmtHdr.sampleFrames * aifa->file_block;

	if (aifa->comp_type == aifc_Sdx2 && aifa->bits_per_sample != 16)
	{
		log_add (log_Warning, "aifa_Open(): unsupported sample size %u for SDX2",
				(unsigned)aifa->fmtHdr.sampleSize);
		aifa_Close (This);
		return false;
	}

	This->format = (aifa->fmtHdr.channels == 1 ?
			(aifa->bits_per_sample == 8 ?
				aifa_formats->mono8 : aifa_formats->mono16)
			:
			(aifa->bits_per_sample == 8 ?
				aifa_formats->stereo8 : aifa_formats->stereo16)
			);
	This->frequency = aifa->fmtHdr.sampleRate;

	uio_fseek (aifa->fp, aifa->data_ofs, SEEK_SET);
	aifa->max_pcm = aifa->fmtHdr.sampleFrames;
	aifa->cur_pcm = 0;
	This->length = (float) aifa->max_pcm / aifa->fmtHdr.sampleRate;
	aifa->last_error = 0;

	return true;
}

static void
aifa_Close (THIS_PTR)
{
	TFB_AiffSoundDecoder* aifa = (TFB_AiffSoundDecoder*) This;

	if (aifa->fp)
	{
		uio_fclose (aifa->fp);
		aifa->fp = NULL;
	}
}

static int
aifa_Decode (THIS_PTR, void* buf, sint32 bufsize)
{
	TFB_AiffSoundDecoder* aifa = (TFB_AiffSoundDecoder*) This;
	switch (aifa->comp_type)
	{
	case aifc_None:
		return aifa_DecodePCM (aifa, buf, bufsize);
	case aifc_Sdx2:
		return aifa_DecodeSDX2 (aifa, buf, bufsize);
	default:
		assert(false && "Unknown comp_type");
		return 0;
	}
}

static int
aifa_DecodePCM (TFB_AiffSoundDecoder* aifa, void* buf, sint32 bufsize)
{
	uint32 dec_pcm;
	uint32 size;

	dec_pcm = bufsize / aifa->block_align;
	if (dec_pcm > aifa->max_pcm - aifa->cur_pcm)
		dec_pcm = aifa->max_pcm - aifa->cur_pcm;

	dec_pcm = uio_fread (buf, aifa->file_block, dec_pcm, aifa->fp);
	aifa->cur_pcm += dec_pcm;
	size = dec_pcm * aifa->block_align;

	if (aifa->bits_per_sample == 8)
	{	// AIFF files store 8-bit data as signed
		// and we need it unsigned
		uint8* ptr = (uint8*)buf;
		uint32 left;
		for (left = size; left > 0; --left, ++ptr)
			*ptr += 128;
	}
	
	return size;
}

static int
aifa_DecodeSDX2 (TFB_AiffSoundDecoder* aifa, void* buf, sint32 bufsize)
{
	uint32 dec_pcm;
	sint8 *src;
	sint16 *dst = buf;
	uint32 left;

	dec_pcm = bufsize / aifa->block_align;
	if (dec_pcm > aifa->max_pcm - aifa->cur_pcm)
		dec_pcm = aifa->max_pcm - aifa->cur_pcm;

	src = (sint8*)buf + bufsize - (dec_pcm * aifa->file_block);
	dec_pcm = uio_fread (src, aifa->file_block, dec_pcm, aifa->fp);
	aifa->cur_pcm += dec_pcm;

	for (left = dec_pcm; left > 0; --left)
	{
		int i;
		sint32 *prev = aifa->prev_val;
		for (i = aifa->fmtHdr.channels; i > 0; --i, ++prev, ++src, ++dst)
		{
			sint32 v = (*src * abs(*src)) << 1;
			if (*src & 1)
				v += *prev;
			// saturate the value
			if (v > 32767)
				v = 32767;
			else if (v < -32768)
				v = -32768;
			*prev = v;
			*dst = v;
		}
	}

	return dec_pcm * aifa->block_align;
}

static uint32
aifa_Seek (THIS_PTR, uint32 pcm_pos)
{
	TFB_AiffSoundDecoder* aifa = (TFB_AiffSoundDecoder*) This;

	if (pcm_pos > aifa->max_pcm)
		pcm_pos = aifa->max_pcm;
	aifa->cur_pcm = pcm_pos;
	uio_fseek (aifa->fp,
			aifa->data_ofs + pcm_pos * aifa->file_block,
			SEEK_SET);

	// reset previous values for SDX2 on seek ops
	// the delta will recover faster with reset
	memset(aifa->prev_val, 0, sizeof(aifa->prev_val));

	return pcm_pos;
}

static uint32
aifa_GetFrame (THIS_PTR)
{
	//TFB_AiffSoundDecoder* aifa = (TFB_AiffSoundDecoder*) This;
	return 0; // only 1 frame for now

	(void)This;	// laugh at compiler warning
}
