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

#include <string.h>
#include "video.h"
#include "videodec.h"
#include "dukvid.h"
#include "libs/log.h"
#include "libs/memlib.h"
#include "../../uqm/units.h"

#define MAX_REG_DECODERS 31

static bool vd_inited = false;
static TFB_PixelFormat vd_vidfmt;
static int vd_flags = 0;

struct TFB_RegVideoDecoder
{
	bool builtin;
	bool used;        // ever used indicator
	const char* ext;
	const TFB_VideoDecoderFuncs* funcs;
};
static TFB_RegVideoDecoder vd_decoders[MAX_REG_DECODERS + 1] = 
{
	{true,  true,  "duk", &dukv_DecoderVtbl},
	{false, false,  NULL, NULL}, // null term
};

static void vd_computeMasks (uint32 mask, DWORD* shift, DWORD* loss);

const char*
VideoDecoder_GetName (TFB_VideoDecoder *decoder)
{
	if (!decoder || !decoder->funcs)
		return "(Null)";
	return decoder->funcs->GetName ();
}

bool
VideoDecoder_Init (int flags, int depth, uint32 Rmask, uint32 Gmask,
		uint32 Bmask, uint32 Amask)
{
	TFB_RegVideoDecoder* info;

	vd_inited = false;

	if (depth < 15 || depth > 32)
	{
		log_add (log_Error, "VideoDecoder_Init: "
				"Unsupported video depth %d", depth);
		return false;
	}

	if ((Rmask & Gmask) || (Rmask & Bmask) || (Rmask & Amask) ||
			(Gmask & Bmask) || (Gmask & Amask) || (Bmask & Amask))
	{
		log_add (log_Error, "VideoDecoder_Init: Invalid channel masks");
		return false;
	}

	// BEGIN: adapted from SDL
	vd_vidfmt.BitsPerPixel = depth;
	vd_vidfmt.BytesPerPixel = (depth + 7) / 8;
	vd_vidfmt.Rmask = Rmask;
	vd_vidfmt.Gmask = Gmask;
	vd_vidfmt.Bmask = Bmask;
	vd_vidfmt.Amask = Amask;
	vd_computeMasks (Rmask, &vd_vidfmt.Rshift, &vd_vidfmt.Rloss);
	vd_computeMasks (Gmask, &vd_vidfmt.Gshift, &vd_vidfmt.Gloss);
	vd_computeMasks (Bmask, &vd_vidfmt.Bshift, &vd_vidfmt.Bloss);
	vd_computeMasks (Amask, &vd_vidfmt.Ashift, &vd_vidfmt.Aloss);
	// END: adapted from SDL
	
	// init built-in decoders
	for (info = vd_decoders; info->ext; info++)
	{
		if (!info->funcs->InitModule (flags))
		{
			log_add (log_Error, "VideoDecoder_Init(): "
					"%s video decoder init failed",
					info->funcs->GetName ());
		}
	}

	vd_flags = flags;
	vd_inited = true;

	return true;
}

void
VideoDecoder_Uninit (void)
{
	TFB_RegVideoDecoder* info;

	// uninit all decoders
	// and unregister loaded decoders
	for (info = vd_decoders; info->used; info++)
	{
		if (info->ext) // check if present
			info->funcs->TermModule ();
		
		if (!info->builtin)
		{
			info->used = false;
			info->ext = NULL;
		}
	}
	
	vd_inited = false;
}

