/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 * Nota bene: later versions of the GNU General Public License do not apply
 * to this program.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 * CDP dlopen() & Co. WIN32 implementation
 */

#ifndef LIBS_CDP_WINDL_H_
#define LIBS_CDP_WINDL_H_

#include "types.h"

extern void *dlopen (const char *filename, int flag);
extern void *dlsym (void *handle, const char *symbol);
extern int dlclose (void *handle);
extern char *dlerror (void);

/* these dlopen() flags are meaningless on win32 */
#define RTLD_LAZY	1	/* lazy function call binding */
#define RTLD_NOW	2	/* immediate function call binding */
#define RTLD_GLOBAL	4	/* symbols in this dlopen'ed obj are visible to other dlopen'ed objs */

#endif  /* LIBS_CDP_WINDL_H_ */
