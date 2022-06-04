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
#include "../../../uqm/units.h"
#include "../../../options.h"

#include <time.h>

#if defined(ANDROID) || defined(__ANDROID__)
#include <SDL/SDL_screenkeyboard.h>
#endif

SDL_Surface *SDL_Screen;
SDL_Surface *TransitionScreen;

SDL_Surface *SDL_Screens[TFB_GFX_NUMSCREENS];

SDL_Surface *format_conv_surf = NULL;

#if SDL_MAJOR_VERSION == 1
const SDL_VideoInfo *SDL_screen_info; 
#endif

static volatile BOOLEAN abortFlag = FALSE;

int GfxFlags = 0;

TFB_GRAPHICS_BACKEND *graphics_backend = NULL;

volatile int QuitPosted = 0;
volatile int GameActive = 1; // Track the SDL_ACTIVEEVENT state SDL_APPACTIVE

int
TFB_InitGraphics (int driver, int flags, const char* renderer, 
		int width, int height, unsigned int *resFactor)
{
	int result, i;
	char caption[200];

	/* Null out screen pointers the first time */
	for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
	{
		SDL_Screens[i] = NULL;
	}

	GfxFlags = flags;
	
#if SDL_MAJOR_VERSION == 1
	// JMS_GFX: Let's read the size of the desktop so we can scale the
	// fullscreen game according to it.
	SDL_screen_info = SDL_GetVideoInfo ();
	
	// JMS_GFX: Upon starting the game, let's find out the resolution
	// of the desktop.
	if (fs_height == 0)
	{
		int curr_h = SDL_screen_info->current_h;
		int curr_w = SDL_screen_info->current_w;
		
		// JMS_GFX: This makes it sure on certain HD 16:9 monitors
		// that a bogus stretched 1600x1200 mode isn't used.
		if ((curr_w == 1920 && curr_h == 1080) || (curr_h == (curr_w / 16) * 10)) { // MB: fix for 16:10 resolutions
			fs_height = curr_h;
			fs_width  = curr_w;
		} else if (curr_h > (curr_w / 4) * 3) { // MB: for monitors using 5:4 modes
			fs_width = curr_w;
			fs_height = (curr_w / 4) * 3;
		} else {
			fs_height = curr_h;
			fs_width  = (4 * fs_height) / 3;
		}

		// MB: Sanitising resolution factor:
		if (fs_height <= 600 && *resFactor == HD) { // ie. probably netbook or otherwise
			*resFactor = 0;
		}
		
		log_add (log_Debug, "fs_height %u, fs_width %u, current_w %u", fs_height, fs_width, SDL_screen_info->current_w);
	}
#endif

	if (driver == TFB_GFXDRIVER_SDL_OPENGL)
	{
#ifdef HAVE_OPENGL
		result = TFB_GL_InitGraphics (driver, flags, width, height, *resFactor);
#else
		driver = TFB_GFXDRIVER_SDL_PURE;
		log_add (log_Warning, "OpenGL support not compiled in,"
				" so using pure SDL driver");
		result = TFB_Pure_InitGraphics (driver, flags, renderer, width, height, *resFactor);
#endif
	}
	else
	{
		result = TFB_Pure_InitGraphics (driver, flags, renderer, width, height, *resFactor);
	}

#if SDL_MAJOR_VERSION == 1
	/* Other versions do this when setting up the window */
	sprintf (caption, "The Ur-Quan Masters v%d.%d.%g %s",
			UQM_MAJOR_VERSION, UQM_MINOR_VERSION,
			UQM_PATCH_VERSION, UQM_EXTRA_VERSION);
	SDL_WM_SetCaption (caption, NULL);
#else
	(void) caption; /* satisfy compiler (unused parameter) */
#endif

	if (flags & TFB_GFXFLAGS_FULLSCREEN)
		SDL_ShowCursor (SDL_DISABLE);

	Init_DrawCommandQueue ();

	TFB_DrawCanvas_Initialize ();

	TFB_InitOnScreenKeyboard();

	return 0;
}

void
TFB_UninitGraphics (void)
{
	int i;

	Uninit_DrawCommandQueue ();

	for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
		UnInit_Screen (&SDL_Screens[i]);

	TFB_Pure_UninitGraphics ();
#ifdef HAVE_OPENGL
	TFB_GL_UninitGraphics ();
#endif

	UnInit_Screen (&format_conv_surf);
}

