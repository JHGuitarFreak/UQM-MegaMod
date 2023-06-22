#ifndef BUILDPICK_H
#define BUILDPICK_H

#include "types.h"
#include "melee.h"
#include "../races.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define RACE_NAME_OFFSET 0
#define RACE_SHIP_OFFSET 3
#define RACE_DESC_OFFSET 5

void BuildBuildPickFrame (void);
void DestroyBuildPickFrame (void);
bool BuildPickShip (MELEE_STATE *pMS);
void GetBuildPickFrameRect (RECT *r);
void GetTooltipBoxRect (RECT *r);

void DrawPickFrame (MELEE_STATE *pMS);
void DrawPickIcon (MeleeShip ship, bool DrawErase);

void DrawTooltip (SHIP_INFO *SIPtr);


#if defined(__cplusplus)
}
#endif

#endif  /* BUILDPICK_H */

