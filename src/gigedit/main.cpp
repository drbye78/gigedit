/*
 * Copyright (C) 2006 - 2019 Andreas Persson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with program; see the file COPYING. If not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#include "global.h"
#include <gtk/gtk.h>
#include "gigedit.h"
#ifdef GTKMM_HEADER_FILE
# include GTKMM_HEADER_FILE(gtkmm.h)
#else
# include <gtkmm.h>
#endif

#if defined(WIN32)
#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    GigEdit app;
    return app.run(__argc, __argv);
}

#else

int main(int argc, char* argv[])
{
#ifdef __APPLE__
    // remove the argument added by the OS
    if (argc > 1 && strncmp(argv[1], "-psn", 4) == 0) {
        argc--;
        for (int i = 1 ; i < argc ; i++) {
            argv[i] = argv[i + 1];
        }
    }
#endif
    GigEdit app;
    return app.run(argc, argv);
}

#endif
