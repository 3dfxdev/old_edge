//------------------------------------------------------------------------
//  Main Window
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2005  Andrew Apted
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#include "includes.h"

#include "ui_log.h"
#include "ui_menu.h"
#include "ui_window.h"

#ifndef WIN32
#include <unistd.h>
#endif


UI_MainWin *main_win;


static void main_win_close_CB(Fl_Widget *w, void *data)
{
	if (main_win)
		main_win->want_quit = true;
}


//
// WindowSmallDelay
//
// This routine is meant to delay a short time (e.g. 1/5th of a
// second) to allow the window manager to move our windows around.
//
// Hopefully such nonsense doesn't happen under Win32.
//
void WindowSmallDelay(void)
{
#ifndef WIN32
	Fl::wait(0);  usleep(100 * 1000);
	Fl::wait(0);  usleep(100 * 1000);
#endif

	Fl::wait(0);
}

int guix_prefs_win_w = 550;  // FIXME
int guix_prefs_win_h = 550;


//
// MainWin Constructor
//
UI_MainWin::UI_MainWin(const char *title) :
    Fl_Window(guix_prefs_win_w, guix_prefs_win_h, title)
{
	// turn off auto-add-widget mode
	end();

	size_range(MAIN_WINDOW_MIN_W, MAIN_WINDOW_MIN_H);

	// Set initial position.
	//
	// Note: this may not work properly.  It seems that when my window
	// manager adds the titlebar/border, it moves the actual window
	// down and slightly to the right, causing subsequent invokations
	// to keep going lower and lower.

	///  position(guix_prefs.win_x, guix_prefs.win_y);

	callback((Fl_Callback *) main_win_close_CB);

	// set a nice darkish gray for the space between main boxes
	color(MAIN_BG_COLOR, MAIN_BG_COLOR);

	want_quit = false;


	int cy = 0;

	/* ---- Menu bar ---- */
	{
		menu_bar = MenuCreate(0, 0, w(), 28);
		add(menu_bar);

#ifndef MACOSX
		cy += menu_bar->h();
///		cy += 8;
#endif
	}

#if 0	/* ---- Tabs ---- */
	{
		tabs = new Fl_Tabs(0, cy, w(), h() - cy);
		tabs->end();
		tabs->selection_color(FL_YELLOW);
		// tabs->box(FL_NO_BOX);

		cy += 26;
	}
#endif

	/* ---- Setup panel ---- */
	{
///		Fl_Group *setup_group = new Fl_Group(0, cy, w(), 200, "Server Setup");
///		setup_group->end();

		setup_box = new UI_Setup(0, cy, w(), 156, "\n Server Setup");
		add(setup_box);

		cy += setup_box->h();

///		add(setup_group);
///		tabs->value(setup_group);
	}

#if 0	/* ---- Client list ---- */
	{
		Fl_Group *client_group = new Fl_Group(0, cy, w(), h() - cy, "Clients");
		client_group->end();

		client_list = new UI_ClientList(0, cy, w(), h() - cy);
		client_group->add(client_list);

		tabs->add(client_group);

	}
#endif

#if 0	/* ---- Game list ---- */
	{
		Fl_Group *game_group = new Fl_Group(0, cy, w(), h() - cy, "Games");
		game_group->end();

		game_list = new UI_GameList(0, cy, w(), h() - cy);
		game_group->add(game_list);

		tabs->add(game_group);
	}
#endif

	/* ---- Log and Statistics tab ---- */
	{
///		Fl_Group *stat_group = new Fl_Group(0, cy, w(), h() - cy, "Log");
///		stat_group->end();

		stat_box = new UI_Stats(0, cy, w(), 136, "\n Statistics and Log");
		add(stat_box);

		cy += stat_box->h();

		log_box = new UI_LogBox(0, cy, w(), h() - cy);
		add(log_box);

///		stat_group->resizable(log_box);
///		add(stat_group);
	}

///	add(tabs);
///	resizable(client_list);
	resizable(log_box);

	// show window (pass some dummy arguments)
	int argc = 1;
	char *argv[] = { "edge-mpserv", NULL };

	show(argc, argv);

	// read initial pos, giving 1/5th of a second for the WM to adjust
	// our window's position (naughty WM...)
	WindowSmallDelay();

	init_x = x(); init_y = y();
	init_w = w(); init_h = h();
}

//
// MainWin Destructor
//
UI_MainWin::~UI_MainWin()
{
	WritePrefs();
}


void UI_MainWin::WritePrefs()
{
	// check if moved or resized
#if 0
	if (x() != init_x || y() != init_y)
	{
		guix_prefs.win_x = x();
		guix_prefs.win_y = y();
	}

	if (w() != init_w || h() != init_h)
	{
		guix_prefs.win_w = w();
		guix_prefs.win_h = h();
	}
#endif
}

