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

#include "port.h"
#include "libs/threadlib.h"
#include "libs/graphics/drawcmd.h"
#include "libs/graphics/drawable.h"
#include "libs/graphics/context.h"
#include "libs/graphics/dcqueue.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/bbox.h"
#include "libs/timelib.h"
#include "libs/log.h"
#include "libs/misc.h"
		// for TFB_DEBUG_HALT
#include "options.h"

static RecursiveMutex DCQ_Mutex;

CondVar RenderingCond;

TFB_DrawCommand DCQ[DCQ_MAX];

TFB_DrawCommandQueue DrawCommandQueue;

#define FPS_PERIOD  (ONE_SECOND / 100)
int RenderedFrames = 0;


// Wait for the queue to be emptied.
static void
TFB_WaitForSpace (int requested_slots)
{
	int old_depth, i;
	log_add (log_Debug, "DCQ overload (Size = %d, FullSize = %d, "
			"Requested = %d).  Sleeping until renderer is done.",
			DrawCommandQueue.Size, DrawCommandQueue.FullSize,
			requested_slots);
	// Restore the DCQ locking level.  I *think* this is
	// always 1, but...
	TFB_BatchReset ();
	old_depth = GetRecursiveMutexDepth (DCQ_Mutex);
	for (i = 0; i < old_depth; i++)
		UnlockRecursiveMutex (DCQ_Mutex);
	WaitCondVar (RenderingCond);
	for (i = 0; i < old_depth; i++)
		LockRecursiveMutex (DCQ_Mutex);
	log_add (log_Debug, "DCQ clear (Size = %d, FullSize = %d).  Continuing.",
			DrawCommandQueue.Size, DrawCommandQueue.FullSize);
}

void
Lock_DCQ (int slots)
{
	LockRecursiveMutex (DCQ_Mutex);
	while (DrawCommandQueue.FullSize >= DCQ_MAX - slots)
	{
		TFB_WaitForSpace (slots);
	}
}

void
Unlock_DCQ (void)
{
	UnlockRecursiveMutex (DCQ_Mutex);
}

// Always have the DCQ locked when calling this.
static void
Synchronize_DCQ (void)
{
	if (!DrawCommandQueue.Batching)
	{
		int front = DrawCommandQueue.Front;
		int back  = DrawCommandQueue.InsertionPoint;
		DrawCommandQueue.Back = DrawCommandQueue.InsertionPoint;
		if (front <= back)
		{
			DrawCommandQueue.Size = (back - front);
		}
		else
		{
			DrawCommandQueue.Size = (back + DCQ_MAX - front);
		}
		DrawCommandQueue.FullSize = DrawCommandQueue.Size;
	}
}

void
TFB_BatchGraphics (void)
{
	LockRecursiveMutex (DCQ_Mutex);
	DrawCommandQueue.Batching++;
	UnlockRecursiveMutex (DCQ_Mutex);
}

void
TFB_UnbatchGraphics (void)
{	
	LockRecursiveMutex (DCQ_Mutex);
	if (DrawCommandQueue.Batching)
	{
		DrawCommandQueue.Batching--;
	}
	Synchronize_DCQ ();
	UnlockRecursiveMutex (DCQ_Mutex);
}

// Cancel all pending batch operations, making them unbatched.  This will
// cause a small amount of flicker when invoked, but prevents 
// batching problems from freezing the game.
void
TFB_BatchReset (void)
{
	LockRecursiveMutex (DCQ_Mutex);
	DrawCommandQueue.Batching = 0;
	Synchronize_DCQ ();
	UnlockRecursiveMutex (DCQ_Mutex);
}


// Draw Command Queue Stuff

void
Init_DrawCommandQueue (void)
{
	DrawCommandQueue.Back = 0;
	DrawCommandQueue.Front = 0;
	DrawCommandQueue.InsertionPoint = 0;
	DrawCommandQueue.Batching = 0;
	DrawCommandQueue.FullSize = 0;
	DrawCommandQueue.Size = 0;

	TFB_BBox_Init (ScreenWidth, ScreenHeight);

	DCQ_Mutex = CreateRecursiveMutex ("DCQ",
			SYNC_CLASS_TOPLEVEL | SYNC_CLASS_VIDEO);

	RenderingCond = CreateCondVar ("DCQ empty",
			SYNC_CLASS_TOPLEVEL | SYNC_CLASS_VIDEO);
}