void
TFB_ProcessEvents ()
{
	SDL_Event Event;

	while (SDL_PollEvent (&Event) > 0)
	{
		/* Run through the InputEvent filter. */
		ProcessInputEvent (&Event);
		/* Handle graphics and exposure events. */
		switch (Event.type) {
#if 0 /* Currently disabled in mainline */
			case SDL_ACTIVEEVENT:    /* Lose/gain visibility or focus */
				/* Up to three different state changes can occur in one event. */
				/* Here, disregard least significant change (mouse focus). */
				// This controls the automatic sleep/pause when minimized.
				// On small displays (e.g. mobile devices), APPINPUTFOCUS would
				//  be an appropriate substitution for APPACTIVE:
				// if (Event.active.state & SDL_APPINPUTFOCUS)
				if (Event.active.state & SDL_APPACTIVE)
					GameActive = Event.active.gain;
				break;
			case SDL_VIDEORESIZE:    /* User resized video mode */
				// TODO
				break;
#endif
			case SDL_QUIT:
				QuitPosted = 1;
				break;
#if SDL_MAJOR_VERSION == 1
			case SDL_VIDEOEXPOSE:    /* Screen needs to be redrawn */
				TFB_SwapBuffers (TFB_REDRAW_EXPOSE);
				break;
#else
			case SDL_WINDOWEVENT:
				if (Event.window.event == SDL_WINDOWEVENT_EXPOSED)
				{
					/* Screen needs to be redrawn */
					TFB_SwapBuffers (TFB_REDRAW_EXPOSE);
				}
				break;
#endif
			default:
				break;
		}
	}
}

static BOOLEAN system_box_active = 0;
static SDL_Rect system_box;

void
SetSystemRect (const RECT *r)
{
	system_box_active = TRUE;
	system_box.x = r->corner.x;
	system_box.y = r->corner.y;
	system_box.w = r->extent.width;
	system_box.h = r->extent.height;
}

void
ClearSystemRect (void)
{
	system_box_active = FALSE;
}

void
TFB_SwapBuffers (int force_full_redraw)
{
	static int last_fade_amount = 255, last_transition_amount = 255;
	static int fade_amount = 255, transition_amount = 255;

	fade_amount = GetFadeAmount ();
	transition_amount = TransitionAmount;

	if (force_full_redraw == TFB_REDRAW_NO && !TFB_BBox.valid &&
			fade_amount == 255 && transition_amount == 255 &&
			last_fade_amount == 255 && last_transition_amount == 255)
		return;

	if (force_full_redraw == TFB_REDRAW_NO &&
			(fade_amount != 255 || transition_amount != 255 ||
			last_fade_amount != 255 || last_transition_amount != 255))
		force_full_redraw = TFB_REDRAW_FADING;

	last_fade_amount = fade_amount;
	last_transition_amount = transition_amount;

	graphics_backend->preprocess (force_full_redraw, transition_amount,
			fade_amount);
	graphics_backend->screen (TFB_SCREEN_MAIN, 255, NULL);

	if (transition_amount != 255)
	{
		SDL_Rect r;
		r.x = TransitionClipRect.corner.x;
		r.y = TransitionClipRect.corner.y;
		r.w = TransitionClipRect.extent.width;
		r.h = TransitionClipRect.extent.height;
		graphics_backend->screen (TFB_SCREEN_TRANSITION,
				255 - transition_amount, &r);
	}

	if (fade_amount != 255)
	{
		if (fade_amount < 255)
		{
			graphics_backend->color (0, 0, 0, 255 - fade_amount, NULL);
		}
		else
		{
			graphics_backend->color (255, 255, 255,
					fade_amount - 255, NULL);
		}
	}

	if (system_box_active)
	{
		graphics_backend->screen (TFB_SCREEN_MAIN, 255, &system_box);
	}

	graphics_backend->postprocess ();
}

/* Probably ought to clean this away at some point. */
SDL_Surface *
TFB_DisplayFormatAlpha (SDL_Surface *surface)
{
	SDL_Surface* newsurf;
	SDL_PixelFormat* dstfmt;
	const SDL_PixelFormat* srcfmt = surface->format;

	// figure out what format to use (alpha/no alpha)
	if (surface->format->Amask)
		dstfmt = format_conv_surf->format;
	else
		dstfmt = SDL_Screen->format;

	if (srcfmt->BytesPerPixel == dstfmt->BytesPerPixel &&
			srcfmt->Rmask == dstfmt->Rmask &&
			srcfmt->Gmask == dstfmt->Gmask &&
			srcfmt->Bmask == dstfmt->Bmask &&
			srcfmt->Amask == dstfmt->Amask)
		return surface; // no conversion needed

	newsurf = SDL_ConvertSurface (surface, dstfmt, surface->flags);
	// Colorkeys and surface-level alphas cannot work at the same time,
	// so we need to disable one of them
	if (TFB_HasColorKey (surface) && newsurf &&
			TFB_HasColorKey (newsurf) &&
			TFB_HasSurfaceAlphaMod (newsurf))
	{
		TFB_DisableSurfaceAlphaMod (newsurf);
	}

	return newsurf;
}

