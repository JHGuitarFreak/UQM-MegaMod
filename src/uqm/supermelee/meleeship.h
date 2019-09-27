#ifndef MELEESHIP_H
#define MELEESHIP_H

#include "types.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum MeleeShip {
	MELEE_ANDROSYNTH,
	MELEE_ARILOU,
	MELEE_CHENJESU,
	MELEE_CHMMR,
	MELEE_DRUUGE,
	MELEE_EARTHLING,
	MELEE_ILWRATH,
	MELEE_KOHR_AH,
	MELEE_MELNORME,
	MELEE_MMRNMHRM,
	MELEE_MYCON,
	MELEE_ORZ,
	MELEE_PKUNK,
	MELEE_SHOFIXTI,
	MELEE_SLYLANDRO,
	MELEE_SPATHI,
	MELEE_SUPOX,
	MELEE_SYREEN,
	MELEE_THRADDASH,
	MELEE_UMGAH,
	MELEE_URQUAN,
	MELEE_UTWIG,
	MELEE_VUX,
	MELEE_YEHAT,
	MELEE_ZOQFOTPIK,
	
	MELEE_UNSET = ((BYTE) ~0) - 1,
			// Used with the Update protocol, to register in the sentTeam
	MELEE_NONE = (BYTE) ~0
			// Empty fleet position.
} MeleeShip;
#define NUM_MELEE_SHIPS (MELEE_ZOQFOTPIK + 1)

static inline bool
MeleeShip_valid (MeleeShip ship)
{
	return (ship < NUM_MELEE_SHIPS) || (ship == MELEE_NONE);
}

#if defined(__cplusplus)
}
#endif

#endif  /* MELEESHIP_H */

