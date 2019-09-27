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
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "mixer.h"
#include "mixerint.h"
#include "libs/misc.h"
#include "libs/threadlib.h"
#include "libs/log.h"
#include "libs/memlib.h"

static uint32 mixer_initialized = 0;
static uint32 mixer_format;
static uint32 mixer_chansize;
static uint32 mixer_sampsize;
static uint32 mixer_freq;
static uint32 mixer_channels;
static uint32 last_error = MIX_NO_ERROR;
static mixer_Quality mixer_quality;
static mixer_Resampling mixer_resampling;
static mixer_Flags mixer_flags;

/* when locking more than one mutex
 * you must lock them in this order
 */
static RecursiveMutex src_mutex;
static RecursiveMutex buf_mutex;
static RecursiveMutex act_mutex;

#define MAX_SOURCES 8
mixer_Source *active_sources[MAX_SOURCES];


/*************************************************
 *  Internals
 */

static void
mixer_SetError (uint32 error)
{
	last_error = error;
}


/*************************************************
 *  General interface
 */

uint32
mixer_GetError (void)
{
	uint32 error = last_error;
	last_error = MIX_NO_ERROR;
	return error;
}

/* Initialize the mixer with a certain audio format */
bool
mixer_Init (uint32 frequency, uint32 format, mixer_Quality quality,
		mixer_Flags flags)
{
	if (mixer_initialized)
		mixer_Uninit ();

	last_error = MIX_NO_ERROR;
	memset (active_sources, 0, sizeof(mixer_Source*) * MAX_SOURCES);
	
	mixer_chansize = MIX_FORMAT_BPC (format);
	mixer_channels = MIX_FORMAT_CHANS (format);
	mixer_sampsize = MIX_FORMAT_SAMPSIZE (format);
	mixer_freq = frequency;
	mixer_quality = quality;
	mixer_format = format;
	mixer_flags = flags;
	
	mixer_resampling.None = mixer_ResampleNone;
	mixer_resampling.Downsample = mixer_ResampleNearest;
	if (mixer_quality == MIX_QUALITY_DEFAULT)
		mixer_resampling.Upsample = mixer_UpsampleLinear;
	else if (mixer_quality == MIX_QUALITY_HIGH)
		mixer_resampling.Upsample = mixer_UpsampleCubic;
	else
		mixer_resampling.Upsample = mixer_ResampleNearest;

	src_mutex = CreateRecursiveMutex("mixer_SourceMutex", SYNC_CLASS_AUDIO);
	buf_mutex = CreateRecursiveMutex("mixer_BufferMutex", SYNC_CLASS_AUDIO);
	act_mutex = CreateRecursiveMutex("mixer_ActiveMutex", SYNC_CLASS_AUDIO);

	mixer_initialized = 1;

	return true;
}

/* Uninitialize the mixer */
void
mixer_Uninit (void)
{
	if (mixer_initialized)
	{
		DestroyRecursiveMutex (src_mutex);
		DestroyRecursiveMutex (buf_mutex);
		DestroyRecursiveMutex (act_mutex);
		mixer_initialized = 0;
	}
}


/**********************************************************
 * THE mixer
 *
 */

void
mixer_MixChannels (void *userdata, uint8 *stream, sint32 len)
{
	uint8 *end_stream = stream + len;
	bool left = true;

	/* keep this order or die */
	LockRecursiveMutex (src_mutex);
	LockRecursiveMutex (buf_mutex);
	LockRecursiveMutex (act_mutex);

	for (; stream < end_stream; stream += mixer_chansize)
	{
		uint32 i;
		float fullsamp = 0;
		
		for (i = 0; i < MAX_SOURCES; i++)
		{
			mixer_Source *src;
			float samp = 0;
			
			/* find next source */
			for (; i < MAX_SOURCES && (
					(src = active_sources[i]) == 0
					|| src->state != MIX_PLAYING
					|| !mixer_SourceGetNextSample (src, &samp, left));
					i++)
				;

			if (i < MAX_SOURCES)
			{
				/* sample acquired */
				fullsamp += samp;
			}
		}

		/* clip the sample */
		if (mixer_chansize == 2)
		{
			/* check S16 clipping */
			if (fullsamp > SINT16_MAX)
				fullsamp = SINT16_MAX;
			else if (fullsamp < SINT16_MIN)
				fullsamp = SINT16_MIN;
		}
		else
		{
			/* check S8 clipping */
			if (fullsamp > SINT8_MAX)
				fullsamp = SINT8_MAX;
			else if (fullsamp < SINT8_MIN)
				fullsamp = SINT8_MIN;
		}

		mixer_PutSampleExt (stream, mixer_chansize, (sint32)fullsamp);
		if (mixer_channels == 2)
			left = !left;
	}

	/* keep this order or die */
	UnlockRecursiveMutex (act_mutex);
	UnlockRecursiveMutex (buf_mutex);
	UnlockRecursiveMutex (src_mutex);

	(void) userdata; // satisfying compiler - unused arg
}

/* fake mixer -- only process buffer and source states */
void
mixer_MixFake (void *userdata, uint8 *stream, sint32 len)
{
	uint8 *end_stream = stream + len;
	bool left = true;

	/* keep this order or die */
	LockRecursiveMutex (src_mutex);
	LockRecursiveMutex (buf_mutex);
	LockRecursiveMutex (act_mutex);

	for (; stream < end_stream; stream += mixer_chansize)
	{
		uint32 i;

		for (i = 0; i < MAX_SOURCES; i++)
		{
			mixer_Source *src;
			float samp;
			
			/* find next source */
			for (; i < MAX_SOURCES && (
					(src = active_sources[i]) == 0
					|| src->state != MIX_PLAYING
					|| !mixer_SourceGetFakeSample (src, &samp, left));
					i++)
				;
		}
		if (mixer_channels == 2)
			left = !left;
	}

	/* keep this order or die */
	UnlockRecursiveMutex (act_mutex);
	UnlockRecursiveMutex (buf_mutex);
	UnlockRecursiveMutex (src_mutex);

	(void) userdata; // satisfying compiler - unused arg
}