void
Uninit_DrawCommandQueue (void)
{
	if (RenderingCond)
	{
		DestroyCondVar (RenderingCond);
		RenderingCond = 0;
	}

	if (DCQ_Mutex)
	{
		DestroyRecursiveMutex (DCQ_Mutex);
		DCQ_Mutex = 0;
	}
}

void
TFB_DrawCommandQueue_Push (TFB_DrawCommand* Command)
{
	Lock_DCQ (1);
	DCQ[DrawCommandQueue.InsertionPoint] = *Command;
	DrawCommandQueue.InsertionPoint = (DrawCommandQueue.InsertionPoint + 1)
			% DCQ_MAX;
	DrawCommandQueue.FullSize++;
	Synchronize_DCQ ();
	Unlock_DCQ ();
}

int
TFB_DrawCommandQueue_Pop (TFB_DrawCommand *target)
{
	LockRecursiveMutex (DCQ_Mutex);

	if (DrawCommandQueue.Size == 0)
	{
		Unlock_DCQ ();
		return (0);
	}

	if (DrawCommandQueue.Front == DrawCommandQueue.Back &&
			DrawCommandQueue.Size != DCQ_MAX)
	{
		log_add (log_Debug, "Augh!  Assertion failure in DCQ!  "
				"Front == Back, Size != DCQ_MAX");
		DrawCommandQueue.Size = 0;
		Unlock_DCQ ();
		return (0);
	}

	*target = DCQ[DrawCommandQueue.Front];
	DrawCommandQueue.Front = (DrawCommandQueue.Front + 1) % DCQ_MAX;

	DrawCommandQueue.Size--;
	DrawCommandQueue.FullSize--;
	UnlockRecursiveMutex (DCQ_Mutex);

	return 1;
}

void
TFB_DrawCommandQueue_Clear ()
{
	LockRecursiveMutex (DCQ_Mutex);
	DrawCommandQueue.Size = 0;
	DrawCommandQueue.Front = 0;
	DrawCommandQueue.Back = 0;
	DrawCommandQueue.Batching = 0;
	DrawCommandQueue.FullSize = 0;
	DrawCommandQueue.InsertionPoint = 0;
	UnlockRecursiveMutex (DCQ_Mutex);
}

static void
checkExclusiveThread (TFB_DrawCommand* DrawCommand)
{
#ifdef DEBUG_DCQ_THREADS
	static uint32 exclusiveThreadId;
	extern uint32 SDL_ThreadID(void);

	// Only one thread is currently allowed to enqueue commands
	// This is not a technical limitation but rather a semantical one atm.
	if (DrawCommand->Type == TFB_DRAWCOMMANDTYPE_REINITVIDEO)
	{	// TFB_DRAWCOMMANDTYPE_REINITVIDEO is an exception
		// It is queued from the main() thread, which is safe to do
		return;
	}
	
	if (!exclusiveThreadId)
		exclusiveThreadId = SDL_ThreadID();
	else
		assert (SDL_ThreadID() == exclusiveThreadId);
#else
	(void) DrawCommand; // suppress unused warning
#endif
}

void
TFB_EnqueueDrawCommand (TFB_DrawCommand* DrawCommand)
{
	if (TFB_DEBUG_HALT)
	{
		return;
	}

	checkExclusiveThread (DrawCommand);

	if (DrawCommand->Type <= TFB_DRAWCOMMANDTYPE_COPYTOIMAGE
			&& _CurFramePtr->Type == SCREEN_DRAWABLE)
	{
		static RECT scissor_rect;

		// Set the clipping region.
		// We allow drawing with no current context set, so the whole screen
		if ((_pCurContext && !rectsEqual (scissor_rect, _pCurContext->ClipRect))
				|| (!_pCurContext && scissor_rect.extent.width != 0))
		{
			// Enqueue command to set the glScissor spec
			TFB_DrawCommand DC;

			if (_pCurContext)
				scissor_rect = _pCurContext->ClipRect;
			else
				scissor_rect.extent.width = 0;

			if (scissor_rect.extent.width)
			{
				DC.Type = TFB_DRAWCOMMANDTYPE_SCISSORENABLE;
				DC.data.scissor.rect = scissor_rect;
			}
			else
			{
				DC.Type = TFB_DRAWCOMMANDTYPE_SCISSORDISABLE;
			}
				
			TFB_EnqueueDrawCommand(&DC);
		}
	}

	TFB_DrawCommandQueue_Push (DrawCommand);
}

