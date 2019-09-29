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

#if SDL_MAJOR_VERSION == 1

void
TFB_PreInit (void)
{
	log_add (log_Info, "Initializing base SDL functionality.");
	log_add (log_Info, "Using SDL version %d.%d.%d (compiled with "
			"%d.%d.%d)", SDL_Linked_Version ()->major,
			SDL_Linked_Version ()->minor, SDL_Linked_Version ()->patch,
			SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
#if 0
	if (SDL_Linked_Version ()->major != SDL_MAJOR_VERSION ||
			SDL_Linked_Version ()->minor != SDL_MINOR_VERSION ||
			SDL_Linked_Version ()->patch != SDL_PATCHLEVEL) {
		log_add (log_Warning, "The used SDL library is not the same version "
				"as the one used to compile The Ur-Quan Masters with! "
				"If you experience any crashes, this would be an excellent "
				"suspect.");
	}
#endif

	if ((SDL_Init (SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) == -1))
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
	char caption[200];

	if (GfxFlags == (flags ^ TFB_GFXFLAGS_FULLSCREEN) &&
			driver == GraphicsDriver &&
			width == ScreenWidthActual && height == ScreenHeightActual)
	{
		togglefullscreen = 1;
	}

	GfxFlags = flags;

	if (driver == TFB_GFXDRIVER_SDL_OPENGL)
	{
#ifdef HAVE_OPENGL
		result = TFB_GL_ConfigureVideo (driver, flags, width, height,
				togglefullscreen);
#else
		driver = TFB_GFXDRIVER_SDL_PURE;
		log_add (log_Warning, "OpenGL support not compiled in,"
				" so using pure SDL driver");
		result = TFB_Pure_ConfigureVideo (driver, flags, width, height,
				togglefullscreen);
#endif
	}
	else
	{
		result = TFB_Pure_ConfigureVideo (driver, flags, width, height,
				togglefullscreen);
	}

	sprintf (caption, "The Ur-Quan Masters v%d.%d.%d%s",
			UQM_MAJOR_VERSION, UQM_MINOR_VERSION,
			UQM_PATCH_VERSION, UQM_EXTRA_VERSION);
	SDL_WM_SetCaption (caption, NULL);

	if (flags & TFB_GFXFLAGS_FULLSCREEN)
		SDL_ShowCursor (SDL_DISABLE);
	else
		SDL_ShowCursor (SDL_ENABLE);

	return result;
}

void
TFB_SetGamma (float gamma)
{
	if (SDL_SetGamma (gamma, gamma, gamma) == -1)
	{
		log_add (log_Warning, "Unable to set gamma correction.");
	}
	else
	{
		log_add (log_Info, "Gamma correction set to %1.4f.", gamma);
	}
}

int
TFB_HasSurfaceAlphaMod (SDL_Surface *surface)
{
	if (!surface)
	{
		return 0;
	}
	return (surface->flags & SDL_SRCALPHA) ? 1 : 0;
}

int
TFB_GetSurfaceAlphaMod (SDL_Surface *surface, Uint8 *alpha)
{
	if (!surface || !surface->format || !alpha)
	{
		return -1;
	}
	if (surface->flags & SDL_SRCALPHA)
	{
		*alpha = surface->format->alpha;
	}
	else
	{
		*alpha = 255;
	}
	return 0;
}

int
TFB_SetSurfaceAlphaMod (SDL_Surface *surface, Uint8 alpha)
{
	if (!surface)
	{
		return -1;
	}
	return SDL_SetAlpha (surface, SDL_SRCALPHA, alpha);
}

int
TFB_DisableSurfaceAlphaMod (SDL_Surface *surface)
{
	if (!surface)
	{
		return -1;
	}
	return SDL_SetAlpha (surface, 0, 255);
}

int
TFB_GetColorKey (SDL_Surface *surface, Uint32 *key)
{
	if (surface && surface->format && key &&
			(surface->flags & SDL_SRCCOLORKEY))
	{
		*key = surface->format->colorkey;
		return 0;
	}
	return -1;
}

int
TFB_SetColorKey (SDL_Surface *surface, Uint32 key, int rleaccel)
{
	if (!surface)
	{
		return -1;
	}
	return SDL_SetColorKey (surface, SDL_SRCCOLORKEY | (rleaccel ? SDL_RLEACCEL : 0), key);
}

int
TFB_DisableColorKey (SDL_Surface *surface)
{
	if (!surface)
	{
		return -1;
	}
	return SDL_SetColorKey (surface, 0, 0);
}

int
TFB_SetColors (SDL_Surface *surface, SDL_Color *colors, int firstcolor, int ncolors)
{
	return SDL_SetColors (surface, colors, firstcolor, ncolors);
}

int
TFB_SupportsHardwareScaling (void)
{
#ifdef HAVE_OPENGL
	return 1;
#else
	return 0;
#endif
}

#endif
