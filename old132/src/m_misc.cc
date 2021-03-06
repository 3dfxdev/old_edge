//----------------------------------------------------------------------------
//  EDGE Misc: Screenshots, Menu and defaults Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2009  The EDGE Team.
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
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//
// -MH- 1998/07/02  Added key_flyup and key_flydown
// -MH- 1998/07/02 "shootupdown" --> "true3dgameplay"
// -ACB- 2000/06/02 Removed Control Defaults
//

#include "i_defs.h"

#include "epi/utility.h"
#include "epi/endianess.h"
#include "epi/file.h"
#include "epi/filesystem.h"
#include "epi/path.h"
#include "epi/str_format.h"

#include "epi/image_data.h"
#include "epi/image_jpeg.h"
#include "epi/image_png.h"

#include "ddf/language.h"

#include "g_state.h"
#include "con_main.h"
#include "e_input.h"
#include "e_player.h"
#include "hu_stuff.h"  // only for showMessages
#include "m_argv.h"
#include "m_misc.h"
#include "m_option.h"
#include "n_network.h"
#include "p_spec.h"
#include "r_gldefs.h"
#include "s_music.h"  // mus_volume
#include "s_sound.h"
#include "am_map.h"
#include "r_colormap.h"
#include "hu_draw.h"
#include "r_modes.h"
#include "r_image.h"
#include "hu_wipe.h"
#include "version.h"


cvar_c m_diskicon;
cvar_c r_fadepower;

bool display_disk = false;
int  display_desync = 0;

static const image_c *disk_image = NULL;
static const image_c *air_images[21] = { NULL };

unsigned short save_screenshot[160][100];
bool save_screenshot_valid = false;

extern cvar_c r_hq2x;


//FIXME !!!    {CFGT_Int,      "save_page",         &save_page, 0},


void M_SaveDefaults(void)
{
	// -ACB- 1999/09/24 idiot proof checking as required by MSVC
	SYS_ASSERT(! cfgfile.empty());

	FILE *f = fopen(cfgfile.c_str(), "w");
	if (!f)
	{
		I_Warning("Couldn't open config file %s for writing.", cfgfile.c_str());
		return;
	}

	fprintf(f, "edge_version\t\t\"%d\"\n", EDGEVER);

	for (int k = 0; all_cvars[k].name; k++)
	{
		cvar_c *var = all_cvars[k].var;

		if (strchr(all_cvars[k].flags, 'c'))
			fprintf(f, "%s\t\"%s\"\n", all_cvars[k].name, var->str);
	}

	for (int n = 0; all_binds[n].name; n++)
	{
		std::string kconf = E_FormatConfig(&all_binds[n]);

		if (! kconf.empty())
			fprintf(f, "%s", kconf.c_str());
	}

	fclose(f);
}


void M_LoadDefaults(void)
{
	I_Printf("Loading defaults from: %s\n", cfgfile.c_str());

	// read the file in, overriding any set defaults
	FILE *f = fopen(cfgfile.c_str(), "r");
	if (! f)
	{
		I_Warning("Couldn't open config file %s for reading.\n", cfgfile.c_str());
		return;
	}

	int edge_version = 0;

	char buffer[200];

	while (fgets(buffer, sizeof(buffer)-1, f))
	{
		if (strncmp(buffer, "edge_vers", 9) == 0)
		{
			char *p = buffer+9;
			while (*p && !isdigit(*p)) p++;
			
			if (*p)
			{
				edge_version = atoi(p);
				I_Printf("Config file from version: %d.%02d\n",
						 edge_version/100, edge_version%100);
			}
			continue;
		}

		// skip any slash at the beginning
		char *pos = buffer;
		if (pos[0] == '/') pos++;
		
		CON_TryCommand(pos);
	}

	fclose(f);

	// do any backwards compatibility stuff here
	// (if edge_version < XXX then ...)
}


void M_DisplayDisk(void)
{
	/* displays disk icon during loading... */

	if (!display_disk || !m_diskicon.d)
		return;
   
	if (!disk_image)
		disk_image = R_ImageLookup("STDISK");

	// reset flag
	display_disk = false;

	float w = IM_WIDTH(disk_image);
	float h = IM_HEIGHT(disk_image);

	HUD_DrawImage(314 - w, 164 - h, disk_image);
}


