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

// BW 2011: fixed using pixmaps to define planet surfaces so it actually works
// on planets you can land on. The second frame of planetmask.ani has to be a
// (Black & White) indexed pic with 128 colors top. Lesser indices (black)
// will correspond to lower altitudes.

#include "planets.h"
#include "scan.h"
#include "../nameref.h"
#include "../resinst.h"
#include "../setup.h"
#include "options.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#include "libs/mathlib.h"
#include "libs/log.h"
#include "libs/memlib.h"
#include "../starmap.h"
#include "../gendef.h"
#include "../colors.h"
#include <math.h>
#include <time.h>

#undef PROFILE_ROTATION

// define USE_ALPHA_SHIELD to use an aloha overlay instead of
// an additive overlay for the shield effect
#undef USE_ALPHA_SHIELD

#define SHIELD_GLOW_COMP    120
#define SHIELD_REFLECT_COMP 100

#define NUM_BATCH_POINTS 64
#define RADIUS RES_SCALE (33)
//2*RADIUS
#define TWORADIUS (RADIUS << 1)
//RADIUS^2
#define RADIUS_2(a) (a * a)
// distance beyond which all pixels are transparent (for aa)
#define RADIUS_THRES(a) ((a + 1) * (a + 1))
#define DIAMETER (TWORADIUS + 1)
#if 0
#	define SPHERE_SPAN_X (MAP_WIDTH >> 1)
#else
#	define SPHERE_SPAN_X (MAP_HEIGHT)
#endif
		// XXX: technically, the sphere's span over X should be MAP_WIDTH/2
		// but this causes visible surface compression over X, because
		// the surface dims ratio is H x H*PI, instead of H x 2*H
		// see bug #885

#define DIFFUSE_BITS 16
#define AA_WEIGHT_BITS 16

#ifndef M_TWOPI
  #ifndef M_PI
     #define M_PI 3.14159265358979323846
  #endif
  #define M_TWOPI (M_PI * 2.0)
#endif
#ifndef M_DEG2RAD
#define M_DEG2RAD (M_TWOPI / 360.0)
#endif

// BW: dynamically allocated in the orbit structure
// JMS_GFX: Changed initialization to constant numbers since DIAMETER is
// now variably defined The value 330 is the value that's reached at the
// biggest resolution, 4x.
// DWORD light_diff[330][330]; //DWORD light_diff[DIAMETER][DIAMETER];

// BW: Moved to planets.h
// typedef struct 
// {
// 	POINT p[4];
// 	DWORD m[4];
// } MAP3D_POINT;

// BW: dynamically allocated in the orbit structure
// JMS_GFX: Changed initialization to constant numbers since DIAMETER is
// now variably defined The value 330 is the value that's reached at the
// biggest resolution, 4x.
// MAP3D_POINT map_rotate[330][330];
// //MAP3D_POINT map_rotate[DIAMETER][DIAMETER];

typedef struct
{
	double x, y, z;
} POINT3;

static const Color tintColors[] =
{
	SCAN_MINERAL_TINT_COLOR,
	SCAN_ENERGY_TINT_COLOR,
	SCAN_BIOLOGICAL_TINT_COLOR,
};

BYTE
clip_channel(int c)
{
	if (c > 255)
		c = 255;
	if (c < 0)
		c = 0;
	return c;
}

void
TransformColor (Color *c, COUNT scan)
{
	if (scan == NUM_SCAN_TYPES)
		return;

	c->r = c->g = c->b = (c->r + c->g + c->b) / 3;

	switch (scan)
	{
		case MINERAL_SCAN:
		{
			c->b = 0x00;
			c->g = 0x00;
			break;
		}
		case ENERGY_SCAN:
		{
			c->r = (c->r / 3) * 2;
			c->b = c->r;
			c->g = c->r;
			break;
		}
		case BIOLOGICAL_SCAN:
		{
			c->r = 0x00;
			c->b = 0x00;
			break;
		}
		default:
			break;
	}
}

SBYTE
ColorDelta (COUNT scan, DWORD avg)
{
	if (scan == NUM_SCAN_TYPES)
		return 0;
	else
	{
		SBYTE diff = 0;

		if (avg > MAX_BRIGHTNESS (scan))
			diff = -(SBYTE)(avg - MAX_BRIGHTNESS (scan));
		else if (avg < MIN_BRIGHTNESS (scan))
			diff = MIN_BRIGHTNESS (scan) - avg;

		return diff;
	}
}

void
SetPlanetColors (COLORMAPPTR cmap)
{	// Setting colors for table that is used to color spheres
	// Hacky, but imitates the original rather well
	// That way we can have only 1 set of spheres and color them
	// appropriately
	BYTE *cbase, *ctab;
	Color *colors, *c;
	COUNT i;
	const COUNT numcolors = 128;

	colors = HMalloc (sizeof(Color) * numcolors);
	c = colors;
	cbase = GetColorMapAddress (pSolarSysState->OrbitalCMap);

	for (i = 0; i < numcolors; ++i, ++c)
	{
		ctab = (cbase + 2) + i * 3;

		*c = BUILD_COLOR_RGBA (ctab[0], ctab[1], ctab[2], 0xFF);
	}

	SetColorMapColors (colors, cmap, 0, numcolors);
	HFree (colors);
}

void
AdjustColor (Color *c, COUNT scan, COUNT height, COUNT width, SBYTE diff)
{
	COUNT y, x;

	for (y = 0; y < height; ++y)
	{
		for (x = 0; x < width; ++x, ++c)
		{
			switch (scan)
			{
			case MINERAL_SCAN:
			{
				c->r += diff;
				break;
			}
			case ENERGY_SCAN:
			{
				c->r += diff;
				c->g += diff;
				c->b += diff;
				break;
			}
			case BIOLOGICAL_SCAN:
			{
				c->g += diff;
				break;
			}
			default:
				break;
			}
		}
	}
}

static void
ExpandLevelMasks (PLANET_ORBIT* Orbit)
{	// Expand mask frame to avoid null spaces
	COUNT spherespanx;
	SIZE width, height;
	Color* colors;
	DWORD y, colorSize, halfBound;
	PLANET_INFO* PlanetInfo;

	PlanetInfo = &pSolarSysState->SysInfo.PlanetInfo;
	spherespanx = Orbit->SphereFrame->Bounds.width;
	width = Orbit->TopoMask->Bounds.width;
	height = Orbit->TopoMask->Bounds.height;

	halfBound = width + spherespanx;
	colorSize = height * (halfBound);

	colors = HMalloc (sizeof (Color) * colorSize);

	ReadFramePixelColors (Orbit->TopoMask, colors, halfBound, height);

	// Destroy mask to recreate it later
	DestroyDrawable (ReleaseDrawable (Orbit->TopoMask));
	Orbit->TopoMask = 0;

	for (y = 0; y < colorSize; y += halfBound)
		memcpy (colors + y + width, colors + y,
				spherespanx * sizeof (Color));

	if (PlanetInfo->AxialTilt == 0)
	{
		Orbit->TopoMask = CaptureDrawable (
				CreateDrawable (WANT_PIXMAP, halfBound, height, 1));

		WriteFramePixelColors (Orbit->TopoMask, colors, halfBound, height);
	}
	else
	{	// Because sphere frame if 75x67 we need to expand it's height to
		// avoid information loss and flat lines on the edges of the sphere
		// itself
		FRAME dupe;
		double err;

		err = sin (M_DEG2RAD * (double)PlanetInfo->AxialTilt)
				* (spherespanx - height);

		if (err < 0)
			err = -err;

		err = ceil (err);
				// Get the closest acceptable value (always ceil)

		dupe = CaptureDrawable (
				CreateDrawable (WANT_PIXMAP, halfBound, height, 1));
		WriteFramePixelColors (dupe, colors, halfBound, height);

		Orbit->TopoMask = CaptureDrawable (
				RescaleFrame (dupe, halfBound, height + (SIZE)err));
		DestroyDrawable (ReleaseDrawable (dupe));
	}

	HFree(colors);
}

static void
RenderLevelMasks (FRAME mask, SBYTE *pTopoData, BOOLEAN SurfDef)
{	// Kruzen: Originally there were 3 frames for levels 2,3,4 that was
	// drawn backwards using masks and prerendered colored spheres
	// Mask code has been nuked some time ago therefore I'm rendering 1
	// frame and using RED channel as offset multiplier I need it to be
	// frame because I need to rotate it later in case of orbital tilt
	FRAME OldFrame;
	COUNT i;
	BYTE AlgoType;
	SIZE base, d;
	const XLAT_DESC *xlatDesc;
	POINT pt;
	const PlanetFrame *PlanDataPtr;
	PRIMITIVE BatchArray[NUM_BATCH_POINTS];
	PRIMITIVE *pBatch;
	SBYTE *pSrc;
	POINT oldOrigin;
	RECT ClipRect;
	SIZE w, h;
	SIZE num_frames = 3;
	const SIZE *level_tab;

	OldFrame = SetContextFGFrame (mask);
	oldOrigin = SetContextOrigin (MAKE_POINT (0, 0));
	GetContextClipRect (&ClipRect);
	SetContextClipRect (NULL);

	w = mask->Bounds.width;
	h = mask->Bounds.height;

	pBatch = &BatchArray[0];
	for (i = 0; i < NUM_BATCH_POINTS; ++i, ++pBatch)
	{
		SetPrimNextLink (pBatch, i + 1);
		SetPrimType (pBatch, POINT_PRIM);
	}
	SetPrimNextLink (&pBatch[-1], END_OF_LIST);

	PlanDataPtr = &PlanData[
		pSolarSysState->pOrbitalDesc->data_index & ~PLANET_SHIELDED
	];
	AlgoType = PLANALGO (PlanDataPtr->Type);

	if (SurfDef)
	{	// Planets given by a pixmap have elevations between -128 and +128
		base = 256;
	}
	else
		base = PlanDataPtr->base_elevation;

	xlatDesc = (const XLAT_DESC *)pSolarSysState->XlatPtr;
	level_tab = (const SIZE *)xlatDesc->level_tab;

	while (num_frames > 0)
	{
		num_frames--;
		i = NUM_BATCH_POINTS;
		pBatch = &BatchArray[i];

		pSrc = pTopoData;
		for (pt.y = 0; pt.y < h; ++pt.y)
		{
			for (pt.x = 0; pt.x < w; ++pt.x)
			{
				d = *pSrc++;
				d += base;

				if (AlgoType == GAS_GIANT_ALGO)
				{	// make elevation value non-negative
					d &= 255;
				}
				else
				{
					if (d < 0)
						d = 0;
					else if (d > 255)
						d = 255;
				}

				if (d >= level_tab[num_frames]
					&& (num_frames == 2
						|| d < level_tab[num_frames + 1]))
				{
					--pBatch;
					pBatch->Object.Point.x = pt.x;
					pBatch->Object.Point.y = pt.y;

					SetPrimColor (pBatch,
						BUILD_COLOR_RGBA (0x01 * (num_frames + 1),
						0x00, 0x00, 0xFF));

					if (--i == 0)
					{	// flush the batch and start the next one
						DrawBatch (BatchArray, 0, 0);
						i = NUM_BATCH_POINTS;
						pBatch = &BatchArray[i];
					}
				}
			}
		}

		if (i < NUM_BATCH_POINTS)
			DrawBatch (BatchArray, i, 0);
	}
	SetContextClipRect (&ClipRect);
	SetContextOrigin (oldOrigin);
	SetContextFGFrame (OldFrame);
}

