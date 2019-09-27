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
 * CDP common internal definitions
 */

#ifndef LIBS_CDP_CDPINT_H_
#define LIBS_CDP_CDPINT_H_

#include "cdpapi.h"
#include "cdp_imem.h"
#include "cdp_iio.h"
#include "cdp_isnd.h"
#include "cdp_ivid.h"

#ifdef WIN32
#	define CDPEXT ".dll"
#else
#	define CDPEXT ".so"
#endif

extern cdp_Itf_HostVtbl_v1 cdp_host_itf_v1;
extern cdp_Itf_MemoryVtbl_v1 cdp_memory_itf_v1;
extern cdp_Itf_IoVtbl_v1 cdp_io_itf_v1;
extern cdp_Itf_SoundVtbl_v1 cdp_sound_itf_v1;

bool cdp_InitApi (void);
void cdp_UninitApi (void);
cdp_Error cdp_GetApiError (void);
cdp_Itf* cdp_GetInterface (const char* name, cdp_ApiVersion);
cdp_ItfReg* cdp_GetInterfaceReg (const char* name, cdp_ApiVersion);

#endif  /* _CDPISND_H */
