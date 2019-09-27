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
 * CDP Unified IO Interface 
 */

#ifndef LIBS_CDP_CDP_IIO_H_
#define LIBS_CDP_CDP_IIO_H_

#include "types.h"
#include "libs/uio.h"

// CDP IO Interface entry points
typedef struct
{
	uio_Stream* (* fopen) (uio_DirHandle *dir, const char *path,
			const char *mode);
	int (* fclose) (uio_Stream *stream);
	size_t (* fread) (void *buf, size_t size, size_t nmemb,
			uio_Stream *stream);
	size_t (* fwrite) (const void *buf, size_t size, size_t nmemb,
			uio_Stream *stream);
	int (* fseek) (uio_Stream *stream, long offset, int whence);
	long (* ftell) (uio_Stream *stream);
	int (* fflush) (uio_Stream *stream);
	int (* feof) (uio_Stream *stream);
	int (* ferror) (uio_Stream *stream);

} cdp_Itf_IoVtbl_v1;

// the following are for the sake of module writers
typedef cdp_Itf_IoVtbl_v1 cdp_Itf_IoVtbl;
typedef cdp_Itf_IoVtbl    cdp_Itf_Io;

#endif  /* LIBS_CDP_CDP_IIO_H_ */
