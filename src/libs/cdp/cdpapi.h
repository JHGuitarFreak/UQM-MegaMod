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
 * CDP API declarations
 * the API is used by both the engine and modules
 */

#ifndef LIBS_CDP_CDPAPI_H_
#define LIBS_CDP_CDPAPI_H_

#include "types.h"

typedef enum
{
	CDPAPI_VERSION_1 = 0x00000001, // version 0.1
	
	CDPAPI_VERSION     = CDPAPI_VERSION_1,
	CDPAPI_VERSION_MIN = CDPAPI_VERSION_1,

} cdp_ApiVersion;

typedef enum
{
	CDPERR_NONE        = 0,
	CDPERR_UKNOWN      = 1,
	CDPERR_NOT_FOUND   = 2,
	CDPERR_BAD_MODULE  = 3,
	CDPERR_OLD_VER     = 4,
	CDPERR_UNKNOWN_VER = 5,
	CDPERR_TOO_MANY    = 6,
	CDPERR_INIT_FAILED = 7,
	CDPERR_NO_ITF      = 8,
	CDPERR_DUPE_ITF    = 9,
	CDPERR_NO_EVENT    = 10,
	CDPERR_DUPE_EVENT  = 11,
	CDPERR_OTHER       = 1000,

} cdp_Error;

typedef struct cdp_Module cdp_Module;
typedef void cdp_Itf;
typedef struct cdp_ItfReg cdp_ItfReg;

// Interface KINDs - for convinience and uniformity
#define CDPITF_KIND_INVALID   NULL
#define CDPITF_KIND_HOST      "UQM.Host"
#define CDPITF_KIND_MEMORY    "UQM.Memory"
#define CDPITF_KIND_IO        "UQM.IO"
#define CDPITF_KIND_THREADS   "UQM.Threads"
#define CDPITF_KIND_TIME      "UQM.Time"
#define CDPITF_KIND_INPUT     "UQM.Input"
#define CDPITF_KIND_TASK      "UQM.Task"
#define CDPITF_KIND_RESOURCE  "UQM.Resource"
#define CDPITF_KIND_SOUND     "UQM.Sound"
#define CDPITF_KIND_VIDEO     "UQM.Video"
#define CDPITF_KIND_GFX       "UQM.Gfx"
#define CDPITF_KIND_MIXER     "UQM.Mixer"

// Interface definition structure
// pass an array of these to Host->GetItfs() for batch lookup
// pass an array of these to Host->RegisterItfs() for batch registration
typedef struct
{
	// fill in the first 4 members for batch registration
	// fill in the 1st member for batch lookup
	// terminate an array of these defs with name == NULL
	const char* name;        // interface ID
	cdp_Itf* itf;            // interface pointer
	cdp_ApiVersion ver_from; // lowest supported version
	cdp_ApiVersion ver_to;   // highest supported version

	cdp_Module* module;      // owner module
	// the following member is only set during registration
	cdp_ItfReg* reg;         // registration handle (not set on lookup)

} cdp_ItfDef;

typedef unsigned int cdp_Event;
typedef struct cdp_EventReg cdp_EventReg;
typedef intptr_t cdp_EventResult;

#define CDP_EVENT_INVALID  (-1)
		// used with cdp_Event

typedef cdp_EventResult (* cdp_EventProc)
			(cdp_Event, uint32, void*, bool* pbHandled);

// Event definition structure
// pass an array of these to Host->GetItfs() for batch lookup
typedef struct
{
	// fill in the 1st member for batch lookup or registration
	// also fill in the 2nd member for batch subscription
	// terminate an array of these defs with name == NULL
	const char* name;        // event ID
	cdp_EventProc proc;      // event proc, set to NULL for no bind

	cdp_Event event;         // subscribable event handle
	cdp_Module* module;      // owner module
	// the following member is only set during registration
	cdp_EventReg* reg;       // registration handle (not set on lookup)

} cdp_EventDef;

// Host Interface
// the main itf of the API, it is passed to a loaded module
// module does everything else through this itf and itfs
// acquired through this itf
typedef struct
{
	uint32 (* GetApiVersion) (void);
	uint32 (* GetVersion) (void);
	cdp_Error (* GetApiError) (void);
	cdp_Itf* (* GetItf) (const char* name);
	bool (* GetItfs) (cdp_ItfDef* defs);
	cdp_ItfReg* (* RegisterItf) (const char* name,
			cdp_ApiVersion ver_from, cdp_ApiVersion ver_to,
			cdp_Itf*, cdp_Module*);
	void (* UnregisterItf) (cdp_ItfReg*);
	bool (* RegisterItfs) (cdp_ItfDef* defs, cdp_Module*);
	void (* UnregisterItfs) (cdp_ItfDef* defs);
	cdp_Event (* GetEvent) (const char* name);
	bool (* GetEvents) (cdp_EventDef* defs);
	cdp_EventReg* (* RegisterEvent) (const char* name, cdp_Module*);
	void (* UnregisterEvent) (cdp_EventReg*);
	bool (* RegisterEvents) (cdp_EventDef* defs, cdp_Module*);
	void (* UnregisterEvents) (cdp_EventDef* defs);
	bool (* SubscribeEvent) (cdp_Event, cdp_EventProc, cdp_Module*);
	void (* UnsubscribeEvent) (cdp_Event, cdp_EventProc);
	bool (* SubscribeEvents) (cdp_EventDef* defs, cdp_Module*);
	void (* UnsubscribeEvents) (cdp_EventDef* defs);
	cdp_EventResult (* FireEvent) (cdp_EventReg*, uint32, void*);

} cdp_Itf_HostVtbl_v1;

typedef cdp_Itf_HostVtbl_v1 cdp_Itf_HostVtbl;
typedef cdp_Itf_HostVtbl    cdp_Itf_Host;

#endif  /* LIBS_CDP_CDPAPI_H_ */
