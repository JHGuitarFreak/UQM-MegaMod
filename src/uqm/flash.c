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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// NOTE: A lot of this code is untested. Only highlite and overlay flash
//       areas, drawing directly to the screen, using a cache, are
//       currently in use.

// NOTE:
// - If you change the properties of the original CONTEXT, specifically the
//   dimensions and origin, you'll need to call Flash_preUpdate() before and
//   Flash_postUpdate() after that change. Note that this may change which
//   part of the screen is flashing.

// TODO:
// - During a few frames during the sequence, the frame to be displayed
//   is equal to a frame which was supplied as a parameter to the flash
//   sequence (instead of generated during the sequence). It is not
//   necessary to make a copy in this case. Instead, the original can be
//   used. I had code to do this, but this doesn't work anymore with the
//   addition of startNumer, endNumer, and denom. This code is still
//   present, but disabled with BEGIN_AND_END_FRAME_EXCEPTIONS.

#define FLASH_INTERNAL
#include "flash.h"

#include "libs/log.h"
#include "libs/memlib.h"
#include "libs/threadlib.h"


static FlashContext *Flash_create (CONTEXT gfxContext);
static void Flash_fixState (FlashContext *context);
static void Flash_nextState (FlashContext *context);
static void Flash_clearCache (FlashContext *context);
static void Flash_initCache (FlashContext *context);
static void Flash_grabOriginal (FlashContext *context);
static void Flash_blendFraction (FlashContext *context, int numer, int denom,
		int *resNumer, int *resDenom);
static void Flash_makeFrame (FlashContext *context,
		FRAME dest, int numer, int denom);
static inline void Flash_prepareCacheFrame (FlashContext *context,
		COUNT index);
static void Flash_drawFrame (FlashContext *context, FRAME frame);
static void Flash_drawCacheFrame (FlashContext *context, COUNT index);
static inline void Flash_drawUncachedFrame (FlashContext *context,
		int numer, int denom);
static inline void Flash_drawCachedFrame (FlashContext *context,
		int numer, int denom);
static void Flash_drawCurrentFrame (FlashContext *context);

static CONTEXT workGfxContext;
		// Off-screen internal drawing context

static FlashContext *
Flash_create (CONTEXT gfxContext)
{
	FlashContext *context = HMalloc (sizeof (FlashContext));

	context->gfxContext     = gfxContext;

	context->original       = 0;

	context->startNumer     = 0;
	context->endNumer       = 1;
	context->denom          = 1;

	context->fadeInTime     = Flash_DEFAULT_FADE_IN_TIME;
	context->onTime         = Flash_DEFAULT_ON_TIME;
	context->fadeOutTime    = Flash_DEFAULT_FADE_OUT_TIME;
	context->offTime        = Flash_DEFAULT_OFF_TIME;

	context->frameTime      = 0;

	context->state          = FlashState_off;
	context->lastStateTime  = 0;
	context->lastFrameTime  = 0;
	
	context->started        = false;
	context->paused         = false;

	context->cache          = 0;
	context->cacheSize      = Flash_DEFAULT_CACHE_SIZE;
	
	context->lastFrameIndex = (COUNT) -1;

	// TODO: Delete the context somewhere
	if (!workGfxContext)
		workGfxContext = CreateContext ("Flash.workGfxContext");

	return context;
}

// 'startNumer / denom' is the brightness in the start state of the sequence.
// 'endNumer / denom' is the brightness in the end state of the sequence.
// These numbers are relative to the brighness of the original image.
FlashContext *
Flash_createHighlight (CONTEXT gfxContext, const RECT *rect)
{
	FlashContext *context = Flash_create (gfxContext);

	if (rect == NULL)
	{
		// No rectangle specified. It should be specified later with
		// Flash_setRect(), before calling Flash_start().
		context->rect.corner.x = 0;
		context->rect.corner.y = 0;
		context->rect.extent.width = 0;
		context->rect.extent.height = 0;
	}
	else
		context->rect = *rect;
	context->type = FlashType_highlight;

	return context;
}

FlashContext *
Flash_createTransition (CONTEXT gfxContext, const POINT *origin,
		FRAME first, FRAME final)
{
	FlashContext *context = Flash_create (gfxContext);
	
	context->type = FlashType_transition;

	context->u.transition.first = first;
	context->u.transition.final = final;
	GetFrameRect (final, &context->rect);
	context->rect.corner = *origin;
	
	return context;
}