/*************************************************
 *  Sources interface
 */

/* generate n sources */
void
mixer_GenSources (uint32 n, mixer_Object *psrcobj)
{
	if (n == 0)
		return; /* do nothing per OpenAL */

	if (!psrcobj)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_GenSources() called with null ptr");
		return;
	}
	for (; n; n--, psrcobj++)
	{
		mixer_Source *src;

		src = (mixer_Source *) HMalloc (sizeof (mixer_Source));
		src->magic = mixer_srcMagic;
		src->locked = false;
		src->state = MIX_INITIAL;
		src->looping = false;
		src->gain = MIX_GAIN_ADJ;
		src->cqueued = 0;
		src->cprocessed = 0;
		src->firstqueued = 0;
		src->nextqueued = 0;
		src->prevqueued = 0;
		src->lastqueued = 0;
		src->pos = 0;
		src->count = 0;

		*psrcobj = (mixer_Object) src;
	}
}

/* delete n sources */
void
mixer_DeleteSources (uint32 n, mixer_Object *psrcobj)
{
	uint32 i;
	mixer_Object *pcurobj;

	if (n == 0)
		return; /* do nothing per OpenAL */

	if (!psrcobj)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_DeleteSources() called with null ptr");
		return;
	}

	LockRecursiveMutex (src_mutex);

	/* check to make sure we can delete all sources */
	for (i = n, pcurobj = psrcobj; i && pcurobj; i--, pcurobj++)
	{
		mixer_Source *src = (mixer_Source *) *pcurobj;

		if (!src)
			continue;

		if (src->magic != mixer_srcMagic)
			break;
	}

	if (i)
	{	/* some source failed */
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_DeleteSources(): not a source");
	}
	else
	{	/* all sources checked out */
		for (; n; n--, psrcobj++)
		{
			mixer_Source *src = (mixer_Source *) *psrcobj;

			if (!src)
				continue;

			/* stopping should not be necessary
			 * under ideal circumstances
			 */
			if (src->state != MIX_INITIAL)
				mixer_SourceStop_internal (src);

			/* unqueueing should not be necessary
			 * under ideal circumstances
			 */
			mixer_SourceUnqueueAll (src);
			HFree (src);
			*psrcobj = 0;
		}
	}

	UnlockRecursiveMutex (src_mutex);
}

/* check if really is a source */
bool
mixer_IsSource (mixer_Object srcobj)
{
	mixer_Source *src = (mixer_Source *) srcobj;
	bool ret;

	if (!src)
		return false;

	LockRecursiveMutex (src_mutex);
	ret = src->magic == mixer_srcMagic;
	UnlockRecursiveMutex (src_mutex);

	return ret;
}

/* set source integer property */
void
mixer_Sourcei (mixer_Object srcobj, mixer_SourceProp pname,
		mixer_IntVal value)
{
	mixer_Source *src = (mixer_Source *) srcobj;

	if (!src)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_Sourcei() called with null source");
		return;
	}

	LockRecursiveMutex (src_mutex);

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_Sourcei(): not a source");
	}
	else
	{
		switch (pname)
		{
		case MIX_LOOPING:
			src->looping = value;
			break;
		case MIX_BUFFER:
			{
				mixer_Buffer *buf = (mixer_Buffer *) value;

				if (src->cqueued > 0)
					mixer_SourceUnqueueAll (src);
				
				if (buf && !mixer_CheckBufferState (buf, "mixer_Sourcei"))
					break;

				src->firstqueued = buf;
				src->nextqueued = src->firstqueued;
				src->prevqueued = 0;
				src->lastqueued = src->nextqueued;
				if (src->lastqueued)
					src->lastqueued->next = 0;
				src->cqueued = 1;
			}
			break;
		case MIX_SOURCE_STATE:
			if (value == MIX_INITIAL)
			{
				mixer_SourceRewind_internal (src);
			}
			else
			{
				log_add (log_Debug, "mixer_Sourcei(MIX_SOURCE_STATE): "
						"unsupported state, call ignored");
			}
			break;
		default:
			mixer_SetError (MIX_INVALID_ENUM);
			log_add (log_Debug, "mixer_Sourcei() called "
					"with unsupported property %u", pname);
		}
	}

	UnlockRecursiveMutex (src_mutex);
}

/* set source float property */
void
mixer_Sourcef (mixer_Object srcobj, mixer_SourceProp pname, float value)
{
	mixer_Source *src = (mixer_Source *) srcobj;
	
	if (!src)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_Sourcef() called with null source");
		return;
	}

	LockRecursiveMutex (src_mutex);

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_Sourcef(): not a source");
	}
	else
	{
		switch (pname)
		{
		case MIX_GAIN:
			src->gain = value * MIX_GAIN_ADJ;
			break;
		default:
			log_add (log_Debug, "mixer_Sourcei() called "
				"with unsupported property %u", pname);
		}
	}

	UnlockRecursiveMutex (src_mutex);
}

/* set source float array property (CURRENTLY NOT IMPLEMENTED) */
void mixer_Sourcefv (mixer_Object srcobj, mixer_SourceProp pname, float *value)
{
	(void)srcobj;
	(void)pname;
	(void)value;
}


