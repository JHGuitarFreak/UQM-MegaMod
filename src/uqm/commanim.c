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

#define COMM_INTERNAL
#include "commanim.h"

#include "comm.h"
#include "element.h"
#include "setup.h"
#include "libs/compiler.h"
#include "libs/graphics/cmap.h"
#include "libs/mathlib.h"


static TimeCount LastTime;
static SEQUENCE Sequences[MAX_ANIMATIONS + 2];
		// 2 extra for Talk and Transition animations
static DWORD ActiveMask;
		// Bit mask of all animations that are currently active.
		// Bit 'i' is set if the animation with index 'i' is active.
static ANIMATION_DESC TalkDesc;
static ANIMATION_DESC TransitDesc;
static SEQUENCE* Talk;
static SEQUENCE* Transit;
static COUNT FirstAmbient;
static COUNT TotalSequences;


static inline DWORD
randomFrameRate (SEQUENCE *pSeq)
{
	ANIMATION_DESC *ADPtr = pSeq->ADPtr;

	return ADPtr->BaseFrameRate	+
			TFB_Random () % (ADPtr->RandomFrameRate + 1);
}

static inline DWORD
randomRestartRate (SEQUENCE *pSeq)
{
	ANIMATION_DESC *ADPtr = pSeq->ADPtr;

	return ADPtr->BaseRestartRate +
			TFB_Random () % (ADPtr->RandomRestartRate + 1);
}

static inline COUNT
randomFrameIndex (SEQUENCE *pSeq, COUNT from)
{
	ANIMATION_DESC *ADPtr = pSeq->ADPtr;

	return from	+ TFB_Random () % (ADPtr->NumFrames - from);
}

static void
SetupAmbientSequences (SEQUENCE *pSeq, COUNT Num)
{
	COUNT i;
	
	for (i = 0; i < Num; ++i, ++pSeq)
	{
		ANIMATION_DESC *ADPtr = &CommData.AlienAmbientArray[i];

		memset (pSeq, 0, sizeof (*pSeq));

		pSeq->ADPtr = ADPtr;
		if (ADPtr->AnimFlags & COLORXFORM_ANIM)
			pSeq->AnimType = COLOR_ANIM;
		else
			pSeq->AnimType = PICTURE_ANIM;
		pSeq->Direction = UP_DIR;
		pSeq->FramesLeft = ADPtr->NumFrames;
		// Default: first frame is neutral
		if (ADPtr->AnimFlags & RANDOM_ANIM)
		{	// Set a random frame/colormap
			pSeq->NextIndex = TFB_Random () % ADPtr->NumFrames;
		}
		else if (ADPtr->AnimFlags & YOYO_ANIM)
		{	// Skip the first frame/colormap (it's neutral)
			pSeq->NextIndex = 1;
			--pSeq->FramesLeft;
		}
		else if (ADPtr->AnimFlags & CIRCULAR_ANIM)
		{	// Exception that makes everything more painful:
			// *Last* frame is neutral
			pSeq->CurIndex = ADPtr->NumFrames - 1;
			pSeq->NextIndex = 0;
		}

		pSeq->Alarm = randomRestartRate (pSeq) + 1;
	}
}

static void
SetupTalkSequence (SEQUENCE *pSeq, ANIMATION_DESC *ADPtr)
{
	memset (pSeq, 0, sizeof (*pSeq));
	// Initially disabled, and until needed
	ADPtr->AnimFlags |= ANIM_DISABLED;
	pSeq->ADPtr = ADPtr;
	pSeq->AnimType = PICTURE_ANIM;
}

static inline BOOLEAN
animAtNeutralIndex (SEQUENCE *pSeq)
{
	ANIMATION_DESC *ADPtr = pSeq->ADPtr;

	if (ADPtr->AnimFlags & CIRCULAR_ANIM)
	{	// CIRCULAR_ANIM's neutral frame is the last
		return pSeq->NextIndex == 0;
	}
	else
	{	// All others, neutral frame is the first
		return pSeq->CurIndex == 0;
	}
}

static inline BOOLEAN
conflictsWithTalkingAnim (SEQUENCE *pSeq)
{
	ANIMATION_DESC *ADPtr = pSeq->ADPtr;

	return ADPtr->AnimFlags & CommData.AlienTalkDesc.AnimFlags & WAIT_TALKING;
}

