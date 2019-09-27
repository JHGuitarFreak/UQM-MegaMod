/*  stringbank.h, Copyright (c) 2005 Michael C. Martin */

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

#ifndef LIBS_RESOURCE_STRINGBANK_H_
#define LIBS_RESOURCE_STRINGBANK_H_

#ifdef SB_DEBUG
#include <stdio.h>
#endif

#define STRBANK_CHUNK_SIZE (1024 - sizeof (void *) - sizeof (int))

typedef struct _stringbank_chunk {
	char data[STRBANK_CHUNK_SIZE];
	int len;
	struct _stringbank_chunk *next;
} stringbank;

/* Constructors and destructors */
stringbank *StringBank_Create (void);
void StringBank_Free (stringbank *bank);

/* Put str or n chars after str into the string bank. */
const char *StringBank_AddString (stringbank *bank, const char *str);
const char *StringBank_AddSubstring (stringbank *bank, const char *str, unsigned int n);

/* Put str or n chars after str into the string bank if it's not already
   there.  Much slower. */
const char *StringBank_AddOrFindString (stringbank *bank, const char *str);
const char *StringBank_AddOrFindSubstring (stringbank *bank, const char *str, unsigned int n);

/* Split a string s into at most n substrings, separated by splitchar.
   Pointers to these substrings will be stored in result; the
   substrings themselves will be filed in the specified stringbank. */
int SplitString (const char *s, char splitchar, int n, const char **result, stringbank *bank);

#ifdef SB_DEBUG
/* Print out a list of the contents of the string bank to the named stream. */
void StringBank_Dump (stringbank *bank, FILE *s);
#endif  /* SB_DEBUG */

#endif  /* LIBS_RESOURCE_STRINGBANK_H_ */
