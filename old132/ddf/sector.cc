//----------------------------------------------------------------------------
//  EDGE Data Definition File Code (Sectors)
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
// Sector Setup and Parser Code
//
// -KM- 1998/09/27 Written.

#include "local.h"

#include "line.h"

#undef  DF
#define DF  DDF_CMD

#define DDF_SectHashFunc(x)  (((x) + LOOKUP_CACHESIZE) % LOOKUP_CACHESIZE)


void DDF_SectGetSpecialFlags(const char *info, void *storage);
static void DDF_SectMakeCrush(const char *info, void *storage);

#undef  DDF_CMD_BASE
#define DDF_CMD_BASE  dummy_sector
static sectortype_c dummy_sector;

static const commandlist_t sect_commands[] =
{
  	DDF_SUB_LIST("FLOOR",    f,      floor_commands),
  	DDF_SUB_LIST("CEILING",  c,      floor_commands),
  	DDF_SUB_LIST("DAMAGE",   damage, damage_commands),

	DF("SECRET", secret, DDF_MainGetBoolean),
	DF("SPECIAL", special_flags, DDF_SectGetSpecialFlags),

	DF("LIGHT_TYPE", l.type, DDF_SectGetLighttype),
	DF("LIGHT_LEVEL", l.level, DDF_MainGetNumeric),
	DF("LIGHT_DARKTIME", l.darktime, DDF_MainGetTime),
	DF("LIGHT_BRIGHTTIME", l.brighttime, DDF_MainGetTime),
	DF("LIGHT_CHANCE", l.chance, DDF_MainGetPercent),
	DF("LIGHT_SYNC", l.sync, DDF_MainGetTime),
	DF("LIGHT_STEP", l.step, DDF_MainGetNumeric),
	DF("EXIT", e_exit, DDF_SectGetExit),
	DF("USE_COLOURMAP", use_colourmap, DDF_MainGetColourmap),
	DF("GRAVITY", gravity, DDF_MainGetFloat),
	DF("FRICTION", friction, DDF_MainGetFloat),
	DF("VISCOSITY", viscosity, DDF_MainGetFloat),
	DF("DRAG", drag, DDF_MainGetFloat),
	DF("AMBIENT_SOUND", ambient_sfx, DDF_MainLookupSound),
	DF("SPLASH_SOUND", splash_sfx, DDF_MainLookupSound),
	DF("WHEN_APPEAR", appear, DDF_MainGetWhenAppear),
	DF("PUSH_ANGLE", push_angle, DDF_MainGetAngle),
	DF("PUSH_SPEED", push_speed, DDF_MainGetFloat),
	DF("PUSH_ZSPEED", push_zspeed, DDF_MainGetFloat),

	// -AJA- backwards compatibility cruft...
	DF("CRUSH", name, DDF_SectMakeCrush),
	DF("CRUSH_DAMAGE", name, DDF_SectMakeCrush),
	DF("CRUSH_TIME", name, DDF_DummyFunction),

	DF("!DAMAGE", damage.nominal, DDF_MainGetFloat),
	DF("!DAMAGETIME", damage.delay, DDF_MainGetTime),
	DF("!SOUND", name, DDF_DummyFunction),
	DF("!LIGHT_PROBABILITY", name, DDF_DummyFunction),

	DDF_CMD_END
};


sectortype_container_c sectortypes; 	// <-- User-defined

static sectortype_c *default_sectortype;

static sectortype_c *dynamic_sector;

//
//  DDF PARSE ROUTINES
//

static void SectorStartEntry(const char *name, bool extend)
{
	int number = MAX(0, atoi(name));

	if (number == 0)
		DDF_Error("Bad sector-type number in sectors.ddf: %s\n", name);

	dynamic_sector = sectortypes.Lookup(number);

	if (extend)
	{
		if (! dynamic_sector)
			DDF_Error("Unknown sector-type to extend: %s\n", name);
		return;
	}

	// replaces the existing entry
	if (dynamic_sector)
	{
		dynamic_sector->Default();
		return;
	}

	// create a new one and insert it
	dynamic_sector = new sectortype_c;
	dynamic_sector->name = name;

	sectortypes.Insert(dynamic_sector);
}


