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

#include "sound.h"
#include "sndintrn.h"
#include "libs/sound/trackplayer.h"
#include "trackint.h"
#include "libs/log.h"
#include "libs/memlib.h"
#include "options.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>


static int track_count;       // total number of tracks
static int no_page_break;     // set when combining several tracks into one

// The one and only sample we play. Track switching is done by modifying
// this sample while it is playing. StreamDecoderTaskFunc() picks up the
// changes *mostly* seamlessly (keyword: mostly).
// This is technically a hack, but a decent one ;)
static TFB_SoundSample *sound_sample;

static volatile uint32 tracks_length; // total length of tracks in game units

static TFB_SoundChunk *chunks_head;   // first decoder in linked list
static TFB_SoundChunk *chunks_tail;   // last decoder in linked list
static TFB_SoundChunk *last_sub;      // last chunk in the list with a subtitle

static TFB_SoundChunk *cur_chunk;     // currently playing chunk
static TFB_SoundChunk *cur_sub_chunk; // currently displayed subtitle chunk

// Accesses to cur_chunk and cur_sub_chunk are guarded by stream_mutex,
// because these should only be accesses by the DoInput and the
// stream player threads. Any other accesses would go unguarded.
// Other data structures are unguarded and should only be accessed from
// the DoInput thread at certain times, i.e. nothing can be modified
// between StartTrack() and JumpTrack()/StopTrack() calls.
// Use caution when changing code, as you may need to guard other data
// structures the same way.

static void seek_track (sint32 offset);

// stream callbacks
static bool OnStreamStart (TFB_SoundSample* sample);
static bool OnChunkEnd (TFB_SoundSample* sample, audio_Object buffer);
static void OnStreamEnd (TFB_SoundSample* sample);
static void OnBufferTag (TFB_SoundSample* sample, TFB_SoundTag* tag);

static TFB_SoundCallbacks trackCBs =
{
	OnStreamStart,
	OnChunkEnd,
	OnStreamEnd,
	OnBufferTag,
	NULL
};

static inline sint32
chunk_end_time (TFB_SoundChunk *chunk)
{
	return (sint32) ((chunk->start_time + chunk->decoder->length)
			* ONE_SECOND);
}

static inline sint32
tracks_end_time (void)
{
	return chunk_end_time (chunks_tail);
}

//JumpTrack currently aborts the current track. However, it doesn't clear the
//data-structures as StopTrack does.  this allows for rewind even after the
//track has finished playing
//Question:  Should 'abort' call StopTrack?
void
JumpTrack (void)
{
	if (!sound_sample)
		return; // nothing to skip

	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	seek_track (tracks_length + 1);
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
}

// This should just start playing a stream
void
PlayTrack (void)
{
	if (!sound_sample)
		return; // nothing to play

	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	tracks_length = tracks_end_time ();
	// decoder will be set in OnStreamStart()
	cur_chunk = chunks_head;
	// Always scope the speech data, we may need it
	PlayStream (sound_sample, SPEECH_SOURCE, false, true, true);
 	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
}

void
PauseTrack (void)
{
	if (!sound_sample)
		return; // nothing to pause

	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	PauseStream (SPEECH_SOURCE);
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
}

// ResumeTrack should resume a paused track, and do nothing for a playing track
void
ResumeTrack (void)
{
	audio_IntVal state;

	if (!sound_sample)
		return; // nothing to resume

	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);

	if (!cur_chunk)
	{	// not playing anything, so no resuming
		UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
		return;
	}

	audio_GetSourcei (soundSource[SPEECH_SOURCE].handle, audio_SOURCE_STATE, &state);
	if (state == audio_PAUSED)
		ResumeStream (SPEECH_SOURCE);

	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
}

COUNT
PlayingTrack (void)
{
	// This ignores the paused state and simply returns what track
	// *should* be playing
	COUNT result = 0;  // default is none

	if (!sound_sample)
		return 0; // not playing anything

	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	if (cur_chunk)
		result = cur_chunk->track_num + 1;
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);

	return result;
}

