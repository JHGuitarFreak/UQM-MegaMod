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

#include "mthintrn.h"

COUNT
square_root (DWORD value)
{
	UWORD sig_word, mask;
	COUNT result, shift;

	if ((sig_word = HIWORD (value)) > 0)
	{
		DWORD mask_squared, result_shift;

		for (mask = 1 << 15, shift = 31;
				!(mask & sig_word); mask >>= 1, --shift)
			;
		shift >>= 1;
		mask = 1 << shift;

		result = mask;
		mask_squared = result_shift = (DWORD)mask << shift;
		value -= mask_squared;
		while (mask >>= 1)
		{
			DWORD remainder;

			mask_squared >>= 1;
			mask_squared >>= 1;
			if ((remainder = result_shift + mask_squared) > value)
				result_shift >>= 1;
			else
			{
				value -= remainder;

				result_shift = (result_shift >> 1) + mask_squared;

				result |= mask;
			}
		}

		return (result);
	}
	else if ((sig_word = LOWORD (value)) > 0)
	{
		UWORD mask_squared, result_shift;

		for (mask = 1 << 15, shift = 15;
				!(mask & sig_word); mask >>= 1, --shift)
			;
		shift >>= 1;
		mask = 1 << shift;

		result = mask;
		mask_squared = result_shift = mask << shift;
		sig_word -= mask_squared;
		while (mask >>= 1)
		{
			UWORD remainder;

			mask_squared >>= 1;
			mask_squared >>= 1;
			if ((remainder = result_shift + mask_squared) > sig_word)
				result_shift >>= 1;
			else
			{
				sig_word -= remainder;

				result_shift = (result_shift >> 1) + mask_squared;

				result |= mask;
			}
		}

		return (result);
	}

	return (0);
}


