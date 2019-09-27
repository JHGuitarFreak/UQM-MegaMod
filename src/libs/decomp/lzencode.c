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
 
 /*
 * LZHUF.C English version 1.0
 * Based on Japanese version 29-NOV-1988
 * LZSS coded by Haruhiko OKUMURA
 * Adaptive Huffman Coding coded by Haruyasu YOSHIZAKI
 * Edited and translated to English by Kenji RIKITAKE
 */

#include <stdio.h>
#include "lzh.h"
#include "libs/reslib.h"

static UWORD match_position, match_length;
static SWORD *lson;
static SWORD *rson;
static SWORD *dad;
static SWORD *encode_arrays;

#define AllocEncodeArrays() \
		HCalloc ( \
		(((N + 1) + (N + 257) + (N + 1)) \
		* sizeof (lson[0])))
#define FreeCodeArrays HFree

static BOOLEAN
InitTree (void)
{
	if ((encode_arrays = AllocEncodeArrays ()) == NULL)
	{
		FreeCodeArrays (encode_arrays);
		encode_arrays = NULL;
		return (FALSE);
	}
	else
	{
		SWORD i;

		lson = encode_arrays;
		rson = lson + (N + 1);
		dad = rson + (N + 257);

		for (i = N + 1; i <= N + 256; i++)
			rson[i] = NIL; /* root */
		for (i = 0; i < N; i++)
			dad[i] = NIL; /* node */

		return (TRUE);
	}
}

static void
InsertNode (SWORD r)
{
	SWORD p, cmp;
	BYTE *lpBuf;

	cmp = 1;
	lpBuf = _lpCurCodeDesc->text_buf;
	p = N + 1 + lpBuf[r];
	rson[r] = lson[r] = NIL;
	match_length = 0;
	for (;;)
	{
		UWORD i;

		if (cmp >= 0)
		{
			if (rson[p] != NIL)
				p = rson[p];
			else
			{
				rson[p] = r;
				dad[r] = p;
				return;
			}
		}
		else
		{
			if (lson[p] != NIL)
				p = lson[p];
			else
			{
				lson[p] = r;
				dad[r] = p;
				return;
			}
		}

		i = F;
		{
			SWORD _r, _p;

			_r = r;
			_p = p;
			while (--i && (cmp = lpBuf[++_r] - lpBuf[++_p]) == 0)
				;
		}
		if ((i = F - i) > THRESHOLD)
		{
			if (i > match_length)
			{
				match_position = ((r - p) & (N - 1)) - 1;
				if ((match_length = i) >= F)
					break;
			}
			else if (i == match_length)
			{
				if ((i = ((r - p) & (N - 1)) - 1) < match_position)
				{
					match_position = i;
				}
			}
		}
	}
	dad[r] = dad[p];
	lson[r] = lson[p];
	rson[r] = rson[p];
	dad[lson[p]] = r;
	dad[rson[p]] = r;
	if (rson[dad[p]] == p)
		rson[dad[p]] = r;
	else
		lson[dad[p]] = r;
	dad[p] = NIL;  /* remove p */
}

static void
DeleteNode (SWORD p)
{
	SWORD q;

	if (dad[p] == NIL)
		return; /* unregistered */
	if (rson[p] == NIL)
		q = lson[p];
	else if (lson[p] == NIL)
		q = rson[p];
	else
	{
		q = lson[p];
		if (rson[q] != NIL)
		{
			do
			{
				q = rson[q];
			} while (rson[q] != NIL);
			rson[dad[q]] = lson[q];
			dad[lson[q]] = dad[q];
			lson[q] = lson[p];
			dad[lson[p]] = q;
		}
		rson[q] = rson[p];
		dad[rson[p]] = q;
	}
	dad[q] = dad[p];
	if (rson[dad[p]] == p)
		rson[dad[p]] = q;
	else
		lson[dad[p]] = q;
	dad[p] = NIL;
}

