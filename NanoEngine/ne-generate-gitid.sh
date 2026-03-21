#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
#
# copied from RefPerSys 
# © Copyright (C) 2020 - 2025, Basile Starynkevitch and the forum@refpersys.org
# mailing list contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

license()
{
    echo this is GPL licensed, see gnu.org/licenses/
    echo 'no warranty'
    exit 0
}

usage()
{
	echo "usage: ./rps-generate-gitid.sh [-hlsv]"
	echo "options:"
	echo "  -h  show usage"
	echo "  -l  show license" 
	echo "  -s  print short git ID" 
	echo "  -v  show version"
	echo "  -v  generate c file"
}

version()
{
	maj="$(grep -R NE_MAJOR_VERSION . \
		| head -n 1 \
		| cut -d ' ' -f 3)"

	min="$(grep -R NE_MINOR_VERSION . \
		| head -n 1 \
		| cut -d ' ' -f 3)"

	echo "v${maj}.${min}"
	exit 0
}

opt()
{
	isshort=0

	while getopts "hlsvc" flag ; do
		case ${flag} in
			h) usage ; exit 0 ;;
			l) license ;;
			s) isshort=1 ;;
			v) version ;;
			c) generate_c ; exit 0;;
			*) usage ; exit 1 ;;
		esac
	done
}

getid()
{
    	gitid="$(git log --format=oneline -q -1 \
		| cut '-d ' -f 1 \
		| tr -d '\n')"
}

isdirty()
{
	if git status | grep -q "nothing to commit" ; then
    		dirty=""
	else
    		dirty="+"
	fi
}

output()
{
	getid
	isdirty

	if [ $isshort -eq 1 ]; then
    		printf "%.12s%s\n" "${gitid}" "${dirty}"
	else
    		printf "%s%s\n" "${gitid}" "${dirty}"
	fi
}

generate_c()
{
	getid
	isdirty
	printf 'const char _ne_short[]="%.12s%s";\n'  "${gitid}" "${dirty}"
	printf 'const char _ne_git[]="%s%s";\n' "${gitid}" "${dirty}"
	printf 'const char _ne_timestamp[]="%s";\n' "$(date)"
	printf 'const long _ne_timelong=%d;\n' "$(date +%s)"
}

main()
{
	opt "$@"
	output
}

main "$@"

