/*
 * Copyright (C) 2005  Serge van den Boom
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

#ifndef LIST_INTERNAL
	// If list is already defined, this file is included
	// as a template. In this case list.h has already been included.
#	define LIST_INTERNAL
#	include "list.h"
#endif

#include "libs/memlib.h"
#define malloc HMalloc
#define free HFree
#define realloc HRealloc

#include <assert.h>

static inline LIST_(List) *LIST_(allocList)(void);
static inline void LIST_(freeList)(LIST_(List) *list);
static inline LIST_(Link) * LIST_(allocLink)(void);
static inline void LIST_(freeLink)(LIST_(Link) *link);


LIST_(List) *
LIST_(newList)(void) {
	LIST_(List) *list;

	list = LIST_(allocList)();
	if (list == NULL)
		return NULL;

	list->first = NULL;
	list->end = &list->first;
	return list;
}

void
LIST_(deleteList)(LIST_(List) *list)
{
	LIST_(Link) *link;
	LIST_(Link) *next;

	for (link = list->first; link != NULL; link = next)
	{
		next = link->next;
		LIST_(freeLink)(link);
	}

	LIST_(freeList)(list);
}

void
LIST_(add)(LIST_(List) *list, LIST_(Entry) entry) {
	LIST_(Link) *link;

	link = LIST_(allocLink)();
	link->entry = entry;
	link->next = NULL;
	*list->end = link;
	list->end = &link->next;
}

static inline LIST_(Link) **
LIST_(findLink)(LIST_(List) *list, LIST_(Entry) entry) {
	LIST_(Link) **linkPtr;

	for (linkPtr = &list->first; *linkPtr != NULL;
			linkPtr = &(*linkPtr)->next) {
		if ((*linkPtr)->entry == entry)
			return linkPtr;
	}
	return NULL;
}

static inline void
LIST_(removeLink)(LIST_(List) *list, LIST_(Link) **linkPtr) {
	LIST_(Link) *link = *linkPtr;

	*linkPtr = link->next;
	if (&link->next == list->end)
		list->end = linkPtr;
	LIST_(freeLink)(link);
}

void
LIST_(remove)(LIST_(List) *list, LIST_(Entry) entry) {
	LIST_(Link) **linkPtr;

	linkPtr = LIST_(findLink)(list, entry);
	assert(linkPtr != NULL);
	LIST_(removeLink)(list, linkPtr);
}


static inline LIST_(List) *
LIST_(allocList)(void) {
	return malloc(sizeof (LIST_(List)));
}

static inline void
LIST_(freeList)(LIST_(List) *list) {
	free(list);
}

static inline LIST_(Link) *
LIST_(allocLink)(void) {
	return malloc(sizeof (LIST_(Link)));
}

static inline void
LIST_(freeLink)(LIST_(Link) *link) {
	free(link);
}