FlashContext *
Flash_createOverlay (CONTEXT gfxContext, const POINT *origin, FRAME overlay)
{
	FlashContext *context = Flash_create (gfxContext);

	context->type = FlashType_overlay;

	if (origin == NULL || overlay == NULL) {
		// No overlay specified. It should be specified later with
		// Flash_setOverlay(), before calling Flash_start().
		context->u.overlay.frame = NULL;
		context->rect.corner.x = 0;
		context->rect.corner.y = 0;
		context->rect.extent.width = 0;
		context->rect.extent.height = 0;
	} else
		Flash_setOverlay (context, origin, overlay, FALSE);
	
	return context;
}

// Set the current state. 'timeSpentInState' determines how much time should
// be considered to be already spent in this state.
void
Flash_setState (FlashContext *context, FlashState state,
		TimeCount timeSpentInState)
{
	TimeCount now;
	
	now = GetTimeCounter ();

	context->state = state;
	Flash_fixState (context);
	
	context->lastStateTime = now - timeSpentInState;
	context->lastFrameTime = now;

	if (context->started)
		Flash_drawCurrentFrame (context);
}

void
Flash_start (FlashContext *context)
{
	if (context->started)
	{
		log_add (log_Warning, "Flash_start() called on already started "
				"FlashContext.\n");
		return;
	}
	
	Flash_initCache (context);

	context->started = true;
	context->paused = false;

	Flash_grabOriginal (context);
	context->lastFrameIndex = 0;

	Flash_fixState (context);
	Flash_drawCurrentFrame (context);
}

void
Flash_terminate (FlashContext *context)
{
	if (context->started)
	{
		// Restore the flash rectangle:
		Flash_drawFrame (context, context->original);

		Flash_clearCache (context);
		HFree (context->cache);
		DestroyDrawable (ReleaseDrawable (context->original));
	}

	HFree (context);
}

void
Flash_pause (FlashContext *context)
{
	context->paused = true;
}

void
Flash_continue (FlashContext *context)
{
	context->paused = false;
}

// Change the state to the next state as long as the current state has
// a zero-time duration.
static void
Flash_fixState (FlashContext *context)
{
	TimeCount stateTime = 0;
	
	for (;;) {
		switch (context->state) {
			case FlashState_fadeIn:
				stateTime = context->fadeInTime;
				break;
			case FlashState_on:
				stateTime = context->onTime;
				break;
			case FlashState_fadeOut:
				stateTime = context->fadeOutTime;
				break;
			case FlashState_off:
				stateTime = context->offTime;
				break;
		}
		if (stateTime != 0)
			break;
		context->state = (context->state + 1) & 0x3;
	}
}

static void
Flash_nextState (FlashContext *context)
{
	context->state = (context->state + 1) & 0x3;
	Flash_fixState (context);
}

void
Flash_process (FlashContext *context)
{
	TimeCount now;

	if (!context->started || context->paused)
		return;
	
	now = GetTimeCounter ();

	switch (context->state)
	{
		case FlashState_fadeIn:
			if (now >= context->lastStateTime + context->fadeInTime)
			{
				Flash_nextState (context);
				context->lastStateTime = now;
			}
			context->lastFrameTime = now;
			break;
		case FlashState_on:
			if (now < context->lastStateTime + context->onTime)
				return;
			Flash_nextState (context);
			context->lastStateTime = now;
			break;
		case FlashState_fadeOut:
			if (now >= context->lastStateTime + context->fadeOutTime)
			{
				Flash_nextState (context);
				context->lastStateTime = now;
			}
			context->lastFrameTime = now;
			break;
		case FlashState_off:
			if (now < context->lastStateTime + context->offTime)
				return;
			Flash_nextState (context);
			context->lastStateTime = now;
			break;
	}

	Flash_drawCurrentFrame (context);
}

void
Flash_setSpeed (FlashContext *context, TimeCount fadeInTime,
		TimeCount onTime, TimeCount fadeOutTime, TimeCount offTime)
{
	context->fadeInTime = fadeInTime;
	context->onTime = onTime;
	context->fadeOutTime = fadeOutTime;
	context->offTime = offTime;
}

