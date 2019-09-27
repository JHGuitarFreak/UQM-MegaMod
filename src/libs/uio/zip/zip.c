/*
 * Copyright (C) 2003  Serge van den Boom
 *
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
 * This file makes use of zlib (http://www.gzip.org/zlib/)
 *
 * References:
 * The .zip format description from PKWare:
 *   http://www.pkware.com/products/enterprise/white_papers/appnote.html
 * The .zip format description from InfoZip:
 *   ftp://ftp.info-zip.org/pub/infozip/doc/appnote-011203-iz.zip
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "zip.h"
#include "../physical.h"
#include "../uioport.h"
#include "../paths.h"
#include "../uioutils.h"
#ifdef uio_MEM_DEBUG
#	include "../memdebug.h"
#endif


#define DIR_STRUCTURE_READ_BUFSIZE 0x10000

static int zip_badFile(zip_GPFileData *gPFileData, char *fileName);
static int zip_fillDirStructure(uio_GPDir *top, uio_Handle *handle);
#if zip_USE_HEADERS == zip_USE_LOCAL_HEADERS
static int zip_fillDirStructureLocal(uio_GPDir *top, uio_Handle *handle);
static int zip_fillDirStructureLocalProcessEntry(uio_GPDir *topGPDir,
		uio_FileBlock *fileBlock, off_t *pos);
#endif
#if zip_USE_HEADERS == zip_USE_CENTRAL_HEADERS
static off_t zip_findEndOfCentralDirectoryRecord(uio_Handle *handle,
		uio_FileBlock *fileBlock);
static int zip_fillDirStructureCentral(uio_GPDir *top, uio_Handle *handle);
static int zip_fillDirStructureCentralProcessEntry(uio_GPDir *topGPDir,
		uio_FileBlock *fileBlock, off_t *pos);
static int zip_updatePFileDataFromLocalFileHeader(zip_GPFileData *gPFileData,
		uio_FileBlock *fileBlock, int pos);
int zip_updateFileDataFromLocalHeader(uio_Handle *handle,
		zip_GPFileData *gPFileData);
#endif
static int zip_fillDirStructureProcessExtraFields(
		uio_FileBlock *fileBlock, off_t extraFieldLength,
		zip_GPFileData *gPFileData, const char *path, off_t pos,
		uio_bool central);
static inline int zip_foundFile(uio_GPDir *gPDir, const char *path,
		zip_GPFileData *gPFileData);
static inline int zip_foundDir(uio_GPDir *gPDir, const char *dirName,
		zip_GPDirData *gPDirData);
static int zip_initZipStream(z_stream *zipStream);
static int zip_unInitZipStream(z_stream *zipStream);
static int zip_reInitZipStream(z_stream *zipStream);

static voidpf zip_alloc(voidpf opaque, uInt items, uInt size);
static void zip_free(voidpf opaque, voidpf address);

static inline zip_GPFileData * zip_GPFileData_new(void);
static inline void zip_GPFileData_delete(zip_GPFileData *gPFileData);
static inline zip_GPFileData *zip_GPFileData_alloc(void);
static inline void zip_GPFileData_free(zip_GPFileData *gPFileData);
static inline void zip_GPDirData_delete(zip_GPDirData *gPDirData);
static inline void zip_GPDirData_free(zip_GPDirData *gPDirData);

static ssize_t zip_readStored(uio_Handle *handle, void *buf, size_t count);
static ssize_t zip_readDeflated(uio_Handle *handle, void *buf, size_t count);
static off_t zip_seekStored(uio_Handle *handle, off_t offset);
static off_t zip_seekDeflated(uio_Handle *handle, off_t offset);

uio_FileSystemHandler zip_fileSystemHandler = {
	/* .init    = */  NULL,
	/* .unInit  = */  NULL,
	/* .cleanup = */  NULL,

	/* .mount  = */  zip_mount,
	/* .umount = */  uio_GPRoot_umount,

	/* .access = */  zip_access,
	/* .close  = */  zip_close,
	/* .fstat  = */  zip_fstat,
	/* .stat   = */  zip_stat,
	/* .mkdir  = */  NULL,
	/* .open   = */  zip_open,
	/* .read   = */  zip_read,
	/* .rename = */  NULL,
	/* .rmdir  = */  NULL,
	/* .seek   = */  zip_seek,
	/* .write  = */  NULL,
	/* .unlink = */  NULL,

	/* .openEntries  = */  uio_GPDir_openEntries,
	/* .readEntries  = */  uio_GPDir_readEntries,
	/* .closeEntries = */  uio_GPDir_closeEntries,

	/* .getPDirEntryHandle     = */  uio_GPDir_getPDirEntryHandle,
	/* .deletePRootExtra       = */  uio_GPRoot_delete,
	/* .deletePDirHandleExtra  = */  uio_GPDirHandle_delete,
	/* .deletePFileHandleExtra = */  uio_GPFileHandle_delete,
};

uio_GPRoot_Operations zip_GPRootOperations = {
	/* .fillGPDir         = */  NULL,
	/* .deleteGPRootExtra = */  NULL,
	/* .deleteGPDirExtra  = */  zip_GPDirData_delete,
	/* .deleteGPFileExtra = */  zip_GPFileData_delete,
};


#define NUM_COMPRESSION_METHODS 11
/*
 *		[0] = stored uncompressed
 *		[1] = Shrunk
 *		[2] = Reduced with compression factor 1
 *		[3] = Reduced with compression factor 2
 *		[4] = Reduced with compression factor 3
 *		[5] = Reduced with compression factor 4
 *		[6] = Imploded
 *		[7] = Reserved for Tokenizing
 *		[8] = Deflated
 *		[9] = Deflate64
 *		[10] = "PKWARE Data Compression Library Imploding"
 */
static const uio_bool
		zip_compressionMethodSupported[NUM_COMPRESSION_METHODS] = {
		true, false, false, false, false, false, false, false,
		true, false, false };

typedef ssize_t (*zip_readFunctionType)(uio_Handle *handle,
		void *buf, size_t count);
zip_readFunctionType zip_readMethods[NUM_COMPRESSION_METHODS] = {
	zip_readStored, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	zip_readDeflated, NULL, NULL
};
typedef off_t (*zip_seekFunctionType)(uio_Handle *handle, off_t offset);
zip_seekFunctionType zip_seekMethods[NUM_COMPRESSION_METHODS] = {
	zip_seekStored, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	zip_seekDeflated, NULL, NULL
};


typedef enum {
	zip_OSType_FAT,
	zip_OSType_Amiga,
	zip_OSType_OpenVMS,
	zip_OSType_UNIX,
	zip_OSType_VMCMS,
	zip_OSType_AtariST,
	zip_OSType_HPFS,
	zip_OSType_HFS,
	zip_OSType_ZSystem,
	zip_OSType_CPM,
	zip_OSType_TOPS20,
	zip_OSType_NTFS,
	zip_OSType_QDOS,
	zip_OSType_Acorn,
	zip_OSType_VFAT,
	zip_OSType_MVS,
	zip_OSType_BeOS,
	zip_OSType_Tandem,
	
	zip_numOSTypes
} zip_OSType;