/* get source integer property */
void
mixer_GetSourcei (mixer_Object srcobj, mixer_SourceProp pname,
		mixer_IntVal *value)
{
	mixer_Source *src = (mixer_Source *) srcobj;

	if (!src || !value)
	{
		mixer_SetError (src ? MIX_INVALID_VALUE : MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_GetSourcei() called with null param");
		return;
	}

	LockRecursiveMutex (src_mutex);

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_GetSourcei(): not a source");
	}
	else
	{
		switch (pname)
		{
		case MIX_LOOPING:
			*value = src->looping;
			break;
		case MIX_BUFFER:
			*value = (mixer_IntVal) src->firstqueued;
			break;
		case MIX_SOURCE_STATE:
			*value = src->state;
			break;
		case MIX_BUFFERS_QUEUED:
			*value = src->cqueued;
			break;
		case MIX_BUFFERS_PROCESSED:
			*value = src->cprocessed;
			break;
		default:
			mixer_SetError (MIX_INVALID_ENUM);
			log_add (log_Debug, "mixer_GetSourcei() called "
					"with unsupported property %u", pname);
		}
	}

	UnlockRecursiveMutex (src_mutex);
}

/* get source float property */
void
mixer_GetSourcef (mixer_Object srcobj, mixer_SourceProp pname,
		float *value)
{
	mixer_Source *src = (mixer_Source *) srcobj;

	if (!src || !value)
	{
		mixer_SetError (src ? MIX_INVALID_VALUE : MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_GetSourcef() called with null param");
		return;
	}

	LockRecursiveMutex (src_mutex);

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_GetSourcef(): not a source");
	}
	else
	{
		switch (pname)
		{
		case MIX_GAIN:
			*value = src->gain / MIX_GAIN_ADJ;
			break;
		default:
			log_add (log_Debug, "mixer_GetSourcef() called "
					"with unsupported property %u", pname);
		}
	}

	UnlockRecursiveMutex (src_mutex);
}

/* start the source; add it to active array */
void
mixer_SourcePlay (mixer_Object srcobj)
{
	mixer_Source *src = (mixer_Source *) srcobj;

	if (!src)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_SourcePlay() called with null source");
		return;
	}

	LockRecursiveMutex (src_mutex);

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_SourcePlay(): not a source");
	}
	else /* should make the source active */
	{
		if (src->state < MIX_PLAYING)
		{
			if (src->firstqueued && !src->nextqueued)
				mixer_SourceRewind_internal (src);
			mixer_SourceActivate (src);
		}
		src->state = MIX_PLAYING;
	}

	UnlockRecursiveMutex (src_mutex);
}

/* stop the source; remove it from active array and requeue buffers */
void
mixer_SourceRewind (mixer_Object srcobj)
{
	mixer_Source *src = (mixer_Source *) srcobj;

	if (!src)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_SourceRewind() called with null source");
		return;
	}

	LockRecursiveMutex (src_mutex);

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_SourcePlay(): not a source");
	}
	else
	{
		mixer_SourceRewind_internal (src);
	}

	UnlockRecursiveMutex (src_mutex);
}

/* pause the source; keep in active array */
void
mixer_SourcePause (mixer_Object srcobj)
{
	mixer_Source *src = (mixer_Source *) srcobj;

	if (!src)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_SourcePause() called with null source");
		return;
	}

	LockRecursiveMutex (src_mutex);

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_SourcePause(): not a source");
	}
	else /* should keep all buffers and offsets */
	{
		if (src->state < MIX_PLAYING)
			mixer_SourceActivate (src);
		src->state = MIX_PAUSED;
	}

	UnlockRecursiveMutex (src_mutex);
}

/* stop the source; remove it from active array
 * and unqueue 'queued' buffers
 */
void
mixer_SourceStop (mixer_Object srcobj)
{
	mixer_Source *src = (mixer_Source *) srcobj;

	if (!src)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_SourceStop() called with null source");
		return;
	}

	LockRecursiveMutex (src_mutex);

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_SourceStop(): not a source");
	}
	else /* should remove queued buffers */
	{
		if (src->state >= MIX_PLAYING)
			mixer_SourceDeactivate (src);
		mixer_SourceStop_internal (src);
		src->state = MIX_STOPPED;
	}

	UnlockRecursiveMutex (src_mutex);
}

/* queue buffers on the source */
void
mixer_SourceQueueBuffers (mixer_Object srcobj, uint32 n,
		mixer_Object* pbufobj)
{
	uint32 i;
	mixer_Object* pobj;
	mixer_Source *src = (mixer_Source *) srcobj;

	if (!src || !pbufobj)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_SourceQueueBuffers() called "
				"with null param");
		return;
	}

	LockRecursiveMutex (buf_mutex);
	/* check to make sure we can safely queue all buffers */
	for (i = n, pobj = pbufobj; i; i--, pobj++)
	{
		mixer_Buffer *buf = (mixer_Buffer *) *pobj;
		if (!buf || !mixer_CheckBufferState (buf,
				"mixer_SourceQueueBuffers"))
		{
			break;
		}
	}
	UnlockRecursiveMutex (buf_mutex);

	if (i == 0)
	{	/* all buffers checked out */
		LockRecursiveMutex (src_mutex);
		LockRecursiveMutex (buf_mutex);

		if (src->magic != mixer_srcMagic)
		{
			mixer_SetError (MIX_INVALID_NAME);
			log_add (log_Debug, "mixer_SourceQueueBuffers(): not a source");
		}
		else
		{
			for (i = n, pobj = pbufobj; i; i--, pobj++)
			{
				mixer_Buffer *buf = (mixer_Buffer *) *pobj;

				/* add buffer to the chain */
				if (src->lastqueued)
					src->lastqueued->next = buf;
				src->lastqueued = buf;

				if (!src->firstqueued)
				{
					src->firstqueued = buf;
					src->nextqueued = buf;
					src->prevqueued = 0;
				}
				src->cqueued++;
				buf->state = MIX_BUF_QUEUED;
			}
		}

		UnlockRecursiveMutex (buf_mutex);
		UnlockRecursiveMutex (src_mutex);
	}
}

