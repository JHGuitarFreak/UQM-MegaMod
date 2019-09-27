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

#include <string.h>
#include "lzh.h"

static void
reconst (void)
{
	COUNT i, j;

	/* halven cumulative freq for leaf nodes */
	j = 0;
	for (i = 0; i < T; i++)
	{
		if (_lpCurCodeDesc->son[i] >= T)
		{
			_lpCurCodeDesc->freq[j] = (_lpCurCodeDesc->freq[i] + 1) >> 1;
			_lpCurCodeDesc->son[j] = _lpCurCodeDesc->son[i];
			j++;
		}
	}
	/* make a tree : first, connect children nodes */
	for (i = 0, j = N_CHAR; j < T; i += 2, j++)
	{
		SWORD k;
		UWORD f, l;

		k = i + 1;
		f = _lpCurCodeDesc->freq[j] = _lpCurCodeDesc->freq[i] + _lpCurCodeDesc->freq[k];
		for (k = j - 1; f < _lpCurCodeDesc->freq[k]; k--)
			;
		k++;
		l = (j - k);
		
		memmove (_lpCurCodeDesc->freq + k + 1, _lpCurCodeDesc->freq + k,
				sizeof(_lpCurCodeDesc->freq[0]) * l);
		_lpCurCodeDesc->freq[k] = f;
		memmove (_lpCurCodeDesc->son + k + 1, _lpCurCodeDesc->son + k,
				sizeof(_lpCurCodeDesc->son[0]) * l);
		_lpCurCodeDesc->son[k] = i;
	}
	/* connect parent nodes */
	for (i = 0; i < T; i++)
	{
		if ((j = _lpCurCodeDesc->son[i]) >= T)
			_lpCurCodeDesc->prnt[j] = i;
		else
			_lpCurCodeDesc->prnt[j] = _lpCurCodeDesc->prnt[j + 1] = i;
	}
}


/* update freq tree */

void
_update (COUNT c)
{
	PLZHCODE_DESC lpCD;

	if ((lpCD = _lpCurCodeDesc)->freq[R] == MAX_FREQ)
		reconst ();

	c = lpCD->prnt[c];
	do
	{
		COUNT i, l;

		i = ++lpCD->freq[c];

		/* swap nodes to keep the tree freq-ordered */
		if (i > lpCD->freq[l = c + 1])
		{
			COUNT j;

			while (i > lpCD->freq[++l])
				;
			l--;
			lpCD->freq[c] = lpCD->freq[l];
			lpCD->freq[l] = i;

			i = lpCD->son[c];
			j = lpCD->son[l];
			lpCD->son[l] = i;
			lpCD->son[c] = j;

			lpCD->prnt[i] = l;
			if (i < T)
				lpCD->prnt[i + 1] = l;

			lpCD->prnt[j] = c;
			if (j < T)
				lpCD->prnt[j + 1] = c;

			c = l;
		}
	} while ((c = lpCD->prnt[c]) != 0); /* do it until reaching the root */
}


