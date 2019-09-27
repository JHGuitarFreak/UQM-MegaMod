//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "lander.h"

#include "lifeform.h"
#include "scan.h"
#include "../cons_res.h"
#include "../controls.h"
#include "../colors.h"
#include "../process.h"
#include "../units.h"
#include "../gamestr.h"
#include "../nameref.h"
#include "../resinst.h"
#include "../setup.h"
#include "../sounds.h"
#include "../element.h"
#include "libs/graphics/gfx_common.h"
#include "libs/mathlib.h"
#include "libs/log.h"
#include "options.h"
#include "uqm/menustat.h"

//define SPIN_ON_LAUNCH to let the planet spin while
// the lander animation is playing
#define SPIN_ON_LAUNCH

// PLANET_SIDE_RATE governs how fast the lander,
// bio and planet effects will be
// We're using the 3DO speed, which is 35 FPS
// The PC speed was 30 FPS.
// Remember that all values need to evenly divide
// ONE_SECOND.
#define PLANET_SIDE_RATE (ONE_SECOND / 35)


// This is a derived type from INPUT_STATE_DESC.
typedef struct LanderInputState LanderInputState;
struct LanderInputState {
	// Fields required by DoInput()
	BOOLEAN (*InputFunc) (LanderInputState *pMS);

	BOOLEAN Initialized;
	TimeCount NextTime;
			// Frame rate control
};

FRAME LanderFrame[8];
static SOUND LanderSounds;
MUSIC_REF LanderMusic;

LIFEFORM_DESC CreatureData[] =
{
	{SPEED_MOTIONLESS | DANGER_HARMLESS, MAKE_BYTE (1, 1)},
			// Roto-Dendron
	{SPEED_MOTIONLESS | DANGER_HARMLESS, MAKE_BYTE (6, 1)},
			// Macrocillia
	{SPEED_MOTIONLESS | DANGER_WEAK, MAKE_BYTE (3, 1)},
			// Splort Wort
	{SPEED_MOTIONLESS | DANGER_NORMAL, MAKE_BYTE (5, 3)},
			// Whackin' Bush
	{SPEED_MOTIONLESS | DANGER_HARMLESS, MAKE_BYTE (2, 10)},
			// Slot Machine Tree
	{BEHAVIOR_UNPREDICTABLE | SPEED_SLOW | DANGER_HARMLESS, MAKE_BYTE (1, 2)},
			// Neon Worm
	{BEHAVIOR_FLEE | AWARENESS_MEDIUM | SPEED_SLOW | DANGER_HARMLESS, MAKE_BYTE (8, 5)},
			// Stiletto Urchin
	{BEHAVIOR_HUNT | AWARENESS_LOW | SPEED_SLOW | DANGER_WEAK, MAKE_BYTE (2, 2)},
			// Deluxe Blob
	{BEHAVIOR_UNPREDICTABLE | SPEED_SLOW | DANGER_NORMAL, MAKE_BYTE (3, 8)},
			// Glowing Medusa
	{BEHAVIOR_HUNT | AWARENESS_MEDIUM | SPEED_SLOW | DANGER_MONSTROUS, MAKE_BYTE (10, 15)},
			// Carousel Beast
	{BEHAVIOR_HUNT | AWARENESS_MEDIUM | SPEED_MEDIUM | DANGER_WEAK, MAKE_BYTE (3, 3)},
			// Mysterious Bees
	{BEHAVIOR_FLEE | AWARENESS_MEDIUM | SPEED_MEDIUM | DANGER_HARMLESS, MAKE_BYTE (2, 1)},
			// Hopping Blobby
	{BEHAVIOR_UNPREDICTABLE | SPEED_MEDIUM | DANGER_WEAK, MAKE_BYTE (2, 2)},
			// Blood Monkey
	{BEHAVIOR_HUNT | AWARENESS_HIGH | SPEED_MEDIUM | DANGER_NORMAL, MAKE_BYTE (4, 6)},
			// Yompin Yiminy
	{BEHAVIOR_UNPREDICTABLE | SPEED_MEDIUM | DANGER_MONSTROUS, MAKE_BYTE (9, 12)},
			// Amorphous Trandicula
	{BEHAVIOR_HUNT | AWARENESS_HIGH | SPEED_FAST | DANGER_WEAK, MAKE_BYTE (3, 1)},
			// Crazy Weasel
	{BEHAVIOR_FLEE | AWARENESS_HIGH | SPEED_FAST | DANGER_HARMLESS, MAKE_BYTE (1, 1)},
			// Merry Whumpet
	{BEHAVIOR_HUNT | AWARENESS_LOW | SPEED_FAST | DANGER_NORMAL, MAKE_BYTE (7, 8)},
			// Fungal Squid
	{BEHAVIOR_FLEE | AWARENESS_HIGH | SPEED_FAST | DANGER_WEAK, MAKE_BYTE (15, 2)},
			// Penguin Cyclops
	{BEHAVIOR_FLEE | AWARENESS_LOW | SPEED_FAST | DANGER_WEAK, MAKE_BYTE (1, 1)},
			// Chicken
	{BEHAVIOR_UNPREDICTABLE | SPEED_SLOW | DANGER_WEAK, MAKE_BYTE (6, 2)},
			// Bubble Vine
	{BEHAVIOR_FLEE | AWARENESS_HIGH | SPEED_SLOW | DANGER_WEAK, MAKE_BYTE (4, 2)},
			// Bug-Eyed Bait
	{SPEED_MOTIONLESS | DANGER_WEAK, MAKE_BYTE (8, 5)},
			// Goo Burger
	{SPEED_MOTIONLESS | DANGER_MONSTROUS, MAKE_BYTE (1, 1)},
			// Evil One
	{BEHAVIOR_UNPREDICTABLE | SPEED_SLOW | DANGER_HARMLESS, MAKE_BYTE (1, 1)}, // ? was 0, 1
			// Brainbox Bulldozers
	{BEHAVIOR_HUNT | AWARENESS_HIGH | SPEED_FAST | DANGER_MONSTROUS, MAKE_BYTE (15, 15)},
			// Zex's Beauty
};


extern PRIM_LINKS DisplayLinks;

#define DAMAGE_CYCLE 6
// XXX: There are actually only 9 explosion images.
//   The last frame is drawn twice.
#define EXPLOSION_LIFE 10
// How long to wait after the lander explodes, so that the full
// gravity of the player's situation sinks in
#define EXPLOSION_WAIT         (ONE_SECOND * 2)
#define EXPLOSION_WAIT_FRAMES  (EXPLOSION_WAIT / PLANET_SIDE_RATE)
// The actual number of frame that the explosion and wait takes is:
//   EXPLOSION_LIFE * 3 + EXPLOSION_WAIT_FRAMES

#define DEATH_EXPLOSION 0

// TODO: redefine these in terms of CONTEXT width/height
#define SURFACE_WIDTH SIS_SCREEN_WIDTH
#define SURFACE_HEIGHT (SIS_SCREEN_HEIGHT - MAP_HEIGHT - MAP_BORDER_HEIGHT)

#define REPAIR_LANDER (1 << 7)
#define REPAIR_TRANSITION (1 << 6)
#define KILL_CREW (1 << 5)
#define ADD_AT_END (1 << 4)
#define REPAIR_COUNT (0xf)

#define LANDER_SPEED_DENOM (10) // JMS_GFX

static BYTE lander_flags;
static POINT curLanderLoc;
static int crew_left;
static int shieldHit;
		// which shield was hit, assuming it helped
static int damage_index;
		// number of lander damage frames left
static int explosion_index;
		// lander explosion progression. Semantics are similar to an
		// inverse of ELEMENT.life_span
static int turn_wait;
		// thus named for similar semantics to ELEMENT.turn_wait
static int weapon_wait;
		// semantics similar to STARSHIP.weapon_counter

// TODO: We may want to make the PLANETSIDE_DESC fields into static vars
static PLANETSIDE_DESC *planetSideDesc;

#define ON_THE_GROUND   0


static Color
DamageColorCycle (Color c, COUNT i)
{
	static const Color damage_tab[DAMAGE_CYCLE + 1] =
	{
		WHITE_COLOR_INIT,
		BUILD_COLOR (MAKE_RGB15_INIT (0x1B, 0x00, 0x00), 0x2A),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x07, 0x00), 0x7E),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0E, 0x00), 0x7C),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x15, 0x00), 0x7A),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x1C, 0x00), 0x78),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x1F, 0x0A), 0x0E),
	};

	if (i)
		c = damage_tab[i];
	else if (sameColor(c, WHITE_COLOR))
		c = damage_tab[6];
	else if (sameColor(c, BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E)))
		c = damage_tab[5];
	else if (sameColor(c, BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1C, 0x00), 0x78)))
		c = damage_tab[4];
	else if (sameColor(c, BUILD_COLOR (MAKE_RGB15 (0x1F, 0x15, 0x00), 0x7A)))
		c = damage_tab[3];
	else if (sameColor(c, BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0E, 0x00), 0x7C)))
		c = damage_tab[2];
	else if (sameColor(c, BUILD_COLOR (MAKE_RGB15 (0x1F, 0x07, 0x00), 0x7E)))
		c = damage_tab[1];
	else
		c = damage_tab[0];

	return c;
}

static HELEMENT AddGroundDisaster (COUNT which_disaster);

