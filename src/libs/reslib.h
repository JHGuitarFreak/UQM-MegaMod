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

#ifndef LIBS_RESLIB_H_
#define LIBS_RESLIB_H_

//#include <stdio.h>
#include "libs/compiler.h"
#include "port.h"
#include "libs/memlib.h"
#include "libs/uio.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct resource_index_desc RESOURCE_INDEX_DESC;
typedef RESOURCE_INDEX_DESC *RESOURCE_INDEX;

typedef const char *RESOURCE;

typedef union {
	DWORD num;
	void *ptr;
	const char *str;
} RESOURCE_DATA;

#define NULL_RESOURCE NULL

extern const char *_cur_resfile_name;

typedef void (ResourceLoadFun) (const char *pathname, RESOURCE_DATA *resdata);
typedef BOOLEAN (ResourceFreeFun) (void *handle);
typedef void (ResourceStringFun) (RESOURCE_DATA *handle, char *buf, unsigned int size);
				  
typedef void *(ResourceLoadFileFun) (uio_Stream *fp, DWORD len);

void *LoadResourceFromPath(const char *pathname, ResourceLoadFileFun fn);

uio_Stream *res_OpenResFile (uio_DirHandle *dir, const char *filename, const char *mode);
size_t ReadResFile (void *lpBuf, size_t size, size_t count, uio_Stream *fp);
size_t WriteResFile (const void *lpBuf, size_t size, size_t count, uio_Stream *fp);
int GetResFileChar (uio_Stream *fp);
int PutResFileChar (char ch, uio_Stream *fp);
int PutResFileNewline (uio_Stream *fp);
long SeekResFile (uio_Stream *fp, long offset, int whence);
long TellResFile (uio_Stream *fp);
size_t LengthResFile (uio_Stream *fp);
BOOLEAN res_CloseResFile (uio_Stream *fp);
BOOLEAN DeleteResFile (uio_DirHandle *dir, const char *filename);

RESOURCE_INDEX InitResourceSystem (void);
void UninitResourceSystem (void);
BOOLEAN InstallResTypeVectors (const char *res_type, ResourceLoadFun *loadFun, ResourceFreeFun *freeFun, ResourceStringFun *stringFun);
void *res_GetResource (RESOURCE res);
void *res_DetachResource (RESOURCE res);
void res_FreeResource (RESOURCE res);
COUNT CountResourceTypes (void);
DWORD res_GetIntResource (RESOURCE res);
BOOLEAN res_GetBooleanResource (RESOURCE res);
const char *res_GetResourceType (RESOURCE res);

void LoadResourceIndex (uio_DirHandle *dir, const char *filename, const char *prefix);
void SaveResourceIndex (uio_DirHandle *dir, const char *rmpfile, const char *root, BOOLEAN strip_root);

void *GetResourceData (uio_Stream *fp, DWORD length);

#define AllocResourceData HMalloc
BOOLEAN FreeResourceData (void *);

#if defined(__cplusplus)
}
#endif

#include "libs/strlib.h"
#include "libs/gfxlib.h"

#if defined(__cplusplus)
extern "C" {
#endif
		// For Color

typedef STRING_TABLE DIRENTRY_REF;
typedef STRING DIRENTRY;

extern DIRENTRY_REF LoadDirEntryTable (uio_DirHandle *dirHandle,
		const char *path, const char *pattern, match_MatchType matchType);
#define CaptureDirEntryTable CaptureStringTable
#define ReleaseDirEntryTable ReleaseStringTable
#define DestroyDirEntryTable DestroyStringTable
#define GetDirEntryTableRef GetStringTable
#define GetDirEntryTableCount GetStringTableCount
#define GetDirEntryTableIndex GetStringTableIndex
#define SetAbsDirEntryTableIndex SetAbsStringTableIndex
#define SetRelDirEntryTableIndex SetRelStringTableIndex
#define GetDirEntryLength GetStringLengthBin
#define GetDirEntryAddress GetStringAddress

/* Key-Value resources */

BOOLEAN res_HasKey (const char *key);

BOOLEAN res_IsString (const char *key);
const char *res_GetString (const char *key);
void res_PutString (const char *key, const char *value);

BOOLEAN res_IsInteger (const char *key);
int res_GetInteger (const char *key);
void res_PutInteger (const char *key, int value);

BOOLEAN res_IsBoolean (const char *key);
BOOLEAN res_GetBoolean (const char *key);
void res_PutBoolean (const char *key, BOOLEAN value);

BOOLEAN res_IsColor (const char *key);
Color res_GetColor (const char *key);
void res_PutColor (const char *key, Color value);

BOOLEAN res_Remove (const char *key);

#if defined(__cplusplus)
}
#endif

#endif /* LIBS_RESLIB_H_ */