static void SectorDoTemplate(const char *contents, int index)
{
	if (index > 0)
		DDF_Error("Template must be a single name (not a list).\n");

	int number = MAX(0, atoi(contents));
	if (number == 0)
		DDF_Error("Bad sector-type number for template: %s\n", contents);

	sectortype_c *other = sectortypes.Lookup(number);
	if (!other || other == dynamic_sector)
		DDF_Error("Unknown sector template: '%s'\n", contents);

	dynamic_sector->CopyDetail(*other);
}


static void SectorParseField(const char *field, const char *contents,
							 int index, bool is_last)
{
#if (DEBUG_DDF)  
	I_Debugf("SECTOR_PARSE: %s = %s;\n", field, contents);
#endif

	if (DDF_CompareName(field, "TEMPLATE") == 0)
	{
		SectorDoTemplate(contents, index);
		return;
	}

	if (! DDF_MainParseField((char *)dynamic_sector, sect_commands, field, contents))
	{
		DDF_WarnError("Unknown sectors.ddf command: %s\n", field);
	}
}


static void SectorFinishEntry(void)
{
	// TODO check stuff
}


static void SectorClearAll(void)
{
	// it is safe to just delete all sector types
	sectortypes.Reset();
}


readinfo_t sector_readinfo =
{
	"DDF_InitSectors",  // message
	"SECTORS",  // tag

	SectorStartEntry,
	SectorParseField,
	SectorFinishEntry,
	SectorClearAll
};


void DDF_SectorInit(void)
{
	sectortypes.Reset();
}

void DDF_SectorCleanUp(void)
{
	sectortypes.Trim();

	default_sectortype = new sectortype_c();
	default_sectortype->name = "0";
}


//----------------------------------------------------------------------------

static specflags_t sector_specials[] =
{
	{"WHOLE_REGION", SECSP_WholeRegion, 0},
	{"PROPORTIONAL", SECSP_Proportional, 0},
	{"PUSH_ALL", SECSP_PushAll, 0},
	{"PUSH_CONSTANT", SECSP_PushConstant, 0},
	{"AIRLESS", SECSP_AirLess, 0},
	{"SWIM", SECSP_Swimming, 0},
	{NULL, 0, 0}
};

//
// DDF_SectGetSpecialFlags
//
// Gets the sector specials.
//
void DDF_SectGetSpecialFlags(const char *info, void *storage)
{
	sector_flag_e *sp_flags = (sector_flag_e *) storage;

	int value;

	switch (DDF_MainCheckSpecialFlag(info, sector_specials, &value, true, false))
	{
		case CHKF_Positive:
			*sp_flags = (sector_flag_e)(*sp_flags | value);
			break;

		case CHKF_Negative:
			*sp_flags = (sector_flag_e)(*sp_flags & ~value);
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown sector special: %s", info);
			break;
	}
}

static specflags_t exit_types[] =
{
	{"NONE", EXIT_None, 0},
	{"NORMAL", EXIT_Normal, 0},
	{"SECRET", EXIT_Secret, 0},

	// -AJA- backwards compatibility cruft...
	{"!EXIT", EXIT_Normal, 0},
	{NULL, 0, 0}
};


//
// Get the exit type
//
void DDF_SectGetExit(const char *info, void *storage)
{
	int *dest = (int *)storage;
	int value;

	switch (DDF_MainCheckSpecialFlag(info, exit_types, &value, false, false))
	{
		case CHKF_Positive:
		case CHKF_Negative:
			(*dest) = value;
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown Exit type: %s\n", info);
			break;
	}
}