TFB_RegVideoDecoder*
VideoDecoder_Register (const char* fileext, TFB_VideoDecoderFuncs* decvtbl)
{
	TFB_RegVideoDecoder* info;
	TFB_RegVideoDecoder* newslot = NULL;

	if (!decvtbl)
	{
		log_add (log_Warning, "VideoDecoder_Register(): Null decoder table");
		return NULL;
	}
	if (!fileext)
	{
		log_add (log_Warning, "VideoDecoder_Register(): Bad file type for %s",
				decvtbl->GetName ());
		return NULL;
	}

	// check if extension already registered
	for (info = vd_decoders; info->used &&
			(!info->ext || strcmp (info->ext, fileext) != 0);
			++info)
	{
		// and pick up an empty slot (where available)
		if (!newslot && !info->ext)
			newslot = info;
	}

	if (info >= vd_decoders + MAX_REG_DECODERS)
	{
		log_add (log_Warning, "VideoDecoder_Register(): Decoders limit reached");
		return NULL;
	}
	else if (info->ext)
	{
		log_add (log_Warning, "VideoDecoder_Register(): "
				"'%s' decoder already registered (%s denied)",
				fileext, decvtbl->GetName ());
		return NULL;
	}

	if (!decvtbl->InitModule (vd_flags))
	{
		log_add (log_Warning, "VideoDecoder_Register(): %s decoder init failed",
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
VideoDecoder_Unregister (TFB_RegVideoDecoder* regdec)
{
	if (regdec < vd_decoders || regdec >= vd_decoders + MAX_REG_DECODERS ||
			!regdec->ext || !regdec->funcs)
	{
		log_add (log_Warning, "VideoDecoder_Unregister(): "
				"Invalid or expired decoder passed");
		return;
	}
	
	regdec->funcs->TermModule ();
	regdec->ext = NULL;
	regdec->funcs = NULL;
}

const TFB_VideoDecoderFuncs*
VideoDecoder_Lookup (const char* fileext)
{
	TFB_RegVideoDecoder* info;

	for (info = vd_decoders; info->used &&
			(!info->ext || strcmp (info->ext, fileext) != 0);
			++info)
		;
	return info->ext ? info->funcs : NULL;
}

TFB_VideoDecoder*
VideoDecoder_Load (uio_DirHandle *dir, const char *filename)
{
	const char* pext;
	TFB_RegVideoDecoder* info;
	TFB_VideoDecoder* decoder;
	

	if (!vd_inited)
		return NULL;

	pext = strrchr (filename, '.');
	if (!pext)
	{
		log_add (log_Warning, "VideoDecoder_Load: Unknown file type");
		return NULL;
	}
	++pext;

	for (info = vd_decoders; info->used &&
			(!info->ext || strcmp (info->ext, pext) != 0);
			++info)
		;
	if (!info->ext)
	{
		log_add (log_Warning, "VideoDecoder_Load: Unsupported file type");
		return NULL;
	}

	decoder = HCalloc (info->funcs->GetStructSize ());
	decoder->funcs = info->funcs;
	if (!decoder->funcs->Init (decoder, &vd_vidfmt))
	{
		log_add (log_Warning, "VideoDecoder_Load: "
				"Cannot init '%s' decoder, code %d",
				decoder->funcs->GetName (),
				decoder->funcs->GetError (decoder));
		HFree (decoder);
		return NULL;
	}

	decoder->dir = dir;
	decoder->filename = (char *) HMalloc (strlen (filename) + 1);
	strcpy (decoder->filename, filename);
	decoder->error = VIDEODECODER_OK;

	if (!decoder->funcs->Open (decoder, dir, filename))
	{
		log_add (log_Warning, "VideoDecoder_Load: "
				"'%s' decoder did not load %s, code %d",
				decoder->funcs->GetName (),	filename,
				decoder->funcs->GetError (decoder));
		
		VideoDecoder_Free (decoder);
		return NULL;
	}

	decoder->w = RES_SCALE(decoder->w);
	decoder->h = RES_SCALE(decoder->h) - IF_HD(7);

	return decoder;
}

// return: >0 = OK, 0 = EOF, <0 = Error
int
VideoDecoder_Decode (TFB_VideoDecoder *decoder)
{
	int ret;

	if (!decoder)
		return 0;

	decoder->cur_frame = decoder->funcs->GetFrame (decoder);
	decoder->pos = decoder->funcs->GetTime (decoder);

	ret = decoder->funcs->DecodeNext (decoder);
	if (ret == 0)
		decoder->error = VIDEODECODER_EOF;
	else if (ret < 0)
		decoder->error = VIDEODECODER_ERROR;
	else
		decoder->error = VIDEODECODER_OK;

	return ret;
}

float
VideoDecoder_Seek (TFB_VideoDecoder *decoder, float pos)
{
	if (!decoder)
		return 0.0;

	decoder->pos = decoder->funcs->SeekTime (decoder, pos);
	decoder->cur_frame = decoder->funcs->GetFrame (decoder);

	return decoder->pos;
}

uint32
VideoDecoder_SeekFrame (TFB_VideoDecoder *decoder, uint32 frame)
{
	if (!decoder)
		return 0;

	decoder->cur_frame = decoder->funcs->SeekFrame (decoder, frame);
	decoder->pos = decoder->funcs->GetTime (decoder);

	return decoder->cur_frame;
}

void
VideoDecoder_Rewind (TFB_VideoDecoder *decoder)
{
	if (!decoder)
		return;

	VideoDecoder_Seek (decoder, 0);
}

void
VideoDecoder_Free (TFB_VideoDecoder *decoder)
{
	if (!decoder)
		return;
	
	decoder->funcs->Close (decoder);
	decoder->funcs->Term (decoder);

	HFree (decoder->filename);
	HFree (decoder);
}

// BEGIN: adapted from SDL
static void
vd_computeMasks (uint32 mask, DWORD* shift, DWORD* loss)
{
	*shift = 0;
	*loss = 8;
	if (mask)
	{
		for (; !(mask & 1); mask >>= 1 )
			++*shift;

		for (; (mask & 1); mask >>= 1 )
			--*loss;
	}
}
// END: adapted from SDL