/* unqueue buffers from the source */
void
mixer_SourceUnqueueBuffers (mixer_Object srcobj, uint32 n,
		mixer_Object* pbufobj)
{
	uint32 i;
	mixer_Source *src = (mixer_Source *) srcobj;
	mixer_Buffer *curbuf = 0;

	if (!src || !pbufobj)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_SourceUnqueueBuffers() called "
				"with null source");
		return;
	}

	LockRecursiveMutex (src_mutex);

	if (src->magic != mixer_srcMagic)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_SourceUnqueueBuffers(): not a source");
	}
	else if (n > src->cqueued)
	{
		mixer_SetError (MIX_INVALID_OPERATION);
	}
	else
	{
		LockRecursiveMutex (buf_mutex);

		/* check to make sure we can unqueue all buffers */
		for (i = n, curbuf = src->firstqueued;
				i && curbuf && curbuf->state != MIX_BUF_PLAYING;
				i--, curbuf = curbuf->next)
			;

		if (i)
		{
			mixer_SetError (MIX_INVALID_OPERATION);
			log_add (log_Debug, "mixer_SourceUnqueueBuffers(): "
					"active buffer attempted");
		}
		else
		{	/* all buffers checked out */
			for (i = n; i; i--, pbufobj++)
			{
				mixer_Buffer *buf = src->firstqueued;

				/* remove buffer from the chain */
				if (src->nextqueued == buf)
					src->nextqueued = buf->next;
				if (src->prevqueued == buf)
					src->prevqueued = 0;
				if (src->lastqueued == buf)
					src->lastqueued = 0;
				src->firstqueued = buf->next;
				src->cqueued--;

				if (buf->state == MIX_BUF_PROCESSED)
					src->cprocessed--;
				
				buf->state = MIX_BUF_FILLED;
				buf->next = 0;
				*pbufobj = (mixer_Object) buf;
			}
		}

		UnlockRecursiveMutex (buf_mutex);
	}

	UnlockRecursiveMutex (src_mutex);
}

/*************************************************
 *  Sources internals
 */

static void
mixer_SourceUnqueueAll (mixer_Source *src)
{
	mixer_Buffer *buf;
	mixer_Buffer *nextbuf;

	if (!src)
	{
		log_add (log_Debug, "mixer_SourceUnqueueAll() called "
				"with null source");
		return;
	}

	LockRecursiveMutex (buf_mutex);

	for (buf = src->firstqueued; buf; buf = nextbuf)
	{
		if (buf->state == MIX_BUF_PLAYING)
		{
			log_add (log_Debug, "mixer_SourceUnqueueAll(): "
					"attempted on active buffer");
		}
		nextbuf = buf->next;
		buf->state = MIX_BUF_FILLED;
		buf->next = 0;
	}

	UnlockRecursiveMutex (buf_mutex);

	src->firstqueued = 0;
	src->nextqueued = 0;
	src->prevqueued = 0;
	src->lastqueued = 0;
	src->cqueued = 0;
	src->cprocessed = 0;
	src->pos = 0;
	src->count = 0;
}

/* add the source to the active array */
static void
mixer_SourceActivate (mixer_Source* src)
{
	uint32 i;

	LockRecursiveMutex (act_mutex);

	/* check active sources, see if this source is there already */
	for (i = 0; i < MAX_SOURCES && active_sources[i] != src; i++)
		;
	if (i < MAX_SOURCES)
	{	/* source found */
		log_add (log_Debug, "mixer_SourceActivate(): "
				"source already active in slot %u", i);
		UnlockRecursiveMutex (act_mutex);
		return;
	}

	/* find an empty slot */
	for (i = 0; i < MAX_SOURCES && active_sources[i] != 0; i++)
		;
	if (i < MAX_SOURCES)
	{	/* slot found */
		active_sources[i] = src;
	}
	else
	{
		log_add (log_Debug, "mixer_SourceActivate(): "
				"no more slots available (max=%d)", MAX_SOURCES);
	}

	UnlockRecursiveMutex (act_mutex);
}

/* remove the source from the active array */
static void
mixer_SourceDeactivate (mixer_Source* src)
{
	uint32 i;

	LockRecursiveMutex (act_mutex);

	/* check active sources, see if this source is there */
	for (i = 0; i < MAX_SOURCES && active_sources[i] != src; i++)
		;
	if (i < MAX_SOURCES)
	{	/* source found */
		active_sources[i] = 0;
	}
	else
	{	/* source not found */
		log_add (log_Debug, "mixer_SourceDeactivate(): source not active");
	}

	UnlockRecursiveMutex (act_mutex);
}

static void
mixer_SourceStop_internal (mixer_Source *src)
{
	mixer_Buffer *buf;
	mixer_Buffer *nextbuf;

	if (!src->firstqueued)
		return;

	/* assert the source buffers state */
	if (!src->lastqueued)
	{	
		log_add (log_Debug, "mixer_SourceStop_internal(): "
				"desynced source state");
#ifdef DEBUG
		explode ();
#endif
	}

	LockRecursiveMutex (buf_mutex);

	/* find last 'processed' buffer */
	for (buf = src->firstqueued;
			buf && buf->next && buf->next != src->nextqueued;
			buf = buf->next)
		;
	src->lastqueued = buf;
	if (buf)
		buf->next = 0; /* break the chain */

	/* unqueue all 'queued' buffers */
	for (buf = src->nextqueued; buf; buf = nextbuf)
	{
		nextbuf = buf->next;
		buf->state = MIX_BUF_FILLED;
		buf->next = 0;
		src->cqueued--;
	}

	if (src->cqueued == 0)
	{	/* all buffers were removed */
		src->firstqueued = 0;
		src->lastqueued = 0;
	}
	src->nextqueued = 0;
	src->prevqueued = 0;
	src->pos = 0;
	src->count = 0;

	UnlockRecursiveMutex (buf_mutex);
}

