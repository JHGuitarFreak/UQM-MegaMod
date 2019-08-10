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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * This code is adapted from IMG_png.c in the SDL2_image library, which is
 * available under the zlib license and is (c) 1997-2019 Sam Lantinga. The
 * code itself is credited to Philippe Lavoie as the original author. It
 * also shares some heritage with libpng's own example.c file, and
 * ultimately inherits the GPL2+ license from the rest of UQM.
 *
 * Differences from SDL2_image are:
 *  - It is PNG-only
 *  - It directly links the relevant version of libpng at compile time
 *  - It always uses libpng and will never forward to alternative
 *    libraries such as ImageIO.framework. This means that palette
 *    information will always be preserved, as UQM requires.
 *  - It locks the surface as the API demands rather than using
 *    volatility markers
 *  - Palette assignment is done through the API rather than by
 *    directly editing the format contents
 */

#include "png2sdl.h"
#include <png.h>

/* Link function between SDL_RWops and PNG's data source */
static void
png_read_data(png_structp ctx, png_bytep area, png_size_t size)
{
	SDL_RWops *src = (SDL_RWops *)png_get_io_ptr (ctx);
	SDL_RWread (src, area, size, 1);
}

SDL_Surface *
TFB_png_to_sdl (SDL_RWops *src)
{
	Sint64 start;
	const char *error;
	SDL_Surface *surface;
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type, num_channels;
	Uint32 Rmask, Gmask, Bmask, Amask;
	png_bytep *row_pointers;
	int row, i;
	int ckey = -1;
	png_color_16 *transv;

	if (!src)
	{
		/* The error message has been set in SDL_RWFromFile */
		return NULL;
	}
	start = SDL_RWtell (src);

	/* Initialize the data we will clean up when we're done */
	error = NULL;
	png_ptr = NULL;
	info_ptr = NULL;
	row_pointers = NULL;
	surface = NULL;

	/* Create the PNG loading context structure */
	png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING,
			NULL, NULL, NULL);
	if (png_ptr == NULL)
	{
		error = "Couldn't allocate memory for PNG file";
		goto done;
	}

	/* Allocate/initialize the memory for image information */
	info_ptr = png_create_info_struct (png_ptr);
	if (info_ptr == NULL)
	{
		error = "Couldn't create image information for PNG file";
		goto done;
	}

	/* Set error handling */
	if (setjmp (png_jmpbuf (png_ptr)))
	{
		error = "Error reading the PNG file.";
		goto done;
	}

	/* Set up the input control */
	png_set_read_fn (png_ptr, src, png_read_data);

	/* Read PNG header info */
	png_read_info (png_ptr, info_ptr);
	png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth,
			&color_type, &interlace_type, NULL, NULL);


	/* Configure the decode based on what we know of the image
	 * already: strip 16 bit color down to 8 bit, automatically
	 * deinterlace, expand grayscale images or those with more
	 * than one transparent color or any translucent colors into
	 * full RGB or RGBA, and expand 1, 2, or 4-bpp paletted
	 * images to 8bpp. */
	png_set_strip_16 (png_ptr);
	png_set_interlace_handling (png_ptr);
	png_set_packing (png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY)
	{
		png_set_expand (png_ptr);
	}
	if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS))
	{
		int num_trans;
		Uint8 *trans;
		png_get_tRNS (png_ptr, info_ptr, &trans, &num_trans, &transv);
		if (color_type == PNG_COLOR_TYPE_PALETTE)
		{
			/* Check if all tRNS entries are opaque except one */
			int j, t = -1;
			for (j = 0; j < num_trans; j++)
			{
				if (trans[j] == 0)
				{
					if (t >= 0)
					{
						break;
					}
					t = j;
				}
				else if (trans[j] != 255)
				{
					break;
				}
			}
			if (j == num_trans)
			{
				/* exactly one transparent index */
				ckey = t;
			}
			else
			{
				/* more than one transparent index, or translucency */
				png_set_expand (png_ptr);
			}
		}
		else
		{
			ckey = 0; /* actual value will be set later */
		}
	}

	if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		png_set_gray_to_rgb (png_ptr);
	}

	/* Register our changes with the reading machinery and refresh
	 * our ancillary data about the image */
	png_read_update_info (png_ptr, info_ptr);
	png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth,
			&color_type, &interlace_type, NULL, NULL);

	/* Allocate the SDL surface to hold the image */
	Rmask = Gmask = Bmask = Amask = 0;
	num_channels = png_get_channels (png_ptr, info_ptr);
	if (num_channels >= 3)
	{
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
		Rmask = 0x000000FF;
		Gmask = 0x0000FF00;
		Bmask = 0x00FF0000;
		Amask = (num_channels == 4) ? 0xFF000000 : 0;
#else
		int s = (num_channels == 4) ? 0 : 8;
		Rmask = 0xFF000000 >> s;
		Gmask = 0x00FF0000 >> s;
		Bmask = 0x0000FF00 >> s;
		Amask = 0x000000FF >> s;
#endif
	}
	surface = SDL_CreateRGBSurface (SDL_SWSURFACE, width, height,
			bit_depth * num_channels,
			Rmask, Gmask, Bmask, Amask);
	if (surface == NULL)
	{
		error = SDL_GetError ();
		goto done;
	}

	if (ckey != -1)
	{
		if (color_type != PNG_COLOR_TYPE_PALETTE)
		{
			/* FIXME: Should these be truncated or shifted down? */
			ckey = SDL_MapRGB(surface->format,
					(Uint8)transv->red,
					(Uint8)transv->green,
					(Uint8)transv->blue);
		}
#if SDL_MAJOR_VERSION >= 2
		SDL_SetColorKey (surface, SDL_TRUE, ckey);
#else
		SDL_SetColorKey (surface, SDL_SRCCOLORKEY, ckey);
#endif
	}

	SDL_LockSurface (surface);
	/* Create the array of pointers to image data */
	row_pointers = (png_bytep *)SDL_malloc (sizeof (png_bytep) * height);
	if (!row_pointers)
	{
		error = "Out of memory";
		goto done;
	}
	for (row = 0; row < (int)height; row++)
	{
		row_pointers[row] = (png_bytep)
			(Uint8 *)surface->pixels + row*surface->pitch;
	}

	/* Read the entire image in one go */
	png_read_image (png_ptr, row_pointers);
	SDL_UnlockSurface (surface);

	/* and we're done!  (png_read_end() can be omitted if no
	 * processing of post-IDAT text/time/etc. is desired)
	 * In some cases it can't read PNGs created by some popular
	 * programs (ACDSEE), we do not want to process comments, so
	 * we omit png_read_end */

	/* Load the palette, if any */
	if (surface->format->palette)
	{
		SDL_Color palette[256];
		int png_num_palette;
		png_colorp png_palette;
		png_get_PLTE (png_ptr, info_ptr, &png_palette, &png_num_palette);
		if (color_type == PNG_COLOR_TYPE_GRAY)
		{
			png_num_palette = 256;
			for (i = 0; i < 256; i++)
			{
				palette[i].r = (Uint8)i;
				palette[i].g = (Uint8)i;
				palette[i].b = (Uint8)i;
			}
		}
		else if (png_num_palette > 0)
		{
			for (i = 0; i < png_num_palette; ++i)
			{
				palette[i].b = png_palette[i].blue;
				palette[i].g = png_palette[i].green;
				palette[i].r = png_palette[i].red;
			}
		}
#if SDL_MAJOR_VERSION >= 2
		SDL_SetPaletteColors (surface->format->palette, palette,
				0, png_num_palette);
#else
		SDL_SetPalette (surface, SDL_LOGPAL, palette,
				0, png_num_palette);
#endif
	}

done:	/* Clean up and return */
	if (png_ptr)
	{
		png_destroy_read_struct (&png_ptr,
				info_ptr ? &info_ptr : (png_infopp)0,
				(png_infopp)0);
	}
	if (row_pointers)
	{
		SDL_free (row_pointers);
	}
	if (error)
	{
		SDL_RWseek(src, start, RW_SEEK_SET);
		if (surface)
		{
			SDL_FreeSurface (surface);
			surface = NULL;
		}
		fprintf (stderr, "%s", error);
	}
	return surface;
}
