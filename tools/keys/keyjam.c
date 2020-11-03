#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <SDL2_gfxPrimitives.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
#define BUFFERSIZE 1024
#define MAX_KEYS 32

static SDL_Keycode keystate[MAX_KEYS];
static char held_keys[BUFFERSIZE];
static char videoname[BUFFERSIZE];


static void
InitHeldKeys (void)
{
	int i;
	for (i = 0; i < MAX_KEYS; ++i)
	{
		keystate[i] = 0;
	}
}

static void
AddHeldKey (SDL_Keycode key)
{
	int i;
	for (i = 0; i < MAX_KEYS; ++i)
	{
		if (keystate[i] == key)
		{
			return;
		}
		if (keystate[i] == 0)
		{
			keystate[i] = key;
			return;
		}
	}
}

static void
RemoveHeldKey (SDL_Keycode key)
{
	int i;
	for (i = 0; i < MAX_KEYS; ++i)
	{
		if (keystate[i] == key)
		{
			int j;
			for (j = i + 1; j < MAX_KEYS; ++j)
			{
				keystate[j - 1] = keystate[j];
			}
			keystate[MAX_KEYS - 1] = 0;
		}
	}
}

static void
DrawCenteredText (SDL_Renderer *r, int y, const char *c)
{
	int w = strlen (c) * 8;
	
	stringRGBA (r, (SCREEN_WIDTH - w) / 2, y, c, 0xc0, 0xc0, 0xc0, 0xff);
}

static void
DrawScreen (SDL_Renderer *s)
{	
	int y;
	SDL_SetRenderDrawColor (s, 0, 0, 0x80, 0xFF);
	SDL_RenderClear (s);
	DrawCenteredText (s, 8, "Key Jamming");

	y = 32;

	DrawCenteredText (s, y, "Many keyboards have hardware that does not permit them to send");
	y += 8;
	DrawCenteredText (s, y, "arbitrary keychords to the computer.  This can complicate games");
	y += 8;
	DrawCenteredText (s, y, "with keyboard control, as activating certain controls may lock out");
	y += 8;
	DrawCenteredText (s, y, "others.  This application lets you pre-test control combinations so");
	y += 8;
	DrawCenteredText (s, y, "as to avoid such conflicts.");

	y += 16;
	DrawCenteredText (s, y, "Test your keyboard by pressing key combinations.  The keyboard's");
	y += 8;
	DrawCenteredText (s, y, "status will be reported in the bottom half of the screen.  When");
	y += 8;
	DrawCenteredText (s, y, "configuring keys for a game, do not select keys that do not all");
	y += 8;
	DrawCenteredText (s, y, "register when simultaneously pressed.");

	y += 16;
	DrawCenteredText (s, y, "You are using the following SDL2 rendering engine:");
	y += 8;
	DrawCenteredText (s, y, videoname);

	y += 16;
	DrawCenteredText (s, y, "Press ESCAPE to exit.");

	DrawCenteredText (s, 300, held_keys);

	SDL_RenderPresent (s);
}

int 
main (int argc, char *argv[])
{        
	SDL_Event event;
	SDL_Window *window;
	SDL_Renderer *screen;
	SDL_RendererInfo renderer_info;
	int quit = 0;
	int i, changed;
        
	/* Initialise SDL */
	if (SDL_Init (SDL_INIT_VIDEO) < 0)
	{
		fprintf (stderr, "Could not initialise SDL: %s\n", SDL_GetError ());
		exit (-1);
	}

	/* Set a video mode */
	if (SDL_CreateWindowAndRenderer (SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &screen))
	{
		fprintf (stderr, "Could not set create window/renderer: %s\n", SDL_GetError ());
		SDL_Quit ();
		exit (-1);
	}

	SDL_SetWindowTitle (window, "Key Jammer");
	if (!SDL_GetRendererInfo (screen, &renderer_info))
	{
		strncpy (videoname, renderer_info.name, BUFFERSIZE);
	}
	else
	{
		strncpy (videoname, "<unknown>", BUFFERSIZE);
	}
	videoname[BUFFERSIZE-1] = 0;

	InitHeldKeys ();

	DrawScreen (screen);

	/* Loop until an SDL_QUIT event is found */
	while (!quit)
	{
		changed = 0;
		/* Poll for events */
		while (SDL_PollEvent (&event))
		{
			switch(event.type)
			{
			case SDL_KEYDOWN:
				AddHeldKey (event.key.keysym.sym);
				if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					quit = 1;
				}
				changed = 1;
				break;
			case SDL_KEYUP:
				RemoveHeldKey (event.key.keysym.sym);
				changed = 1;
				break;
			case SDL_QUIT:
				quit = 1;
				break;
			default:
				break;
			}
		}
		if (changed)
		{
			held_keys[0] = '\0';
			for (i = 0; i < MAX_KEYS; i++)
			{
				if (keystate[i])
				{
					if (held_keys[0])
					{
						strcat (held_keys, " | ");
					}
					strcat (held_keys, SDL_GetKeyName (keystate[i]));
				}
				if (strlen (held_keys) > 200)
				{
					break;
				}
			}
			for (i = 0; i < BUFFERSIZE; i++)
			{
				if (!held_keys[i])
				{
					break;
				}
				held_keys[i] = toupper (held_keys[i]);
			}
			DrawScreen (screen);
		}
		SDL_Delay (20);
	}

	/* Clean up */
	SDL_DestroyRenderer (screen);
	SDL_DestroyWindow (window);
	SDL_Quit ();
	exit (0);
}