// Determines how the brightness of the flashing changes.
// For highlights:
//   'startNumer / denom' is the brightness, at the start state of the flash.
//   'endNumer / denom' is the brightness, at the end state of the flash.
// For overlays:
//   'startNumer / denom' is the brightness of the image to overlay, at
//       the start state of the flash.
//   'endNumer / denom' is the brightness of the image to overlay, at
//       the end state of the flash.
// For transitions:
//   'startNumer / denom' is the brightness of the second image, at
//       the start state of the flash; '1 - startNumer / denom' is the
//       brightness of the first image at the start state of the flash.
//   'endNumer / denom' is the brightness of the second image, at
//       the end state of the flash; '1 - endNumer / denom' is the
//       brightness of the first image at the end state of the flash.
// These numbers are relative to the brighness of each original image.
void
Flash_setMergeFactors(FlashContext *context, int startNumer, int endNumer,
		int denom) {
	if (context->started)
	{
		Flash_drawFrame (context, context->original);
		Flash_clearCache (context);
	}

	context->startNumer = startNumer;
	context->endNumer = endNumer;
	context->denom = denom;

	if (context->started)
	{
		Flash_grabOriginal (context);
		Flash_drawCurrentFrame (context);
	}
}

// Set the time between updates of the flash area.
void
Flash_setFrameTime (FlashContext *context, TimeCount frameTime)
{
	context->frameTime = frameTime;
}

// Returns the time when the flash area is to be updated.
TimeCount
Flash_nextTime (FlashContext *context)
{
	if (!context->started || context->paused)
		return (TimeCount) -1;

	if (context->state == FlashState_fadeIn ||
			context->state == FlashState_fadeOut)
	{
		// When we're fading in or out, we need updates during
		// the fade.
		return context->lastFrameTime + context->frameTime;
	}
	else
	{
		// When the flash area is completely on or off, we don't
		// need an update until we're ready to change state again.
		if (context->state == FlashState_on)
			return context->lastStateTime + context->onTime;
		else /* context->state == FlashState_off */
			return context->lastStateTime + context->offTime;
	}
}

static void
Flash_clearCache (FlashContext *context)
{
	COUNT i;

#ifdef BEGIN_AND_END_FRAME_EXCEPTIONS
	if (context->type == FlashType_transition ||
			context->type == FlashType_overlay)
	{
		// First frame is not allocated by the flash code, so
		// we shouldn't free it.
		context->cache[0] = (FRAME) 0;
	}
	if (context->type == FlashType_transition)
	{
		// Final frame is not allocated by the flash code, so
		// we shouldn't free it.
		context->cache[context->cacheSize - 1] = (FRAME) 0;
	}
#endif  /* BEGIN_AND_END_FRMAE_EXCEPTIONS */
	
	for (i = 0; i < context->cacheSize; i++)
	{
		if (context->cache[i] != (FRAME) 0)
		{
			DestroyDrawable (ReleaseDrawable (context->cache[i]));
			context->cache[i] = (FRAME) 0;
		}
	}
}

void
Flash_setRect (FlashContext *context, const RECT *rect)
{
	assert(context->type == FlashType_highlight);

	if (context->started)
	{
		Flash_drawFrame (context, context->original);
		Flash_clearCache (context);
	}

	context->rect = *rect;
	context->lastFrameIndex = (COUNT) -1;

	if (context->started)
	{
		Flash_grabOriginal (context);
		Flash_drawCurrentFrame (context);
	}
}

void
Flash_getRect (FlashContext *context, RECT *rect)
{
	assert (!context->type == FlashType_highlight);

	*rect = context->rect;
}

// JMS_GFX: The cleanup boolean can be used when changing between normal and hi-res modes.
// It ensures that an ugly wrong-sized flash overlay from previous resolution is cleaned
// from the flash process.
void
Flash_setOverlay (FlashContext *context, const POINT *origin, FRAME overlay, BOOLEAN cleanup)
{
	assert(context->type == FlashType_overlay);

	if (context->started && !cleanup)
	{
		Flash_drawFrame (context, context->original);
		Flash_clearCache (context);
	}
	
	if (!cleanup)
		context->u.overlay.frame = overlay;

	GetFrameRect (overlay, &context->rect);
	context->rect.corner.x += origin->x;
	context->rect.corner.y += origin->y;

	if (context->started)
	{
		Flash_grabOriginal (context);
		Flash_drawCurrentFrame (context);
	}
}

// Call before you update the graphics in the currently flashing area,
// or before you change the dimensions or origin of the graphics context.
void
Flash_preUpdate (FlashContext *context)
{
	if (context->started)
	{
		Flash_drawFrame (context, context->original);
		Flash_clearCache (context);
	}
}

// Call after you update the graphics in the currently flashing area,
// or after you change the dimensions or origin of the graphics context.
void
Flash_postUpdate (FlashContext *context)
{
	if (context->started)
	{
		Flash_grabOriginal (context);
		Flash_drawCurrentFrame (context);
	}
}