void
object_animation (ELEMENT *ElementPtr)
{
	COUNT frame_index, angle;
	PRIMITIVE *pPrim;

	pPrim = &DisplayArray[ElementPtr->PrimIndex];
	if (GetPrimType (pPrim) == STAMPFILL_PRIM
			&& !((ElementPtr->state_flags & FINITE_LIFE)
			&& ElementPtr->mass_points == EARTHQUAKE_DISASTER))
	{
		Color c;

		c = DamageColorCycle (GetPrimColor (pPrim), 0);
		if (sameColor(c, WHITE_COLOR))
		{
			SetPrimType (pPrim, STAMP_PRIM);
			if (ElementPtr->hit_points == 0)
			{
				ZeroVelocityComponents (&ElementPtr->velocity);
				pPrim->Object.Stamp.frame =
						SetAbsFrameIndex (pPrim->Object.Stamp.frame, 0);

				PlaySound (SetAbsSoundIndex (LanderSounds, LIFEFORM_CANNED),
						NotPositional (), NULL, GAME_SOUND_PRIORITY);
			}
		}

		SetPrimColor (pPrim, c);
	}

	frame_index = GetFrameIndex (pPrim->Object.Stamp.frame) + 1;
	if (LONIBBLE (ElementPtr->turn_wait))
		--ElementPtr->turn_wait;
	else
	{
		ElementPtr->turn_wait += HINIBBLE (ElementPtr->turn_wait);

		pPrim->Object.Stamp.frame = IncFrameIndex (pPrim->Object.Stamp.frame);
		
		if (ElementPtr->state_flags & FINITE_LIFE)
		{
			/* A natural disaster */
			if (ElementPtr->mass_points == DEATH_EXPLOSION)
			{	// Lander explosion
				++explosion_index;
				if (explosion_index >= EXPLOSION_LIFE)
				{	// XXX: The last frame is drawn twice
					pPrim->Object.Stamp.frame =
							DecFrameIndex (pPrim->Object.Stamp.frame);
				}
			}
			else if (ElementPtr->mass_points == EARTHQUAKE_DISASTER)
			{
				SIZE s;
				SIZE frame_amount = 14; // JMS_GFX

				if (frame_index >= (frame_amount-1))
					s = 0;
				else
					s = (frame_amount - frame_index) >> 1;
				// XXX: Was 0x8000 the background flag on 3DO?
				//SetPrimColor (pPrim, BUILD_COLOR (0x8000 | MAKE_RGB15 (0x1F, 0x1F, 0x1F), s));
				SetPrimColor (pPrim, BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), s));
				if (frame_index == (frame_amount - 1))
					PlaySound (SetAbsSoundIndex (LanderSounds, EARTHQUAKE_DISASTER),
							NotPositional (), NULL, GAME_SOUND_PRIORITY);
			}
			
			if (ElementPtr->mass_points == LAVASPOT_DISASTER
					&& frame_index == 5
					&& TFB_Random () % 100 < 90)
			{
				HELEMENT hLavaElement;

				/* Change lava-spot direction of travel */
				hLavaElement = AddGroundDisaster (LAVASPOT_DISASTER);
				if (hLavaElement)
				{
					ELEMENT *LavaElementPtr;

					angle = FACING_TO_ANGLE (ElementPtr->facing);
					LockElement (hLavaElement, &LavaElementPtr);
					LavaElementPtr->next.location = ElementPtr->next.location;
					LavaElementPtr->next.location.x += COSINE (angle, RES_SCALE(4)); // JMS_GFX
					LavaElementPtr->next.location.y += SINE (angle, RES_SCALE(4)); // JMS_GFX
					if (LavaElementPtr->next.location.y < 0)
						LavaElementPtr->next.location.y = 0;
					else if (LavaElementPtr->next.location.y >= (MAP_HEIGHT << MAG_SHIFT))
						LavaElementPtr->next.location.y = (MAP_HEIGHT << MAG_SHIFT) - 1;
					if (LavaElementPtr->next.location.x < 0)
						LavaElementPtr->next.location.x += MAP_WIDTH << MAG_SHIFT;
					else
						LavaElementPtr->next.location.x %= MAP_WIDTH << MAG_SHIFT;
					LavaElementPtr->facing = NORMALIZE_FACING (
							ElementPtr->facing + (TFB_Random () % 3 - 1));
					UnlockElement (hLavaElement);
				}
			}
		}
		else if (!(frame_index & 3) && ElementPtr->hit_points)
		{
			BYTE index;
			COUNT speed;

			index = ElementPtr->mass_points & ~CREATURE_AWARE;
			speed = CreatureData[index].Attributes & SPEED_MASK;
			if (speed)
			{
				SIZE dx, dy;
				COUNT old_angle;

				dx = curLanderLoc.x - ElementPtr->next.location.x;
				if (dx < 0 && dx < -(MAP_WIDTH << (MAG_SHIFT - 1)))
					dx += MAP_WIDTH << MAG_SHIFT;
				else if (dx > (MAP_WIDTH << (MAG_SHIFT - 1)))
					dx -= MAP_WIDTH << MAG_SHIFT;
				dy = curLanderLoc.y - ElementPtr->next.location.y;
				angle = ARCTAN (dx, dy);
				if (dx < 0)
					dx = -dx;
				if (dy < 0)
					dy = -dy;

				if (dx >= SURFACE_WIDTH || dy >= SURFACE_WIDTH
						|| dx * dx + dy * dy >= SURFACE_WIDTH * SURFACE_WIDTH)
					ElementPtr->mass_points &= ~CREATURE_AWARE;
				else if (!(ElementPtr->mass_points & CREATURE_AWARE))
				{
					BYTE DetectPercent;

					DetectPercent = (((BYTE)(CreatureData[index].Attributes
							& AWARENESS_MASK) >> AWARENESS_SHIFT) + 1)
							* (30 / 6);
							// XXX: Shouldn't this be dependent on
							// PLANET_SIDE_RATE somehow? And why is it
							// written as '30 / 6' instead of 5? Does the 30
							// specify the (PC) framerate? That doesn't make
							// sense; I would expect it to be in the
							// denominator. And even then, it wouldn't give
							// the same results with different frame rates,
							// as repeating 'random(x / 30)' 30 times doesn't
							// generally have the same result as repeating
							// 'random(x / 35)' 25 times. - SvdB
					if (TFB_Random () % 100 < DetectPercent)
					{
						ElementPtr->thrust_wait = 0;
						ElementPtr->mass_points |= CREATURE_AWARE;
					}
				}

				if (ElementPtr->next.location.y == 0
						|| ElementPtr->next.location.y ==
						(MAP_HEIGHT << MAG_SHIFT) - 1)
					ElementPtr->thrust_wait = 0;

				old_angle = GetVelocityTravelAngle (&ElementPtr->velocity);
				if (ElementPtr->thrust_wait)
				{
					--ElementPtr->thrust_wait;
					angle = old_angle;
				}
				else if (!(ElementPtr->mass_points & CREATURE_AWARE)
						|| (CreatureData[index].Attributes
						& BEHAVIOR_MASK) == BEHAVIOR_UNPREDICTABLE)
				{
					COUNT rand_val;

					rand_val = TFB_Random ();
					angle = NORMALIZE_ANGLE (LOBYTE (rand_val));
					ElementPtr->thrust_wait =
							(HIBYTE (rand_val) >> 2) + 10;
				}
				else if ((CreatureData[index].Attributes
						& BEHAVIOR_MASK) == BEHAVIOR_FLEE)
				{
					if (ElementPtr->next.location.y == 0
							|| ElementPtr->next.location.y ==
							(MAP_HEIGHT << MAG_SHIFT) - 1)
					{
						if (angle & (HALF_CIRCLE - 1))
							angle = HALF_CIRCLE - angle;
						else if (old_angle == QUADRANT
								|| old_angle == (FULL_CIRCLE - QUADRANT))
							angle = old_angle;
						else
							angle = ((TFB_Random () & 1)
										* HALF_CIRCLE) - QUADRANT;
						ElementPtr->thrust_wait = 5;
					}
					angle = NORMALIZE_ANGLE (angle + HALF_CIRCLE);
				}

				switch (speed)
				{
					case SPEED_SLOW:
						speed = WORLD_TO_VELOCITY (2 * 1) >> 2;
						break;
					case SPEED_MEDIUM:
						speed = WORLD_TO_VELOCITY (2 * 1) >> 1;
						break;
					case SPEED_FAST:
						speed = WORLD_TO_VELOCITY (2 * 1) * 9 / 10;
						break;
				}
				speed = RES_SCALE(speed); // JMS_GFX

				SetVelocityComponents (&ElementPtr->velocity,
						COSINE (angle, speed), SINE (angle, speed));
			}
		}
	}

	if ((ElementPtr->state_flags & FINITE_LIFE)
			&& ElementPtr->mass_points == DEATH_EXPLOSION
			&& GetSuccLink (DisplayLinks) != ElementPtr->PrimIndex)
		lander_flags |= ADD_AT_END;
}

#define NUM_CREW_COLS 6
#define NUM_CREW_ROWS 2

static void
DeltaLanderCrew (SIZE crew_delta, COUNT which_disaster)
{
	STAMP s;
	CONTEXT OldContext;

	if (crew_delta > 0)
	{
		// Filling up the crew bar when landing.
		crew_delta = crew_left;
		crew_left += 1;

		s.frame = SetAbsFrameIndex (LanderFrame[0], 55);
	}
	else /* if (crew_delta < 0) */
	{
		if (crew_left < 1)
			return; // irrelevant -- all dead
		
		shieldHit = GET_GAME_STATE (LANDER_SHIELDS);
		shieldHit &= 1 << which_disaster;
		if (!shieldHit || TFB_Random () % 100 >= 95)
		{	// No shield, or it did not help
			if (!optGodMode) {
				shieldHit = 0; 
				--crew_left; 
			} else 
				shieldHit = 1; 
		}

		damage_index = DAMAGE_CYCLE;
		if (shieldHit)
			return;

		crew_delta = crew_left;
		s.frame = SetAbsFrameIndex (LanderFrame[0], 56);

		PlaySound (SetAbsSoundIndex (LanderSounds, LANDER_INJURED),
				NotPositional (), NULL, GAME_SOUND_PRIORITY);
	}

	s.origin.x = RES_STAT_SCALE(11) + (RES_STAT_SCALE(6) * (crew_delta % NUM_CREW_COLS));
	s.origin.y = RES_STAT_SCALE(35) - (RES_STAT_SCALE(6) * (crew_delta / NUM_CREW_COLS));

	OldContext = SetContext (RadarContext);
	DrawStamp (&s);
	SetContext (OldContext);
}

static void
FillLanderHold (PLANETSIDE_DESC *pPSD, COUNT scan, COUNT NumRetrieved)
{
	COUNT start_count, tmpholdint;
	STAMP s;
	CONTEXT OldContext;
	SIZE  rounding_error_startcount = 0; // JMS_GFX
	SIZE  rounding_error_numretrieved = 0; // JMS_GFX

	PlaySound (SetAbsSoundIndex (LanderSounds, LANDER_PICKUP),
			NotPositional (), NULL, GAME_SOUND_PRIORITY);

	if (scan == BIOLOGICAL_SCAN)
	{
		start_count = pPSD->BiologicalLevel;

		s.frame = SetAbsFrameIndex (LanderFrame[0], 41);

		pPSD->BiologicalLevel += NumRetrieved;
	} else {
		start_count = pPSD->ElementLevel;
		pPSD->ElementLevel += NumRetrieved;

		if (GET_GAME_STATE(IMPROVED_LANDER_CARGO)) {
			start_count >>= 1;
			NumRetrieved = (pPSD->ElementLevel >> 1) - start_count;
		}

		s.frame = SetAbsFrameIndex (LanderFrame[0], 43);
	}

	tmpholdint = ((start_count + NumRetrieved) * MAX_HOLD_BARS / MAX_SCROUNGED)
			- ((start_count * MAX_HOLD_BARS / MAX_SCROUNGED) + (NumRetrieved *
			MAX_HOLD_BARS / MAX_SCROUNGED));
	start_count = start_count * MAX_HOLD_BARS / MAX_SCROUNGED;
	NumRetrieved = (NumRetrieved * MAX_HOLD_BARS / MAX_SCROUNGED) + tmpholdint;

	start_count *= RES_STAT_SCALE(1); // JMS_GFX

	s.origin.x = 0;
	s.origin.y = -(int)start_count;
	if (!(start_count & 1))
		s.frame = IncFrameIndex (s.frame);

	OldContext = SetContext (RadarContext);

	while (NumRetrieved--)
	{
		if (start_count++ & 1)
			s.frame = IncFrameIndex (s.frame);
		else
			s.frame = DecFrameIndex (s.frame);
		DrawStamp (&s);
		s.origin.y -= RES_STAT_SCALE(1); // JMS_GFX
	}
	SetContext (OldContext);
}

