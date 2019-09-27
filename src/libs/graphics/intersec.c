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

#include "libs/graphics/context.h"
#include "libs/graphics/drawable.h"
#include "libs/graphics/tfb_draw.h"
#include "libs/log.h"

//#define DEBUG_INTERSEC

static inline BOOLEAN
images_intersect (IMAGE_BOX *box1, IMAGE_BOX *box2, const RECT *rect)
{
	return TFB_DrawImage_Intersect (box1->FramePtr->image, box1->Box.corner,
			box2->FramePtr->image, box2->Box.corner, rect);
}

static TIME_VALUE
frame_intersect (INTERSECT_CONTROL *pControl0, RECT *pr0,
		INTERSECT_CONTROL *pControl1, RECT *pr1, TIME_VALUE t0,
		TIME_VALUE t1)
{
	SDWORD time_error0, time_error1;
	SDWORD cycle0, cycle1;
	SDWORD dx_0, dy_0, dx_1, dy_1;
	SDWORD xincr0, yincr0, xincr1, yincr1;
	SDWORD xerror0, xerror1, yerror0, yerror1;
	RECT r_intersect;
	IMAGE_BOX IB0, IB1;
	BOOLEAN check0, check1;

	IB0.FramePtr = pControl0->IntersectStamp.frame;
	IB0.Box.corner = pr0->corner;
	IB0.Box.extent.width = GetFrameWidth (IB0.FramePtr);
	IB0.Box.extent.height = GetFrameHeight (IB0.FramePtr);
	IB1.FramePtr = pControl1->IntersectStamp.frame;
	IB1.Box.corner = pr1->corner;
	IB1.Box.extent.width = GetFrameWidth (IB1.FramePtr);
	IB1.Box.extent.height = GetFrameHeight (IB1.FramePtr);

	dx_0 = pr0->extent.width;
	dy_0 = pr0->extent.height;
	if (dx_0 >= 0)
		xincr0 = 1;
	else
	{
		xincr0 = -1;
		dx_0 = -dx_0;
	}
	if (dy_0 >= 0)
		yincr0 = 1;
	else
	{
		yincr0 = -1;
		dy_0 = -dy_0;
	}
	if (dx_0 >= dy_0)
		cycle0 = dx_0;
	else
		cycle0 = dy_0;
	xerror0 = yerror0 = cycle0;
			
	dx_1 = pr1->extent.width;
	dy_1 = pr1->extent.height;
	if (dx_1 >= 0)
		xincr1 = 1;
	else
	{
		xincr1 = -1;
		dx_1 = -dx_1;
	}
	if (dy_1 >= 0)
		yincr1 = 1;
	else
	{
		yincr1 = -1;
		dy_1 = -dy_1;
	}
	if (dx_1 >= dy_1)
		cycle1 = dx_1;
	else
		cycle1 = dy_1;
	xerror1 = yerror1 = cycle1;
			
	check0 = check1 = FALSE;
	if (t0 <= 1)
	{
		time_error0 = time_error1 = 0;
		if (t0 == 0)
		{
			++t0;
			goto CheckFirstIntersection;
		}
	}
	else
	{
		SDWORD delta;
		DWORD start;
		long error;

		start = (DWORD)cycle0 * (DWORD)(t0 - 1);
		time_error0 = start & ((1 << TIME_SHIFT) - 1);
		if ((start >>= (DWORD)TIME_SHIFT) > 0)
		{
			if ((error = (long)xerror0
					- (long)dx_0 * (long)start) > 0)
				xerror0 = (SDWORD)error;
			else
			{
				delta = -(SDWORD)(error / (long)cycle0) + 1;
				IB0.Box.corner.x += xincr0 * delta;
				xerror0 = (SDWORD)(error + (long)cycle0 * (long)delta);
			}
			if ((error = (long)yerror0
					- (long)dy_0 * (long)start) > 0)
				yerror0 = (SDWORD)error;
			else
			{
				delta = -(SDWORD)(error / (long)cycle0) + 1;
				IB0.Box.corner.y += yincr0 * delta;
				yerror0 = (SDWORD)(error + (long)cycle0 * (long)delta);
			}
			pr0->corner = IB0.Box.corner;
		}
	
		start = (DWORD)cycle1 * (DWORD)(t0 - 1);
		time_error1 = start & ((1 << TIME_SHIFT) - 1);
		if ((start >>= (DWORD)TIME_SHIFT) > 0)
		{
			if ((error = (long)xerror1
					- (long)dx_1 * (long)start) > 0)
				xerror1 = (SDWORD)error;
			else
			{
				delta = -(SDWORD)(error / (long)cycle1) + 1;
				IB1.Box.corner.x += xincr1 * delta;
				xerror1 = (SDWORD)(error + (long)cycle1 * (long)delta);
			}
			if ((error = (long)yerror1
					- (long)dy_1 * (long)start) > 0)
				yerror1 = (SDWORD)error;
			else
			{
				delta = -(SDWORD)(error / (long)cycle1) + 1;
				IB1.Box.corner.y += yincr1 * delta;
				yerror1 = (SDWORD)(error + (long)cycle1 * (long)delta);
			}
			pr1->corner = IB1.Box.corner;
		}
	}

	pControl0->last_time_val = pControl1->last_time_val = t0;
	do
	{
		++t0;
		if ((time_error0 += cycle0) >= (1 << TIME_SHIFT))
		{
			if ((xerror0 -= dx_0) <= 0)
			{
				IB0.Box.corner.x += xincr0;
				xerror0 += cycle0;
			}
			if ((yerror0 -= dy_0) <= 0)
			{
				IB0.Box.corner.y += yincr0;
				yerror0 += cycle0;
			}

			check0 = TRUE;
			time_error0 -= (1 << TIME_SHIFT);
		}
			
		if ((time_error1 += cycle1) >= (1 << TIME_SHIFT))
		{
			if ((xerror1 -= dx_1) <= 0)
			{
				IB1.Box.corner.x += xincr1;
				xerror1 += cycle1;
			}
			if ((yerror1 -= dy_1) <= 0)
			{
				IB1.Box.corner.y += yincr1;
				yerror1 += cycle1;
			}

			check1 = TRUE;
			time_error1 -= (1 << TIME_SHIFT);
		}

		if (check0 || check1)
		{ /* if check0 && check1, this may not be quite right --
						 * if shapes had a pixel's separation to begin with
						 * and both moved toward each other, you would actually
						 * get a pixel overlap but since the last positions were
						 * separated by a pixel, the shapes wouldn't be touching
						 * each other.
						 */
CheckFirstIntersection:
			if (BoxIntersect (&IB0.Box, &IB1.Box, &r_intersect)
					&& images_intersect (&IB0, &IB1, &r_intersect))
				return (t0);
			
			if (check0)
			{
				pr0->corner = IB0.Box.corner;
				pControl0->last_time_val = t0;
				check0 = FALSE;
			}
			if (check1)
			{
				pr1->corner = IB1.Box.corner;
				pControl1->last_time_val = t0;
				check1 = FALSE;
			}
		}
	} while (t0 <= t1);

	return ((TIME_VALUE)0);
}