#if zip_USE_HEADERS == zip_USE_CENTRAL_HEADERS
static mode_t zip_makeFileMode(zip_OSType creatorOS, uio_uint32 modeBytes);
#endif

#define zip_INPUT_BUFFER_SIZE 0x10000
		// TODO: make this configurable a la sysctl?
#define zip_SEEK_BUFFER_SIZE zip_INPUT_BUFFER_SIZE


void
zip_close(uio_Handle *handle) {
	zip_Handle *zip_handle;

#if defined(DEBUG) && DEBUG > 1
	fprintf(stderr, "zip_close - handle=%p\n", (void *) handle);
#endif
	zip_handle = handle->native;
	uio_GPFile_unref(zip_handle->file);
	zip_unInitZipStream(&zip_handle->zipStream);
	uio_closeFileBlock(zip_handle->fileBlock);
	uio_free(zip_handle);
}

static void
zip_fillStat(struct stat *statBuf, const zip_GPFileData *gPFileData) {
	memset(statBuf, '\0', sizeof (struct stat));
	statBuf->st_size = gPFileData->uncompressedSize;
	statBuf->st_uid = gPFileData->uid;
	statBuf->st_gid = gPFileData->gid;
	statBuf->st_mode = gPFileData->mode;
	statBuf->st_atime = gPFileData->atime;
	statBuf->st_mtime = gPFileData->mtime;
	statBuf->st_ctime = gPFileData->ctime;
}

int
zip_access(uio_PDirHandle *pDirHandle, const char *name, int mode) {
	errno = ENOSYS;  // Not implemented.
	(void) pDirHandle;
	(void) name;
	(void) mode;
	return -1;

#if 0
	uio_GPDirEntry *entry;

	if (name[0] == '.' && name[1] == '\0') {
		entry = (uio_GPDirEntry *) pDirHandle->extra;
	} else {
		entry = uio_GPDir_getGPDirEntry(pDirHandle->extra, name);
		if (entry == NULL) {
			errno = ENOENT;
			return -1;
		}
	}

	if (mode & R_OK)
	{
		// Read permission is always granted. Nothing to check here.
	}
	
	if (mode & W_OK) {
		errno = EACCES;
		return -1;
	}

	if (mode & X_OK) {
		if (uio_GPDirEntry_isDir(entry)) {
			// Search permission on directories is always granted.
		} else {
			// WORK
#error
		}
	}

	if (mode & F_OK) {
		// WORK
#error
	}

	return 0;
#endif
}

int
zip_fstat(uio_Handle *handle, struct stat *statBuf) {
#if zip_USE_HEADERS == zip_USE_CENTRAL_HEADERS
	if (handle->native->file->extra->fileOffset == -1) {
		// The local header wasn't read in yet.
		if (zip_updateFileDataFromLocalHeader(handle->root->handle,
				handle->native->file->extra) == -1) {
			// errno is set
			return -1;
		}
	}
#endif  /* zip_USE_HEADERS == zip_USE_CENTRAL_HEADERS */
	zip_fillStat(statBuf, handle->native->file->extra);
	return 0;
}

int
zip_stat(uio_PDirHandle *pDirHandle, const char *name, struct stat *statBuf) {
	uio_GPDirEntry *entry;

	if (name[0] == '.' && name[1] == '\0') {
		entry = (uio_GPDirEntry *) pDirHandle->extra;
	} else {
		entry = uio_GPDir_getGPDirEntry(pDirHandle->extra, name);
		if (entry == NULL) {
			errno = ENOENT;
			return -1;
		}
	}
	
	if (uio_GPDirEntry_isDir(entry) && entry->extra == NULL) {
		// No information about this directory was stored.
		// We'll have to make something up.
		memset(statBuf, '\0', sizeof (struct stat));
		statBuf->st_mode = S_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR |
				S_IRGRP | S_IWGRP | S_IXOTH |
				S_IROTH | S_IWOTH | S_IXOTH;
		statBuf->st_uid = 0;
		statBuf->st_gid = 0;
		return 0;
	}

#if zip_USE_HEADERS == zip_USE_CENTRAL_HEADERS
#ifndef zip_INCOMPLETE_STAT
	if (((zip_GPFileData *) entry->extra)->fileOffset == -1) {
		// The local header wasn't read in yet.
		if (zip_updateFileDataFromLocalHeader(pDirHandle->pRoot->handle,
				(zip_GPFileData *) entry->extra) == -1) {
			// errno is set
			return -1;
		}
	}
#endif  /* !defined(zip_INCOMPLETE_STAT) */
#endif  /* zip_USE_HEADERS == zip_USE_CENTRAL_HEADERS */

	zip_fillStat(statBuf, (zip_GPFileData *) entry->extra);
	return 0;
}

/*
 * Function name: zip_open
 * Description:   open a file in zip file
 * Arguments:     pDirHandle - handle to the dir where to open the file
 *                name - the name of the file to open
 *                flags - flags, as to stdio open()
 *                mode - mode, as to stdio open()
 * Returns:       handle, as from stdio open()
 *                If failed, errno is set and handle is -1.
 */
uio_Handle *
zip_open(uio_PDirHandle *pDirHandle, const char *name, int flags,
		mode_t mode) {
	zip_Handle *handle;
	uio_GPFile *gPFile;

#if defined(DEBUG) && DEBUG > 1
	fprintf(stderr, "zip_open - pDirHandle=%p name=%s flags=%d mode=0%o\n",
			(void *) pDirHandle, name, flags, mode);
#endif
	
	if ((flags & O_ACCMODE) != O_RDONLY) {
		errno = EACCES;
		return NULL;
	}

	gPFile = (uio_GPFile *) uio_GPDir_getGPDirEntry(pDirHandle->extra, name);	
	if (gPFile == NULL) {
		errno = ENOENT;
		return NULL;
	}

#if zip_USE_HEADERS == zip_USE_CENTRAL_HEADERS
	if (gPFile->extra->fileOffset == -1) {
		// The local header wasn't read in yet.
		if (zip_updateFileDataFromLocalHeader(pDirHandle->pRoot->handle,
				gPFile->extra) == -1) {
			// errno is set
			return NULL;
		}
	}
#endif
	
	handle = uio_malloc(sizeof (zip_Handle));
	uio_GPFile_ref(gPFile);
	handle->file = gPFile;
	handle->fileBlock = uio_openFileBlock2(pDirHandle->pRoot->handle,
			gPFile->extra->fileOffset, gPFile->extra->compressedSize);
	if (handle->fileBlock == NULL) {
		// errno is set
		return NULL;
	}

	if (zip_initZipStream(&handle->zipStream) == -1) {
		uio_GPFile_unref(gPFile);
		uio_closeFileBlock(handle->fileBlock);
		return NULL;
	}
	handle->compressedOffset = 0;
	handle->uncompressedOffset = 0;
	
	(void) mode;
	return uio_Handle_new(pDirHandle->pRoot, handle, flags);
}

ssize_t
zip_read(uio_Handle *handle, void *buf, size_t count) {
	ssize_t result;

#if defined(DEBUG) && DEBUG > 1
	fprintf(stderr, "zip_read - handle=%p buf=%p count=%d: ", (void *) handle,
			(void *) buf, count);
#endif
	result = zip_readMethods[handle->native->file->extra->compressionMethod]
			(handle, buf, count);
#if defined(DEBUG) && DEBUG > 1
	fprintf(stderr, "%d\n", result);
#endif
	return result;
}