static void
RenderTopography (FRAME DstFrame, SBYTE *pTopoData, int w, int h,
		BOOLEAN SurfDef, COLORMAP scanTable)
{
	FRAME OldFrame;

	OldFrame = SetContextFGFrame (DstFrame);

	if (pSolarSysState->XlatRef == 0)
	{
		// There is currently nothing we can do w/o an xlat table
		// This is still called for Earth for 4x scaled topo, but we
		// do not need it because we cannot land on Earth.
		log_add (log_Warning,
				"No xlat table -- could not generate surface.\n");
	}
	else
	{
		COUNT i;
		BYTE AlgoType;
		SIZE base, d;
		const XLAT_DESC *xlatDesc;
		POINT pt;
		const PlanetFrame *PlanDataPtr;
		PRIMITIVE BatchArray[NUM_BATCH_POINTS];
		PRIMITIVE *pBatch;
		SBYTE *pSrc;
		const BYTE *xlat_tab;
		BYTE *cbase;
		POINT oldOrigin;
		RECT ClipRect;

		oldOrigin = SetContextOrigin (MAKE_POINT (0, 0));
		GetContextClipRect (&ClipRect);
		SetContextClipRect (NULL);

		pBatch = &BatchArray[0];
		for (i = 0; i < NUM_BATCH_POINTS; ++i, ++pBatch)
		{
			SetPrimNextLink (pBatch, i + 1);
			SetPrimType (pBatch, POINT_PRIM);
		}
		SetPrimNextLink (&pBatch[-1], END_OF_LIST);

		PlanDataPtr = &PlanData[
			pSolarSysState->pOrbitalDesc->data_index & ~PLANET_SHIELDED
		];
		AlgoType = PLANALGO (PlanDataPtr->Type);

		if (SurfDef)
		{	// Planets given by a pixmap have elevations between -128 and
			// +128
			base = 256;
		}
		else
			base = PlanDataPtr->base_elevation;

		xlatDesc = (const XLAT_DESC *)pSolarSysState->XlatPtr;
		xlat_tab = (const BYTE *)xlatDesc->xlat_tab;

		if (scanTable == NULL)
			cbase = GetColorMapAddress (pSolarSysState->OrbitalCMap);
		else
			cbase = GetColorMapAddress (scanTable);

		pSrc = pTopoData;

		for (pt.y = 0; pt.y < h; ++pt.y)
		{
			for (pt.x = 0; pt.x < w; ++pt.x, ++pSrc)
			{
				BYTE *ctab;

				d = *pSrc;
				if (AlgoType == GAS_GIANT_ALGO)
				{	// make elevation value non-negative
					if (optScanSphere == 1)
						d += base;
					d &= 255;
				}
				else
				{
					d += base;
					if (d < 0)
						d = 0;
					else if (d > 255)
						d = 255;
				}

				--pBatch;
				pBatch->Object.Point.x = pt.x;
				pBatch->Object.Point.y = pt.y;

				d = xlat_tab[d] - cbase[0];
				ctab = (cbase + 2) + d * 3;

				// using new truecolor tables ripped from DOS version
				SetPrimColor (pBatch, BUILD_COLOR_RGBA (ctab[0], ctab[1],
						ctab[2], 0xFF));

				if (--i == 0)
				{	// flush the batch and start the next one
					DrawBatch (BatchArray, 0, 0);
					i = NUM_BATCH_POINTS;
					pBatch = &BatchArray[i];
				}
			}
		}

		if (i < NUM_BATCH_POINTS)
		{
			DrawBatch (BatchArray, i, 0);
		}

		SetContextClipRect (&ClipRect);
		SetContextOrigin (oldOrigin);
	}

	SetContextFGFrame (OldFrame);
}

static inline void
P3mult (POINT3 *res, POINT3 *vec, double cnst)
{
	res->x = vec->x * cnst;
	res->y = vec->y * cnst;
	res->z = vec->z * cnst;
}

static inline void
P3sub (POINT3 *res, POINT3 *v1, POINT3 *v2)
{
	res->x = v1->x - v2->x;
	res->y = v1->y - v2->y;
	res->z = v1->z - v2->z;
}

static inline double
P3dot (POINT3 *v1, POINT3 *v2)
{
	return (v1->x * v2->x + v1->y * v2->y + v1->z * v2->z);
}

static inline void
P3norm (POINT3 *res, POINT3 *vec)
{
	double mag = sqrt (P3dot (vec, vec));
	P3mult (res, vec, 1/mag);
}

// GenerateSphereMask builds a shadow map for the rotating planet
//  loc indicates the planet's position relative to the sun
static void
GenerateSphereMask (POINT loc, COUNT radius)
{
	POINT pt;
	POINT3 light;
	double lrad;
	const DWORD step = 1 << DIFFUSE_BITS;
	int y, x;
	COUNT tworadius = radius << 1;
	COUNT radius_thres = (radius + 1) * (radius + 1);
	COUNT radius_2 = radius * radius;

#define AMBIENT_LIGHT 0.1
#define LIGHT_Z       1.2
	// lrad is the distance from the sun to the planet
	lrad = sqrt (loc.x * loc.x + loc.y * loc.y);
	// light is the sun's position.  the z-coordinate is whatever
	// looks good
	light.x = -((double)loc.x);
	light.y = -((double)loc.y);
	light.z = LIGHT_Z * lrad;
	P3norm (&light, &light);
	
	for (pt.y = 0, y = -radius; pt.y <= tworadius; ++pt.y, y++)
	{
		DWORD y_2 = y * y;

		for (pt.x = 0, x = -radius; pt.x <= tworadius; ++pt.x, x++)
		{
			DWORD x_2 = x * x;
			DWORD rad_2 = x_2 + y_2;
			DWORD diff_int = 0;
			POINT3 norm;
			double diff;
			
			if (rad_2 < radius_thres) 
			{
				// norm is the sphere's surface normal.
				norm.x = (double)x;
				norm.y = (double)y;
				norm.z = (sqrt (radius_2 - x_2) * sqrt (radius_2 - y_2)) /
						radius;
				P3norm (&norm, &norm);
				// diffuse component is norm dot light
				diff = P3dot (&norm, &light);
				// negative diffuse is bad
				if (diff < 0)
					diff = 0.0;
#if 0
				// Specular is not used in practice and is left here
				//  if someone decides to use it later for some reason.
				//  Specular highlight is only good for perfectly smooth
				//  surfaces, like balls (of which planets are not)
				//  This is the Phong equation
#define LIGHT_INTENS  0.3
#define MSHI          2
				double fb, spec;
				POINT3 rvec;
				POINT3 view;

				// always view along the z-axis
				// ideally use a view point, and have the view change
				// per pixel, but that is too much effort for now.
				// the view MUST be normalized!
				view.x = 0;
				view.y = 0;
				view.z = 1.0;

				// specular highlight is the phong equation:
				// (rvec dot view)^MSHI
				// where rvec = (2*diff)*norm - light (reflection of light
				// around norm)
				P3mult (&rvec, &norm, 2 * diff);
				P3sub (&rvec, &rvec, &light);
				fb = P3dot (&rvec, &view);
				if (fb > 0.0)
					spec = LIGHT_INTENS * pow (fb, MSHI);
				else
					spec = 0;
#endif
				// adjust for the ambient light
				if (diff < AMBIENT_LIGHT)
					diff = AMBIENT_LIGHT;
				// Now we antialias the edge of the spere to look nice
				if (rad_2 > radius_2) 
				{
					diff *= 1 - (sqrt(rad_2) - radius);
					if (diff < 0) 
						diff = 0;
				}
				// diff_int allows us multiply by a ratio without using
				// floating-point.
				diff_int = (DWORD)(diff * step);
			}

			pSolarSysState->Orbit.light_diff[pt.y][pt.x] = diff_int;
		}
	}
}

//create_aa_points creates weighted averages for
//  4 points around the 'ideal' point at x,y
//  the concept is to compute the weight based on the
//  distance from the integer location points to the ideal point
static void
create_aa_points (MAP3D_POINT *ppt, double x, double y, COUNT height)
{
	double deltax, deltay, inv_deltax, inv_deltay;
	COORD nextx, nexty;
	COUNT i;
	double d1, d2, d3, d4, m[4];
	COUNT spherespanx = height;

	if (x < 0)
		x = 0;
	else if (x >= spherespanx)
		x = spherespanx - 1;
	if (y < 0)
		y = 0;
	else if (y >= height)
		y = height - 1;

	// get  the integer value of this point
	ppt->p[0].x = (COORD)x;
	ppt->p[0].y = (COORD)y;
	deltax = x - ppt->p[0].x;
	deltay = y - ppt->p[0].y;
	
	// if this point doesn't need modificaton, set m[0]=0
	if (deltax == 0 && deltay == 0)
	{
		ppt->m[0] = 0;
		return;
	}

	// get the neighboring points surrounding the 'ideal' point
	if (deltax != 0)
		nextx = ppt->p[0].x + 1;
	else
		nextx = ppt->p[0].x;
	if (deltay != 0)
		nexty = ppt->p[0].y + 1;
	else 
		nexty = ppt->p[0].y;
	//(x1,y)
	ppt->p[1].x = nextx;
	ppt->p[1].y = ppt->p[0].y;
	//(x,y1)
	ppt->p[2].x = ppt->p[0].x;
	ppt->p[2].y = nexty;
	//(x1y1)
	ppt->p[3].x = nextx;
	ppt->p[3].y = nexty;
	//the square  1x1, so opposite poinnts are at 1-delta
	inv_deltax = 1.0 - fabs (deltax);
	inv_deltax *= inv_deltax;
	inv_deltay = 1.0 - fabs (deltay);
	inv_deltay *= inv_deltay;
	deltax *= deltax;
	deltay *= deltay;
	//d1-d4 contain the distances from the poinnts to the ideal point
	d1 = sqrt (deltax + deltay);
	d2 = sqrt (inv_deltax + deltay);
	d3 = sqrt (deltax + inv_deltay);
	d4 = sqrt (inv_deltax + inv_deltay);
	//compute the weights.  the sum(ppt->m[])=65536
	m[0] = 1 / (1 + d1 * (1 / d2 + 1 / d3 + 1 / d4));
	m[1] = m[0] * d1 / d2;
	m[2] = m[0] * d1 / d3;
	m[3] = m[0] * d1 / d4;

	for (i = 0; i < 4; i++)
		ppt->m[i] = (DWORD)(m[i] * (1 << AA_WEIGHT_BITS) + 0.5);
}

static inline BYTE
get_color_channel (Color c, int channel)
{
	switch (channel)
	{
		case 0:
			return c.r;
		case 1:
			return c.g;
		case 2:
			return c.b;
		default:
			return 0;
	}
}

// Creates either a red, green, or blue value by
// computing the weighted averages of the 4 points in p
static BYTE
get_avg_channel (Color p[4], DWORD mult[4], int channel)
{
	COUNT j;
	DWORD ci = 0;
	
	//sum(mult[])==65536
	//c is the red/green/blue value of this pixel
	for (j = 0; j < 4; j++)
	{
		BYTE c = get_color_channel (p[j], channel);
		ci += c * mult[j];
	}
	ci >>= AA_WEIGHT_BITS;
	//check for overflow
	if (ci > 255)
		ci = 255;
	
	return ((UBYTE)ci);
}