// returns true iff the node was picked up.
static bool
pickupNode (PLANETSIDE_DESC *pPSD, COUNT NumRetrieved,
		ELEMENT *ElementPtr, const INTERSECT_CONTROL *LanderControl,
		const INTERSECT_CONTROL *ElementControl, COUNT Scan)
{
	BYTE EType;
	UNICODE ch, *pStr;
	COUNT *Amount, Max, Offset;
	BOOLEAN PartialPickup;

	Amount = &pPSD->BiologicalLevel;
	Max = MAX_SCROUNGED;
	EType = ElementPtr->thrust_wait;
	Offset = BIOLOGICAL_STRING_BASE;

	if (Scan != BIOLOGICAL_SCAN){
		Amount = &pPSD->ElementLevel;
		Max = pPSD->MaxElementLevel;
		EType = ElementPtr->turn_wait;
		Offset = ELEMENTS_STRING_BASE;
	}

	// JMS: The rest of partially scavenged minerals stay on the surface.
	PartialPickup = FALSE;

	if (*Amount >= Max) {
		// Lander full
		PlaySound (SetAbsSoundIndex (LanderSounds, LANDER_FULL),
				NotPositional (), NULL, GAME_SOUND_PRIORITY);
		return false;
	}

	if (*Amount + NumRetrieved > Max) {
		SIZE which_node;
		COUNT oldsize = ElementPtr->mass_points;

		// Deposit could only be picked up partially.
		NumRetrieved = (COUNT)(Max - *Amount);

		if (Scan != BIOLOGICAL_SCAN && optPartialPickup){
			// JMS: Subtract the scavenged kilotons from the mineral deposit.
			// The rest will stay on the surface.
			ElementPtr->mass_points -= NumRetrieved;
		
			// JMS: This makes the mineral deposit subtraction keep  
			// in effect even after leaving & re-entering the planet.
			which_node = HIBYTE (ElementPtr->scan_node) - 1;
			pSolarSysState->SysInfo.PlanetInfo.PartiallyScavengedList[MINERAL_SCAN][which_node] = NumRetrieved;
		
			// JMS: If the deposit was large and its amount now equates to a smaller
			// deposit, change its graphics.
			if ((oldsize > 22 && ElementPtr->mass_points <= 22)
				|| (oldsize > 15 && ElementPtr->mass_points <= 15))
			{
				PRIMITIVE *pPrim = &DisplayArray[ElementPtr->PrimIndex];
				BYTE gfx_index_change = 0;
			
				if (oldsize > 22 && ElementPtr->mass_points <= 15)
					gfx_index_change = 2;
				else
					gfx_index_change = 1;
			
				// Change the scan screen gfx.
				ElementPtr->current.image.frame = SetRelFrameIndex (
					ElementPtr->current.image.frame, (2 - gfx_index_change));
				ElementPtr->next.image.frame = ElementPtr->current.image.frame;

				// Notify the engine that the scan screen gfx should be updated.
				ElementPtr->state_flags |= CHANGING;
				SET_GAME_STATE (PLANETARY_CHANGE, 1);
			
				// Change the surface screen gfx.
				pPrim->Object.Stamp.frame = SetRelFrameIndex (pPrim->Object.Stamp.frame, -gfx_index_change);
			}
		
			PartialPickup = TRUE;
		}
	}

	FillLanderHold (pPSD, Scan, NumRetrieved);

	if (Scan != BIOLOGICAL_SCAN)
		pPSD->ElementAmounts[ElementCategory (EType)] += NumRetrieved;

	pPSD->NumFrames = NUM_TEXT_FRAMES;
	sprintf (pPSD->AmountBuf, "%u", NumRetrieved);
	pStr = GAME_STRING (EType + Offset);

	pPSD->MineralText[0].baseline.x = (SURFACE_WIDTH >> 1)
			+ (ElementControl->EndPoint.x - LanderControl->EndPoint.x);
	pPSD->MineralText[0].baseline.y = (SURFACE_HEIGHT >> 1)
			+ (ElementControl->EndPoint.y - LanderControl->EndPoint.y);
	pPSD->MineralText[0].CharCount = (COUNT)~0;
	pPSD->MineralText[1].pStr = pStr;

	while ((ch = *pStr++) && ch != ' ')
		;
	if (ch == '\0') {
		pPSD->MineralText[1].CharCount = (COUNT)~0;
		pPSD->MineralText[2].CharCount = 0;
	} else {  /* ch == ' ' */
		// Name contains a space. Print over
		// two lines.
		pPSD->MineralText[1].CharCount = utf8StringCountN(
				pPSD->MineralText[1].pStr, pStr - 1);
		pPSD->MineralText[2].pStr = pStr;
		pPSD->MineralText[2].CharCount = (COUNT)~0;
	}

	// JMS
	return (PartialPickup ? false : true);
}

static void
ExplodeCritter (ELEMENT *ElementPtr)
{
	HELEMENT hExplosionElement;
	SIZE temp_which_node;
				
	hExplosionElement = AllocElement ();
	if (hExplosionElement)
	{
		ELEMENT *ExplosionElementPtr;
		LockElement (hExplosionElement, &ExplosionElementPtr);
					
		ExplosionElementPtr->mass_points = DEATH_EXPLOSION;
		ExplosionElementPtr->state_flags = FINITE_LIFE;
		ExplosionElementPtr->playerNr = PS_NON_PLAYER;
		ExplosionElementPtr->next.location = ElementPtr->next.location;
		ExplosionElementPtr->preprocess_func = object_animation;
		ExplosionElementPtr->turn_wait = MAKE_BYTE (2, 2);
		ExplosionElementPtr->life_span = EXPLOSION_LIFE * (LONIBBLE (ExplosionElementPtr->turn_wait));
					
		SetPrimType (&DisplayArray[ExplosionElementPtr->PrimIndex], STAMP_PRIM);
		DisplayArray[ExplosionElementPtr->PrimIndex].Object.Stamp.frame = SetAbsFrameIndex (LanderFrame[0], 46);

		// JMS: This keeps track of the explosion frames. Normally explosion occurs only once (lander explodes).
		// If we don't zero this variable here, the explosion anim can run only once properly and would
		// get stuck in the last frame after that on all the subsequent explosions.
		explosion_index = 0;
					
		UnlockElement (hExplosionElement);
		InsertElement (hExplosionElement, GetHeadElement ());
					
		PlaySound (SetAbsSoundIndex (LanderSounds, LANDER_DESTROYED), NotPositional (), NULL, GAME_SOUND_PRIORITY + 1);
					
		ElementPtr->state_flags |= DISAPPEARING; // JMS: Delete the critter frame
		ElementPtr->mass_points = 0;			 // JMS: Make sure critter/explosion doesn't give biodata.
					
		// JMS: This marks the exploded critter "collected". (even though there was no biodata to collect).
		// This ensures the critter isn't resurrected when visiting the planet next time.
		temp_which_node = HIBYTE (ElementPtr->scan_node) - 1;
		pSolarSysState->SysInfo.PlanetInfo.ScanRetrieveMask[BIOLOGICAL_SCAN] |= (1L << temp_which_node); // Mark this bio blip's state as "collected".
		//pSolarSysState->CurNode = (COUNT)~0; // GenerateLifeForms will update the states of ALL bio-blips when run.
		//callGenerateForScanType (pSolarSysState, pSolarSysState->pOrbitalDesc, &pSolarSysState->CurNode, BIOLOGICAL_SCAN); // Re-run GenerateLifeForms so the changed state takes effect
		SET_GAME_STATE (PLANETARY_CHANGE, 1); // Save the changes to the file containing the states of all lifeforms.
	}
}

static void
shotCreature (ELEMENT *ElementPtr, BYTE value,
		INTERSECT_CONTROL *LanderControl, PRIMITIVE *pPrim)
{
	if (ElementPtr->hit_points == 0)
	{
		// Creature is already canned.
		return;
	}

	--ElementPtr->hit_points;
	if (ElementPtr->hit_points == 0)
	{
		// Brainbox bulldozers (Tractors at moon) explode.
		if ((ElementPtr->mass_points & ~CREATURE_AWARE) == 24)
		{
			ExplodeCritter (ElementPtr);
		}
		// Can other creatures.
		else
		{
			// stash the type of creature in the
			// thrust_wait field.  It seems to be unused
			// by the game for anything at this point
			ElementPtr->thrust_wait = ElementPtr->mass_points & ~CREATURE_AWARE;
			ElementPtr->mass_points = value;
			DisplayArray[ElementPtr->PrimIndex].Object.Stamp.frame =
			pSolarSysState->PlanetSideFrame[0];
		}
	}
	else if (CreatureData[ElementPtr->mass_points & ~CREATURE_AWARE]
			.Attributes & SPEED_MASK)
	{
		COUNT angle;

		angle = FACING_TO_ANGLE (GetFrameIndex (
				LanderControl->IntersectStamp.frame) -
				ANGLE_TO_FACING (FULL_CIRCLE));
		DeltaVelocityComponents (&ElementPtr->velocity,
				COSINE (angle, WORLD_TO_VELOCITY (RES_SCALE(1))),
				SINE (angle, WORLD_TO_VELOCITY (RES_SCALE(1)))); // JMS_GFX
		ElementPtr->thrust_wait = 0;
		ElementPtr->mass_points |= CREATURE_AWARE;
	}

	SetPrimType (pPrim, STAMPFILL_PRIM);
	SetPrimColor (pPrim, WHITE_COLOR);

	PlaySound (SetAbsSoundIndex (LanderSounds, LANDER_HITS),
			NotPositional (), NULL, GAME_SOUND_PRIORITY);
}