static void
mixer_SourceRewind_internal (mixer_Source *src)
{
	/* should change the processed buffers to queued */
	mixer_Buffer *buf;

	if (src->state >= MIX_PLAYING)
		mixer_SourceDeactivate (src);

	LockRecursiveMutex (buf_mutex);

	for (buf = src->firstqueued;
			buf && buf->state != MIX_BUF_QUEUED;
			buf = buf->next)
	{
		buf->state = MIX_BUF_QUEUED;
	}

	UnlockRecursiveMutex (buf_mutex);

	src->pos = 0;
	src->count = 0;
	src->cprocessed = 0;
	src->nextqueued = src->firstqueued;
	src->prevqueued = 0;
	src->state = MIX_INITIAL;
}

/* get the sample next in queue in internal format */
static inline bool
mixer_SourceGetNextSample (mixer_Source *src, float *psamp, bool left)
{
	/* fake the data if requested */
	if (mixer_flags & MIX_FAKE_DATA)
		return mixer_SourceGetFakeSample (src, psamp, left);

	while (src->nextqueued)
	{
		mixer_Buffer *buf = src->nextqueued;
		
		if (!buf->data || buf->size < mixer_sampsize)
		{
			/* buffer invalid, go next */
			buf->state = MIX_BUF_PROCESSED;
			src->pos = 0;
			src->nextqueued = src->nextqueued->next;
			src->cprocessed++;
			continue;
		}

		if (!left && buf->orgchannels == 1)
		{
			/* mono source so we can copy left channel to right */
			*psamp = src->samplecache;
		}
		else
		{
			*psamp = src->samplecache = buf->Resample(src, left) * src->gain;
		}

		if (src->pos < buf->size ||
				(left && buf->sampsize != mixer_sampsize))
		{
			buf->state = MIX_BUF_PLAYING;
		}
		else
		{
			/* buffer exhausted, go next */
			buf->state = MIX_BUF_PROCESSED;
			src->pos = 0;
			src->prevqueued = src->nextqueued;
			src->nextqueued = src->nextqueued->next;
			src->cprocessed++;
		}
		
		return true;
	}
	
	/* no more playable buffers */
	if (src->state >= MIX_PLAYING)
		mixer_SourceDeactivate (src);

	src->state = MIX_STOPPED;

	return false;
}

/* fake the next sample, but process buffers and states */
static inline bool
mixer_SourceGetFakeSample (mixer_Source *src, float *psamp, bool left)
{	
	while (src->nextqueued)
	{
		mixer_Buffer *buf = src->nextqueued;

		if (left || buf->orgchannels != 1)
		{
			if (mixer_freq == buf->orgfreq)
				src->pos += mixer_chansize;
			else
				mixer_SourceAdvance(src, left);
		}
		*psamp = 0;
		
		if (src->pos < buf->size ||
				(left && buf->sampsize != mixer_sampsize))
		{
			buf->state = MIX_BUF_PLAYING;
		}
		else
		{
			/* buffer exhausted, go next */
			buf->state = MIX_BUF_PROCESSED;
			src->pos = 0;
			src->prevqueued = src->nextqueued;
			src->nextqueued = src->nextqueued->next;
			src->cprocessed++;
		}
		
		return true;
	}
	
	/* no more playable buffers */
	if (src->state >= MIX_PLAYING)
		mixer_SourceDeactivate (src);

	src->state = MIX_STOPPED;

	return false;
}

/* advance position in currently queued buffer */
static inline uint32
mixer_SourceAdvance (mixer_Source *src, bool left)
{
	mixer_Buffer *curr = src->nextqueued;
	if (curr->orgchannels == 2 && mixer_channels == 2)
	{
		if (!left)
		{
			src->pos += curr->high;
			src->count += curr->low;
			if (src->count > UINT16_MAX)
			{
				src->count -= UINT16_MAX;
				src->pos += curr->sampsize;
			}
			return mixer_chansize;
		}
	}
	else
	{
		src->pos += curr->high;
		src->count += curr->low;
		if (src->count > UINT16_MAX)
		{
			src->count -= UINT16_MAX;
			src->pos += curr->sampsize;
		}
	}
	return 0;
}


/*************************************************
 *  Buffers interface
 */

/* generate n buffer objects */
void
mixer_GenBuffers (uint32 n, mixer_Object *pbufobj)
{
	if (n == 0)
		return; /* do nothing per OpenAL */

	if (!pbufobj)
	{
		mixer_SetError (MIX_INVALID_VALUE);
		log_add (log_Debug, "mixer_GenBuffers() called with null ptr");
		return;
	}
	for (; n; n--, pbufobj++)
	{
		mixer_Buffer *buf;

		buf = (mixer_Buffer *) HMalloc (sizeof (mixer_Buffer));
		buf->magic = mixer_bufMagic;
		buf->locked = false;
		buf->state = MIX_BUF_INITIAL;
		buf->data = 0;
		buf->size = 0;
		buf->next = 0;
		buf->orgdata = 0;
		buf->orgfreq = 0;
		buf->orgsize = 0;
		buf->orgchannels = 0;
		buf->orgchansize = 0;

		*pbufobj = (mixer_Object) buf;
	}
}