// CreateSphereTiltMap creates 'map_rotate' to map the topo data
//  for a tilted planet.  It also does the sphere->plane mapping
static void
CreateSphereTiltMap (int angle, COUNT height, COUNT radius)
{
	int x, y;
	const double multx = ((double)height / M_PI);
	const double multy = ((double)height / M_PI);
	const double xadj = ((double)height / 2.0);

	for (y = -radius; y <= radius; y++)
	{
		int y_2 = y * y;

		for (x = -radius; x <= radius; x++)
		{
			double dx, dy, newx, newy;
			double da, rad, rad_2;
			double xa, ya;
			MAP3D_POINT *ppt =
					&pSolarSysState->Orbit.map_rotate[y + radius]
						[x + radius];
			
			rad_2 = x * x + y_2;

			if (rad_2 >= RADIUS_THRES(radius))
			{	// pixel won't be present
				ppt->p[0].x = x + radius;
				ppt->p[0].y = y + radius;
				ppt->m[0] = 0;

				continue;
			}
			
			rad = sqrt (rad_2);
			// antialiasing goes beyond the actual radius
			if (rad >= radius)
				rad = (double)radius - 0.1;
			
			da = atan2 ((double)y, (double)x);
			// compute the planet-tilt
			da += M_DEG2RAD * -angle;
			dx = rad * cos (da);
			dy = rad * sin (da);

			// Map the sphere onto a plane
			xa = acos (-dx / radius);
			ya = acos (-dy / radius);
			newx = multx * xa;
			newy = multy * ya;
			// Adjust for vertical curvature
			if (ya <= 0.05 || ya >= 3.1 /* almost PI */)
				newx = xadj; // exact centerline
			else
				newx = xadj + ((newx - xadj) / sin (ya));

			create_aa_points (ppt, newx, newy, height);
		}
	}
}

//CreateShieldMask
// The shield is created in two parts.  This routine creates the Halo.
// The red tint of the planet is currently applied in RenderPlanetSphere
// This was done because the shield glows and needs to modify how the
// planet gets lit. Currently, the planet area is transparent in the mask
// made by this routine, but a filter can be applied if desired too.

// HALO rim size
#define SHIELD_HALO          RES_SCALE (actuallyInOrbit ? 5 : 16)
#define SHIELD_RADIUS        (RADIUS + SHIELD_HALO)
#define SHIELD_DIAM          ((SHIELD_RADIUS << 1) + 1)
#define SHIELD_RADIUS_2      (SHIELD_RADIUS * SHIELD_RADIUS)
#define SHIELD_RADIUS_THRES \
		((SHIELD_RADIUS + RES_SCALE (1)) * (SHIELD_RADIUS + RES_SCALE (1)))
#define SHIELD_HALO_GLOW     (SHIELD_GLOW_COMP + SHIELD_REFLECT_COMP)
#define SHIELD_HALO_GLOW_MIN (SHIELD_HALO_GLOW >> 2)

static FRAME
CreateShieldMask (COUNT radius)
{
	Color clear;
	Color *pix;
	int x, y;
	FRAME ShieldFrame;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;
	COUNT shieldradius = SHIELD_RADIUS * radius / RADIUS;
	COUNT shielddiam = (shieldradius << 1) + 1;

	ShieldFrame = CaptureDrawable (
		CreateDrawable (WANT_PIXMAP | WANT_ALPHA,
			shielddiam, shielddiam, 1));

	pix = Orbit->ScratchArray;
	//  This is 100% transparent.
	clear = BUILD_COLOR_RGBA (0, 0, 0, 0);	

	for (y = -shieldradius; y <= shieldradius; y++)
	{
		for (x = -shieldradius; x <= shieldradius; ++x, ++pix)
		{
			int rad_2 = x * x + y * y;
			// This is a non-transparent red for the halo
			int red = SHIELD_HALO_GLOW;
			int alpha = 255;
			double rad;
			
			if (rad_2 >= RADIUS_THRES (shieldradius))
			{	// outside all bounds
				*pix = clear;
				continue;
			}
			// Inside the halo
			if (rad_2 <= RADIUS_2 (radius))
			{	// planet's pixels, ours transparent
				*pix = clear;
				continue;
			}
			
			// The halo itself
			rad = sqrt (rad_2);

			if (rad <= radius + 0.8)
			{	// pixels common between the shield and planet
				// do antialiasing using alpha
				alpha = (int) (red * (rad - radius));
				red = 255;
			}
			else
			{	// shield pixels
				red -= (int) ((red - SHIELD_HALO_GLOW_MIN) * (rad - radius)
						/ (SHIELD_HALO * radius / RADIUS));
				if (red < 0)
					red = 0;
			}
			
			if (optNebulae)
			{
				if (alpha != 255)
					*pix = BUILD_COLOR_RGBA(red, 0, 0, alpha);
				else
					*pix = BUILD_COLOR_RGBA(255, 0, 0, red);
			}
			else
				*pix = BUILD_COLOR_RGBA(red, 0, 0, alpha);
		}
	}
	
	WriteFramePixelColors (ShieldFrame, Orbit->ScratchArray,
			shielddiam, shielddiam);
	SetFrameHot (ShieldFrame, MAKE_HOT_SPOT (shieldradius + 1,
				shieldradius + 1));
	
	return ShieldFrame;
}

FRAME
SaveBackFrame (COUNT radius)
{
	RECT r;
	CONTEXT oldContext;
	FRAME BackFrame;
	COUNT shieldradius = SHIELD_RADIUS * radius / RADIUS;
	COUNT shielddiam = (shieldradius << 1) + 1;

	// Kruzen: prepare back frame in OffScreenContext.
	// Cannot do it directly in planet context
	// because on game load Star background being drawn after
	// this part. Drawing Star background to PlanetContext
	// kills smooth transition
	oldContext = SetContext (PlanetContext);
	GetContextClipRect (&r);
	SetContext (OffScreenContext);
	SetContextClipRect (&r);
	DrawStarBackGround ();

	r.corner = MAKE_POINT ((RES_SCALE(ORIG_SIS_SCREEN_WIDTH >> 1))
			- (shieldradius + 1), PLANET_ORG_Y - (shieldradius + 1));
	r.extent.height = r.extent.width = shielddiam;

	BackFrame = CaptureDrawable (CopyContextRect (&r));

	SetFrameHot (BackFrame, MAKE_HOT_SPOT (shieldradius + 1,
		shieldradius + 1));

	SetContext (oldContext);

	return BackFrame;
}

// SetShieldThrobEffect adjusts the red levels in the shield glow graphic
//  the throbbing cycle is tied to the planet rotation cycle
#define SHIELD_THROBS 7
		// throb cycles per revolution
#define THROB_CYCLE      ((MAP_WIDTH << 8) / SHIELD_THROBS)
#define THROB_HALF_CYCLE (THROB_CYCLE >> 1)

#define THROB_MAX_LEVEL 256
#define THROB_MIN_LEVEL 100
#define THROB_D_LEVEL   (THROB_MAX_LEVEL - THROB_MIN_LEVEL)

static inline int
shield_level (int offset)
{
	int level;

	offset = (offset << 8) % THROB_CYCLE;
	level = abs (offset - THROB_HALF_CYCLE);
	level = THROB_MIN_LEVEL + level * THROB_D_LEVEL / THROB_HALF_CYCLE;

	return level;
}

// See description above
// offset is effectively the angle of rotation around the planet's axis
void
SetShieldThrobEffect (FRAME ShieldFrame, int offset, FRAME ThrobFrame)
{
	int i;
	int width, height;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;
	Color *pix;
	int level;
	
	level = shield_level (offset);

	width = GetFrameWidth (ShieldFrame);
	height = GetFrameHeight (ShieldFrame);
	ReadFramePixelColors (ShieldFrame, Orbit->ScratchArray, width, height);
	
	for (i = 0, pix = Orbit->ScratchArray; i < width * height; ++i, ++pix)
	{
		Color p = *pix;

		if (optNebulae)
		{
			p.a = p.a * level / THROB_MAX_LEVEL;
		}
		else
		{
			if (p.a == 255)
			{	// adjust color data for full-alpha pixels
				p.r = p.r * level / THROB_MAX_LEVEL;
				p.g = p.g * level / THROB_MAX_LEVEL;
				p.b = p.b * level / THROB_MAX_LEVEL;
			}
			else if (p.a > 0)
			{	// adjust alpha for translucent pixels
				p.a = p.a * level / THROB_MAX_LEVEL;
			}
		}

		*pix = p;
	}
	
	WriteFramePixelColors (ThrobFrame, Orbit->ScratchArray, width, height);
	SetFrameHot (ThrobFrame, GetFrameHot (ShieldFrame));
}

static BYTE opacity[10] =
		{ 0xFF, 0xE6, 0xC5, 0xA4, 0x83, 0x62,
			0x83, 0xA4, 0xC5, 0xE6 };

#define FRAME_SKIP RES_DBL (2)

void
Draw3DOShield (STAMP ShieldFrame)
{
	static DWORD i = 0;
	static DWORD cr = 0;
	DrawMode oldmode;

	oldmode = SetContextDrawMode (MAKE_DRAW_MODE (DRAW_ADDITIVE,
			opacity[i % 10]));

	DrawStamp (&ShieldFrame);

	SetContextDrawMode (oldmode);

	if (cr % FRAME_SKIP == 0)
		i++;

	cr++;
}

// Apply the shield to the topo image
static void
ApplyShieldTint (void)
{
	DrawMode mode, oldMode;
	FRAME oldFrame;
	Color tint;
	RECT r;

	// TopoFrame will be permanently changed
	oldFrame = SetContextFGFrame (pSolarSysState->TopoFrame);
	SetContextClipRect (NULL);
	GetContextClipRect (&r);

	tint = BUILD_COLOR_RGBA (0xff, 0x00, 0x00, 0xff);
#ifdef USE_ALPHA_SHIELD
	mode = MAKE_DRAW_MODE (DRAW_ALPHA, 150);
#else
	mode = MAKE_DRAW_MODE (DRAW_ADDITIVE, DRAW_FACTOR_1);
#endif
	oldMode = SetContextDrawMode (mode);
	SetContextForeGroundColor (tint);
	DrawFilledRectangle (&r);
	SetContextDrawMode (oldMode);
	SetContextFGFrame (oldFrame);
}

static inline UBYTE
calc_map_light (UBYTE val, DWORD dif, int lvf)
{
	int i;

	// apply diffusion
	i = (dif * val) >> DIFFUSE_BITS;
	// apply light variance for 3d lighting effect
	i += (lvf * val) >> 7;

	if (i < 0)
		i = 0;
	else if (i > 255)
		i = 255;

	return ((UBYTE)i);
}

static inline Color
get_map_pixel (Color *pixels, int x, int y, COUNT width, COUNT spherespanx)
{
	/* if (y * (width + spherespanx) + x > 463000)
		log_add (log_Warning,"x:%u, y:%u, width:%u, spherespanx:%u, "
				"slot:%u. Max:%u", x, y, width, spherespanx,
				y * (width + spherespanx) + x,
				(MAP_HEIGHT + 1) * (MAP_WIDTH + spherespanx)); */
	return pixels[y * (width + spherespanx) + x];
}

static inline Color
apply_alpha_pixel (Color pix, int scan)
{
	Color c;
	c.r = (tintColors[scan].r * 0x45 / 0xFF)
			+ (pix.r * 0xFF * (0xFF - 0x45) / (255 * 255));
	c.g = (tintColors[scan].g * 0x45 / 0xFF)
			+ (pix.g * 0xFF * (0xFF - 0x45) / (255 * 255));
	c.b = (tintColors[scan].b * 0x45 / 0xFF)
			+ (pix.b * 0xFF * (0xFF - 0x45) / (255 * 255));

	return c;
}

static inline Color
apply_additive_pixel (Color pix, int scan)
{
	Color c;
	c.r = clip_channel (pix.r + ((tintColors[scan].r 
		* 0x45) >> 8));
	c.g = clip_channel (pix.g + ((tintColors[scan].g 
		* 0x45) >> 8));
	c.b = clip_channel (pix.b + ((tintColors[scan].b 
		* 0x45) >> 8));

	return c;
}

static inline int
get_map_elev (SBYTE *elevs, int x, int y, int offset, COUNT width)
{
	return elevs[y * width + (offset + x) % width];
}

