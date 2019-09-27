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

#ifndef UQM_SUPERMELEE_NETPLAY_CHECKSUM_H_
#define UQM_SUPERMELEE_NETPLAY_CHECKSUM_H_


#include "types.h"

typedef uint32 Checksum;


#include "netplay.h"
#include "crc.h"

#include "../../element.h"
#include "libs/gfxlib.h"

#include "netconnection.h"

#if defined(__cplusplus)
extern "C" {
#endif


static inline void
crc_processELEMENT_FLAGS(crc_State *state, ELEMENT_FLAGS val) {
	crc_processUint16(state, (uint16) val);
}

static inline void
crc_processCOUNT(crc_State *state, COUNT val) {
	crc_processUint16(state, (uint16) val);
}

static inline void
crc_processBYTE(crc_State *state, BYTE val) {
	crc_processUint8(state, (uint8) val);
}

static inline void
crc_processDWORD(crc_State *state, DWORD val) {
	crc_processUint32(state, (uint32) val);
}

static inline void
crc_processCOORD(crc_State *state, COORD val) {
	crc_processUint16(state, (uint16) val);
}

#if 0
static inline void
crc_processTIME_VALUE(crc_State *state, const TIME_VALUE val) {
	crc_processUint16(state, (uint16) val);
}
#endif

void crc_processEXTENT(crc_State *state, const EXTENT *val);
void crc_processVELOCITY_DESC(crc_State *state, const VELOCITY_DESC *val);
void crc_processPOINT(crc_State *state, const POINT *val);
#if 0
void crc_processSTAMP(crc_State *state, const STAMP *val);
void crc_processINTERSECT_CONTROL(crc_State *state,
		const INTERSECT_CONTROL *val);
#endif
void crc_processSTATE(crc_State *state, const STATE *val);
void crc_processELEMENT(crc_State *state, const ELEMENT *val);
void crc_processDispQueue(crc_State *state);
void crc_processRNG(crc_State *state);
void crc_processState(crc_State *state);


void initChecksumBuffers(void);
void uninitChecksumBuffers(void);
void addLocalChecksum(BattleFrameCounter frameNr, Checksum checksum);
void addRemoteChecksum(NetConnection *conn, BattleFrameCounter frameNr,
		Checksum checksum);
bool verifyChecksums(BattleFrameCounter frameNr);

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_SUPERMELEE_NETPLAY_CHECKSUM_H_ */


