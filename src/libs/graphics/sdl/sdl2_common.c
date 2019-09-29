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

#include "sdl_common.h"
#include "opengl.h"
#include "pure.h"
#include "primitives.h"
#include "options.h"
#include "uqmversion.h"
#include "libs/graphics/drawcmd.h"
#include "libs/graphics/dcqueue.h"
#include "libs/graphics/cmap.h"
#include "libs/input/sdl/input.h"
		// for ProcessInputEvent()
#include "libs/graphics/bbox.h"
#include "port.h"
#include "libs/uio.h"
#include "libs/log.h"
#include "libs/memlib.h"
#include "libs/vidlib.h"

#if SDL_MAJOR_VERSION > 1

void
TFB_PreInit (void)
{
	SDL_version compiled, linked;
	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	log_add (log_Info, "Initializing base SDL functionality.");
	log_add (log_Info, "Using SDL version %d.%d.%d (compiled with "
			"%d.%d.%d)", linked.major, linked.minor, linked.patch,
			compiled.major, compiled.minor, compiled.patch);
#if 0
	if (compiled.major != linked.major || compiled.minor != linked.minor ||
			compiled.patch != linked.patch)
	{
		log_add (log_Warning, "The used SDL library is not the same version "
				"as the one used to compile The Ur-Quan Masters with! "
				"If you experience any crashes, this would be an excellent "
				"suspect.");
	}
#endif

	if ((SDL_Init (SDL_INIT_VIDEO) == -1))
	{
		log_add (log_Fatal, "Could not initialize SDL: %s.", SDL_GetError ());
		exit (EXIT_FAILURE);
	}
}

int
TFB_ReInitGraphics (int driver, int flags, int width, int height)
{
	int result;
	int togglefullscreen = 0;

	if (GfxFlags == (flags ^ TFB_GFXFLAGS_FULLSCREEN) &&
			driver == GraphicsDriver &&
			width == ScreenWidthActual && height == ScreenHeightActual)
	{
		togglefullscreen = 1;
	}

	GfxFlags = flags;

	result = TFB_Pure_ConfigureVideo (TFB_GFXDRIVER_SDL_PURE, flags,
			width, height, togglefullscreen);

	if (flags & TFB_GFXFLAGS_FULLSCREEN)
		SDL_ShowCursor (SDL_DISABLE);
	else
		SDL_ShowCursor (SDL_ENABLE);

	return result;
}

void
TFB_SetGamma (float gamma)
{
	log_add (log_Warning, "Custom gamma correction is not available in the SDL2 engine.");
}

int
TFB_HasSurfaceAlphaMod (SDL_Surface *surface)
{
	SDL_BlendMode blend_mode;
	if (!surface)
	{
		return 0;
	}
	if (SDL_GetSurfaceBlendMode (surface, &blend_mode) != 0)
	{
		return 0;
	}
	return blend_mode == SDL_BLENDMODE_BLEND;
}

int
TFB_GetSurfaceAlphaMod (SDL_Surface *surface, Uint8 *alpha)
{
	SDL_BlendMode blend_mode;
	if (!surface || !alpha)
	{
		return -1;
	}
	if (SDL_GetSurfaceBlendMode (surface, &blend_mode) == 0)
	{
		if (blend_mode == SDL_BLENDMODE_BLEND)
		{
			return SDL_GetSurfaceAlphaMod (surface, alpha);
		}
	}
	*alpha = 255;
	return 0;
}

int
TFB_SetSurfaceAlphaMod (SDL_Surface *surface, Uint8 alpha)
{
	int result;
	if (!surface)
	{
		return -1;
	}
	result = SDL_SetSurfaceBlendMode (surface, SDL_BLENDMODE_BLEND);
	if (result == 0)
	{
		result = SDL_SetSurfaceAlphaMod (surface, alpha);
	}
	return result;
}

int
TFB_DisableSurfaceAlphaMod (SDL_Surface *surface)
{
	if (!surface)
	{
		return -1;
	}
	SDL_SetSurfaceAlphaMod (surface, 255);
	return SDL_SetSurfaceBlendMode (surface, SDL_BLENDMODE_NONE);
}

int
TFB_GetColorKey (SDL_Surface *surface, Uint32 *key)
{
	if (!surface || !key)
	{
		return -1;
	}
	return SDL_GetColorKey (surface, key);
}

int
TFB_SetColorKey (SDL_Surface *surface, Uint32 key, int rleaccel)
{
	if (!surface)
	{
		return -1;
	}
	SDL_SetSurfaceRLE (surface, rleaccel);
	return SDL_SetColorKey (surface, SDL_TRUE, key);
}

int
TFB_DisableColorKey (SDL_Surface *surface)
{
	if (!surface)
	{
		return -1;
	}
	return SDL_SetColorKey (surface, SDL_FALSE, 0);
}

int
TFB_SetColors (SDL_Surface *surface, SDL_Color *colors, int firstcolor, int ncolors)
{
	if (!surface || !colors || !surface->format || !surface->format->palette)
	{
		return 0;
	}
	if (SDL_SetPaletteColors (surface->format->palette, colors, firstcolor, ncolors) == 0)
	{
		// SDL2's success code is opposite from SDL1's SDL_SetColors
		return 1;
	}
	return 0;
}

int
TFB_SupportsHardwareScaling (void)
{
	return 1;
}
#endif