// RenderPlanetSphere builds a frame for the rotating planet view
// offset is effectively the angle of rotation around the planet's axis
// We use the SDL routines to directly write to the SDL_Surface to improve
// performance
void
RenderPlanetSphere (PLANET_ORBIT *Orbit, FRAME MaskFrame, int offset,
		BOOLEAN shielded, BOOLEAN doThrob, COUNT width, COUNT height,
		COUNT radius)
{
	POINT pt;
	Color *pix;
	Color clear;
	int x, y;
	Color *pixels;
	SBYTE *elevs;
	int shLevel;
	COUNT spherespanx = height;
	COUNT tworadius = radius << 1;
	COUNT diameter = tworadius + 1;

#if PROFILE_ROTATION
	static clock_t t = 0;
	static int frames_done = 1;
	clock_t t1;
	t1 = clock ();
#endif

	shLevel = shield_level (offset);

	pix = Orbit->ScratchArray;
	clear = BUILD_COLOR_RGBA (0, 0, 0, 0);

	if ((Orbit->scanType < NUM_SCAN_TYPES) && Orbit->ScanColors)
		pixels = Orbit->ScanColors[Orbit->scanType] + offset;
	else
		pixels = Orbit->TopoColors + offset;

	elevs = Orbit->lpTopoData;
	
	for (pt.y = 0, y = -radius; pt.y <= tworadius; ++pt.y, ++y)
	{
		for (pt.x = 0, x = -radius; pt.x <= tworadius; ++pt.x, ++x, ++pix)
		{
			Color c;
			DWORD diffus = Orbit->light_diff[pt.y][pt.x];
			int i;
			MAP3D_POINT *ppt = &Orbit->map_rotate[pt.y][pt.x];
			int lvf; // light variance factor
	
			if (diffus == 0)
			{	// full diffusion
				*pix = clear;
				continue;
			}

			// get pixel from topo map and factor from light variance map
			if (ppt->m[0] == 0) 
			{	// exact pixel from the topo map
				c = get_map_pixel (pixels, ppt->p[0].x, ppt->p[0].y, width,
						spherespanx);
				lvf = get_map_elev (elevs, ppt->p[0].x, ppt->p[0].y,
						offset, width);
			}
			else
			{	// fractional pixel -- blend from 4
				Color p[4];
				int lvsum;

				// compute 'ideal' pixel
				for (i = 0; i < 4; i++)
					p[i] = get_map_pixel (pixels, ppt->p[i].x, ppt->p[i].y,
							width, spherespanx);
				
				c.r = get_avg_channel (p, ppt->m, 0);
				c.g = get_avg_channel (p, ppt->m, 1);
				c.b = get_avg_channel (p, ppt->m, 2);

				// compute 'ideal' light variance
				for (i = 0, lvsum = 0; i < 4; i++)
					lvsum += get_map_elev (elevs, ppt->p[0].x, ppt->p[0].y,
							offset, width) * ppt->m[i];
				lvf = lvsum >> AA_WEIGHT_BITS;
			}
		
			// Apply the lighting model.  This also bounds the sphere
			// to make it circular.
			if (shielded)
			{
				int r;
				
				// add lite red filter (3/4) component
				c.g = (c.g >> 1) + (c.g >> 2);
				c.b = (c.b >> 1) + (c.b >> 2);

				c.r = calc_map_light (c.r, diffus, lvf);
				c.g = calc_map_light (c.g, diffus, lvf);
				c.b = calc_map_light (c.b, diffus, lvf);

				// The shield is glow + reflect (+ filter for others)
				r = calc_map_light (SHIELD_REFLECT_COMP, diffus, 0);
				r += SHIELD_GLOW_COMP;
				
				if (doThrob)
				{	// adjust red level for throbbing shield
					r = r * shLevel / THROB_MAX_LEVEL;
				}

				r += c.r;
				if (r > 255)
					r = 255;
				c.r = r;
			}
			else
			{
				c.r = calc_map_light (c.r, diffus, lvf);
				c.g = calc_map_light (c.g, diffus, lvf);
				c.b = calc_map_light (c.b, diffus, lvf);
			}

			c.a = 0xff;

			if (optScanStyle == OPT_3DO && optTintPlanSphere == OPT_PC
					&& Orbit->scanType < NUM_SCAN_TYPES)
				*pix = apply_alpha_pixel (c, Orbit->scanType);
			else
				*pix = c;
		}
	}
	
	WriteFramePixelColors (MaskFrame, Orbit->ScratchArray, diameter,
			diameter);
	SetFrameHot (MaskFrame, MAKE_HOT_SPOT (radius + 1, radius + 1));

#if PROFILE_ROTATION
	t += clock() - t1;
	if (frames_done == width)
	{
		log_add (log_Debug, "Rotation frames/sec: %d/%ld(msec)=%f",
				frames_done,
				(long int) (((double)t / CLOCKS_PER_SEC) * 1000.0 + 0.5),
				frames_done / ((double)t / CLOCKS_PER_SEC + 0.5));
		frames_done = 1;
		t = clock () - t1;
	}
	else
		frames_done++;
#endif
}

void
RenderDOSPlanetSphere (PLANET_ORBIT *Orbit, FRAME MaskFrame, int offset)
{
	if (!Orbit->TopoMask)
		return;
	else
	{	// Prepare new frame (oh god...)
		BYTE *pix;
		Color *color;
		COUNT x, y;
		SIZE width = MaskFrame->Bounds.width;
		SIZE height = Orbit->TopoMask->Bounds.height;
		RECT r;
		FRAME dupeframe;
		PLANET_INFO *PlanetInfo = &pSolarSysState->SysInfo.PlanetInfo;

		r.corner.y = 0;
		r.corner.x = offset;
		r.extent.width = width;
		r.extent.height = height;

		// Get rect with offset 
		dupeframe = CaptureDrawable (CopyFrameRect (Orbit->TopoMask, &r));

		if (PlanetInfo->AxialTilt != 0)
		{	// We need to tilt the frame
			STAMP s;
			FRAME baseframe, rotFrame;
			DrawMode oldMode;
			CONTEXT oldContext;
			SIZE trueheight = MaskFrame->Bounds.height;

			rotFrame = CaptureDrawable (
					RotateFrame (dupeframe, PlanetInfo->AxialTilt));
			GetFrameRect (rotFrame, &r);
			baseframe = CaptureDrawable (
				CreateDrawable (WANT_PIXMAP, (SIZE)width, trueheight, 1));

			// Draw everything in offscreen context
			oldContext = SetContext (OffScreenContext);
			SetContextFGFrame (baseframe);
			SetContextClipRect (NULL);

			oldMode = SetContextDrawMode (DRAW_REPLACE_MODE);

			SetFrameHot (rotFrame, MAKE_HOT_SPOT (0, 0));

			s.origin.x = (width - r.extent.width) / 2 - 1;
			s.origin.y = (trueheight - r.extent.height) / 2;
			s.frame = rotFrame;
			DrawStamp (&s);

			SetContextDrawMode (oldMode);
			SetContext (oldContext);

			// We got our colors - dump everything to avoid memory leak
			ReadFramePixelColors (baseframe, Orbit->ScratchArray, width,
					trueheight);
			DestroyDrawable (ReleaseDrawable (baseframe));
			DestroyDrawable (ReleaseDrawable (rotFrame));
		}
		else
			ReadFramePixelColors (dupeframe, Orbit->ScratchArray, width,
					height);

		pix = Orbit->sphereBytes;
		color = Orbit->ScratchArray;

		// Set indexes for sphere frame pixel by pixel
		for (y = 0; y < MaskFrame->Bounds.height; ++y)
		{
			for (x = 0; x < MaskFrame->Bounds.width; ++x, ++color, ++pix)
			{
				if (*pix < 0xFF)// If not transparent
				{	// Normalize index to first 32-bit range, then add
					// offset from mask
					*pix = *pix - ((*pix / 32) * 32) + (color->r * 32);
				}
			}
		}
		WriteFramePixelIndexes (MaskFrame, Orbit->sphereBytes,
				MaskFrame->Bounds.width, MaskFrame->Bounds.height);

		DestroyDrawable (ReleaseDrawable (dupeframe));
		dupeframe = 0;
	}
}

void
Render3DOPlanetSphere (PLANET_ORBIT* Orbit, FRAME MaskFrame, int offset,
		COUNT rotwidth, COUNT height)
{
	int x, y;
	Color *c, *pixels, *shade;
	Color clear;
	POINT pt;
	COUNT spherespanx = height;
	COUNT radius = (spherespanx >> 1) - IF_HD(2);
	COUNT tworadius = radius << 1;
	COUNT diameter = tworadius + 1;

	c = Orbit->ScratchArray;
	shade = Orbit->ShadeColors;
	pixels = Orbit->TopoColors + offset;
	clear = BUILD_COLOR_RGBA (0, 0, 0, 0);

	for (pt.y = 0, y = -radius; pt.y <= tworadius; ++pt.y, ++y)
	{
		for (pt.x = 0, x = -radius; pt.x <= tworadius; ++pt.x, ++x, ++c,
				++shade)
		{
			MAP3D_POINT* ppt = &Orbit->map_rotate[pt.y][pt.x];

			if (ppt->m[0] == 0 || shade->r == 0xFF)
			{	// exact pixel from the topo map
				*c = clear;
				continue;
			}
			else
			{
				*c = get_map_pixel (pixels, ppt->p[0].x, ppt->p[0].y,
						rotwidth, spherespanx);

				c->r = clip_channel (c->r - shade->r);
				c->g = clip_channel (c->g - shade->g);
				c->b = clip_channel (c->b - shade->b);

				if (optTintPlanSphere == OPT_PC
					&& Orbit->scanType < NUM_SCAN_TYPES)
				{
					if (optScanStyle == OPT_3DO)
						*c = apply_additive_pixel (
							*c, Orbit->scanType);
					else if (optScanStyle == OPT_PC)
						TransformColor (c, Orbit->scanType);
				}
			}
		}
	}

	WriteFramePixelColors (MaskFrame, Orbit->ScratchArray, diameter,
			diameter);
	SetFrameHot (MaskFrame, MAKE_HOT_SPOT (radius + 1, radius + 1));
}

#define RANGE_SHIFT 6

static void
DitherMap (SBYTE *DepthArray, COUNT width, COUNT height)
{
#define DITHER_VARIANCE  (1 << (RANGE_SHIFT - 3))
	DWORD i;
	SBYTE *elev;
	DWORD rand_val = 0;

	for (i = 0, elev = DepthArray; i < (DWORD)(width * height); ++i,
			++elev)
	{
		// Use up the random value byte by byte
		if ((i & 3) == 0)
			rand_val = TFB_Random ();
		else
			rand_val >>= 8;

		// Bring the elevation point up or down
		*elev += DITHER_VARIANCE / 2 - (rand_val & (DITHER_VARIANCE - 1));
	}
}

