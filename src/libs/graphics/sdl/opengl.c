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

#ifdef HAVE_OPENGL

#include "libs/graphics/sdl/opengl.h"
#include "libs/graphics/bbox.h"
#include "scalers.h"
#include "options.h"
#include "libs/log.h"
#include "uqm/units.h"

typedef struct _gl_screeninfo {
	SDL_Surface *scaled;
	GLuint texture;
	BOOLEAN dirty, active;
	SDL_Rect updated;
} TFB_GL_SCREENINFO;

static TFB_GL_SCREENINFO GL_Screens[TFB_GFX_NUMSCREENS];

static int ScreenFilterMode;

static TFB_ScaleFunc scaler = NULL;
static BOOLEAN first_init = TRUE;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define R_MASK 0xff000000
#define G_MASK 0x00ff0000
#define B_MASK 0x0000ff00
#define A_MASK 0x000000ff
#else
#define R_MASK 0x000000ff
#define G_MASK 0x0000ff00
#define B_MASK 0x00ff0000
#define A_MASK 0xff000000
#endif

static void TFB_GL_Preprocess (int force_full_redraw, int transition_amount, int fade_amount);
static void TFB_GL_Postprocess (void);
static void TFB_GL_Scaled_ScreenLayer (SCREEN screen, Uint8 a, SDL_Rect *rect);
static void TFB_GL_Unscaled_ScreenLayer (SCREEN screen, Uint8 a, SDL_Rect *rect);
static void TFB_GL_ColorLayer (Uint8 r, Uint8 g, Uint8 b, Uint8 a, SDL_Rect *rect);

static TFB_GRAPHICS_BACKEND opengl_scaled_backend = {
	TFB_GL_Preprocess,
	TFB_GL_Postprocess,
	TFB_GL_Scaled_ScreenLayer,
	TFB_GL_ColorLayer };

static TFB_GRAPHICS_BACKEND opengl_unscaled_backend = {
	TFB_GL_Preprocess,
	TFB_GL_Postprocess,
	TFB_GL_Unscaled_ScreenLayer,
	TFB_GL_ColorLayer };

static int
AttemptColorDepth (int flags, int width, int height, int bpp, unsigned int resFactor)
{
	int videomode_flags;
	ScreenColorDepth = bpp;
	ScreenWidthActual = width;
	ScreenHeightActual = height;

	switch (bpp) {
		case 15:
			SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 5);
			SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 5);
			SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 5);
			break;

		case 16:
			SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 5);
			SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 6);
			SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 5);
			break;

		case 24:
			SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 8);
			break;

		case 32:
			SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 8);
			SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 8);
			SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 8);
			break;
		default:
			break;
	}

	SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);

	videomode_flags = SDL_OPENGL;
	if (flags & TFB_GFXFLAGS_FULLSCREEN)
		videomode_flags |= SDL_FULLSCREEN;
	videomode_flags |= SDL_ANYFORMAT;

	if (resFactor == HD && flags & TFB_GFXFLAGS_FULLSCREEN)
	{
		height = fs_height;
		width  = fs_width;
			
		log_add (log_Debug,"X:%d y:%d", width, height);
	}
	
	ScreenWidthActual = width;
	ScreenHeightActual = height;

	SDL_Video = SDL_SetVideoMode (ScreenWidthActual, ScreenHeightActual, 
		bpp, videomode_flags);
	if (SDL_Video == NULL)
	{
		log_add (log_Error, "Couldn't set OpenGL %ix%ix%i video mode: %s",
				ScreenWidthActual, ScreenHeightActual, bpp,
				SDL_GetError ());

		if (flags & TFB_GFXFLAGS_FULLSCREEN)
		{
			videomode_flags &= ~SDL_FULLSCREEN;
			log_add (log_Error, "Falling back to windowed mode!!");
			SDL_Video = SDL_SetVideoMode (ScreenWidthActual, ScreenHeightActual, bpp, videomode_flags);
			
			if (SDL_Video != NULL)
				goto successful_change;
		}

		return -1;
	}
	else
	{
		successful_change:
		log_add (log_Info, "Set the resolution to: %ix%ix%i"
				" (surface reports %ix%ix%i) (res_cat %u)",
				width, height, bpp,			 
				SDL_GetVideoSurface()->w, SDL_GetVideoSurface()->h,
				SDL_GetVideoSurface()->format->BitsPerPixel, resFactor);

		log_add (log_Info, "OpenGL renderer: %s version: %s",
				glGetString (GL_RENDERER), glGetString (GL_VERSION));

		// JMS: Now, this makes the game center horizontally
		// between the black bars on the sides.
		ScreenWidthActual = SDL_GetVideoSurface()->w;
	}
	return 0;
}