void
StopTrack (void)
{
	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	StopStream (SPEECH_SOURCE);
	track_count = 0;
	tracks_length = 0;
	cur_chunk = NULL;
	cur_sub_chunk = NULL;
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);

	if (chunks_head)
	{
		chunks_tail = NULL;
		destroy_SoundChunk_list (chunks_head);
		chunks_head = NULL;
		last_sub = NULL;
	}
	if (sound_sample)
	{
		// We delete the decoders ourselves
		sound_sample->decoder = NULL;
		TFB_DestroySoundSample (sound_sample);
		sound_sample = NULL;
	}
}

static void
DoTrackTag (TFB_SoundChunk *chunk)
{
	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	if (chunk->callback)
		chunk->callback(0);
	
	cur_sub_chunk = chunk;
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
}

// This func is called by PlayStream() when stream is about
// to start. We have a chance to tweak the stream here.
// This is called on the DoInput thread.
static bool
OnStreamStart (TFB_SoundSample* sample)
{
	if (sample != sound_sample)
		return false; // Huh? Why did we get called on this?

	if (!cur_chunk)
		return false; // Stream shouldn't be playing at all

	// Adjust the sample to play what we want
	sample->decoder = cur_chunk->decoder;
	sample->offset = (sint32) (cur_chunk->start_time * ONE_SECOND);

	if (cur_chunk->tag_me)
		DoTrackTag (cur_chunk);

	return true;
}

// This func is called by StreamDecoderTaskFunc() when the last buffer
// of the current chunk has been decoded (not when it has been *played*).
// This is called on the stream task thread.
static bool
OnChunkEnd (TFB_SoundSample* sample, audio_Object buffer)
{
	if (sample != sound_sample)
		return false; // Huh? Why did we get called on this?

	if (!cur_chunk || !cur_chunk->next)
	{	// all chunks and tracks are done
		return false;
	}

	// Move on to the next chunk
	cur_chunk = cur_chunk->next;
	// Adjust the sample to play what we want
	sample->decoder = cur_chunk->decoder;
	SoundDecoder_Rewind (sample->decoder);

	log_add (log_Info, "Switching to stream %s at pos %d",
			sample->decoder->filename, sample->decoder->start_sample);
	
	if (cur_chunk->tag_me)
	{	// Tag the last buffer of the chunk with the next chunk
		TFB_TagBuffer (sample, buffer, (intptr_t)cur_chunk);
	}

	return true;
}

// This func is called by StreamDecoderTaskFunc() when stream has ended
// This is called on the stream task thread.
static void
OnStreamEnd (TFB_SoundSample* sample)
{
	if (sample != sound_sample)
		return; // Huh? Why did we get called on this?

	cur_chunk = NULL;
	cur_sub_chunk = NULL;
}

// This func is called by StreamDecoderTaskFunc() when a tagged buffer
// has finished playing.
// This is called on the stream task thread.
static void
OnBufferTag (TFB_SoundSample* sample, TFB_SoundTag* tag)
{
	TFB_SoundChunk* chunk = (TFB_SoundChunk*) tag->data;

	assert (sizeof (tag->data) >= sizeof (chunk));

	if (sample != sound_sample)
		return; // Huh? Why did we get called on this?

	TFB_ClearBufferTag (tag);
	DoTrackTag (chunk);
}

// Parse the timestamps string into an int array.
// Rerturns number of timestamps parsed.
static int
GetTimeStamps (UNICODE *TimeStamps, sint32 *time_stamps)
{
	int pos;
	int num = 0;
	
	while (*TimeStamps && (pos = strcspn (TimeStamps, ",\r\n")))
	{
		UNICODE valStr[32];
		uint32 val;
		
		memcpy (valStr, TimeStamps, pos);
		valStr[pos] = '\0';
		val = strtoul (valStr, NULL, 10);
		if (val)
		{
			*time_stamps = val;
			num++;
			time_stamps++;
		}
		TimeStamps += pos;
		TimeStamps += strspn (TimeStamps, ",\r\n");
	}
	return (num);
}