static void
MakeCrater (RECT *pRect, SBYTE *DepthArray, SIZE rim_delta, SIZE
		crater_delta, BOOLEAN SetDepth, COUNT width)
{
	COORD x, y, lf_x, rt_x;
	SIZE A, B;
	SDWORD Asquared, TwoAsquared, Bsquared, TwoBsquared;
	SDWORD d, dx, dy;
	DWORD TopIndex, BotIndex, rim_pixels;
 

	A = pRect->extent.width >> 1;
	B = pRect->extent.height >> 1;

	x = 0;
	y = B;

	Asquared = (DWORD)A * A;
	TwoAsquared = Asquared << 1;
	Bsquared = (DWORD)B * B;
	TwoBsquared = Bsquared << 1;

	dx = 0;
	dy = TwoAsquared * B;
	d = Bsquared - (dy >> 1) + (Asquared >> 2);

	A += pRect->corner.x;
	B += pRect->corner.y;
	TopIndex = (B - y) * width;
	BotIndex = (B + y) * width;
	rim_pixels = 1;
	while (dx < dy)
	{
		if (d > 0)
		{
			lf_x = A - x;
			rt_x = A + x;
			if (SetDepth)
			{
				memset (&DepthArray[TopIndex + lf_x], 0, rt_x - lf_x + 1);
				memset (&DepthArray[BotIndex + lf_x], 0, rt_x - lf_x + 1);
			}
			if (lf_x == rt_x)
			{
				DepthArray[TopIndex + lf_x] += rim_delta;
				DepthArray[BotIndex + lf_x] += rim_delta;
				rim_pixels = 0;
			}
			else
			{
				do
				{
					DepthArray[TopIndex + lf_x] += rim_delta;
					DepthArray[BotIndex + lf_x] += rim_delta;
					if (lf_x != rt_x)
					{
						DepthArray[TopIndex + rt_x] += rim_delta;
						DepthArray[BotIndex + rt_x] += rim_delta;
					}
					++lf_x;
					--rt_x;
				} while (--rim_pixels);

				while (lf_x < rt_x)
				{
					DepthArray[TopIndex + lf_x] += crater_delta;
					DepthArray[BotIndex + lf_x] += crater_delta;
					DepthArray[TopIndex + rt_x] += crater_delta;
					DepthArray[BotIndex + rt_x] += crater_delta;
					++lf_x;
					--rt_x;
				}

				if (lf_x == rt_x)
				{
					DepthArray[TopIndex + lf_x] += crater_delta;
					DepthArray[BotIndex + lf_x] += crater_delta;
				}
			}
		
			--y;
			TopIndex += width;
			BotIndex -= width;
			dy -= TwoAsquared;
			d -= dy;
		}

		++rim_pixels;
		++x;
		dx += TwoBsquared;
		d += Bsquared + dx;
	}

	d += ((((Asquared - Bsquared) * 3) >> 1) - (dx + dy)) >> 1;

	while (y > 0)
	{
		lf_x = A - x;
		rt_x = A + x;
		if (SetDepth)
		{
			memset (&DepthArray[TopIndex + lf_x], 0, rt_x - lf_x + 1);
			memset (&DepthArray[BotIndex + lf_x], 0, rt_x - lf_x + 1);
		}
		if (lf_x == rt_x)
		{
			DepthArray[TopIndex + lf_x] += rim_delta;
			DepthArray[BotIndex + lf_x] += rim_delta;
		}
		else
		{
			do
			{
				DepthArray[TopIndex + lf_x] += rim_delta;
				DepthArray[BotIndex + lf_x] += rim_delta;
				if (lf_x != rt_x)
				{
					DepthArray[TopIndex + rt_x] += rim_delta;
					DepthArray[BotIndex + rt_x] += rim_delta;
				}
				++lf_x;
				--rt_x;
			} while (--rim_pixels);

			while (lf_x < rt_x)
			{
				DepthArray[TopIndex + lf_x] += crater_delta;
				DepthArray[BotIndex + lf_x] += crater_delta;
				DepthArray[TopIndex + rt_x] += crater_delta;
				DepthArray[BotIndex + rt_x] += crater_delta;
				++lf_x;
				--rt_x;
			}

			if (lf_x == rt_x)
			{
				DepthArray[TopIndex + lf_x] += crater_delta;
				DepthArray[BotIndex + lf_x] += crater_delta;
			}
		}
		
		if (d < 0)
		{
			++x;
			dx += TwoBsquared;
			d += dx;
		}

		rim_pixels = 1;
		--y;
		TopIndex += width;
		BotIndex -= width;
		dy -= TwoAsquared;
		d += Asquared - dy;
	}

	lf_x = A - x;
	rt_x = A + x;
	if (SetDepth)
		memset (&DepthArray[TopIndex + lf_x], 0, rt_x - lf_x + 1);
	if (lf_x == rt_x)
	{
		DepthArray[TopIndex + lf_x] += rim_delta;
	}
	else
	{
		do
		{
			DepthArray[TopIndex + lf_x] += rim_delta;
			if (lf_x != rt_x)
				DepthArray[TopIndex + rt_x] += rim_delta;
			++lf_x;
			--rt_x;
		} while (--rim_pixels);

		while (lf_x < rt_x)
		{
			DepthArray[TopIndex + lf_x] += crater_delta;
			DepthArray[TopIndex + rt_x] += crater_delta;
			++lf_x;
			--rt_x;
		}

		if (lf_x == rt_x)
		{
			DepthArray[TopIndex + lf_x] += crater_delta;
		}
	}
}

#define NUM_BAND_COLORS 4

static void
MakeStorms (COUNT storm_count, SBYTE *DepthArray, COUNT width,
		COUNT height)
{
#define MAX_STORMS 8
	COUNT i;
	RECT storm_r[MAX_STORMS];
	RECT *pstorm_r;

	pstorm_r = &storm_r[i = storm_count];
	while (i--)
	{
		BOOLEAN intersect;
		DWORD rand_val;
		UWORD loword, hiword;
		SIZE band_delta;

		--pstorm_r;
		do
		{
			COUNT j;

			intersect = FALSE;

			rand_val = RandomContext_Random (SysGenRNG);
			loword = LOWORD (rand_val);
			hiword = HIWORD (rand_val);
			switch (HIBYTE (hiword) & 31)
			{
				case 0:
					pstorm_r->extent.height =
							(LOBYTE (hiword) % (ORIGINAL_MAP_HEIGHT >> 2))
							+ (ORIGINAL_MAP_HEIGHT >> 2);
					break;
				case 1:
				case 2:
				case 3:
				case 4:
					pstorm_r->extent.height =
							(LOBYTE (hiword) % (ORIGINAL_MAP_HEIGHT >> 3))
							+ (ORIGINAL_MAP_HEIGHT >> 3);
					break;
				default:
					pstorm_r->extent.height =
							(LOBYTE (hiword) % (ORIGINAL_MAP_HEIGHT >> 4))
							+ 4;
					break;
			}

			if (pstorm_r->extent.height <= 4)
				pstorm_r->extent.height += 4;

			rand_val = RandomContext_Random (SysGenRNG);
			loword = LOWORD (rand_val);
			hiword = HIWORD (rand_val);

			pstorm_r->extent.width = pstorm_r->extent.height
					+ (LOBYTE (loword) % pstorm_r->extent.height);

			pstorm_r->corner.x = HIBYTE (loword)
					% (ORIGINAL_MAP_WIDTH - pstorm_r->extent.width);
			pstorm_r->corner.y = LOBYTE (loword)
					% (ORIGINAL_MAP_HEIGHT - pstorm_r->extent.height);

			pstorm_r->corner.x = pstorm_r->corner.x * width
					/ ORIGINAL_MAP_WIDTH;
			pstorm_r->extent.width = pstorm_r->extent.width * width
					/ ORIGINAL_MAP_WIDTH;
			pstorm_r->corner.y = pstorm_r->corner.y * height
					/ ORIGINAL_MAP_HEIGHT;
			pstorm_r->extent.height = pstorm_r->extent.height * height
					/ ORIGINAL_MAP_HEIGHT;

			for (j = i + 1; j < storm_count; ++j)
			{
				COORD x, y;
				SIZE w, h;

				x = storm_r[j].corner.x - pstorm_r->corner.x;
				y = storm_r[j].corner.y - pstorm_r->corner.y;
				w = x + storm_r[j].extent.width + 4;
				h = y + storm_r[j].extent.height + 4;
				intersect = (BOOLEAN) (w > 0 && h > 0
						&& x < pstorm_r->extent.width + 4
						&& y < pstorm_r->extent.height + 4);
				if (intersect)
					break;
			}

		} while (intersect);

		MakeCrater (pstorm_r, DepthArray, 6, 6, FALSE, width);
		++pstorm_r->corner.x;
		++pstorm_r->corner.y;
		pstorm_r->extent.width -= 2;
		pstorm_r->extent.height -= 2;

		band_delta = HIBYTE (loword) & ((3 << RANGE_SHIFT) + 20);

		MakeCrater (pstorm_r, DepthArray,
				band_delta, band_delta, TRUE, width);
		++pstorm_r->corner.x;
		++pstorm_r->corner.y;
		pstorm_r->extent.width -= 2;
		pstorm_r->extent.height -= 2;

		band_delta += 2;
		if (pstorm_r->extent.width > 2 && pstorm_r->extent.height > 2)
		{
			MakeCrater (pstorm_r, DepthArray,
					band_delta, band_delta, TRUE, width);
			++pstorm_r->corner.x;
			++pstorm_r->corner.y;
			pstorm_r->extent.width -= 2;
			pstorm_r->extent.height -= 2;
		}

		band_delta += 2;
		if (pstorm_r->extent.width > 2 && pstorm_r->extent.height > 2)
		{
			MakeCrater (pstorm_r, DepthArray,
					band_delta, band_delta, TRUE, width);
			++pstorm_r->corner.x;
			++pstorm_r->corner.y;
			pstorm_r->extent.width -= 2;
			pstorm_r->extent.height -= 2;
		}

		band_delta += 4;
		MakeCrater (pstorm_r, DepthArray,
				band_delta, band_delta, TRUE, width);
	}
}

static void
MakeGasGiant (COUNT num_bands, SBYTE *DepthArray, RECT *pRect, SIZE
		depth_delta)
{
	COORD last_y, next_y;
	SIZE band_error, band_bump, band_delta;
	COUNT i, j, band_height;
	SBYTE *lpDst;
	UWORD loword, hiword;
	DWORD rand_val;

	band_height = ORIGINAL_MAP_HEIGHT / num_bands;
	band_bump = ORIGINAL_MAP_HEIGHT % num_bands;
	band_error = num_bands >> 1;
	lpDst = DepthArray;

	band_delta = ((LOWORD (RandomContext_Random (SysGenRNG))
			& (NUM_BAND_COLORS - 1)) << RANGE_SHIFT)
			+ (1 << (RANGE_SHIFT - 1));
	last_y = next_y = 0;
	for (i = num_bands; i > 0; --i)
	{
		COORD cur_y;

		rand_val = RandomContext_Random (SysGenRNG);
		loword = LOWORD (rand_val);
		hiword = HIWORD (rand_val);

		next_y += band_height;
		if ((band_error -= band_bump) < 0)
		{
			++next_y;
			band_error += num_bands;
		}
		if (i == 1)
			cur_y = pRect->extent.height;
		else
		{
			RECT r;

			cur_y = next_y
					+ ((band_height - 2) >> 1)
					- ((LOBYTE (hiword) % (band_height - 2)) + 1);
			cur_y = cur_y * pRect->extent.height / ORIGINAL_MAP_HEIGHT;
			r.corner.x = r.corner.y = 0;
			r.extent.width = pRect->extent.width;
			r.extent.height = 5 * pRect->extent.height
					/ ORIGINAL_MAP_HEIGHT;
			DeltaTopography (50,
					&DepthArray[(cur_y - (r.extent.height >> 1))
						* r.extent.width],
					&r, depth_delta);
		}

		for (j = cur_y - last_y; j > 0; --j)
		{
			COUNT k;

			for (k = pRect->extent.width; k > 0; --k)
				*lpDst++ += band_delta;
		}

		last_y = cur_y;
		band_delta = (band_delta
				+ ((((LOBYTE (loword) & 1) << 1) - 1) << RANGE_SHIFT))
				& (((1 << RANGE_SHIFT) * NUM_BAND_COLORS) - 1);
	}

	MakeStorms (4 + (RandomContext_Random (SysGenRNG) & 3) + 1,
			DepthArray, pRect->extent.width, pRect->extent.height);

	DitherMap (DepthArray, pRect->extent.width, pRect->extent.height);
}