static ssize_t
zip_readStored(uio_Handle *handle, void *buf, size_t count) {
	int numBytes;
	zip_Handle *zipHandle;

	zipHandle = handle->native;
	numBytes = uio_copyFileBlock(zipHandle->fileBlock,
			zipHandle->uncompressedOffset, buf, count);
	if (numBytes == -1) {
		// errno is set
		return -1;
	}
	zipHandle->uncompressedOffset += numBytes;
	zipHandle->compressedOffset += numBytes;
	return numBytes;
}

static ssize_t
zip_readDeflated(uio_Handle *handle, void *buf, size_t count) {
	zip_Handle *zipHandle;
	int inflateResult;

	zipHandle = handle->native;

	if (count > ((zip_GPFileData *) (zipHandle->file->extra))->
			uncompressedSize - zipHandle->zipStream.total_out)
		count = ((zip_GPFileData *) (zipHandle->file->extra))->
				uncompressedSize - zipHandle->zipStream.total_out;

	zipHandle->zipStream.next_out = (Bytef *) buf;
	zipHandle->zipStream.avail_out = count;
	while (zipHandle->zipStream.avail_out > 0) {
		if (zipHandle->zipStream.avail_in == 0) {
			ssize_t numBytes;
			numBytes = uio_accessFileBlock(zipHandle->fileBlock,
					zipHandle->compressedOffset, zip_INPUT_BUFFER_SIZE,
					(char **) &zipHandle->zipStream.next_in);
			if (numBytes == -1) {
				// errno is set
				return -1;
			}
#if 0
			if (numBytes == 0) {
				if (zipHandle->uncompressedOffset !=
						zipHandle->file->extra->uncompressedSize) {
					// premature eof
					errno = EIO;
					return -1;
				}
				break;
			}
#endif
			zipHandle->zipStream.avail_in = numBytes;
			zipHandle->compressedOffset += numBytes;
		}
		inflateResult = inflate(&zipHandle->zipStream, Z_SYNC_FLUSH);
		zipHandle->uncompressedOffset = zipHandle->zipStream.total_out;
		if (inflateResult == Z_STREAM_END) {
			// Everything is decompressed
			break;
		}
		if (inflateResult != Z_OK) {
			switch (inflateResult) {
				case Z_VERSION_ERROR:
					fprintf(stderr, "Error: Incompatible version problem for "
							" decompression.\n");
					break;
				case Z_NEED_DICT:
					fprintf(stderr, "Error: Decompressing requires "
							"preset dictionary.\n");
					break;
				case Z_DATA_ERROR:
					fprintf(stderr, "Error: Compressed file is corrupted.\n");
					break;
				case Z_STREAM_ERROR:
					// This means zipHandle->zipStream is bad, which is
					// most likely an error in the code using zlib.
					fprintf(stderr, "Fatal: internal error using zlib.\n");
					abort();
					break;
				case Z_MEM_ERROR:
					fprintf(stderr, "Error: Not enough memory available "
							"while decompressing.\n");
					break;
				case Z_BUF_ERROR:
					// No progress possible. Probably caused by premature
					// end of input file.
					fprintf(stderr, "Error: When decompressing: premature "
							"end of input file.\n");
					errno = EIO;
					return -1;
#if 0
					// If this happens, either the input buffer is empty
					// or the output buffer is full. This should not happen.
					fprintf(stderr, "Fatal: internal error using zlib: "
							" no progress is possible in decompression.\n");
					abort();
					break;
#endif
				default:
					fprintf(stderr, "Fatal: unknown error from inflate().\n");
					abort();
			}
			if (zipHandle->zipStream.msg != NULL)
				fprintf(stderr, "ZLib reports: %s\n",
						zipHandle->zipStream.msg);
			errno = EIO;
					// Using EIO to report an error in the backend.
			return -1;
		}
	}
	return count - zipHandle->zipStream.avail_out;
}

off_t
zip_seek(uio_Handle *handle, off_t offset, int whence) {
	zip_Handle *zipHandle;

#if defined(DEBUG) && DEBUG > 1
	fprintf(stderr, "zip_seek - handle=%p offset=%d whence=%s\n",
			(void *) handle, (int) offset,
			whence == SEEK_SET ? "SEEK_SET" :
			whence == SEEK_CUR ? "SEEK_CUR" :
			whence == SEEK_END ? "SEEK_END" : "INVALID");
#endif
	zipHandle = handle->native;

	assert(whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END);
	switch(whence) {
		case SEEK_SET:
			break;
		case SEEK_CUR:
			offset += zipHandle->uncompressedOffset;
			break;
		case SEEK_END:
			offset += zipHandle->file->extra->uncompressedSize;
			break;
	}
	if (offset < 0) {
		offset = 0;
	} else if (offset > zipHandle->file->extra->uncompressedSize) {
		offset = zipHandle->file->extra->uncompressedSize;
	}
	return zip_seekMethods[handle->native->file->extra->compressionMethod]
			(handle, offset);
}

static off_t
zip_seekStored(uio_Handle *handle, off_t offset) {
	zip_Handle *zipHandle;

	zipHandle = handle->native;
	if (offset > zipHandle->file->extra->uncompressedSize)
		offset = zipHandle->file->extra->uncompressedSize;

	zipHandle->compressedOffset = offset;
	zipHandle->uncompressedOffset = offset;
	return offset;
}

static off_t
zip_seekDeflated(uio_Handle *handle, off_t offset) {
	zip_Handle *zipHandle;

	zipHandle = handle->native;

	if (offset < zipHandle->uncompressedOffset) {
		// The new offset is earlier than the current offset. We need to
		// seek from the beginning.
		if (zip_reInitZipStream(&zipHandle->zipStream) == -1) {
			// Need to abort. Handle would get in an inconsistent state.
			// Should not fail anyhow.
			fprintf(stderr, "Fatal: Could not reinitialise zip stream: "
					"%s.\n", strerror(errno));
			abort();
		}
		zipHandle->compressedOffset = 0;
		zipHandle->uncompressedOffset = 0;
	}

	if (offset == zipHandle->uncompressedOffset)
		return offset;

	// Seek from the current position.
	{
		char *buffer;
		ssize_t numRead;
		size_t toRead;

		buffer = uio_malloc(zip_SEEK_BUFFER_SIZE);
		toRead = offset - zipHandle->uncompressedOffset;
		while (toRead > 0) {
			numRead = zip_read(handle, buffer,
					toRead < zip_SEEK_BUFFER_SIZE ?
					toRead : zip_SEEK_BUFFER_SIZE);
			if (numRead == -1) {
				fprintf(stderr, "Warning: Could not read zipped file: %s\n",
						strerror(errno));
				break;
						// The current location is returned.
			}
			toRead -= numRead;
		}
		uio_free(buffer);
	}
	return zipHandle->uncompressedOffset;
}

