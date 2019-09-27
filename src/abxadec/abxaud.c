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

/* CDP module sample
 * .abx speech track decoder
 */

/*
 * Derived from decoder by Serge van den Boom (svdb@stack.nl),
 * The actual conversion code (somewhat moded) is from Toys for Bob.
 * So far, it ignores sample rates, so it will work ok as long as all
 * the frames have the same frequency. This is probably
 * enough for our purposes.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
//#include "port.h"
#include "types.h"
#include "port.h"
#include "libs/cdp/cdp_imem.h"
#include "libs/cdp/cdp_iio.h"
#include "libs/cdp/cdp_isnd.h"
#include "libs/cdp/cdpmod.h"
#include "abxaud.h"
#include "endian_uqm.h"

#define DATA_BUF_SIZE	  0x8000
#define DUCK_GENERAL_FPS  14.622f


#define THIS_PTR TFB_SoundDecoder* This

static const char* abxa_GetName (void);
static bool abxa_InitModule (int flags, const TFB_DecoderFormats*);
static void abxa_TermModule ();
static uint32 abxa_GetStructSize (void);
static int abxa_GetError (THIS_PTR);
static bool abxa_Init (THIS_PTR);
static void abxa_Term (THIS_PTR);
static bool abxa_Open (THIS_PTR, uio_DirHandle *dir, const char *filename);
static void abxa_Close (THIS_PTR);
static int abxa_Decode (THIS_PTR, void* buf, sint32 bufsize);
static uint32 abxa_Seek (THIS_PTR, uint32 pcm_pos);
static uint32 abxa_GetFrame (THIS_PTR);

static TFB_SoundDecoderFuncs abxa_DecoderVtbl = 
{
	abxa_GetName,
	abxa_InitModule,
	abxa_TermModule,
	abxa_GetStructSize,
	abxa_GetError,
	abxa_Init,
	abxa_Term,
	abxa_Open,
	abxa_Close,
	abxa_Decode,
	abxa_Seek,
	abxa_GetFrame,
};

static bool abxa_module_init (cdp_Module* module, cdp_Itf_Host* hostitf);
static void abxa_module_term ();

// The one and only "cdpmodinfo" symbol
// on win32, it does not have to be named like so
// the exported name can be overridden with .def file
cdp_ModuleInfo CDPEXPORT CDP_INFO_SYM =
{
	sizeof (cdp_ModuleInfo),    // size of struct for version control
	CDPAPI_VERSION,             // API version we are using
	1, 0, 2,                    // our module version
	0, 3, 0,                    // host version required, purely informational
	CDP_MODINFO_RESERVED1,      // reserved
	"UQM",                      // CDP context name (we can use UQM)
	"Abx Decoder",              // CDP mod name
	"1.0",                      // CDP mod version
	"Alex Volkov",              // CDP mod author
	"http://sc2.sf.net",        // CDP mod URL (do not have any yet)
	"Sample CDP-based decoder", // CDP mod comment
	CDP_MODINFO_RESERVED2,      // reserved
	abxa_module_init,           // init entrypoint
	abxa_module_term,           // term entrypoint
};

typedef struct
{
	uint16 num_frames;   // total number of frame
	uint32 tot_size;     // total size of decoded stream
	uint16 frame_samps;  // samples per frame
	uint16 freq;         // general sampling frequency

} abxa_Header;

typedef struct
{
	uint32 ofs;    // file offset of frame
	uint16 fsize;  // compressed file size
	uint16 usize;  // uncompressed file size

} abxa_FrameInfo;

#define SQLCH 0x40		// Squelch byte flag
#define RESYNC 0x80 	// Resync byte flag.

#define DELTAMOD 0x30 	// Delta modulation bits.

#define ONEBIT 0x10 		// One bit delta modulate
#define TWOBIT 0x20 		// Two bit delta modulate
#define FOURBIT 0x30		// four bit delta modulate

#define MULTIPLIER 0x0F  // Bottom nibble contains multiplier value.
#define SQUELCHCNT 0x3F  // Bits for squelching.

typedef struct
{
	uint16 usize;
	uint16 freq;
	uint8  frame_size;
	uint8  sqelch;
	uint16 max_error;

} abxa_FrameHeader;

typedef struct
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
	uio_Stream* abx;
	abxa_FrameInfo* frames;
	// buffer
	void* data;
	uint32 maxdata;
	uint32 cbdata;
	uint32 dataofs;

} abxa_SoundDecoder;

// interfaces our decoder needs
cdp_ItfDef game_itfs[] =
{
	{CDPITF_KIND_MEMORY},
	{CDPITF_KIND_IO},
	{CDPITF_KIND_SOUND},
	
	{CDPITF_KIND_INVALID} // term
};

static cdp_Module* abxa_mod = NULL;       // our module handle
static cdp_Itf_Host* abxa_ihost = NULL;   // HOST interface ptr
static cdp_Itf_Memory* abxa_imem = NULL;  // MEMORY interface ptr
static cdp_Itf_Io* abxa_iio = NULL;       // IO interface ptr
static cdp_Itf_Sound* abxa_isnd = NULL;   // SOUND interface ptr
static const TFB_DecoderFormats* abxa_formats = NULL;
static TFB_RegSoundDecoder* abxa_regdec = NULL;   // registered decoder

static bool
abxa_module_init (cdp_Module* module, cdp_Itf_Host* hostitf)
{
	abxa_mod = module;
	abxa_ihost = hostitf;

	if (!hostitf->GetItfs (game_itfs))
		return false;

	abxa_imem = game_itfs[0].itf;
	abxa_iio = game_itfs[1].itf;
	abxa_isnd = game_itfs[2].itf;

	abxa_regdec = abxa_isnd->RegisterDecoder ("abx", &abxa_DecoderVtbl);
	if (!abxa_regdec)
	{
		fprintf (stderr, "abxa_module_init(): "
				"Could not register audio decoder\n");
		return false;
	}

	return true;
}

static void
abxa_module_term ()
{
	if (abxa_regdec)
		abxa_isnd->UnregisterDecoder (abxa_regdec);

	// do nothing loop for 1 trillion iterations
}

static sint32
abxa_readHeader (abxa_SoundDecoder* abxa, abxa_Header* hdr)
{
	if (1 != abxa_iio->fread (&hdr->num_frames, sizeof (hdr->num_frames), 1, abxa->abx) ||
		1 != abxa_iio->fread (&hdr->tot_size, sizeof (hdr->tot_size), 1, abxa->abx) ||
		1 != abxa_iio->fread (&hdr->frame_samps, sizeof (hdr->frame_samps), 1, abxa->abx) ||
		1 != abxa_iio->fread (&hdr->freq, sizeof (hdr->freq), 1, abxa->abx))
	{
		abxa->last_error = errno;
		return abxa_ErrBadFile;
	}
	// byte swap when necessary
	hdr->num_frames  = UQM_SwapLE16 (hdr->num_frames);
	hdr->tot_size    = UQM_SwapLE32 (hdr->tot_size);
	hdr->frame_samps = UQM_SwapLE16 (hdr->frame_samps);
	hdr->freq        = UQM_SwapLE16 (hdr->freq);

	return 0;
}

static signed char abxa_trans[16 * 16] =
{
    -8,-7,-6,-5,-4,-3,-2,-1,1,2,3,4,5,6,7,8,                // Multiplier of 1
    -16,-14,-12,-10,-8,-6,-4,-2,2,4,6,8,10,12,14,16,        // Multiplier of 2
    -24,-21,-18,-15,-12,-9,-6,-3,3,6,9,12,15,18,21,24,      // Multiplier of 3
    -32,-28,-24,-20,-16,-12,-8,-4,4,8,12,16,20,24,28,32,    // Multiplier of 4
    -40,-35,-30,-25,-20,-15,-10,-5,5,10,15,20,25,30,35,40,  // Multiplier of 5
    -48,-42,-36,-30,-24,-18,-12,-6,6,12,18,24,30,36,42,48,  // Multiplier of 6
    -56,-49,-42,-35,-28,-21,-14,-7,7,14,21,28,35,42,49,56,  // Multiplier of 7
    -64,-56,-48,-40,-32,-24,-16,-8,8,16,24,32,40,48,56,64,  // Multiplier of 8
    -72,-63,-54,-45,-36,-27,-18,-9,9,18,27,36,45,54,63,72,  // Multiplier of 9
    -80,-70,-60,-50,-40,-30,-20,-10,10,20,30,40,50,60,70,80,  // Multiplier of 10
    -88,-77,-66,-55,-44,-33,-22,-11,11,22,33,44,55,66,77,88,  // Multiplier of 11
    -96,-84,-72,-60,-48,-36,-24,-12,12,24,36,48,60,72,84,96,  // Multiplier of 12
    -104,-91,-78,-65,-52,-39,-26,-13,13,26,39,52,65,78,91,104,  // Multiplier of 13
    -112,-98,-84,-70,-56,-42,-28,-14,14,28,42,56,70,84,98,112,  // Multiplier of 14
    -120,-105,-90,-75,-60,-45,-30,-15,15,30,45,60,75,90,105,120,// Multiplier of 15
    -128,-112,-96,-80,-64,-48,-32,-16,16,32,48,64,80,96,112,127,// Multiplier of 16
};

static sint32
abxa_decodeFrame (abxa_SoundDecoder* abxa, abxa_FrameHeader* hdr,
		uint8* input, uint32 inputsize)
{
	uint8* inend;
	uint8* output;
	uint8* outptr;
	sint16 prev;
	sint32 outputsize;
	
	output = outptr = abxa->data;
	inend = input + inputsize;
	prev = *input++;  // Get initial previous data point.
	*output++ = prev;

	while (input < inend)
	{
		uint16 bytes;
		uint8 sample;

		sample = *input++;  // Get sample.
		if (sample & RESYNC) // Is it a resync byte?
		{
			//--slen;  // Decrement output sample length.

			prev = (sample & 0x7F) << 1; // Store resync byte.
			*output++ = prev;
		}
		else if (sample & SQLCH) // Is it a squelch byte?
		{
			bytes = sample & SQUELCHCNT;	// And off the number of squelch bytes
			// ?? the following makes no sense, should be --slen;
			//slen -= bytes;	// Decrement total samples remaining count.

			memset (output, prev, bytes);
			output += bytes;
		}
		else  // Must be a delta modulate byte!!
		{
			sint8 *base;

			//slen -= hdr->frame_size; // Pulling one frame out.
					// Compute base address to multiplier table.
			base = abxa_trans + (sample & MULTIPLIER) * 16;
			switch (sample & DELTAMOD) // Delta mod resolution.
			{
				case ONEBIT:
				{
					sint16 up;

					up = base[8];	 // Go up 1 bit.
					for (bytes = hdr->frame_size / 8; bytes; bytes--)
					{
						uint8 mask;

						sample = *input++;
						for (mask = 0x80; mask; mask >>= 1)
						{
							if ( sample & mask )
								prev += up;
							else
								prev -= up;
							if ( prev < 0 ) prev = 0;
							else if ( prev > 255 ) prev = 255;
							*output++ = prev;
						}
					}
					break;
				}
				case TWOBIT:
					base += 6; // Base address of two bit delta's.
					for (bytes = hdr->frame_size / 4; bytes; bytes--)
					{
						sample = *input++;

						prev += base[sample>>6];
						if ( prev < 0 ) prev = 0;
						else if ( prev > 255 ) prev = 255;
						*output++ = prev;

						prev += base[(sample>>4)&0x3];
						if ( prev < 0 ) prev = 0;
						else if ( prev > 255 ) prev = 255;
						*output++ = prev;

						prev += base[(sample>>2)&0x3];
						if ( prev < 0 ) prev = 0;
						else if ( prev > 255 ) prev = 255;
						*output++ = prev;

						prev += base[sample&0x3];
						if ( prev < 0 ) prev = 0;
						else if ( prev > 255 ) prev = 255;
						*output++ = prev;
					}
					break;
				case FOURBIT:
					for (bytes = hdr->frame_size / 2; bytes; bytes--)
					{
						sample = *input++;

						prev += base[sample>>4];
						if ( prev < 0 ) prev = 0;
						else if ( prev > 255 ) prev = 255;
						*output++ = prev;

						prev += base[sample&0x0F];
						if ( prev < 0 ) prev = 0;
						else if ( prev > 255 ) prev = 255;
						*output++ = prev;
					}
					break;
			}
		}
		// While still audio data to decompress....
	}

	outputsize = output - outptr;
	abxa->cbdata += outputsize;

	return outputsize;
}


static sint32
abxa_readNextFrame (abxa_SoundDecoder* abxa)
{
	abxa_FrameHeader* hdr;
	uint8* p;
	uint32 fsize;
	sint32 ret;

	fsize = abxa->frames[abxa->iframe].fsize;
	abxa_iio->fseek (abxa->abx, abxa->frames[abxa->iframe].ofs, SEEK_SET);
	// dump encoded data at the end of the buffer aligned on 8-byte
	p = ((uint8*)abxa->data + abxa->maxdata - ((fsize + 7) & (-8)));
	if (abxa_iio->fread (p, 1, fsize, abxa->abx) != fsize)
	{
		abxa->last_error = errno;
		return abxa_ErrBadFile;
	}
	hdr = (abxa_FrameHeader*) p;
	p += sizeof (abxa_FrameHeader);
	fsize -= sizeof (abxa_FrameHeader);

	hdr->usize     = UQM_SwapLE16 (hdr->usize);
	hdr->freq      = UQM_SwapLE16 (hdr->freq);
	hdr->max_error = UQM_SwapLE16 (hdr->max_error);

	if (hdr->freq == 0)
	{
		hdr->freq = abxa->decoder.frequency;
	}
	else if (hdr->freq != abxa->decoder.frequency)
	{
		fprintf (stderr, "abxa_readNextFrame(): "
				"WARNING: Frame frequency (%u) != global frequency (%u) "
				"for frame %u\n",
				hdr->freq, abxa->decoder.frequency, abxa->iframe);
		//abxa->decoder.frequency = hdr->freq;
	}

	abxa->iframe++;

	ret = abxa_decodeFrame (abxa, hdr, p, fsize);
	if (ret > 0 && ret != hdr->usize)
	{
		fprintf (stderr, "abxa_readNextFrame(): "
				"WARNING: decompressed frame size (%d) != specified size (%u) "
				"for frame %u\n",
				ret, hdr->usize, abxa->iframe);
	}

	return ret;
}

static sint32
abxa_stuffBuffer (abxa_SoundDecoder* abxa, void* buf, sint32 bufsize)
{
	sint32 dataleft;

	dataleft = abxa->cbdata - abxa->dataofs;
	if (dataleft > 0)
	{
		if (dataleft > bufsize)
			dataleft = bufsize & (-4);
		memcpy (buf, (uint8*)abxa->data + abxa->dataofs, dataleft);
		abxa->dataofs += dataleft;
	}

	if (abxa->cbdata > 0 && abxa->dataofs >= abxa->cbdata)
		abxa->cbdata = abxa->dataofs = 0; // reset for new data

	return dataleft;
}


static const char*
abxa_GetName (void)
{
	return "Abx";
}

static bool
abxa_InitModule (int flags, const TFB_DecoderFormats* fmts)
{
	abxa_formats = fmts;
	return true;

	(void)flags;	// laugh at compiler warning
}

static void
abxa_TermModule ()
{
	// no specific module term
}

static uint32
abxa_GetStructSize (void)
{
	return sizeof (abxa_SoundDecoder);
}

static int
abxa_GetError (THIS_PTR)
{
	abxa_SoundDecoder* abxa = (abxa_SoundDecoder*) This;
	int ret = abxa->last_error;
	abxa->last_error = abxa_ErrNone;
	return ret;
}

static bool
abxa_Init (THIS_PTR)
{
	//abxa_SoundDecoder* abxa = (abxa_SoundDecoder*) This;
	/* enable if 16bit PCM abx exist
	This->need_swap =
			abxa_formats->big_endian != abxa_formats->want_big_endian;
	*/
	This->need_swap = false;
	return true;
}