static void
Putcode (SWORD l, UWORD c)
{
	_workbuf |= c >> _workbuflen;
	if ((_workbuflen += l) >= 8)
	{
		OutChar ((BYTE)(_workbuf >> 8));
		++_lpCurCodeDesc->StreamIndex;
		if ((_workbuflen -= 8) >= 8)
		{
			OutChar ((BYTE)(_workbuf));
			++_lpCurCodeDesc->StreamIndex;
			_workbuflen -= 8;
			_workbuf = c << (l - _workbuflen);
		}
		else
		{
			_workbuf <<= 8;
		}
		_workbuf &= 0xFFFF;
	}
}

static void
EncodeChar (UWORD c)
{
	UWORD i;
	SWORD j, k;

	i = 0;
	j = 0;
	k = _lpCurCodeDesc->prnt[c + T];

	/* search connections from leaf node to the root */
	do
	{
		i >>= 1;

		/*
		if node's address is odd, output 1
		else output 0
		*/
		if (k & 1)
			i += 0x8000;

		j++;
	} while ((k = _lpCurCodeDesc->prnt[k]) != R);
	Putcode (j, i);
	_update (c + T);
}

static void
EncodePosition (UWORD c)
{
	UWORD i;
		/*
		 * Tables for encoding/decoding upper 6 bits of
		 * sliding dictionary pointer
		 */
		/* encoder table */
	static const BYTE p_len[64] =
	{
		0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05,
		0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06,
		0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
		0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
		0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
		0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
		0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
		0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
	};

	static const BYTE p_code[64] =
	{
		0x00, 0x20, 0x30, 0x40, 0x50, 0x58, 0x60, 0x68,
		0x70, 0x78, 0x80, 0x88, 0x90, 0x94, 0x98, 0x9C,
		0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
		0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
		0xD0, 0xD2, 0xD4, 0xD6, 0xD8, 0xDA, 0xDC, 0xDE,
		0xE0, 0xE2, 0xE4, 0xE6, 0xE8, 0xEA, 0xEC, 0xEE,
		0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
		0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
	};

	/* output upper 6 bits with encoding */
	i = c >> 6;
	Putcode (p_len[i], (UWORD)p_code[i] << 8);

	/* output lower 6 bits directly */
	Putcode (6, (c & 0x3f) << 10);
}

static void
UninitTree (void)
{
	if (_workbuflen)
	{
		OutChar ((BYTE)(_workbuf >> 8));
		++_lpCurCodeDesc->StreamIndex;
	}

	FreeCodeArrays (encode_arrays);
	encode_arrays = NULL;
	lson = NULL;
	rson = NULL;
	dad = NULL;
}

static void
_encode_cleanup (void)
{
	UWORD r, s, last_match_length, len;

	_StreamType = _lpCurCodeDesc->StreamType;
	_Stream = _lpCurCodeDesc->Stream;
	_workbuf = _lpCurCodeDesc->workbuf;
	_workbuflen = _lpCurCodeDesc->workbuflen;

	r = _lpCurCodeDesc->buf_index;
	s = _lpCurCodeDesc->restart_index;
	last_match_length = _lpCurCodeDesc->bytes_left;
	if (_lpCurCodeDesc->StreamLength >= F)
		len = F;
	else
	{
		UWORD i;

		for (i = 1; i <= F; i++)
			InsertNode (r - i);
		InsertNode (r);

		len = (UWORD)_lpCurCodeDesc->StreamLength;
	}

	while (1)
	{
		while (last_match_length--)
		{
			DeleteNode (s);
			if (--len == 0)
			{
				BYTE lobyte, hibyte;
				UWORD loword, hiword;

				UninitTree ();

				_lpCurCodeDesc->StreamIndex += 4;
						/* rewind */
				if (_lpCurCodeDesc->StreamType == FILE_STREAM)
					SeekResFile ((uio_Stream *)_Stream,
							-(int)_lpCurCodeDesc->StreamIndex, SEEK_CUR);
				else /* _lpCurCodeDesc->StreamType == MEMORY_STREAM */
					_Stream = (BYTE*)_Stream - _lpCurCodeDesc->StreamIndex;

				loword = LOWORD (_lpCurCodeDesc->StreamLength);
				lobyte = LOBYTE (loword);
				hibyte = HIBYTE (loword);
				OutChar (lobyte);
				OutChar (hibyte);
				hiword = HIWORD (_lpCurCodeDesc->StreamLength);
				lobyte = LOBYTE (hiword);
				hibyte = HIBYTE (hiword);
				OutChar (lobyte);
				OutChar (hibyte);

				return;
			}
			s = (s + 1) & (N - 1);
			r = (r + 1) & (N - 1);
			InsertNode (r);
		}
		if (match_length > len)
			match_length = len;
		if (match_length <= THRESHOLD)
		{
			match_length = 1;
			EncodeChar (_lpCurCodeDesc->text_buf[r]);
		}
		else
		{
			EncodeChar (255 - THRESHOLD + match_length);
			EncodePosition (match_position);
		}
		last_match_length = match_length;
	}
}

