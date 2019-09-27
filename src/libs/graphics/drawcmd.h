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

#ifndef DRAWCMD_H
#define DRAWCMD_H

#include "libs/graphics/tfb_draw.h"

enum
{
	TFB_DRAWCOMMANDTYPE_LINE,
	TFB_DRAWCOMMANDTYPE_RECTANGLE,
	TFB_DRAWCOMMANDTYPE_IMAGE,
	TFB_DRAWCOMMANDTYPE_FILLEDIMAGE,
	TFB_DRAWCOMMANDTYPE_FONTCHAR,

	TFB_DRAWCOMMANDTYPE_COPY,
	TFB_DRAWCOMMANDTYPE_COPYTOIMAGE,

	TFB_DRAWCOMMANDTYPE_SCISSORENABLE,
	TFB_DRAWCOMMANDTYPE_SCISSORDISABLE,

	TFB_DRAWCOMMANDTYPE_SETMIPMAP,
	TFB_DRAWCOMMANDTYPE_DELETEIMAGE,
	TFB_DRAWCOMMANDTYPE_DELETEDATA,
	TFB_DRAWCOMMANDTYPE_SENDSIGNAL,
	TFB_DRAWCOMMANDTYPE_REINITVIDEO,
	TFB_DRAWCOMMANDTYPE_CALLBACK,
};

typedef struct tfb_dc_line
{
	int x1, y1, x2, y2;
	Color color;
	DrawMode drawMode;
	SCREEN destBuffer;
} TFB_DrawCommand_Line;

typedef struct tfb_dc_rect
{
	RECT rect;
	Color color;
	DrawMode drawMode;
	SCREEN destBuffer;
} TFB_DrawCommand_Rect;

typedef struct tfb_dc_img
{
	TFB_Image *image;
	int x, y;
	SCREEN destBuffer;
	TFB_ColorMap *colormap;
	DrawMode drawMode;
	int scale;
	int scaleMode;
} TFB_DrawCommand_Image;

typedef struct tfb_dc_filledimg
{
	TFB_Image *image;
	int x, y;
	Color color;
	SCREEN destBuffer;
	DrawMode drawMode;
	int scale;
	int scaleMode;
} TFB_DrawCommand_FilledImage;

typedef struct tfb_dc_fontchar
{
	TFB_Char *fontchar;
	TFB_Image *backing;
	int x, y;
	DrawMode drawMode;
	SCREEN destBuffer;
} TFB_DrawCommand_FontChar;

typedef struct tfb_dc_copy
{
	RECT rect;
	SCREEN srcBuffer, destBuffer;
} TFB_DrawCommand_Copy;

typedef struct tfb_dc_copyimg
{
	TFB_Image *image;
	RECT rect;
	SCREEN srcBuffer;
} TFB_DrawCommand_CopyToImage;

typedef struct tfb_dc_scissor
{
	RECT rect;
} TFB_DrawCommand_Scissor;

typedef struct tfb_dc_setmip
{
	TFB_Image *image;
	TFB_Image *mipmap;
	int hotx, hoty;
} TFB_DrawCommand_SetMipmap;

typedef struct tfb_dc_delimg
{
	TFB_Image *image;
} TFB_DrawCommand_DeleteImage;

typedef struct tfb_dc_deldata
{
	void *data;
		// data must be a result of HXalloc() call
} TFB_DrawCommand_DeleteData;

typedef struct tfb_dc_signal
{
	Semaphore sem;
} TFB_DrawCommand_SendSignal;

typedef struct tfb_dc_reinit_video
{
	int driver, flags, width, height;
} TFB_DrawCommand_ReinitVideo;

typedef struct tfb_dc_callback
{
	void (*callback)(void *arg);
	void *arg;
} TFB_DrawCommand_Callback;

typedef struct tfb_drawcommand
{
	int Type;
	union {
		TFB_DrawCommand_Line line;
		TFB_DrawCommand_Rect rect;
		TFB_DrawCommand_Image image;
		TFB_DrawCommand_FilledImage filledimage;
		TFB_DrawCommand_FontChar fontchar;
		TFB_DrawCommand_Copy copy;
		TFB_DrawCommand_CopyToImage copytoimage;
		TFB_DrawCommand_Scissor scissor;
		TFB_DrawCommand_SetMipmap setmipmap;
		TFB_DrawCommand_DeleteImage deleteimage;
		TFB_DrawCommand_DeleteData deletedata;
		TFB_DrawCommand_SendSignal sendsignal;
		TFB_DrawCommand_ReinitVideo reinitvideo;
		TFB_DrawCommand_Callback callback;
	} data;
} TFB_DrawCommand;

// Queue Stuff

typedef struct tfb_drawcommandqueue
{
	int Front;
	int Back;
	int InsertionPoint;
	int Batching;
	volatile int FullSize;
	volatile int Size;
} TFB_DrawCommandQueue;

void Init_DrawCommandQueue (void);

void Uninit_DrawCommandQueue (void);

void TFB_BatchGraphics (void);

void TFB_UnbatchGraphics (void);

void TFB_BatchReset (void);

void TFB_DrawCommandQueue_Push (TFB_DrawCommand* Command);

int TFB_DrawCommandQueue_Pop (TFB_DrawCommand* Command);

void TFB_DrawCommandQueue_Clear (void);

extern TFB_DrawCommandQueue DrawCommandQueue;

void TFB_EnqueueDrawCommand (TFB_DrawCommand* DrawCommand);

void Lock_DCQ (int slots);

void Unlock_DCQ (void);

#endif