static specflags_t light_types[] =
{
	{"NONE", LITE_None, 0},
	{"SET",  LITE_Set,  0},
	{"FADE", LITE_Fade, 0},
	{"STROBE", LITE_Strobe, 0},
	{"FLASH",  LITE_Flash,  0},
	{"GLOW",   LITE_Glow,   0},
	{"FLICKER", LITE_FireFlicker, 0},
	{NULL, 0, 0}
};

//
// Get the light type
//
void DDF_SectGetLighttype(const char *info, void *storage)
{
	int *dest = (int *)storage;
	int value;

	switch (DDF_MainCheckSpecialFlag(info, light_types, &value, false, false))
	{
		case CHKF_Positive:
		case CHKF_Negative:
			(*dest) = value;
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown light type: %s\n", info);
			break;
	}
}

static specflags_t movement_types[] =
{
	{"MOVE", mov_Once, 0},
	{"MOVEWAITRETURN", mov_MoveWaitReturn, 0},
	{"CONTINUOUS", mov_Continuous, 0},
	{"PLAT", mov_Plat, 0},
	{"BUILDSTAIRS", mov_Stairs, 0},
	{"STOP", mov_Stop, 0},
	{"TOGGLE", mov_Toggle, 0},
	{"ELEVATOR", mov_Elevator, 0},
	{NULL, 0, 0}
};

//
// Get movement types: MoveWaitReturn etc
//
void DDF_SectGetMType(const char *info, void *storage)
{
	int *dest = (int *)storage;
	int value;

	switch (DDF_MainCheckSpecialFlag(info, movement_types, &value,
		false, false))
	{
		case CHKF_Positive:
		case CHKF_Negative:
			(*dest) = value;
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown Movement type: %s\n", info);
			break;
	}
}

static specflags_t reference_types[] =
{
	{"ABSOLUTE", REF_Absolute, false},

	{"FLOOR", REF_Current, false},
	{"CEILING", REF_Current + REF_CEILING, false},

	{"TRIGGERFLOOR", REF_Trigger, false},
	{"TRIGGERCEILING", REF_Trigger + REF_CEILING, false},

	// Note that LOSURROUNDINGFLOOR has the REF_INCLUDE flag, but the
	// others do not.  It's there to maintain backwards compatibility.
	// 
	{"LOSURROUNDINGCEILING", REF_Surrounding + REF_CEILING, false},
	{"HISURROUNDINGCEILING", REF_Surrounding + REF_CEILING + REF_HIGHEST, false},
	{"LOSURROUNDINGFLOOR", REF_Surrounding + REF_INCLUDE, false},
	{"HISURROUNDINGFLOOR", REF_Surrounding + REF_HIGHEST, false},

	// Note that REF_HIGHEST is used for the NextLowest types, and
	// vice versa, which may seem strange.  It's because the next
	// lowest sector is actually the highest of all adjacent sectors
	// that are lower than the current sector.
	// 
	{"NEXTLOWESTFLOOR", REF_Surrounding + REF_NEXT + REF_HIGHEST, false},
	{"NEXTHIGHESTFLOOR", REF_Surrounding + REF_NEXT, false},
	{"NEXTLOWESTCEILING", REF_Surrounding + REF_NEXT + REF_CEILING + REF_HIGHEST, false},
	{"NEXTHIGHESTCEILING", REF_Surrounding + REF_NEXT + REF_CEILING, false},

	{"LOWESTBOTTOMTEXTURE", REF_LowestLoTexture, false}
};

//
// Get surroundingsectorceiling/floorheight etc
//
void DDF_SectGetDestRef(const char *info, void *storage)
{
	int *dest = (int *)storage;
	int flag_value;

	// check for modifier flags
	if (DDF_CompareName(info, "INCLUDE") == 0)
	{
		*dest |= REF_INCLUDE;
		return;
	}
	else if (DDF_CompareName(info, "EXCLUDE") == 0)
	{
		*dest &= ~REF_INCLUDE;
		return;
	}

	switch (DDF_MainCheckSpecialFlag(info, reference_types, &flag_value,
		false, false))
	{
		case CHKF_Positive:
		case CHKF_Negative:
			(*dest) = flag_value;
			break;

		case CHKF_User:
		case CHKF_Unknown:
			DDF_WarnError("Unknown Reference Point: %s\n", info);
			break;
	}
}