static void
abxa_Term (THIS_PTR)
{
	//abxa_SoundDecoder* abxa = (abxa_SoundDecoder*) This;
	abxa_Close (This); // ensure cleanup
}

static bool
abxa_Open (THIS_PTR, uio_DirHandle *dir, const char *file)
{
	abxa_SoundDecoder* abxa = (abxa_SoundDecoder*) This;
	abxa_Header hdr;
	abxa_FrameInfo* info;
	size_t cread;
	uint32 i;

	abxa->abx = abxa_iio->fopen (dir, file, "rb");
	if (!abxa->abx)
	{
		abxa->last_error = errno;
		return false;
	}
	if (abxa_readHeader (abxa, &hdr))
	{
		abxa_Close (This);
		return false;
	}

	abxa->cframes = hdr.num_frames;

	abxa->frames = abxa_imem->malloc (abxa->cframes * sizeof (abxa_FrameInfo));
	cread = abxa_iio->fread (abxa->frames, sizeof (abxa_FrameInfo),
			abxa->cframes, abxa->abx);
	if (cread != abxa->cframes)
	{
		abxa->last_error = abxa_ErrBadFile;
		abxa_Close (This);
		return false;
	}

	// byte swap when necessary
	for (i = 0, info = abxa->frames; i < abxa->cframes; ++i, ++info)
	{
		info->ofs =   UQM_SwapLE32 (info->ofs);
		info->fsize = UQM_SwapLE16 (info->fsize);
		info->usize = UQM_SwapLE16 (info->usize);
	}

	This->frequency = hdr.freq;
	This->format = abxa_formats->mono8;
	abxa->channels = 1;
	abxa->pcm_frame = hdr.frame_samps;
	abxa->data = abxa_imem->malloc (hdr.frame_samps * 2);
	abxa->maxdata = hdr.frame_samps * 2;

	// estimate
	This->length = (float) hdr.tot_size / hdr.freq;

	abxa->last_error = 0;

	return true;
}