static void
CheckObjectCollision (COUNT index)
{
	INTERSECT_CONTROL LanderControl;
	DRAWABLE LanderHandle;
	PRIMITIVE *pPrim;
	PRIMITIVE *pLanderPrim;
	PLANETSIDE_DESC *pPSD = planetSideDesc;

	if (index != END_OF_LIST)
	{
		pLanderPrim = &DisplayArray[index];
		LanderControl.IntersectStamp = pLanderPrim->Object.Stamp;
		index = GetPredLink (GetPrimLinks (pLanderPrim));
	}
	else
	{
		pLanderPrim = 0;
		LanderControl.IntersectStamp.origin.x = SURFACE_WIDTH >> 1;
		LanderControl.IntersectStamp.origin.y = SURFACE_HEIGHT >> 1;
		LanderControl.IntersectStamp.frame = LanderFrame[0];
		index = GetSuccLink (DisplayLinks);
	}

	LanderControl.EndPoint = LanderControl.IntersectStamp.origin;
	LanderHandle = GetFrameParentDrawable (LanderControl.IntersectStamp.frame);

	for (; index != END_OF_LIST; index = GetPredLink (GetPrimLinks (pPrim)))
	{
		INTERSECT_CONTROL ElementControl;
		HELEMENT hElement, hNextElement;

		pPrim = &DisplayArray[index];
		ElementControl.IntersectStamp = pPrim->Object.Stamp;
		ElementControl.EndPoint = ElementControl.IntersectStamp.origin;

		if (GetFrameParentDrawable (ElementControl.IntersectStamp.frame)
				== LanderHandle)
		{
			CheckObjectCollision (index);
			continue;
		}

		if (!DrawablesIntersect (&LanderControl,
				&ElementControl, MAX_TIME_VALUE))
			continue;

		for (hElement = GetHeadElement (); hElement; hElement = hNextElement)
		{
			ELEMENT *ElementPtr;

			LockElement (hElement, &ElementPtr);
			hNextElement = GetSuccElement (ElementPtr);

			if (&DisplayArray[ElementPtr->PrimIndex] == pLanderPrim)
			{
				ElementPtr->state_flags |= DISAPPEARING;
				UnlockElement (hElement);
				continue;
			}
			
			if (&DisplayArray[ElementPtr->PrimIndex] != pPrim
					|| ElementPtr->playerNr != PS_NON_PLAYER)
			{
				UnlockElement (hElement);
				continue;
			}

			{
				COUNT scan, NumRetrieved;
				SIZE which_node;

				scan = LOBYTE (ElementPtr->scan_node);
				if (pLanderPrim == 0)
				{
					/* Collision of lander with another object */
					if (crew_left == 0 || pPSD->InTransit)
						break;

					if (ElementPtr->state_flags & FINITE_LIFE)
					{
						/* A natural disaster */
						scan = ElementPtr->mass_points;
						switch (scan)
						{
							case EARTHQUAKE_DISASTER:
							case LAVASPOT_DISASTER:
								if (scan == LAVASPOT_DISASTER 
									&& IS_HD
									&& TFB_Random () % 100 < 9)
									DeltaLanderCrew (-1, scan);
								else if (TFB_Random () % 100 < 25)
									DeltaLanderCrew (-1, scan);
								break;
						}

						UnlockElement (hElement);
						continue;
					}
					else if (scan == ENERGY_SCAN)
					{
						// noop; handled by generation funcs, see below
					}
					else if (scan == BIOLOGICAL_SCAN && ElementPtr->hit_points)
					{
						BYTE danger_vals[] =
						{
							0, 6, 13, 26
						};
						int creatureIndex = ElementPtr->mass_points
								& ~CREATURE_AWARE;
						int dangerLevel =
								(CreatureData[creatureIndex].Attributes &
								DANGER_MASK) >> DANGER_SHIFT;

						if (TFB_Random () % 128 < danger_vals[dangerLevel])
						{
							PlaySound (SetAbsSoundIndex (
									LanderSounds, BIOLOGICAL_DISASTER),
									NotPositional (), NULL,
									GAME_SOUND_PRIORITY);
							DeltaLanderCrew (-1, BIOLOGICAL_DISASTER);
						}
						UnlockElement (hElement);
						continue;
					}

					NumRetrieved = ElementPtr->mass_points;
				}
				else if (ElementPtr->state_flags & FINITE_LIFE)
				{
					/* Collision of a stun bolt with a natural disaster */
					UnlockElement (hElement);
					continue;
				}
				else
				{
					BYTE value;

					if (scan == ENERGY_SCAN)
					{
						/* Collision of a stun bolt with an energy node */
						UnlockElement (hElement);
						break;
					}

					if (scan == BIOLOGICAL_SCAN
							&& (value = LONIBBLE (CreatureData[
							ElementPtr->mass_points
							& ~CREATURE_AWARE
							].ValueAndHitPoints)))
					{
						/* Collision of a stun bolt with a viable creature */
						shotCreature (ElementPtr, value, &LanderControl,
								pPrim);
						UnlockElement (hElement);
						break;
					}

					NumRetrieved = 0;
				}

				if (NumRetrieved)
				{
					switch (scan)
					{
						case ENERGY_SCAN:
							break;
						case MINERAL_SCAN:
						case BIOLOGICAL_SCAN:
							if (!pickupNode (pPSD, NumRetrieved, ElementPtr, &LanderControl, &ElementControl, scan))
								continue;
							break;
					}
				}

				which_node = HIBYTE (ElementPtr->scan_node) - 1;
				if (callPickupForScanType (pSolarSysState,
						pSolarSysState->pOrbitalDesc, which_node, scan))
				{	// Node retrieved, remove from the surface
					setNodeRetrieved (&pSolarSysState->SysInfo.PlanetInfo,
							scan, which_node);
					SET_GAME_STATE (PLANETARY_CHANGE, 1);
					ElementPtr->state_flags |= DISAPPEARING;
				}
				UnlockElement (hElement);
			}
		}
	}
}

static void
lightning_process (ELEMENT *ElementPtr)
{
	PRIMITIVE *pPrim;

	pPrim = &DisplayArray[ElementPtr->PrimIndex];
	if (LONIBBLE (ElementPtr->turn_wait))
		--ElementPtr->turn_wait;
	else
	{
		COUNT num_frames;

		num_frames = GetFrameCount (pPrim->Object.Stamp.frame) - 7;
		if (GetFrameIndex (pPrim->Object.Stamp.frame) >= num_frames)
		{
			/* Advance to the next surface strike effect frame */
			// XXX: This is unused, we never get here
			pPrim->Object.Stamp.frame =
					IncFrameIndex (pPrim->Object.Stamp.frame);
		}
		else
		{
			SIZE s;
#define NUM_CYCLES 8
			static const Color color_tab[] =
			{
				BUILD_COLOR (MAKE_RGB15_INIT (0x11, 0x11, 0x11), 0x18),
				BUILD_COLOR (MAKE_RGB15_INIT (0x13, 0x13, 0x13), 0x17),
				BUILD_COLOR (MAKE_RGB15_INIT (0x15, 0x15, 0x15), 0x15),
				BUILD_COLOR (MAKE_RGB15_INIT (0x17, 0x17, 0x17), 0x14),
				BUILD_COLOR (MAKE_RGB15_INIT (0x19, 0x19, 0x19), 0x13),
				BUILD_COLOR (MAKE_RGB15_INIT (0x1B, 0x1B, 0x1B), 0x12),
				BUILD_COLOR (MAKE_RGB15_INIT (0x1D, 0x1D, 0x1D), 0x10),
				BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x1F, 0x1F), 0x0f),
			};

			s = ElementPtr->life_span;
			if (s > NUM_CYCLES - 1)
				s = NUM_CYCLES - 1;
			SetPrimColor (pPrim, color_tab[s]);

			if (ElementPtr->mass_points == LIGHTNING_DISASTER)
			{
				/* This one always strikes the lander and can hurt */
				if (crew_left && TFB_Random () % 100 < 10
						&& !planetSideDesc->InTransit)
					lander_flags |= KILL_CREW;

				ElementPtr->next.location = curLanderLoc;
			}

			pPrim->Object.Stamp.frame =
					 SetAbsFrameIndex (pPrim->Object.Stamp.frame,
					 TFB_Random () % num_frames);
		}

		ElementPtr->turn_wait += HINIBBLE (ElementPtr->turn_wait);
	}

	if (GetSuccLink (DisplayLinks) != ElementPtr->PrimIndex)
		lander_flags |= ADD_AT_END;
}

static void
AddLightning (void)
{
	HELEMENT hLightningElement;

	hLightningElement = AllocElement ();
	if (hLightningElement)
	{
		DWORD rand_val;
		ELEMENT *LightningElementPtr;

		LockElement (hLightningElement, &LightningElementPtr);

		LightningElementPtr->playerNr = PS_NON_PLAYER;
		LightningElementPtr->state_flags = FINITE_LIFE;
		LightningElementPtr->preprocess_func = lightning_process;
		if (TFB_Random () % 100 >= 25)
			LightningElementPtr->mass_points = 0; /* harmless */
		else
			LightningElementPtr->mass_points = LIGHTNING_DISASTER;

		rand_val = TFB_Random ();
		LightningElementPtr->life_span = 10 + (HIWORD (rand_val) % 10) + 1;

		if (!IS_HD) {
			LightningElementPtr->next.location.x = (curLanderLoc.x
				+ ((MAP_WIDTH << MAG_SHIFT) - ((SURFACE_WIDTH >> 1) - 6))
				+ (LOBYTE (rand_val) % (SURFACE_WIDTH - 12))
				) % (MAP_WIDTH << MAG_SHIFT);
			LightningElementPtr->next.location.y = (curLanderLoc.y
				+ ((MAP_HEIGHT << MAG_SHIFT) - ((SURFACE_HEIGHT >> 1) - 6))
				+ (HIBYTE (rand_val) % (SURFACE_HEIGHT - 12))
				) % (MAP_HEIGHT << MAG_SHIFT);
		} else {
			LightningElementPtr->next.location.x = (curLanderLoc.x
				+ ((MAP_WIDTH << MAG_SHIFT) - ((SURFACE_WIDTH >> 1) - 6))
				+ (rand_val % (SURFACE_WIDTH - RES_SCALE(12)))
				) % (MAP_WIDTH << MAG_SHIFT);
			LightningElementPtr->next.location.y = (curLanderLoc.y
				+ ((MAP_HEIGHT << MAG_SHIFT) - ((SURFACE_HEIGHT >> 1) - 6))
				+ (rand_val % (SURFACE_HEIGHT - RES_SCALE(12)))
				) % (MAP_HEIGHT << MAG_SHIFT);
		}

		LightningElementPtr->cycle = LightningElementPtr->life_span;
		
		SetPrimType (&DisplayArray[LightningElementPtr->PrimIndex], STAMPFILL_PRIM);
		SetPrimColor (&DisplayArray[LightningElementPtr->PrimIndex], WHITE_COLOR);
		DisplayArray[LightningElementPtr->PrimIndex].Object.Stamp.frame =
				LanderFrame[2];

		UnlockElement (hLightningElement);

		PutElement (hLightningElement);

		PlaySound (SetAbsSoundIndex (LanderSounds, LIGHTNING_DISASTER),
				NotPositional (), NULL, GAME_SOUND_PRIORITY);
	}
}

