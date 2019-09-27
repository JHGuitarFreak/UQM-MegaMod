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

#include "../ship.h"
#include "probe.h"
#include "resinst.h"

#define MAX_CREW 1
#define MAX_ENERGY 1
#define ENERGY_REGENERATION 0
#define WEAPON_ENERGY_COST 0
#define SPECIAL_ENERGY_COST 0
#define ENERGY_WAIT 0
#define MAX_THRUST 0
#define THRUST_INCREMENT 0
#define TURN_WAIT 0
#define THRUST_WAIT 0
#define WEAPON_WAIT 0
#define SPECIAL_WAIT 0

#define SHIP_MASS 0

static RACE_DESC probe_desc =
{
	{ /* SHIP_INFO */
		"urquanprobe",
		0,
		0, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		0,
		0,
		URQUAN_DRONE_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		0, /* Initial sphere of influence radius */
		{ /* Known location (center of SoI) */
			0, 0,
		},
	},
	{
		MAX_THRUST,
		THRUST_INCREMENT,
		ENERGY_REGENERATION,
		WEAPON_ENERGY_COST,
		SPECIAL_ENERGY_COST,
		ENERGY_WAIT,
		TURN_WAIT,
		THRUST_WAIT,
		WEAPON_WAIT,
		SPECIAL_WAIT,
		SHIP_MASS,
	},
	{
		{
			NULL_RESOURCE,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			NULL_RESOURCE,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			NULL_RESOURCE,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			NULL_RESOURCE,
			NULL, NULL, NULL, NULL, NULL
		},
		NULL_RESOURCE,
		NULL_RESOURCE,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		0,
		NULL,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
	0, /* CodeRef */
};

RACE_DESC*
init_probe (void)
{
	RACE_DESC *RaceDescPtr;

	RaceDescPtr = &probe_desc;

	return (RaceDescPtr);
}

