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
 * CDP API definitions
 * the API is used by both the engine and modules
 */

#include "cdp.h"
#include "port.h"
#include "cdpint.h"
#include "uqmversion.h"

#define MAX_REG_ITFS   255
#define MAX_REG_EVENTS 1023

static cdp_Error cdp_api_error = CDPERR_NONE;

static uint32 cdp_Host_GetApiVersion (void);
static uint32 cdp_Host_GetVersion (void);
static cdp_Error cdp_Host_GetApiError (void);
static cdp_Itf* cdp_Host_GetItf (const char* name);
static bool cdp_Host_GetItfs (cdp_ItfDef* defs);
static cdp_ItfReg* cdp_Host_RegisterItf (const char* name,
			cdp_ApiVersion ver_from, cdp_ApiVersion ver_to,
			cdp_Itf*, cdp_Module*);
static void cdp_Host_UnregisterItf (cdp_ItfReg*);
static bool cdp_Host_RegisterItfs (cdp_ItfDef* defs, cdp_Module*);
static void cdp_Host_UnregisterItfs (cdp_ItfDef* defs);
static cdp_Event cdp_Host_GetEvent (const char* name);
static bool cdp_Host_GetEvents (cdp_EventDef* defs);
static cdp_EventReg* cdp_Host_RegisterEvent (const char* name, cdp_Module*);
static void cdp_Host_UnregisterEvent (cdp_EventReg*);
static bool cdp_Host_RegisterEvents (cdp_EventDef* defs, cdp_Module*);
static void cdp_Host_UnregisterEvents (cdp_EventDef* defs);
static bool cdp_Host_SubscribeEvent (cdp_Event, cdp_EventProc, cdp_Module*);
static void cdp_Host_UnsubscribeEvent (cdp_Event, cdp_EventProc);
static bool cdp_Host_SubscribeEvents (cdp_EventDef* defs, cdp_Module*);
static void cdp_Host_UnsubscribeEvents (cdp_EventDef* defs);
static cdp_EventResult cdp_Host_FireEvent (cdp_EventReg*, uint32, void*);

// Interfaces
cdp_Itf_HostVtbl_v1 cdp_host_itf_v1 = 
{
	cdp_Host_GetApiVersion,
	cdp_Host_GetVersion,
	cdp_Host_GetApiError,
	cdp_Host_GetItf,
	cdp_Host_GetItfs,
	cdp_Host_RegisterItf,
	cdp_Host_UnregisterItf,
	cdp_Host_RegisterItfs,
	cdp_Host_UnregisterItfs,
	cdp_Host_GetEvent,
	cdp_Host_GetEvents,
	cdp_Host_RegisterEvent,
	cdp_Host_UnregisterEvent,
	cdp_Host_RegisterEvents,
	cdp_Host_UnregisterEvents,
	cdp_Host_SubscribeEvent,
	cdp_Host_UnsubscribeEvent,
	cdp_Host_SubscribeEvents,
	cdp_Host_UnsubscribeEvents,
	cdp_Host_FireEvent,
};

cdp_Itf_MemoryVtbl_v1 cdp_memory_itf_v1 =
{
	HMalloc,
	HFree,
	HCalloc,
	HRealloc,
};

cdp_Itf_IoVtbl_v1 cdp_io_itf_v1 = 
{
	uio_fopen,
	uio_fclose,
	uio_fread,
	uio_fwrite,
	uio_fseek,
	uio_ftell,
	uio_fflush,
	uio_feof,
	uio_ferror,
};

cdp_Itf_SoundVtbl_v1 cdp_sound_itf_v1 = 
{
	SoundDecoder_Register,
	SoundDecoder_Unregister,
	SoundDecoder_Lookup,
};

cdp_Itf_VideoVtbl_v1 cdp_video_itf_v1 = 
{
	VideoDecoder_Register,
	VideoDecoder_Unregister,
	VideoDecoder_Lookup,
};