uio_PRoot *
zip_mount(uio_Handle *handle, int flags) {
	uio_PRoot *result;
	uio_PDirHandle *rootDirHandle;
	
	if ((flags & uio_MOUNT_RDONLY) != uio_MOUNT_RDONLY) {
		errno = EACCES;
		return NULL;
	}

	uio_Handle_ref(handle);
	result = uio_GPRoot_makePRoot(
			uio_getFileSystemHandler(uio_FSTYPE_ZIP), flags,
			&zip_GPRootOperations, NULL, uio_GPRoot_PERSISTENT,
			handle, NULL, uio_GPDir_COMPLETE);

	rootDirHandle = uio_PRoot_getRootDirHandle(result);
	if (zip_fillDirStructure(rootDirHandle->extra, handle) == -1) {
		int savedErrno = errno;
#ifdef DEBUG
		fprintf(stderr, "Error: failed to read the zip directory "
				"structure - %s.\n", strerror(errno));
#endif
		uio_GPRoot_umount(result);
		errno = savedErrno;
		return NULL;
	}
	
	return result;
}

static int
zip_fillDirStructure(uio_GPDir *top, uio_Handle *handle) {
#if zip_USE_HEADERS == zip_USE_CENTRAL_HEADERS
	return zip_fillDirStructureCentral(top, handle);
#endif
#if zip_USE_HEADERS == zip_USE_LOCAL_HEADERS
	return zip_fillDirStructureLocal(top, handle);
#endif
}

#if zip_USE_HEADERS == zip_USE_CENTRAL_HEADERS
static int
zip_fillDirStructureCentral(uio_GPDir *top, uio_Handle *handle) {
	uio_FileBlock *fileBlock;
	off_t pos;
	char *buf;
	ssize_t numBytes;
	uio_uint16 numEntries;
			// TODO: use numEntries to initialise the hash table
			//       to a smart size
	off_t eocdr;
	off_t startCentralDir;

	fileBlock = uio_openFileBlock(handle);
	if (fileBlock == NULL) {
		// errno is set
		goto err;
	}

	// first find the 'End of Central Directory Record'
	eocdr = zip_findEndOfCentralDirectoryRecord(handle, fileBlock);
	if (eocdr == -1) {
		// errno is set
		goto err;
	}

	numBytes = uio_accessFileBlock(fileBlock, eocdr, 22, &buf);
	if (numBytes == -1) {
		// errno is set
		goto err;
	}
	if (numBytes != 22) {
		errno = EIO;
		goto err;
	}

	numEntries = makeUInt16(buf[10], buf[11]);
	if (numEntries == 0xffff) {
		fprintf(stderr, "Error: Zip64 .zip files are not supported.\n");
		errno = ENOSYS;
		goto err;
	}

	startCentralDir = makeUInt32(buf[16], buf[17], buf[18], buf[19]);

	// Enable read-ahead buffering, for speed.
	uio_setFileBlockUsageHint(fileBlock, uio_FB_USAGE_FORWARD,
			DIR_STRUCTURE_READ_BUFSIZE);

	pos = startCentralDir;
	while (numEntries--) {
		if (zip_fillDirStructureCentralProcessEntry(top, fileBlock, &pos)
				== -1) {
			// errno is set
			goto err;
		}
	}

	uio_closeFileBlock(fileBlock);
	return 0;

err:
	{
		int savedErrno = errno;

		if (fileBlock != NULL)
			uio_closeFileBlock(fileBlock);
		errno = savedErrno;
		return -1;
	}
}

static int
zip_fillDirStructureCentralProcessEntry(uio_GPDir *topGPDir,
		uio_FileBlock *fileBlock, off_t *pos) {
	char *buf;
	zip_GPFileData *gPFileData;
	ssize_t numBytes;

	uio_uint32 signature;
	uio_uint16 lastModTime;
	uio_uint16 lastModDate;
	uio_uint32 crc;
	uio_uint16 fileNameLength;
	uio_uint16 extraFieldLength;
	uio_uint16 fileCommentLength;
	char *fileName;
	zip_OSType creatorOS;

	off_t nextEntryOffset;

	numBytes = uio_accessFileBlock(fileBlock, *pos, 46, &buf);
	if (numBytes != 46)
		return zip_badFile(NULL, NULL);

	signature = makeUInt32(buf[0], buf[1], buf[2], buf[3]);
	if (signature != 0x02014b50) {
		fprintf(stderr, "Error: Premature end of central directory.\n");
		errno = EIO;
		return -1;
	}
	
	gPFileData = zip_GPFileData_new();
	creatorOS = (zip_OSType) buf[5];
	gPFileData->compressionFlags = makeUInt16(buf[8], buf[9]);
	gPFileData->compressionMethod = makeUInt16(buf[10], buf[11]);
	lastModTime = makeUInt16(buf[12], buf[13]);
	lastModDate = makeUInt16(buf[14], buf[15]);
	gPFileData->atime = (time_t) 0;
	gPFileData->mtime = dosToUnixTime(lastModDate, lastModTime);
	gPFileData->ctime = (time_t) 0;
	crc = makeUInt32(buf[16], buf[17], buf[18], buf[19]);
	gPFileData->compressedSize =
			makeUInt32(buf[20], buf[21], buf[22], buf[23]);
	gPFileData->uncompressedSize =
			makeUInt32(buf[24], buf[25], buf[26], buf[27]);
	fileNameLength = makeUInt16(buf[28], buf[29]);
	extraFieldLength = makeUInt16(buf[30], buf[31]);
	fileCommentLength = makeUInt16(buf[32], buf[33]);
	gPFileData->uid = 0;
	gPFileData->gid = 0;
	gPFileData->mode = zip_makeFileMode(creatorOS,
			makeUInt32(buf[38], buf[39], buf[40], buf[41]));

	gPFileData->headerOffset =
			(off_t) makeSInt32(buf[42], buf[43], buf[44], buf[45]);
	gPFileData->fileOffset = (off_t) -1;

	*pos += 46;
	nextEntryOffset = *pos + fileNameLength + extraFieldLength +
			fileCommentLength;

	numBytes = uio_accessFileBlock(fileBlock, *pos, fileNameLength, &buf);
	if (numBytes != fileNameLength)
		return zip_badFile(gPFileData, NULL);
	fileName = uio_malloc(fileNameLength + 1);
	memcpy(fileName, buf, fileNameLength);
	fileName[fileNameLength] = '\0';
	*pos += fileNameLength;

	if (gPFileData->compressionMethod >= NUM_COMPRESSION_METHODS ||
			!zip_compressionMethodSupported[gPFileData->compressionMethod]) {
		fprintf(stderr, "Warning: File '%s' is compressed with "
				"unsupported method %d - skipped.\n", fileName,
				gPFileData->compressionMethod);
		*pos = nextEntryOffset;
		zip_GPFileData_delete(gPFileData);
		return 0;
	}
	
	if (gPFileData->compressedSize == (off_t) 0xffffffff ||
			gPFileData->uncompressedSize == (off_t) 0xffffffff ||
			gPFileData->headerOffset < 0) {
		fprintf(stderr, "Warning: Skipping Zip64 file '%s'\n", fileName);
		*pos = nextEntryOffset;
		zip_GPFileData_delete(gPFileData);
		return 0;
	}
	
	if (isBitSet(gPFileData->compressionFlags, 0)) {
		fprintf(stderr, "Warning: Skipping encrypted file '%s'\n", fileName);
		*pos = nextEntryOffset;
		zip_GPFileData_delete(gPFileData);
		return 0;
	}

	switch (zip_fillDirStructureProcessExtraFields(fileBlock,
				extraFieldLength, gPFileData, fileName, *pos, true)) {
		case 0:  // file is ok
			break;
		case 1:  // file is not acceptable - skip file
			*pos = nextEntryOffset;
			zip_GPFileData_delete(gPFileData);
			uio_free(fileName);
			return 0;
		case -1:
			return zip_badFile(gPFileData, fileName);
	}
	
	*pos += extraFieldLength;

	// If ctime or atime is 0, they will be filled in when the local
	// file header is read.

	if (S_ISREG(gPFileData->mode)) {
		if (zip_foundFile(topGPDir, fileName, gPFileData) == -1) {
			if (errno == EISDIR) {
				zip_GPFileData_delete(gPFileData);
				uio_free(fileName);
				return 0;
			}
			return zip_badFile(gPFileData, fileName);
		}

#if defined(DEBUG) && DEBUG > 1
		fprintf(stderr, "Debug: Found file '%s'.\n", fileName);
#endif
	} else if (S_ISDIR(gPFileData->mode)) {
		if (fileName[fileNameLength - 1] == '/')
			fileName[fileNameLength - 1] = '\0';
		if (zip_foundDir(topGPDir, fileName, gPFileData) == -1) {
			if (errno == EISDIR) {
				fprintf(stderr, "Warning: file '%s' already exists as a dir - "
						"skipped.\n", fileName);
				zip_GPFileData_delete(gPFileData);
				uio_free(fileName);
				return 0;
			}
			return zip_badFile(gPFileData, fileName);
		}
#if defined(DEBUG) && DEBUG > 1
		fprintf(stderr, "Debug: Found dir '%s'.\n", fileName);
#endif
	} else {
		fprintf(stderr, "Warning: '%s' is not a regular file, nor a "
				"directory - skipped.\n", fileName);
		zip_GPFileData_delete(gPFileData);
		uio_free(fileName);
		return 0;
	}

	uio_free(fileName);
	return 0;
}

