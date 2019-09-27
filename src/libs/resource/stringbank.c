/*  stringbank.c, Copyright (c) 2005 Michael C. Martin */

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope thta it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  Se the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stringbank.h"

typedef stringbank chunk;

static stringbank *
add_chunk (stringbank *bank)
{
	stringbank *n = malloc (sizeof (stringbank));
	n->len = 0;
	n->next = NULL;
	if (bank)
	{
		while (bank->next)
			bank = bank->next;
		bank->next = n;
	}
	return n;
}

stringbank *
StringBank_Create (void)
{
	return add_chunk (NULL);
}

void
StringBank_Free (stringbank *bank)
{
	if (bank)
	{
		StringBank_Free (bank->next);
		free (bank);
	}
}

const char *
StringBank_AddString (stringbank *bank, const char *str)
{
	unsigned int len = strlen (str) + 1;
	stringbank *x = bank;
	if (len > STRBANK_CHUNK_SIZE)
		return NULL;
	while (x) {
		unsigned int remaining = STRBANK_CHUNK_SIZE - x->len;
		if (len < remaining) {
			char *result = x->data + x->len;
			strcpy (result, str);
			x->len += len;
			return result;
		}
		x = x->next;
	}
	/* No room in any currently existing chunk */
	x = add_chunk (bank);
	strcpy (x->data, str);
	x->len += len;
	return x->data;
}

const char *
StringBank_AddOrFindString (stringbank *bank, const char *str)
{
	unsigned int len = strlen (str) + 1;
	stringbank *x = bank;
	if (len > STRBANK_CHUNK_SIZE)
		return NULL;
	while (x) {
		int i = 0;
		while (i < x->len) {
			if (!strcmp (x->data + i, str))
				return x->data + i;
			while (x->data[i]) i++;
			i++;
		}
		x = x->next;
	}
	/* We didn't find it, so add it */
	return StringBank_AddString (bank, str);
}

static char buffer[STRBANK_CHUNK_SIZE];

const char *
StringBank_AddSubstring (stringbank *bank, const char *str, unsigned int n)
{
	unsigned int len = strlen (str);
	if (n > len)
	{
		return StringBank_AddString (bank, str);
	}
	if (n >= STRBANK_CHUNK_SIZE)
	{
		return NULL;
	}
	strncpy (buffer, str, n);
	buffer[n] = '\0';
	return StringBank_AddString(bank, buffer);
}

const char *
StringBank_AddOrFindSubstring (stringbank *bank, const char *str, unsigned int n)
{
	unsigned int len = strlen (str);
	if (n > len)
	{
		return StringBank_AddOrFindString (bank, str);
	}
	if (n >= STRBANK_CHUNK_SIZE)
	{
		return NULL;
	}
	strncpy (buffer, str, n);
	buffer[n] = '\0';
	return StringBank_AddOrFindString(bank, buffer);
}

int
SplitString (const char *s, char splitchar, int n, const char **result, stringbank *bank)
{
        int i;
        const char *index = s;

        for (i = 0; i < n-1; i++)
        {
                const char *next;
                int len;

                next = strchr (index, splitchar);
                if (!next)
                {
                        break;
                }

                len = next - index;
                result[i] = StringBank_AddOrFindSubstring (bank, index, len);
                index = next+1;
        }
        result[i] = StringBank_AddOrFindString (bank, index);
        return i+1;
}

#ifdef SB_DEBUG

void
StringBank_Dump (stringbank *bank, FILE *s)
{
	stringbank *x = bank;
	while (x) {
		int i = 0;
		while (i < x->len) {
			fprintf (s, "\"%s\"\n", x->data + i);
			while (x->data[i]) i++;
			i++;
		}
		x = x->next;
	}
}

#endif
