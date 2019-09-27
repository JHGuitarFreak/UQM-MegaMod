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

/* DUCK video player
 *
 * Status: fully functional
 */

#include "video.h"
#include "dukvid.h"
#include <stdio.h>
#include <string.h>
#include "libs/uio.h"
#include "libs/memlib.h"
#include "endian_uqm.h"
#include "uqm/units.h"

#define THIS_PTR    TFB_VideoDecoder* This

static const char* dukv_GetName (void);
static bool dukv_InitModule (int flags);
static void dukv_TermModule (void);
static uint32 dukv_GetStructSize (void);
static int dukv_GetError (THIS_PTR);
static bool dukv_Init (THIS_PTR, TFB_PixelFormat* fmt);
static void dukv_Term (THIS_PTR);
static bool dukv_Open (THIS_PTR, uio_DirHandle *dir, const char *filename);
static void dukv_Close (THIS_PTR);
static int dukv_DecodeNext (THIS_PTR);
static uint32 dukv_SeekFrame (THIS_PTR, uint32 frame);
static float dukv_SeekTime (THIS_PTR, float time);
static uint32 dukv_GetFrame (THIS_PTR);
static float dukv_GetTime (THIS_PTR);

TFB_VideoDecoderFuncs dukv_DecoderVtbl = 
{
	dukv_GetName,
	dukv_InitModule,
	dukv_TermModule,
	dukv_GetStructSize,
	dukv_GetError,
	dukv_Init,
	dukv_Term,
	dukv_Open,
	dukv_Close,
	dukv_DecodeNext,
	dukv_SeekFrame,
	dukv_SeekTime,
	dukv_GetFrame,
	dukv_GetTime,
};

typedef struct tfb_duckvideoheader
{
	uint32 version;
	uint32 scrn_x_ofs;   // horz screen offset in pixels
	uint32 scrn_y_ofs;   // vert screen offset in pixels 
	uint16 wb, hb;       // width + height in blocks
	sint16 lumas[8];     // future luminance deltas
	sint16 chromas[8];   // future chrominance deltas

} TFB_DuckVideoHeader;

#define NUM_VEC_ITEMS	0x010
#define NUM_VECTORS		0x100

typedef struct tfb_duckvideodeltas
{
	sint32 lumas[NUM_VECTORS][NUM_VEC_ITEMS];
	sint32 chromas[NUM_VECTORS][NUM_VEC_ITEMS];

} TFB_DuckVideoDeltas;

// specific video decoder struct derived from TFB_VideoDecoder
// the only sane way in C one can :)
typedef struct tfb_duckvideodecoder
{
	// always the first member
	TFB_VideoDecoder decoder;

	sint32 last_error;
	uio_DirHandle* basedir;
	char* basename;
	uio_Stream *stream;

// loaded from disk
	uint32* frames;
	uint32 cframes;
	uint32 iframe;
	uint32 version;
	uint32 wb, hb;     // width, height in blocks

// generated
	TFB_DuckVideoDeltas d;

	uint8* inbuf;
	uint32* decbuf;

} TFB_DuckVideoDecoder;

#define DUCK_GENERAL_FPS     14.622f
#define DUCK_MAX_FRAME_SIZE  0x8000U
#define DUCK_END_OF_SEQUENCE 1