static void
ValidateMap (SBYTE *DepthArray, COUNT width, COUNT height)
{
	BYTE state;
	BYTE pixel_count[2], lb[2];
	SBYTE last_byte;
	DWORD i;
	SBYTE *lpDst;

	i = width - 1;
	lpDst = DepthArray;
	last_byte = *lpDst++;
	state = pixel_count[0] = pixel_count[1] = 0;
	do
	{
		if (pixel_count[state]++ == 0)
			lb[state] = last_byte;

		if (last_byte > *lpDst)
		{
			if (last_byte - *lpDst > 128)
				state ^= 1;
		}
		else
		{
			if (*lpDst - last_byte > 128)
				state ^= 1;
		}
		last_byte = *lpDst++;
	} while (--i);

	i = width * height;
	lpDst = DepthArray;
	if (pixel_count[0] > pixel_count[1])
		last_byte = lb[0];
	else
		last_byte = lb[1];
	do
	{
		if (last_byte > *lpDst)
		{
			if (last_byte - *lpDst > 128)
				*lpDst = last_byte;
		}
		else
		{
			if (*lpDst - last_byte > 128)
				*lpDst = last_byte;
		}
		last_byte = *lpDst++;
	} while (--i);
}

static void
planet_orbit_init (COUNT width, COUNT height, BOOLEAN forOrbit)
{
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;
	COUNT spherespanx = height;
	COUNT shieldradius = (height >> 1) * SHIELD_RADIUS / RADIUS;
	COUNT shielddiam = (shieldradius << 1) + 1;
	COUNT diameter = height + 1;
	COUNT i;

	// always needed
	Orbit->lpTopoData = HCalloc (width * height);

	{
		Orbit->ObjectFrame = 0;

		// tints for 3DO scan
		if (forOrbit && optScanStyle != OPT_PC)
			Orbit->TintFrame = CaptureDrawable (CreateDrawable (
					WANT_PIXMAP, width, height, 1));

		Orbit->TintColor = BLACK_COLOR;

		Orbit->TopoColors = NULL;
		Orbit->ScanColors = NULL;

		Orbit->ScratchArray = HMalloc (sizeof (Orbit->ScratchArray[0])
				* (shielddiam) * (shielddiam));

		Orbit->WorkFrame = 0;
		Orbit->BackFrame = 0;

		Orbit->Shade = 0;
		Orbit->ShadeColors = NULL;

		Orbit->TopoMask = 0;
		Orbit->sphereBytes = NULL;
		Orbit->sphereMap = 0;
		Orbit->scanType = NUM_SCAN_TYPES;
	}
	
	if (!forOrbit || optScanSphere)
	{	// UQM & 3DO sphere
		Orbit->SphereFrame = CaptureDrawable (CreateDrawable (
				WANT_PIXMAP | WANT_ALPHA, diameter, diameter, 2));

		Orbit->TopoColors = HMalloc (sizeof (Orbit->TopoColors[0])
				* (height * (width + spherespanx)));

		if (forOrbit && isPC (optScanStyle) && isPC (optTintPlanSphere)
				&& !use3DOSpheres)
		{	// generate only on that conditions and then use if not NULL
			Orbit->ScanColors =
				HMalloc (sizeof (Color*) * NUM_SCAN_TYPES);
			for (i = 0; i < NUM_SCAN_TYPES; i++)
			{
				Orbit->ScanColors[i] =
					HMalloc (sizeof (Orbit->ScanColors[0][0])
						* (height * (width + spherespanx)));
			}
		}

		if (!use3DOSpheres)
			Orbit->light_diff = HMalloc (sizeof (DWORD *) * diameter);
		else
			Orbit->light_diff = NULL;

		Orbit->map_rotate = HMalloc (sizeof (MAP3D_POINT *) * diameter);

		for (i = 0; i < diameter; i++)
		{
			if (!use3DOSpheres)
				Orbit->light_diff[i] = HMalloc (sizeof (DWORD) * diameter);
			Orbit->map_rotate[i] =
					HMalloc (sizeof (MAP3D_POINT) * diameter);
		}

		if (use3DOSpheres)
		{
			Orbit->Shade =
					CaptureDrawable (LoadGraphic (PLANET_MASK_SHADE));

			Orbit->ShadeColors = Orbit->ShadeColors =
					HMalloc (sizeof (Color) * GetFrameWidth (Orbit->Shade)
					* GetFrameHeight (Orbit->Shade));
		}
	}
	else
	{	// DOS sphere
		Orbit->SphereFrame = 
			CaptureDrawable (LoadGraphic (DOS_PLANET_MASK_ANIM));

		Orbit->TopoMask = CaptureDrawable (CreateDrawable (
				WANT_PIXMAP, (SIZE)width, (SIZE)height, 1));

		Orbit->sphereBytes = HMalloc (sizeof (BYTE)
				* GetFrameWidth (Orbit->SphereFrame)
				* GetFrameHeight (Orbit->SphereFrame));

		Orbit->sphereMap =
			CaptureColorMap (LoadColorMap (DOS_SPHERE_COLOR_TAB));
		SetColorMap (GetColorMapAddress (Orbit->sphereMap));
	}
}

static unsigned
frandom (void)
{
	static unsigned seed = 0x12345678;
	
	if (seed == 0)
		seed = 15807;
	seed = (seed >> 4) * 227;

	return seed;
}

static inline int
TopoVarianceFactor (int step, int allowed, int min)
{
#define SCALE_SHIFT 8
	return ((abs(step) * allowed) >> SCALE_SHIFT) + min;
}

static inline int
TopoVarianceCalc (int factor)
{
	if (factor == 0)
		return 0;
	else
		return (frandom () % factor) - (factor >> 1);
}

static void
TopoScale4x (SBYTE *pDstTopo, SBYTE *pSrcTopo, int num_faults,
		int fault_var)
{
		// Interpolate the topographical data by connecting the elevations
		// to their nearest neighboors using straight lines (in random
		// direction) with random variance factors defined by
		// num_faults and fault_var args
#define AVG_VARIANCE 250
	int x, y;
	const int w = SCALED_MAP_WIDTH, h = MAP_HEIGHT;
	const int spitch = SCALED_MAP_WIDTH, dpitch = SCALED_MAP_WIDTH * 4;
	SBYTE *pSrc;
	SBYTE *pDst;
	int* prevrow;
	int* prow;
	int elev[5][5];
	int var_allow, var_min;
	static const struct line_def_t
	{
		int x0, y0, x1, y1;
		int dx, dy;
	}
	fill_lines[4][6] = 
	{
		{	// diag set 0
			{ 0,  2,  2,  0,  1, -1},
			{ 0,  3,  3,  0,  1, -1},
			{ 0,  4,  4,  0,  1, -1},
			{ 1,  4,  4,  1,  1, -1},
			{ 2,  4,  4,  2,  1, -1},
			{-1, -1, -1, -1,  0,  0}, // term
		},
		{	// diag set 1
			{ 0,  2,  2,  4,  1,  1},
			{ 0,  1,  3,  4,  1,  1},
			{ 0,  0,  4,  4,  1,  1},
			{ 1,  0,  4,  3,  1,  1},
			{ 2,  0,  4,  2,  1,  1},
			{-1, -1, -1, -1,  0,  0}, // term
		},
		{	// horizontal
			{ 0,  1,  4,  1,  1,  0},
			{ 0,  2,  4,  2,  1,  0},
			{ 0,  3,  4,  3,  1,  0},
			{-1, -1, -1, -1,  0,  0}, // term
		},
		{	// vertical
			{ 1,  0,  1,  4,  0,  1},
			{ 2,  0,  2,  4,  0,  1},
			{ 3,  0,  3,  4,  0,  1},
			{-1, -1, -1, -1,  0,  0}, // term
		},
	};
	
	prevrow = (int *) HMalloc ((SCALED_MAP_WIDTH * 4 + 1)
			* sizeof(prevrow[0]));

	var_allow = (num_faults << SCALE_SHIFT) / AVG_VARIANCE;
	var_min = fault_var << SCALE_SHIFT;

	//memset (pDstTopo, 0, MAP_WIDTH * MAP_HEIGHT * 16);

	// init the first row in advance
	pSrc = pSrcTopo;
	prow = prevrow;
#define STEP_RANGE (4 - 1)
	prow[0] = ((int)pSrc[0]) << SCALE_SHIFT;
	for (x = 0; x < w; ++x, ++pSrc, prow += 4)
	{
		int x2;
		int val, step, rndfact;

		// next point in row
		if (x < w - 1)
			// one right
			prow[4] = ((int)pSrc[1]) << SCALE_SHIFT;
		else
			// wrap around
			prow[4] = ((int)pSrc[1 - spitch]) << SCALE_SHIFT;

		// compute elevations between 2 points
		val = prow[0];
		step = (prow[4] - val) / STEP_RANGE;
		rndfact = TopoVarianceFactor (step, var_allow, var_min);
		for (x2 = 1, val += step; x2 < 4; ++x2, val += step)
			prow[x2] = val + TopoVarianceCalc (rndfact);
	}

	pSrc = pSrcTopo;
	pDst = pDstTopo;
	for (y = 0; y < h; ++y, pDst += dpitch * 3)
	{
		int x2, y2;
		SBYTE *p;
		int val, step, rndfact;
		const struct line_def_t* pld;

		prow = prevrow;
		// prime the first interpolated column
		elev[4][0] = prow[0];
		if (y < h - 1)
			elev[4][4] = ((int)pSrc[spitch]) << SCALE_SHIFT;
		else
			elev[4][4] = elev[4][0];
		// compute elevations for interpolated column
		val = elev[4][0];
		step = (elev[4][4] - val) / STEP_RANGE;
		rndfact = TopoVarianceFactor (step, var_allow, var_min);
		for (y2 = 1, val += step; y2 < 4; ++y2, val += step)
			elev[4][y2] = val + TopoVarianceCalc (rndfact);

		for (x = 0; x < w; ++x, ++pSrc, pDst += 4, prow += 4)
		{
			// recall the first interpolated row from prevrow
			for (x2 = 0; x2 <= 4; ++x2)
				elev[x2][0] = prow[x2];
			// recall the first interpolated column
			for (y2 = 1; y2 <= 4; ++y2)
				elev[0][y2] = elev[4][y2];
			
			if (y < h - 1)
			{
				if (x < w - 1)
					// one right, one down
					elev[4][4] = ((int)pSrc[1 + spitch]) << SCALE_SHIFT;
				else
					// wrap around, one down
					elev[4][4] = ((int)pSrc[1]) << SCALE_SHIFT;
			}
			else
			{
				elev[4][4] = elev[4][0];
			}

			// compute elevations for the rest of square borders first
			val = elev[0][4];
			step = (elev[4][4] - val) / STEP_RANGE;
			rndfact = TopoVarianceFactor (step, var_allow, var_min);
			for (x2 = 1, val += step; x2 < 4; ++x2, val += step)
				elev[x2][4] = val + TopoVarianceCalc (rndfact);

			val = elev[4][0];
			step = (elev[4][4] - val) / STEP_RANGE;
			rndfact = TopoVarianceFactor (step, var_allow, var_min);
			for (y2 = 1, val += step; y2 < 4; ++y2, val += step)
				elev[4][y2] = val + TopoVarianceCalc (rndfact);

			// fill in the rest by connecting opposing elevations
			// some randomness to determine which elevations to connect
			for (pld = fill_lines[frandom () & 3]; pld->x0 >= 0; ++pld)
			{
				int num_steps;

				x2 = pld->x0;
				y2 = pld->y0;
				val = elev[x2][y2];
				num_steps = pld->x1 - pld->x0;
				if (num_steps == 0)
					num_steps = pld->y1 - pld->y0;
				step = (elev[pld->x1][pld->y1] - val) / num_steps;
				rndfact = TopoVarianceFactor (step, var_allow, var_min);
				
				for (x2 += pld->dx, y2 += pld->dy, val += step;
						x2 != pld->x1 || y2 != pld->y1;
						x2 += pld->dx, y2 += pld->dy, val += step)
				{
					elev[x2][y2] = val + TopoVarianceCalc (rndfact);
				}
			}

			// output the interpolated topography
			for (y2 = 0; y2 < 4; ++y2)
			{
				p = pDst + y2 * dpitch;
				for (x2 = 0; x2 < 4; ++x2, ++p)
				{
					int e = elev[x2][y2] >> SCALE_SHIFT;
					if (e > 127)
						e = 127;
					else if (e < -128)
						e = -128;
					*p = (SBYTE)e;
				}
			}

			// save last interpolated row to prevrow for later
			for (x2 = 0; x2 < 4; ++x2)
				prow[x2] = elev[x2][4];
		}
		// save last row point
		prow[0] = elev[4][4];
	}
	
	HFree (prevrow);
}