// This function should only be called from the graphics thread,
// like from a TFB_DrawCommand_Callback command.
TFB_Canvas
TFB_GetScreenCanvas (SCREEN screen)
{
	return SDL_Screens[screen];
}

void
TFB_BlitSurface (SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst,
		SDL_Rect *dstrect, int blend_numer, int blend_denom)
{
	BOOLEAN has_colorkey;
	int x, y, x1, y1, x2, y2, dst_x2, dst_y2, nr, ng, nb;
	int srcx, srcy, w, h;
	Uint8 sr, sg, sb, dr, dg, db;
	Uint32 src_pixval, dst_pixval, colorkey;
	GetPixelFn src_getpix, dst_getpix;
	PutPixelFn putpix;
	SDL_Rect fulldst;

	if (blend_numer == blend_denom)
	{
		// normal blit: dst = src

		// log_add (log_Debug, "normal blit\n");
		SDL_BlitSurface (src, srcrect, dst, dstrect);
		return;
	}

	// NOTE: following clipping code is copied from SDL-1.2.4 sources

	// If the destination rectangle is NULL, use the entire dest surface
	if (dstrect == NULL)
	{
		fulldst.x = fulldst.y = 0;
		dstrect = &fulldst;
	}

	// clip the source rectangle to the source surface
	if (srcrect)
	{
		int maxw, maxh;

		srcx = srcrect->x;
		w = srcrect->w;
		if (srcx < 0)
		{
			w += srcx;
			dstrect->x -= srcx;
			srcx = 0;
		}
		maxw = src->w - srcx;
		if (maxw < w)
			w = maxw;

		srcy = srcrect->y;
		h = srcrect->h;
		if (srcy < 0)
		{
			h += srcy;
			dstrect->y -= srcy;
			srcy = 0;
		}
		maxh = src->h - srcy;
		if (maxh < h)
			h = maxh;
	}
	else
	{
		srcx = 0;
		srcy = 0;
		w = src->w;
		h = src->h;
	}

	// clip the destination rectangle against the clip rectangle
	{
		SDL_Rect *clip = &dst->clip_rect;
		int dx, dy;

		dx = clip->x - dstrect->x;
		if (dx > 0)
		{
			w -= dx;
			dstrect->x += dx;
			srcx += dx;
		}
		dx = dstrect->x + w - clip->x - clip->w;
		if (dx > 0)
			w -= dx;

		dy = clip->y - dstrect->y;
		if (dy > 0)
		{
			h -= dy;
			dstrect->y += dy;
			srcy += dy;
		}
		dy = dstrect->y + h - clip->y - clip->h;
		if (dy > 0)
			h -= dy;
	}

	dstrect->w = w;
	dstrect->h = h;

	if (w <= 0 || h <= 0)
		return;

	x1 = srcx;
	y1 = srcy;
	x2 = srcx + w;
	y2 = srcy + h;

	if (TFB_GetColorKey (src, &colorkey) < 0)
	{
		has_colorkey = FALSE;
		colorkey = 0;  /* Satisfying compiler */
	}
	else
	{
		has_colorkey = TRUE;
	}

	src_getpix = getpixel_for (src);
	dst_getpix = getpixel_for (dst);
	putpix = putpixel_for (dst);

	if (blend_denom < 0)
	{
		// additive blit: dst = src + dst
#if 0
		log_add (log_Debug, "additive blit %d %d, src %d %d %d %d dst %d %d,"
				" srcbpp %d", blend_numer, blend_denom, x1, y1, x2, y2,
				dstrect->x, dstrect->y, src->format->BitsPerPixel);
#endif
		for (y = y1; y < y2; ++y)
		{
			dst_y2 = dstrect->y + (y - y1);
			for (x = x1; x < x2; ++x)
			{
				dst_x2 = dstrect->x + (x - x1);
				src_pixval = src_getpix (src, x, y);

				if (has_colorkey && src_pixval == colorkey)
					continue;

				dst_pixval = dst_getpix (dst, dst_x2, dst_y2);

				SDL_GetRGB (src_pixval, src->format, &sr, &sg, &sb);
				SDL_GetRGB (dst_pixval, dst->format, &dr, &dg, &db);

				nr = sr + dr;
				ng = sg + dg;
				nb = sb + db;

				if (nr > 255)
					nr = 255;
				if (ng > 255)
					ng = 255;
				if (nb > 255)
					nb = 255;

				putpix (dst, dst_x2, dst_y2,
						SDL_MapRGB (dst->format, nr, ng, nb));
			}
		}
	}
	else if (blend_numer < 0)
	{
		// subtractive blit: dst = src - dst
#if 0
		log_add (log_Debug, "subtractive blit %d %d, src %d %d %d %d"
				" dst %d %d, srcbpp %d", blend_numer, blend_denom,
					x1, y1, x2, y2, dstrect->x, dstrect->y,
					src->format->BitsPerPixel);
#endif
		for (y = y1; y < y2; ++y)
		{
			dst_y2 = dstrect->y + (y - y1);
			for (x = x1; x < x2; ++x)
			{
				dst_x2 = dstrect->x + (x - x1);
				src_pixval = src_getpix (src, x, y);

				if (has_colorkey && src_pixval == colorkey)
					continue;

				dst_pixval = dst_getpix (dst, dst_x2, dst_y2);

				SDL_GetRGB (src_pixval, src->format, &sr, &sg, &sb);
				SDL_GetRGB (dst_pixval, dst->format, &dr, &dg, &db);

				nr = sr - dr;
				ng = sg - dg;
				nb = sb - db;

				if (nr < 0)
					nr = 0;
				if (ng < 0)
					ng = 0;
				if (nb < 0)
					nb = 0;

				putpix (dst, dst_x2, dst_y2,
						SDL_MapRGB (dst->format, nr, ng, nb));
			}
		}
	}
	else
	{
		// modulated blit: dst = src * (blend_numer / blend_denom)

		float f = blend_numer / (float)blend_denom;
#if 0
		log_add (log_Debug, "modulated blit %d %d, f %f, src %d %d %d %d"
				" dst %d %d, srcbpp %d\n", blend_numer, blend_denom, f,
				x1, y1, x2, y2, dstrect->x, dstrect->y,
				src->format->BitsPerPixel);
#endif
		for (y = y1; y < y2; ++y)
		{
			dst_y2 = dstrect->y + (y - y1);
			for (x = x1; x < x2; ++x)
			{
				dst_x2 = dstrect->x + (x - x1);
				src_pixval = src_getpix (src, x, y);

				if (has_colorkey && src_pixval == colorkey)
					continue;

				SDL_GetRGB (src_pixval, src->format, &sr, &sg, &sb);

				nr = (int)(sr * f);
				ng = (int)(sg * f);
				nb = (int)(sb * f);

				if (nr > 255)
					nr = 255;
				if (ng > 255)
					ng = 255;
				if (nb > 255)
					nb = 255;

				putpix (dst, dst_x2, dst_y2,
						SDL_MapRGB (dst->format, nr, ng, nb));
			}
		}
	}
}