static HELEMENT
AddGroundDisaster (COUNT which_disaster)
{
	HELEMENT hGroundDisasterElement;

	hGroundDisasterElement = AllocElement ();
	if (hGroundDisasterElement)
	{
		DWORD rand_val;
		ELEMENT *GroundDisasterElementPtr;
		PRIMITIVE *pPrim;

		LockElement (hGroundDisasterElement, &GroundDisasterElementPtr);

		pPrim = &DisplayArray[GroundDisasterElementPtr->PrimIndex];
		GroundDisasterElementPtr->mass_points = which_disaster;
		GroundDisasterElementPtr->playerNr = PS_NON_PLAYER;
		GroundDisasterElementPtr->state_flags = FINITE_LIFE;
		GroundDisasterElementPtr->preprocess_func = object_animation;

		rand_val = TFB_Random ();
		GroundDisasterElementPtr->next.location.x = (curLanderLoc.x
				+ ((MAP_WIDTH << MAG_SHIFT) - (SURFACE_WIDTH * 3 / 8))
				+ (LOWORD (rand_val) % (SURFACE_WIDTH * 3 / 4))
				) % (MAP_WIDTH << MAG_SHIFT);
		GroundDisasterElementPtr->next.location.y = (curLanderLoc.y
				+ ((MAP_HEIGHT << MAG_SHIFT) - (SURFACE_HEIGHT * 3 / 8))
				+ (HIWORD (rand_val) % (SURFACE_HEIGHT * 3 / 4))
				) % (MAP_HEIGHT << MAG_SHIFT);


		if (which_disaster == EARTHQUAKE_DISASTER)
		{
			SetPrimType (pPrim, STAMP_PRIM); // JMS: was STAMPFILL_PRIM (this rendered it totally white).
			pPrim->Object.Stamp.frame = LanderFrame[1];
			GroundDisasterElementPtr->turn_wait = MAKE_BYTE (2, 2);
		}
		else /* if (which_disaster == LAVASPOT_DISASTER) */
		{
			SetPrimType (pPrim, STAMP_PRIM);
			GroundDisasterElementPtr->facing =
					NORMALIZE_FACING (TFB_Random ());
			pPrim->Object.Stamp.frame = LanderFrame[3];
			GroundDisasterElementPtr->turn_wait = MAKE_BYTE (0, 0);
		}
		GroundDisasterElementPtr->life_span =
				GetFrameCount (pPrim->Object.Stamp.frame)
				* (LONIBBLE (GroundDisasterElementPtr->turn_wait) + 1) - 1;

		UnlockElement (hGroundDisasterElement);

		PutElement (hGroundDisasterElement);
	}

	return (hGroundDisasterElement);
}

// This function replaces the ELEMENT manipulations typically done by
// PreProcess() and PostProcess() in process.c. Lander code does not
// call RedrawQueue() & Co and thus does not reap the benefits (or curses,
// depending how you look at it) of automatic flags processing.
static void
BuildObjectList (void)
{
	DWORD rand_val;
	POINT org;
	HELEMENT hElement, hNextElement;
	PLANETSIDE_DESC *pPSD = planetSideDesc;

	DisplayLinks = MakeLinks (END_OF_LIST, END_OF_LIST);
	
	lander_flags &= ~KILL_CREW;

	rand_val = TFB_Random ();
	if (LOBYTE (HIWORD (rand_val)) < pPSD->FireChance)
	{
		AddGroundDisaster (LAVASPOT_DISASTER);
		PlaySound (SetAbsSoundIndex (LanderSounds, LAVASPOT_DISASTER),
				NotPositional (), NULL, GAME_SOUND_PRIORITY);
	}

	if (HIBYTE (LOWORD (rand_val)) < pPSD->TectonicsChance)
		AddGroundDisaster (EARTHQUAKE_DISASTER);

	if (LOBYTE (LOWORD (rand_val)) < pPSD->WeatherChance)
		AddLightning ();

	org = curLanderLoc;
	for (hElement = GetHeadElement ();
			hElement; hElement = hNextElement)
	{
		SIZE dx, dy;
		ELEMENT *ElementPtr;

		LockElement (hElement, &ElementPtr);

		if (ElementPtr->life_span == 0
				|| (ElementPtr->state_flags & DISAPPEARING))
		{
			hNextElement = GetSuccElement (ElementPtr);
			UnlockElement (hElement);
			RemoveElement (hElement);
			FreeElement (hElement);
			continue;
		}
		else if (ElementPtr->state_flags & FINITE_LIFE)
			--ElementPtr->life_span;

		lander_flags &= ~ADD_AT_END;

		if (ElementPtr->preprocess_func)
			(*ElementPtr->preprocess_func) (ElementPtr);

		GetNextVelocityComponents (&ElementPtr->velocity, &dx, &dy, 1);
		if (dx || dy)
		{
			ElementPtr->next.location.x += dx;
			ElementPtr->next.location.y += dy;
				/* if not lander's shot */
			if (ElementPtr->playerNr != PS_HUMAN_PLAYER)
			{
				if (ElementPtr->next.location.y < 0)
					ElementPtr->next.location.y = 0;
				else if (ElementPtr->next.location.y >= (MAP_HEIGHT << MAG_SHIFT))
					ElementPtr->next.location.y = (MAP_HEIGHT << MAG_SHIFT) - 1;
			}
			if (ElementPtr->next.location.x < 0)
				ElementPtr->next.location.x += MAP_WIDTH << MAG_SHIFT;
			else
				ElementPtr->next.location.x %= MAP_WIDTH << MAG_SHIFT;
			
			// XXX: APPEARING flag is set by scan.c for scanned blips
			if (ElementPtr->state_flags & APPEARING)
			{	// Update the location of a moving object on the scan map
				ElementPtr->current.location.x =
						ElementPtr->next.location.x >> MAG_SHIFT;
				ElementPtr->current.location.y =
						ElementPtr->next.location.y >> MAG_SHIFT;
			}
		}

		{
			PRIMITIVE *pPrim;

			pPrim = &DisplayArray[ElementPtr->PrimIndex];
			pPrim->Object.Stamp.origin.x =
					ElementPtr->next.location.x
					- org.x + (SURFACE_WIDTH >> 1);
			if (pPrim->Object.Stamp.origin.x >=
					(MAP_WIDTH << MAG_SHIFT) - (SURFACE_WIDTH * 3 / 2))
				pPrim->Object.Stamp.origin.x -= MAP_WIDTH << MAG_SHIFT;
			else if (pPrim->Object.Stamp.origin.x <=
					-((MAP_WIDTH << MAG_SHIFT) - (SURFACE_WIDTH * 3 / 2)))
				pPrim->Object.Stamp.origin.x += MAP_WIDTH << MAG_SHIFT;

			pPrim->Object.Stamp.origin.y =
					ElementPtr->next.location.y
					- org.y + (SURFACE_HEIGHT >> 1);

			if (lander_flags & ADD_AT_END)
				InsertPrim (&DisplayLinks, ElementPtr->PrimIndex, END_OF_LIST);
			else
				InsertPrim (&DisplayLinks, ElementPtr->PrimIndex, GetPredLink (DisplayLinks));
		}

		hNextElement = GetSuccElement (ElementPtr);
		UnlockElement (hElement);
	}
}

static void
ScrollPlanetSide (SIZE dx, SIZE dy, int landingOffset)
{
	POINT new_pt;
	STAMP lander_s, shadow_s, shield_s;
	CONTEXT OldContext;

	new_pt.y = curLanderLoc.y + dy;
	if (new_pt.y < 0)
	{
		new_pt.y = 0;
		dy = new_pt.y - curLanderLoc.y;
		dx = 0;
		ZeroVelocityComponents (&GLOBAL (velocity));
	}
	else if (new_pt.y > (MAP_HEIGHT << MAG_SHIFT) - 1)
	{
		new_pt.y = (MAP_HEIGHT << MAG_SHIFT) - 1;
		dy = new_pt.y - curLanderLoc.y;
		dx = 0;
		ZeroVelocityComponents (&GLOBAL (velocity));
	}

	new_pt.x = curLanderLoc.x + dx;
	if (new_pt.x < 0)
		new_pt.x += MAP_WIDTH << MAG_SHIFT;
	else if (new_pt.x >= MAP_WIDTH << MAG_SHIFT)
		new_pt.x -= MAP_WIDTH << MAG_SHIFT;
	
	curLanderLoc = new_pt;

	OldContext = SetContext (PlanetContext);

	BatchGraphics ();

	// Display planet area, accounting for horizontal wrapping if
	// near the edges.
	{
		STAMP s;

		ClearDrawable ();
		s.origin.x = -new_pt.x + (SURFACE_WIDTH >> 1);
		s.origin.y = -new_pt.y + (SURFACE_HEIGHT >> 1);
		s.frame = pSolarSysState->Orbit.TopoZoomFrame;
		DrawStamp (&s);
		s.origin.x += MAP_WIDTH << MAG_SHIFT;
		DrawStamp (&s);
		s.origin.x -= MAP_WIDTH << (MAG_SHIFT + 1);
		DrawStamp (&s);
	}

	BuildObjectList ();
	
	DrawBatch (DisplayArray, DisplayLinks, 0);

	// Draw the lander while is still alive and keep drawing for a few
	// frames while it is exploding
	if (crew_left || damage_index || explosion_index < 3)
	{
		lander_s.origin.x = SURFACE_WIDTH >> 1;
		lander_s.origin.y = (SURFACE_HEIGHT >> 1) + landingOffset;
		lander_s.frame = LanderFrame[0];

		if (landingOffset != ON_THE_GROUND)
		{	// Landing, draw a shadow
			shadow_s.origin.x = lander_s.origin.y + (SURFACE_WIDTH >> 1) - (SURFACE_HEIGHT >> 1);//2;
			shadow_s.origin.y = lander_s.origin.y;
			shadow_s.frame = lander_s.frame;
			SetContextForeGroundColor (BLACK_COLOR);
			DrawFilledStamp (&shadow_s);
		}

		if (damage_index == 0)
		{	// No damage -- normal lander
			DrawStamp (&lander_s);
		}
		else if (shieldHit)
		{	// Was protected by a shield
			--damage_index;
			if (damage_index > 0)
			{
				shield_s.origin = lander_s.origin;
				shield_s.frame = SetEquFrameIndex (
						LanderFrame[4], lander_s.frame);

				// XXX: Shouldn't this color-cycle with damage_index?
				//   damage_index is used, but only as a VGA index!
				/*SetContextForeGroundColor (BUILD_COLOR (
						MAKE_RGB15 (0x1F, 0x1F, 0x1F) | 0x8000,
						damage_index));*/
				SetContextForeGroundColor (
						BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), damage_index));
				DrawFilledStamp (&shield_s);
			}
			DrawStamp (&lander_s);
		}
		else
		{	// Direct hit, no shield
			--damage_index;
			SetContextForeGroundColor (
					DamageColorCycle (BLACK_COLOR, damage_index));
			DrawFilledStamp (&lander_s);
		}
	}
	
	if (landingOffset == ON_THE_GROUND && crew_left
			&& GetPredLink (DisplayLinks) != END_OF_LIST)
		CheckObjectCollision (END_OF_LIST);

	{
		PLANETSIDE_DESC *pPSD = planetSideDesc;
		if (pPSD->NumFrames)
		{
			--pPSD->NumFrames;
			SetContextForeGroundColor (pPSD->ColorCycle[pPSD->NumFrames >> 1]);

			pPSD->MineralText[0].baseline.x -= dx;
			pPSD->MineralText[0].baseline.y -= dy;
			font_DrawText (&pPSD->MineralText[0]);
			pPSD->MineralText[1].baseline.x = pPSD->MineralText[0].baseline.x;
			pPSD->MineralText[1].baseline.y = pPSD->MineralText[0].baseline.y + RES_SCALE(7); // JMS_GFX
			font_DrawText (&pPSD->MineralText[1]);
			pPSD->MineralText[2].baseline.x = pPSD->MineralText[1].baseline.x;
			pPSD->MineralText[2].baseline.y = pPSD->MineralText[1].baseline.y + RES_SCALE(7); // JMS_GFX
			font_DrawText (&pPSD->MineralText[2]);
		}
	}

	RedrawSurfaceScan (&new_pt);

	if (lander_flags & KILL_CREW)
		DeltaLanderCrew (-1, LIGHTNING_DISASTER);

	UnbatchGraphics ();

	SetContext (OldContext);
}

