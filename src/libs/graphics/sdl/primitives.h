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

#ifndef PRIMITIVES_H
#define PRIMITIVES_H

/* Function types for the pixel functions */

typedef Uint32 (*GetPixelFn)(SDL_Surface *, int x, int y);
// 'pixel' is in destination surface format
typedef void (*PutPixelFn)(SDL_Surface *, int x, int y, Uint32 pixel);

GetPixelFn getpixel_for(SDL_Surface *surface);
PutPixelFn putpixel_for(SDL_Surface *surface);

// This currently matches gfxlib.h:DrawKind for simplicity
typedef enum
{
	renderReplace = 0,
	renderAdditive,
	renderAlpha,
} RenderKind;

#define FULLY_OPAQUE_ALPHA  255
#define ADDITIVE_FACTOR_1   255

// 'pixel' is in destination surface format
// See gfxlib.h:DrawKind for 'factor' spec
typedef void (*RenderPixelFn)(SDL_Surface *, int x, int y, Uint32 pixel,
		int factor);

RenderPixelFn renderpixel_for(SDL_Surface *surface, RenderKind);

void line_prim(int x1, int y1, int x2, int y2, Uint32 color,
		RenderPixelFn plot, int factor, SDL_Surface *dst);
void fillrect_prim(SDL_Rect r, Uint32 color,
		RenderPixelFn plot, int factor, SDL_Surface *dst);
void blt_prim(SDL_Surface *src, SDL_Rect src_r,
		RenderPixelFn plot, int factor,
		SDL_Surface *dst, SDL_Rect dst_r);

int clip_line(int *lx1, int *ly1, int *lx2, int *ly2, const SDL_Rect *clip_r);
int clip_rect(SDL_Rect *r, const SDL_Rect *clip_r);
int clip_blt_rects(SDL_Rect *src_r, SDL_Rect *dst_r, const SDL_Rect *clip_r);


#endif /* PRIMITIVES_H */
