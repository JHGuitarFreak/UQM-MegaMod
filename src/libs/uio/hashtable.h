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

// The 'already included' check must be done slightly more complicated
// than usually. This file may be included directly only once,
// but it may be included my derivative HashTable definitions that use
// this file as a template more than once.
#if !defined(_HASHTABLE_H) || defined(HASHTABLE_GENERIC)
#if defined(HASHTABLE_)
#	define HASHTABLE_GENERIC
#endif

#include "types.h"
#include "uioport.h"

// Define to enable profiling.
#define HashTable_PROFILE

// You can use inline hash functions for extra speed, by using this file as
// a template.
// To do this, make a new .h and .c file. In the .h file, define the macros
// (and typedefs) from the HASHTABLE_ block below.
// In the .c file, #define HASHTABLE_INTERNAL, #include the .h file
// and hashtable.c (in this order), and add the necessary functions.
// If you do not need to free the Value, you can define HashTable_FREEVALUE
// as a no-op.
#ifndef HASHTABLE_
#	define HASHTABLE_(identifier) HashTable ## _ ## identifier
	typedef void HashTable_Key;
	typedef void HashTable_Value;
#	define HashTable_HASH(hashTable, hashValue) \
		(hashTable)->hashFunction(hashValue)
#	define HashTable_EQUAL(hashTable, hashKey1, hashKey2) \
		(hashTable)->equalFunction(hashKey1, hashKey2)
#	define HashTable_COPY(hashTable, hashKey) \
		(hashTable)->copyFunction(hashKey)
#	define HashTable_FREEKEY(hashTable, hashKey) \
		(hashTable)->freeKeyFunction(hashKey)
#	define HashTable_FREEVALUE(hashTable, hashValue) \
		(hashTable)->freeValueFunction(hashValue)
#endif



typedef uio_uint32 (*HASHTABLE_(HashFunction))(const HASHTABLE_(Key) *);
typedef uio_bool (*HASHTABLE_(EqualFunction))(const HASHTABLE_(Key) *,
		const HASHTABLE_(Key) *);
typedef HASHTABLE_(Value) *(*HASHTABLE_(CopyFunction))(
		const HASHTABLE_(Key) *);
typedef void (*HASHTABLE_(FreeKeyFunction))(HASHTABLE_(Key) *);
typedef void (*HASHTABLE_(FreeValueFunction))(HASHTABLE_(Value) *);

typedef struct HASHTABLE_(HashTable) HASHTABLE_(HashTable);
typedef struct HASHTABLE_(HashEntry) HASHTABLE_(HashEntry);
typedef struct HASHTABLE_(Iterator) HASHTABLE_(Iterator);

struct HASHTABLE_(HashTable) {
	HASHTABLE_(HashFunction) hashFunction;	
			// Function creating a uio_uint32 hash of a key.
	HASHTABLE_(EqualFunction) equalFunction;
			// Function used to compare two keys.
	HASHTABLE_(CopyFunction) copyFunction;
			// Function used to copy a key.
	HASHTABLE_(FreeKeyFunction) freeKeyFunction;
			// Function used to free a key.
	HASHTABLE_(FreeValueFunction) freeValueFunction;
			// Function used to free a value. Called when an entry is
			// removed using the remove function, or for entries
			// still in the HashTable when the HashTable is deleted.

	double minFillQuotient;
			// How much of half of the hashtable needs to be filled
			// before resizing to size/2.
	double maxFillQuotient;
			// How much of the hashTable needs to be filled before
			// resizing to size*2.
	uio_uint32 minSize;
			// Resize to size/2 when below this size.
	uio_uint32 maxSize;
			// Resize to size*2 when above this size.
	uio_uint32 size;
			// The number of buckets in the hash table.
	uio_uint32 hashMask;
			// Mask to take on a the calculated hash value, to make it
			// fit into the table.

	HASHTABLE_(HashEntry) **entries;
			// The actual entries

	uio_uint32 numEntries;
#ifdef HashTable_PROFILE
	uio_uint32 numCollisions;
#endif
};

struct HASHTABLE_(HashEntry) {
	uio_uint32 hash;
	HASHTABLE_(Key) *key;
	HASHTABLE_(Value) *value;
	HASHTABLE_(HashEntry) *next;
};

struct HASHTABLE_(Iterator) {
	const HASHTABLE_(HashTable) *hashTable;
	uio_uint32 bucketNr;
	HASHTABLE_(HashEntry) *entry;
};

HASHTABLE_(HashTable) *HASHTABLE_(newHashTable)(
		HASHTABLE_(HashFunction) hashFunction,
		HASHTABLE_(EqualFunction) equalFunction,
		HASHTABLE_(CopyFunction) copyFunction,
		HASHTABLE_(FreeKeyFunction) freeKeyFunction,
		HASHTABLE_(FreeValueFunction) freeValueFunction,
		uio_uint32 initialSize,
		double minFillQuotient, double maxFillQuotient);
uio_bool HASHTABLE_(add)(HASHTABLE_(HashTable) *hashTable,
		const HASHTABLE_(Key) *key, HASHTABLE_(Value) *value);
uio_bool HASHTABLE_(remove)(HASHTABLE_(HashTable) *hashTable,
		const HASHTABLE_(Key) *key);
HASHTABLE_(Value) *HASHTABLE_(find)(
		HASHTABLE_(HashTable) *hashTable, const HASHTABLE_(Key) *key);
uio_uint32 HASHTABLE_(count)(const HASHTABLE_(HashTable) *hashTable);
void HASHTABLE_(deleteHashTable)(HASHTABLE_(HashTable) *hashTable);
HASHTABLE_(Iterator) *HASHTABLE_(getIterator)(
		const HASHTABLE_(HashTable) *hashTable);
int HASHTABLE_(iteratorDone)(const HASHTABLE_(Iterator) *iterator);
HASHTABLE_(Key) *HASHTABLE_(iteratorKey)(HASHTABLE_(Iterator) *iterator);
HASHTABLE_(Value) *HASHTABLE_(iteratorValue)(HASHTABLE_(Iterator) *iterator);
HASHTABLE_(Iterator) *HASHTABLE_(iteratorNext)(HASHTABLE_(Iterator) *iterator);
void HASHTABLE_(freeIterator)(HASHTABLE_(Iterator) *iterator);

#ifndef HASHTABLE_INTERNAL
#	undef HASHTABLE_
#endif

#endif /* !defined(_HASHTABLE_H) || defined(HASHTABLE_GENERIC) */