static void
animationInterframe (TimeCount *TimeIn, COUNT periods)
{
#define ANIM_FRAME_RATE  (ONE_SECOND / 30)

	for ( ; periods; --periods)
	{
		RotatePlanetSphere (TRUE);

		SleepThreadUntil (*TimeIn + ANIM_FRAME_RATE);
		*TimeIn = GetTimeCounter ();
	}
}

static void
AnimateLaunch (FRAME farray, BOOLEAN landing)
{
	RECT r;
	STAMP s;
	COUNT num_frames;
	TimeCount NextTime;

	SetContext (PlanetContext);

	r.corner.x = 0;
	r.corner.y = 0;
	r.extent.width = 0;
	r.extent.height = 0;
	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = farray;

	for (num_frames = GetFrameCount (s.frame); num_frames; --num_frames)
	{
		NextTime = GetTimeCounter () + (ONE_SECOND / 22);

		BatchGraphics ();
		RepairBackRect (&r, TRUE);
#ifdef SPIN_ON_LAUNCH
		RotatePlanetSphere (FALSE);
#else
		DrawDefaultPlanetSphere ();
#endif
		DrawStamp (&s);
		UnbatchGraphics ();

		GetFrameRect (s.frame, &r);
		s.frame = IncFrameIndex (s.frame);

		SleepThreadUntil (NextTime);
	}

	// This clears the last lander return / launch) anim frame from the planet window.
	if (!IS_HD || !landing)
		RepairBackRect (&r, FALSE);
}

static void
AnimateLanderWarmup (void)
{
	SIZE num_crew;
	STAMP s;
	CONTEXT OldContext;
	TimeCount TimeIn = GetTimeCounter ();

	OldContext = SetContext (RadarContext);

	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = SetAbsFrameIndex (LanderFrame[0],
			(ANGLE_TO_FACING (FULL_CIRCLE) << 1) + 1);

	DrawStamp (&s);

	animationInterframe (&TimeIn, 2);

	for (num_crew = 0; num_crew < (NUM_CREW_COLS * NUM_CREW_ROWS)
			&& GLOBAL_SIS (CrewEnlisted); ++num_crew)
	{
		animationInterframe (&TimeIn, 1);
		
		DeltaSISGauges (-1, 0, 0);
		DeltaLanderCrew (1, 0);
	}

	animationInterframe (&TimeIn, 2);

	if (GET_GAME_STATE (IMPROVED_LANDER_SHOT))
		s.frame = SetAbsFrameIndex (s.frame, 58);
	else
		s.frame = SetAbsFrameIndex (s.frame,
				(ANGLE_TO_FACING (FULL_CIRCLE) << 1) + 2);
	DrawStamp (&s);

	animationInterframe (&TimeIn, 2);

	if (GET_GAME_STATE (IMPROVED_LANDER_SPEED))
		s.frame = SetAbsFrameIndex (s.frame, 57);
	else
	{
		s.frame = SetAbsFrameIndex (s.frame,
				(ANGLE_TO_FACING (FULL_CIRCLE) << 1) + 3);
		DrawStamp (&s);

		animationInterframe (&TimeIn, 2);

		s.frame = IncFrameIndex (s.frame);
	}
	DrawStamp (&s);

	if (GET_GAME_STATE (IMPROVED_LANDER_CARGO))
	{
		animationInterframe (&TimeIn, 2);

		s.frame = SetAbsFrameIndex (s.frame, 59);
		DrawStamp (&s);
	}

	animationInterframe (&TimeIn, 2);

	PlaySound (SetAbsSoundIndex (LanderSounds, LANDER_DEPARTS),
			NotPositional (), NULL, GAME_SOUND_PRIORITY + 1);
}

static void
InitPlanetSide (POINT pt)
{
	// Adjust landing location by a random jitter.
#define RANDOM_MISS RES_SCALE(64) // JMS_GFX
	if(!optGodMode){
		pt.x -= RANDOM_MISS - TFB_Random () % (RANDOM_MISS << 1);
		pt.y -= RANDOM_MISS - TFB_Random () % (RANDOM_MISS << 1);
	} else { 
		pt.x -= 0;
		pt.y -= 0;
	}
	// Jitter the X landing point.
	if (pt.x < 0)
		pt.x += (MAP_WIDTH << MAG_SHIFT);
	else if (pt.x >= (MAP_WIDTH << MAG_SHIFT))
		pt.x -= (MAP_WIDTH << MAG_SHIFT);

	// Jitter the Y landing point.
	if (pt.y < 0)
		pt.y = 0;
	else if (pt.y >= (MAP_HEIGHT << MAG_SHIFT))
		pt.y = (MAP_HEIGHT << MAG_SHIFT) - 1;

	curLanderLoc = pt;

	SetContext (PlanetContext);
	SetContextFont (TinyFont);

	{
		RECT r;

		GetContextClipRect (&r);

		SetTransitionSource (&r);
		BatchGraphics ();
		
		{
			STAMP s;

			// Note - This code is the same as in ScrollPlanetSize,
			// Display planet area, accounting for horizontal wrapping if
			// near the edges.
			ClearDrawable ();
			s.origin.x = -pt.x + (SURFACE_WIDTH >> 1);
			s.origin.y = -pt.y + (SURFACE_HEIGHT >> 1);
			s.frame = pSolarSysState->Orbit.TopoZoomFrame;
			DrawStamp (&s);
			s.origin.x += MAP_WIDTH << MAG_SHIFT;
			DrawStamp (&s);
			s.origin.x -= MAP_WIDTH << (MAG_SHIFT + 1);
			DrawStamp (&s);
		}

		ScreenTransition (3, &r);
		UnbatchGraphics ();
	}


	SET_GAME_STATE (PLANETARY_LANDING, 1);
}

static void
LanderFire (SIZE facing)
{
#define SHUTTLE_FIRE_WAIT 15
	HELEMENT hWeaponElement;
	SIZE wdx, wdy;
	ELEMENT *WeaponElementPtr;
	COUNT angle;

	hWeaponElement = AllocElement ();
	if (hWeaponElement == NULL)
		return;

	LockElement (hWeaponElement, &WeaponElementPtr);

	WeaponElementPtr->playerNr = PS_HUMAN_PLAYER;
	WeaponElementPtr->mass_points = 1;
	WeaponElementPtr->life_span = 12;
	WeaponElementPtr->state_flags = FINITE_LIFE;
	WeaponElementPtr->next.location = curLanderLoc;

	SetPrimType (&DisplayArray[WeaponElementPtr->PrimIndex], STAMP_PRIM);
	DisplayArray[WeaponElementPtr->PrimIndex].Object.Stamp.frame =
			SetAbsFrameIndex (LanderFrame[0],
			/* shot images immediately follow the lander images */
			facing + ANGLE_TO_FACING (FULL_CIRCLE));

	if (!CurrentInputState.key[PlayerControls[0]][KEY_UP])
	{
		wdx = 0;
		wdy = 0;
	}
	else
	{
		GetCurrentVelocityComponents (&GLOBAL (velocity), &wdx, &wdy);
	}

	angle = FACING_TO_ANGLE (facing);
	SetVelocityComponents (
		&WeaponElementPtr->velocity,
		COSINE (angle, WORLD_TO_VELOCITY (RES_SCALE(2 * 3))) + wdx,
		SINE (angle, WORLD_TO_VELOCITY (RES_SCALE(2 * 3))) + wdy); // JMS_GFX

	UnlockElement (hWeaponElement);

	InsertElement (hWeaponElement, GetHeadElement ());

	PlaySound (SetAbsSoundIndex (LanderSounds, LANDER_SHOOTS),
			NotPositional (), NULL, GAME_SOUND_PRIORITY);
}

static BOOLEAN
LanderExplosion (void)
{
	HELEMENT hExplosionElement;
	ELEMENT *ExplosionElementPtr;

	hExplosionElement = AllocElement ();
	if (!hExplosionElement)
		return FALSE;

	LockElement (hExplosionElement, &ExplosionElementPtr);

	ExplosionElementPtr->playerNr = PS_HUMAN_PLAYER;
	ExplosionElementPtr->mass_points = DEATH_EXPLOSION;
	ExplosionElementPtr->state_flags = FINITE_LIFE;
	ExplosionElementPtr->next.location = curLanderLoc;
	ExplosionElementPtr->preprocess_func = object_animation;
	// Animation advances every 3rd frame
	ExplosionElementPtr->turn_wait = MAKE_BYTE (2, 2);
	ExplosionElementPtr->life_span = EXPLOSION_LIFE
			* (LONIBBLE (ExplosionElementPtr->turn_wait) + 1);

	SetPrimType (&DisplayArray[ExplosionElementPtr->PrimIndex],
			STAMP_PRIM);
	DisplayArray[ExplosionElementPtr->PrimIndex].Object.Stamp.frame =
			SetAbsFrameIndex (LanderFrame[0], 46);

	UnlockElement (hExplosionElement);

	InsertElement (hExplosionElement, GetHeadElement ());

	PlaySound (SetAbsSoundIndex (LanderSounds, LANDER_DESTROYED),
			NotPositional (), NULL, GAME_SOUND_PRIORITY + 1);

	return TRUE;
}