static void
dukv_DecodeFrame (uint8* src_p, uint32* dst_p, uint32 wb, uint32 hb,
		TFB_DuckVideoDeltas* deltas)
{
	int iVec;
	int iSeq;
	uint32 x, y;
	sint32 w;
	uint32 *d_p0, *d_p1;
	int i;

	w = wb * 4;

	iVec = *(src_p++);
	iSeq = 0;

	for (y = 0; y < hb; ++y)
	{
		sint32 accum0, accum1, corr, corr0, corr1, delta;
		sint32 pix[4];

		d_p0 = dst_p + y * w * 2;
		d_p1 = d_p0 + w;

		accum0 = 0;
		accum1 = 0;
		corr0 = 0;
		corr1 = 0;

		for (x = 0; x < wb; ++x)
		{
			if (y == 0)
			{
				pix[0] = pix[1] = pix[2] = pix[3] = 0;
			}
			else
			{
				uint32* p_p = d_p0 - w;
				pix[0] = p_p[0];
				pix[1] = p_p[1];
				pix[2] = p_p[2];
				pix[3] = p_p[3];
			}

			// start with chroma delta
			delta = deltas->chromas[iVec][iSeq++];
			iSeq++; // correctors ignored

			accum0 += delta >> 1;
			if (delta & 1)
			{
				iVec = *(src_p++);
				iSeq = 0;
			}

			// line 0
			for (i = 0; i < 4; ++i, ++d_p0)
			{
				delta = deltas->lumas[iVec][iSeq++];
				corr = deltas->lumas[iVec][iSeq++];

				accum0 += delta >> 1;
				corr0 ^= corr;
				pix[i] += accum0;
				pix[i] ^= corr0;

				if (delta & 1)
				{
					iVec = *(src_p++);
					iSeq = 0;
				}
				
				*d_p0 = pix[i];
			}

			// line 1
			for (i = 0; i < 4; ++i, ++d_p1)
			{
				delta = deltas->lumas[iVec][iSeq++];
				corr = deltas->lumas[iVec][iSeq++];

				accum1 += delta >> 1;
				corr1 ^= corr;
				pix[i] += accum1;
				pix[i] ^= corr1;

				if (delta & 1)
				{
					iVec = *(src_p++);
					iSeq = 0;
				}
				
				*d_p1 = pix[i];
			}
		}
	}
}

static void
dukv_DecodeFrameV3 (uint8* src_p, uint32* dst_p, uint32 wb, uint32 hb,
		TFB_DuckVideoDeltas* deltas)
{
	int iVec;
	int iSeq;
	uint32 x, y;
	sint32 w;
	uint32* d_p;
	int i;

	iVec = *(src_p++);
	iSeq = 0;

	hb *= 2;
	w = wb * 4;

	for (y = 0; y < hb; ++y)
	{
		sint32 accum, delta, pix;

		d_p = dst_p + y * w;

		accum = 0;

		for (x = 0; x < wb; ++x)
		{
			// start with chroma delta
			delta = deltas->chromas[iVec][iSeq];
			iSeq += 2; // correctors ignored

			accum += delta >> 1;

			if (delta & DUCK_END_OF_SEQUENCE)
			{
				iVec = *(src_p++);
				iSeq = 0;
			}

			for (i = 0; i < 4; ++i, ++d_p)
			{
				if (y == 0)
					pix = 0;
				else
					pix = d_p[-w];

				// get next luma delta
				delta = deltas->lumas[iVec][iSeq];
				iSeq += 2; // correctors ignored

				accum += delta >> 1;
				pix += accum;

				if (delta & DUCK_END_OF_SEQUENCE)
				{
					iVec = *(src_p++);
					iSeq = 0;
				}
				
				*d_p = pix;
			}
		}
	}
}

static bool
dukv_OpenStream (TFB_DuckVideoDecoder* dukv)
{
	char filename[280];

	strcat (strcpy (filename, dukv->basename), ".duk");

	return (dukv->stream =
			uio_fopen (dukv->basedir, filename, "rb")) != NULL;
}

static bool
dukv_ReadFrames (TFB_DuckVideoDecoder* dukv)
{
	char filename[280];
	uint32 i;
	uio_Stream *fp;

	strcat (strcpy (filename, dukv->basename), ".frm");

	if (!(fp = uio_fopen (dukv->basedir, filename, "rb")))
		return false;

	// get number of frames
	uio_fseek (fp, 0, SEEK_END);
	dukv->cframes = uio_ftell (fp) / sizeof (uint32);
	uio_fseek (fp, 0, SEEK_SET);
	dukv->frames = (uint32*) HMalloc (dukv->cframes * sizeof (uint32));

	if (uio_fread (dukv->frames, sizeof (uint32), dukv->cframes,
			fp) != dukv->cframes)
	{
		HFree (dukv->frames);
		dukv->frames = 0;
		return 0;
	}
	uio_fclose (fp);

	for (i = 0; i < dukv->cframes; ++i)
		dukv->frames[i] = UQM_SwapBE32 (dukv->frames[i]);

	return true;
}

static bool
dukv_ReadVectors (TFB_DuckVideoDecoder* dukv, uint8* vectors)
{
	uio_Stream *fp;
	char filename[280];
	int ret;

	strcat (strcpy (filename, dukv->basename), ".tbl");

	if (!(fp = uio_fopen (dukv->basedir, filename, "rb")))
		return false;
	
	ret = uio_fread (vectors, NUM_VEC_ITEMS, NUM_VECTORS, fp);
	uio_fclose (fp);

	return ret == NUM_VECTORS;
}

