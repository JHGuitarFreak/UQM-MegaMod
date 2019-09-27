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
 * CDP library declarations
 */

#ifndef LIBS_CDP_CDP_H_
#define LIBS_CDP_CDP_H_

#include "types.h"
#include "cdpapi.h"

// these will be called by the UQM engine
// and plugins manager
bool cdp_Init (void);
void cdp_Uninit (void);
cdp_Error cdp_GetError (void);
cdp_Module* cdp_LoadModule (const char* modname);
void cdp_FreeModule (cdp_Module* module);
// in the following calls when bMetaString is set
// function will never return a NULL, instead it will
// return a valid string -- error meta-string
const char* cdp_GetModuleContext (cdp_Module* module, bool bMetaString);
const char* cdp_GetModuleName (cdp_Module* module, bool bMetaString);
uint32 cdp_GetModuleVersion (cdp_Module* module);
const char* cdp_GetModuleVersionString (cdp_Module* module, bool bMetaString);
const char* cdp_GetModuleComment (cdp_Module* module, bool bMetaString);

int cdp_LoadAllModules (void);
void cdp_FreeAllModules (void);

#endif  /* LIBS_CDP_CDP_H_ */
