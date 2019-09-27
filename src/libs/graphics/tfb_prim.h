// Copyright Michael Martin, 2003

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

#include "libs/gfxlib.h"
#include "tfb_draw.h"


void TFB_Prim_Line (LINE *, Color, DrawMode, POINT ctxOrigin);
void TFB_Prim_Point (POINT *, Color, DrawMode, POINT ctxOrigin);
void TFB_Prim_Rect (RECT *, Color, DrawMode, POINT ctxOrigin);
void TFB_Prim_FillRect (RECT *, Color, DrawMode, POINT ctxOrigin);
void TFB_Prim_Stamp (STAMP *, DrawMode, POINT ctxOrigin);
void TFB_Prim_StampFill (STAMP *, Color, DrawMode, POINT ctxOrigin);
void TFB_Prim_FontChar (POINT charOrigin, TFB_Char *fontChar,
		TFB_Image *backing, DrawMode, POINT ctxOrigin);
