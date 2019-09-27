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

#ifdef NETPLAY

#include "checksum.h"
#include "netoptions.h"

#ifdef NETPLAY_CHECKSUM

#include "checkbuf.h"
#include "crc.h"
		// for DUMP_CRC_OPS
#include "netconnection.h"
#include "netmelee.h"
#include "libs/log.h"
#include "libs/mathlib.h"

ChecksumBuffer localChecksumBuffer;

void
crc_processEXTENT(crc_State *state, const EXTENT *val) {
#ifdef DUMP_CRC_OPS
	crc_log("START crc_processEXTENT().");
#endif
	crc_processCOORD(state, val->width);
	crc_processCOORD(state, val->height);
#ifdef DUMP_CRC_OPS
	crc_log("END   crc_processEXTENT().");
#endif
}

void
crc_processVELOCITY_DESC(crc_State *state, const VELOCITY_DESC *val) {
#ifdef DUMP_CRC_OPS
	crc_log("START crc_processVELOCITY_DESC().");
#endif
	crc_processCOUNT(state, val->TravelAngle);
	crc_processEXTENT(state, &val->vector);
	crc_processEXTENT(state, &val->fract);
	crc_processEXTENT(state, &val->error);
	crc_processEXTENT(state, &val->incr);
#ifdef DUMP_CRC_OPS
	crc_log("END   crc_processVELOCITY_DESC().");
#endif
}

void
crc_processPOINT(crc_State *state, const POINT *val) {
#ifdef DUMP_CRC_OPS
	crc_log("START crc_processPOINT().");
#endif
	crc_processCOORD(state, val->x);
	crc_processCOORD(state, val->y);
#ifdef DUMP_CRC_OPS
	crc_log("END   crc_processPOINT().");
#endif
}

#if 0
void
crc_processSTAMP(crc_State *state, const STAMP *val) {
#ifdef DUMP_CRC_OPS
	crc_log("START crc_processSTAMP().");
#endif
	crc_processPOINT(state, val->origin);
	crc_processFRAME(state, val->frame);
#ifdef DUMP_CRC_OPS
	crc_log("END   crc_processSTAMP().");
#endif
}

void
crc_processINTERSECT_CONTROL(crc_State *state, const INTERSECT_CONTROL *val) {
#ifdef DUMP_CRC_OPS
	crc_log("START crc_processINTERSECT_CONTROL().");
#endif
	crc_processTIME_VALUE(state, val->last_time_val);
	crc_processPOINT(state, &val->EndPoint);
#ifdef DUMP_CRC_OPS
	crc_log("END   crc_processINTERSECT_CONTROL().");
#endif
}
#endif

void
crc_processSTATE(crc_State *state, const STATE *val) {
	crc_processPOINT(state, &val->location);
}

void
crc_processELEMENT(crc_State *state, const ELEMENT *val) {
#ifdef DUMP_CRC_OPS
	crc_log("START crc_processELEMENT().");
#endif
	if (val->state_flags & BACKGROUND_OBJECT) {
		// The element never influences the state of other elements,
		// and is to be excluded from checksums.
#ifdef DUMP_CRC_OPS
		crc_log("      BACKGROUND_OBJECT element omited");
#endif
	} else {
		crc_processELEMENT_FLAGS(state, val->state_flags);
		crc_processCOUNT(state, val->life_span);
		crc_processCOUNT(state, val->crew_level);
		crc_processBYTE(state, val->mass_points);
		crc_processBYTE(state, val->turn_wait);
		crc_processBYTE(state, val->thrust_wait);
		crc_processVELOCITY_DESC(state, &val->velocity);
		crc_processSTATE(state, &val->current);
		crc_processSTATE(state, &val->next);
	}
#ifdef DUMP_CRC_OPS
	crc_log("END   crc_processELEMENT().");
#endif
}

