//------------------------------------------------------------------------
// SYSTEM : Bridging code
//------------------------------------------------------------------------
//
//  GL-Friendly Node Builder (C) 2000-2004 Andrew Apted
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

#ifndef __SYSTEM_H__
#define __SYSTEM_H__


void System_Startup(void);
void System_Shutdown(void);

bool FileExists(const char *filename);

// text output functions
void PrintMsg(const char *str, ...);
void FatalError(const char *str, ...);
void InternalError(const char *str, ...);

void ShowProgress(int count, int limit);
void ClearProgress(void);

// endian handling
unsigned short Endian_U16(unsigned short);
unsigned int   Endian_U32(unsigned int);

// these are only used for debugging
void Debug_PrintMsg(const char *str, ...);


#endif /* __SYSTEM_H__ */
