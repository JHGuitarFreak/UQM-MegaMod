#ifndef UQM_GRPINTRN_H_
#define UQM_GRPINTRN_H_

// For IPGROUP
#include "grpinfo.h"

// For SHIP_FRAGMENT
#include "races.h"

// For GAME_STATE_FILE
#include "state.h"

//#define DEBUG_GROUPS

// A group header describes battle groups present in a star system. There is
//    at most 1 group header per system.
// 'Random' group info file (RANDGRPINFO_FILE) always contains only one
//    group header record, which describes the last-visited star system,
//    (which may be the current system). Thus the randomly generated groups
//    are valid for 7 days (set in PutGroupInfo) after the player leaves 
//    the system, or until the player enters another star system.
typedef struct
{
	BYTE NumGroups;
	BYTE day_index, month_index;
	COUNT star_index, year_index;
			// day_index, month_index, year_index specify when
			//   random groups expire (if you were to leave the system
			//   by going to HSpace and stay there till such time)
			// star_index is the index of a star this group header
			//   applies to; ~0 means uninited
	DWORD GroupOffset[NUM_SAVED_BATTLE_GROUPS + 1];
			// Absolute offsets of group definitions in a state file
			// Group 0 is a list of groups present in solarsys
			//    (RANDGRPINFO_FILE only)
			// Groups 1..max are definitions of actual battle groups
			//    containing ship makeup and status

	// Each group has the following format:
	// 1 byte, RaceType (LastEncGroup in Group 0)
	// 1 byte, NumShips (NumGroups in Group 0)
	// Ships follow:
	// 1 byte, RaceType
	// 16 bytes, part of SHIP_FRAGMENT struct 
        //                  (part of IP_GROUP struct in Group 0)

} GROUP_HEADER;

void ReadGroupHeader (GAME_STATE_FILE *fp, GROUP_HEADER *pGH);
void WriteGroupHeader (GAME_STATE_FILE *fp, const GROUP_HEADER *pGH);
void ReadShipFragment (GAME_STATE_FILE *fp, SHIP_FRAGMENT *FragPtr);
void WriteShipFragment (GAME_STATE_FILE *fp, const SHIP_FRAGMENT *FragPtr);
void ReadIpGroup (GAME_STATE_FILE *fp, IP_GROUP *GroupPtr);
void WriteIpGroup (GAME_STATE_FILE *fp, const IP_GROUP *GroupPtr);

#endif
