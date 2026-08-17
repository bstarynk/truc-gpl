// file GTKmm-test/app.cc
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


#include "gtkmm-basile.hh"

extern "C" const char bsgtk_app_git[];
const char bsgtk_app_git[] = SHORT_GITID;

int
BsGtk_Application::run(int argc, char**argv)
{
  std::clog << __func__ << " argc=" << argc
            << " from "  << __FILE__ << ":" << __LINE__ << std::endl;
  int r = Gtk::Application::run(argc, argv);
  return r;
} // end BsGtk_Application::run


/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make -C $TRUCGPL_DIR/GtkMM-test" ;;
 ** End: ;;
 ****************/
