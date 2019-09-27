// Copyright 2008 Michael Martin

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

#ifndef VIDINTERN_H_
#define VIDINTERN_H_

#include "types.h"
#include "libs/vidlib.h"
#include "libs/threadlib.h"

struct legacy_video_desc
{
	char *video, *audio, *speech;
	uint32 loop;
};

typedef struct legacy_video_desc LEGACY_VIDEO_DESC;

struct legacy_video_ref
{
	VIDEO_REF vidref;
	MUSIC_REF audref;
	MUSIC_REF speechref;
};

#endif
