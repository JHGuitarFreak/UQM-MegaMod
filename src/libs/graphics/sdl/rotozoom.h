/*  

  rotozoom.h - rotozoomer for 32bit or 8bit surfaces
  LGPL (c) A. Schiffler
 
  Note by sc2 developers:
  Taken from SDL_gfx library and modified, original code can be downloaded
  from http://www.ferzkopp.net/Software/SDL_gfx-2.0/

*/


#ifndef ROTOZOOM_H
#define ROTOZOOM_H

#include <math.h>
#ifndef M_PI
#define M_PI	3.141592654
#endif

#include "port.h"
#include SDL_INCLUDE(SDL.h)


/* ---- Defines */

#define SMOOTHING_OFF		0
#define SMOOTHING_ON		1

/* ---- Structures */

typedef struct tColorRGBA {
	Uint8 r;
	Uint8 g;
	Uint8 b;
	Uint8 a;
} tColorRGBA;

typedef struct tColorY {
	Uint8 y;
} tColorY;


/* ---- Prototypes */

/*
 zoomSurfaceRGBA()

 Zoom the src surface into dst.  The zoom amount is determined
 by the dimensions of src and dst

*/
int zoomSurfaceRGBA(SDL_Surface * src, SDL_Surface * dst, int smooth);

/* 
 
 rotozoomSurface()

 Rotates and zoomes a 32bit or 8bit 'src' surface to newly created 'dst' surface.
 'angle' is the rotation in degrees. 'zoom' a scaling factor. If 'smooth' is 1
 then the destination 32bit surface is anti-aliased. If the surface is not 8bit
 or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

*/

SDL_Surface *rotozoomSurface(SDL_Surface * src, double angle, double zoom,
							 int smooth);

int rotateSurface(SDL_Surface * src, SDL_Surface * dst, double angle,
				  int smooth);

/* Returns the size of the target surface for a rotozoomSurface() call */

void rotozoomSurfaceSize(int width, int height, double angle, double zoom,
						 int *dstwidth, int *dstheight);

/* 
 
 zoomSurface()

 Zoomes a 32bit or 8bit 'src' surface to newly created 'dst' surface.
 'zoomx' and 'zoomy' are scaling factors for width and height. If 'smooth' is 1
 then the destination 32bit surface is anti-aliased. If the surface is not 8bit
 or 32bit RGBA/ABGR it will be converted into a 32bit RGBA format on the fly.

*/

SDL_Surface *zoomSurface(SDL_Surface * src, double zoomx, double zoomy,
						 int smooth);

/* Returns the size of the target surface for a zoomSurface() call */

void zoomSurfaceSize(int width, int height, double zoomx, double zoomy,
					 int *dstwidth, int *dstheight);

#endif
