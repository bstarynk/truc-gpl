// file GTKmm-test/main.cc
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

extern "C" const char bsgtk_main_git[];
const char bsgtk_main_git[] = SHORT_GITID;

BsGtk_Window::BsGtk_Window()
{
  set_title("Basic BsGtk gtkmm-basile application");
  set_default_size(600, 200);
}

int
main(int argc, char* argv[])
{
  auto app = BsGtk_Application::create("truc-gpl.GtkMM-test");

  return app->make_window_and_run<BsGtk_Window>(argc, argv);
}