static off_t
zip_findEndOfCentralDirectoryRecord(uio_Handle *handle,
		uio_FileBlock *fileBlock) {
	off_t fileSize;
	off_t endPos, startPos;
	char *buf, *bufPtr;
	ssize_t bufLen;
	
	struct stat statBuf;
	if (uio_fstat(handle, &statBuf) == -1) {
		// errno is set
		return -1;
	}
	fileSize = statBuf.st_size;
	startPos = fileSize - 0xffff - 22;  // max comment and record size
	if (startPos < 0)
		startPos = 0;
	endPos = fileSize - 22;  // last position to be checked
	bufLen = uio_accessFileBlock(fileBlock, startPos, endPos - startPos + 4,
			&buf);
	if (bufLen == -1) {
		int savedErrno = errno;
		fprintf(stderr, "Error: Read error while searching for "
				"'end-of-central-directory record'.\n");
		errno = savedErrno;
		return -1;
	}
	if (bufLen != endPos - startPos + 4) {
		fprintf(stderr, "Error: Read error while searching for "
				"'end-of-central-directory record'.\n");
		errno = EIO;
		return -1;
	}
	bufPtr = buf + (endPos - startPos);
	while (1) {
		if (bufPtr < buf) {
			fprintf(stderr, "Error: Zip file corrupt; could not find "
					"'end-of-central-directory record'.\n");
			errno = EIO;
			return -1;
		}
		if (bufPtr[0] == 0x50 && bufPtr[1] == 0x4b && bufPtr[2] == 0x05 &&
				bufPtr[3] == 0x06)
			break;
		bufPtr--;
	}
	return startPos + (bufPtr - buf);
}

static mode_t
zip_makeFileMode(zip_OSType creatorOS, uio_uint32 modeBytes) {
	switch (creatorOS) {
		case zip_OSType_FAT:
		case zip_OSType_NTFS:
		case zip_OSType_VFAT: {
			// Only the least signigicant byte is relevant.
			mode_t mode;

			if (modeBytes == 0) {
				// File came from standard input
				return S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
						S_IROTH | S_IWOTH;
			}
			if (modeBytes & 0x10) {
				// Directory
				mode = S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP |
						S_IROTH | S_IXOTH;
			} else {
				// Regular file
				mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
			}
			if (modeBytes & 0x01) {
				// readonly
				return mode;
			} else {
				// Write allowed
				return mode | S_IWUSR | S_IWGRP | S_IWOTH;
			}
		}
		case zip_OSType_UNIX:
			return (mode_t) (modeBytes >> 16);
		default:
			fprintf(stderr, "Warning: file created by unknown OS.\n");
			return S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	}
}

// If the data is read from the central directory, certain data
// will need to be updated from the local directory when it is needed.
// This function does that.
// returns 0 for success, -1 for error (errno is set)
// NB: Only the fields that may offer new information are checked.
//     the fields that were already read from the central directory
//     aren't verified.
static int
zip_updatePFileDataFromLocalFileHeader(zip_GPFileData *gPFileData,
		uio_FileBlock *fileBlock, int pos) {
	ssize_t numBytes;
	char *buf;
	uio_uint32 signature;
	uio_uint16 fileNameLength;
	uio_uint16 extraFieldLength;

	numBytes = uio_accessFileBlock(fileBlock, pos, 30, &buf);
	if (numBytes != 30) {
		errno = EIO;
		return -1;
	}
	signature = makeUInt32(buf[0], buf[1], buf[2], buf[3]);
	if (signature != 0x04034b50) {
		errno = EIO;
		return -1;
	}
	fileNameLength = makeUInt16(buf[26], buf[27]);
	extraFieldLength = makeUInt16(buf[28], buf[29]);
	pos += 30 + fileNameLength;
	
	switch (zip_fillDirStructureProcessExtraFields(fileBlock,
				extraFieldLength, gPFileData, "<unknown>", pos, false)) {
		case 0:  // file is ok
			break;
		case 1:
			// File is not acceptable (but according to the central header
			// it was)
			fprintf(stderr, "Warning: according to the central directory "
					"of a zip file, some file inside is acceptable, "
					"but according to the local header it isn't.\n");
			errno = EIO;
			return -1;
		case -1:
			errno = EIO;
			return -1;
	}
	pos += extraFieldLength;
	gPFileData->fileOffset = pos;
	return 0;
}
#endif  /* zip_USE_HEADERS == zip_USE_CENTRAL_HEADERS */

