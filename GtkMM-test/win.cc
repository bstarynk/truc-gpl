// file GTKmm-test/win.cc
// SPDX-License-Identifier: GPL-3.0-or-later
// file github.com/bstarynk/truc-gpl/Gtkmm-test/win.cc
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


#include "gtkmm-basile.hh"

extern "C" const char bsgtk_win_git[];
const char bsgtk_win_git[] = SHORT_GITID;

extern "C" const char bsgtk_win_basename[];
const char bsgtk_win_basename[] = BSGTK_BASENAME;

int bsgtk_def_width=567;
int bsgtk_def_height=333;

BsGtk_Window::BsGtk_Window()
  : Gtk::Window() {
  BSGTK_DEBUG_LOG("BsGtk_Window constructor @" << (void*)this);
  set_title("Basic BsGtk gtkmm-basile application");
  set_default_size(bsgtk_def_width, bsgtk_def_height);
#warning incomplete BsGtk_Window constructor
} // end BsGtk_Window::BsGtk_Window

BsGtk_Window::~BsGtk_Window()
{
#warning incomplete BsGtk_Window destructor
  BSGTK_DEBUG_LOG("BsGtk_Window destructor @" << (void*)this);
} // end Bsgtk_Window::~BsGtk_Window

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make -C $TRUCGPL_DIR/GtkMM-test" ;;
 ** End: ;;
 ****************/
