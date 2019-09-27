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

/* CDP module sample
 * .abx speech track decoder
 */

#ifndef ABXADEC_ABXAUD_H_
#define ABXADEC_ABXAUD_H_

typedef enum
{
	// positive values are the same as in errno
	abxa_ErrNone = 0,
	abxa_ErrUnknown = -1,
	abxa_ErrBadFile = -2,
	abxa_ErrBadArg = -3,
	abxa_ErrOther = -1000,
} abxa_Error;

#endif // ABXADEC_ABXAUD_H_