// the actual interface registration struct/handle
struct cdp_ItfReg
{
	bool builtin;
	bool used;
	const char* name;
	cdp_ApiVersion ver_from;
	cdp_ApiVersion ver_to;
	cdp_Itf* itfvtbl;
	cdp_Module* module;
};

#define CDP_DECLARE_ITF(kind,vf,vt,vtbl) \
	{true, true, CDPITF_KIND_##kind, \
	CDPAPI_VERSION_##vf, CDPAPI_VERSION_##vt, vtbl, NULL}

// Built-in interfaces + space for loadable
cdp_ItfReg cdp_itfs[MAX_REG_ITFS + 1] =
{
	CDP_DECLARE_ITF (HOST,   1, 1, &cdp_host_itf_v1),
	CDP_DECLARE_ITF (MEMORY, 1, 1, &cdp_memory_itf_v1),
	CDP_DECLARE_ITF (IO,     1, 1, &cdp_io_itf_v1),
	CDP_DECLARE_ITF (SOUND,  1, 1, &cdp_sound_itf_v1),
	CDP_DECLARE_ITF (VIDEO,  1, 1, &cdp_video_itf_v1),
	// TODO: put newly defined built-in interfaces here

	{false, false, "", 0, 0, NULL} // term
};

// event bind descriptor
typedef struct
{
	cdp_EventProc proc;
	cdp_Module* module;

} cdp_EventBind;

#define EVENT_BIND_GROW  16

// the actual event registration struct/handle
struct cdp_EventReg
{
	bool builtin;
	bool used;
	const char* name;
	cdp_EventBind* binds;
	uint32 bindslots;
	cdp_Module* module;
};

#define CDP_DECLARE_EVENT(name) \
	{true, true, "UQM." #name, NULL, 0, NULL}

// Built-in events + space for loadable
// a cdp_Event handle is an index into this array
cdp_EventReg cdp_evts[MAX_REG_EVENTS + 1] =
{
	// sample - no real events defined yet
	CDP_DECLARE_EVENT (PlanetSide.TouchDown),
	CDP_DECLARE_EVENT (PlanetSide.LiftOff),
	// TODO: put newly defined built-in events here

	{false, false, "", NULL, 0, NULL} // term
};

cdp_Error
cdp_GetApiError (void)
{
	cdp_Error ret = cdp_api_error;
	cdp_api_error = CDPERR_NONE;
	return ret;
}

bool
cdp_InitApi (void)
{
	int i;
	cdp_Module* kernel;
	
	// preprocess built-in itfs
	kernel = cdp_LoadModule (NULL);

	for (i = 0; cdp_itfs[i].builtin; ++i)
	{
		cdp_itfs[i].module = kernel;
	}
	// clear the rest
	//memset (cdp_itfs + i, 0,
	//		sizeof (cdp_itfs) - sizeof (cdp_ItfReg) * i);

	for (i = 0; cdp_evts[i].builtin; ++i)
	{
		cdp_evts[i].module = kernel;
		cdp_evts[i].bindslots = 0;
		cdp_evts[i].binds = NULL;
	}

	return true;
}

void
cdp_UninitApi (void)
{
	cdp_ItfReg* itf;

	// unregister custom interfaces
	for (itf = cdp_itfs; itf->used; ++itf)
	{
		if (!itf->builtin)
		{
			itf->used = false;
			if (itf->name)
				HFree (itf->name);
			itf->name = NULL;
			itf->itfvtbl = NULL;
			itf->module = NULL;
		}
	}
}

static uint32
cdp_Host_GetApiVersion (void)
{
	return CDPAPI_VERSION;
}

static uint32
cdp_Host_GetVersion (void)
{
	return (UQM_MAJOR_VERSION << 20) | (UQM_MINOR_VERSION << 15) |
			UQM_PATCH_VERSION;
}

static cdp_Error
cdp_Host_GetApiError (void)
{
	return cdp_GetApiError ();
}

static char*
cdp_MakeContextName (const char* ctx, const char* name)
{
	int namelen;
	char* id_name;

	namelen = strlen(ctx) + strlen(name) + 2;
	id_name = HMalloc (namelen);
	strcpy(id_name, ctx);
	strcat(id_name, ".");
	strcat(id_name, name);

	return id_name;
}

/***********************************************************
 *  Interface system                                       *
 ***********************************************************/

cdp_ItfReg*
cdp_GetInterfaceReg (const char* name, cdp_ApiVersion api_ver)
{
	cdp_ItfReg* itf;

	for (itf = cdp_itfs; itf->used &&
			(!itf->name || strcasecmp(itf->name, name) != 0 ||
			 api_ver < itf->ver_from || api_ver > itf->ver_to);
			itf++)
		;
	if (!itf->name)
	{
		cdp_api_error = CDPERR_NO_ITF;
		return NULL;
	}
	
	return itf;
}

cdp_Itf*
cdp_GetInterface (const char* name, cdp_ApiVersion api_ver)
{
	cdp_ItfReg* reg;

	reg = cdp_GetInterfaceReg (name, api_ver);
	return reg ? reg->itfvtbl : NULL;
}

static cdp_Itf*
cdp_Host_GetItf (const char* name)
{
	return cdp_GetInterface (name, CDPAPI_VERSION_1);
}

static bool
cdp_Host_GetItfs (cdp_ItfDef* defs)
{
	cdp_ItfDef* def;
	cdp_ItfReg* reg;
	int errors = 0;

	for (def = defs; def->name; ++def)
	{
		// registration handle is not returned
		def->reg = NULL;

		reg = cdp_GetInterfaceReg (def->name, CDPAPI_VERSION_1);
		if (reg)
		{
			def->itf = reg->itfvtbl;
			def->name = reg->name; // set to cannonical name
			def->ver_from = reg->ver_from;
			def->ver_to = reg->ver_to;
			def->module = reg->module;
		}
		else
		{
			def->itf = NULL;
			def->module = NULL;
			def->ver_from = 0;
			def->ver_to = 0;
			++errors;
		}
	}

	return !errors;
}

static cdp_ItfReg*
cdp_Host_RegisterItf (const char* name, cdp_ApiVersion ver_from,
		cdp_ApiVersion ver_to, cdp_Itf* itfvtbl,
		cdp_Module* owner)
{
	cdp_ItfReg* itfreg;
	cdp_ItfReg* newslot = NULL;
	char* id_name;
	const char* ctx;

	if (!owner)
	{
		fprintf (stderr, "cdp_Host_RegisterItf(): "
				"No owner info supplied\n");
		//return NULL;
	}
	if (!name || !*name || !itfvtbl)
	{
		fprintf (stderr, "cdp_Host_RegisterItf(): "
				"Null or invalid interface (from %s)\n",
				cdp_GetModuleName (owner, true));
		return NULL;
	}
	ctx = cdp_GetModuleContext (owner, false);
	if (!ctx)
	{
		fprintf (stderr, "cdp_Host_RegisterItf(): "
				"Null or invalid context (from %s)\n",
				cdp_GetModuleName (owner, true));
		return NULL;
	}

	// TODO: review version policy (below)
	// enforce version policy and do not allow obsolete interfaces
	// POLICY: all modules MUST be aware of recent API changes and will not
	// be allowed to expose interfaces that support and/or utilize obsoleted
	// API versions
	if (ver_from < CDPAPI_VERSION_MIN)
		ver_from = CDPAPI_VERSION_MIN;
	if (ver_to < CDPAPI_VERSION_MIN)
	{
		fprintf (stderr, "cdp_Host_RegisterItf(): "
				"Obsolete interface %s (from %s)\n",
				name, cdp_GetModuleName (owner, true));
		return NULL;
	}

	id_name = cdp_MakeContextName (ctx, name);

	// check if interface already registered
	for (itfreg = cdp_itfs; itfreg->used &&
			(!itfreg->name || strcasecmp(itfreg->name, id_name) != 0 ||
			 ver_from < itfreg->ver_from || ver_to > itfreg->ver_to);
			++itfreg)
	{
		// and pick up an empty slot (where available)
		if (!newslot && !itfreg->name)
			newslot = itfreg;
	}

	if (itfreg >= cdp_itfs + MAX_REG_ITFS)
	{
		fprintf (stderr, "cdp_Host_RegisterItf(): "
				"Interfaces limit reached\n");
		HFree (id_name);
		return NULL;
	}
	else if (itfreg->name)
	{
		fprintf (stderr, "cdp_Host_RegisterItf(): "
				"Interface %s already registered for these versions, "
				"%s denied\n",
				name, cdp_GetModuleName (owner, true));
		HFree (id_name);
		return NULL;
	}
	
	if (!newslot)
	{
		newslot = itfreg;
		newslot->used = true;
		// make next one a term
		itfreg[1].builtin = false;
		itfreg[1].used = false;
		itfreg[1].name = NULL;
		itfreg[1].itfvtbl = NULL;
	}

	newslot->name = id_name;
	newslot->ver_from = ver_from;
	newslot->ver_to = ver_to;
	newslot->itfvtbl = itfvtbl;
	newslot->module = owner;
	
	return newslot;
}

static void
cdp_Host_UnregisterItf (cdp_ItfReg* itfreg)
{
	if (itfreg < cdp_itfs || itfreg >= cdp_itfs + MAX_REG_ITFS ||
			!itfreg->name || !itfreg->itfvtbl)
	{
		fprintf (stderr, "cdp_Host_UnregisterItf(): "
				"Invalid or expired interface passed\n");
		return;
	}

	if (!itfreg->builtin)
	{
		HFree (itfreg->name);
	}
	itfreg->module = NULL;
	itfreg->name = NULL;
	itfreg->itfvtbl = NULL;
}

static bool
cdp_Host_RegisterItfs (cdp_ItfDef* defs, cdp_Module* owner)
{
	cdp_ItfDef* def;
	int errors = 0;

	for (def = defs; def->name; ++def)
	{
		def->reg = cdp_Host_RegisterItf (def->name, def->ver_from,
				def->ver_to, def->itf, owner);
		if (def->reg)
		{
			def->module = owner;
		}
		else
		{
			def->module = NULL;
			++errors;
		}
	}

	return !errors;
}

static void
cdp_Host_UnregisterItfs (cdp_ItfDef* defs)
{
	cdp_ItfDef* def;

	for (def = defs; def->name; ++def)
	{
		if (def->reg)
			cdp_Host_UnregisterItf (def->reg);
	}
}

/***********************************************************
 *  Event system                                           *
 ***********************************************************/

cdp_EventReg*
cdp_GetEventReg (const char* name)
{
	cdp_EventReg* evt;

	for (evt = cdp_evts; evt->used &&
			(!evt->name || strcasecmp(evt->name, name) != 0);
			evt++)
		;
	if (!evt->name)
	{
		cdp_api_error = CDPERR_NO_EVENT;
		return NULL;
	}
	
	return evt;
}

// hopefully inlinable
static cdp_Event
cdp_EventFromReg (cdp_EventReg* reg)
{
	return (reg - cdp_evts) / sizeof (cdp_EventReg);
}

// hopefully inlinable
static cdp_EventReg*
cdp_RegFromEvent (cdp_Event event)
{
	return cdp_evts + event;
}

cdp_Event
cdp_GetEvent (const char* name)
{
	cdp_EventReg* reg;

	reg = cdp_GetEventReg (name);
	return reg ? cdp_EventFromReg (reg) : CDP_EVENT_INVALID;
}

static cdp_EventBind*
cdp_AllocEventBinds (cdp_EventBind* binds, uint32 ccur, uint32 cnew)
{
	cdp_EventBind* newbinds;
	uint32 newsize;

	newsize = cnew * sizeof (cdp_EventBind);
	if (binds)
		newbinds = HRealloc (binds, newsize);
	else
		newbinds = HMalloc (newsize);

	if (cnew > ccur)
		memset (newbinds + ccur, 0,
				(cnew - ccur) * sizeof (cdp_EventBind));

	return newbinds;
}

static cdp_Event
cdp_Host_GetEvent (const char* name)
{
	return cdp_GetEvent (name);
}

static bool
cdp_Host_GetEvents (cdp_EventDef* defs)
{
	cdp_EventDef* def;
	cdp_EventReg* reg;
	int errors = 0;

	for (def = defs; def->name; ++def)
	{
		// registration handle is not returned
		def->reg = NULL;

		reg = cdp_GetEventReg (def->name);
		if (reg)
		{
			def->event = cdp_EventFromReg(reg);
			def->name = reg->name; // set to cannonical name
			def->module = reg->module;
		}
		else
		{
			def->event = CDP_EVENT_INVALID;
			def->module = NULL;
			++errors;
		}
	}

	return !errors;
}

static cdp_EventReg*
cdp_Host_RegisterEvent (const char* name, cdp_Module* owner)
{
	cdp_EventReg* evtreg;
	cdp_EventReg* newslot = NULL;
	char* id_name;
	const char* ctx;

	if (!owner)
	{
		fprintf (stderr, "cdp_Host_RegisterEvent(): "
				"No owner info supplied\n");
		//return NULL;
	}
	if (!name || !*name)
	{
		fprintf (stderr, "cdp_Host_RegisterEvent(): "
				"Null or invalid event (from %s)\n",
				cdp_GetModuleName (owner, true));
		return NULL;
	}
	ctx = cdp_GetModuleContext (owner, false);
	if (!ctx)
	{
		fprintf (stderr, "cdp_Host_RegisterEvent(): "
				"Null or invalid context (from %s)\n",
				cdp_GetModuleName (owner, true));
		return NULL;
	}

	id_name = cdp_MakeContextName (ctx, name);

	// check if event already registered
	for (evtreg = cdp_evts; evtreg->used &&
			(!evtreg->name || strcasecmp(evtreg->name, id_name) != 0);
			++evtreg)
	{
		// and pick up an empty slot (where available)
		if (!newslot && !evtreg->name)
			newslot = evtreg;
	}

	if (evtreg >= cdp_evts + MAX_REG_EVENTS)
	{
		fprintf (stderr, "cdp_Host_RegisterEvent(): "
				"Event limit reached\n");
		HFree (id_name);
		return NULL;
	}
	else if (evtreg->name)
	{
		fprintf (stderr, "cdp_Host_RegisterEvent(): "
				"Event %s already registered, "
				"%s denied\n",
				name, cdp_GetModuleName (owner, true));
		HFree (id_name);
		return NULL;
	}
	
	if (!newslot)
	{
		newslot = evtreg;
		newslot->used = true;
		// make next one a term
		evtreg[1].builtin = false;
		evtreg[1].used = false;
		evtreg[1].name = NULL;
	}

	newslot->name = id_name;
	newslot->module = owner;
	newslot->binds = NULL;
	newslot->bindslots = 0;
	
	return newslot;
}

static void
cdp_Host_UnregisterEvent (cdp_EventReg* evtreg)
{
	if (evtreg < cdp_evts || evtreg >= cdp_evts + MAX_REG_EVENTS ||
			!evtreg->name)
	{
		fprintf (stderr, "cdp_Host_UnregisterEvent(): "
				"Invalid or expired event passed\n");
		return;
	}

	if (!evtreg->builtin)
	{
		HFree (evtreg->name);
	}
	evtreg->module = NULL;
	evtreg->name = NULL;
	if (evtreg->binds)
		HFree (evtreg->binds);
	evtreg->binds = NULL;
	evtreg->bindslots = 0;
}

static bool
cdp_Host_RegisterEvents (cdp_EventDef* defs, cdp_Module* owner)
{
	cdp_EventDef* def;
	int errors = 0;

	for (def = defs; def->name; ++def)
	{
		def->reg = cdp_Host_RegisterEvent (def->name, owner);
		if (def->reg)
		{
			def->module = owner;
		}
		else
		{
			def->module = NULL;
			++errors;
		}
	}

	return !errors;
}

static void
cdp_Host_UnregisterEvents (cdp_EventDef* defs)
{
	cdp_EventDef* def;

	for (def = defs; def->name; ++def)
	{
		if (def->reg)
			cdp_Host_UnregisterEvent (def->reg);
	}
}

static bool
cdp_Host_SubscribeEvent (cdp_Event event, cdp_EventProc proc, cdp_Module* module)
{
	cdp_EventReg* reg = cdp_RegFromEvent (event);
	cdp_EventBind* bind = NULL;
	uint32 i;

	if (reg < cdp_evts || reg >= cdp_evts + MAX_REG_EVENTS ||
			!reg->name)
	{
		fprintf (stderr, "cdp_Host_SubscribeEvent(): "
				"Invalid or expired event passed\n");
		return false;
	}

	if (reg->binds)
	{
		// check for duplicate or find a new slot
		for (i = 0, bind = reg->binds; i < reg->bindslots &&
				(!bind->proc || bind->proc != proc);
				++i, ++bind)
			;
		if (i >= reg->bindslots)
		{	// full - add more slots
			reg->binds = cdp_AllocEventBinds (reg->binds,
					reg->bindslots, reg->bindslots + EVENT_BIND_GROW);
			bind = reg->binds + reg->bindslots;
			reg->bindslots += EVENT_BIND_GROW;
		}
		else if (bind->proc == proc)
		{	// already bound
			return true;
		}
	}
	else
	{
		reg->binds = cdp_AllocEventBinds (NULL, 0, EVENT_BIND_GROW);
		reg->bindslots = EVENT_BIND_GROW;
		bind = reg->binds;
	}

	bind->proc = proc;
	bind->module = module;

	return true;
}

static void
cdp_Host_UnsubscribeEvent (cdp_Event event, cdp_EventProc proc)
{
	cdp_EventReg* reg = cdp_RegFromEvent (event);
	cdp_EventBind* bind = NULL;
	uint32 i;

	if (reg < cdp_evts || reg >= cdp_evts + MAX_REG_EVENTS ||
			!reg->name)
	{	// event either expired or invalid
		return;
	}

	if (!reg->binds || !reg->bindslots)
		return; // hmm, no bindings

	// check for duplicate or find a new slot
	for (i = 0, bind = reg->binds; i < reg->bindslots &&
			bind->proc != proc;
			++i, ++bind)
		;
	if (i >= reg->bindslots)
		return; // binding not found

	bind->proc = NULL;
	bind->module = NULL;
}

static bool
cdp_Host_SubscribeEvents (cdp_EventDef* defs, cdp_Module* module)
{
	cdp_EventDef* def;
	int errors = 0;

	for (def = defs; def->name; ++def)
	{
		if (def->event != CDP_EVENT_INVALID && def->proc)
			if (!cdp_Host_SubscribeEvent (def->event, def->proc, module))
				++errors;
	}
	return !errors;
}

static void
cdp_Host_UnsubscribeEvents (cdp_EventDef* defs)
{
	cdp_EventDef* def;

	for (def = defs; def->name; ++def)
	{
		if (def->event != CDP_EVENT_INVALID && def->proc)
			cdp_Host_UnsubscribeEvent (def->event, def->proc);
	}
}

static cdp_EventResult
cdp_Host_FireEvent (cdp_EventReg* evtreg, uint32 iparam, void* pparam)
{
	bool bHandled = false;
	cdp_EventResult ret = 0;
	cdp_Event event;
	cdp_EventBind* bind;
	uint32 i;

	if (evtreg < cdp_evts || evtreg >= cdp_evts + MAX_REG_EVENTS ||
			!evtreg->name)
	{
#ifdef DEBUG
		fprintf (stderr, "cdp_Host_FireEvent(): Invalid event\n");
#endif
		return 0;
	}

	if (!evtreg->binds)
		return 0; // no subscribers

	event = cdp_EventFromReg (evtreg);

	// call event procs in opposite order of binding
	for (i = evtreg->bindslots, bind = evtreg->binds + i - 1;
			!bHandled && i > 0;
			--i, --bind)
	{
		if (bind->proc)
			ret = bind->proc (event, iparam, pparam, &bHandled);
	}
	return ret;
}