#if zip_USE_HEADERS == zip_USE_LOCAL_HEADERS
static int
zip_fillDirStructureLocal(uio_GPDir *top, uio_Handle *handle) {
	uio_FileBlock *fileBlock;
	off_t pos;
	char *buf;
	ssize_t numBytes;

	pos = uio_lseek(handle, 0, SEEK_SET);
	if (pos == -1) {
		int savedErrno = errno;
		errno = savedErrno;
		return -1;
	}
	if (pos != 0) {
		errno = EIO;
				// Using EIO to report an error in the backend.
		return -1;
	}

	// We read all the files from the beginning of the zip file to the end.
	// (the directory record at the end of the file is ignored)
	fileBlock = uio_openFileBlock(handle);
	if (fileBlock == NULL) {
		// errno  is set
		return -1;
	}
	
	pos = 0;
	while (1) {
		uio_uint32 signature;

		numBytes = uio_accessFileBlock(fileBlock, pos, 4, &buf);
		if (numBytes == -1)
			goto err;
		if (numBytes != 4)
			break;
		signature = makeUInt32(buf[0], buf[1], buf[2], buf[3]);
		if (signature != 0x04034b50) {
			// End of file data reached.
			break;
		}
		pos += 4;
		if (zip_fillDirStructureLocalProcessEntry(top, fileBlock, &pos) == -1)
			goto err;
	}

	uio_closeFileBlock(fileBlock);
	return 0;

err:
	{
		int savedErrno = errno;

		uio_closeFileBlock(fileBlock);
		errno = savedErrno;
		return -1;
	}
}

static int
zip_fillDirStructureLocalProcessEntry(uio_GPDir *topGPDir,
		uio_FileBlock *fileBlock, off_t *pos) {
	char *buf;
	zip_GPFileData *gPFileData;
	ssize_t numBytes;

	uio_uint16 lastModTime;
	uio_uint16 lastModDate;
	uio_uint32 crc;
	uio_uint16 fileNameLength;
	uio_uint16 extraFieldLength;
	char *fileName;

	off_t nextEntryOffset;

	numBytes = uio_accessFileBlock(fileBlock, *pos, 26, &buf);
	if (numBytes != 26)
		return zip_badFile(NULL, NULL);

	gPFileData = zip_GPFileData_new();
	gPFileData->compressionFlags = makeUInt16(buf[2], buf[3]);
	gPFileData->compressionMethod = makeUInt16(buf[4], buf[5]);
	lastModTime = makeUInt16(buf[6], buf[7]);
	lastModDate = makeUInt16(buf[8], buf[9]);
	gPFileData->atime = (time_t) 0;
	gPFileData->mtime = dosToUnixTime(lastModDate, lastModTime);
	gPFileData->ctime = (time_t) 0;
	gPFileData->uid = 0;
	gPFileData->gid = 0;
	gPFileData->mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	if (!isBitSet(gPFileData->compressionFlags, 3)) {
		// If bit 3 is not set, this info will be in the data descriptor
		// behind the file data.
		crc = makeUInt32(buf[10], buf[11], buf[12], buf[13]);
		gPFileData->compressedSize =
				makeUInt32(buf[14], buf[15], buf[16], buf[17]);
		gPFileData->uncompressedSize =
				makeUInt32(buf[18], buf[19], buf[20], buf[21]);
	}
	fileNameLength = makeUInt16(buf[22], buf[23]);
	extraFieldLength = makeUInt16(buf[24], buf[25]);
	*pos += 26;
	nextEntryOffset = *pos + fileNameLength + extraFieldLength +
			gPFileData->compressedSize;
	if (isBitSet(gPFileData->compressionFlags, 3)) {
		// There's a data descriptor present behind the file data.
		nextEntryOffset += 16;
	}

	if (gPFileData->compressionMethod >= NUM_COMPRESSION_METHODS ||
			!zip_compressionMethodSupported[gPFileData->compressionMethod]) {
		fprintf(stderr, "Warning: File '%s' is compressed with "
				"unsupported method %d - skipped.\n", fileName,
				gPFileData->compressionMethod);
		*pos = nextEntryOffset;
		zip_GPFileData_delete(gPFileData);
		return 0;
	}
	
	if (gPFileData->compressedSize == (off_t) 0xffffffff ||
			gPFileData->uncompressedSize == (off_t) 0xffffffff) {
		fprintf(stderr, "Warning: Skipping Zip64 file '%s'\n", fileName);
		*pos = nextEntryOffset;
		zip_GPFileData_delete(gPFileData);
		return 0;
	}
	
	if (isBitSet(gPFileData->compressionFlags, 0)) {
		fprintf(stderr, "Warning: Skipping encrypted file '%s'\n", fileName);
		*pos = nextEntryOffset;
		zip_GPFileData_delete(gPFileData);
		return 0;
	}

	numBytes = uio_accessFileBlock(fileBlock, *pos, fileNameLength, &buf);
	if (numBytes != fileNameLength)
		return zip_badFile(gPFileData, NULL);
	*pos += fileNameLength;
	if (buf[fileNameLength - 1] == '/') {
		gPFileData->mode |= S_IFDIR;
		fileNameLength--;
	} else
		gPFileData->mode |= S_IFREG;
	fileName = uio_malloc(fileNameLength + 1);
	memcpy(fileName, buf, fileNameLength);
	fileName[fileNameLength] = '\0';

	switch (zip_fillDirStructureProcessExtraFields(fileBlock,
				extraFieldLength, gPFileData, fileName, *pos, false)) {
		case 0:  // file is ok
			break;
		case 1:  // file is not acceptable - skip file
			*pos = nextEntryOffset;
			zip_GPFileData_delete(gPFileData);
			uio_free(fileName);
			return 0;
		case -1:
			return zip_badFile(gPFileData, fileName);
	}
	*pos += extraFieldLength;
	
	gPFileData->fileOffset = *pos;

	*pos += gPFileData->compressedSize;
	if (isBitSet(gPFileData->compressionFlags, 3)) {
		// Now comes a data descriptor.
		// The PKWare version (which was never used) misses the signature.
		// The InfoZip version is used below.
		uio_uint32 signature;

		numBytes = uio_accessFileBlock(fileBlock, *pos, 16, &buf);
		if (numBytes != 16)
			return zip_badFile(gPFileData, fileName);

		signature = makeUInt32(buf[0], buf[1], buf[2], buf[3]);
		if (signature != 0x08074b50)
			return zip_badFile(gPFileData, fileName);
		crc = makeUInt32(buf[4], buf[5], buf[6], buf[7]);
		gPFileData->compressedSize =
				makeUInt32(buf[8], buf[9], buf[10], buf[11]);
		gPFileData->uncompressedSize =
				makeUInt32(buf[12], buf[13], buf[14], buf[15]);
	}

	if (gPFileData->ctime == (time_t) 0)
		gPFileData->ctime = gPFileData->mtime;

	if (gPFileData->atime == (time_t) 0)
		gPFileData->atime = gPFileData->mtime;
	
	if (S_ISREG(gPFileData->mode)) {
		if (zip_foundFile(topGPDir, fileName, gPFileData) == -1) {
			if (errno == EISDIR) {
				zip_GPFileData_delete(gPFileData);
				uio_free(fileName);
				return 0;
			}
			return zip_badFile(gPFileData, fileName);
		}

#if defined(DEBUG) && DEBUG > 1
		fprintf(stderr, "Debug: Found file '%s'.\n", fileName);
#endif
	} else if (S_ISDIR(gPFileData->mode)) {
		if (fileName[fileNameLength - 1] == '/')
			fileName[fileNameLength - 1] = '\0';
		if (zip_foundDir(topGPDir, fileName, gPFileData) == -1) {
			if (errno == EISDIR) {
				fprintf(stderr, "Warning: file '%s' already exists as a dir - "
						"skipped.\n", fileName);
				zip_GPFileData_delete(gPFileData);
				uio_free(fileName);
				return 0;
			}
			return zip_badFile(gPFileData, fileName);
		}
#if defined(DEBUG) && DEBUG > 1
		fprintf(stderr, "Debug: Found dir '%s'.\n", fileName);
#endif
	} else {
		fprintf(stderr, "Warning: '%s' is not a regular file, nor a "
				"directory - skipped.\n", fileName);
		zip_GPFileData_delete(gPFileData);
		uio_free(fileName);
		return 0;
	}

	uio_free(fileName);
	return 0;
}
#endif  /* zip_USE_HEADERS == zip_USE_LOCAL_HEADERS */