#define TEXT_SPEED 80
// Returns number of parsed pages
static int
SplitSubPages (UNICODE *text, UNICODE *pages[], sint32 timestamp[], int size)
{
	int lead_ellips = 0;
	COUNT page;
	
	for (page = 0; page < size && *text != '\0'; ++page)
	{
		int aft_ellips;
		int pos;

		// seek to EOL or end of the string
		pos = strcspn (text, "\r\n");
		// XXX: this will only work when ASCII punctuation and spaces
		//   are used exclusively
		aft_ellips = 3 * (text[pos] != '\0' && pos > 0 &&
				!ispunct (text[pos - 1]) && !isspace (text[pos - 1]));
		pages[page] = HMalloc (sizeof (UNICODE) *
				(lead_ellips + pos + aft_ellips + 1));
		if (lead_ellips)
			strcpy (pages[page], "..");
		memcpy (pages[page] + lead_ellips, text, pos);
		pages[page][lead_ellips + pos] = '\0'; // string term
		if (aft_ellips)
			strcpy (pages[page] + lead_ellips + pos, "...");
		timestamp[page] = pos * TEXT_SPEED;
		if (timestamp[page] < 1000)
			timestamp[page] = 1000;
		lead_ellips = aft_ellips ? 2 : 0;
		text += pos;
		// Skip any EOL
		text += strspn (text, "\r\n");
	}

	return page;
}

// decodes several tracks into one and adds it to queue
// track list is NULL-terminated
// May only be called after at least one SpliceTrack(). This is a limitation
// for the sake of timestamps, but it does not have to be so.
void
SpliceMultiTrack (UNICODE *TrackNames[], UNICODE *TrackText)
{
#define MAX_MULTI_TRACKS  20
#define MAX_MULTI_BUFFERS 100
	TFB_SoundDecoder* track_decs[MAX_MULTI_TRACKS + 1];
	int tracks;
	int slen1, slen2;

	if (!TrackText)
	{
		log_add (log_Debug, "SpliceMultiTrack(): no track text");
		return;
	}

	if (!sound_sample || !chunks_tail)
	{
		log_add (log_Warning, "SpliceMultiTrack(): Cannot be called before SpliceTrack()");
		return;
	}

	log_add (log_Info, "SpliceMultiTrack(): loading...");
	for (tracks = 0; *TrackNames && tracks < MAX_MULTI_TRACKS; TrackNames++, tracks++)
	{
		track_decs[tracks] = SoundDecoder_Load (contentDir, *TrackNames,
				32768, 0, - 3 * TEXT_SPEED);
		if (track_decs[tracks])
		{
			log_add (log_Info, "  track: %s, decoder: %s, rate %d format %x",
					*TrackNames,
					SoundDecoder_GetName (track_decs[tracks]),
					track_decs[tracks]->frequency,
					track_decs[tracks]->format);
			SoundDecoder_DecodeAll (track_decs[tracks]);

			chunks_tail->next = create_SoundChunk (track_decs[tracks], sound_sample->length);
			chunks_tail = chunks_tail->next;
			chunks_tail->track_num = track_count - 1;
			sound_sample->length += track_decs[tracks]->length;
		}
		else
		{
			log_add (log_Warning, "SpliceMultiTrack(): couldn't load %s\n",
					*TrackNames);
			tracks--;
		}
	}
	track_decs[tracks] = 0; // term

	if (tracks == 0)
	{
		log_add (log_Warning, "SpliceMultiTrack(): no tracks loaded");
		return;
	}

	slen1 = strlen (last_sub->text);
	slen2 = strlen (TrackText);
	last_sub->text = HRealloc (last_sub->text, slen1 + slen2 + 1);
	strcpy (last_sub->text + slen1, TrackText);

	no_page_break = 1;
}

