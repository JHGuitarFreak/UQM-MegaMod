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

#ifndef DCQUEUE_H
#define DCQUEUE_H

// Maximum size of the DCQ.  The larger the DCQ, the larger frameskips
// become tolerable before initiating livelock deterrence and game
// slowdown.  Other constants for controlling the frameskip/slowdown
// balance may be found in sdl_common.c near TFB_FlushGraphics.

// Livelock deterrance constants.  Because the entire screen is rarely
// refreshed, we may not drop draw commands on the floor with abandon.
// Furthermore, if the main program is queuing commands at a speed
// comparable to our processing of the commands, we never finish and
// the game freezes.  Thus, if the queue starts out larger than
// DCQ_FORCE_SLOWDOWN_SIZE, or DCQ_LIVELOCK_MAX commands find
// themselves being processed in one go, livelock deterrence is
// enabled, and TFB_FlushGraphics locks the DCQ until it has processed
// all entries.  If batched but pending commands exceed DCQ_FORCE_BREAK_SIZE,
// a continuity break is performed.  This will effectively slow down the 
// game logic, a fate we seek to avoid - however, it seems to be unavoidable
// on slower machines.  Even there, it's seems nonexistent outside of
// communications screens.  --Michael

#ifdef DCQ_OF_DOOM
#define DCQ_MAX 512
#define DCQ_FORCE_SLOWDOWN_SIZE 128
#define DCQ_FORCE_BREAK_SIZE 512
#define DCQ_LIVELOCK_MAX 256
#else
#define DCQ_MAX 16384
#define DCQ_FORCE_SLOWDOWN_SIZE 4096
#define DCQ_FORCE_BREAK_SIZE 16384
#define DCQ_LIVELOCK_MAX 4096
#endif

extern CondVar RenderingCond;

#endif