TIME_VALUE
DrawablesIntersect (INTERSECT_CONTROL *pControl0,
		INTERSECT_CONTROL *pControl1, TIME_VALUE max_time_val)
{
	SDWORD dy;
	SDWORD time_y_0, time_y_1;
	RECT r0, r1;
	FRAME FramePtr0, FramePtr1;

	if (!ContextActive () || max_time_val == 0)
		return ((TIME_VALUE)0);
	else if (max_time_val > MAX_TIME_VALUE)
		max_time_val = MAX_TIME_VALUE;

	pControl0->last_time_val = pControl1->last_time_val = 0;

	r0.corner = pControl0->IntersectStamp.origin;
	r1.corner = pControl1->IntersectStamp.origin;

	r0.extent.width = pControl0->EndPoint.x - r0.corner.x;
	r0.extent.height = pControl0->EndPoint.y - r0.corner.y;
	r1.extent.width = pControl1->EndPoint.x - r1.corner.x;
	r1.extent.height = pControl1->EndPoint.y - r1.corner.y;
		
	FramePtr0 = pControl0->IntersectStamp.frame;
	if (FramePtr0 == 0)
		return(0);
	r0.corner.x -= FramePtr0->HotSpot.x;
	r0.corner.y -= FramePtr0->HotSpot.y;

	FramePtr1 = pControl1->IntersectStamp.frame;
	if (FramePtr1 == 0)
		return(0);
	r1.corner.x -= FramePtr1->HotSpot.x;
	r1.corner.y -= FramePtr1->HotSpot.y;

	dy = r1.corner.y - r0.corner.y;
	time_y_0 = dy - GetFrameHeight (FramePtr0) + 1;
	time_y_1 = dy + GetFrameHeight (FramePtr1) - 1;
	dy = r0.extent.height - r1.extent.height;

	if ((time_y_0 <= 0 && time_y_1 >= 0)
			|| (time_y_0 > 0 && dy >= time_y_0)
			|| (time_y_1 < 0 && dy <= time_y_1))
	{
		SDWORD dx;
		SDWORD time_x_0, time_x_1;

		dx = r1.corner.x - r0.corner.x;
		time_x_0 = dx - GetFrameWidth (FramePtr0) + 1;
		time_x_1 = dx + GetFrameWidth (FramePtr1) - 1;
		dx = r0.extent.width - r1.extent.width;

		if ((time_x_0 <= 0 && time_x_1 >= 0)
				|| (time_x_0 > 0 && dx >= time_x_0)
				|| (time_x_1 < 0 && dx <= time_x_1))
		{
			TIME_VALUE intersect_time;

			if (dx == 0 && dy == 0)
				time_y_0 = time_y_1 = 0;
			else
			{
				SDWORD t;
				long time_beg, time_end, fract;

				if (time_y_1 < 0)
				{
					t = time_y_0;
					time_y_0 = -time_y_1;
					time_y_1 = -t;
				}
				else if (time_y_0 <= 0)
				{
					if (dy < 0)
						time_y_1 = -time_y_0;
					time_y_0 = 0;
				}
				if (dy < 0)
					dy = -dy;
				if (dy < time_y_1)
					time_y_1 = dy;
					/* just to be safe, widen search area */
				--time_y_0;
				++time_y_1;

				if (time_x_1 < 0)
				{
					t = time_x_0;
					time_x_0 = -time_x_1;
					time_x_1 = -t;
				}
				else if (time_x_0 <= 0)
				{
					if (dx < 0)
						time_x_1 = -time_x_0;
					time_x_0 = 0;
				}
				if (dx < 0)
					dx = -dx;
				if (dx < time_x_1)
					time_x_1 = dx;
					/* just to be safe, widen search area */
				--time_x_0;
				++time_x_1;

#ifdef DEBUG_INTERSEC
				log_add (log_Debug, "FramePtr0<%d, %d> --> <%d, %d>",
						GetFrameWidth (FramePtr0), GetFrameHeight (FramePtr0),
						r0.corner.x, r0.corner.y);
				log_add (log_Debug, "FramePtr1<%d, %d> --> <%d, %d>",
						GetFrameWidth (FramePtr1), GetFrameHeight (FramePtr1),
						r1.corner.x, r1.corner.y);
				log_add (log_Debug, "time_x(%d, %d)-%d, time_y(%d, %d)-%d",
						time_x_0, time_x_1, dx, time_y_0, time_y_1, dy);
#endif /* DEBUG_INTERSEC */
				if (dx == 0)
				{
					time_beg = time_y_0;
					time_end = time_y_1;
					fract = dy;
				}
				else if (dy == 0)
				{
					time_beg = time_x_0;
					time_end = time_x_1;
					fract = dx;
				}
				else
				{
					long time_x, time_y;

					time_x = (long)time_x_0 * (long)dy;
					time_y = (long)time_y_0 * (long)dx;
					time_beg = time_x < time_y ? time_y : time_x;

					time_x = (long)time_x_1 * (long)dy;
					time_y = (long)time_y_1 * (long)dx;
					time_end = time_x > time_y ? time_y : time_x;

					fract = (long)dx * (long)dy;
				}

				if ((time_beg <<= TIME_SHIFT) < fract)
					time_y_0 = 0;
				else
					time_y_0 = (SDWORD)(time_beg / fract);

				if (time_end >= fract /* just in case of overflow */
						|| (time_end <<= TIME_SHIFT) >=
						fract * (long)max_time_val)
					time_y_1 = max_time_val - 1;
				else
					time_y_1 = (SDWORD)((time_end + fract - 1) / fract) - 1;
			}

#ifdef DEBUG_INTERSEC
			log_add (log_Debug, "start_time = %d, end_time = %d",
					time_y_0, time_y_1);
#endif /* DEBUG_INTERSEC */
			if (time_y_0 <= time_y_1
					&& (intersect_time = frame_intersect (
					pControl0, &r0, pControl1, &r1,
					(TIME_VALUE)time_y_0, (TIME_VALUE)time_y_1)))
			{
				FramePtr0 = pControl0->IntersectStamp.frame;
				pControl0->EndPoint.x = r0.corner.x + FramePtr0->HotSpot.x;
				pControl0->EndPoint.y = r0.corner.y + FramePtr0->HotSpot.y;
				FramePtr1 = pControl1->IntersectStamp.frame;
				pControl1->EndPoint.x = r1.corner.x + FramePtr1->HotSpot.x;
				pControl1->EndPoint.y = r1.corner.y + FramePtr1->HotSpot.y;

				return (intersect_time);
			}
		}
	}

	return ((TIME_VALUE)0);
}

