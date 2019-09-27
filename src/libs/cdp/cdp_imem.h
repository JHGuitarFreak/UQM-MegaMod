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
 * CDP Memory Interface 
 */

#ifndef LIBS_CDP_CDP_IMEM_H_
#define LIBS_CDP_CDP_IMEM_H_

#include "types.h"
#include "libs/memlib.h"

// CDP Memory Interface entry points
typedef struct
{
	void* (* malloc) (int size);
	void (* free) (void *p);
	void* (* calloc) (int size);
	void* (* realloc) (void *p, int size);

} cdp_Itf_MemoryVtbl_v1;

// the following are for the sake of module writers
typedef cdp_Itf_MemoryVtbl_v1 cdp_Itf_MemoryVtbl;
typedef cdp_Itf_MemoryVtbl    cdp_Itf_Memory;

#endif  /* LIBS_CDP_CDP_IMEM_H_ */
