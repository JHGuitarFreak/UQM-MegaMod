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

// See doc/devel/planettopo for details.

#include "libs/gfxlib.h"
#include "libs/mathlib.h"
#include "planets.h"

void
DeltaTopography (COUNT num_iterations, SBYTE *DepthArray, RECT *pRect,
		SIZE depth_delta)
{
	SIZE width, height, delta_y;
	struct
	{
		COORD x_top, x_bot;
		SIZE x_incr, delta_x, error_term;
	} LineDDA0, LineDDA1;

	width = pRect->extent.width;
	height = pRect->extent.height;
	delta_y = (height - 1) << 1;
	do
	{
		SIZE d;
		COUNT h, w1, w2;
		DWORD rand_val;
		SBYTE *lpDst;

		if ((RandomContext_Random (SysGenRNG) & 1) == 0)
			depth_delta = -depth_delta;

		rand_val = RandomContext_Random (SysGenRNG);
		w1 = LOWORD (rand_val);
		w2 = HIWORD (rand_val);

		LineDDA0.x_top = (LOBYTE (w1) % ORIGINAL_MAP_WIDTH) * width / ORIGINAL_MAP_WIDTH;
		LineDDA0.x_bot = (HIBYTE (w1) % ORIGINAL_MAP_WIDTH) * width / ORIGINAL_MAP_WIDTH;
		//LineDDA0.x_top = w1 % width; // JMS_GFX: Replaced previous lines with these: BYTE is too small for 640x480 sized maps.
		//LineDDA0.x_bot = w2 % width; // Using w1 and w2 to get difference between top and bottom.
		// BW: reinstate previous method and adapt it for higher res.
		// Overusing w1 and w2 was hampering randomness.
		// Now planets will look the same no matter what the res is.

		LineDDA0.delta_x = (LineDDA0.x_bot - LineDDA0.x_top) << 1;
		if (LineDDA0.delta_x >= 0)
			LineDDA0.x_incr = 1;
		else
		{
			LineDDA0.x_incr = -1;
			LineDDA0.delta_x = -LineDDA0.delta_x;
		}
		if (LineDDA0.delta_x > delta_y)
			LineDDA0.error_term = -(LineDDA0.delta_x >> 1);
		else
			LineDDA0.error_term = -(delta_y >> 1);

		LineDDA1.x_top = (LOBYTE (w2) % (ORIGINAL_MAP_WIDTH - 1)) * width / ORIGINAL_MAP_WIDTH + LineDDA0.x_top + 1;
		LineDDA1.x_bot = (HIBYTE (w2) % (ORIGINAL_MAP_WIDTH - 1)) * width / ORIGINAL_MAP_WIDTH + LineDDA0.x_bot + 1;
		LineDDA1.delta_x = (LineDDA1.x_bot - LineDDA1.x_top) << 1;
		if (LineDDA1.delta_x >= 0)
			LineDDA1.x_incr = 1;
		else
		{
			LineDDA1.x_incr = -1;
			LineDDA1.delta_x = -LineDDA1.delta_x;
		}
		if (LineDDA1.delta_x > delta_y)
			LineDDA1.error_term = -(LineDDA1.delta_x >> 1);
		else
			LineDDA1.error_term = -(delta_y >> 1);

		lpDst = &DepthArray[LineDDA0.x_top];
		h = height;
		do
		{
			COUNT w;

			w1 = LineDDA1.x_top - LineDDA0.x_top;
			w2 = width - w1;

			if ((int)(LineDDA0.x_top + w1) > (int)width)
				w = width - LineDDA0.x_top;
			else
			{
				w = w1;
				LineDDA0.x_top += w1;
			}
			w1 -= w;
			while (w--)
			{
				d = *lpDst + depth_delta;
				if (d >= -128 && d <= 127)
					*lpDst = (SBYTE)d;
				++lpDst;
			}
			if (w1 == 0)
			{
				if (LineDDA0.x_top == width)
				{
					LineDDA0.x_top = 0;
					lpDst -= width;
				}
			}
			else
			{
				LineDDA0.x_top = w1;
				lpDst -= width;
				do
				{
					d = *lpDst + depth_delta;
					if (d >= -128 && d <= 127)
						*lpDst = (SBYTE)d;
					++lpDst;
				} while (--w1);
			}

			if ((int)(LineDDA0.x_top + w2) > (int)width)
				w = width - LineDDA0.x_top;
			else
			{
				w = w2;
				LineDDA0.x_top += w2;
			}
			w2 -= w;
			while (w--)
			{
				d = *lpDst - depth_delta;
				if (d >= -128 && d <= 127)
					*lpDst = (SBYTE)d;
				++lpDst;
			}
			if (w2 == 0)
			{
				if (LineDDA0.x_top == width)
				{
					LineDDA0.x_top = 0;
					lpDst -= width;
				}
			}
			else
			{
				LineDDA0.x_top = w2;
				lpDst -= width;
				do
				{
					d = *lpDst - depth_delta;
					if (d >= -128 && d <= 127)
						*lpDst = (SBYTE)d;
					++lpDst;
				} while (--w2);
			}

			lpDst += pRect->extent.width;

			if (delta_y >= LineDDA0.delta_x)
			{
				if ((LineDDA0.error_term += LineDDA0.delta_x) >= 0)
				{
					lpDst += LineDDA0.x_incr;
					LineDDA0.x_top += LineDDA0.x_incr;
					LineDDA0.error_term -= delta_y;
				}
			}
			else
			{
				do
				{
					lpDst += LineDDA0.x_incr;
					LineDDA0.x_top += LineDDA0.x_incr;
				} while ((LineDDA0.error_term += delta_y) < 0);
				LineDDA0.error_term -= LineDDA0.delta_x;
			}

			if (delta_y >= LineDDA1.delta_x)
			{
				if ((LineDDA1.error_term += LineDDA1.delta_x) >= 0)
				{
					LineDDA1.x_top += LineDDA1.x_incr;
					LineDDA1.error_term -= delta_y;
				}
			}
			else
			{
				do
				{
					LineDDA1.x_top += LineDDA1.x_incr;
				} while ((LineDDA1.error_term += delta_y) < 0);
				LineDDA1.error_term -= LineDDA1.delta_x;
			}
		} while (--h);
	} while (--num_iterations);
}



