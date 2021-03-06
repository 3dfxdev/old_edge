//------------------------------------------------------------------------
//  Information Panel
//------------------------------------------------------------------------
//
//  RTS Layout Tool (C) 2007 Andrew Apted
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

#include "headers.h"
#include "hdr_fltk.h"

#include "lib_util.h"
#include "main.h"

#include "ui_window.h"
#include "ui_grid.h"
#include "ui_panel.h"
#include "ui_radius.h"
#include "ui_thing.h"


#define INDEX_NONE_STR  "Index: None Selected"


//
// UI_Panel Constructor
//
UI_Panel::UI_Panel(int X, int Y, int W, int H, const char *label) : 
    Fl_Group(X, Y, W, H, label)
{
  end();  // cancel begin() in Fl_Group constructor

  box(FL_FLAT_BOX);


  X += 6;
  Y += 6;

  W -= 12;
  H -= 12;

  // ---- top section ----
 
  map_name = new Fl_Output(X+96, Y, 80, 22, "Current Map: ");
  map_name->align(FL_ALIGN_LEFT);
  map_name->value("MAP01");

  add(map_name);

  Y += map_name->h() + 8;


  grid_size = new Fl_Output(X+50, Y, 72, 22, "Scale:");
  grid_size->align(FL_ALIGN_LEFT);
  add(grid_size);

  Y += grid_size->h() + 4;


  m_label = new Fl_Box(FL_NO_BOX, X+4, Y, W, 22, "Mouse Coords:");
  m_label->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
  add(m_label);
  
  Y += m_label->h() + 4;


  mouse_x = new Fl_Output(X+28,   Y, 72, 22, "x");
  mouse_y = new Fl_Output(X+W-72, Y, 72, 22, "y");

  mouse_x->align(FL_ALIGN_LEFT);
  mouse_y->align(FL_ALIGN_LEFT);

  add(mouse_x);
  add(mouse_y);

  Y += mouse_x->h() + 20;


  // ---- bottom section ----

  mode = new Fl_Choice(X+50, Y, W-64, 22, "Mode:");
  mode->align(FL_ALIGN_LEFT);
  mode->add("Scripts|Things");
  mode->value(0);
  mode->color(FL_RED);
  mode->callback(mode_callback, this);

  add(mode);

  Y += mode->h() + 8;


  which = new Fl_Box(FL_NO_BOX, X+4, Y, W-4, 22, INDEX_NONE_STR);
  which->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

  add(which);

  Y += which->h() + 4;


  script_box = new UI_RadiusInfo(X-2, Y, W+4, H-Y-12);

  add(script_box);

  script_box->deactivate();
  

  thing_box = new UI_ThingInfo(X-2, Y, W+4, H-Y-12);

  add(thing_box);
    
  thing_box->deactivate();
  thing_box->hide();


  // ---- resizable ----
 
#if 1
  Fl_Box *resize_control = new Fl_Box(FL_NO_BOX, x(), H-4, w(), 4, NULL);

  add(resize_control);
  resizable(resize_control);
#endif 

}

//
// UI_Panel Destructor
//
UI_Panel::~UI_Panel()
{
}

int UI_Panel::handle(int event)
{
  return Fl_Group::handle(event);
}

void UI_Panel::mode_callback(Fl_Widget *w, void *data)
{
  UI_Panel *panel = (UI_Panel *)data;

  SYS_ASSERT(panel);

  panel->mode->color(panel->mode->value() ? FL_CYAN : FL_RED);

  switch (panel->mode->value())
  {
    case 0:
      panel->script_box->show();
      panel->thing_box->hide();

      main_win->grid->SetEditMode(UI_Grid::EDIT_RadTrig);
      break;

    case 1:
      panel->thing_box->show();
      panel->script_box->hide();

      main_win->grid->SetEditMode(UI_Grid::EDIT_Things);
      break;

    default:
      Main_FatalError("INTERNAL ERROR: mode_callback: bad value!\n");
      break; /* NOT REACHED */
  }
}

//------------------------------------------------------------------------

void UI_Panel::SetMap(const char *name)
{
  char *upper = StrUpper(name);

  map_name->value(upper);

  StringFree(upper);
}

void UI_Panel::SetZoom(float zoom_mul)
{
  char buffer[60];

///  if (0.99 < zoom_mul && zoom_mul < 1.01)
///  {
///    grid_size->value("1:1");
///    return;
///  }

  if (zoom_mul < 0.99)
  {
    sprintf(buffer, "/ %1.3f", 1.0/zoom_mul);
  }
  else // zoom_mul > 1
  {
    sprintf(buffer, "x %1.3f", zoom_mul);
  }

  grid_size->value(buffer);
}


void UI_Panel::SetNodeIndex(int index)
{
#if 0
  char buffer[60];

  sprintf(buffer, "%d", index);
  
  ns_index->label("Node #");
  ns_index->value(buffer);

  redraw();
#endif
}


void UI_Panel::SetMouse(double mx, double my)
{
  if (mx < -32767.0 || mx > 32767.0 ||
      my < -32767.0 || my > 32767.0)
  {
    mouse_x->value("off map");
    mouse_y->value("off map");

    return;
  }

  char x_buffer[60];
  char y_buffer[60];

  sprintf(x_buffer, "%1.1f", mx);
  sprintf(y_buffer, "%1.1f", my);

  mouse_x->value(x_buffer);
  mouse_y->value(y_buffer);
}

void UI_Panel::SetWhich(int index, int total)
{
  if (index < 0)
  {
    which->label(INDEX_NONE_STR);
  }
  else
  {
    char buffer[200];
    sprintf(buffer, "Index: #%d of %d", index, total);
 
    which->copy_label(buffer);
  }

  redraw();
}


//--- editor settings ---
// vi:ts=2:sw=2:expandtab