static bool
dukv_ReadHeader (TFB_DuckVideoDecoder* dukv, sint32* pl, sint32* pc)
{
	uio_Stream *fp;
	char filename[280];
	int ret;
	int i;
	TFB_DuckVideoHeader hdr;

	strcat (strcpy (filename, dukv->basename), ".hdr");

	if (!(fp = uio_fopen (dukv->basedir, filename, "rb")))
		return false;
	
	ret = uio_fread (&hdr, sizeof (hdr), 1, fp);
	uio_fclose (fp);
	if (!ret)
		return false;

	dukv->version = UQM_SwapBE32 (hdr.version);
	dukv->wb = UQM_SwapBE16 (hdr.wb);
	dukv->hb = UQM_SwapBE16 (hdr.hb);

	for (i = 0; i < 8; ++i)
	{
		pl[i] = (sint16) UQM_SwapBE16 (hdr.lumas[i]);
		pc[i] = (sint16) UQM_SwapBE16 (hdr.chromas[i]);
	}

	dukv->decoder.w = dukv->wb * 4;
	dukv->decoder.h = dukv->hb * 4;

	return true;
}

static sint32
dukv_make_delta (sint32* protos, bool is_chroma, int i1, int i2)
{
	sint32 d1, d2;

	if (!is_chroma)
	{
		// 0x421 is (r,g,b)=(1,1,1) in 15bit pixel coding
		d1 = (protos[i1] >> 1) * 0x421;
		d2 = (protos[i2] >> 1) * 0x421;
		return ((d1 << 16) + d2) << 1;
	}
	else
	{
		d1 = (protos[i1] << 10) + protos[i2];
		return ((d1 << 16) + d1) << 1;
	}
}

static sint32
dukv_make_corr (sint32* protos, bool is_chroma, int i1, int i2)
{
	sint32 d1, d2;

	if (!is_chroma)
	{
		d1 = (protos[i1] & 1) << 15;
		d2 = (protos[i2] & 1) << 15;
		return (d1 << 16) + d2;
	}
	else
	{
		return (i1 << 3) + i2;
	}
}

static void
dukv_DecodeVector (uint8* vec, sint32* p, bool is_chroma, sint32* deltas)
{
	int citems = vec[0];
	int i;

	for (i = 0; i < citems; i += 2, vec += 2, deltas += 2)
	{
		sint32 d = dukv_make_delta (p, is_chroma, vec[1], vec[2]);

		if (i == citems - 2)
			d |= DUCK_END_OF_SEQUENCE;

		deltas[0] = d;
		deltas[1] = dukv_make_corr (p, is_chroma, vec[1], vec[2]);
	}
	
}

static void
dukv_InitDeltas (TFB_DuckVideoDecoder* dukv, uint8* vectors,
		sint32* pl, sint32* pc)
{
	int i;

	for (i = 0; i < NUM_VECTORS; ++i)
	{
		uint8* vector = vectors + i * NUM_VEC_ITEMS;
		dukv_DecodeVector (vector, pl, false, dukv->d.lumas[i]);
		dukv_DecodeVector (vector, pc, true, dukv->d.chromas[i]);
	}
}

static inline uint32
dukv_PixelConv (uint16 pix, const TFB_PixelFormat* fmt)
{
	uint32 r, g, b;

	r = (pix >> 7) & 0xf8;
	g = (pix >> 2) & 0xf8;
	b = (pix << 3) & 0xf8;

	return
		((r >> fmt->Rloss) << fmt->Rshift) |
		((g >> fmt->Gloss) << fmt->Gshift) |
		((b >> fmt->Bloss) << fmt->Bshift);
}