// XXX: This code and the entire trackplayer are begging to be overhauled
void
SpliceTrack (UNICODE *TrackName, UNICODE *TrackText, UNICODE *TimeStamp, CallbackFunction cb)
{
	static UNICODE last_track_name[128] = "";
	static unsigned long dec_offset = 0;
#define MAX_PAGES 50
	UNICODE *pages[MAX_PAGES];
	sint32 time_stamps[MAX_PAGES];
	int num_pages;
	int page;

	if (!TrackText)
		return;

	if (!TrackName)
	{	// Appending a piece of subtitles to the last track
		int slen1, slen2;

		if (track_count == 0)
		{
			log_add (log_Warning, "SpliceTrack(): Tried to append a subtitle,"
					" but no current track");
			return;
		}

		if (!last_sub || !last_sub->text)
		{
			log_add (log_Warning, "SpliceTrack(): Tried to append a subtitle"
					" to a NULL string");
			return;
		}
		
		num_pages = SplitSubPages (TrackText, pages, time_stamps, MAX_PAGES);
		if (num_pages == 0)
		{
			log_add (log_Warning, "SpliceTrack(): Failed to parse subtitles");
			return;
		}
		// The last page's stamp is a suggested value. The track should
		// actually play to the end.
		time_stamps[num_pages - 1] = -time_stamps[num_pages - 1];

		// Add the first piece to the last subtitle page
		slen1 = strlen (last_sub->text);
		slen2 = strlen (pages[0]);
		last_sub->text = HRealloc (last_sub->text, slen1 + slen2 + 1);
		strcpy (last_sub->text + slen1, pages[0]);
		HFree (pages[0]);
		
		// Add the rest of the pages
		for (page = 1; page < num_pages; ++page)
		{
			TFB_SoundChunk *next_sub = find_next_page (last_sub);
			if (next_sub)
			{	// nodes prepared by previous call, just fill in the subs
				next_sub->text = pages[page];
				last_sub = next_sub;
			}
			else
			{	// probably no timestamps were provided, so need more work
				TFB_SoundDecoder *decoder = SoundDecoder_Load (contentDir,
						last_track_name, 4096, dec_offset, time_stamps[page]);
				if (!decoder)
				{
					log_add (log_Warning, "SpliceTrack(): couldn't load %s", TrackName);
					break;
				}
				dec_offset += (unsigned long)(decoder->length * 1000);
				chunks_tail->next = create_SoundChunk (decoder, sound_sample->length);
				chunks_tail = chunks_tail->next;
				chunks_tail->tag_me = 1;
				chunks_tail->track_num = track_count - 1;
				chunks_tail->text = pages[page];
				chunks_tail->callback = cb;
				// TODO: We may have to tag only one page with a callback
				//cb = NULL;
				last_sub = chunks_tail;
				sound_sample->length += decoder->length;
			}
		}
	}
	else
	{	// Adding a new track
		int num_timestamps = 0;

		utf8StringCopy (last_track_name, sizeof (last_track_name), TrackName);

		num_pages = SplitSubPages (TrackText, pages, time_stamps, MAX_PAGES);
		if (num_pages == 0)
		{
			log_add (log_Warning, "SpliceTrack(): Failed to parse sutitles");
			return;
		}
		// The last page's stamp is a suggested value. The track should
		// actually play to the end.
		time_stamps[num_pages - 1] = -time_stamps[num_pages - 1];

		if (no_page_break && track_count)
		{
			int slen1, slen2;

			slen1 = strlen (last_sub->text);
			slen2 = strlen (pages[0]);
			last_sub->text = HRealloc (last_sub->text, slen1 + slen2 + 1);
			strcpy (last_sub->text + slen1, pages[0]);
			HFree (pages[0]);
		}
		else
			track_count++;

		log_add (log_Info, "SpliceTrack(): loading %s", TrackName);

		if (TimeStamp)
		{
			num_timestamps = GetTimeStamps (TimeStamp, time_stamps) + 1;
			if (num_timestamps < num_pages)
			{
				log_add (log_Warning, "SpliceTrack(): number of timestamps"
						" doesn't match number of pages!");
			}
			else if (num_timestamps > num_pages)
			{	// We most likely will get more subtitles appended later
				// Set the last page to the rest of the track
				time_stamps[num_timestamps - 1] = -100000;
			}
		}
		else
		{
			num_timestamps = num_pages;
		}
		
		// Reset the offset for the new track
		dec_offset = 0;
		for (page = 0; page < num_timestamps; ++page)
		{
			TFB_SoundDecoder *decoder = SoundDecoder_Load (contentDir,
					TrackName, 4096, dec_offset, time_stamps[page]);
			if (!decoder)
			{
				log_add (log_Warning, "SpliceTrack(): couldn't load %s", TrackName);
				break;
			}

			if (!sound_sample)
			{
				sound_sample = TFB_CreateSoundSample (NULL, 8, &trackCBs);
				chunks_head = create_SoundChunk (decoder, 0.0);
				chunks_tail = chunks_head;
			}
			else
			{
				chunks_tail->next = create_SoundChunk (decoder, sound_sample->length);
				chunks_tail = chunks_tail->next;
			}
			dec_offset += (unsigned long)(decoder->length * 1000);
#if 0
			log_add (log_Debug, "page (%d of %d): %d ts: %d",
					page, num_pages,
					dec_offset, time_stamps[page]);
#endif
			sound_sample->length += decoder->length;
			chunks_tail->track_num = track_count - 1;
			if (!no_page_break)
			{
				chunks_tail->tag_me = 1;
				if (page < num_pages)
				{
					chunks_tail->text = pages[page];
					last_sub = chunks_tail;
				}
				chunks_tail->callback = cb;
				// TODO: We may have to tag only one page with a callback
				//cb = NULL;
			}
			no_page_break = 0;
		}
	}
}