void M_DisplayAir(void)
{
	/* displays air indicator when underwater */

	int i;
  
	if (numplayers == 0)
		return;

	player_t *p = players[displayplayer];

	if (p->playerstate != PST_LIVE || ! p->underwater)
		return;

	SYS_ASSERT(p->mo);

	// load in patches for air indicator
	if (!air_images[0])
	{
		for (i=1; i <= 21; i++)
		{
			char buffer[16];
			sprintf(buffer, "AIRBAR%02d", i);
			air_images[i - 1] = R_ImageLookup(buffer);
		}
	}

	i = 21;

	if (p->air_in_lungs > 0)
	{
		int nom   = p->air_in_lungs;
		int denom = p->mo->info->lung_capacity;
    
		i = 1 + (20 * (denom - nom) / denom);

		SYS_ASSERT(1 <= i && i <= 20);
	}
  
	HUD_DrawImage(0, 0, air_images[i - 1]);
}


#define PIXEL_RED(pix)  (playpal_data[0][pix][0])
#define PIXEL_GRN(pix)  (playpal_data[0][pix][1])
#define PIXEL_BLU(pix)  (playpal_data[0][pix][2])


void M_ScreenShot(bool show_msg)
{
	const char *extension = "png";

	std::string fn;

	// find a file name to save it to
	for (int i = 1; i <= 9999; i++)
	{
		std::string base(epi::STR_Format("shot%02d.%s", i, extension));

		fn = epi::PATH_Join(shot_dir.c_str(), base.c_str());
  
		if (! epi::FS_Access(fn.c_str(), epi::file_c::ACCESS_READ))
        {
			break; // file doesn't exist
        }
	}

	FILE *fp = fopen(fn.c_str(), "wb");
	if (fp == NULL)
	{
		if (show_msg)
			I_Printf("Unable to create file: %s\n", fn.c_str());

		return;
	}

	epi::image_data_c *img = new epi::image_data_c(SCREENWIDTH, SCREENHEIGHT, 3);

	RGL_ReadScreen(0, 0, SCREENWIDTH, SCREENHEIGHT, img->PixelAt(0,0));

	bool result;
	result = epi::PNG_Save(fp, img);

	if (show_msg)
	{
		if (result)
			I_Printf("Captured to file: %s\n", fn.c_str());
		else
			I_Printf("Error saving file: %s\n", fn.c_str());
	}

	delete img;

	fclose(fp);
}


void M_MakeSaveScreenShot(void)
{
#if 0 /// FIXME:
	// byte* buffer = new byte[SCREENWIDTH*SCREENHEIGHT*4];
	// glReadBuffer(GL_FRONT);
	// glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	// glReadPixels(0, 0, SCREENWIDTH, SCREENHEIGHT, GL_RGB, GL_UNSIGNED_BYTE, buffer);
	// ...
	// delete [] buffer;
#endif

#if 0  // OLD SOFTWARE VERSION
	int x, y;
	int ax, ay;

	byte pixel;
	unsigned short rgb;
	truecol_info_t colinfo;

	save_screenshot_valid = true;

	I_GetTruecolInfo(&colinfo);

	switch (main_scr->bytepp)
	{
		case 1:
		{
			byte *top = (byte *) main_scr->data;

			top += viewwindowy * main_scr->pitch + viewwindowx;

			for (y=0; y < 100; y++)
				for (x=0; x < 160; x++)    
				{
					ax = x * viewwindowwidth  / 160;
					ay = y * viewwindowheight / 100;

					SYS_ASSERT(0 <= ax && ax < SCREENWIDTH);
					SYS_ASSERT(0 <= ay && ay < SCREENHEIGHT);

					pixel = top[ay * main_scr->pitch + ax];

					rgb = ((PIXEL_RED(pixel) & 0xF8) << 7) |
						((PIXEL_GRN(pixel) & 0xF8) << 2) |
						((PIXEL_BLU(pixel) & 0xF8) >> 3);
        
					save_screenshot[x][y] = rgb;
				}
		}
		break;

		case 2:
		{
			unsigned short *top = (unsigned short *) main_scr->data;

			top += viewwindowy * main_scr->pitch/2 + viewwindowx;

			for (y=0; y < 100; y++)
				for (x=0; x < 160; x++)    
				{
					ax = x * viewwindowwidth  / 160;
					ay = y * viewwindowheight / 100;

					SYS_ASSERT(0 <= ax && ax < SCREENWIDTH);
					SYS_ASSERT(0 <= ay && ay < SCREENHEIGHT);

					rgb = top[ay * main_scr->pitch/2 + ax];

					// hack !
					if (colinfo.green_bits == 6)
						rgb = ((rgb & 0xFFC0) >> 1) | (rgb & 0x001F);
        
					save_screenshot[x][y] = rgb;
				}
		}
		break;
	}
#endif
}