/* delete n buffer objects */
void
mixer_DeleteBuffers (uint32 n, mixer_Object *pbufobj)
{
	uint32 i;
	mixer_Object *pcurobj;

	if (n == 0)
		return; /* do nothing per OpenAL */

	if (!pbufobj)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_DeleteBuffers() called with null ptr");
		return;
	}
	
	LockRecursiveMutex (buf_mutex);

	/* check to make sure we can delete all buffers */
	for (i = n, pcurobj = pbufobj; i && pcurobj; i--, pcurobj++)
	{
		mixer_Buffer *buf = (mixer_Buffer *) *pcurobj;

		if (!buf)
			continue;

		if (buf->magic != mixer_bufMagic)
		{
			mixer_SetError (MIX_INVALID_NAME);
			log_add (log_Debug, "mixer_DeleteBuffers(): not a buffer");
			break;
		}
		else if (buf->locked)
		{
			mixer_SetError (MIX_INVALID_OPERATION);
			log_add (log_Debug, "mixer_DeleteBuffers(): locked buffer");
			break;
		}
		else if (buf->state >= MIX_BUF_QUEUED)
		{
			mixer_SetError (MIX_INVALID_OPERATION);
			log_add (log_Debug, "mixer_DeleteBuffers(): "
					"attempted on queued/active buffer");
			break;
		}
	}

	if (i == 0)
	{
		/* all buffers check out */
		for (; n; n--, pbufobj++)
		{
			mixer_Buffer *buf = (mixer_Buffer *) *pbufobj;

			if (!buf)
				continue;

			if (buf->data)
				HFree (buf->data);
			HFree (buf);

			*pbufobj = 0;
		}
	}
	UnlockRecursiveMutex (buf_mutex);
}

/* check if really a buffer object */
bool
mixer_IsBuffer (mixer_Object bufobj)
{
	mixer_Buffer *buf = (mixer_Buffer *) bufobj;
	bool ret;

	if (!buf)
		return false;

	LockRecursiveMutex (buf_mutex);
	ret = buf->magic == mixer_bufMagic;
	UnlockRecursiveMutex (buf_mutex);

	return ret;
}

/* get buffer property */
void
mixer_GetBufferi (mixer_Object bufobj, mixer_BufferProp pname,
		mixer_IntVal *value)
{
	mixer_Buffer *buf = (mixer_Buffer *) bufobj;

	if (!buf || !value)
	{
		mixer_SetError (buf ? MIX_INVALID_VALUE : MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_GetBufferi() called with null param");
		return;
	}

	LockRecursiveMutex (buf_mutex);
	
	if (buf->locked)
	{
		UnlockRecursiveMutex (buf_mutex);
		mixer_SetError (MIX_INVALID_OPERATION);
		log_add (log_Debug, "mixer_GetBufferi() called with locked buffer");
		return;
	}

	if (buf->magic != mixer_bufMagic)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_GetBufferi(): not a buffer");
	}
	else
	{
		/* Return original buffer values
		 */
		switch (pname)
		{
		case MIX_FREQUENCY:
			*value = buf->orgfreq;
			break;
		case MIX_BITS:
			*value = buf->orgchansize << 3;
			break;
		case MIX_CHANNELS:
			*value = buf->orgchannels;
			break;
		case MIX_SIZE:
			*value = buf->orgsize;
			break;
		case MIX_DATA:
			*value = (mixer_IntVal) buf->orgdata;
			break;
		default:
			mixer_SetError (MIX_INVALID_ENUM);
			log_add (log_Debug, "mixer_GetBufferi() called "
					"with invalid property %u", pname);
		}
	}

	UnlockRecursiveMutex (buf_mutex);
}