// This function figures out the chunk that should be playing based on
// 'offset' into the total playing time of all tracks. It then sets
// the speech source's sample to the necessary decoder and seeks the
// decoder to the proper point.
// XXX: This means that whatever speech has already been queued on the
//   source will continue playing, so we may need some small timing
//   adjustments. It may be simpler to just call PlayStream().
static void
seek_track (sint32 offset)
{
	TFB_SoundChunk *cur;
	TFB_SoundChunk *last_tag = NULL;

	if (!sound_sample)
		return; // nothing to recompute

	if (offset < 0)
		offset = 0;
	else if ((uint32)offset > tracks_length)
		offset = tracks_length + 1;

	// Adjusting the stream start time is the only way we can arbitrarily
	// seek the stream right now
	soundSource[SPEECH_SOURCE].start_time = GetTimeCounter () - offset;
	
	// Find the chunk that should be playing at this time offset
	for (cur = chunks_head; cur && offset >= chunk_end_time (cur);
			cur = cur->next)
	{
		// .. looking for the last callback as we go along
		// XXX: this effectively set the last point where Fot is looking at.
		// TODO: this should be somehow changed if we implement more
		//   callbacks, like Melnorme trading, offloading at Starbase, etc.
		if (cur->tag_me)
			last_tag = cur;
	}

	if (cur)
	{
		cur_chunk = cur;
		SoundDecoder_Seek (cur->decoder, (uint32) (((float)offset / ONE_SECOND
				- cur->start_time) * 1000));
		sound_sample->decoder = cur->decoder;
		
		if (cur->tag_me)
			last_tag = cur;
		if (last_tag)
			DoTrackTag (last_tag);
	}
	else
	{	// The offset is beyond the length of all tracks
		StopStream (SPEECH_SOURCE);
		cur_chunk = NULL;
		cur_sub_chunk = NULL;
	}
}

static sint32
get_current_track_pos (void)
{
	sint32 start_time = soundSource[SPEECH_SOURCE].start_time;
	sint32 pos = GetTimeCounter () - start_time;
	if (pos < 0)
		pos = 0;
	else if ((uint32)pos > tracks_length)
		pos = tracks_length;
	return pos;
}

void
FastReverse_Smooth (void)
{
	sint32 offset;

	if (!sound_sample)
		return; // nothing is playing, so.. bye!

	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	
	offset = get_current_track_pos ();
	offset -= ACCEL_SCROLL_SPEED;
	seek_track (offset);

	// Restart the stream in case it ended previously
	if (!PlayingStream (SPEECH_SOURCE))
		PlayStream (sound_sample, SPEECH_SOURCE, false, true, false);
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
}

void
FastForward_Smooth (void)
{
	sint32 offset;

	if (!sound_sample)
		return; // nothing is playing, so.. bye!

	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);

	offset = get_current_track_pos ();
	offset += ACCEL_SCROLL_SPEED;
	seek_track (offset);

	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
}