static void
ProcessColormapAnims (SEQUENCE *pSeq, COUNT Num)
{
	COUNT i;

	for (i = 0; i < Num; ++i, ++pSeq)
	{
		ANIMATION_DESC *ADPtr = pSeq->ADPtr;

		if ((ADPtr->AnimFlags & ANIM_DISABLED)
				|| pSeq->AnimType != COLOR_ANIM
				|| !pSeq->Change)
			continue;

		XFormColorMap (GetColorMapAddress (
				SetAbsColorMapIndex (CommData.AlienColorMap,
				ADPtr->StartIndex + pSeq->CurIndex)),
				pSeq->Alarm - 1);
		pSeq->Change = FALSE;
	}
}

static BOOLEAN
AdvanceAmbientSequence (SEQUENCE *pSeq)
{
	BOOLEAN active;
	ANIMATION_DESC *ADPtr = pSeq->ADPtr;

	--pSeq->FramesLeft;
	// YOYO_ANIM does not actually end until it comes back
	// in reverse direction, even if FramesLeft gets to 0 here
	if (pSeq->FramesLeft
			|| ((ADPtr->AnimFlags & YOYO_ANIM) && pSeq->NextIndex != 0))
	{
		active = TRUE;
		pSeq->Alarm = randomFrameRate (pSeq) + 1;
	}
	else
	{	// last animation frame
		active = FALSE;
		pSeq->Alarm = randomRestartRate (pSeq) + 1;

		// RANDOM_ANIM must end on a neutral frame
		if (ADPtr->AnimFlags & RANDOM_ANIM)
			pSeq->NextIndex = 0;
	}

	// Will draw the next frame or change to next colormap
	pSeq->CurIndex = pSeq->NextIndex;
	pSeq->Change = TRUE;

	if (pSeq->FramesLeft == 0)
	{	// Animation ended
		// Set it up for the next round
		pSeq->FramesLeft = ADPtr->NumFrames;

		if (ADPtr->AnimFlags & YOYO_ANIM)
		{	// YOYO_ANIM never draws the first frame
			// ("first" depends on direction)
			--pSeq->FramesLeft;
			pSeq->Direction = -pSeq->Direction;
		}
		else if (ADPtr->AnimFlags & CIRCULAR_ANIM)
		{	// Rewind the CIRCULAR_ANIM
			// NextIndex will be brought to 0 just below
			pSeq->NextIndex = -1;
		}
		// RANDOM_ANIM is setup just below
	}

	if (ADPtr->AnimFlags & RANDOM_ANIM)
		pSeq->NextIndex = randomFrameIndex (pSeq, 0);
	else
		pSeq->NextIndex += pSeq->Direction;

	return active;
}

static void
ResetSequence (SEQUENCE *pSeq)
{
	// Reset the animation and cause a redraw of the neutral frame,
	// assuming it is not ANIM_DISABLED
	// NOTE: This does not handle CIRCULAR_ANIM properly
	pSeq->Direction = NO_DIR;
	pSeq->CurIndex = 0;
	pSeq->Change = TRUE;
}

static void
AdvanceTalkingSequence (SEQUENCE *pSeq, DWORD ElapsedTicks)
{
	// We use the actual descriptor for flags processing and
	// a copied one for drawing. A copied one is updated only
	// when it is safe to do so.
	ANIMATION_DESC *ADPtr = pSeq->ADPtr;
	
	if (pSeq->Direction == NO_DIR)
	{	// just starting now
		pSeq->Direction = UP_DIR;
		// It's now safe to pick up new Talk descriptor if changed
		// (e.g. Zoq and Pik taking turns to talk)
		if (CommData.AlienTalkDesc.StartIndex != ADPtr->StartIndex)
		{	// copy the new one
			*ADPtr = CommData.AlienTalkDesc;
		}

		assert (pSeq->CurIndex == 0);
		pSeq->Alarm = 0; // now!
		ADPtr->AnimFlags &= ~ANIM_DISABLED;
	}

	if (pSeq->Alarm > ElapsedTicks)
	{	// Not time yet
		pSeq->Alarm -= ElapsedTicks;
		return;
	}

	// Time to start or advance the animation
	pSeq->Alarm = randomFrameRate (pSeq);
	pSeq->Change = TRUE;
	// Talking animation is like RANDOM_ANIM, except that
	// random frames always alternate with the neutral one
	// The animation does not stop until we reset it
	if (pSeq->CurIndex == 0)
	{	// random frame next
		pSeq->CurIndex = randomFrameIndex (pSeq, 1);
		pSeq->Alarm += randomRestartRate (pSeq);
	}
	else
	{	// neutral frame next
		pSeq->CurIndex = 0;
	}
}