/* fill buffer with external data */
void
mixer_BufferData (mixer_Object bufobj, uint32 format, void* data,
		uint32 size, uint32 freq)
{
	mixer_Buffer *buf = (mixer_Buffer *) bufobj;
	mixer_Convertion conv;
	uint32 dstsize;

	if (!buf || !data || !size)
	{
		mixer_SetError (buf ? MIX_INVALID_VALUE : MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_BufferData() called with bad param");
		return;
	}

	LockRecursiveMutex (buf_mutex);
	
	if (buf->locked)
	{
		UnlockRecursiveMutex (buf_mutex);
		mixer_SetError (MIX_INVALID_OPERATION);
		log_add (log_Debug, "mixer_BufferData() called "
				"with locked buffer");
		return;
	}

	if (buf->magic != mixer_bufMagic)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "mixer_BufferData(): not a buffer");
	}
	else if (buf->state > MIX_BUF_FILLED)
	{
		mixer_SetError (MIX_INVALID_OPERATION);
		log_add (log_Debug, "mixer_BufferData() attempted "
				"on in-use buffer");
	}
	else
	{
		if (buf->data)
			HFree (buf->data);
		buf->data = 0;
		buf->size = 0;

		/* Store original buffer values for OpenAL compatibility */
		buf->orgdata = data;
		buf->orgfreq = freq;
		buf->orgsize = size;
		buf->orgchannels = MIX_FORMAT_CHANS (format);
		buf->orgchansize = MIX_FORMAT_BPC (format);

		conv.srcsamples = conv.dstsamples =
			size / MIX_FORMAT_SAMPSIZE (format);
				
		if (conv.dstsamples >
				UINT32_MAX / MIX_FORMAT_SAMPSIZE (format))
		{
			mixer_SetError (MIX_INVALID_VALUE);
		}
		else
		{
			if (MIX_FORMAT_CHANS (format) < MIX_FORMAT_CHANS (mixer_format))
				buf->sampsize = MIX_FORMAT_BPC (mixer_format) *
						MIX_FORMAT_CHANS (format);
			else
				buf->sampsize = MIX_FORMAT_SAMPSIZE (mixer_format);
			buf->size = dstsize = conv.dstsamples * buf->sampsize;
			
			/* only copy/convert the data if not faking */
			if (! (mixer_flags & MIX_FAKE_DATA))
			{
				buf->data = HMalloc (dstsize);
			
				if (MIX_FORMAT_BPC (format) == MIX_FORMAT_BPC (mixer_format) &&
					MIX_FORMAT_CHANS (format) <= MIX_FORMAT_CHANS (mixer_format))
				{
					/* format is compatible with internal */
					buf->locked = true;
					UnlockRecursiveMutex (buf_mutex);

					memcpy (buf->data, data, size);
					if (MIX_FORMAT_BPC (format) == 1)
					{
						/* convert buffer to S8 format internally */
						uint8* dst;
						for (dst = buf->data; dstsize; dstsize--, dst++)
							*dst ^= 0x80;
					}

					LockRecursiveMutex (buf_mutex);
					buf->locked = false;
				}
				else 
				{
					/* needs convertion */
					conv.srcfmt = format;
					conv.srcdata = data;
					conv.srcsize = size;
					
					if (MIX_FORMAT_CHANS (format) < MIX_FORMAT_CHANS (mixer_format))
						conv.dstfmt = MIX_FORMAT_MAKE (mixer_chansize,
								MIX_FORMAT_CHANS (format));
					else
						conv.dstfmt = mixer_format;
					conv.dstdata = buf->data;
					conv.dstsize = dstsize;

					buf->locked = true;
					UnlockRecursiveMutex (buf_mutex);

					mixer_ConvertBuffer_internal (&conv);
					
					LockRecursiveMutex (buf_mutex);
					buf->locked = false;
				}
			}

			buf->state = MIX_BUF_FILLED;
			buf->high = (buf->orgfreq / mixer_freq) * buf->sampsize;
			buf->low = (((buf->orgfreq % mixer_freq) << 16) / mixer_freq);
			if (mixer_freq == buf->orgfreq)
				buf->Resample = mixer_resampling.None;
			else if (mixer_freq > buf->orgfreq)
				buf->Resample = mixer_resampling.Upsample;
			else
				buf->Resample = mixer_resampling.Downsample;
		}
	}

	UnlockRecursiveMutex (buf_mutex);
}


/*************************************************
 *  Buffer internals
 */

static inline bool
mixer_CheckBufferState (mixer_Buffer *buf, const char* FuncName)
{
	if (!buf)
		return false;

	if (buf->magic != mixer_bufMagic)
	{
		mixer_SetError (MIX_INVALID_NAME);
		log_add (log_Debug, "%s(): not a buffer", FuncName);
		return false;
	}

	if (buf->locked)
	{
		mixer_SetError (MIX_INVALID_OPERATION);
		log_add (log_Debug, "%s(): locked buffer attempted", FuncName);
		return false;
	}

	if (buf->state != MIX_BUF_FILLED)
	{
		mixer_SetError (MIX_INVALID_OPERATION);
		log_add (log_Debug, "%s: invalid buffer attempted", FuncName);
		return false;
	}
	return true;
}

static void
mixer_ConvertBuffer_internal (mixer_Convertion *conv)
{
	conv->srcbpc = MIX_FORMAT_BPC (conv->srcfmt);
	conv->srcchans = MIX_FORMAT_CHANS (conv->srcfmt);
	conv->dstbpc = MIX_FORMAT_BPC (conv->dstfmt);
	conv->dstchans = MIX_FORMAT_CHANS (conv->dstfmt);

	conv->flags = 0;
	if (conv->srcbpc > conv->dstbpc)
		conv->flags |= mixConvSizeDown;
	else if (conv->srcbpc < conv->dstbpc)
		conv->flags |= mixConvSizeUp;
	if (conv->srcchans > conv->dstchans)
		conv->flags |= mixConvStereoDown;
	else if (conv->srcchans < conv->dstchans)
		conv->flags |= mixConvStereoUp;

	mixer_ResampleFlat (conv);
}

/*************************************************
 *  Resampling routines
 */

/* get a sample from external buffer
 * in internal format
 */
static inline sint32
mixer_GetSampleExt (void *src, uint32 bpc)
{
	if (bpc == 2)
		return *(sint16 *)src;
	else
		return (*(uint8 *)src) - 128;
}

/* get a sample from internal buffer */
static inline sint32
mixer_GetSampleInt (void *src, uint32 bpc)
{
	if (bpc == 2)
		return *(sint16 *)src;
	else
		return *(sint8 *)src;
}

/* put a sample into an external buffer
 * from internal format
 */
static inline void
mixer_PutSampleExt (void *dst, uint32 bpc, sint32 samp)
{
	if (bpc == 2)
		*(sint16 *)dst = samp;
	else
		*(uint8 *)dst = samp ^ 0x80;
}

/* put a sample into an internal buffer
 * in internal format
 */
static inline void
mixer_PutSampleInt (void *dst, uint32 bpc, sint32 samp)
{
	if (bpc == 2)
		*(sint16 *)dst = samp;
	else
		*(sint8 *)dst = samp;
}

/* get a sample from source */
static float
mixer_ResampleNone (mixer_Source *src, bool left)
{
	uint8 *d0 = src->nextqueued->data + src->pos;
	src->pos += mixer_chansize;
	(void) left; // satisfying compiler - unused arg
	return mixer_GetSampleInt (d0, mixer_chansize);
}

