/*
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
#include "stringhashtable.h"
#include "types.h"
#include "libs/misc.h"
		// For unconst()
#include "libs/uio/uioport.h"

static inline uio_uint32 StringHashTable_hash(
		StringHashTable_HashTable *hashTable, const char *string);
static inline uio_bool StringHashTable_equal(
		StringHashTable_HashTable *hashTable,
		const char *key1, const char *key2);
static inline char *StringHashTable_copy(
		StringHashTable_HashTable *hashTable, const char *key);

#include "libs/uio/hashtable.c"


static inline uio_uint32
StringHashTable_hash(StringHashTable_HashTable *hashTable, const char *key) {
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
StringHashTable_equal(StringHashTable_HashTable *hashTable,
		const char *key1, const char *key2) {
	(void) hashTable;
	return strcmp(key1, key2) == 0;
}

static inline char *
StringHashTable_copy(StringHashTable_HashTable *hashTable,
		const char *key) {
	(void) hashTable;
	return unconst(key);
}