void
crc_processDispQueue(crc_State *state) {
	HELEMENT element;
	HELEMENT nextElement;

#ifdef DUMP_CRC_OPS
	size_t i = 0;
	crc_log("START crc_processDispQueue().");
#endif
	for (element = GetHeadElement(); element != 0; element = nextElement) {
		ELEMENT *elementPtr;

#ifdef DUMP_CRC_OPS
		crc_log("===== disp_q[%d]:", i);
#endif
		LockElement(element, &elementPtr);

		crc_processELEMENT(state, elementPtr);

		nextElement = GetSuccElement(elementPtr);
		UnlockElement(element);
#ifdef DUMP_CRC_OPS
		i++;
#endif
	}
#ifdef DUMP_CRC_OPS
	crc_log("END   crc_processDispQueue().");
#endif
}

void
crc_processRNG(crc_State *state) {
	DWORD seed;

#ifdef DUMP_CRC_OPS
	crc_log("START crc_processRNG().");
#endif

	seed = TFB_SeedRandom(0);
			// This modifies the seed too.
	crc_processDWORD(state, seed);
	TFB_SeedRandom(seed);
			// Restore the old seed.

#ifdef DUMP_CRC_OPS
	crc_log("END   crc_processRNG().");
#endif
}

void
crc_processState(crc_State *state) {
#ifdef DUMP_CRC_OPS
	crc_log("--------------------\n"
			"START crc_processState() (frame %u).", battleFrameCount);
#endif

	crc_processRNG(state);
	crc_processDispQueue(state);

#ifdef DUMP_CRC_OPS
	crc_log("END   crc_processState() (frame %u).",
			battleFrameCount);
#endif
}

void
initChecksumBuffers(void) {
	size_t player;

	for (player = 0; player < NETPLAY_NUM_PLAYERS; player++)
	{
		NetConnection *conn;
		ChecksumBuffer *cb;

		conn = netConnections[player];
		if (conn == NULL)
			continue;

		cb = NetConnection_getChecksumBuffer(conn);
		ChecksumBuffer_init(cb, getBattleInputDelay(),
				NETPLAY_CHECKSUM_INTERVAL);
	}

	ChecksumBuffer_init(&localChecksumBuffer, getBattleInputDelay(),
			NETPLAY_CHECKSUM_INTERVAL);
}

void
uninitChecksumBuffers(void)
{
	size_t player;

	for (player = 0; player < NETPLAY_NUM_PLAYERS; player++)
	{
		NetConnection *conn;
		ChecksumBuffer *cb;

		conn = netConnections[player];
		if (conn == NULL)
			continue;

		cb = NetConnection_getChecksumBuffer(conn);

		ChecksumBuffer_uninit(cb);
	}
	
	ChecksumBuffer_uninit(&localChecksumBuffer);
}

void
addLocalChecksum(BattleFrameCounter frameNr, Checksum checksum) {
	assert(frameNr == battleFrameCount);

	ChecksumBuffer_addChecksum(&localChecksumBuffer, frameNr, checksum);
}

void
addRemoteChecksum(NetConnection *conn, BattleFrameCounter frameNr,
		Checksum checksum) {
	ChecksumBuffer *cb;
	
	assert(frameNr <= battleFrameCount + getBattleInputDelay() + 1);
	assert(frameNr + getBattleInputDelay() >= battleFrameCount);

	cb = NetConnection_getChecksumBuffer(conn);
	ChecksumBuffer_addChecksum(cb, frameNr, checksum);
}

bool
verifyChecksums(BattleFrameCounter frameNr) {
	Checksum localChecksum;
	size_t player;

	if (!ChecksumBuffer_getChecksum(&localChecksumBuffer, frameNr,
			&localChecksum)) {
		// Right now, we require that a checksum is present.
		// If/when we move to UDP, and packets may get lost, we may prefer
		// not to do any checks in this case.
		return false;
	}

	for (player = 0; player < NETPLAY_NUM_PLAYERS; player++)
	{
		NetConnection *conn;
		ChecksumBuffer *cb;
		Checksum remoteChecksum;

		conn = netConnections[player];
		if (conn == NULL)
			continue;

		cb = NetConnection_getChecksumBuffer(conn);
		
		if (!ChecksumBuffer_getChecksum(cb, frameNr, &remoteChecksum))
			return false;

		if (localChecksum != remoteChecksum) {
			log_add(log_Error, "Network connections have gone out of "
					"sync.\n");
			return false;
		}
	}
	return true;
}


#endif  /* NETPLAY_CHECKSUM */

#endif  /* NETPLAY */