//
// Creates the file name "dir/file", or
// just "file" in the given string if it 
// was an absolute address.
//
std::string M_ComposeFileName(const char *dir, const char *file)
{
	if (epi::PATH_IsAbsolute(file))
		return std::string(file);

	return epi::PATH_Join(dir, file);
}


epi::file_c *M_OpenComposedEPIFile(const char *dir, const char *file)
{
	std::string fullname = M_ComposeFileName(dir, file);

	return epi::FS_Open(fullname.c_str(),
		epi::file_c::ACCESS_READ | epi::file_c::ACCESS_BINARY);
}

//
// Loads file into memory. This sets a pointer to the data and
// the length.
//
// NOTE: The data must be freed by delete[] when not used.
//
// Returns NULL on failure.
//
// -ACB- 2000/01/08 Written
// -ES-  2000/06/12 Now returns the allocated pointer, or NULL on failure.
//
byte* M_GetFileData(const char *filename, int *length)
{
	FILE *lumpfile;
	byte *data;

	// Sanity Checks..
	SYS_ASSERT(filename);
	SYS_ASSERT(length);

	lumpfile = fopen(filename, "rb");  
	if (!lumpfile)
	{
		I_Warning("M_GetFileData: Cannot open '%s'\n", filename);
		return NULL;
	}

	fseek(lumpfile, 0, SEEK_END);                   // get the end of the file
	(*length) = ftell(lumpfile);                    // get the size
	fseek(lumpfile, 0, SEEK_SET);                   // reset to beginning

	data = new byte[*length];						// malloc the size
	fread(data, sizeof(char), (*length), lumpfile); // read file
	fclose(lumpfile);                               // close the file

	return data;
}


void M_WarnError(const char *error,...)
{
	// Either displays a warning or produces a fatal error, depending
	// on whether the "-strict" option is used.

	char message_buf[4096];

	message_buf[4095] = 0;

	va_list argptr;

	va_start(argptr, error);
	vsprintf(message_buf, error, argptr);
	va_end(argptr);

	// I hope nobody is printing strings longer than 4096 chars...
	SYS_ASSERT(message_buf[4095] == 0);

	if (strict_errors)
		I_Error("%s", message_buf);
	else if (! no_warnings)
		I_Warning("%s", message_buf);
}


extern FILE *debugfile; // FIXME

void I_Debugf(const char *message,...)
{
	// Write into the debug file.
	//
	// -ACB- 1999/09/22: From #define to Procedure
	// -AJA- 2001/02/07: Moved here from platform codes.
	//
	if (!debugfile)
		return;

	char message_buf[4096];

	message_buf[4095] = 0;

	// Print the message into a text string
	va_list argptr;

	va_start(argptr, message);
	vsprintf(message_buf, message, argptr);
	va_end(argptr);

	// I hope nobody is printing strings longer than 4096 chars...
	SYS_ASSERT(message_buf[4095] == 0);

	fprintf(debugfile, "%s", message_buf);
	fflush(debugfile);
}


extern FILE *logfile; // FIXME: make file_c and unify with debugfile

void I_Logf(const char *message,...)
{
	if (!logfile)
		return;

	char message_buf[4096];

	message_buf[4095] = 0;

	// Print the message into a text string
	va_list argptr;

	va_start(argptr, message);
	vsprintf(message_buf, message, argptr);
	va_end(argptr);

	// I hope nobody is printing strings longer than 4096 chars...
	SYS_ASSERT(message_buf[4095] == 0);

	fprintf(logfile, "%s", message_buf);
	fflush(logfile);
}


//
// Just like L_CompareTimeStamps above, but give the filenames.
//
int L_CompareFileTimes(const char *A, const char *B)
{
	epi::timestamp_c A_time;
	epi::timestamp_c B_time;

	if (! FS_GetModifiedTime(A, A_time))
		I_Error("AddFile: I_GetModifiedTime failed on %s\n", A);

	if (! FS_GetModifiedTime(B, B_time))
		I_Error("AddFile: I_GetModifiedTime failed on %s\n", B);

	return A_time.Cmp(B_time);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