static void
dukv_RenderFrame (THIS_PTR)
{
	TFB_DuckVideoDecoder* dukv = (TFB_DuckVideoDecoder*) This;
	const TFB_PixelFormat* fmt = This->format;
	uint32 h, x, y, pair;
	uint32* dec = dukv->decbuf;
	uint32 bufInc = 0;
	int scale = 4;

	h = dukv->decoder.h / 2;

	// separate bpp versions for speed
	switch (fmt->BytesPerPixel) {
		case 2:
			for (y = 0; y < h; ++y) {
				uint16 *dst0, *dst1;

				dst0 = (uint16*) This->callbacks.GetCanvasLine (This, y * 2 + IF_HD(1));
				dst1 = (uint16*) This->callbacks.GetCanvasLine (This, y * 2 + 1);

				if (IS_HD && y % scale != 0)
					dec -= dukv->decoder.w;

				for (x = 0; x < dukv->decoder.w; ++x, RES_BOOL(++dec, ++bufInc), ++dst0, ++dst1) {

					if (bufInc % scale == 0 && IS_HD)
						dec++;

					pair = *dec;
					*dst0 = dukv_PixelConv ((uint16)(pair >> 16), fmt);
					*dst1 = dukv_PixelConv ((uint16)(pair & 0xffff), fmt);
				} 
			}
			break;
		case 3:
			for (y = 0; y < h; ++y) {
				uint8 *dst0, *dst1;

				dst0 = (uint8*) This->callbacks.GetCanvasLine (This, y * 2 + IF_HD(1));
				dst1 = (uint8*) This->callbacks.GetCanvasLine (This, y * 2 + 1);

				if (IS_HD && y % scale != 0)
					dec -= dukv->decoder.w;

				for (x = 0; x < dukv->decoder.w; ++x, RES_BOOL(++dec, ++bufInc), dst0 += 3, dst1 += 3) {
					if (bufInc % scale == 0 && IS_HD)
						dec++;

					pair = *dec;
					*(uint32*)dst0 =
							dukv_PixelConv ((uint16)(pair >> 16), fmt);
					*(uint32*)dst1 =
							dukv_PixelConv ((uint16)(pair & 0xffff), fmt);
				}
			}
			break;
		case 4:
			for (y = 0; y < h; ++y) {
				uint32 *dst0, *dst1;

				dst0 = (uint32*) This->callbacks.GetCanvasLine (This, y * 2 + IF_HD(0));
				dst1 = (uint32*) This->callbacks.GetCanvasLine (This, y * 2 + 1);

				if (IS_HD && y % scale != 0)
					dec -= RES_DESCALE(dukv->decoder.w);

				for (x = 0; x < dukv->decoder.w; ++x, RES_BOOL(++dec, ++bufInc), ++dst0, ++dst1) {
					if (bufInc % scale == 0 && IS_HD)
						dec++;

					pair = *dec;
					*dst0 = dukv_PixelConv ((uint16)(pair >> 16), fmt);
					*dst1 = dukv_PixelConv ((uint16)(pair & 0xffff), fmt);
				}
			}
			break;
		default:
			break;
	}
}

static const char*
dukv_GetName (void)
{
	return "DukVid";
}

static bool
dukv_InitModule (int flags)
{
	// no flags are defined for now
	return true;
	
	(void)flags; // dodge compiler warning
}

static void
dukv_TermModule (void)
{
	// do an extensive search on the word 'nothing'
}

static uint32
dukv_GetStructSize (void)
{
	return sizeof (TFB_DuckVideoDecoder);
}

static int
dukv_GetError (THIS_PTR)
{
	return This->error;
}

static bool
dukv_Init (THIS_PTR, TFB_PixelFormat* fmt)
{
	This->format = fmt;
	This->audio_synced = true;
	return true;
}

static void
dukv_Term (THIS_PTR)
{
	//TFB_DuckVideoDecoder* dukv = (TFB_DuckVideoDecoder*) This;

	dukv_Close (This);
}

static bool
dukv_Open (THIS_PTR, uio_DirHandle *dir, const char *filename)
{
	TFB_DuckVideoDecoder* dukv = (TFB_DuckVideoDecoder*) This;
	char* pext;
	sint32 lumas[8], chromas[8];
	uint8* vectors;
	
	dukv->basedir = dir;
	dukv->basename = HMalloc (strlen (filename) + 1);
	strcpy (dukv->basename, filename);
	pext = strrchr (dukv->basename, '.');
	if (pext) // strip extension
		*pext = 0;

	vectors = HMalloc (NUM_VEC_ITEMS * NUM_VECTORS);

	if (!dukv_OpenStream (dukv)
			|| !dukv_ReadFrames (dukv)
			|| !dukv_ReadHeader (dukv, lumas, chromas)
			|| !dukv_ReadVectors (dukv, vectors))
	{
		HFree (vectors);
		dukv_Close (This);
		dukv->last_error = dukve_BadFile;
		return false;
	}

	dukv_InitDeltas (dukv, vectors, lumas, chromas);
	HFree (vectors);

	This->length = (float) dukv->cframes / DUCK_GENERAL_FPS;
	This->frame_count = dukv->cframes;
	This->interframe_wait = (uint32) (1000.0 / DUCK_GENERAL_FPS);

	dukv->inbuf = HMalloc (DUCK_MAX_FRAME_SIZE);
	dukv->decbuf = HMalloc (
			dukv->decoder.w * dukv->decoder.h * sizeof (uint16));

	return true;
}

