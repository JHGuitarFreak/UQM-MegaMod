//Copyright Paul Reiche, Fred Ford. 1992-2002

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

#include <stdio.h>

#include "cons_res.h"
#include "resinst.h"
#include "nameref.h"
#include "setup.h"
#include "units.h"
		// for NUM_VIEWS
#include "planets/planets.h"
		// for NUMBER_OF_PLANET_TYPES, PLANET_SHIELDED

static const char *planet_types[] = {
	"oolite", "yttric", "quasidegenerate", "lanthanide", "treasure",
	"urea", "metal", "radioactive", "opalescent", "cyanic",
	"acid", "alkali", "halide", "green", "copper",
	"carbide", "ultramarine", "noble", "azure", "chondrite",
	"purple", "superdense", "pellucid", "dust", "crimson",
	"cimmerian", "infrared", "selenic", "auric", "fluorescent",
	"ultraviolet", "plutonic", "rainbow", "shattered", "sapphire",
	"organic", "xenolithic", "redux", "primordial", "emerald",
	"chlorine", "magnetic", "water", "telluric", "hydrocarbon",
	"iodine", "vinylogous", "ruby", "magma", "maroon",
	"bluegas", "cyangas", "greengas", "greygas", "orangegas",
	"purplegas", "redgas", "violetgas", "yellowgas"
};

static const char *planet_sizes[] = {
	"large", "medium", "small"
}; 

FRAME planet[NUM_VIEWS];
static char buffer[80];

void
load_gravity_well (BYTE selector)
{
	COUNT i;

	if (selector == NUMBER_OF_PLANET_TYPES)
	{
		planet[0] = CaptureDrawable (
				LoadGraphic (SAMATRA_BIG_MASK_PMAP_ANIM)
				);
		planet[1] = planet[2] = 0;
	}
	else
	{
		const char *ptype;
		if (selector & PLANET_SHIELDED)
		{
			ptype = "slaveshield";
		}
		else
		{
			ptype = planet_types[selector];
		}

		for (i = 0; i < NUM_VIEWS; ++i)
		{
			snprintf (buffer, 79, "planet.%s.%s", ptype, planet_sizes[i]);
			buffer[79] = '\0';
			planet[i] = CaptureDrawable (LoadGraphic (buffer));
		}
	}

}

void
free_gravity_well (void)
{
	COUNT i;

	for (i = 0; i < NUM_VIEWS; ++i)
	{
		DestroyDrawable (ReleaseDrawable (planet[i]));
		planet[i] = 0;
	}
}

FRAME
load_life_form (BYTE selector)
{
	snprintf (buffer, 79, "graphics.life.%d", selector);
	buffer[79] = '\0'; /* Shouldn't be necessary, but better safe than sorry */
	return CaptureDrawable (LoadGraphic (buffer));
}

MUSIC_REF
load_orbit_theme (BYTE selector)
{
	snprintf (buffer, 79, "music.orbit%d", selector + 1);
	buffer[79] = '\0'; /* Shouldn't be necessary, but better safe than sorry */	
	return LoadMusic (buffer);
}

MUSIC_REF
loadMainMenuMusic (BYTE selector)
{
	snprintf (buffer, 79, "music.mainmenu%d", selector + 1);
	buffer[79] = '\0'; /* Shouldn't be necessary, but better safe than sorry */	
	return LoadMusic (buffer);
}