static BOOLEAN
AdvanceTransitSequence (SEQUENCE *pSeq, DWORD ElapsedTicks)
{
	BOOLEAN done = FALSE;
	// We use the actual descriptor for flags processing and
	// a copied one for drawing. A copied one is updated only
	// when it is safe to do so.
	ANIMATION_DESC *ADPtr = pSeq->ADPtr;

	if (pSeq->Direction == NO_DIR)
	{	// just starting now
		pSeq->Alarm = 0; // now!
		ADPtr->AnimFlags &= ~ANIM_DISABLED;
	}

	if (pSeq->Alarm > ElapsedTicks)
	{	// Not time yet
		pSeq->Alarm -= ElapsedTicks;
		return FALSE;
	}

	// Time to start or advance the animation
	pSeq->Change = TRUE;

	if (pSeq->Direction == NO_DIR)
	{	// just starting now
		pSeq->FramesLeft = ADPtr->NumFrames;
		// Both INTRO and DONE may be set at the same time,
		// when e.g. Zoq and Pik are taking turns to talk
		// Process the DONE transition first to go into
		// a neutral state before switching over.
		if (CommData.AlienTransitionDesc.AnimFlags & TALK_DONE)
		{
			pSeq->Direction = DOWN_DIR;
			pSeq->CurIndex = ADPtr->NumFrames - 1;
		}
		else if (CommData.AlienTransitionDesc.AnimFlags & TALK_INTRO)
		{
			pSeq->Direction = UP_DIR;
			// It's now safe to pick up new Transition descriptor if changed
			// (e.g. Zoq and Pik taking turns to talk)
			if (CommData.AlienTransitionDesc.StartIndex
					!= ADPtr->StartIndex)
			{	// copy the new one
				*ADPtr = CommData.AlienTransitionDesc;
			}
			
			pSeq->CurIndex = 0;
		}
	}

	--pSeq->FramesLeft;
	if (pSeq->FramesLeft == 0)
	{	// animation is done
		if (pSeq->Direction == UP_DIR)
		{	// done with TALK_INTRO transition
			CommData.AlienTransitionDesc.AnimFlags &= ~TALK_INTRO;
		}
		else if (pSeq->Direction == DOWN_DIR)
		{	// done with TALK_DONE transition
			CommData.AlienTransitionDesc.AnimFlags &= ~TALK_DONE;

			// Done with all transition frames
			ADPtr->AnimFlags |= ANIM_DISABLED;
			done = TRUE;
		}
		pSeq->Direction = NO_DIR;
	}
	else
	{	// next frame
		pSeq->Alarm = randomFrameRate (pSeq);
		pSeq->CurIndex += pSeq->Direction;
	}

	return done;
}

void
InitCommAnimations (void)
{
	ActiveMask = 0;

	TalkDesc = CommData.AlienTalkDesc;
	TransitDesc = CommData.AlienTransitionDesc;

	// Animation sequences have to be drawn in reverse, and
	// talk animations have to be drawn last (so we add them first)
	TotalSequences = 0;
	// Transition animation last
	Transit = Sequences + TotalSequences;
	SetupTalkSequence (Transit, &TransitDesc);
	++TotalSequences;
	// Talk animation second last
	Talk = Sequences + TotalSequences;
	SetupTalkSequence (Talk, &TalkDesc);
	++TotalSequences;
	FirstAmbient = TotalSequences;
	SetupAmbientSequences (Sequences + FirstAmbient, CommData.NumAnimations);
	TotalSequences += CommData.NumAnimations;

	LastTime = GetTimeCounter ();
}