// GenerateLightMap produces a surface light variance map for the
//  rotating planet by, first, transforming absolute elevation data
//  into normalized relative and then applying a weighted
//  average-median of surrounding points
// Lots of pure Voodoo here ;)
//  the goal is a 3D illusion, not mathematically correct lighting

#define LMAP_AVG_BLOCK       ((SC2_MAP_HEIGHT + 64) / 5)
#define LMAP_MAX_DIST        ((LMAP_AVG_BLOCK + 1) >> 1)
#define LMAP_WEIGHT_THRES    (LMAP_MAX_DIST * 2 / 3)

typedef struct
{
	int min;
	int max;
	int avg;

} elev_block_t;

static inline void
get_vblock_avg (elev_block_t *pblk, SBYTE *pTopo, int x, int y,
		COUNT width, COUNT height)
{
	SBYTE *elev = pTopo;
	int y0, y1, i;
	int min = 127, max = -127;
	int avg = 0, total_weight = 0;

	// surface wraps around along x
	x = (x + width) % width;
	
	y0 = y - LMAP_MAX_DIST;
	y1 = y + LMAP_MAX_DIST;
	if (y0 < 0)
		y0 = 0;
	if (y1 > height)
		y1 = height;

	elev = pTopo + y0 * height + x;
	for (i = y0; i < y1; ++i, elev += height)
	{
		int delta = abs (i - y);
		int weight = 255; // full weight
		int v = *elev;

		if (delta >= LMAP_WEIGHT_THRES)
		{	// too far -- progressively reduced weight
			weight = weight * (LMAP_MAX_DIST - delta + 1)
					/ (LMAP_MAX_DIST - LMAP_WEIGHT_THRES + 2);
		}

		if (v > max)
			max = v;
		if (v < min)
			min = v;
		avg += pblk->avg * weight;
		total_weight += weight;
	}
	avg /= total_weight;

	pblk->min = min;
	pblk->max = max;
	pblk->avg = avg / (y1 - y0);
}

// See description above
static void
GenerateLightMap (SBYTE *pTopo, int w, int h)
{
#define LMAP_BLOCKS       (2 * LMAP_MAX_DIST + 1)
	int x, y;
	elev_block_t vblocks[LMAP_BLOCKS];
			// we use a running block average to reduce the amount of work
			// where a block is a vertical line of map points
	SBYTE *elev;
	int min, max, med;
	int sfact, spread;

	// normalize the topo data
	min = 127;
	max = -128;
	for (elev = pTopo; elev != pTopo + w * h; ++elev)
	{
		// Louis Delacroix: Bug#1151
		if (*elev > max)
			max = *elev;
		if (*elev < min)
			min = *elev;
	}
	med = (min + max) / 2;
	spread = max - med;

	if (spread == 0)
	{	// perfectly smooth surface -- nothing to do but
		// level it out completely
		if (max != 0)
			memset (pTopo, 0, w * h);
		return;
	}

	// these are whatever looks right
	if (spread < 10)
		sfact = 30; // minimal spread
	else if (spread < 30)
		sfact = 60;
	else
		sfact = 100; // full spread
	
	// apply spread
	for (elev = pTopo; elev != pTopo + w * h; ++elev)
	{
		// Louis Delacroix: Bug#1151
		*elev = (*elev - med) * sfact / spread;
	}

	// compute and apply weighted averages of surrounding points
	for (y = 0, elev = pTopo; y < h; ++y)
	{
		elev_block_t *pblk;
		int i;

		// prime the running block average
		// get the minimum, maximum and avg elevation for each block
		for (i = -LMAP_MAX_DIST; i < LMAP_MAX_DIST; ++i)
		{
			// blocks wrap around on both sides
			pblk = vblocks + ((i + LMAP_BLOCKS) % LMAP_BLOCKS);

			get_vblock_avg (pblk, pTopo, i, y, w, h);
		}

		for (x = 0; x < w; ++x, ++elev)
		{
			int avg = 0, total_weight = 0;

			min = 127;
			max = -127;

			// prepare next block as we move along x
			pblk = vblocks + ((x + LMAP_MAX_DIST) % LMAP_BLOCKS);
			get_vblock_avg (pblk, pTopo, x + LMAP_MAX_DIST, y, w, h);

			// compute the min, max and weighted avg of blocks
			for (i = x - LMAP_MAX_DIST; i <= x + LMAP_MAX_DIST; ++i)
			{
				int delta = abs (i - x);
				int weight = 255; // full weight

				pblk = vblocks + ((i + LMAP_BLOCKS) % LMAP_BLOCKS);

				if (delta >= LMAP_WEIGHT_THRES)
				{	// too far -- progressively reduced weight
					weight = weight * (LMAP_MAX_DIST - delta + 1)
							/ (LMAP_MAX_DIST - LMAP_WEIGHT_THRES + 2);
				}

				if (pblk->max > max)
					max = pblk->max;
				if (pblk->min < min)
					min = pblk->min;
				
				avg += pblk->avg * weight;
				total_weight += weight;
			}
			avg /= total_weight;

			// This is mostly Voodoo
			// figure out what kind of relative lighting factor
			// to assign to this point
#if 0
			// relative to median
			med = (min + max) / 2; // median
			*elev = (int)*elev - med;
#else
			// relative to median of (average, median)
			med = (min + max) / 2; // median
			med = (med + avg) / 2;
			*elev = (int)*elev - med;
#endif
		}
	}
}

void
load_color_resources (PLANET_DESC *pPlanetDesc, PlanetFrame *PlanDataPtr,
	PLANET_INFO *PlanetInfo, BOOLEAN dosshielded, BOOLEAN ForIP)
{
	if (CheckColorMap (pPlanetDesc->alternate_colormap)
			&& !useDosSpheres && !ForIP)
	{	// JMS: Planets with special colormaps
		pSolarSysState->OrbitalCMap = CaptureColorMap (
				LoadColorMap (pPlanetDesc->alternate_colormap));
		pSolarSysState->XlatRef = CaptureStringTable (
				LoadStringTable (SPECIAL_CMAP_XLAT_TAB));
	}
	else
	{	// JMS: Normal planets
		pSolarSysState->OrbitalCMap = CaptureColorMap (
				LoadColorMap (dosshielded ? DOS_SHIELDED_COLOR_TAB
					: PlanDataPtr->CMapInstance));
		pSolarSysState->XlatRef = CaptureStringTable (
				LoadStringTable (PlanDataPtr->XlatTabInstance));

		if (PlanetInfo->SurfaceTemperature > HOT_THRESHOLD)
		{
			pSolarSysState->OrbitalCMap = SetAbsColorMapIndex (
					pSolarSysState->OrbitalCMap, 2);
			pSolarSysState->XlatRef = SetAbsStringTableIndex (
					pSolarSysState->XlatRef, 2);
		}
		else if (PlanetInfo->SurfaceTemperature > COLD_THRESHOLD)
		{
			pSolarSysState->OrbitalCMap = SetAbsColorMapIndex (
					pSolarSysState->OrbitalCMap, 1);
			pSolarSysState->XlatRef = SetAbsStringTableIndex (
					pSolarSysState->XlatRef, 1);
		}
	}
	pSolarSysState->XlatPtr = GetStringAddress (pSolarSysState->XlatRef);
}

void
generate_surface_frame (COUNT width, COUNT height, PLANET_ORBIT *Orbit,
	PlanetFrame *PlanDataPtr)
{	// Generate planet surface elevation data and look
	RECT r;
	COUNT i;

	r.corner.x = r.corner.y = 0;
	r.extent.width = width;
	r.extent.height = height;

	memset (Orbit->lpTopoData, 0, width * height);
	switch (PLANALGO (PlanDataPtr->Type))
	{
		case GAS_GIANT_ALGO:
			MakeGasGiant (PlanDataPtr->num_faults,
					Orbit->lpTopoData, &r, PlanDataPtr->fault_depth);
			break;
		case TOPO_ALGO:
		case CRATERED_ALGO:
			if (PlanDataPtr->num_faults)
				DeltaTopography (PlanDataPtr->num_faults,
						Orbit->lpTopoData, &r,
						PlanDataPtr->fault_depth);

			for (i = 0; i < PlanDataPtr->num_blemishes; ++i)
			{
				RECT crater_r;
				UWORD loword;

				loword = LOWORD (RandomContext_Random (SysGenRNG));
				switch (HIBYTE (loword) & 31)
				{
					case 0:
						crater_r.extent.width =
								(LOBYTE (loword)
									% (ORIGINAL_MAP_HEIGHT >> 2))
									+ (ORIGINAL_MAP_HEIGHT >> 2);
						break;
					case 1:
					case 2:
					case 3:
					case 4:
						crater_r.extent.width =
								(LOBYTE (loword)
									% (ORIGINAL_MAP_HEIGHT >> 3))
									+ (ORIGINAL_MAP_HEIGHT >> 3);
						break;
					default:
						crater_r.extent.width =
								(LOBYTE (loword)
									% (ORIGINAL_MAP_HEIGHT >> 4))
									+ 4;
						break;
				}

				loword = LOWORD (RandomContext_Random (SysGenRNG));
				crater_r.extent.height = crater_r.extent.width;
				crater_r.corner.x = HIBYTE (loword)
						% (ORIGINAL_MAP_WIDTH
							- crater_r.extent.width);
				crater_r.corner.y = LOBYTE (loword)
						% (ORIGINAL_MAP_HEIGHT
							- crater_r.extent.height);

				// BW: ... then scale them up
				crater_r.extent.width = crater_r.extent.width
						* height / ORIGINAL_MAP_HEIGHT;
				crater_r.extent.height = crater_r.extent.width;
				crater_r.corner.x = crater_r.corner.x
						* width / ORIGINAL_MAP_WIDTH;
				crater_r.corner.y = crater_r.corner.y
						* height / ORIGINAL_MAP_HEIGHT;

				MakeCrater (&crater_r, Orbit->lpTopoData,
						PlanDataPtr->fault_depth << 2,
						-(PlanDataPtr->fault_depth << 2),
						FALSE, width);
			}
			if (PLANALGO (PlanDataPtr->Type) == CRATERED_ALGO)
				DitherMap (Orbit->lpTopoData, width, height);
			ValidateMap (Orbit->lpTopoData, width, height);
			break;
	}
	pSolarSysState->TopoFrame = CaptureDrawable (
			CreateDrawable (WANT_PIXMAP, (SIZE)width,
				(SIZE)height, 1));

	RenderTopography (pSolarSysState->TopoFrame,
			Orbit->lpTopoData, width, height, FALSE, NULL);

}

