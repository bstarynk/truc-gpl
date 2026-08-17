// file GTKmm-test/gtkmm-basile.hh
// SPDX-License-Identifier: GPL-3.0-or-later
// file github.com/bstarynk/truc-gpl/Gtkmm-test/GNUmakefile
// Author:
//      Basile STARYNKEVITCH, 92340 Bourg-la-Reine, France,
//                    <basile@starynkevitch.net>
//                 or <b.starynkevitch@gmail.com>

// License:
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>

#ifndef GTKMM_BASILE_INCLUDED
#define GTKMM_BASILE_INCLUDED


#ifndef SHORT_GITID
#error undefined SHORT_GITID in compilation command
#endif


#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <gtkmm.h>

extern "C" bool bsgtk_debug;
extern "C" char* bsgtk_prog_name;
extern "C" void* bsgtk_dlh;

/// see https://gnome.pages.gitlab.gnome.org/gtkmm-documentation/chapter-basics.html#sec-basics-simple-example
class BsGtk_Window : public Gtk::Window
{
public:
  BsGtk_Window();
}; // end class BsGtk_Window

class BsGtk_Application : public Gtk::Application
{
  virtual int run (int argc, char**argv);
};      // end class BsGtk_Application



/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make -C $TRUCGPL_DIR/GtkMM-test" ;;
 ** End: ;;
 ****************/

#endif /*GTKMM_BASILE_INCLUDED*/