// Pre: context->original has been initialised.
static void
Flash_initCache (FlashContext *context)
{
	COUNT i;

	context->cache = HMalloc (context->cacheSize * sizeof (FRAME));
	for (i = 0; i < context->cacheSize; i++)
		context->cache[i] = (FRAME) 0;
}

void
Flash_setCacheSize (FlashContext *context, COUNT size)
{
	assert (size == 0 || size >= 2);

	if (context->cache != NULL)
	{
		Flash_clearCache (context);
		HFree (context->cache);
		context->cache = NULL;
	}

	context->cacheSize = size;

	if (size != 0)
		Flash_initCache (context);
}

COUNT
Flash_getCacheSize (const FlashContext *context)
{
	return context->cacheSize;
}

static void
Flash_grabOriginal (FlashContext *context)
{
	CONTEXT oldGfxContext;

	if (context->original != (FRAME) 0)
		DestroyDrawable (ReleaseDrawable (context->original));

	oldGfxContext = SetContext (context->gfxContext);
	context->original = CaptureDrawable (CopyContextRect (&context->rect));
	SetContext (oldGfxContext);
	FlushGraphics ();
			// CopyContextRect() may have queued the command to read
			// a rectangle from the screen; a FlushGraphics()
			// is necessary to ensure that it can actually be used.
}

static inline void
Flash_blendFraction (FlashContext *context, int numer, int denom,
		int *resNumer, int *resDenom)
{
	// This function merges two fractions (F0 and F1),
	// based on another fraction (P) (yielding R).
	// F0 = context->u.highlight.startNumer / context->u.highlight.denom
	// F1 = context->u.highlight.endNumer / context->u.highlight.denom
	// P = *numer / *denom
	// R = P * F1 + (1 - P) * F0
	//   = numer * context->endNumer / (denom * context->denom) +
	//     (denom - numer) * startNumer / denom * context->denom
	
	assert (numer >= 0 && numer <= denom);

	*resNumer = numer * context->endNumer +
			(denom - numer) * context->startNumer;
	*resDenom = denom * context->denom;
}

static void
Flash_makeFrame (FlashContext *context, FRAME dest, int numer, int denom)
{
	CONTEXT oldGfxContext;
	STAMP s;
	int blendedNumer;
	int blendedDenom;

	s.origin.x = 0;
	s.origin.y = 0;

	Flash_blendFraction (context, numer, denom, &blendedNumer, &blendedDenom);

	oldGfxContext = SetContext (workGfxContext);
	SetContextFGFrame (dest);

	switch (context->type) {
		case FlashType_highlight:
		{
			// Clear the destination just in case
			SetContextBackGroundColor (BUILD_COLOR_RGBA (0, 0, 0, 0));
			ClearDrawable ();
			// Draw the frame at modulated strength (0 < strength <= 128)
			SetContextDrawMode (MAKE_DRAW_MODE (DRAW_ADDITIVE,
					DRAW_FACTOR_1 * blendedNumer / blendedDenom));
			s.frame = context->original;
			DrawStamp (&s);
			break;
		}
		case FlashType_transition:
		{
			FRAME first;
			FRAME final;

			first = context->u.transition.first;
			if (first == (FRAME) 0)
				first = context->original;
			final = context->u.transition.final;
			if (final == (FRAME) 0)
				final = context->original;

			// Draw the first frame at full strength
			SetContextDrawMode (DRAW_REPLACE_MODE);
			s.frame = first;
			DrawStamp (&s);
			// Merge in the final frame
			SetContextDrawMode (MAKE_DRAW_MODE (DRAW_ALPHA,
					DRAW_FACTOR_1 * blendedNumer / blendedDenom));
			s.frame = final;
			DrawStamp (&s);
			break;
		}
		case FlashType_overlay:
		{
			POINT oldOrigin;

			// Draw the original at full strength
			SetContextDrawMode (DRAW_REPLACE_MODE);
			s.frame = context->original;
			DrawStamp (&s);
			// Add or subtract the overlay at partial strength
			SetContextDrawMode (MAKE_DRAW_MODE (DRAW_ADDITIVE,
					DRAW_FACTOR_1 * blendedNumer / blendedDenom));
			s.frame = context->u.overlay.frame;
			// Offset the draw origin to hit the right area
			oldOrigin = SetContextOrigin (GetFrameHot (s.frame));
			DrawStamp (&s);
			SetContextOrigin (oldOrigin);
			break;
		}
	}

	SetContext (oldGfxContext);
}