// Sets the SysGenRNG to the required state first.
void
GeneratePlanetSurface (PLANET_DESC *pPlanetDesc, FRAME SurfDefFrame,
		COUNT width, COUNT height)
{
	const PlanetFrame *PlanDataPtr;
	PLANET_INFO *PlanetInfo = &pSolarSysState->SysInfo.PlanetInfo;
	POINT loc;
	CONTEXT OldContext, TopoContext;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;
	BOOLEAN SurfDef = FALSE;
	BOOLEAN shielded = (pPlanetDesc->data_index & PLANET_SHIELDED) != 0;
	SDWORD PlanetRotation;
	COUNT spherespanx, radius;
	BOOLEAN ForIP;
	BOOLEAN customTexture =
			solTexturesPresent && CurStarDescPtr->Index == SOL_DEFINED;

	if (width == NULL && height == NULL)
	{
		width = SCALED_MAP_WIDTH;
		height = MAP_HEIGHT;
		spherespanx = SPHERE_SPAN_X;
		radius = RADIUS;
		ForIP = FALSE;

		useDosSpheres = optScanSphere == 0;
		use3DOSpheres = optScanSphere == 1;
	}
	else
	{
		spherespanx = height;
		radius = (height >> 1) - IF_HD (2);
		ForIP = TRUE;

		useDosSpheres = FALSE;
		use3DOSpheres = FALSE;
	}

	actuallyInOrbit = !ForIP;

	RandomContext_SeedRandom (SysGenRNG, pPlanetDesc->rand_seed);

	TopoContext = CreateContext ("Plangen.TopoContext");
	OldContext = SetContext (TopoContext);
	
	planet_orbit_init (width, height, !ForIP);

	PlanDataPtr = &PlanData[pPlanetDesc->data_index & ~PLANET_SHIELDED];

	load_color_resources (
			pPlanetDesc, PlanDataPtr, PlanetInfo,
			shielded && useDosSpheres, ForIP);

	if (SurfDefFrame)
	{	// This is a defined planet; pixmap for the topography and
		// elevation data is supplied in Surface Definition frame
		BOOLEAN DeleteDef = FALSE;
		BOOLEAN DeleteElev = FALSE;
		FRAME ElevFrame;
		COUNT index = 0;

		// load special frame to render Earth with DOS spheres on
		if (GetFrameCount (SurfDefFrame) == 4 && useDosSpheres && !ForIP)
			index = 2;

		// surface pixmap
		SurfDef = TRUE;
		SurfDefFrame = SetAbsFrameIndex (SurfDefFrame, index);
		if (GetFrameWidth (SurfDefFrame) != width
				|| GetFrameHeight (SurfDefFrame) != height)
		{
			pSolarSysState->TopoFrame = CaptureDrawable (RescaleFrame (
					SurfDefFrame, width, height));
			// will not need the passed FRAME anymore
			DeleteDef = TRUE;
		}
		else
			pSolarSysState->TopoFrame = SurfDefFrame;

		if (GetFrameCount (SurfDefFrame) > 1)
		{	// 2nd frame is elevation data 
			int i;
			SBYTE* elev;

			ElevFrame = SetAbsFrameIndex (SurfDefFrame, index + 1);
			if (GetFrameWidth (ElevFrame) != width
					|| GetFrameHeight (ElevFrame) != height)
			{	// Should ALWAYS be paletted
				ElevFrame = CaptureDrawable (
								RescaleFrame (ElevFrame, width, height));
				DeleteElev = TRUE;
			}

			// grab the elevation data in 1 byte per pixel format
			ReadFramePixelIndexes (ElevFrame, (BYTE *)Orbit->lpTopoData,
				width, height, !ForIP);
			// the supplied data is in unsigned format, must convert
			for (i = 0, elev = Orbit->lpTopoData;
					i < width * height;
					++i, ++elev)
			{
				*elev = *(BYTE *)elev - 128;
			}
		}
		else
		{	// no elevation data -- planet flat as a pancake
			memset (Orbit->lpTopoData, 0, width * height);
		}

		if (DeleteDef)
			DestroyDrawable (ReleaseDrawable (SurfDefFrame));
		if (DeleteElev)
			DestroyDrawable (ReleaseDrawable (ElevFrame));
	}
	else
	{	
		generate_surface_frame (width, height, Orbit, PlanDataPtr);
	}

	if (!ForIP && useDosSpheres)
	{		
		RenderLevelMasks (Orbit->TopoMask, Orbit->lpTopoData, SurfDef);
		SetPlanetColors (GetColorMapAddress (Orbit->sphereMap));
		if (Orbit->TopoMask != NULL)
			ExpandLevelMasks (Orbit);
		else
			log_add (log_Warning, "No planet mask generated.\n");
	}

	if (!ForIP && optScanStyle == OPT_PC && !shielded)
	{
		COUNT i;

		if (SurfDef)
		{
			for (i = 0; i < NUM_SCAN_TYPES; i++)
			{
				COUNT x, y;
				Color *pix;
				Color *map;

				pSolarSysState->ScanFrame[i] = CaptureDrawable (
					CreateDrawable (WANT_PIXMAP, (SIZE)width,
					(SIZE)height, 1));

				map = HMalloc (sizeof (Color) * width * height);
				ReadFramePixelColors (
					pSolarSysState->TopoFrame, map, width, height);

				pix = map;

				for (y = 0; y < height; ++y)
				{
					for (x = 0; x < width; ++x, ++pix)
					{
						TransformColor (pix, i);
					}
				}

				WriteFramePixelColors (
					pSolarSysState->ScanFrame[i], map, width, height);
				HFree (map);
			}
		}
		else
		{
			COLORMAP scanTable;

			scanTable = CaptureColorMap (LoadColorMap (SCAN_COLOR_TAB));

			for (i = 0; i < NUM_SCAN_TYPES; i++)
			{
				pSolarSysState->ScanFrame[i] = CaptureDrawable (
					CreateDrawable (WANT_PIXMAP, (SIZE)width,
					(SIZE)height, 1));

				scanTable = SetAbsColorMapIndex (scanTable, i);

				RenderTopography (pSolarSysState->ScanFrame[i],
						Orbit->lpTopoData, width, height, FALSE, scanTable);
			}

			DestroyColorMap (ReleaseColorMap (scanTable));
			scanTable = 0;
		}
	}

	if (!ForIP && !shielded
			&& PlanetInfo->AtmoDensity != GAS_GIANT_ATMOSPHERE)
	{	// produce 4x scaled topo image for Planetside
		// for the planets that we can land on

		if (isPC (optSuperPC) && !IS_HD && !SurfDefFrame)
		{	// crispy PC-DOS landscape
			Orbit->TopoZoomFrame = CaptureDrawable (
					RescaleFrame (
						pSolarSysState->TopoFrame,
						width << 2, height << 2));
		}
		else
		{	// usual smooth 3DO landscape
			SBYTE* pScaledTopo = HMalloc (
					SCALED_MAP_WIDTH * 4 * MAP_HEIGHT * 4);

			Orbit->TopoZoomFrame = CaptureDrawable (CreateDrawable (
				WANT_PIXMAP, width << 2, height << 2, 1));

			if (pScaledTopo)
			{
				TopoScale4x (pScaledTopo, Orbit->lpTopoData,
						PlanDataPtr->num_faults, PlanDataPtr->fault_depth
						* (PLANALGO (
							PlanDataPtr->Type) == CRATERED_ALGO ? 2 : 1 ));
				RenderTopography (Orbit->TopoZoomFrame, pScaledTopo,
						SCALED_MAP_WIDTH * 4, MAP_HEIGHT * 4, SurfDef, NULL
					);

				HFree (pScaledTopo);
			}
		}
	}

	if (Orbit->TopoColors)
	{	// Generate a pixel array from the Topography map.
		// We use this instead of lpTopoData because it needs to be
		// WAP_WIDTH+SPHERE_SPAN_X wide and we need this method for Earth
		// anyway. It may be more efficient to build it from lpTopoData
		// instead of the FRAMPTR though.
		DWORD y;

		ReadFramePixelColors(pSolarSysState->TopoFrame, Orbit->TopoColors,
				width + spherespanx, height);
		// Extend the width from MAP_WIDTH to MAP_WIDTH+SPHERE_SPAN_X
		for (y = 0; y < (DWORD)(height * (width + spherespanx));
			y += width + spherespanx)
			memcpy(Orbit->TopoColors + y + width, Orbit->TopoColors + y,
					spherespanx * sizeof(Orbit->TopoColors[0]));
	}

	if (Orbit->ScanColors)
	{	// prepare colors for every scan tint if we ever created them
		COUNT i;
		DWORD y;

		for (i = 0; i < NUM_SCAN_TYPES; i++)
		{
			ReadFramePixelColors (pSolarSysState->ScanFrame[i],
					Orbit->ScanColors[i], width + spherespanx, height);

			for (y = 0; y < (DWORD)(height * (width + spherespanx));
					y += width + spherespanx)
			{
				memcpy (Orbit->ScanColors[i] + y + width,
						Orbit->ScanColors[i] + y,
						spherespanx * sizeof(Orbit->ScanColors[0][0]));
			}
		}
	}

	if (PLANALGO (PlanDataPtr->Type) == GAS_GIANT_ALGO
			|| (customTexture && use3DOSpheres))
	{	// convert topo data to a light map, based on relative
		// map point elevations
		memset (Orbit->lpTopoData, 0, width * height);
	}
	else
		GenerateLightMap (Orbit->lpTopoData, width, height);

	if (pSolarSysState->pOrbitalDesc->pPrevDesc ==
			&pSolarSysState->SunDesc[0])
	{	// this is a planet -- get its location
		loc = pSolarSysState->pOrbitalDesc->location;
	}
	else
	{	// this is a moon -- get its planet's location
		loc = pSolarSysState->pOrbitalDesc->pPrevDesc->location;
	}
	
	// Rotating planet sphere initialization
	if (!(useDosSpheres || use3DOSpheres) || ForIP)
	{
		GenerateSphereMask (loc, radius);
		CreateSphereTiltMap (PlanetInfo->AxialTilt, height, radius);
	}
	else if (useDosSpheres || use3DOSpheres)
	{
		COUNT facing;
		facing =
				NORMALIZE_FACING (ANGLE_TO_FACING (ARCTAN (loc.x, loc.y)));

		if (useDosSpheres)
		{
			Orbit->SphereFrame =
					SetAbsFrameIndex (Orbit->SphereFrame, facing & 14);
			ReadFramePixelIndexes (Orbit->SphereFrame, Orbit->sphereBytes,
					GetFrameWidth(Orbit->SphereFrame),
					GetFrameHeight(Orbit->SphereFrame), TRUE);
		}
		else if (use3DOSpheres)
		{
			CreateSphereTiltMap (PlanetInfo->AxialTilt, height, radius);
			Orbit->Shade =
					SetAbsFrameIndex (Orbit->Shade, facing);
			ReadFramePixelColors (Orbit->Shade, Orbit->ShadeColors,
					GetFrameWidth (Orbit->Shade),
					GetFrameHeight (Orbit->Shade));
		}
	}
	if (shielded)
	{
		Orbit->ObjectFrame =
				(((useDosSpheres || use3DOSpheres) && !ForIP) ?
					(useDosSpheres ?
					CaptureDrawable (LoadGraphic (DOS_SHIELD_MASK_ANIM)) :
					CaptureDrawable (LoadGraphic (TDO_SHIELD_MASK_ANIM)))
					: CreateShieldMask (radius));

		// Create background frame if we have nebula on
		// but not for IP and if we're using DOS shield
		if ((optNebulae || use3DOSpheres) && !ForIP && !useDosSpheres)
			Orbit->BackFrame = SaveBackFrame (radius);
	}

	
	PlanetRotation = (CurStarDescPtr->Index == SOL_DEFINED ? -1
			: 1 - 2 * (PlanetInfo->AxialTilt & 1));

	InitSphereRotation (PlanetRotation, shielded, width, height);
	
	if (ForIP)
	{
		pPlanetDesc->rotwidth = width;
		pPlanetDesc->rotheight = height;
		pPlanetDesc->rotFrameIndex = 0;
		pPlanetDesc->rotPointIndex = 0;
		pPlanetDesc->rot_speed =
				((double)(pPlanetDesc->rotwidth * PlanetRotation * 240))
				/ PlanetInfo->RotationPeriod;
	}
	
	if (shielded && !useDosSpheres)
	{	// This overwrites pSolarSysState->TopoFrame, so everything that
		// needs it has to come before
		ApplyShieldTint ();
	}

	SetContext (OldContext);
	DestroyContext (TopoContext);
}