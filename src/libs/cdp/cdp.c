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
 * CDP library definitions
 */

#include <string.h>
#include <stdio.h>
#include "cdp.h"
#include "port.h"
#include "cdpint.h"
#include "cdpmod.h"
#include "uio.h"
#include "uqmversion.h"
#ifdef WIN32
#	include "windl.h"
#else
#	include <dlfcn.h>
#endif

#define MAX_CDPS 63
#define CDPDIR "cdps"

// internal CDP module representation
struct cdp_Module
{
	bool builtin;         // used at least once indicator
	bool used;            // used at least once indicator
	void* hmodule;        // loaded module handle
	uint32 refcount;      // reference count
	cdp_ModuleInfo* info; // cdp exported info

};

// Kernel module info
// not a real module, and not loadable either
// this just provides information to other modules
cdp_ModuleInfo cdp_kernel_info =
{
	sizeof (cdp_ModuleInfo),
	CDPAPI_VERSION,              // API version we are using
	UQM_MAJOR_VERSION, UQM_MINOR_VERSION, UQM_PATCH_VERSION,
	UQM_MAJOR_VERSION, UQM_MINOR_VERSION, UQM_PATCH_VERSION,
	CDP_MODINFO_RESERVED1,
	"UQM",                       // CDP context cannonical name
	"UQM Kernel",                // CDP mod name
#	define S(i) #i
	// CDP mod version
	S(UQM_MAJOR_VERSION) "." S(UQM_MINOR_VERSION) UQM_EXTRA_VERSION,
#	undef S
	"UQM Team",                  // CDP mod author
	"http://sc2.sf.net",         // CDP mod URL
	"Eternal doctrine executor", // CDP mod comment
	CDP_MODINFO_RESERVED2,
	NULL, NULL                   // no entrypoints defined/needed
};

static cdp_Module cdp_modules[MAX_CDPS + 1] = 
{
	{true,  true,  NULL, 1, &cdp_kernel_info},

	{false, false, NULL, 0, NULL} // term
};

extern uio_DirHandle *cdpDir;

static bool cdp_inited = false;
static cdp_Error cdp_last_error = CDPERR_NONE;
static char cdp_path[PATH_MAX] = "";

cdp_Error
cdp_GetError (void)
{
	cdp_Error ret = cdp_last_error;
	cdp_last_error = CDPERR_NONE;
	return ret;
}

bool
cdp_Init (void)
{
	int i;
	void* hkernel;

	if (cdp_inited)
	{
		fprintf (stderr, "cdp_Init(): called when already inited\n");
		return true;
	}
	
	// preprocess built-in modules
	hkernel = dlopen (NULL, RTLD_LAZY);
	
	for (i = 0; cdp_modules[i].builtin; ++i)
		cdp_modules[i].hmodule = hkernel;
	
	// clear the rest
	//memset (cdp_modules + i, 0,
	//		sizeof (cdp_modules) - sizeof (cdp_Module) * i);
	
	//strcpy (cdp_path, cdpDir->path);
	cdp_InitApi ();
	cdp_inited = true;

	return true;
}

void
cdp_Uninit (void)
{
	if (!cdp_inited)
	{
		fprintf (stderr, "cdp_Uninit(): called when not inited\n");
		return;
	}
	
	cdp_UninitApi ();
	cdp_FreeAllModules ();
	cdp_inited = false;
}

cdp_Module*
cdp_LoadModule (const char* modname)
		// special value for modname: NULL - refers to kernel (UQM exe)
{
	void* mod;
	char modpath[PATH_MAX];
	const char* errstr;
	cdp_ModuleInfo* info;
	int i;
	cdp_Module* cdp;
	cdp_Module* newslot = 0;
	cdp_Itf* ihost;

	if (modname == NULL)
		return cdp_modules;

	if (!cdp_inited)
	{
		fprintf (stderr, "cdp_LoadModule(): called when not inited\n");
		return 0;
	}
	
	// load dynamic lib
	sprintf (modpath, "%s/%s%s", CDPDIR, modname, CDPEXT);
	mod = dlopen (modpath, RTLD_NOW);
	if (!mod)
	{
		cdp_last_error = CDPERR_NOT_FOUND;
		return NULL;
	}

	// look it up in already loaded
	for (i = 0, cdp = cdp_modules; cdp->used && cdp->hmodule != mod;
			++cdp, ++i)
	{
		// and pick up an empty slot (where available)
		if (!newslot && !cdp->hmodule)
			newslot = cdp;
	}
	if (i >= MAX_CDPS)
	{
		fprintf (stderr, "cdp_LoadModule(): "
				"CDPs limit reached while loading %s\n",
				modname);
		dlclose (mod);
		cdp_last_error = CDPERR_TOO_MANY;
		return NULL;
	}

	if (cdp->hmodule)
	{	// module has already been loaded
		cdp->refcount++;
		return cdp;
	}

	dlerror ();	// clear any error
	info = dlsym (mod, CDP_INFO_SYM_NAME);
	if (!info && (errstr = dlerror ()))
	{
		dlclose (mod);
		cdp_last_error = CDPERR_BAD_MODULE;
		return NULL;
	}
	
	if (info->size < CDP_MODINFO_MIN_SIZE || info->api_ver > CDPAPI_VERSION)
	{
		fprintf (stderr, "cdp_LoadModule(): "
				"CDP %s is invalid or newer API version\n",
				modname);
		dlclose (mod);
		cdp_last_error = CDPERR_UNKNOWN_VER;
		return NULL;
	}

	ihost = cdp_GetInterface (CDPITF_KIND_HOST, info->api_ver);
	if (!ihost)
	{
		fprintf (stderr, "cdp_LoadModule(): "
				"CDP %s requested unsupported API version 0x%08x\n",
				modname, info->api_ver);
		dlclose (mod);
		cdp_last_error = CDPERR_UNKNOWN_VER;
		return NULL;
	}

	if (!newslot)
	{
		newslot = cdp;
		newslot->used = true;
		// make next one a term
		cdp[1].builtin = false;
		cdp[1].used = false;
		cdp[1].hmodule = NULL;
		cdp[1].refcount = 0;
	}
	newslot->hmodule = mod;
	newslot->refcount = 1;
	newslot->info = info;

	if (!info->module_init (newslot, (cdp_Itf_Host*)ihost))
	{
		fprintf (stderr, "cdp_LoadModule(): "
				"CDP %s failed to init\n",
				modname);
		dlclose (mod);
		newslot->hmodule = NULL;
		newslot->info = NULL;
		newslot->refcount = 0;
		cdp_last_error = CDPERR_INIT_FAILED;
		return NULL;
	}

	
	return newslot;
}