void
FastReverse_Page (void)
{
	TFB_SoundChunk *prev;

	if (!sound_sample)
		return; // nothing is playing, so.. bye!

	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	prev = find_prev_page (cur_sub_chunk);
	if (prev)
	{	// Set the chunk to be played
		cur_chunk = prev;
		cur_sub_chunk = prev;
		// Decoder will be set in OnStreamStart()
		PlayStream (sound_sample, SPEECH_SOURCE, false, true, true);
	}
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
}

void
FastForward_Page (void)
{
	TFB_SoundChunk *next;

	if (!sound_sample)
		return; // nothing is playing, so.. bye!

	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	next = find_next_page (cur_sub_chunk);
	if (next)
	{	// Set the chunk to be played
		cur_chunk = next;
		cur_sub_chunk = next;
		// Decoder will be set in OnStreamStart()
		PlayStream (sound_sample, SPEECH_SOURCE, false, true, true);
	}
	else
	{	// End of the tracks (pun intended)
		seek_track (tracks_length + 1);
	}
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
}

// Tells current position of streaming speech in the units
// specified by the caller.
// This is normally called on the ambient_anim_task thread.
int
GetTrackPosition (int in_units)
{	
	uint32 offset;
	uint32 length = tracks_length;
			// detach from the static one, otherwise, we can race for
			// it and thus divide by 0

	if (!sound_sample || length == 0)
		return 0; // nothing is playing

	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	offset = get_current_track_pos ();
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	
	return in_units * offset / length;
}

TFB_SoundChunk *
create_SoundChunk (TFB_SoundDecoder *decoder, float start_time)
{
	TFB_SoundChunk *chunk;
	chunk = HCalloc (sizeof (*chunk));
	chunk->decoder = decoder;
	chunk->start_time = start_time;
	return chunk;
}

void
destroy_SoundChunk_list (TFB_SoundChunk *chunk)
{
	TFB_SoundChunk *next = NULL;
	for ( ; chunk; chunk = next)
	{
		next = chunk->next;
		if (chunk->decoder)
			SoundDecoder_Free (chunk->decoder);
		HFree (chunk->text);
		HFree (chunk);
	}
}

// Returns the next chunk with a subtitle
TFB_SoundChunk *
find_next_page (TFB_SoundChunk *cur)
{
	if (!cur)
		return NULL;
	for (cur = cur->next; cur && !cur->tag_me; cur = cur->next)
		;
	return cur;
}

// Returns the previous chunk with a subtitle.
// cur == 0 is treated as end of the list.
TFB_SoundChunk *
find_prev_page (TFB_SoundChunk *cur)
{
	TFB_SoundChunk *prev;
	TFB_SoundChunk *last_valid = chunks_head;
	
	if (cur == chunks_head)
		return cur; // cannot go below the first track

	for (prev = chunks_head; prev && prev != cur; prev = prev->next)
	{
		if (prev->tag_me)
			last_valid = prev;
	}
	return last_valid;
}


// External access to the chunks list
SUBTITLE_REF
GetFirstTrackSubtitle (void)
{
	return chunks_head;
}

// External access to the chunks list
SUBTITLE_REF
GetNextTrackSubtitle (SUBTITLE_REF LastRef)
{
	if (!LastRef)
		return NULL; // enumeration already ended

	return find_next_page (LastRef);
}

// External access to the chunk subtitles
const UNICODE *
GetTrackSubtitleText (SUBTITLE_REF SubRef)
{
	if (!SubRef)
		return NULL;

	return SubRef->text;
}

// External access to currently active subtitle text
// Returns NULL is none is active
const UNICODE *
GetTrackSubtitle (void)
{
	const UNICODE *cur_sub = NULL;

	if (!sound_sample)
		return NULL; // not playing anything

	LockMutex (soundSource[SPEECH_SOURCE].stream_mutex);
	if (cur_sub_chunk)
		cur_sub = cur_sub_chunk->text;
	UnlockMutex (soundSource[SPEECH_SOURCE].stream_mutex);

	return cur_sub;
}