/* get a resampled (up/down) sample from source (nearest neighbor) */
static float
mixer_ResampleNearest (mixer_Source *src, bool left)
{
	uint8 *d0 = src->nextqueued->data + src->pos;
	d0 += mixer_SourceAdvance (src, left);
	return mixer_GetSampleInt (d0, mixer_chansize);
}

/* get an upsampled sample from source (linear interpolation) */
static float
mixer_UpsampleLinear (mixer_Source *src, bool left)
{
	mixer_Buffer *curr = src->nextqueued;
	mixer_Buffer *next = src->nextqueued->next;
	uint8 *d0, *d1;
	float s0, s1, t;
	
	t = src->count / 65536.0f;
	d0 = curr->data + src->pos;
	d0 += mixer_SourceAdvance (src, left);

	if (d0 + curr->sampsize >= curr->data + curr->size)
	{
		if (next && next->data && next->size >= curr->sampsize)
		{
			d1 = next->data;
			if (!left)
				d1 += mixer_chansize;
		}
		else
			d1 = d0;
	}
	else
		d1 = d0 + curr->sampsize;

	s0 = mixer_GetSampleInt (d0, mixer_chansize);
	s1 = mixer_GetSampleInt (d1, mixer_chansize);
	return s0 + t * (s1 - s0);
}

/* get an upsampled sample from source (cubic interpolation) */
static float
mixer_UpsampleCubic (mixer_Source *src, bool left)
{
	mixer_Buffer *prev = src->prevqueued;
	mixer_Buffer *curr = src->nextqueued;
	mixer_Buffer *next = src->nextqueued->next;
	uint8 *d0, *d1, *d2, *d3; /* prev, curr, next, next + 1 */
	float t, t2, a, b, c, s0, s1, s2, s3;

	t = src->count / 65536.0f;
	t2 = t * t;
	d1 = curr->data + src->pos;	
	d1 += mixer_SourceAdvance (src, left);

	if (d1 - curr->sampsize < curr->data)
	{
		if (prev && prev->data && prev->size >= curr->sampsize)
		{
			d0 = prev->data + prev->size - curr->sampsize;
			if (!left)
				d0 += mixer_chansize;
		}
		else
			d0 = d1;
	}
	else
		d0 = d1 - curr->sampsize;

	if (d1 + curr->sampsize >= curr->data + curr->size)
	{
		if (next && next->data && next->size >= curr->sampsize * 2)
		{
			d2 = next->data;
			if (!left)
				d2 += mixer_chansize;
			d3 = d2 + curr->sampsize;
		}
		else
			d2 = d3 = d1;
	}
	else
	{
		d2 = d1 + curr->sampsize;
		if (d2 + curr->sampsize >= curr->data + curr->size)
		{
			if (next && next->data && next->size >= curr->sampsize)
			{
				d3 = next->data;
				if (!left)
					d3 += mixer_chansize;
			}
			else
				d3 = d2;
		}
		else
			d3 = d2 + curr->sampsize;
	}

	s0 = mixer_GetSampleInt (d0, mixer_chansize);
	s1 = mixer_GetSampleInt (d1, mixer_chansize);
	s2 = mixer_GetSampleInt (d2, mixer_chansize);
	s3 = mixer_GetSampleInt (d3, mixer_chansize);

	a = (3.0f * (s1 - s2) - s0 + s3) * 0.5f;
	b = 2.0f * s2 + s0 - ((5.0f * s1 + s3) * 0.5f);
	c = (s2 - s0) * 0.5f;
	
	return a * t2 * t + b * t2 + c * t + s1;
}

/* get next sample from external buffer
 * in internal format, while performing
 * convertion if necessary
 */
static inline sint32
mixer_GetConvSample (uint8 **psrc, uint32 bpc, uint32 flags)
{
	sint32 samp;
	
	samp = mixer_GetSampleExt (*psrc, bpc);
	*psrc += bpc;
	if (flags & mixConvStereoDown)
	{
		/* downmix to mono - average up channels */
		samp = (samp + mixer_GetSampleExt (*psrc, bpc)) / 2;
		*psrc += bpc;
	}

	if (flags & mixConvSizeUp)
	{
		/* convert S8 to S16 */
		samp <<= 8;
	}
	else if (flags & mixConvSizeDown)
	{
		/* convert S16 to S8
		 * if arithmetic shift is available to the compiler
		 * it will use it to optimize this
		 */
		samp /= 0x100;
	}

	return samp;
}

/* put next sample into an internal buffer
 * in internal format, while performing
 * convertion if necessary
 */
static inline void
mixer_PutConvSample (uint8 **pdst, uint32 bpc, uint32 flags, sint32 samp)
{
	mixer_PutSampleInt (*pdst, bpc, samp);
	*pdst += bpc;
	if (flags & mixConvStereoUp)
	{
		mixer_PutSampleInt (*pdst, bpc, samp);
		*pdst += bpc;
	}
}

/* resampling with respect to sample size only */
static void
mixer_ResampleFlat (mixer_Convertion *conv)
{
	mixer_ConvFlags flags = conv->flags;
	uint8 *src = conv->srcdata;
	uint8 *dst = conv->dstdata;
	uint32 srcbpc = conv->srcbpc;
	uint32 dstbpc = conv->dstbpc;
	uint32 samples;

	samples = conv->srcsamples;
	if ( !(conv->flags & (mixConvStereoUp | mixConvStereoDown)))
		samples *= conv->srcchans;

	for (; samples; samples--)
	{
		sint32 samp;

		samp = mixer_GetConvSample (&src, srcbpc, flags);
		mixer_PutConvSample (&dst, dstbpc, flags, samp);
	}
}