int
TFB_GL_ConfigureVideo (int driver, int flags, int width, int height, int togglefullscreen, unsigned int resFactor)
{
	int i, texture_width, texture_height;
	GraphicsDriver = driver;

	if (AttemptColorDepth (flags, width, height, 32, resFactor) &&
			AttemptColorDepth (flags, width, height, 24, resFactor) &&
			AttemptColorDepth (flags, width, height, 16, resFactor))
	{
		log_add (log_Error, "Couldn't set any OpenGL %ix%i video mode!",
			 width, height);
		return -1;
	}

	if (!togglefullscreen)
	{
		if (format_conv_surf)
			SDL_FreeSurface (format_conv_surf);
		format_conv_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, 0, 0, 32,
			R_MASK, G_MASK, B_MASK, A_MASK);
		if (format_conv_surf == NULL)
		{
			log_add (log_Error, "Couldn't create format_conv_surf: %s",
					SDL_GetError());
			return -1;
		}

		for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
		{
			if (0 != ReInit_Screen (&SDL_Screens[i], format_conv_surf,
					ScreenWidth, ScreenHeight))
				return -1;
		}

		SDL_Screen = SDL_Screens[0];
		TransitionScreen = SDL_Screens[2];

		if (first_init)
		{
			for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
			{
				GL_Screens[i].scaled = NULL;
				GL_Screens[i].dirty = TRUE;
				GL_Screens[i].active = TRUE;
			}
			GL_Screens[1].active = FALSE;
			first_init = FALSE;
		}
	}

	if (GfxFlags & TFB_GFXFLAGS_SCALE_SOFT_ONLY)
	{
		if (!togglefullscreen)
		{
			for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
			{
				if (!GL_Screens[i].active)
					continue;
				if (0 != ReInit_Screen (&GL_Screens[i].scaled, format_conv_surf,
						ScreenWidth * 2, ScreenHeight * 2))
				return -1;
			}
			scaler = Scale_PrepPlatform (flags, SDL_Screen->format);
		}

		texture_width = 1024;
		texture_height = 512;

		graphics_backend = &opengl_scaled_backend;
	}
	else
	{
		texture_width = 512 << resFactor;
		texture_height = 256 << resFactor;
		graphics_backend = &opengl_unscaled_backend;
		
		scaler = NULL;
	}


	if (GfxFlags & TFB_GFXFLAGS_SCALE_ANY)
		ScreenFilterMode = GL_LINEAR;
	else
		ScreenFilterMode = GL_NEAREST;

	glViewport (0, 0, ScreenWidthActual, ScreenHeightActual);
	glClearColor (0,0,0,0);
	glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	SDL_GL_SwapBuffers ();
	glClear (GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glDisable (GL_DITHER);
	glDepthMask(GL_FALSE);

	for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
	{
		if (!GL_Screens[i].active)
			continue;
		glGenTextures (1, &GL_Screens[i].texture);
		glBindTexture (GL_TEXTURE_2D, GL_Screens[i].texture);
		glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGB, texture_width, texture_height,
				0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	}

	return 0;
}