COUNT
cwrite (const void *buf, COUNT size, COUNT count, PLZHCODE_DESC lpCodeDesc)
{
	UWORD r, s, last_match_length;
	BYTE *lpBuf;
	const BYTE *lpStr;

	if ((_lpCurCodeDesc = lpCodeDesc) == 0
			|| (size *= count) == 0)
		return (0);

	_StreamType = lpCodeDesc->StreamType;
	_Stream = lpCodeDesc->Stream;
	_workbuf = lpCodeDesc->workbuf;
	_workbuflen = lpCodeDesc->workbuflen;
	lpStr = (const BYTE *) buf;
	lpBuf = lpCodeDesc->text_buf;

	r = lpCodeDesc->buf_index;
	s = lpCodeDesc->restart_index;
	last_match_length = lpCodeDesc->bytes_left;
	if (last_match_length)
	{
		lpCodeDesc->StreamLength += size;
		goto EncodeRestart;
	}
	else if (lpCodeDesc->StreamLength < F)
	{
		UWORD i;

		if ((i = (UWORD)lpCodeDesc->StreamLength) == 0)
		{
			if (!InitTree ())
				return (0);

			_lpCurCodeDesc->StreamIndex = 0;
			lpCodeDesc->CleanupFunc = _encode_cleanup;
		}

		lpCodeDesc->StreamLength += size;

		for (; i < F && size; ++i, --size)
			lpBuf[r + i] = *lpStr++;
		if (i < F)
			goto EncodeExit;

		for (i = 1; i <= F; i++)
			InsertNode (r - i);
		InsertNode (r);
		if (size == 0)
			goto EncodeExit;
	}
	else
		lpCodeDesc->StreamLength += size;

	do
	{
		if (match_length > F)
			match_length = F;
		if (match_length <= THRESHOLD)
		{
			match_length = 1;
			EncodeChar (lpBuf[r]);
		}
		else
		{
			EncodeChar (255 - THRESHOLD + match_length);
			EncodePosition (match_position);
		}
		last_match_length = match_length;
EncodeRestart:
		while (last_match_length && size)
		{
			BYTE c;

			--size;
			--last_match_length;

			DeleteNode (s);
			c = *lpStr++;
			lpBuf[s] = c;
			if (s < F - 1)
				lpBuf[s + N] = c;
			s = (s + 1) & (N - 1);
			r = (r + 1) & (N - 1);
			InsertNode (r);
		}
	} while (last_match_length == 0);

EncodeExit:
	lpCodeDesc->buf_index = r;
	lpCodeDesc->restart_index = s;
	lpCodeDesc->bytes_left = last_match_length;

	lpCodeDesc->Stream = _Stream;
	lpCodeDesc->workbuf = _workbuf;
	lpCodeDesc->workbuflen = _workbuflen;

	return (count);
}