static void
abxa_Close (THIS_PTR)
{
	abxa_SoundDecoder* abxa = (abxa_SoundDecoder*) This;

	if (abxa->data)
	{
		abxa_imem->free (abxa->data);
		abxa->data = NULL;
	}
	if (abxa->frames)
	{
		abxa_imem->free (abxa->frames);
		abxa->frames = NULL;
	}
	if (abxa->abx)
	{
		abxa_iio->fclose (abxa->abx);
		abxa->abx = NULL;
	}
	abxa->last_error = 0;
}

static int
abxa_Decode (THIS_PTR, void* buf, sint32 bufsize)
{
	abxa_SoundDecoder* abxa = (abxa_SoundDecoder*) This;
	sint32 stuffed;
	sint32 total = 0;

	if (bufsize <= 0)
		return abxa->last_error = abxa_ErrBadArg;

	do
	{
		stuffed = abxa_stuffBuffer (abxa, buf, bufsize);
		((uint8*)buf) += stuffed;
		bufsize -= stuffed;
		total += stuffed;
	
		if (bufsize > 0 && abxa->iframe < abxa->cframes)
		{
			stuffed = abxa_readNextFrame (abxa);
			if (stuffed <= 0)
				return stuffed;
		}
	} while (bufsize > 0 && abxa->iframe < abxa->cframes);

	return total;
}

static uint32
abxa_Seek (THIS_PTR, uint32 pcm_pos)
{
	abxa_SoundDecoder* abxa = (abxa_SoundDecoder*) This;
	uint32 iframe;

	iframe = pcm_pos / abxa->pcm_frame;
	if (iframe < abxa->cframes)
	{
		abxa->iframe = iframe;
		abxa->cbdata = 0;
		abxa->dataofs = 0;
	}
	return abxa->iframe * abxa->pcm_frame;
}

static uint32
abxa_GetFrame (THIS_PTR)
{
	abxa_SoundDecoder* abxa = (abxa_SoundDecoder*) This;

	// if there is nothing buffered return the actual current frame
	//  otherwise return previous
	return abxa->dataofs == abxa->cbdata ?
			abxa->iframe : abxa->iframe - 1;
}
