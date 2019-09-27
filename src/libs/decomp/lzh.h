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

#ifndef LIBS_DECOMP_LZH_H_
#define LIBS_DECOMP_LZH_H_

#include "libs/declib.h"
#include "libs/memlib.h"

/* LZSS Parameters */

#define N 4096 /* Size of string buffer */
#define F 16 /* Size of look-ahead buffer */
//#define F 60 /* Size of look-ahead buffer */
#define THRESHOLD 2
#define NIL N /* End of tree's node  */

/* Huffman coding parameters */

#define N_CHAR   (256 - THRESHOLD + F)
								/* character code (= 0..N_CHAR-1) */
#define T  (N_CHAR * 2 - 1) /* Size of table */
#define R  (T - 1) /* root position */
#define MAX_FREQ 0x8000
										/* update when cumulative frequency */

struct _LZHCODE_DESC
{
	COUNT buf_index, restart_index, bytes_left;
	BYTE text_buf[N + F - 1];
		/* reconstruct freq tree */
	COUNT freq[T + 1]; /* cumulative freq table */
		/*
		 * pointing parent nodes.
		 * area [T..(T + N_CHAR - 1)] are pointers for leaves
		 */
	COUNT prnt[T + N_CHAR];
		/* pointing children nodes (son[], son[] + 1)*/
	COUNT son[T];
	UWORD workbuf;
	BYTE workbuflen;

	STREAM_TYPE StreamType;

	void *Stream;
	DWORD StreamIndex, StreamLength;

	STREAM_MODE StreamMode;
	PVOIDFUNC CleanupFunc;
};

typedef struct _LZHCODE_DESC LZHCODE_DESC;
typedef LZHCODE_DESC *PLZHCODE_DESC;

#define InChar() (_StreamType == FILE_STREAM ? \
								GetResFileChar ((uio_Stream *)_Stream) : \
								(int)*_Stream++)
#define OutChar(c) (_StreamType == FILE_STREAM ? \
								PutResFileChar ((c), (uio_Stream *)_Stream) : \
								(*_Stream++ = (BYTE)(c)))


#define AllocCodeDesc() HCalloc (sizeof (LZHCODE_DESC))
#define FreeCodeDesc HFree

extern void _update (COUNT c);
extern void StartHuff (void);

extern PLZHCODE_DESC  _lpCurCodeDesc;
extern STREAM_TYPE    _StreamType;
extern BYTE*          _Stream;
extern UWORD          _workbuf;
extern BYTE           _workbuflen;

#endif /* LIBS_DECOMP_LZH_H_ */

