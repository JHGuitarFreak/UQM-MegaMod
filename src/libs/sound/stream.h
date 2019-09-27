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

#ifndef STREAM_H
#define STREAM_H

int InitStreamDecoder (void);
void UninitStreamDecoder (void);

void PlayStream (TFB_SoundSample *sample, uint32 source, bool looping, 
				 bool scope, bool rewind);
void StopStream (uint32 source);
void PauseStream (uint32 source);
void ResumeStream (uint32 source);
void SeekStream (uint32 source, uint32 pos);
BOOLEAN PlayingStream (uint32 source);

int GraphForegroundStream (uint8 *data, sint32 width, sint32 height,
		bool wantSpeech);

// returns TRUE if the fade was accepted by stream decoder
bool SetMusicStreamFade (sint32 howLong, int endVolume);

#endif
