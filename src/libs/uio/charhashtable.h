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

#ifndef LIBS_UIO_CHARHASHTABLE_H_
#define LIBS_UIO_CHARHASHTABLE_H_


#define HASHTABLE_(identifier) CharHashTable ## _ ## identifier
typedef char HASHTABLE_(Key);
typedef void HASHTABLE_(Value);
#define CharHashTable_HASH CharHashTable_hash
#define CharHashTable_EQUAL CharHashTable_equal
#define CharHashTable_COPY CharHashTable_copy
#define CharHashTable_FREEKEY CharHashTable_freeKey
#define CharHashTable_FREEVALUE(hashTable, value) \
		((void) (hashTable), (void) (value))

#include "hashtable.h"


#endif  /* LIBS_UIO_CHARHASHTABLE_H_ */

