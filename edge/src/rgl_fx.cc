//----------------------------------------------------------------------------
//  EDGE OpenGL Rendering (Screen Effects)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2004  The EDGE Team.
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
//----------------------------------------------------------------------------

// this conditional applies to the whole file
#include "i_defs.h"

#include "am_map.h"
#include "dm_defs.h"
#include "dm_state.h"
#include "e_search.h"
#include "m_bbox.h"
#include "m_random.h"
#include "p_local.h"
#include "p_mobj.h"
#include "r_defs.h"
#include "r_main.h"
#include "r_sky.h"
#include "r_state.h"
#include "r_things.h"
#include "r2_defs.h"
#include "rgl_defs.h"
#include "v_colour.h"
#include "v_ctx.h"
#include "v_res.h"
#include "w_textur.h"
#include "w_wad.h"
#include "z_zone.h"

#define DEBUG  0


bool ren_allbright;
float ren_red_mul;
float ren_grn_mul;
float ren_blu_mul;

//
// RGL_RainbowEffect
//
// Effects that modify all colours, e.g. nightvision green.
//
void RGL_RainbowEffect(player_t *player)
{
	float s;
  
	ren_allbright = false;
	ren_red_mul = ren_grn_mul = ren_blu_mul = 1.0f;

	s = player->powers[PW_Invulnerable];  

	if (s > 0)
		return;

	s = player->powers[PW_NightVision];

	if (s > 0)
	{
		s = MIN(128.0f, s);
		ren_red_mul = ren_blu_mul = 1.0f - s / 128.0f;
		ren_allbright = true;
		return;
	}
}


//
// RGL_ColourmapEffect
//
// For example: all white for invulnerability.
//
void RGL_ColourmapEffect(player_t *player)
{
	int x1, y1;
	int x2, y2;

	if (player->powers[PW_Invulnerable] > 0)
	{
		glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);

		glEnable(GL_BLEND);

		glBegin(GL_QUADS);
	  
		x1 = viewwindowx;
		x2 = viewwindowx + viewwindowwidth;

		y1 = SCREENHEIGHT - viewwindowy;
		y2 = SCREENHEIGHT - viewwindowy - viewwindowheight;

		glVertex2i(x1, y1);
		glVertex2i(x2, y1);
		glVertex2i(x2, y2);
		glVertex2i(x1, y2);

		glEnd();
	  
		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
}

//
// RGL_PaletteEffect
//
// For example: red wash for pain.
//
void RGL_PaletteEffect(player_t *player)
{
	byte rgb_data[3];

	float s = player->powers[PW_Invulnerable];

	if (s > 0)
	{
		s = MIN(128.0f, s);

		if (s < 40.0)
			glColor4f(1.0f, 1.0f, 1.0f, (40.0f - s) / 80.0f);
		else
			glColor4f(0.0f, 0.0f, 0.0f, (s - 40.0f) / 320.0f);
	}
	else
	{
		V_IndexColourToRGB(pal_black, rgb_data);

		int rgb_max = MAX(rgb_data[0], MAX(rgb_data[1], rgb_data[2]));

		if (rgb_max == 0)
			return;
	  
		rgb_max = MIN(200, rgb_max);

		glColor4f((float) rgb_data[0] / (float) rgb_max,
				  (float) rgb_data[1] / (float) rgb_max,
				  (float) rgb_data[2] / (float) rgb_max,
			      (float) rgb_max / 255.0f);
	}

	glEnable(GL_BLEND);

	glBegin(GL_QUADS);
  
	glVertex2i(0, SCREENHEIGHT);
	glVertex2i(SCREENWIDTH, SCREENHEIGHT);
	glVertex2i(SCREENWIDTH, 0);
	glVertex2i(0, 0);

	glEnd();
  
	glDisable(GL_BLEND);
}