static BOOLEAN
DoPlanetSide (LanderInputState *pMS)
{
	SIZE dx = 0;
	SIZE dy = 0;

#define SHUTTLE_TURN_WAIT 2
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE);

	if (!pMS->Initialized)
	{
		COUNT landerSpeedNumer;
		COUNT angle;

		pMS->Initialized = TRUE;
		
		turn_wait = 0;
		weapon_wait = 0;

		angle = FACING_TO_ANGLE (GetFrameIndex (LanderFrame[0]));
		landerSpeedNumer = GET_GAME_STATE (IMPROVED_LANDER_SPEED) ?
			WORLD_TO_VELOCITY (2 * RES_SCALE(16)) :
			WORLD_TO_VELOCITY (2 * RES_SCALE(8));

#ifdef FAST_FAST
landerSpeedNumer = WORLD_TO_VELOCITY (RES_SCALE(48)); // JMS
#endif

		SetVelocityComponents (&GLOBAL (velocity),
				COSINE (angle, landerSpeedNumer) / LANDER_SPEED_DENOM,
				SINE (angle, landerSpeedNumer) / LANDER_SPEED_DENOM);

		return TRUE;
	}
	else if (crew_left /* alive and taking off */
			&& ((CurrentInputState.key[PlayerControls[0]][KEY_ESCAPE] ||
			CurrentInputState.key[PlayerControls[0]][KEY_SPECIAL])
			|| planetSideDesc->InTransit))
	{
		return FALSE;
	}
	else if (!crew_left && !damage_index)
	{	// Dead, damage dealt, and exploding
		if (explosion_index > EXPLOSION_LIFE + EXPLOSION_WAIT_FRAMES)
			return FALSE;
		
		if (explosion_index > EXPLOSION_LIFE)
		{	// Keep going until the wait expires
			++explosion_index;
		}
		else if (explosion_index == 0)
		{	// Start the explosion animation
			if (LanderExplosion ())
			{
				// Advance the state only once we've got the element
				++explosion_index;
			}
			else
			{	// We could not allocate because the queue was full, but
				// we will get another chance on the next iteration
				log_add (log_Warning, "DoPlanetSide(): could not"
						" allocate explosion element!");
			}
		}
	}
	else
	{
		if (crew_left)
		{
			SIZE index = GetFrameIndex (LanderFrame[0]);
#if defined(ANDROID) || defined(__ANDROID__)
			BATTLE_INPUT_STATE InputState = GetDirectionalJoystickInput(index, 0);
#endif
			if (turn_wait)
				--turn_wait;
#if defined(ANDROID) || defined(__ANDROID__)
			else if ((InputState & BATTLE_LEFT) || (InputState & BATTLE_RIGHT))
#else
			else if (CurrentInputState.key[PlayerControls[0]][KEY_LEFT] ||
				CurrentInputState.key[PlayerControls[0]][KEY_RIGHT])
#endif

			{
				COUNT landerSpeedNumer;
				COUNT angle;

#if defined(ANDROID) || defined(__ANDROID__)
				if (InputState & BATTLE_LEFT)
#else
				if (CurrentInputState.key[PlayerControls[0]][KEY_LEFT])
#endif
					--index;
				else
					++index;

				index = NORMALIZE_FACING (index);
				LanderFrame[0] = SetAbsFrameIndex (LanderFrame[0], index);

				angle = FACING_TO_ANGLE (index);
				landerSpeedNumer = GET_GAME_STATE (IMPROVED_LANDER_SPEED) ?
					WORLD_TO_VELOCITY (RES_SCALE(2 * 16)) :
					WORLD_TO_VELOCITY (RES_SCALE(2 * 8));

#ifdef FAST_FAST
landerSpeedNumer = WORLD_TO_VELOCITY (RES_SCALE(48));
#endif

				SetVelocityComponents (&GLOBAL (velocity),
						COSINE (angle, landerSpeedNumer) / LANDER_SPEED_DENOM,
						SINE (angle, landerSpeedNumer) / LANDER_SPEED_DENOM);

				turn_wait = SHUTTLE_TURN_WAIT;
			}
#if defined(ANDROID) || defined(__ANDROID__)
			if (!(InputState & BATTLE_THRUST))
#else
			if (!CurrentInputState.key[PlayerControls[0]][KEY_UP])
#endif
			{
				dx = 0;
				dy = 0;
			}
			else
				GetNextVelocityComponents (&GLOBAL (velocity), &dx, &dy, 1);

			if (weapon_wait)
				--weapon_wait;
			else if (CurrentInputState.key[PlayerControls[0]][KEY_WEAPON])
			{
				LanderFire (index);

				weapon_wait = SHUTTLE_FIRE_WAIT;
				if (GET_GAME_STATE (IMPROVED_LANDER_SHOT))
					weapon_wait >>= 1;
			}
		}
	}

	ScrollPlanetSide (dx, dy, ON_THE_GROUND);

	SleepThreadUntil (pMS->NextTime);
	// NOTE: The rate is not stabilized
	pMS->NextTime = GetTimeCounter () + PLANET_SIDE_RATE;

	return TRUE;
}

void
FreeLanderData (void)
{
	COUNT i;
	COUNT landerFrameCount;

	if (LanderFrame[0] == NULL)
		return;

	for (i = 0; i < NUM_ORBIT_THEMES; ++i)
	{
		DestroyMusic (OrbitMusic[i]);
		OrbitMusic[i] = 0;
	}

	DestroySound (ReleaseSound (LanderSounds));
	LanderSounds = 0;

	landerFrameCount = sizeof (LanderFrame) / sizeof (LanderFrame[0]);
	for (i = 0; i < landerFrameCount; ++i)
	{
		DestroyDrawable (ReleaseDrawable (LanderFrame[i]));
		LanderFrame[i] = 0;
	}
}

void
LoadLanderData (void)
{
	if (LanderFrame[0] != 0)
		return;

	LanderFrame[0] =
			CaptureDrawable (LoadGraphic (LANDER_MASK_PMAP_ANIM));
	LanderFrame[1] =
			CaptureDrawable (LoadGraphic (QUAKE_MASK_PMAP_ANIM));
	LanderFrame[2] =
			CaptureDrawable (LoadGraphic (LIGHTNING_MASK_ANIM));
	LanderFrame[3] =
			CaptureDrawable (LoadGraphic (LAVA_MASK_PMAP_ANIM));
	LanderFrame[4] =
			CaptureDrawable (LoadGraphic (LANDER_SHIELD_MASK_ANIM));
	LanderFrame[5] =
			CaptureDrawable (LoadGraphic (LANDER_LAUNCH_MASK_PMAP_ANIM));
	LanderFrame[6] =
			CaptureDrawable (LoadGraphic (LANDER_RETURN_MASK_PMAP_ANIM));
	LanderFrame[7] =
			CaptureDrawable (LoadGraphic (ORBIT_VIEW_ANIM));
	
	LanderSounds = CaptureSound (LoadSound (LANDER_SOUNDS));

	{
		COUNT i;

		for (i = 0; i < NUM_ORBIT_THEMES; ++i)
			OrbitMusic[i] = load_orbit_theme (i);
	}
}

void
SetPlanetMusic (BYTE planet_type)
{
	LanderMusic = OrbitMusic[planet_type % NUM_ORBIT_THEMES];
}

static void
ReturnToOrbit (void)
{
	CONTEXT OldContext;
	RECT r;

	OldContext = SetContext (PlanetContext);
	GetContextClipRect (&r);

	SetTransitionSource (&r);
	BatchGraphics ();
	
	// JMS: This will hide the table of mineral values on the status bar.
	if (optSubmenu){
		if(optCustomBorder)
			DrawBorder(13, FALSE);
		else
			DrawSubmenu (0);
	}

	DrawStarBackGround ();
	DrawPlanetSurfaceBorder ();
	RedrawSurfaceScan (NULL);
	ScreenTransition (3, &r);
	UnbatchGraphics ();

	SetContext (OldContext);
}

static void
IdlePlanetSide (LanderInputState *inputState, TimeCount howLong)
{
#define IDLE_OFFSET     
	TimeCount TimeOut = GetTimeCounter () + howLong;

	while (GetTimeCounter () < TimeOut)
	{
		// 10 to clear the lander off of the screen
		ScrollPlanetSide (0, 0, -(SURFACE_HEIGHT / 2 + RES_SCALE(10))); // JMS_GFX
		SleepThreadUntil (inputState->NextTime);
		inputState->NextTime += PLANET_SIDE_RATE;
	}
}

static void
LandingTakeoffSequence (LanderInputState *inputState, BOOLEAN landing)
{
// We cannot solve a quadratic equation in a macro, so use a sensible max
#define MAX_OFFSETS  20
#define MAX_OFFSETS_HD 400 // JMS_GFX
// RES_SCALE(10) to clear the lander off of the screen
#define DISTANCE_COVERED  (SURFACE_HEIGHT / 2 + RES_SCALE(10))
	int landingOfs[MAX_OFFSETS];
	int start;
	int end;
	int delta;
	int index;
	int max_offsets; // JMS_GFX
	int landingOfsHD[MAX_OFFSETS_HD]; // JMS_GFX

	// Produce smooth acceleration deltas from a simple 1..x progression
	delta = 0;
	// JMS_GFX: In HD graphics we run out of default offsets. -> Use larger offset value.
	max_offsets = MAX_OFFSETS;
	if (IS_HD) 
		max_offsets = MAX_OFFSETS_HD;

	for (index = 0; index < max_offsets && delta < DISTANCE_COVERED; ++index)
	{
		delta += index + 1;
		// JMS_GFX
		if (IS_HD)
			landingOfsHD[index] = -delta;
		else
			landingOfs[index] = -delta;
	}
	assert (delta >= DISTANCE_COVERED && "Increase MAX_OFFSETS!");

	if (landing)
	{
		start = index - 1;
		end = -1;
		delta = -1;
	}
	else
	{	// takeoff
		start = 0;
		end = index;
		delta = +1;
	}

	if (landing)
		IdlePlanetSide (inputState, ONE_SECOND);

	// Draw the landing/takeoff lander positions
	for (index = start; index != end; index += delta)
	{
		// JMS_GFX
		if (IS_HD)
			ScrollPlanetSide (0, 0, landingOfsHD[index]);
		else
			ScrollPlanetSide (0, 0, landingOfs[index]);

		SleepThreadUntil (inputState->NextTime);
		inputState->NextTime += PLANET_SIDE_RATE;
	}

	if (!landing)
		IdlePlanetSide (inputState, ONE_SECOND / 2);
}

void
SetLanderTakeoff (void)
{
	assert (planetSideDesc != NULL);
	if (planetSideDesc)
		planetSideDesc->InTransit = TRUE;
}

// Returns whether the lander is still alive at the end of the sequence
bool
KillLanderCrewSeq (COUNT numKilled, DWORD period)
{
	TimeCount TimeOut;
	COUNT i;

	TimeOut = GetTimeCounter ();
	for (i = 0; i < numKilled && crew_left; ++i)
	{
		TimeOut += period;
		DeltaLanderCrew (-1, LANDER_INJURED);
		SleepThreadUntil (TimeOut);
	}
	
	return crew_left > 0;
}