BOOLEAN
ProcessCommAnimations (BOOLEAN FullRedraw, BOOLEAN paused)
{
	if (paused)
	{	// Drive colormap xforms and nothing else
		XFormColorMap_step ();
		return FALSE;
	}
	else
	{
		COUNT i;
		SEQUENCE *pSeq;
		BOOLEAN Change;
		BOOLEAN CanTalk = TRUE;
		TimeCount CurTime;
		DWORD ElapsedTicks;
		DWORD NextActiveMask;

		CurTime = GetTimeCounter ();
		ElapsedTicks = CurTime - LastTime;
		LastTime = CurTime;

		// Process ambient animations
		NextActiveMask = ActiveMask;
		pSeq = Sequences + FirstAmbient;
		for (i = 0; i < CommData.NumAnimations; ++i, ++pSeq)
		{
			ANIMATION_DESC *ADPtr = pSeq->ADPtr;
			DWORD ActiveBit = 1L << i;

			if (ADPtr->AnimFlags & ANIM_DISABLED)
				continue;
			
			if (pSeq->Direction == NO_DIR)
			{	// animation is paused
				if (!conflictsWithTalkingAnim (pSeq))
				{	// start it up
					pSeq->Direction = UP_DIR;
				}
			}
			else if (pSeq->Alarm > ElapsedTicks)
			{	// not time yet
				pSeq->Alarm -= ElapsedTicks;
			}
			else if (ActiveMask & ADPtr->BlockMask)
			{	// animation is blocked
				assert (!(ActiveMask & ActiveBit) &&
						"Check animations' mutual blocking masks");
				assert (animAtNeutralIndex (pSeq));
				// reschedule
				pSeq->Alarm = randomRestartRate (pSeq) + 1;
				continue;
			}
			else
			{	// Time to start or advance the animation
				if (AdvanceAmbientSequence (pSeq))
				{	// Animation is active this frame and the next
					ActiveMask |= ActiveBit;
					NextActiveMask |= ActiveBit;
				}
				else
				{	// Animation remains active this frame but not the next
					// This keeps any conflicting animations (BlockMask)
					// from activating in the same frame and scribbling over
					// our last image.
					NextActiveMask &= ~ActiveBit;
				}
			}

			if (pSeq->AnimType == PICTURE_ANIM && pSeq->Direction != NO_DIR
					&& conflictsWithTalkingAnim (pSeq))
			{
				// We want to talk, but this is a running picture animation
				// which conflicts with the talking animation
				// See if it is safe to stop it now.
				if (animAtNeutralIndex (pSeq))
				{	// pause the animation
					pSeq->Direction = NO_DIR;
					NextActiveMask &= ~ActiveBit;
					// Talk animation is drawn last, so it's not a conflict
					// for this frame. The talk animation will be drawn
					// over the neutral frame.
				}
				else
				{	// Otherwise, let the animation run until it's safe
					CanTalk = FALSE;
				}
			}			
			
			// BW: to be checked. I've tried to remove what's supposed to be removed while keeping the Syreen zoom-in feature.
			// It may have to be re-programmed in the new commanim style.
			if (pSeq->AnimType == PICTURE_ANIM
				&& (ADPtr->AnimFlags & CommData.AlienTalkDesc.AnimFlags & WAIT_TALKING)
				&& pSeq->Direction != NO_DIR)
			{
				// JMS: Cut marked animations short when starting talk.
				// The animations are marked with FAST_STOP_AT_TALK_START in the races' comm source codes.
				if (ADPtr->AnimFlags & FAST_STOP_AT_TALK_START)
				{	CanTalk = TRUE;
					//pSeq->AnimObj.CurFrame = SetAbsFrameIndex(pSeq->AnimObj.CurFrame, ADPtr->StartIndex);
					pSeq->Direction = NO_DIR;
				}
			}
			
			// JMS: This handles ambient animations which should occur only during talk
			// A lot of conditions are necessary to eliminate unwanted animations
			// from the duration of talk transition!
			if (pSeq->AnimType == PICTURE_ANIM
				&& ADPtr->AnimFlags & WHEN_TALKING 
				&& (!(CommData.AlienTalkDesc.AnimFlags & WAIT_TALKING) 
					|| (CommData.AlienTalkDesc.AnimFlags & TALK_INTRO)
					|| (CommData.AlienTalkDesc.AnimFlags & TALK_DONE))
				&& !(CommData.AlienTransitionDesc.AnimFlags & PAUSE_TALKING)
				&& pSeq->Direction != NO_DIR)
			{
				// Stop the anim if not talking
				pSeq->Direction = NO_DIR;
			}
		}
		// All ambient animations have been processed. Advance the mask.
		ActiveMask = NextActiveMask;

		// Process the talking and transition animations
		if (CanTalk	&& haveTalkingAnim () && runningTalkingAnim ())
		{
			BOOLEAN done = FALSE;

			if (signaledStopTalkingAnim () && haveTransitionAnim ())
			{	// Run the transition. We will clear everything
				// when it is done
				CommData.AlienTransitionDesc.AnimFlags |= TALK_DONE;
			}

			if (CommData.AlienTransitionDesc.AnimFlags
					& (TALK_INTRO | TALK_DONE))
			{	// Transitioning in or out of talking
				if ((CommData.AlienTransitionDesc.AnimFlags & TALK_DONE)
						&& Transit->Direction == NO_DIR)
				{	// This is needed when switching talking anims
					ResetSequence (Talk);
				}
				done = AdvanceTransitSequence (Transit, ElapsedTicks);
			}
			else if (!signaledStopTalkingAnim ())
			{	// Talking, transition is done
				AdvanceTalkingSequence (Talk, ElapsedTicks);
			}
			else
			{	// Not talking
				ResetSequence (Talk);
				done = TRUE;
			}

			if (signaledStopTalkingAnim () && done)
			{
				clearRunTalkingAnim ();
				clearStopTalkingAnim ();
			}
		}
		else
		{	// Not talking -- disable talking anim if it is done
			if (Talk->Direction == NO_DIR)
				TalkDesc.AnimFlags |= ANIM_DISABLED;
		}

		BatchGraphics ();

		// Draw all animations
		{
			BOOLEAN ColorChange = XFormColorMap_step ();

			if (ColorChange)
				FullRedraw = TRUE;

			// Colormap animations are processed separately
			// from picture anims (see XFormColorMap_step)
			ProcessColormapAnims (Sequences + FirstAmbient,
					CommData.NumAnimations);

			Change = DrawAlienFrame (Sequences, TotalSequences, FullRedraw);
			if (FullRedraw)
				Change = TRUE;
		}
		
		UnbatchGraphics ();

		// Post-process ambient animations
		pSeq = Sequences + FirstAmbient;
		for (i = 0; i < CommData.NumAnimations; ++i, ++pSeq)
		{
			ANIMATION_DESC *ADPtr = pSeq->ADPtr;
			DWORD ActiveBit = 1L << i;

			if (ADPtr->AnimFlags & ANIM_DISABLED)
				continue;

			// We can only disable a one-shot anim here, otherwise the
			// last frame will not be drawn
			if ((ADPtr->AnimFlags & ONE_SHOT_ANIM)
					&& !(NextActiveMask & ActiveBit))
			{	// One-shot animation, inactive next frame
				ADPtr->AnimFlags |= ANIM_DISABLED;
			}
		}

		return Change;
	}
}

