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

#ifndef SCALERS_H_
#define SCALERS_H_

void Scale_Init (void);

typedef void (* TFB_ScaleFunc) (SDL_Surface *src, SDL_Surface *dst,
				SDL_Rect *r);

TFB_ScaleFunc Scale_PrepPlatform (int flags, const SDL_PixelFormat* fmt);

#endif /* SCALERS_H_ */