// Maps a temperature to a (0-7) hazard rating.
// Thermal hazards aren't exposed to the user as a hazard number,
// but the code still works with them that way.
#define ARRAY_SIZE(array) (sizeof(array) / sizeof (*array))
unsigned
GetThermalHazardRating (int temp)
{
	static const int tempBreakpoints[] = { 50, 100, 150, 250, 350, 550, 800 };
	const size_t numBreakpoints = ARRAY_SIZE (tempBreakpoints);
	unsigned i;

	for (i = 0; i < numBreakpoints; ++i)
	{
		if (temp < tempBreakpoints[i])
			return i;
	}

	return numBreakpoints;
}

// Given a hazard type and rating, return the chance (out of 256) of the hazard
// being generated.
static BYTE
GetHazardChance (int hazardType, unsigned HazardRating)
{
	static const BYTE TectonicsChanceTab[] = {0*3, 0*3, 1*3, 2*3, 4*3,  8*3, 16*3, 32*3};
	static const BYTE WeatherChanceTab  [] = {0*3, 0*3, 1*3, 2*3, 3*3,  6*3, 12*3, 24*3};
	static const BYTE FireChanceTab     [] = {0*3, 0*3, 1*3, 2*3, 4*3, 12*3, 24*3, 48*3};

	switch (hazardType)
	{
		case EARTHQUAKE_DISASTER:
			return TectonicsChanceTab[HazardRating];
		case LIGHTNING_DISASTER:
			return WeatherChanceTab[HazardRating];
		case LAVASPOT_DISASTER:
			return FireChanceTab[HazardRating];
	}

	return 0;
}

void
PlanetSide (POINT planetLoc)
{
	SIZE index;
	LanderInputState landerInputState;
	PLANETSIDE_DESC PSD;

	memset (&PSD, 0, sizeof (PSD));
	PSD.InTransit = TRUE;

	// Set our chances of hazards occurring.
	PSD.TectonicsChance = GetHazardChance (EARTHQUAKE_DISASTER,
			pSolarSysState->SysInfo.PlanetInfo.Tectonics);
	PSD.WeatherChance = GetHazardChance (LIGHTNING_DISASTER,
			pSolarSysState->SysInfo.PlanetInfo.Weather);
	PSD.FireChance = GetHazardChance (LAVASPOT_DISASTER, GetThermalHazardRating (
			pSolarSysState->SysInfo.PlanetInfo.SurfaceTemperature));

	PSD.ElementLevel = GetStorageBayCapacity () - GLOBAL_SIS (TotalElementMass);
	PSD.MaxElementLevel = MAX_SCROUNGED;
	if (GET_GAME_STATE (IMPROVED_LANDER_CARGO))
		PSD.MaxElementLevel <<= 1;
	if (PSD.ElementLevel < PSD.MaxElementLevel)
		PSD.MaxElementLevel = PSD.ElementLevel;
	PSD.ElementLevel = 0;

	PSD.MineralText[0].align = ALIGN_CENTER;
	PSD.MineralText[0].pStr = PSD.AmountBuf;
	PSD.MineralText[1] = PSD.MineralText[0];
	PSD.MineralText[2] = PSD.MineralText[1];

	PSD.ColorCycle[0] = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x03, 0x00), 0x7F);
	PSD.ColorCycle[1] = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x00), 0x7D);
	PSD.ColorCycle[2] = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x11, 0x00), 0x7B);
	PSD.ColorCycle[3] = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x00), 0x71);
	for (index = 4; index < (NUM_TEXT_FRAMES >> 1) - 4; ++index)
	{
		PSD.ColorCycle[index] =
				BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F);
	}
	PSD.ColorCycle[(NUM_TEXT_FRAMES >> 1) - 4] =
			BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x00), 0x71);
	PSD.ColorCycle[(NUM_TEXT_FRAMES >> 1) - 3] =
			BUILD_COLOR (MAKE_RGB15 (0x1F, 0x11, 0x00), 0x7B);
	PSD.ColorCycle[(NUM_TEXT_FRAMES >> 1) - 2] =
			BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x00), 0x7D);
	PSD.ColorCycle[(NUM_TEXT_FRAMES >> 1) - 1] =
			BUILD_COLOR (MAKE_RGB15 (0x1F, 0x03, 0x00), 0x7F);
	planetSideDesc = &PSD;
	
	index = NORMALIZE_FACING (TFB_Random ());
	LanderFrame[0] = SetAbsFrameIndex (LanderFrame[0], index);
	crew_left = 0;
	damage_index = 0;
	explosion_index = 0;

	AnimateLanderWarmup ();
	AnimateLaunch (LanderFrame[5], TRUE);

	if (optSubmenu){
		if(optCustomBorder)
			DrawBorder(DIF_CASE(14, 15, 16), FALSE);
		else
			DrawSubmenu (DIF_CASE(1, 2, 3));
	}

	InitPlanetSide (planetLoc);

	landerInputState.NextTime = GetTimeCounter () + PLANET_SIDE_RATE;
	LandingTakeoffSequence (&landerInputState, TRUE);
	PSD.InTransit = FALSE;

	landerInputState.Initialized = FALSE;
	landerInputState.InputFunc = DoPlanetSide;
	SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
#if defined(ANDROID) || defined(__ANDROID__)
	TFB_SetOnScreenKeyboard_Melee();
	DoInput(&landerInputState, FALSE);
	TFB_SetOnScreenKeyboard_Menu();
#else
	DoInput (&landerInputState, FALSE);
#endif

	if (!(GLOBAL (CurrentActivity) & CHECK_ABORT))
	{
		if (crew_left == 0)
		{
			--GLOBAL_SIS (NumLanders);
			DrawLanders ();

			ReturnToOrbit ();
		}
		else
		{
			PSD.InTransit = TRUE;
			PlaySound (SetAbsSoundIndex (LanderSounds, LANDER_RETURNS),
					NotPositional (), NULL, GAME_SOUND_PRIORITY + 1);

			LandingTakeoffSequence (&landerInputState, FALSE);
			ReturnToOrbit ();
			AnimateLaunch (LanderFrame[6], FALSE);
			
			DeltaSISGauges (crew_left, 0, 0);

			if (PSD.ElementLevel)
			{
				for (index = 0; index < NUM_ELEMENT_CATEGORIES; ++index)
				{
					GLOBAL_SIS (ElementAmounts[index]) +=
							PSD.ElementAmounts[index];
					GLOBAL_SIS (TotalElementMass) +=
							PSD.ElementAmounts[index];
				}
				DrawStorageBays (FALSE);
			}

			GLOBAL_SIS (TotalBioMass) += PSD.BiologicalLevel;
		}
	}

	planetSideDesc = NULL;

	{
		HELEMENT hElement, hNextElement;

		for (hElement = GetHeadElement ();
				hElement; hElement = hNextElement)
		{
			ELEMENT *ElementPtr;

			LockElement (hElement, &ElementPtr);
			hNextElement = _GetSuccLink (ElementPtr);
			if (ElementPtr->state_flags & FINITE_LIFE)
			{
				UnlockElement (hElement);

				RemoveElement (hElement);
				FreeElement (hElement);

				continue;
			}
			UnlockElement (hElement);
		}
	}

	ZeroVelocityComponents (&GLOBAL (velocity));
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
}

void
InitLander (BYTE LanderFlags)
{
	RECT r;

	SetContext (RadarContext);
	BatchGraphics ();
	
	r.corner.x = 0;
	r.corner.y = 0;
	r.extent.width = RADAR_WIDTH;
	r.extent.height = RADAR_HEIGHT;
	SetContextForeGroundColor (BLACK_COLOR);
	DrawFilledRectangle (&r);
	
	if (GLOBAL_SIS (NumLanders) || LanderFlags)
	{
		BYTE ShieldFlags, capacity_shift;
		COUNT free_space;
		STAMP s;

		s.origin.x = 0; /* set up powered-down lander */
		s.origin.y = 0;
		s.frame = SetAbsFrameIndex (LanderFrame[0],
				ANGLE_TO_FACING (FULL_CIRCLE) << 1);
		DrawStamp (&s);
		if (LanderFlags == 0)
		{
			ShieldFlags = GET_GAME_STATE (LANDER_SHIELDS);
			capacity_shift = GET_GAME_STATE (IMPROVED_LANDER_CARGO);
		}
		else
		{
			ShieldFlags = (unsigned char)(LanderFlags &
					((1 << EARTHQUAKE_DISASTER)
					| (1 << BIOLOGICAL_DISASTER)
					| (1 << LIGHTNING_DISASTER)
					| (1 << LAVASPOT_DISASTER)));
			s.frame = IncFrameIndex (s.frame);
			DrawStamp (&s);
			if (LanderFlags & (1 << (4 + 0)))
				s.frame = SetAbsFrameIndex (s.frame, 57);
			else
			{
				s.frame = SetAbsFrameIndex (s.frame,
						(ANGLE_TO_FACING (FULL_CIRCLE) << 1) + 3);
				DrawStamp (&s);
				s.frame = IncFrameIndex (s.frame);
			}
			DrawStamp (&s);
			if (!(LanderFlags & (1 << (4 + 1))))
				capacity_shift = 0;
			else
			{
				capacity_shift = 1;
				s.frame = SetAbsFrameIndex (s.frame, 59);
				DrawStamp (&s);
			}
			if (LanderFlags & (1 << (4 + 2)))
				s.frame = SetAbsFrameIndex (s.frame, 58);
			else
				s.frame = SetAbsFrameIndex (s.frame,
						(ANGLE_TO_FACING (FULL_CIRCLE) << 1) + 2);
			DrawStamp (&s);
		}

		free_space = GetStorageBayCapacity () - GLOBAL_SIS (TotalElementMass);
		if ((int)free_space < (int)(MAX_SCROUNGED << capacity_shift))
		{
			r.corner.x = RES_STAT_SCALE(1);
			r.extent.width = RES_STAT_SCALE(4);
			r.extent.height = RES_STAT_SCALE(MAX_HOLD_BARS - ((free_space >> capacity_shift) * MAX_HOLD_BARS / MAX_SCROUNGED) + 2);
			SetContextForeGroundColor (BLACK_COLOR);
			DrawFilledRectangle (&r);
		}

		s.frame = SetAbsFrameIndex (s.frame, 37);
		if (ShieldFlags & (1 << EARTHQUAKE_DISASTER))
			DrawStamp (&s);
		s.frame = IncFrameIndex (s.frame);
		if (ShieldFlags & (1 << BIOLOGICAL_DISASTER))
			DrawStamp (&s);
		s.frame = IncFrameIndex (s.frame);
		if (ShieldFlags & (1 << LIGHTNING_DISASTER))
			DrawStamp (&s);
		s.frame = IncFrameIndex (s.frame);
		if (ShieldFlags & (1 << LAVASPOT_DISASTER))
			DrawStamp (&s);
	}

	UnbatchGraphics ();
}