BOOLEAN
DrawAlienFrame (SEQUENCE *Sequences, COUNT Num, BOOLEAN fullRedraw)
{
	int i;
	STAMP s;
	BOOLEAN Change = FALSE;

	BatchGraphics ();

	s.origin.x = 0;
	s.origin.y = 0;
	
	if (fullRedraw)
	{
		// Draw the main frame
		s.frame = CommData.AlienFrame;
		DrawStamp (&s);

		// Draw any static frames (has to be in reverse)
		for (i = CommData.NumAnimations - 1; i >= 0; --i)
		{
			ANIMATION_DESC *ADPtr = &CommData.AlienAmbientArray[i];

			if (ADPtr->AnimFlags & ANIM_MASK)
				continue;

			ADPtr->AnimFlags |= ANIM_DISABLED;

			if (!(ADPtr->AnimFlags & COLORXFORM_ANIM))
			{	// It's a static frame (e.g. Flagship picture at Starbase)
				s.frame = SetAbsFrameIndex (CommData.AlienFrame,
						ADPtr->StartIndex);
				DrawStamp (&s);
			}
		}
	}

	if (Sequences)
	{	// Draw the animation sequences (has to be in reverse)
		for (i = Num - 1; i >= 0; --i)
		{
			SEQUENCE *pSeq = &Sequences[i];
			ANIMATION_DESC *ADPtr = pSeq->ADPtr;

			if ((ADPtr->AnimFlags & ANIM_DISABLED)
					|| pSeq->AnimType != PICTURE_ANIM)
				continue;

			// Draw current animation frame only if changed
			if (!fullRedraw && !pSeq->Change)
				continue;

			s.frame = SetAbsFrameIndex (CommData.AlienFrame,
					ADPtr->StartIndex + pSeq->CurIndex);
			DrawStamp (&s);
			pSeq->Change = FALSE;

			Change = TRUE;
		}
	}

	UnbatchGraphics ();

	return Change;
}