#if zip_USE_HEADERS == zip_USE_CENTRAL_HEADERS
int
zip_updateFileDataFromLocalHeader(uio_Handle *handle,
		zip_GPFileData *gPFileData) {
	uio_FileBlock *fileBlock;

	fileBlock = uio_openFileBlock(handle);
	if (fileBlock == NULL) {
		// errno is set
		return -1;
	}
	if (zip_updatePFileDataFromLocalFileHeader(gPFileData,
			fileBlock, gPFileData->headerOffset) == -1) {
		int savedErrno = errno;
		uio_closeFileBlock(fileBlock);
		errno = savedErrno;
		return -1;
	}
	if (gPFileData->ctime == (time_t) 0)
		gPFileData->ctime = gPFileData->mtime;
	if (gPFileData->atime == (time_t) 0)
		gPFileData->atime = gPFileData->mtime;
	uio_closeFileBlock(fileBlock);
	return 0;
}
#endif

// If the zip file is bad, -1 is returned (no errno set!)
// If the file in the zip file should be skipped, 1 is returned.
// If the file in the zip file is ok, 0 is returned.
static int
zip_fillDirStructureProcessExtraFields(uio_FileBlock *fileBlock,
		off_t extraFieldLength, zip_GPFileData *gPFileData,
		const char *fileName, off_t pos, uio_bool central) {
	off_t posEnd;
	uio_uint16 headerID;
	ssize_t dataSize;
	ssize_t numBytes;
	char *buf;
	
	posEnd = pos + extraFieldLength;
	while (pos < posEnd) {
		numBytes = uio_accessFileBlock(fileBlock, pos, 4, &buf);
		if (numBytes != 4)
			return -1;
		headerID = makeUInt16(buf[0], buf[1]);
		dataSize = (ssize_t) makeUInt16(buf[2], buf[3]);
		pos += 4;
		numBytes = uio_accessFileBlock(fileBlock, pos, dataSize, &buf);
		if (numBytes != dataSize)
			return -1;
		switch(headerID) {
			case 0x000d:  // 'Unix0'
				// fallthrough
			case 0x5855:  // 'Unix1'
				gPFileData->atime = (time_t) makeUInt32(
						buf[0], buf[1], buf[2], buf[3]);
				gPFileData->mtime = (time_t) makeUInt32(
						buf[4], buf[5], buf[6], buf[7]);
				if (central)
					break;
				if (dataSize > 8) {
					gPFileData->uid = (uid_t) makeUInt16(buf[8], buf[9]);
					gPFileData->gid = (uid_t) makeUInt16(buf[10], buf[11]);
				}
				// Unix0 has an extra (ignored) field at the end.
				break;
			case 0x5455: {  // 'time'
				uio_uint8 flags;
				const char *bufPtr;
				flags = buf[0];
				bufPtr = buf + 1;
				if (isBitSet(flags, 0)) {
					// modification time is present
					gPFileData->mtime = (time_t) makeUInt32(
							bufPtr[0], bufPtr[1], bufPtr[2], bufPtr[3]);
					bufPtr += 4;
				}
				// The flags field, even in the central header, specifies what
				// is present in the local header.
				// The central header only contains a field for the mtime
				// when it is present in the local header too, and
				// never contains fields for other times.
				if (central)
					break;
				if (isBitSet(flags, 1)) {
					// modification time is present
					gPFileData->atime = (time_t) makeUInt32(
							bufPtr[0], bufPtr[1], bufPtr[2], bufPtr[3]);
					bufPtr += 4;
				}
				// Creation time and possible other times are skipped.
				break;
			}
			case 0x7855:  // 'Unix2'
				if (central)
					break;
				gPFileData->uid = (uid_t) makeUInt16(buf[0], buf[1]);
				gPFileData->gid = (uid_t) makeUInt16(buf[2], buf[3]);
				break;
			case 0x756e: {  // 'Unix3'
				mode_t mode;
				mode = (mode_t) makeUInt16(buf[4], buf[5]);
				if (!S_ISREG(mode) && !S_ISDIR(mode)) {
					fprintf(stderr, "Warning: Skipping '%s'; not a regular "
							"file, nor a directory.\n", fileName);
					return 1;
				}
				gPFileData->uid = (uid_t) makeUInt16(buf[10], buf[11]);
				gPFileData->gid = (uid_t) makeUInt16(buf[12], buf[13]);
				break;
			case 0x7875:  // 'Unix string UID/GID'
				// Just skip it
				break;
			}
			default:
#ifdef DEBUG
				fprintf(stderr, "Debug: Extra field 0x%04x unsupported, "
						"used for file '%s' - ignored.\n", headerID,
						fileName);
#endif
				break;
		} // switch
		pos += dataSize;
	}  // while
	if (pos != posEnd)
		return -1;
	return 0;
}

static int
zip_badFile(zip_GPFileData *gPFileData, char *fileName) {
	fprintf(stderr, "Error: Bad file format for .zip file.\n");
	if (gPFileData != NULL)
		zip_GPFileData_delete(gPFileData);
	if (fileName != NULL)
		uio_free(fileName);
	errno = EINVAL;  // Is this the best choice?
	return -1;
}

static inline int
zip_foundFile(uio_GPDir *gPDir, const char *path, zip_GPFileData *gPFileData) {
	uio_GPFile *file;
	size_t pathLen;
	const char *rest;
	const char *pathEnd;
	const char *start, *end;
	char *buf;

	if (path[0] == '/')
		path++;
	pathLen = strlen(path);
	if (path[pathLen - 1] == '/') {
		fprintf(stderr, "Warning: '%s' is not a valid file name - skipped.\n",
				path);
		errno = EISDIR;
		return -1;
	}
	pathEnd = path + pathLen;

	switch (uio_walkGPPath(gPDir, path, pathLen, &gPDir, &rest)) {
		case 0:
			// The entire path was matched. The last part was not supposed
			// to be a dir.
			fprintf(stderr, "Warning: '%s' already exists as a dir - "
					"skipped.\n", path);
			errno = EISDIR;
			return -1;
		case ENOTDIR:
			fprintf(stderr, "Warning: A component to '%s' is not a "
					"directory - file skipped.\n", path);
			errno = ENOTDIR;
			return -1;
		case ENOENT:
			break;
	}

	buf = uio_malloc(pathLen + 1);
	getFirstPathComponent(rest, pathEnd, &start, &end);
	while (1) {
		uio_GPDir *newGPDir;
		
		if (end == start || (end - start == 1 && start[0] == '.') ||
				(end - start == 2 && start[0] == '.' && start[1] == '.')) {
			fprintf(stderr, "Warning: file '%s' has an invalid path - "
					"skipped.\n", path);
			uio_free(buf);
			errno = EINVAL;
			return -1;
		}
		if (end == pathEnd) {
			// This is the last component; it should be the name of the dir.
			rest = start;
			break;
		}
		memcpy(buf, start, end - start);
		buf[end - start] = '\0';
		newGPDir = uio_GPDir_prepareSubDir(gPDir, buf);
		newGPDir->flags |= uio_GPDir_COMPLETE;
				// It will be complete when we're done adding
				// all files, and it won't be used before that.
		uio_GPDir_commitSubDir(gPDir, buf, newGPDir);

		gPDir = newGPDir;
		getNextPathComponent(pathEnd, &start, &end);
	}

	uio_free(buf);
	
	file = uio_GPFile_new(gPDir->pRoot, (uio_GPFileExtra) gPFileData,
			uio_gPFileFlagsFromPRootFlags(gPDir->pRoot->flags));
	uio_GPDir_addFile(gPDir, rest, file);
	return 0;
}