void
UnInit_Screen (SDL_Surface **screen)
{
	if (*screen == NULL)
		return;
	
	SDL_FreeSurface (*screen);
	*screen = NULL;
}

void
TFB_UploadTransitionScreen (void)
{
	graphics_backend->uploadTransitionScreen ();
}

int
TFB_HasColorKey (SDL_Surface *surface)
{
	Uint32 key;
	return TFB_GetColorKey (surface, &key) == 0;
}

void
TFB_ScreenShot (void)
{

	char curTime[PATH_MAX], fullPath[PATH_MAX];
	time_t t = time (NULL);
	struct tm *tm = localtime (&t);
	const char *shotDirName = getenv ("UQM_SCR_SHOT_DIR");

	strftime (curTime, sizeof (curTime),
		"%Y-%m-%d_%H-%M-%S", tm);
	snprintf (fullPath, sizeof (fullPath),
		"%s%s v%d.%d.%g %s.%s", shotDirName, curTime,
		UQM_MAJOR_VERSION, UQM_MINOR_VERSION, UQM_PATCH_VERSION,
		UQM_EXTRA_VERSION, "png");

#if SDL_MAJOR_VERSION == 1
	TFB_SDL1_ScreenShot (fullPath);
#else
	TFB_SDL2_ScreenShot (fullPath);
#endif
}

#if defined(ANDROID) || defined(__ANDROID__)

static SDL_Rect SDL_LeftJoystickRect, SDL_RightJoystickRect, SDL_TextInputRect;
static BOOLEAN HideScreenKeyboard = FALSE;

