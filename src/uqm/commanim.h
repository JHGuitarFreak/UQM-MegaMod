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

#ifndef UQM_COMMANIM_H_
#define UQM_COMMANIM_H_

#include "libs/compiler.h"
#include "libs/gfxlib.h"

#if defined(__cplusplus)
extern "C" {
#endif

// Some background: every animation has a neutral frame which returns
// the image to the state it was in before the animation began. Which
// frame is neutral depends on the animation type.
// Animation types:
#define RANDOM_ANIM (1 << 0)
		// The next index is randomly chosen.
		// The first frame is neutral
#define CIRCULAR_ANIM (1 << 1)
		// After the last index has been reached, the animation starts over.
		// The last frame is neutral. This complicates the animation task.
#define YOYO_ANIM (1 << 2)
		// After the last index has been reached, the order that the
		// animation frames are used is reversed.
		// The first frame is neutral
#define ANIM_MASK (RANDOM_ANIM | CIRCULAR_ANIM | YOYO_ANIM)
		// Mask of all animation types.
		// Static frames do not have any of these flags set.

#define WAIT_TALKING (1 << 3)
		// This is set in AlienTalkDesc when the talking animation is active
		// or should be active.
		// In AlienAmbientArray, this is set for those ambient animations
		// which can not be active while the talking animation is active.
		// Such animations stop at the end of the current animation cycle
		// when the talking animation activates.
#define PAUSE_TALKING (1 << 4)
		// Set in AlienTalkDesc when we do not want the talking animation
#define TALK_INTRO (1 << 5)
		// In AlienTransitionDesc: indicates a transition to talking state
#define TALK_DONE (1 << 6)
		// In AlienTransitionDesc: indicates a transition to silent state
		// In AlienTalkDesc: signals the end of talking animation
#define WHEN_TALKING (1L << 7) // JMS
#define ANIM_DISABLED (1L << 8) // BW (needed for news anchor and animated background)

#define FAST_STOP_AT_TALK_START (TALK_DONE) // JMS: If there's a very loooong animation, it can be forced to stop when talking with this.
// (otherwise there'll be nasty, unwanted pauses in the conversation.) 

#define COLORXFORM_ANIM PAUSE_TALKING

#define ONE_SHOT_ANIM  TALK_INTRO
		// Set in AlienAmbientArray for animations that should be
		// disabled after they run once.

typedef struct
{
	COUNT StartIndex;
			// Index of the first image (for image animation) or
			// index of the first color map (for palette animation)
	BYTE NumFrames;
			// Number of frames in the animation.

	COUNT AnimFlags;
			// One of RANDOM_ANIM, CIRCULAR_ANIM, or YOYO_ANIM
			// plus flags (WAIT_TALKING, ANIM_DISABLED)
			// JMS: Changed from BYTE to COUNT to house more possible flags

	COUNT BaseFrameRate;
			// Minimum interframe delay
	COUNT RandomFrameRate;
			// Maximum additional interframe delay
			// Actual delay: BaseFrameRate + Random(0..RandomFrameRate)
	COUNT BaseRestartRate;
			// Minimum delay before restarting animation
	COUNT RandomRestartRate;
			// Maximum additional delay before restarting animation
			// Actual delay: BaseRestartRate + Random(0..RandomRestartRate)

	DWORD BlockMask;
			// Bit mask of the indices of all animations that can not
			// be active at the same time as this animation, usually,
			// due to the image overlap conflicts.
} ANIMATION_DESC;

#define MAX_ANIMATIONS 30 // JMS: Was 20


#ifdef COMM_INTERNAL

typedef enum
{
	DOWN_DIR = -1, // Animation indices are decreasing
	NO_DIR = 0,
	UP_DIR = 1,    // Animation indices are increasing
} ANIM_DIR;

typedef enum
{
	PICTURE_ANIM,
			// Parts of a picture are replaced
	COLOR_ANIM
			// Colormap tricks on a picture
} ANIM_TYPE;

// Describes an active animation.
struct SEQUENCE
{
	ANIMATION_DESC *ADPtr;
	DWORD Alarm;
	ANIM_DIR Direction;
	COUNT CurIndex;
	COUNT NextIndex;
	COUNT FramesLeft;
	ANIM_TYPE AnimType;
	BOOLEAN Change;
};
#endif

typedef struct SEQUENCE SEQUENCE;

// Returns TRUE if there was an animation change
extern BOOLEAN DrawAlienFrame (SEQUENCE *pSeq, COUNT Num, BOOLEAN fullRedraw);
extern void InitCommAnimations (void);
extern BOOLEAN ProcessCommAnimations (BOOLEAN fullRedraw, BOOLEAN paused);

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_COMMANIM_H_ */


