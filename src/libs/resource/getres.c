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

#include "options.h"
#include "port.h"
#include "resintrn.h"
#include "libs/memlib.h"
#include "libs/log.h"
#include "libs/uio/charhashtable.h"

const char *_cur_resfile_name;
// When a file is being loaded, _cur_resfile_name is set to its name.
// At other times, it is NULL.

ResourceDesc *
lookupResourceDesc (RESOURCE_INDEX idx, RESOURCE res)
{
	return (ResourceDesc *) CharHashTable_find (idx->map, res);
}

void
loadResourceDesc (ResourceDesc *desc)
{
	desc->vtable->loadFun (desc->fname, &desc->resdata);
}

void *
LoadResourceFromPath (const char *path, ResourceLoadFileFun *loadFun)
{
	uio_Stream *stream;
	unsigned long dataLen;
	void *resdata;

	stream = res_OpenResFile (contentDir, path, "rb");
	if (stream == NULL)
	{
		log_add (log_Warning, "Warning: Can't open '%s'", path);
		return NULL;
	}

	dataLen = LengthResFile (stream);
	log_add (log_Info, "\t'%s' -- %lu bytes", path, dataLen);
	
	if (dataLen == 0)
	{
		log_add (log_Warning, "Warning: Trying to load empty file '%s'.", path);
		goto err;
	}

	_cur_resfile_name = path;
	resdata = (*loadFun) (stream, dataLen);
	_cur_resfile_name = NULL;
	res_CloseResFile (stream);

	return resdata;

err:
	res_CloseResFile (stream);
	return NULL;
}

const char *
res_GetResourceType (RESOURCE res)
{
	RESOURCE_INDEX resourceIndex;
	ResourceDesc *desc;
	
	if (res == NULL_RESOURCE)
	{
		log_add (log_Warning, "Trying to get type of null resource");
		return NULL;
	}
	
	resourceIndex = _get_current_index_header ();
	desc = lookupResourceDesc (resourceIndex, res);
	if (desc == NULL)
	{
		log_add (log_Warning, "Trying to get type of undefined resource '%s'",
				res);
		return NULL;
	}
	
	return desc->vtable->resType;
}
	

// Get a resource by its resource ID.
void *
res_GetResource (RESOURCE res)
{
	RESOURCE_INDEX resourceIndex;
	ResourceDesc *desc;
	
	if (res == NULL_RESOURCE)
	{
		log_add (log_Warning, "Trying to get null resource");
		return NULL;
	}
	
	resourceIndex = _get_current_index_header ();

	desc = lookupResourceDesc (resourceIndex, res);
	if (desc == NULL)
	{
		log_add (log_Warning, "Trying to get undefined resource '%s'",
				 res);
		return NULL;
	}

	if (desc->resdata.ptr == NULL)
		loadResourceDesc (desc);
	if (desc->resdata.ptr != NULL)
		++desc->refcount;

	return desc->resdata.ptr;
			// May still be NULL, if the load failed.
}

DWORD
res_GetIntResource (RESOURCE res)
{
	RESOURCE_INDEX resourceIndex;
	ResourceDesc *desc;
	
	if (res == NULL_RESOURCE)
	{
		log_add (log_Warning, "Trying to get null resource");
		return 0;
	}
	
	resourceIndex = _get_current_index_header ();

	desc = lookupResourceDesc (resourceIndex, res);
	if (desc == NULL)
	{
		log_add (log_Warning, "Trying to get undefined resource '%s'",
				 res);
		return 0;
	}

	return desc->resdata.num;
}

BOOLEAN
res_GetBooleanResource (RESOURCE res)
{
	return (res_GetIntResource (res) != 0);
}

// NB: this function appears to be never called!
void
res_FreeResource (RESOURCE res)
{
	ResourceDesc *desc;
	ResourceFreeFun *freeFun;

	desc = lookupResourceDesc (_get_current_index_header(), res);
	if (desc == NULL)
	{
		log_add (log_Debug, "Warning: trying to free an unrecognised "
				"resource.");
		return;
	}

	if (desc->refcount > 0)
		--desc->refcount;
	else
		log_add (log_Debug, "Warning: freeing an unreferenced resource.");
	if (desc->refcount > 0)
		return; // Still references left

	freeFun = desc->vtable->freeFun;
	if (freeFun == NULL)
	{
		log_add (log_Debug, "Warning: trying to free a non-heap resource.");
		return;
	}
	
	if (desc->resdata.ptr == NULL)
	{
		log_add (log_Debug, "Warning: trying to free not loaded "
				"resource.");
		return;
	}

	(*freeFun) (desc->resdata.ptr);
	desc->resdata.ptr = NULL;
}

// By calling this function the caller will be responsible of unloading
// the resource. If res_GetResource() get called again for this
// resource, a NEW copy will be loaded, regardless of whether a detached
// copy still exists.
void *
res_DetachResource (RESOURCE res)
{
	ResourceDesc *desc;
	ResourceFreeFun *freeFun;
	void *result;

	desc = lookupResourceDesc (_get_current_index_header(), res);
	if (desc == NULL)
	{
		log_add (log_Debug, "Warning: trying to detach from an unrecognised "
				"resource.");
		return NULL;
	}
	
	freeFun = desc->vtable->freeFun;
	if (freeFun == NULL)
	{
		log_add (log_Debug, "Warning: trying to detach from a non-heap resource.");
		return NULL;
	}
	
	if (desc->resdata.ptr == NULL)
	{
		log_add (log_Debug, "Warning: trying to detach from a not loaded "
				"resource.");
		return NULL;
	}

	if (desc->refcount > 1)
	{
		log_add (log_Debug, "Warning: trying to detach a resource referenced "
				"%u times", desc->refcount);
		return NULL;
	}

	result = desc->resdata.ptr;
	desc->resdata.ptr = NULL;
	desc->refcount = 0;

	return result;
}

BOOLEAN
FreeResourceData (void *data)
{
	HFree (data);
	return TRUE;
}
