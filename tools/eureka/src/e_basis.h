//------------------------------------------------------------------------
//  BASIC OBJECT HANDLING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor (C) 2001-2009 Andrew Apted
//                     (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#ifndef __E_BASIS_H__
#define __E_BASIS_H__


//
// DESIGN NOTES
//
// Every field in these structures are a plain 'int'.  This is a
// design decision aiming to simplify the logic and code for undo
// and redo.
//
// Strings are represented as offsets into a string table, where
// fetching the actual (read-only) string is fast, but adding new
// strings is slow (with the current code).
//
// These structures are always ensured to have valid fields, e.g.
// the LineDef vertex numbers are OK, the SideDef sector number is
// valid, etc.  For LineDefs, either/both of side_L and side_R can
// contain -1 to mean "no sidedef", but note that a missing right
// sidedef can cause problems when playing the map in DOOM.
//


class Thing
{
public:
	int x;
	int y;
	int angle;
	int type;
	int options;

	enum { F_X, F_Y, F_ANGLE, F_TYPE, F_OPTIONS };

public:
};


class Vertex
{
public:
	int x;
	int y;

	enum { F_X, F_Y };

public:
};


class Sector
{
public:
	int floorh;
	int ceilh;
	int floor_tex;
	int ceil_tex;
	int light;
	int type;
	int tag;

	enum { F_FLOORH, F_CEILH, F_FLOOR_TEX, F_CEIL_TEX, F_LIGHT, F_TYPE, F_TAG };

public:
	const char *FloorTex() const;
	const char *CeilTex() const;

	int HeadRoom() const
	{
		return ceilh - floorh;
	}
};


class SideDef
{
public:
	int x_offset;
	int y_offset;
	int upper_tex;
	int mid_tex;
	int lower_tex;
	int sector;

	enum { F_X_OFFSET, F_Y_OFFSET, F_UPPER_TEX, F_MID_TEX, F_LOWER_TEX, F_SECTOR };

public:
	const char *UpperTex() const;
	const char *MidTex()   const;
	const char *LowerTex() const;

	Sector *SecRef() const;
};


class LineDef
{
public:
	int start;
	int end;
	int flags;
	int type;
	int tag;
	int right;
	int left;

	enum { F_START, F_END, F_FLAGS, F_TYPE, F_TAG, F_RIGHT, F_LEFT };

public:
	Vertex *Start() const;
	Vertex *End()   const;

	// remember: these two can return NULL!
	SideDef *Right() const;
	SideDef *Left()  const;
};


#define RADF_Square  (1 << 20)

class RadTrig
{
public:
	int mx;  // mid-point
	int my;
	int rw;  // radius (width/2 and height/2)
	int rh;

	int z1;
	int z2;

	int name;
	int tag;
	int options;
	int code;

	enum { F_MX, F_MY, F_RW, F_RH, F_Z1, F_Z2, F_NAME, F_TAG, F_FLAGS, F_CODE };

public:
	const char *Name() const;
	const char *Code() const;

	bool isSquare() const
	{
		return (options & RADF_Square) ? true : false;
	}
};


extern std::vector<Thing *>   Things;
extern std::vector<Vertex *>  Vertices;
extern std::vector<Sector *>  Sectors;
extern std::vector<SideDef *> SideDefs;
extern std::vector<LineDef *> LineDefs;
extern std::vector<RadTrig *> RadTrigs;

#define NumThings     ((int)Things.size())
#define NumVertices   ((int)Vertices.size())
#define NumSectors    ((int)Sectors.size())
#define NumSideDefs   ((int)SideDefs.size())
#define NumLineDefs   ((int)LineDefs.size())
#define NumRadTrigs   ((int)RadTrigs.size())

#endif  /* __E_BASIS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab