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

/* Mixer for low-level sound output drivers
 * Internals
 */

#ifndef LIBS_SOUND_MIXER_MIXERINT_H_
#define LIBS_SOUND_MIXER_MIXERINT_H_

#include "port.h"
#include "types.h"

/*************************************************
 *  Internals
 */

/* Conversion info types and funcs */
typedef enum
{
	mixConvNone = 0,
	mixConvStereoUp = 1,
	mixConvStereoDown = 2,
	mixConvSizeUp = 4,
	mixConvSizeDown = 8

} mixer_ConvFlags;

typedef struct
{
	uint32 srcfmt;
	void *srcdata;
	uint32 srcsize;
	uint32 srcbpc; /* bytes/sample for 1 chan */
	uint32 srcchans;
	uint32 srcsamples;
	
	uint32 dstfmt;
	void *dstdata;
	uint32 dstsize;
	uint32 dstbpc; /* bytes/sample for 1 chan */
	uint32 dstchans;
	uint32 dstsamples;

	mixer_ConvFlags flags;

} mixer_Convertion;

typedef struct
{
	float (* Upsample) (mixer_Source *src, bool left);
	float (* Downsample) (mixer_Source *src, bool left);
	float (* None) (mixer_Source *src, bool left);
} mixer_Resampling;

static void mixer_ConvertBuffer_internal (mixer_Convertion *conv);
static void mixer_ResampleFlat (mixer_Convertion *conv);

static inline sint32 mixer_GetSampleExt (void *src, uint32 bpc);
static inline sint32 mixer_GetSampleInt (void *src, uint32 bpc);
static inline void mixer_PutSampleInt (void *dst, uint32 bpc,
		sint32 samp);
static inline void mixer_PutSampleExt (void *dst, uint32 bpc,
		sint32 samp);

static float mixer_ResampleNone (mixer_Source *src, bool left);
static float mixer_ResampleNearest (mixer_Source *src, bool left);
static float mixer_UpsampleLinear (mixer_Source *src, bool left);
static float mixer_UpsampleCubic (mixer_Source *src, bool left);

/* Source manipulation */
static void mixer_SourceUnqueueAll (mixer_Source *src);
static void mixer_SourceStop_internal (mixer_Source *src);
static void mixer_SourceRewind_internal (mixer_Source *src);
static void mixer_SourceActivate (mixer_Source* src);
static void mixer_SourceDeactivate (mixer_Source* src);

static inline bool mixer_CheckBufferState (mixer_Buffer *buf,
		const char* FuncName);

/* Clipping boundaries */
#define MIX_S16_MAX ((float) SINT16_MAX)
#define MIX_S16_MIN ((float) SINT16_MIN)
#define MIX_S8_MAX  ((float) SINT8_MAX)
#define MIX_S8_MIN  ((float) SINT8_MIN)

/* Channel gain adjustment for clipping reduction */
#define MIX_GAIN_ADJ (0.75f)

/* The Mixer */
static inline bool mixer_SourceGetNextSample (mixer_Source *src,
		float *psamp, bool left);
static inline bool mixer_SourceGetFakeSample (mixer_Source *src,
		float *psamp, bool left);
static inline uint32 mixer_SourceAdvance (mixer_Source *src, bool left);

#endif /* LIBS_SOUND_MIXER_MIXERINT_H_ */