int
TFB_GL_InitGraphics (int driver, int flags, int width, int height, unsigned int resFactor)
{
	char VideoName[256];

	log_add (log_Info, "Initializing SDL with OpenGL support.");

	SDL_VideoDriverName (VideoName, sizeof (VideoName));
	log_add (log_Info, "SDL driver used: %s", VideoName);
	log_add (log_Info, "SDL initialized.");
	log_add (log_Info, "Initializing Screen.");

	ScreenWidth = (320 << resFactor); // JMS_GFX
	ScreenHeight = (240 << resFactor); // JMS_GFX

	if (TFB_GL_ConfigureVideo (driver, flags, width, height, 0, resFactor))
	{
		log_add (log_Fatal, "Could not initialize video: "
				"no fallback at start of program!");
		exit (EXIT_FAILURE);
	}	 

	// Initialize scalers (let them precompute whatever)
	Scale_Init ();

	return 0;
}

void
TFB_GL_UninitGraphics (void)
{
	int i;

	for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
		UnInit_Screen (&GL_Screens[i].scaled);
}

void TFB_GL_UploadTransitionScreen (void)
{
	GL_Screens[TFB_SCREEN_TRANSITION].updated.x = 0;
	GL_Screens[TFB_SCREEN_TRANSITION].updated.y = 0;
	GL_Screens[TFB_SCREEN_TRANSITION].updated.w = ScreenWidth;
	GL_Screens[TFB_SCREEN_TRANSITION].updated.h = ScreenHeight;
	GL_Screens[TFB_SCREEN_TRANSITION].dirty = TRUE;
}

static void
TFB_GL_ScanLines (void)
{
	int y;

	glDisable (GL_TEXTURE_2D);
	glEnable (GL_BLEND);
	glBlendFunc (GL_DST_COLOR, GL_ZERO);
	// glColor3f (0.85f, 0.85f, 0.85f);
	glColor3f (0.4f, 0.4f, 0.4f); // Darkened scanlines
	for (y = 0; y < ScreenHeightActual; y += 2)
	{
		glBegin (GL_LINES);
		glVertex2i (0, y);
		glVertex2i (ScreenWidthActual, y);
		glEnd ();
	}

	glBlendFunc (GL_DST_COLOR, GL_ONE);
	glColor3f (0.3f, 0.3f, 0.3f);
	for (y = 1; y < ScreenHeightActual; y += 2)
	{
		glBegin (GL_LINES);
		glVertex2i (0, y);
		glVertex2i (ScreenWidthActual, y);
		glEnd ();
	}
}

