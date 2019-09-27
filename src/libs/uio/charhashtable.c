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


#define HASHTABLE_INTERNAL
#include "charhashtable.h"
#include "types.h"
#include "uioport.h"

static inline uio_uint32 CharHashTable_hash(CharHashTable_HashTable *hashTable,
		const char *string);
static inline uio_bool CharHashTable_equal(CharHashTable_HashTable *hashTable,
		const char *key1, const char *key2);
static inline char *CharHashTable_copy(CharHashTable_HashTable *hashTable,
		const char *key);
static inline void CharHashTable_freeKey(CharHashTable_HashTable *hashTable,
		char *key);

#include "hashtable.c"


static inline uio_uint32
CharHashTable_hash(CharHashTable_HashTable *hashTable, const char *key) {
	uio_uint32 hash;

	(void) hashTable;
	// Rotating hash, variation of something on the web which
	// wasn't original itself.
	hash = 0;
			// Hash was on that web page initialised as the length,
			// but that isn't known at this time.
	while (*key != '\0') {
		hash = (hash << 4) ^ (hash >> 28) ^ *key;
		key++;
	}
	return hash ^ (hash >> 10) ^ (hash >> 20);
}

static inline uio_bool
CharHashTable_equal(CharHashTable_HashTable *hashTable,
		const char *key1, const char *key2) {
	(void) hashTable;
	return strcmp(key1, key2) == 0;
}

static inline char *
CharHashTable_copy(CharHashTable_HashTable *hashTable,
		const char *key) {
	(void) hashTable;
	return uio_strdup(key);
}

static inline void
CharHashTable_freeKey(CharHashTable_HashTable *hashTable,
		char *key) {
	(void) hashTable;
	uio_free(key);
}


