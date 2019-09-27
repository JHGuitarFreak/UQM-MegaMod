/*
 *  Copyright 2006  Serge van den Boom <svdb@stack.nl>
 *
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef LIBS_HEAP_HEAP_H_
#define LIBS_HEAP_HEAP_H_

#include "types.h"

#include <sys/types.h>
#include <stdlib.h>


typedef struct Heap Heap;
typedef struct HeapValue HeapValue;

// The actual value stored should "inherit" from this.
struct HeapValue {
	size_t index;
};
typedef int (*HeapValue_Comparator)(HeapValue *v1, HeapValue *v2);


struct Heap {
	HeapValue_Comparator comparator;
			// Comparison function to determine the order of the
			// elements.
	size_t minSize;
			// Never resize below this many number of entries.
	double minFillQuotient;
			// How much of half of the heap needs to be filled before
			// resizing to size/2.
	
	HeapValue **entries;
			// Actual values
	size_t numEntries;
	size_t size;
			// Number of entries that fit in the heap as it is now.
	size_t minFill;
			// Resize to size/2 when below this size.
};


Heap *Heap_new(HeapValue_Comparator comparator, size_t initialSize,
		size_t minSize, double minFillQuotient);
void Heap_delete(Heap *heap);
void Heap_add(Heap *heap, HeapValue *value);
HeapValue *Heap_first(const Heap *heap);
HeapValue *Heap_pop(Heap *heap);
size_t Heap_count(const Heap *heap);
bool Heap_hasMore(const Heap *heap);
void Heap_remove(Heap *heap, HeapValue *value);

#endif  /* LIBS_HEAP_HEAP_H_ */

