#!/usr/bin/gmake
# SPDX-License-Identifier: GPL-3.0-or-later
# file github.com/bstarynk/truc-gpl/NanoEngine/GNUmakefile
# Author:
##      Basile STARYNKEVITCH, 92340 Bourg-la-Reine, France,
##                    <basile@starynkevitch.net>
##                 or <b.starynkevitch@gmail.com>

## License:
##    This program is free software: you can redistribute it and/or modify
##    it under the terms of the GNU General Public License as published by
##    the Free Software Foundation, either version 3 of the License, or
##    (at your option) any later version.
##
##    This program is distributed in the hope that it will be useful,
##    but WITHOUT ANY WARRANTY; without even the implied warranty of
##    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##    GNU General Public License for more details.
##
##    You should have received a copy of the GNU General Public License
##    along with this program.  If not, see <http://www.gnu.org/licenses/>

## tell GNU make to export all variables by default
export


CC= gcc
CFLAGS= -O -g -Wall -Wextra $(pkg-config --cflags gtk4)
LIBS= $(pkg-config --libs gtk4)

NE_GIT_ID:= $(shell ./ne-generate-gitid.sh)
NE_SHORTGIT_ID:= $(shell ./ne-generate-gitid.sh -s)
NE_HOST := $(shell /bin/hostname -f)
NE_ARCH := $(shell /bin/uname -m)
NE_OPERSYS := $(shell /bin/uname -o | /bin/sed 1s/[^a-zA-Z0-9_]/_/g )
NE_ATSHARP := $(shell printf '@#')
NE_HOMETMP := $(shell echo '$$HOME/tmp')
HUMAN_SOURCES= $(wildcard ne_*.c)
HUMAN_HEADERS= $(wildcard ne_*.h)
GENERATED_SOURCES= $(wildcard _*.c)

HUMAN_OBJECTS=  $(patsubst %.c,obj/%.o, $(HUMAN_SOURCES))
GENERATED_OBJECTS= $(patsubst %.c,obj/%.o, $(GENERATED_SOURCES))


.PHONY: all clean

all: nanoeng

_gitid.c: ne-genrate-gitid.sh | GNUmakefile
	./ne-generate-gitid.sh -c

nanoeng: $(HUMAN_OBJECTS) $(GENERATED_OBJECTS)
	$(LINK.c) $< -o $@ $(LIBS)

## end of file truc-gpl/NanoEngine/GNUmakefile
