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

#include "heap.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "port.h"

static inline size_t nextPower2(size_t x);


static void
Heap_resize(Heap *heap, size_t size) {
	heap->entries = realloc(heap->entries, size * sizeof (HeapValue *));
	heap->size = size;
}

// Heap inv: comparator(parent, child) <= 0 for every child of every parent.
Heap *
Heap_new(HeapValue_Comparator comparator, size_t initialSize, size_t minSize,
		double minFillQuotient) {
	Heap *heap;

	assert(minFillQuotient >= 0.0);

	heap = malloc(sizeof (Heap));

	if (initialSize < minSize)
		initialSize = minSize;

	heap->comparator = comparator;
	heap->minSize = minSize;
	heap->minFillQuotient = minFillQuotient;
	heap->size = nextPower2(initialSize);
	heap->minFill = ceil(((double) (heap->size >> 1))
			* heap->minFillQuotient);
	heap->entries = malloc(heap->size * sizeof (HeapValue *));
	heap->numEntries = 0;

	return heap;
}

void
Heap_delete(Heap *heap) {
	free(heap->entries);
	free(heap);
}

void
Heap_add(Heap *heap, HeapValue *value) {
	size_t i;

	if (heap->numEntries >= heap->size)
		Heap_resize(heap, heap->size * 2);

	i = heap->numEntries;
	heap->numEntries++;

	while (i > 0) {
		size_t parentI = (i - 1) / 2;
		if (heap->comparator(heap->entries[parentI], value) <= 0)
			break;

		heap->entries[i] = heap->entries[parentI];
		heap->entries[i]->index = i;
		i = parentI;
	}
	heap->entries[i] = value;
	heap->entries[i]->index = i;
}

HeapValue *
Heap_first(const Heap *heap) {
	assert(heap->numEntries > 0);

	return heap->entries[0];
}

static void
Heap_removeByIndex(Heap *heap, size_t i) {
	assert(heap->numEntries > i);

	heap->numEntries--;

	if (heap->numEntries != 0) {
		// Restore the heap invariant. We're shifting entries into the
		// gap that was created until we find the place where we can
		// insert the last entry.
		HeapValue *lastEntry = heap->entries[heap->numEntries];

		for (;;) {
			size_t childI = i * 2 + 1;
			// The two children are childI and 'childI + 1'.

			if (childI + 1 >= heap->numEntries) {
				// There is no right child.

				if (childI >= heap->numEntries) {
					// There is no left child either.
					break;
				}
			} else {
				if (heap->comparator(heap->entries[childI + 1],
						heap->entries[childI]) < 0) {
					// The right child is the child with the lowest value.
					childI++;
				}
			}
			// childI is now the child with the lowest value.

			if (heap->comparator(lastEntry, heap->entries[childI]) <= 0) {
				// The last entry goes here.
				break;
			}

			// Move the child into the gap.
			heap->entries[i] = heap->entries[childI];
			heap->entries[i]->index = i;

			// and repeat for the child.
			i = childI;
		}

		// Fill the gap with the last entry.
		heap->entries[i] = lastEntry;
		heap->entries[i]->index = i;
	}

	// Resize if necessary:
	if (heap->numEntries < heap->minFill &&
			heap->numEntries > heap->minSize)
		Heap_resize(heap, heap->size / 2);
}

HeapValue *
Heap_pop(Heap *heap) {
	HeapValue *result;

	assert(heap->numEntries > 0);

	result = heap->entries[0];
	Heap_removeByIndex(heap, 0);

	return result;
}

size_t
Heap_count(const Heap *heap) {
	return heap->numEntries;
}

bool
Heap_hasMore(const Heap *heap) {
	return heap->numEntries > 0;
}

void
Heap_remove(Heap *heap, HeapValue *value) {
	Heap_removeByIndex(heap, value->index);
}

// Adapted from "Hackers Delight"
// Returns the smallest power of two greater or equal to x.
static inline size_t
nextPower2(size_t x) {
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
#	if (SIZE_MAX > 0xffff)
		x |= x >> 16;
#		if (SIZE_MAX > 0xffffffff)
			x |= x >> 32;
#		endif
#	endif
	return x + 1;
}