static void
dukv_Close (THIS_PTR)
{
	TFB_DuckVideoDecoder* dukv = (TFB_DuckVideoDecoder*) This;

	if (dukv->basename)
	{
		HFree (dukv->basename);
		dukv->basename = NULL;
	}
	if (dukv->frames)
	{
		HFree (dukv->frames);
		dukv->frames = NULL;
	}
	if (dukv->stream)
	{
		uio_fclose (dukv->stream);
		dukv->stream = NULL;
	}
	if (dukv->inbuf)
	{
		HFree (dukv->inbuf);
		dukv->inbuf = NULL;
	}
	if (dukv->decbuf)
	{
		HFree (dukv->decbuf);
		dukv->decbuf = NULL;
	}
}

static int
dukv_DecodeNext (THIS_PTR)
{
	TFB_DuckVideoDecoder* dukv = (TFB_DuckVideoDecoder*) This;
	uint32 fh[2];
	uint32 vofs;
	uint32 vsize;
	uint16 ver;

	if (!dukv->stream || dukv->iframe >= dukv->cframes)
		return 0;

	uio_fseek (dukv->stream, dukv->frames[dukv->iframe], SEEK_SET);
	if (uio_fread (&fh, sizeof (fh), 1, dukv->stream) != 1)
	{
		dukv->last_error = dukve_EOF;
		return 0;
	}

	vofs = UQM_SwapBE32 (fh[0]);
	vsize = UQM_SwapBE32 (fh[1]);
	if (vsize > DUCK_MAX_FRAME_SIZE)
	{
		dukv->last_error = dukve_OutOfBuf;
		return -1;
	}

	uio_fseek (dukv->stream, vofs, SEEK_CUR);
	if (uio_fread (dukv->inbuf, 1, vsize, dukv->stream) != vsize)
	{
		dukv->last_error = dukve_EOF;
		return 0;
	}

	ver = UQM_SwapBE16 (*(uint16*)dukv->inbuf);
	if (ver == 0x0300)
		dukv_DecodeFrameV3 (dukv->inbuf + 0x10, dukv->decbuf,
				dukv->wb, dukv->hb, &dukv->d);
	else
		dukv_DecodeFrame (dukv->inbuf + 0x10, dukv->decbuf,
				dukv->wb, dukv->hb, &dukv->d);

	dukv->iframe++;

	This->callbacks.BeginFrame (This);
	dukv_RenderFrame (This);
	This->callbacks.EndFrame (This);

	if (!This->audio_synced)
	   This->callbacks.SetTimer (This, (uint32) (1000.0f / DUCK_GENERAL_FPS));

	return 1;
}

static uint32
dukv_SeekFrame (THIS_PTR, uint32 frame)
{
	TFB_DuckVideoDecoder* dukv = (TFB_DuckVideoDecoder*) This;
	
	if (frame > dukv->cframes)
		frame = dukv->cframes; // EOS

	return dukv->iframe = frame;
}

static float
dukv_SeekTime (THIS_PTR, float time)
{
	//TFB_DuckVideoDecoder* dukv = (TFB_DuckVideoDecoder*) This;
	uint32 frame = (uint32) (time * DUCK_GENERAL_FPS);
	
	// Note that DUCK_GENERAL_FPS is a float constant
	return dukv_SeekFrame (This, frame) / DUCK_GENERAL_FPS;
}

static uint32
dukv_GetFrame (THIS_PTR)
{
	TFB_DuckVideoDecoder* dukv = (TFB_DuckVideoDecoder*) This;
	
	return dukv->iframe;
}

static float
dukv_GetTime (THIS_PTR)
{
	TFB_DuckVideoDecoder* dukv = (TFB_DuckVideoDecoder*) This;
	
	return (float) dukv->iframe / DUCK_GENERAL_FPS;
}