static void
TFB_GL_DrawQuad (SDL_Rect *r, BYTE ResFactor)
{
	BOOLEAN keep_aspect_ratio = optKeepAspectRatio;
	int x1 = 0, y1 = 0, x2 = ScreenWidthActual, y2 = ScreenHeightActual;
	int sx = 0, sy = 0;
	int sw, sh;
	float sx_multiplier = 1;
	float sy_multiplier = 1;
	float tex1, tex2;

	if (keep_aspect_ratio)
	{
		float threshold = 0.75f;
		float ratio = ScreenHeightActual / (float)ScreenWidthActual;

		if (ratio > threshold)
		{
			// screen is narrower than 4:3
			int height = (int)(ScreenWidthActual * threshold);
			y1 = (ScreenHeightActual - height) / 2;
			y2 = ScreenHeightActual - y1;

			if (r != NULL)
			{
				sx_multiplier = ScreenWidthActual / (float)ScreenWidth;
				sy_multiplier = height / (float)ScreenHeight;
				sx = (int)(r->x * sx_multiplier);
				sy = (int)(((ScreenHeight - (r->y + r->h)) * sy_multiplier) + y1);
			}
		}
		else if (ratio < threshold)
		{
			// screen is wider than 4:3
			int width = (int)(ScreenHeightActual / threshold);
			x1 = (ScreenWidthActual - width) / 2;
			x2 = ScreenWidthActual - x1;

			if (r != NULL)
			{
				sx_multiplier = width / (float)ScreenWidth;
				sy_multiplier = ScreenHeightActual / (float)ScreenHeight;
				sx = (int)((r->x * sx_multiplier) + x1);
				sy = (int)((ScreenHeight - (r->y + r->h)) * sy_multiplier);
			}
		}
		else
		{
			// screen is 4:3
			keep_aspect_ratio = FALSE;
		}
	}

	if (r != NULL)
	{
		if (!keep_aspect_ratio)
		{
			sx_multiplier = ScreenWidthActual / (float)ScreenWidth;
			sy_multiplier = ScreenHeightActual / (float)ScreenHeight;
			sx = (int)(r->x * sx_multiplier);
			sy = (int)((ScreenHeight - (r->y + r->h)) * sy_multiplier);
		}
		sw = (int)(r->w * sx_multiplier);
		sh = (int)(r->h * sy_multiplier);
		glScissor (sx, sy, sw, sh);
		glEnable (GL_SCISSOR_TEST);
	}
	tex1 = (ResFactor != HD ? 512.0f : 2048.0f);
	tex2 = (ResFactor != HD ? 256.0f : 1024.0f);
	glBegin (GL_TRIANGLE_FAN);
	glTexCoord2f (0, 0);
	glVertex2i (x1, y1);
	glTexCoord2f (ScreenWidth / tex1, 0);
	glVertex2i (x2, y1);	
	glTexCoord2f (ScreenWidth / tex1, ScreenHeight / tex2);
	glVertex2i (x2, y2);
	glTexCoord2f (0, ScreenHeight / tex2);
	glVertex2i (x1, y2);
	glEnd ();
	if (r != NULL)
	{
		glDisable (GL_SCISSOR_TEST);
	}
}

static void
TFB_GL_Preprocess (int force_full_redraw, int transition_amount, int fade_amount)
{
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glOrtho (0,ScreenWidthActual,ScreenHeightActual, 0, -1, 1);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	if (optKeepAspectRatio)
		glClear (GL_COLOR_BUFFER_BIT);

	(void) transition_amount;
	(void) fade_amount;

	if (force_full_redraw == TFB_REDRAW_YES)
	{
		GL_Screens[TFB_SCREEN_MAIN].updated.x = 0;
		GL_Screens[TFB_SCREEN_MAIN].updated.y = 0;
		GL_Screens[TFB_SCREEN_MAIN].updated.w = ScreenWidth;
		GL_Screens[TFB_SCREEN_MAIN].updated.h = ScreenHeight;
		GL_Screens[TFB_SCREEN_MAIN].dirty = TRUE;
	}
	else if (TFB_BBox.valid)
	{
		GL_Screens[TFB_SCREEN_MAIN].updated.x = TFB_BBox.region.corner.x;
		GL_Screens[TFB_SCREEN_MAIN].updated.y = TFB_BBox.region.corner.y;
		GL_Screens[TFB_SCREEN_MAIN].updated.w = TFB_BBox.region.extent.width;
		GL_Screens[TFB_SCREEN_MAIN].updated.h = TFB_BBox.region.extent.height;
		GL_Screens[TFB_SCREEN_MAIN].dirty = TRUE;
	}
}

