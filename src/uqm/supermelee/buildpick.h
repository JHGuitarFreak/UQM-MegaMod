#ifndef BUILDPICK_H
#define BUILDPICK_H

#include "types.h"
#include "melee.h"

#if defined(__cplusplus)
extern "C" {
#endif

void BuildBuildPickFrame (void);
void DestroyBuildPickFrame (void);
bool BuildPickShip (MELEE_STATE *pMS);
void GetBuildPickFrameRect (RECT *r);

void DrawPickFrame (MELEE_STATE *pMS);
void DrawPickIcon (MeleeShip ship, bool DrawErase);


#if defined(__cplusplus)
}
#endif

#endif  /* BUILDPICK_H */

