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

#ifndef UQM_JOURNAL_H
#define UQM_JOURNAL_H

static char* JournalStrings[] = {
	"",

	"Return to Earth",
	"Contact Earth Starbase",
	"Acquire radioactives for Earth Starbase",
	"Give radioactives to Earth Starbase",
	"Request replacement planet lander from Earth Starbase",
	"Request another replacement planet lander from Earth Starbase",
	"Request fuel from Earth Starbase",
	"Request fuel again from Earth Starbase",
	"Neutralize Hierarchy base on Luna",
	"Inform Earth Starbase about Hierarchy base on Luna",
	"Convince Earth Starbase to join the fight against the Ur-Quan"
};

enum
{
	NO_JOURNAL_ENTRY,

	VISIT_EARTH,
	CONTACT_EARTH,
	GET_RADIOACTIVES,
	GIVE_RADIOACTIVES,
	NEED_LANDER,
	NEED_LANDER_AGAIN,
	NEED_FUEL,
	NEED_FUEL_AGAIN,
	DESTROY_MOONBASE,
	REPORT_MOONBASE,
	RECRUIT_EARTH
};

#endif /* UQM_JOURNAL_H */
