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
const char* bsgtk_prog_name=nullptr;
bool bsgtk_debug;

static void
bsgtk_usage(void)
{
  std::cout << bsgtk_prog_name << " usage (git " << bsgtk_main_git
            << ") :" << std::endl;
  std::cout << "\t --help                         # this help"
            << std::endl;
  std::cout << "\t --version                      # version info"
            << std::endl;
  std::cout << "\t --debug | -D                   # debug messages"
            << std::endl;
} // end bsgtk_usage

static void
bsgtk_prog_args(int& argc, char**argv)
{
  bsgtk_prog_name = argv[0];
  BSGTK_UNIQUE_BREAKPOINT();
  for (int aix=1; aix<argc; aix++)
    {
      char*curarg = argv[aix];
      if (!strcmp(curarg, "--help"))
        {
          bsgtk_usage();
          std::cout << " see github.com/bstarynk/truc-gpl/Gtkmm-test/"
                    << std::endl;
          exit(EXIT_SUCCESS);
        }
      else if (!strcmp(curarg, "--version"))
        {
          std::cout << bsgtk_prog_name << " git " << bsgtk_main_git
                    << std::endl;
          std::cout << " see github.com/bstarynk/truc-gpl/Gtkmm-test/"
                    << std::endl;
          exit(EXIT_SUCCESS);
        }
      else if (!strcmp(curarg, "-D") || !strcmp(curarg, "--debug"))
        {
          bsgtk_debug = true;
	  std::cout << bsgtk_prog_name << " git " << bsgtk_main_git
		    << " enable debug (" << __FILE__ << ":" << __LINE__-2
		    << ")"
		    << " removing prog.arg#" << aix << std::endl;
	  argc--;
	  for (int ix=aix+1; ix<argc; ix++)
	    argv[ix] = argv[ix+1];
	  argv[argc] = nullptr;
        }
    }
} // end bsgtk_prog_args

int
main(int argc, char* argv[])
{
  BSGTK_UNIQUE_BREAKPOINT();
  bsgtk_prog_args(argc, argv);
  BSGTK_UNIQUE_BREAKPOINT();
  BSGTK_DEBUG_LOG(argv[0] << " main with " << argc << " arguments");
  auto app = BsGtk_Application::create("truc-gpl.GtkMM-test");
  BSGTK_DEBUG_LOG("main with app=" << app << " git=" << bsgtk_main_git);
  return app->make_window_and_run<BsGtk_Window>(argc, argv);
}

/****************
 **                           for Emacs...
 ** Local Variables: ;;
 ** compile-command: "make -C $TRUCGPL_DIR/GtkMM-test" ;;
 ** End: ;;
 ****************/

//// end of file GTKmm-test/main.cc