static void DDF_SectMakeCrush(const char *info, void *storage)
{
	sectortype_c *sec = (sectortype_c *) storage;

	sec->f.crush_damage = 10;
	sec->c.crush_damage = 10;
}


//----------------------------------------------------------------------------

// --> Sector type definition class

sectortype_c::sectortype_c() : name()
{
	Default();
}

sectortype_c::~sectortype_c()
{
}


void sectortype_c::Default()
{
	secret = false;
	gravity = GRAVITY;
	friction = FRICTION;
	viscosity = VISCOSITY;
	drag = DRAG;
	
	f.Default(movplanedef_c::DEFAULT_FloorSect);
	c.Default(movplanedef_c::DEFAULT_CeilingSect);

	l.Default();

	damage.Default(damage_c::DEFAULT_Sector);
	
	special_flags = SECSP_None;
	e_exit = EXIT_None;
	use_colourmap = NULL;
	ambient_sfx = NULL;
	splash_sfx = NULL;

	appear = DEFAULT_APPEAR;

	push_speed = 0.0f;
	push_zspeed = 0.0f;

	push_angle = 0;
}

void sectortype_c::CopyDetail(const sectortype_c &src)
{
	secret = src.secret;
	gravity = src.gravity;
	friction = src.friction;
	viscosity = src.viscosity;
	drag = src.drag;

	f = src.f;
	c = src.c;
	l = src.l;

	damage = src.damage;
	
	special_flags = src.special_flags;
	e_exit = src.e_exit;
	
	use_colourmap = src.use_colourmap;
	
	ambient_sfx = src.ambient_sfx;
	splash_sfx = src.splash_sfx;

	appear = src.appear;

	push_speed = src.push_speed;
	push_zspeed = src.push_zspeed;
	push_angle = src.push_angle;
}


// --> Sector definition type container class

//
// sectortype_container_c Constructor
//
sectortype_container_c::sectortype_container_c() : 
	epi::array_c(sizeof(sectortype_c*))
{
	Reset();
}

//
// sectortype_container_c Destructor
//
sectortype_container_c::~sectortype_container_c()
{
	Clear();
}

//
// sectortype_container_c::CleanupObject
//
void sectortype_container_c::CleanupObject(void *obj)
{
	sectortype_c *s = *(sectortype_c**)obj;

	if (s)
		delete s;

	return;
}

//
// sectortype_c* sectortype_container_c::Lookup()
//
// Looks an linetype by id, returns NULL if line can't be found.
//
sectortype_c* sectortype_container_c::Lookup(const int id)
{
	if (id == 0)
		return default_sectortype;

	int slot = DDF_SectHashFunc(id);

	// check the cache
///---	if (lookup_cache[slot] &&
///---		lookup_cache[slot]->ddf.number == id)
///---	{
///---		return lookup_cache[slot];
///---	}	

	epi::array_iterator_c it;
	sectortype_c *s = NULL;

	for (it = GetTailIterator(); it.IsValid(); it--)
	{
		s = ITERATOR_TO_TYPE(it, sectortype_c*);
		if (atoi(s->name.c_str()) == id)
		{
			break;
		}
	}

	if (!it.IsValid())
		return NULL;

	lookup_cache[slot] = s; // Update the cache
	return s;
}

//
// sectortype_container_c::Reset()
//
// Clears down both the data and the cache
//
void sectortype_container_c::Reset()
{
	Clear();
	memset(lookup_cache, 0, sizeof(sectortype_c*) * LOOKUP_CACHESIZE);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
