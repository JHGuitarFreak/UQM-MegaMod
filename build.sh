#!/bin/sh
# Build helper
# Copyright (c) 2002 Serge van den Boom
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

# You can start this program as 'SH="/bin/sh -x" ./build.sh' to
# enable command tracing.

if [ -z "$SH" ]; then
	if [ `uname -s` = SunOS ]; then
		# /bin/sh of Solaris is incompatible. Fortunately, Sun ships
		# a decent sh in /usr/xpg4/bin/ nowadays.
		SH=/usr/xpg4/bin/sh
	else
		SH=/bin/sh
	fi
	export SH
fi

$SH build/unix/build.sh "$@"

