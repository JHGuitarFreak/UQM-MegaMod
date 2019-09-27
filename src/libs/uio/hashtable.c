/*
 * Copyright (C) 2003  Serge van den Boom
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

#ifndef HASHTABLE_INTERNAL
	// If HASHTABLE_INTERNAL is already defined, this file is included
	// as a template. In this case hashtable.h has already been included.
#	define HASHTABLE_INTERNAL
#	include "hashtable.h"
#endif

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "mem.h"
#include "uioport.h"

static void HASHTABLE_(setup)(HASHTABLE_(HashTable) *HashTable,
		uio_uint32 size);
static void HASHTABLE_(resize)(HASHTABLE_(HashTable) *hashTable);
static inline uio_uint32 nextPower2(uio_uint32 x);

static inline HASHTABLE_(HashTable) *HASHTABLE_(allocHashTable)(void);
static inline HASHTABLE_(HashEntry) *HASHTABLE_(newHashEntry)(uio_uint32 hash,
		HASHTABLE_(Key) *key, HASHTABLE_(Value) *value,
		HASHTABLE_(HashEntry) *next);
static inline HASHTABLE_(HashEntry) *HASHTABLE_(allocHashEntry)(void);
static inline void HASHTABLE_(freeHashEntry)(
		HASHTABLE_(HashEntry) *entry);

// Create a new HashTable.
HASHTABLE_(HashTable) *
HASHTABLE_(newHashTable)(
		HASHTABLE_(HashFunction) hashFunction,
		HASHTABLE_(EqualFunction) equalFunction,
		HASHTABLE_(CopyFunction) copyFunction,
		HASHTABLE_(FreeKeyFunction) freeKeyFunction,
		HASHTABLE_(FreeValueFunction) freeValueFunction,
		uio_uint32 initialSize,
		double minFillQuotient,
		double maxFillQuotient) {
	HASHTABLE_(HashTable) *hashTable;

	assert(maxFillQuotient >= minFillQuotient);
	
	hashTable = HASHTABLE_(allocHashTable)();
	hashTable->hashFunction = hashFunction;
	hashTable->equalFunction = equalFunction;
	hashTable->copyFunction = copyFunction;
	hashTable->freeKeyFunction = freeKeyFunction;
	hashTable->freeValueFunction = freeValueFunction;

	hashTable->minFillQuotient = minFillQuotient;
	hashTable->maxFillQuotient = maxFillQuotient;
	HASHTABLE_(setup)(hashTable, initialSize);

	return hashTable;
}

// Add an entry to the HashTable.
uio_bool
HASHTABLE_(add)(HASHTABLE_(HashTable) *hashTable,
		const HASHTABLE_(Key) *key, HASHTABLE_(Value) *value) {
	uio_uint32 hash;
	struct HASHTABLE_(HashEntry) *entry;

	hash = HASHTABLE_(HASH)(hashTable, key);
	entry = hashTable->entries[hash & hashTable->hashMask];
	while (entry != NULL) {
		if (HASHTABLE_(EQUAL)(hashTable, key, entry->key)) {
			// key is already present
			return false;
		}
		entry = entry->next;
	}

#ifdef HashTable_PROFILE
	if (hashTable->entries[hash & hashTable->hashMask] != NULL)
		hashTable->numCollisions++;
#endif
	hashTable->entries[hash & hashTable->hashMask] =
			HASHTABLE_(newHashEntry)(hash,
			HASHTABLE_(COPY)(hashTable, key), value,
			hashTable->entries[hash & hashTable->hashMask]);

	hashTable->numEntries++;
	if (hashTable->numEntries > hashTable->maxSize)
		HASHTABLE_(resize)(hashTable);

	return true;
}

// Remove an entry with a specified Key from the HashTable.
uio_bool
HASHTABLE_(remove)(HASHTABLE_(HashTable) *hashTable,
		const HASHTABLE_(Key) *key) {
	uio_uint32 hash;
	struct HASHTABLE_(HashEntry) **entry, *next;

	hash = HASHTABLE_(HASH)(hashTable, key);
	entry = &hashTable->entries[hash & hashTable->hashMask];
	while (1) {
		if (*entry == NULL)
			return false;
		if (HASHTABLE_(EQUAL)(hashTable, key, (*entry)->key)) {
			// found the key
			break;
		}
		entry = &(*entry)->next;
	}
	next = (*entry)->next;
	HASHTABLE_(FREEKEY)(hashTable, (*entry)->key);
	HASHTABLE_(FREEVALUE)(hashTable, (*entry)->value);
	HASHTABLE_(freeHashEntry)(*entry);
	*entry = next;

	hashTable->numEntries--;
	if (hashTable->numEntries < hashTable->minSize)
		HASHTABLE_(resize)(hashTable);
	
	return true;
}

// Find the Value stored for some Key.
HASHTABLE_(Value) *
HASHTABLE_(find)(HASHTABLE_(HashTable) *hashTable,
		const HASHTABLE_(Key) *key) {
	uio_uint32 hash;
	struct HASHTABLE_(HashEntry) *entry;

	hash = HASHTABLE_(HASH)(hashTable, key);
	entry = hashTable->entries[hash & hashTable->hashMask];
	while (entry != NULL) {
		if (HASHTABLE_(EQUAL)(hashTable, key, entry->key)) {
			// found the key
			return entry->value;
		}
		entry = entry->next;
	}
	return NULL;
}

// Returns the number of entries in the HashTable.
uio_uint32
HASHTABLE_(count)(const HASHTABLE_(HashTable) *hashTable) {
	return hashTable->numEntries;
}

// Auxiliary function to (re)initialise the buckets in the HashTable.
static void
HASHTABLE_(setup)(HASHTABLE_(HashTable) *hashTable, uio_uint32 initialSize) {
	if (initialSize < 4)
		initialSize = 4;
	hashTable->size = nextPower2(ceil(((double) initialSize) /
			hashTable->maxFillQuotient));
	hashTable->hashMask = hashTable->size - 1;
	hashTable->minSize = ceil(((double) (hashTable->size >> 1))
			* hashTable->minFillQuotient);
	hashTable->maxSize = floor(((double) hashTable->size)
			* hashTable->maxFillQuotient);
	hashTable->entries = uio_calloc(hashTable->size,
			sizeof (HASHTABLE_(HashEntry) *));
	hashTable->numEntries = 0;
#ifdef HashTable_PROFILE
	hashTable->numCollisions = 0;
#endif
}

// Resize the buckets in the HashTable.
static void
HASHTABLE_(resize)(HASHTABLE_(HashTable) *hashTable) {
	HASHTABLE_(HashEntry) **oldEntries;
	HASHTABLE_(HashEntry) *entry, *next;
	HASHTABLE_(HashEntry) **newLocation;
	uio_uint32 oldNumEntries;
	uio_uint32 i;
	
	oldNumEntries = hashTable->numEntries;
	oldEntries = hashTable->entries;

	HASHTABLE_(setup)(hashTable, hashTable->numEntries);
	hashTable->numEntries = oldNumEntries;
	
	i = 0;
	while (oldNumEntries > 0) {
		entry = oldEntries[i];
		while (entry != NULL) {
			next = entry->next;
			newLocation = &hashTable->entries[entry->hash &
					hashTable->hashMask];
#ifdef HashTable_PROFILE
			if (*newLocation != NULL)
				hashTable->numCollisions++;
#endif
			entry->next = *newLocation;
			*newLocation = entry;
			oldNumEntries--;
			entry = next;
		}
		i++;
	}
	
	uio_free(oldEntries);
}

// Adapted from "Hackers Delight"
// Returns the smallest power of two greater or equal to x.
static inline uio_uint32
nextPower2(uio_uint32 x) {
	x--;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

// Get an iterator to iterate through all the entries in the HashTable.
// NB: Iterator should be considered invalid if the HashTable is changed.
// TODO: change this (make it thread-safe)
//       this can be done by only marking items as deleted when
//       there are outstanding iterators.
HASHTABLE_(Iterator) *
HASHTABLE_(getIterator)(const HASHTABLE_(HashTable) *hashTable) {
	HASHTABLE_(Iterator) *iterator;
	uio_uint32 i;

	iterator = uio_malloc(sizeof (HASHTABLE_(Iterator)));
	iterator->hashTable = hashTable;
	
	// Look for the first used bucket.
	for (i = 0; i < iterator->hashTable->size; i++) {
		if (iterator->hashTable->entries[i] != NULL) {
			// Found a used bucket.
			iterator->bucketNr = i;
			iterator->entry = iterator->hashTable->entries[i];
			return iterator;
		}
	}

	// No entries were found.
	iterator->bucketNr = i;
	iterator->entry = NULL;
	return iterator;
}

// Returns true if and only if there are no more entries in the hash table
// for the Iterator to find.
int
HASHTABLE_(iteratorDone)(const HASHTABLE_(Iterator) *iterator) {
	return iterator->bucketNr >= iterator->hashTable->size;
}

// Get the Key of the entry pointed to by an Iterator.
HASHTABLE_(Key) *
HASHTABLE_(iteratorKey)(HASHTABLE_(Iterator) *iterator) {
	return iterator->entry->key;
}

// Get the Value of the entry pointed to by an Iterator.
HASHTABLE_(Value) *
HASHTABLE_(iteratorValue)(HASHTABLE_(Iterator) *iterator) {
	return iterator->entry->value;
}

// Move the Iterator to the next entry in the HashTable.
// Should not be called if the iterator is already past the last entry.
HASHTABLE_(Iterator) *
HASHTABLE_(iteratorNext)(HASHTABLE_(Iterator) *iterator) {
	uio_uint32 i;

	// If there's another entry in this bucket, use that.
	iterator->entry = iterator->entry->next;
	if (iterator->entry != NULL)
		return iterator;

	// Look for the next used bucket.
	for (i = iterator->bucketNr + 1; i < iterator->hashTable->size; i++) {
		if (iterator->hashTable->entries[i] != NULL) {
			// Found another used bucket.
			iterator->bucketNr = i;
			iterator->entry = iterator->hashTable->entries[i];
			return iterator;
		}
	}

	// No more entries were found.
	iterator->bucketNr = i;
	iterator->entry = NULL;
	return iterator;
}

// Free the Iterator.
void
HASHTABLE_(freeIterator)(HASHTABLE_(Iterator) *iterator) {
	uio_free(iterator);
}

// Auxiliary function to allocate a HashTable.
static inline HASHTABLE_(HashTable) *
HASHTABLE_(allocHashTable)(void) {
	return uio_malloc(sizeof (HASHTABLE_(HashTable)));
}

// Auxiliary function to create a HashEntry.
static inline HASHTABLE_(HashEntry) *
HASHTABLE_(newHashEntry)(uio_uint32 hash, HASHTABLE_(Key) *key,
		HASHTABLE_(Value) *value, HASHTABLE_(HashEntry) *next) {
	HASHTABLE_(HashEntry) *result;

	result = HASHTABLE_(allocHashEntry)();
	result->hash = hash;
	result->key = key;
	result->value = value;
	result->next = next;
	return result;
}

// Allocate a new HashEntry.
static inline HASHTABLE_(HashEntry) *
HASHTABLE_(allocHashEntry)(void) {
	return uio_malloc(sizeof (HASHTABLE_(HashEntry)));
}

// Delete the HashTable.
void
HASHTABLE_(deleteHashTable)(HASHTABLE_(HashTable) *hashTable) {
	uio_uint32 i;
	HASHTABLE_(HashEntry) *entry, *next;
	HASHTABLE_(HashEntry) **bucketPtr;

	i = hashTable->numEntries;
	bucketPtr = hashTable->entries;
	while (i > 0) {
		entry = *bucketPtr;
		while (entry != NULL) {
			next = entry->next;
			HASHTABLE_(FREEKEY)(hashTable, entry->key);
			HASHTABLE_(FREEVALUE)(hashTable, entry->value);
			HASHTABLE_(freeHashEntry)(entry);
			entry = next;
			i--;
		}
		bucketPtr++;
	}
	uio_free(hashTable->entries);
	uio_free(hashTable);
}

// Auxiliary function to deallocate a HashEntry.
static inline void
HASHTABLE_(freeHashEntry)(HASHTABLE_(HashEntry) *entry) {
	uio_free(entry);
}

