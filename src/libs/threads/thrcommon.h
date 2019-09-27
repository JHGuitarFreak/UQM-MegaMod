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

#ifndef LIBS_THREADS_THRCOMMON_H_
#define LIBS_THREADS_THRCOMMON_H_


#if defined(THREADLIB_SDL)
#	include "sdl/sdlthreads.h"
#elif defined(THREADLIB_PTHREAD)
#	include "pthread/posixthreads.h"
#endif  /* defined(THREADLIB_PTHREAD) */


#endif  /* _THR_COMMON_H */