static inline int
zip_foundDir(uio_GPDir *gPDir, const char *path, zip_GPDirData *gPDirData) {
	size_t pathLen;
	const char *pathEnd;
	const char *rest;
	const char *start, *end;
	char *buf;

	if (path[0] == '/')
		path++;
	pathLen = strlen(path);
	pathEnd = path + pathLen;

	switch (uio_walkGPPath(gPDir, path, pathLen, &gPDir, &rest)) {
		case 0:
			// The dir already exists. Only need to add gPDirData
			if (gPDir->extra != NULL) {
				fprintf(stderr, "Warning: '%s' is present more than once "
						"in the zip file. The last entry will be used.\n",
						path);
				zip_GPDirData_delete(gPDir->extra);
			}
			gPDir->extra = gPDirData;
			return 0;
		case ENOTDIR:
			fprintf(stderr, "Warning: A component of '%s' is not a "
					"directory - file skipped.\n", path);
			errno = ENOTDIR;
			return -1;
		case ENOENT:
			break;
	}

	buf = uio_malloc(pathLen + 1);
	getFirstPathComponent(rest, pathEnd, &start, &end);
	while (start < pathEnd) {
		uio_GPDir *newGPDir;
		
		if (end == start || (end - start == 1 && start[0] == '.') ||
				(end - start == 2 && start[0] == '.' && start[1] == '.')) {
			fprintf(stderr, "Warning: directory '%s' has an invalid path - "
					"skipped.\n", path);
			uio_free(buf);
			errno = EINVAL;
			return -1;
		}
		memcpy(buf, start, end - start);
		buf[end - start] = '\0';
		newGPDir = uio_GPDir_prepareSubDir(gPDir, buf);
		newGPDir->flags |= uio_GPDir_COMPLETE;
				// It will be complete when we're done adding
				// all files, and it won't be used before that.
		uio_GPDir_commitSubDir(gPDir, buf, newGPDir);

		gPDir = newGPDir;
		getNextPathComponent(pathEnd, &start, &end);
	}
	gPDir->extra = gPDirData;

	uio_free(buf);
	return 0;
}

static int
zip_initZipStream(z_stream *zipStream) {
	int retVal;

	zipStream->next_in = Z_NULL;
	zipStream->avail_in = 0;
	zipStream->zalloc = zip_alloc;
	zipStream->zfree = zip_free;
	zipStream->opaque = NULL;
	retVal = inflateInit2(zipStream, -MAX_WBITS);
			// Negative window size means that no zlib header is present.
			// This feature is undocumented in zlib, but it's used
			// in the minizip program from the zlib contrib dir.
			// The absolute value is used as real Window size.
	if (retVal != Z_OK) {
		switch (retVal) {
			case Z_MEM_ERROR:
				fprintf(stderr, "Error: Not enough memory available for "
						" decompression.\n");
				break;
			case Z_VERSION_ERROR:
				fprintf(stderr, "Error: Incompatible version problem for "
						" decompression.\n");
				break;
			default:
				fprintf(stderr, "Fatal: unknown error from inflateInit().\n");
				abort();
		}
		if (zipStream->msg != NULL)
			fprintf(stderr, "ZLib reports: %s\n", zipStream->msg);
		errno = EIO;
				// Using EIO to report an error in the backend.
		return -1;
	}
	return 0;
}

static int
zip_unInitZipStream(z_stream *zipStream) {
	int retVal;
	
	retVal = inflateEnd(zipStream);
	if (retVal != Z_OK) {
		switch (retVal) {
			case Z_STREAM_ERROR:
				// This means zipStream is bad, which is most likely an
				// error in the code using zlib.
				fprintf(stderr, "Fatal: internal error using zlib.\n");
				abort();
				break;
			default:
				fprintf(stderr, "Fatal: unknown error from inflateEnd().\n");
				abort();
		}
		if (zipStream->msg != NULL)
			fprintf(stderr, "ZLib reports: %s\n", zipStream->msg);
		errno = EIO;
				// Using EIO to report an error in the backend.
		return -1;
	}
	return 0;
}

static int
zip_reInitZipStream(z_stream *zipStream) {
	int retVal;

	zipStream->next_in = Z_NULL;
	zipStream->avail_in = 0;
	retVal = inflateReset(zipStream);
	if (retVal != Z_OK) {
		switch (retVal) {
			case Z_STREAM_ERROR:
				// This means zipStream is bad, which is  most likely an
				// error in the code using zlib.
				fprintf(stderr, "Fatal: internal error using zlib.\n");
				abort();
				break;
			default:
				fprintf(stderr, "Fatal: unknown error from inflateInit().\n");
				abort();
		}
		if (zipStream->msg != NULL)
			fprintf(stderr, "ZLib reports: %s\n", zipStream->msg);
		errno = EIO;
				// Using EIO to report an error in the backend.
		return -1;
	}
	return 0;
}


// Used internally by zlib for allocating memory.
static voidpf
zip_alloc(voidpf opaque, uInt items, uInt size) {
	(void) opaque;
	return (voidpf) uio_calloc((size_t) items, (size_t) size);
}

// Used internally by zlib for freeing memory.
static void
zip_free(voidpf opaque, voidpf address) {
	(void) opaque;
	uio_free((void *) address);
}

static inline zip_GPFileData *
zip_GPFileData_new(void) {
	return zip_GPFileData_alloc();
}

static inline void
zip_GPFileData_delete(zip_GPFileData *gPFileData) {
	zip_GPFileData_free(gPFileData);
}

static inline zip_GPFileData *
zip_GPFileData_alloc(void) {
	zip_GPFileData *result = uio_malloc(sizeof (zip_GPFileData));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(zip_GPFileData, (void *) result);
#endif
	return result;
}

static inline void
zip_GPFileData_free(zip_GPFileData *gPFileData) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(zip_GPFileData, (void *) gPFileData);
#endif
	uio_free(gPFileData);
}

static inline void
zip_GPDirData_delete(zip_GPDirData *gPDirData) {
	zip_GPDirData_free(gPDirData);
}

static inline void
zip_GPDirData_free(zip_GPDirData *gPDirData) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(zip_GPFileData, (void *) gPDirData);
#endif
	uio_free(gPDirData);
}



