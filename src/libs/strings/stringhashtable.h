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

#ifndef LIBS_STRINGS_STRINGHASHTABLE_H_
#define LIBS_STRINGS_STRINGHASHTABLE_H_

// HashTable from 'char *' to STRING.
// We don't actually copy the index, which means that the caller is
// responsible for keeping them unchanged during the time that it is used in
// the hash table.

#include "libs/strlib.h"

#define HASHTABLE_(identifier) StringHashTable ## _ ## identifier
typedef char HASHTABLE_(Key);
typedef STRING_TABLE_ENTRY_DESC HASHTABLE_(Value);
#define StringHashTable_HASH StringHashTable_hash
#define StringHashTable_EQUAL StringHashTable_equal
#define StringHashTable_COPY StringHashTable_copy
#define StringHashTable_FREEKEY(hashTable, key) \
		((void) (hashTable), (void) (key))
#define StringHashTable_FREEVALUE(hashTable, value) \
		((void) (hashTable), (void) (value))

#include "libs/uio/hashtable.h"


#endif  /* LIBS_STRINGS_STRINGHASHTABLE_H_ */