static void
computeFPS (void)
{
	static TimeCount last_time;
	static TimePeriod fps_counter;
	TimeCount current_time;
	TimePeriod delta_time;

	current_time = GetTimeCounter ();
	delta_time = current_time - last_time;
	last_time = current_time;
	
	fps_counter += delta_time;
	if (fps_counter > FPS_PERIOD)
	{
		log_add (log_User, "fps %.2f, effective %.2f",
				(float)ONE_SECOND / delta_time,
				(float)ONE_SECOND * RenderedFrames / fps_counter);

		fps_counter = 0;
		RenderedFrames = 0;
	}
}

// Only call from main() thread!!
void
TFB_FlushGraphics (void)
{
	int commands_handled;
	BOOLEAN livelock_deterrence;

	// This is technically a locking violation on DrawCommandQueue.Size,
	// but it is likely to not be very destructive.
	if (DrawCommandQueue.Size == 0)
	{
		static int last_fade = 255;
		static int last_transition = 255;
		int current_fade = GetFadeAmount ();
		int current_transition = TransitionAmount;
		
		if ((current_fade != 255 && current_fade != last_fade) ||
			(current_transition != 255 &&
			current_transition != last_transition) ||
			(current_fade == 255 && last_fade != 255) ||
			(current_transition == 255 && last_transition != 255))
		{
			TFB_SwapBuffers (TFB_REDRAW_FADING);
					// if fading, redraw every frame
		}
		else
		{
			TaskSwitch ();
		}
		
		last_fade = current_fade;
		last_transition = current_transition;
		BroadcastCondVar (RenderingCond);
		return;
	}

	if (GfxFlags & TFB_GFXFLAGS_SHOWFPS)
		computeFPS ();

	commands_handled = 0;
	livelock_deterrence = FALSE;

	if (DrawCommandQueue.FullSize > DCQ_FORCE_BREAK_SIZE)
	{
		TFB_BatchReset ();
	}

	if (DrawCommandQueue.Size > DCQ_FORCE_SLOWDOWN_SIZE)
	{
		Lock_DCQ (-1);
		livelock_deterrence = TRUE;
	}

	TFB_BBox_Reset ();

	for (;;)
	{
		TFB_DrawCommand DC;

		if (!TFB_DrawCommandQueue_Pop (&DC))
		{
			// the Queue is now empty.
			break;
		}

		++commands_handled;
		if (!livelock_deterrence && commands_handled + DrawCommandQueue.Size
				> DCQ_LIVELOCK_MAX)
		{
			// log_add (log_Debug, "Initiating livelock deterrence!");
			livelock_deterrence = TRUE;
			
			Lock_DCQ (-1);
		}

		switch (DC.Type)
		{
			case TFB_DRAWCOMMANDTYPE_SETMIPMAP:
			{
				TFB_DrawCommand_SetMipmap *cmd = &DC.data.setmipmap;
				TFB_DrawImage_SetMipmap (cmd->image, cmd->mipmap,
						cmd->hotx, cmd->hoty);
				break;
			}

			case TFB_DRAWCOMMANDTYPE_IMAGE:
			{
				TFB_DrawCommand_Image *cmd = &DC.data.image;
				TFB_Image *DC_image = cmd->image;
				const int x = cmd->x;
				const int y = cmd->y;

				TFB_DrawCanvas_Image (DC_image, x, y,
						cmd->scale, cmd->scaleMode, cmd->colormap,
						cmd->drawMode,
						TFB_GetScreenCanvas (cmd->destBuffer));

				if (cmd->destBuffer == TFB_SCREEN_MAIN)
				{
					LockMutex (DC_image->mutex);
					if (cmd->scale)
						TFB_BBox_RegisterCanvas (DC_image->ScaledImg,
								x - DC_image->last_scale_hs.x,
								y - DC_image->last_scale_hs.y);
					else
						TFB_BBox_RegisterCanvas (DC_image->NormalImg,
								x - DC_image->NormalHs.x,
								y - DC_image->NormalHs.y);
					UnlockMutex (DC_image->mutex);
				}

				break;
			}
			
			case TFB_DRAWCOMMANDTYPE_FILLEDIMAGE:
			{
				TFB_DrawCommand_FilledImage *cmd = &DC.data.filledimage;
				TFB_Image *DC_image = cmd->image;
				const int x = cmd->x;
				const int y = cmd->y;

				TFB_DrawCanvas_FilledImage (DC_image, x, y,
						cmd->scale, cmd->scaleMode, cmd->color,
						cmd->drawMode,
						TFB_GetScreenCanvas (cmd->destBuffer));

				if (cmd->destBuffer == TFB_SCREEN_MAIN)
				{
					LockMutex (DC_image->mutex);
					if (cmd->scale)
						TFB_BBox_RegisterCanvas (DC_image->ScaledImg,
								x - DC_image->last_scale_hs.x,
								y - DC_image->last_scale_hs.y);
					else
						TFB_BBox_RegisterCanvas (DC_image->NormalImg,
								x - DC_image->NormalHs.x,
								y - DC_image->NormalHs.y);
					UnlockMutex (DC_image->mutex);
				}

				break;
			}
			
			case TFB_DRAWCOMMANDTYPE_FONTCHAR:
			{
				TFB_DrawCommand_FontChar *cmd = &DC.data.fontchar;
				TFB_Char *DC_char = cmd->fontchar;
				const int x = cmd->x;
				const int y = cmd->y;

				TFB_DrawCanvas_FontChar (DC_char, cmd->backing, x, y,
						cmd->drawMode, TFB_GetScreenCanvas (cmd->destBuffer));

				if (cmd->destBuffer == TFB_SCREEN_MAIN)
				{
					RECT r;
					
					r.corner.x = x - DC_char->HotSpot.x;
					r.corner.y = y - DC_char->HotSpot.y;
					r.extent.width = DC_char->extent.width;
					r.extent.height = DC_char->extent.height;

					TFB_BBox_RegisterRect (&r);
				}

				break;
			}
			
			case TFB_DRAWCOMMANDTYPE_LINE:
			{
				TFB_DrawCommand_Line *cmd = &DC.data.line;

				if (cmd->destBuffer == TFB_SCREEN_MAIN)
				{
					TFB_BBox_RegisterPoint (cmd->x1, cmd->y1);
					TFB_BBox_RegisterPoint (cmd->x2, cmd->y2);
				}
				TFB_DrawCanvas_Line (cmd->x1, cmd->y1, cmd->x2, cmd->y2,
						cmd->color, cmd->drawMode,
						TFB_GetScreenCanvas (cmd->destBuffer));
				break;
			}
			
			case TFB_DRAWCOMMANDTYPE_RECTANGLE:
			{
				TFB_DrawCommand_Rect *cmd = &DC.data.rect;

				if (cmd->destBuffer == TFB_SCREEN_MAIN)
					TFB_BBox_RegisterRect (&cmd->rect);
				TFB_DrawCanvas_Rect (&cmd->rect, cmd->color, cmd->drawMode,
						TFB_GetScreenCanvas (cmd->destBuffer));

				break;
			}
			
			case TFB_DRAWCOMMANDTYPE_SCISSORENABLE:
			{
				TFB_DrawCommand_Scissor *cmd = &DC.data.scissor;

				TFB_DrawCanvas_SetClipRect (
						TFB_GetScreenCanvas (TFB_SCREEN_MAIN), &cmd->rect);
				TFB_BBox_SetClipRect (&DC.data.scissor.rect);
				break;
			}
			
			case TFB_DRAWCOMMANDTYPE_SCISSORDISABLE:
				TFB_DrawCanvas_SetClipRect (
						TFB_GetScreenCanvas (TFB_SCREEN_MAIN), NULL);
				TFB_BBox_SetClipRect (NULL);
				break;
			
			case TFB_DRAWCOMMANDTYPE_COPYTOIMAGE:
			{
				TFB_DrawCommand_CopyToImage *cmd = &DC.data.copytoimage;
				TFB_Image *DC_image = cmd->image;
				const POINT dstPt = {0, 0};

				if (DC_image == 0)
				{
					log_add (log_Debug, "DCQ ERROR: COPYTOIMAGE passed null "
							"image ptr");
					break;
				}
				LockMutex (DC_image->mutex);
				TFB_DrawCanvas_CopyRect (
						TFB_GetScreenCanvas (cmd->srcBuffer), &cmd->rect,
						DC_image->NormalImg, dstPt);
				UnlockMutex (DC_image->mutex);
				break;
			}
			
			case TFB_DRAWCOMMANDTYPE_COPY:
			{
				TFB_DrawCommand_Copy *cmd = &DC.data.copy;
				const RECT r = cmd->rect;

				if (cmd->destBuffer == TFB_SCREEN_MAIN)
					TFB_BBox_RegisterRect (&cmd->rect);

				TFB_DrawCanvas_CopyRect	(
						TFB_GetScreenCanvas (cmd->srcBuffer), &r,
						TFB_GetScreenCanvas (cmd->destBuffer), r.corner);
				break;
			}
			
			case TFB_DRAWCOMMANDTYPE_DELETEIMAGE:
			{
				TFB_Image *DC_image = DC.data.deleteimage.image;
				TFB_DrawImage_Delete (DC_image);
				break;
			}
			
			case TFB_DRAWCOMMANDTYPE_DELETEDATA:
			{
				void *data = DC.data.deletedata.data;
				HFree (data);
				break;
			}
			
			case TFB_DRAWCOMMANDTYPE_SENDSIGNAL:
				ClearSemaphore (DC.data.sendsignal.sem);
				break;
			
			case TFB_DRAWCOMMANDTYPE_REINITVIDEO:
			{
				TFB_DrawCommand_ReinitVideo *cmd = &DC.data.reinitvideo;
				int oldDriver = GraphicsDriver;
				int oldFlags = GfxFlags;
				int oldWidth = ScreenWidthActual;
				int oldHeight = ScreenHeightActual;
				if (TFB_ReInitGraphics (cmd->driver, cmd->flags,
						cmd->width, cmd->height, &resolutionFactor))
				{
					log_add (log_Error, "Could not provide requested mode: "
							"reverting to last known driver.");
					// We don't know what exactly failed, so roll it all back
					if (TFB_ReInitGraphics (oldDriver, oldFlags,
							oldWidth, oldHeight, &resolutionFactor))
					{
						log_add (log_Fatal,
								"Couldn't reinit at that point either. "
								"Your video has been somehow tied in knots.");
						exit (EXIT_FAILURE);
					}
				}
				TFB_SwapBuffers (TFB_REDRAW_YES);
				break;
			}
			
			case TFB_DRAWCOMMANDTYPE_CALLBACK:
			{
				DC.data.callback.callback (DC.data.callback.arg);
				break;
			}
		}
	}
	
	if (livelock_deterrence)
		Unlock_DCQ ();

	TFB_SwapBuffers (TFB_REDRAW_NO);
	RenderedFrames++;
	BroadcastCondVar (RenderingCond);
}

void
TFB_PurgeDanglingGraphics (void)
{
	Lock_DCQ (-1);

	for (;;)
	{
		TFB_DrawCommand DC;

		if (!TFB_DrawCommandQueue_Pop (&DC))
		{
			// the Queue is now empty.
			break;
		}

		switch (DC.Type)
		{
			case TFB_DRAWCOMMANDTYPE_DELETEIMAGE:
			{
				TFB_Image *DC_image = DC.data.deleteimage.image;
				TFB_DrawImage_Delete (DC_image);
				break;
			}
			case TFB_DRAWCOMMANDTYPE_DELETEDATA:
			{
				void *data = DC.data.deletedata.data;
				HFree (data);
				break;
			}
			case TFB_DRAWCOMMANDTYPE_IMAGE:
			{
				TFB_ColorMap *cmap = DC.data.image.colormap;
				if (cmap)
					TFB_ReturnColorMap (cmap);
				break;
			}
			case TFB_DRAWCOMMANDTYPE_SENDSIGNAL:
			{
				ClearSemaphore (DC.data.sendsignal.sem);
				break;
			}
		}
	}
	Unlock_DCQ ();
}