static void
TFB_GL_Unscaled_ScreenLayer (SCREEN screen, Uint8 a, SDL_Rect *rect)
{
	glBindTexture (GL_TEXTURE_2D, GL_Screens[screen].texture);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (GL_Screens[screen].dirty)
	{
		int PitchWords = SDL_Screens[screen]->pitch / 4;
		glPixelStorei (GL_UNPACK_ROW_LENGTH, PitchWords);
		/* Matrox OpenGL drivers do not handle GL_UNPACK_SKIP_*
		   correctly */
		glPixelStorei (GL_UNPACK_SKIP_ROWS, 0);
		glPixelStorei (GL_UNPACK_SKIP_PIXELS, 0);
		SDL_LockSurface (SDL_Screens[screen]);
		glTexSubImage2D (GL_TEXTURE_2D, 0, GL_Screens[screen].updated.x, 
				GL_Screens[screen].updated.y,
				GL_Screens[screen].updated.w, 
				GL_Screens[screen].updated.h,
				GL_RGBA, GL_UNSIGNED_BYTE,
				(Uint32 *)SDL_Screens[screen]->pixels +
					(GL_Screens[screen].updated.y * PitchWords + 
					GL_Screens[screen].updated.x));
		SDL_UnlockSurface (SDL_Screens[screen]);
		GL_Screens[screen].dirty = FALSE;
	}

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ScreenFilterMode);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ScreenFilterMode);
	glEnable (GL_TEXTURE_2D);

	if (a == 255)
	{
		glDisable (GL_BLEND);
		glColor4f (1, 1, 1, 1);
	}
	else
	{
		float a_f = a / 255.0f;
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable (GL_BLEND);
		glColor4f (1, 1, 1, a_f);
	}

	TFB_GL_DrawQuad (rect, RESOLUTION_FACTOR);
}

static void
TFB_GL_Scaled_ScreenLayer (SCREEN screen, Uint8 a, SDL_Rect *rect)
{
	glBindTexture (GL_TEXTURE_2D, GL_Screens[screen].texture);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	if (GL_Screens[screen].dirty)
	{
		int PitchWords = GL_Screens[screen].scaled->pitch / 4;
		scaler (SDL_Screens[screen], GL_Screens[screen].scaled, &GL_Screens[screen].updated);
		glPixelStorei (GL_UNPACK_ROW_LENGTH, PitchWords);

		 /* Matrox OpenGL drivers do not handle GL_UNPACK_SKIP_*
		    correctly */
		glPixelStorei (GL_UNPACK_SKIP_ROWS, 0);
		glPixelStorei (GL_UNPACK_SKIP_PIXELS, 0);
		SDL_LockSurface (GL_Screens[screen].scaled);
		glTexSubImage2D (GL_TEXTURE_2D, 0, GL_Screens[screen].updated.x * 2, 
				GL_Screens[screen].updated.y * 2,
				GL_Screens[screen].updated.w * 2, 
				GL_Screens[screen].updated.h * 2,
				GL_RGBA, GL_UNSIGNED_BYTE,
				(Uint32 *)GL_Screens[screen].scaled->pixels +
				(GL_Screens[screen].updated.y * 2 * PitchWords + 
				GL_Screens[screen].updated.x * 2));
		SDL_UnlockSurface (GL_Screens[screen].scaled);
		GL_Screens[screen].dirty = FALSE;
	}

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ScreenFilterMode);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ScreenFilterMode);
	glEnable (GL_TEXTURE_2D);

	if (a == 255)
	{
		glDisable (GL_BLEND);
		glColor4f (1, 1, 1, 1);
	}
	else
	{
		float a_f = a / 255.0f;
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable (GL_BLEND);
		glColor4f (1, 1, 1, a_f);
	}
	
	TFB_GL_DrawQuad (rect, 0);
}

static void
TFB_GL_ColorLayer (Uint8 r, Uint8 g, Uint8 b, Uint8 a, SDL_Rect *rect)
{
	float r_f = r / 255.0f;
	float g_f = g / 255.0f;
	float b_f = b / 255.0f;
	float a_f = a / 255.0f;
	glColor4f(r_f, g_f, b_f, a_f);

	glDisable (GL_TEXTURE_2D);
	if (a != 255)
	{
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable (GL_BLEND);
	}
	else
	{
		glDisable (GL_BLEND);
	}
	
	TFB_GL_DrawQuad (rect, 0);
}

static void
TFB_GL_Postprocess (void)
{
	if (GfxFlags & TFB_GFXFLAGS_SCANLINES)
		TFB_GL_ScanLines ();

	SDL_GL_SwapBuffers ();
}	

#endif