cdp_Module*
cdp_CheckModule (cdp_Module* module)
{
	if (module < cdp_modules || module >= cdp_modules + MAX_CDPS ||
			!module->hmodule || !module->info)
		return NULL;
	else
		return module;
}

void
cdp_FreeModule (cdp_Module* module)
{
	cdp_Module* modslot = cdp_CheckModule (module);

	if (!modslot || modslot->builtin)
		return;

	modslot->refcount--;
	if (modslot->refcount == 0)
		modslot->info->module_term ();
	
	dlclose (modslot->hmodule);

	if (modslot->refcount == 0)
	{
		modslot->hmodule = NULL;
		modslot->info = NULL;
	}
}

const char*
cdp_GetModuleContext (cdp_Module* module, bool bMetaString)
{
	cdp_Module* modslot = cdp_CheckModule (module);
	if (bMetaString)
	{
		if (!modslot)
			return "(Error)";
		if (!modslot->info->context_name)
			return "(Null)";
	}
	else if (!modslot)
	{
		return NULL;
	}
	return modslot->info->context_name;
}

const char*
cdp_GetModuleName (cdp_Module* module, bool bMetaString)
{
	cdp_Module* modslot = cdp_CheckModule (module);
	if (bMetaString)
	{
		if (!modslot)
			return "(Error)";
		if (!modslot->info->name)
			return "(Null)";
	}
	else if (!modslot)
	{
		return NULL;
	}
	return modslot->info->name;
}

uint32
cdp_GetModuleVersion (cdp_Module* module)
{
	cdp_Module* modslot = cdp_CheckModule (module);
	if (!modslot)
		return 0;
	return (modslot->info->ver_major << 16) | modslot->info->ver_minor;
}

const char*
cdp_GetModuleVersionString (cdp_Module* module, bool bMetaString)
{
	cdp_Module* modslot = cdp_CheckModule (module);
	if (bMetaString)
	{
		if (!modslot)
			return "(Error)";
		if (!modslot->info->ver_string)
			return "(Null)";
	}
	else if (!modslot)
	{
		return NULL;
	}
	return modslot->info->ver_string;
}

const char*
cdp_GetModuleComment (cdp_Module* module, bool bMetaString)
{
	cdp_Module* modslot = cdp_CheckModule (module);
	if (bMetaString)
	{
		if (!modslot)
			return "(Error)";
		if (!modslot->info->comments)
			return "(Null)";
	}
	else if (!modslot)
	{
		return NULL;
	}
	return modslot->info->comments;
}

// load-all and free-all are here temporarily until
// configs are in place
int
cdp_LoadAllModules (void)
{
	uio_DirList *dirList;
	int nummods = 0;
	int i;

	if (!cdp_inited)
	{
		fprintf (stderr, "cdp_LoadAllModules(): called when not inited\n");
		return 0;
	}

	if (!cdpDir)
		return 0;

	fprintf (stderr, "Loading all CDPs...\n");

	dirList = uio_getDirList (cdpDir, "", CDPEXT, match_MATCH_SUFFIX);
	if (!dirList)
		return 0;

	for (i = 0; i < dirList->numNames; i++)
	{
		char modname[PATH_MAX];
		char* pext;
		cdp_Module* mod;

		fprintf (stderr, "Loading CDP %s...\n", dirList->names[i]);
		strcpy (modname, dirList->names[i]);
		pext = strrchr (modname, '.');
		if (pext) // strip extension
			*pext = 0;

		mod = cdp_LoadModule (modname);
		if (mod)
		{
			nummods++;
			fprintf (stderr, "\tloaded CDP: %s v%s (%s)\n",
					cdp_GetModuleName (mod, true),
					cdp_GetModuleVersionString (mod, true),
					cdp_GetModuleComment (mod, true));
		}
		else
		{
			fprintf (stderr, "\tload failed, error %u\n",
					cdp_GetError ());
		}
	}
	uio_freeDirList (dirList);

	return nummods;
}

void
cdp_FreeAllModules (void)
{
	cdp_Module* cdp;

	if (!cdp_inited)
	{
		fprintf (stderr, "cdp_FreeAllModules(): called when not inited\n");
		return;
	}
	
	for (cdp = cdp_modules; cdp->used; ++cdp)
	{
		if (!cdp->builtin && cdp->hmodule)
			cdp_FreeModule (cdp);
	}
}