// Prepare an entry in the cache.
static inline void
Flash_prepareCacheFrame (FlashContext *context, COUNT index)
{
	if (context->cache[index] != (FRAME) 0)
		return;

#ifdef BEGIN_AND_END_FRAME_EXCEPTIONS
	if (index == 0 && context->type == FlashType_overlay)
		context->cache[index] = context->original;
	else if (index == 0 && context->type == FlashType_transition)
		context->cache[index] = context->u.transition.first != (FRAME) 0 ?
				context->u.transition.first : context->original;
	else if (index == context->cacheSize - 1 &&
			context->type == FlashType_transition)
		context->cache[index] = context->u.transition.final != (FRAME) 0 ?
				context->u.transition.final : context->original;
	else
#endif  /* BEGIN_AND_END_FRMAE_EXCEPTIONS */
	{
		context->cache[index] = CaptureDrawable (CreateDrawable (WANT_PIXMAP,
				context->rect.extent.width, context->rect.extent.height, 1));
		Flash_makeFrame (context, context->cache[index],
				index, context->cacheSize - 1);
	}
}

static void
Flash_drawFrame (FlashContext *context, FRAME frame)
{
	CONTEXT oldGfxContext;
	STAMP stamp;

	oldGfxContext = SetContext (context->gfxContext);

	stamp.origin = context->rect.corner;
	stamp.frame = frame;
	DrawStamp(&stamp);

	SetContext (oldGfxContext);
}

static void
Flash_drawCacheFrame (FlashContext *context, COUNT index)
{
	FRAME frame;

	if (context->lastFrameIndex == index)
		return;

	frame = context->cache[index];
	Flash_drawFrame (context, frame);
	context->lastFrameIndex = index;
}

static inline void
Flash_drawUncachedFrame (FlashContext *context, int numer, int denom)
{
#ifdef BEGIN_AND_END_FRAME_EXCEPTIONS
	// 'lastFrameIndex' is 0 for the first image, 1 for the final
	// image, and 2 otherwise.

	if (numer == 0 && context->type == FlashType_overlay)
	{
		if (context->lastFrameIndex != 0)
			return;

		Flash_drawFrame (context, context->original);
		context->lastFrameIndex = 0;
		return;
	}
	else if (numer == 0 && context->type == FlashType_transition)
	{
		if (context->lastFrameIndex == 0)
			return;

		Flash_drawFrame (context, context->u.transition.first);
		context->lastFrameIndex = 0;
		return;
	}
	else if (numer == denom && context->type == FlashType_transition)
	{
		if (context->lastFrameIndex == 1)
			return;

		Flash_drawFrame (context, context->u.transition.final);
		context->lastFrameIndex = 1;
		return;
	}

	context->lastFrameIndex = 2;
#endif  /* BEGIN_AND_END_FRMAE_EXCEPTIONS */

	{
		// Painting to the screen; we need a temporary frame to draw to.
		FRAME work;

		work = CaptureDrawable (CreateDrawable (WANT_PIXMAP,
				context->rect.extent.width, context->rect.extent.height, 1));
		Flash_makeFrame (context, work, numer, denom);
		Flash_drawFrame (context, work);

		DestroyDrawable (ReleaseDrawable (work));
	}
}

static inline void
Flash_drawCachedFrame (FlashContext *context, int numer, int denom)
{
	COUNT cachePos;

	cachePos = ((context->cacheSize - 1) * numer + (denom / 2)) / denom;
	Flash_prepareCacheFrame (context, cachePos);
	Flash_drawCacheFrame (context, cachePos);
}

static void
Flash_drawCurrentFrame (FlashContext *context)
{
	int numer;
	int denom;

	if (context->state == FlashState_off)
	{
		numer = 0;
		denom = 1;
	}
	else if (context->state == FlashState_on)
	{
		numer = 1;
		denom = 1;
	}
	else
	{
		TimeCount now = GetTimeCounter ();

		if (context->state == FlashState_fadeIn)
			denom = (int) context->fadeInTime;
		else
			denom = (int) context->fadeOutTime;

		numer = (int) (now - context->lastStateTime);

		if (numer > denom)
			numer = denom;

		if (context->state == FlashState_fadeOut)
			numer = (int) context->fadeOutTime - numer;
	}

	if (context->cacheSize == 0)
		Flash_drawUncachedFrame (context, numer, denom);
	else
		Flash_drawCachedFrame (context, numer, denom);
}