void
TFB_InitOnScreenKeyboard (void)
{

	SDL_ANDROID_GetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_DPAD, &SDL_LeftJoystickRect);
	SDL_ANDROID_GetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_DPAD2, &SDL_RightJoystickRect);
	SDL_ANDROID_GetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_TEXT, &SDL_TextInputRect);
	
	TFB_SetOnScreenKeyboard_Menu ();

	HideScreenKeyboard = (getenv ("OUYA") != NULL);

	if (HideScreenKeyboard)
		TFB_SetOnScreenKeyboard_Hidden ();
}

void 
TFB_SetOnScreenKeyboard_HiddenPermanently (void)
{
	if (HideScreenKeyboard)
		return;

	HideScreenKeyboard = TRUE;

	TFB_SetOnScreenKeyboard_Hidden ();
}

void
TFB_SetOnScreenKeyboard_Hidden (void)
{
	SDL_Rect b;

	if (SDL_ANDROID_GetScreenKeyboardRedefinedByUser ())
		return;

	b.w = b.h = b.x = b.y = 0;

	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_0, &b);
	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_1, &b);
	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_2, &b);
	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_3, &b);
	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_4, &b);
	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_5, &b);
	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_DPAD, &b);
	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_DPAD2, &b);
	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_TEXT, &b);
}

void
TFB_SetOnScreenKeyboard_Menu (void)
{

	SDL_Rect b;

	if (SDL_ANDROID_GetScreenKeyboardRedefinedByUser () || HideScreenKeyboard)
		return;

	TFB_SetOnScreenKeyboard_Hidden ();

	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_DPAD, &SDL_LeftJoystickRect);
	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_TEXT, &SDL_TextInputRect);

	b.w = b.h = SDL_RightJoystickRect.w / 2;
	b.x = SDL_RightJoystickRect.x + b.w;
	b.y = SDL_RightJoystickRect.y - (b.w / 2);

	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_0, &b);

	b.x = SDL_RightJoystickRect.x;

	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_1, &b);
}

void
TFB_SetOnScreenKeyboard_Starmap (void)
{
	SDL_Rect b;

	if (SDL_ANDROID_GetScreenKeyboardRedefinedByUser () || HideScreenKeyboard)
		return;

	TFB_SetOnScreenKeyboard_Menu ();

	b.h = b.w = SDL_RightJoystickRect.w / 2;
	b.x = SDL_RightJoystickRect.x + b.w;
	b.y = SDL_RightJoystickRect.y + (b.w / 2);

	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_3, &b);

	b.x = SDL_RightJoystickRect.x;

	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_4, &b);
}

void
TFB_SetOnScreenKeyboard_Melee (void)
{
	SDL_Rect b;

	if (SDL_ANDROID_GetScreenKeyboardRedefinedByUser () || HideScreenKeyboard)
		return;

	TFB_SetOnScreenKeyboard_Hidden ();

	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_DPAD, &SDL_LeftJoystickRect);

	b.w = b.h = SDL_RightJoystickRect.w / 2;
	b.x = SDL_RightJoystickRect.x + b.w;
	b.y = SDL_RightJoystickRect.y - (b.w / 2);

	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_0, &b);

	b.x = SDL_RightJoystickRect.x;

	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_1, &b);

	b.w = SDL_RightJoystickRect.w;
	b.y += (b.w / 2);

	SDL_ANDROID_SetScreenKeyboardButtonPos (SDL_ANDROID_SCREENKEYBOARD_BUTTON_2, &b);
}

void
TFB_SetOnScreenKeyboard_TwoPlayersMelee (void)
{
	SDL_Rect b = SDL_RightJoystickRect;

	if (SDL_ANDROID_GetScreenKeyboardRedefinedByUser () || HideScreenKeyboard)
		return;

	TFB_SetOnScreenKeyboard_Melee();

	b.y = 0;

	SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_DPAD2, &b);

	b.w = b.h = SDL_RightJoystickRect.w / 2;
	b.x = 0;
	b.y = SDL_RightJoystickRect.w - b.w;

	SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_3, &b);

	b.x = SDL_RightJoystickRect.w - b.w;

	SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_4, &b);

	b.w = SDL_RightJoystickRect.w;
	b.x = 0;
	b.y = 0;

	SDL_ANDROID_SetScreenKeyboardButtonPos(SDL_ANDROID_SCREENKEYBOARD_BUTTON_5, &b);
}

#else

/* Stubs */
void TFB_InitOnScreenKeyboard(void) { }
void TFB_SetOnScreenKeyboard_Hidden(void) { }
void TFB_SetOnScreenKeyboard_Menu(void) { }
void TFB_SetOnScreenKeyboard_Starmap(void) { }
void TFB_SetOnScreenKeyboard_Melee(void) { }
void TFB_SetOnScreenKeyboard_TwoPlayersMelee(void) { }

#endif