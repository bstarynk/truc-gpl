#!/usr/bin/gmake
# SPDX-License-Identifier: GPL-3.0-or-later
# file github.com/bstarynk/truc-gpl/Gtkmm-test/GNUmakefile
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

.PHONY: all clean
export


CXX?= g++

PACKAGES= gtkmm-4.0
SHORTGITID=$(shell ../truc-generate-gitid.sh -s)
CFLAGS= -O -g -std=gnu++17 -Wall -Wextra $(shell pkg-config --cflags $(PACKAGES))
CFLAGS+= -DSHORT_GITID=\"$(SHORTGITID)\"
LIBS= $(shell pkg-config --libs $(PACKAGES))

CXXFILES= $(wildcard [a-z]*.cc)
OBJECTS= $(patsubst %.cc, %.o, $(CXXFILES))

all: gtkmm-basile | GNUmakefile

gtkmm-basile: $(OBJECTS) 
	$(CXX) $(OBJECTS) $(LIBS) -o $@

clean:
	$(RM) *.o *.so *~

main.o: main.cc gtkmm-basile.hh
	$(CXX) $(CFLAGS) -c $< -o $@
