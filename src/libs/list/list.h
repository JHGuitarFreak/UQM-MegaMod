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

// The 'already included' check must be done slightly more complicated
// than usually. This file may be included directly only once,
// but it may be included my derivative List definitions that use
// this file as a template more than once.
#if !defined(_LIST_H) || defined(LIST_GENERIC)
#if defined(LIST_)
#	define LIST_GENERIC
#endif

#include "types.h"
#include "port.h"

// You can use inline lists, by using this file as a template.
// To do this, make a new .h and .c file. In the .h file, define the macros
// (and typedefs) from the LIST_ block below.
// In the .c file, #define LIST_INTERNAL, #include the .h file
// and list.c (in this order), and add the necessary functions.
#ifndef LIST_
#	define LIST_(identifier) List ## _ ## identifier
	typedef void *List_Entry;
#endif


typedef struct LIST_(List) LIST_(List);
typedef struct LIST_(Link) LIST_(Link);

struct LIST_(Link) {
	LIST_(Entry) entry;
	LIST_(Link) *next;
};

struct LIST_(List) {
	LIST_(Link) *first;
	LIST_(Link) **end;
};


LIST_(List) *LIST_(newList)(void);
void LIST_(deleteList)(LIST_(List) *list);
void LIST_(add)(LIST_(List) *list, LIST_(Entry) entry);
void LIST_(remove)(LIST_(List) *list, LIST_(Entry) entry);


#ifndef LIST_INTERNAL
#	undef LIST_
#endif

#endif /* !defined(_LIST_H) || defined(LIST_GENERIC) */


