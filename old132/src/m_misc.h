//----------------------------------------------------------------------------
//  EDGE Misc: Screenshots, Menu and defaults Code
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2008  The EDGE Team.
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
// 1998/07/02 -MH- Added key_flyup and key_flydown
//

#ifndef __M_MISC__
#define __M_MISC__

#include "epi/file.h"
#include "epi/utility.h"


void M_LoadDefaults(void);
void M_SaveDefaults(void);

void M_DisplayDisk(void);
void M_DisplayDesynch(void);
void M_DisplayAir(void);
void M_ScreenShot(bool show_msg);
void M_MakeSaveScreenShot(void);
byte *M_GetFileData(const char *filename, int *length);
std::string M_ComposeFileName(const char *dir, const char *file);
epi::file_c *M_OpenComposedEPIFile(const char *dir, const char *file);
void M_WarnError(const char *error,...) GCCATTR((format(printf, 1, 2)));
int L_CompareFileTimes(const char *wad_file, const char *gwa_file);

extern unsigned short save_screenshot[160][100];
extern bool save_screenshot_valid;

extern bool display_disk;

extern cvar_c r_fadepower;
extern cvar_c m_obituaries;

#endif  /* __M_MISC__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
