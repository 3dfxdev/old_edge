//---------------------------------------------------------------------------
//  EDGE Main Init + Program Loop Code
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
// DESCRIPTION:
//      EDGE main program (E_Main),
//      game loop (E_Loop) and startup functions.
//
// -MH- 1998/07/02 "shootupdown" --> "true3dgameplay"
// -MH- 1998/08/19 added up/down movement variables
//

#include "i_defs.h"
#include "e_main.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#include "epi/exe_path.h"
#include "epi/file.h"
#include "epi/filesystem.h"
#include "epi/path.h"
#include "epi/utility.h"

#include "ddf/game.h"
#include "ddf/language.h"

#include "am_map.h"
#include "con_gui.h"
#include "con_main.h"
#include "con_var.h"
#include "g_state.h"
#include "m_strings.h"
#include "e_keys.h"
#include "e_input.h"
#include "f_finale.h"
#include "f_interm.h"
#include "g_game.h"
#include "hu_draw.h"
#include "hu_stuff.h"
#include "m_argv.h"
#include "m_bbox.h"
#include "m_cheat.h"
#include "m_misc.h"
#include "m_menu.h"
#include "n_network.h"
#include "p_setup.h"
#include "p_spec.h"
#include "r_local.h"
#include "rad_trig.h"
#include "r_gldefs.h"
#include "hu_wipe.h"
#include "s_sound.h"
#include "s_music.h"
#include "sv_chunk.h"
#include "sv_main.h"
#include "r_colormap.h"
#include "hu_draw.h"
#include "r_modes.h"
#include "r_image.h"
#include "vm_coal.h"
#include "w_model.h"
#include "w_sprite.h"
#include "w_texture.h"
#include "w_wad.h"
#include "version.h"
#include "z_zone.h"


#define E_TITLE  "EDGE v" EDGEVERSTR

// Application active?
int app_state = APP_STATE_ACTIVE;

bool singletics = false;  // debug flag to cancel adaptiveness

// -ES- 2000/02/13 Takes screenshot every screenshot_rate tics.
// Must be used in conjunction with singletics.
static int screenshot_rate;

// For screenies...
bool m_screenshot_required = false;
bool need_save_screenshot  = false;

FILE *logfile = NULL;
FILE *debugfile = NULL;


int newnmrespawn = 0;

bool swapstereo = false;
bool mus_pause_stop = false;

std::string cfgfile;
std::string ewadfile;
std::string iwad_base;

std::string cache_dir;
std::string ddf_dir;
std::string game_dir;
std::string home_dir;
std::string save_dir;
std::string hub_dir;
std::string shot_dir;

static void E_TitleDrawer(void);

cvar_c ddf_strict;
cvar_c ddf_lax;
cvar_c ddf_quiet;

extern cvar_c r_width;
extern cvar_c r_height;
extern cvar_c r_depth;
extern cvar_c r_fullscreen;

extern cvar_c m_language;
extern cvar_c debug_fullbright;


void E_ProgressMessage(const char *message)
{
	I_Printf("%s", message);
}

void E_LocalProgress(int step, int total)
{
}

void E_GlobalProgress(int step, int size, int total)
{
}


extern void E_SplashScreen(void);


//
// -ACB- 1999/09/20 Created. Sets Global Stuff.
//
static void SetGlobalVars(void)
{
	int p;
	const char *s;

	// Screen Resolution Check...
	s = M_GetParm("-width");
	if (s)
		r_width = atoi(s);

	s = M_GetParm("-height");
	if (s)
		r_height = atoi(s);

	p = M_CheckParm("-res");
	if (p && p + 2 < M_GetArgCount())
	{
		r_width  = atoi(M_GetArgument(p + 1));
		r_height = atoi(M_GetArgument(p + 2));
	}

	// Bits per pixel check....
	s = M_GetParm("-bpp");
	if (! s)
		s = M_GetParm("-depth");
	if (s)
	{
		r_depth = atoi(s);

		if (r_depth.d <= 4) // backwards compat
			r_depth = r_depth.d * 8;
	}

	M_CheckBooleanCVar("fullscreen", &r_fullscreen, false);
	M_CheckBooleanCVar("windowed",   &r_fullscreen, true);

	s = M_GetParm("-screenshot");
	if (s)
	{
		screenshot_rate = atoi(s);
		singletics = true;
	}

	// -AJA- 1999/10/18: Reworked these with M_CheckBooleanParm
	M_CheckBooleanParm("sound", &nosound, true);
	M_CheckBooleanParm("music", &nomusic, true);

	if (M_CheckParm("-nomonsters"))
		debug_nomonsters = 1;

	if (M_CheckParm("-fullbright"))
		debug_fullbright = 1;

	if (M_CheckParm("-ecompat") || M_CheckParm("-edgecompat"))
		edge_compat = 1;

	if (M_CheckParm("-singletics"))
		singletics = true;

///???	M_CheckBooleanParm("itemrespawn", &global_flags.itemrespawn, false);

	// check for strict, lax and no-warning options
	M_CheckBooleanCVar("strict", &ddf_strict, false);
	M_CheckBooleanCVar("lax",    &ddf_lax,    false);
	M_CheckBooleanCVar("warn",   &ddf_quiet,  true);

	strict_errors = ddf_strict.d ? true : false;
	lax_errors    = ddf_lax.d    ? true : false;
	no_warnings   = ddf_quiet.d  ? true : false;
}


void SetLanguage(void)
{
	const char *want_lang = M_GetParm("-lang");
	if (! want_lang)
		want_lang = M_GetParm("-language");

	if (want_lang)
		m_language = want_lang;

	if (language.Select(m_language.str))
		return;

	I_Warning("Invalid language: '%s'\n", m_language.str);

	if (! language.IsValid())
		if (! language.Select(0))
			I_Error("Unable to select any language!");

	m_language = language.GetName();
}


static void ShowNotice(void)
{
	CON_MessageColor(RGB_MAKE(64,192,255));

	I_Printf("%s", language["Notice"]);
}


static void DoSystemStartup(void)
{
	// startup the system now
	R_InitImages();

	I_Debugf("- System startup begun.\n");

	I_SystemStartup();

	// -ES- 1998/09/11 Use R_ChangeResolution to enter gfx mode

	R_DumpResList();

	// -KM- 1998/09/27 Change res now, so music doesn't start before
	// screen.  Reset clock too.
	I_Debugf("- Changing Resolution...\n");

	R_InitialResolution();

	RGL_Init();
	R_SoftInitResolution();

	I_Debugf("- System startup done.\n");
}


static void M_DisplayPause(void)
{
	static const image_c *pause_image = NULL;

	if (! pause_image)
		pause_image = R_ImageLookup("M_PAUSE");

	// make sure image is centered horizontally

	float w = IM_WIDTH(pause_image);

    HUD_DrawImage(160 - w/2, 10, pause_image);
}

static int need_wipe = 0;  // method kind

void E_ForceWipe(void)
{
	if (gamestate == GS_NOTHING)
		return;

	if (r_wipemethod.d > (int)WIPE_None &&
	    r_wipemethod.d < (int)WIPE_NUMWIPES)
	{
		need_wipe = r_wipemethod.d;

		// capture screen now (before new level is loaded etc..)
		E_Display();
	}
}

static bool wipe_gl_active = false;

//
// Draw current display, possibly wiping it from the previous
//
// -ACB- 1998/07/27 Removed doublebufferflag check (unneeded).  
//
void E_Display(void)
{
	if (nodrawers)
		return;  // for comparative timing / profiling

	// Start the frame - should we need to.
	I_StartFrame();

	HUD_FrameSetup();

	// -AJA- 1999/08/02: Make sure palette/gamma is OK.
	V_ColourNewFrame();

	switch (gamestate)
	{
		case GS_LEVEL:
			HU_Erase();

			R_PaletteStuff();

			HU_RunHud();

			if (need_save_screenshot)
			{
				M_MakeSaveScreenShot();
				need_save_screenshot = false;
			}

			HU_Drawer();
			RAD_Drawer();
			break;

		case GS_INTERMISSION:
			WI_Drawer();
			break;

		case GS_FINALE:
			F_Drawer();
			break;

		case GS_TITLESCREEN:
			E_TitleDrawer();
			break;

		case GS_NOTHING:
			break;
	}

	if (wipe_gl_active)
	{
		// -AJA- Wipe code for GL.  Sorry for all this ugliness, but it just
		//       didn't fit into the existing wipe framework.
		//
		if (RGL_DoWipe())
		{
			RGL_EndWipe();
			wipe_gl_active = false;
		}
	}

	// save the current screen if about to wipe
	if (need_wipe > 0)
	{
		RGL_StartWipe((wipetype_e)need_wipe, r_wipereverse.d);
		wipe_gl_active = true;
		need_wipe = 0;
	}

	if (paused)
		M_DisplayPause();

	// menus go directly to the screen
	M_Drawer();  // menu is drawn even on top of everything (except console)

	N_NetUpdate();  // send out any new accumulation

	if (m_screenshot_required)
	{
		m_screenshot_required = false;
		M_ScreenShot(true);
	}
	else if (screenshot_rate && (gamestate >= GS_LEVEL))
	{
		SYS_ASSERT(singletics);

		if (leveltime % screenshot_rate == 0)
			M_ScreenShot(false);
	}

	// draw console _after_ doing screenshots
	CON_Drawer();

	M_DisplayDisk();

	I_FinishFrame();  // page flip or blit buffer
}


//
//  DEMO LOOP
//
static int title_game;
static int title_pic;
static int title_countdown;

static const image_c *title_image = NULL;


static void E_TitleDrawer(void)
{
	if (title_image)
		HUD_StretchImage(0, 0, 320, 200, title_image);
	else
		HUD_SolidBox(0, 0, 320, 200, T_DGREY);
}


//
// This cycles through the demo sequences.
// -KM- 1998/12/16 Fixed for DDF.
//
void E_AdvanceTitle(void)
{
	title_pic++;

	// prevent an infinite loop
	for (int loop=0; loop < 100; loop++)
	{
		gamedef_c *g = gamedefs[title_game];
		SYS_ASSERT(g);

		if (title_pic >= g->titlepics.GetSize())
		{
			title_game = (title_game + 1) % gamedefs.GetSize();
			title_pic  = 0;
			continue;
		}

		// ignore non-existing episodes.  Doesn't include title-only ones
		// like [EDGE].
		if (title_pic == 0 && g->firstmap && g->firstmap[0] &&
			W_CheckNumForName(g->firstmap) == -1)
		{
			title_game = (title_game + 1) % gamedefs.GetSize();
			title_pic  = 0;
			continue;
		}

		// ignore non-existing images
		title_image = R_ImageLookup(g->titlepics[title_pic], INS_Graphic, ILF_Null);

		if (! title_image)
		{
			title_pic++;
			continue;
		}

		// found one !!

		if (title_pic == 0 && g->titlemusic > 0)
			S_ChangeMusic(g->titlemusic, false);

		title_countdown = g->titletics;
		return;
	}

	// not found

	title_image = NULL;
	title_countdown = TICRATE;
}


void E_StartTitle(void)
{
	gameaction = ga_nothing;
	gamestate  = GS_TITLESCREEN;

	paused = false;

	// force pic overflow -> first available titlepic
	title_game = gamedefs.GetSize() - 1;
	title_pic = 29999;
	title_countdown = 1;
 
	E_AdvanceTitle();
}


void E_TitleTicker(void)
{
	if (title_countdown > 0)
	{
		title_countdown--;

		if (title_countdown == 0)
			E_AdvanceTitle();
	}
}


//
// Detects which directories to search for DDFs, WADs and other files in.
//
// -ES- 2000/01/01 Written.
//
void InitDirectories(void)
{
    std::string path;

	const char *s = M_GetParm("-home");
    if (s)
        home_dir = s;

	// Get the Home Directory from environment if set
    if (home_dir.empty())
    {
        s = getenv("HOME");
        if (s)
        {
            home_dir = epi::PATH_Join(s, EDGEHOMESUBDIR); 

			if (! epi::FS_IsDir(home_dir.c_str()))
			{
                epi::FS_MakeDir(home_dir.c_str());

                // Check whether the directory was created
                if (! epi::FS_IsDir(home_dir.c_str()))
                    home_dir.clear();
			}
        }
    }

    if (home_dir.empty())
        home_dir = "."; // Default to current directory

	// Get the Game Directory from parameter.
	game_dir = epi::GetResourcePath();

	s = M_GetParm("-game");
	if (s)
		game_dir = s;

#if 0  // -AJA- the feature is not documented anywhere,
       //       hence nobody should miss it :-)

	// add parameter file "gamedir/parms" if it exists.
	std::string parms = epi::PATH_Join(game_dir.c_str(), "parms");

	if (epi::FS_Access(parms.c_str(), epi::file_c::ACCESS_READ))
	{
		// Insert it right after the game parameter
		M_ApplyResponseFile(parms.c_str(), M_CheckParm("-game") + 2);
	}
#endif

	s = M_GetParm("-ddf");
	if (s)
	{
		ddf_dir = std::string(s);
	} 
	else
	{
		ddf_dir = epi::PATH_Join(game_dir.c_str(), "doom_ddf");
	}

	DDF_SetWhere(ddf_dir);

	// config file
	s = M_GetParm("-config");
	if (s)
	{
		cfgfile = M_ComposeFileName(home_dir.c_str(), s);
	}
	else
    {
        cfgfile = epi::PATH_Join(home_dir.c_str(), EDGECONFIGFILE);
	}

	// edge.wad file
	s = M_GetParm("-ewad");
	if (s)
	{
		ewadfile = M_ComposeFileName(home_dir.c_str(), s);
	}
	else
    {
        ewadfile = epi::PATH_Join(home_dir.c_str(), "edge.wad");
	}

	// cache directory
    cache_dir = epi::PATH_Join(home_dir.c_str(), CACHEDIR);

    if (! epi::FS_IsDir(cache_dir.c_str()))
        epi::FS_MakeDir(cache_dir.c_str());

	// savegame directory
    save_dir = epi::PATH_Join(home_dir.c_str(), SAVEGAMEDIR);
	
    if (! epi::FS_IsDir(save_dir.c_str()))
        epi::FS_MakeDir(save_dir.c_str());

	// HUB savegame directory
    hub_dir = epi::PATH_Join(save_dir.c_str(), HUBDIR);
	
    if (! epi::FS_IsDir(hub_dir.c_str()))
        epi::FS_MakeDir(hub_dir.c_str());

	// screenshot directory
    shot_dir = epi::PATH_Join(home_dir.c_str(), SCRNSHOTDIR);

    if (!epi::FS_IsDir(shot_dir.c_str()))
        epi::FS_MakeDir(shot_dir.c_str());
}


//
// Adds an IWAD and EDGE.WAD. -ES-  2000/01/01 Rewritten.
//
const char *wadname[] = { "doom2", "doom", "plutonia", "tnt", "freedoom", "freedm", NULL };

static void IdentifyVersion(void)
{
	I_Debugf("- Identify Version\n");

	// Check -iwad parameter, find out if it is the IWADs directory
    std::string iwad_par;
    std::string iwad_file;
    std::string iwad_dir;

	const char *s = M_GetParm("-iwad");

    iwad_par = std::string(s ? s : "");

    if (! iwad_par.empty())
    {
        if (epi::FS_IsDir(iwad_par.c_str()))
        {
            iwad_dir = iwad_par;
            iwad_par.clear(); // Discard 
        }
    }   

    // If we haven't yet set the IWAD directory, then we check
    // the DOOMWADDIR environment variable
    if (iwad_dir.empty())
    {
        s = getenv("DOOMWADDIR");

        if (s && epi::FS_IsDir(s))
            iwad_dir = std::string(s);
    }

    // Should the IWAD directory not be set by now, then we
    // use our standby option of the current directory.
    if (iwad_dir.empty())
        iwad_dir = ".";

    // Should the IWAD Parameter not be empty then it means
    // that one was given which is not a directory. Therefore
    // we assume it to be a name
    if (!iwad_par.empty())
    {
        std::string fn = iwad_par;
        
        // Is it missing the extension?
        std::string ext = epi::PATH_GetExtension(iwad_par.c_str());
        if (ext.empty())
        {
            fn += ("." EDGEWADEXT);
        }

        // If no directory given use the IWAD directory
        std::string dir = epi::PATH_GetDir(fn.c_str());
        if (dir.empty())
            iwad_file = epi::PATH_Join(iwad_dir.c_str(), fn.c_str()); 
        else
            iwad_file = fn;

        if (!epi::FS_Access(iwad_file.c_str(), epi::file_c::ACCESS_READ))
        {
			I_Error("IdentifyVersion: Unable to add specified '%s'", fn.c_str());
        }
    }
    else
    {
        const char *location;
		
        int max = 1;

        if (stricmp(iwad_dir.c_str(), game_dir.c_str()) != 0) 
        {
            // IWAD directory & game directory differ 
            // therefore do a second loop which will
            // mean we check both.
            max++;
        } 

		bool done = false;
		for (int i = 0; i < max && !done; i++)
		{
			location = (i == 0 ? iwad_dir.c_str() : game_dir.c_str());

			//
			// go through the available wad names constructing an access
			// name for each, adding the file if they exist.
			//
			// -ACB- 2000/06/08 Quit after we found a file - don't load
			//                  more than one IWAD
			//
			for (int w_idx=0; wadname[w_idx]; w_idx++)
			{
				std::string fn(epi::PATH_Join(location, wadname[w_idx]));

                fn += ("." EDGEWADEXT);

				if (epi::FS_Access(fn.c_str(), epi::file_c::ACCESS_READ))
				{
                    iwad_file = fn;
					done = true;
					break;
				}
			}
		}
    }

	if (iwad_file.empty())
		I_Error("IdentifyVersion: No IWADS found!\n");

    W_AddRawFilename(iwad_file.c_str(), FLKIND_IWad);

    iwad_base = epi::PATH_GetBasename(iwad_file.c_str());

	I_Debugf("IWAD BASE = [%s]\n", iwad_base.c_str());

    // Emulate this behaviour?

    // Look for the required wad in the IWADs dir and then the gamedir
    std::string reqwad(epi::PATH_Join(iwad_dir.c_str(), REQUIREDWAD "." EDGEWADEXT));

    if (! epi::FS_Access(reqwad.c_str(), epi::file_c::ACCESS_READ))
    {
        reqwad = epi::PATH_Join(game_dir.c_str(), REQUIREDWAD "." EDGEWADEXT);

        if (! epi::FS_Access(reqwad.c_str(), epi::file_c::ACCESS_READ))
        {
            I_Error("IdentifyVersion: Could not find required %s.%s!\n", 
                    REQUIREDWAD, EDGEWADEXT);
        }
    }

    W_AddRawFilename(reqwad.c_str(), FLKIND_EWad);
}

static void CheckTurbo(void)
{
	int turbo_scale = 100;

	int p = M_CheckParm("-turbo");

	if (p)
	{
		if (p + 1 < M_GetArgCount())
			turbo_scale = atoi(M_GetArgument(p + 1));
		else
			turbo_scale = 200;

		if (turbo_scale < 10)  turbo_scale = 10;
		if (turbo_scale > 400) turbo_scale = 400;

		CON_MessageLDF("TurboScale", turbo_scale);
	}

	E_SetTurboScale(turbo_scale);
}


static void ShowDateAndVersion(void)
{
	time_t cur_time;
	char timebuf[100];

	time(&cur_time);
	strftime(timebuf, 99, "%I:%M %p on %d/%b/%Y", localtime(&cur_time));

	I_Debugf("[Log file created at %s]\n\n", timebuf);
	I_Debugf("[Debug file created at %s]\n\n", timebuf);

	// 23-6-98 KM Changed to hex to allow versions such as 0.65a etc
	I_Printf("EDGE v" EDGEVERSTR " compiled on " __DATE__ " at " __TIME__ "\n");
	I_Printf("EDGE homepage is at http://edge.sourceforge.net/\n");
	I_Printf("EDGE is based on DOOM by id Software http://www.idsoftware.com/\n");

	I_Printf("Executable path: '%s'\n", 
		epi::GetExecutablePath(M_GetArgument(0)).c_str());

	M_DebugDumpArgs();
}

static void SetupLogAndDebugFiles(void)
{
	// -AJA- 2003/11/08 The log file gets all CON_Printfs, I_Printfs,
	//                  I_Warnings and I_Errors.

	std::string log_fn  (epi::PATH_Join(home_dir.c_str(), EDGELOGFILE));
	std::string debug_fn(epi::PATH_Join(home_dir.c_str(), "debug.txt"));

	logfile = NULL;
	debugfile = NULL;

	if (! M_CheckParm("-nolog"))
	{
		logfile = fopen(log_fn.c_str(), "w");

		if (!logfile)
			I_Error("[E_Startup] Unable to create log file\n");
	}

	//
	// -ACB- 1998/09/06 Only used for debugging.
	//                  Moved here to setup debug file for DDF Parsing...
	//
	// -ES- 1999/08/01 Debugfiles can now be used without -DDEVELOPERS, and
	//                 then logs all the CON_Printfs, I_Printfs and I_Errors.
	//
	// -ACB- 1999/10/02 Don't print to console, since we don't have a console yet.

	/// int p = M_CheckParm("-debug");
	if (true)
	{
		debugfile = fopen(debug_fn.c_str(), "w");

		if (!debugfile)
			I_Error("[E_Startup] Unable to create debugfile");
	}
}

static void AddSingleCmdLineFile(const char *name)
{
    std::string ext = epi::PATH_GetExtension(name);
	int kind = FLKIND_Lump;

	if (stricmp(ext.c_str(), "edm") == 0)
		I_Error("Demos are no longer supported\n");

	// no need to check for GWA (shouldn't be added manually)

	if (stricmp(ext.c_str(), "wad") == 0)
		kind = FLKIND_PWad;
	else if (stricmp(ext.c_str(), "hwa") == 0)
		kind = FLKIND_HWad;
	else if (stricmp(ext.c_str(), "scr") == 0)
		kind = FLKIND_Script;
	else if (stricmp(ext.c_str(), "ddf") == 0 ||
			 stricmp(ext.c_str(), "ldf") == 0)
		kind = FLKIND_DDF;
	else if (stricmp(ext.c_str(), "deh") == 0 ||
			 stricmp(ext.c_str(), "bex") == 0)
		kind = FLKIND_Deh;

	std::string fn = M_ComposeFileName(game_dir.c_str(), name);

	W_AddRawFilename(fn.c_str(), kind);
}

static void AddCommandLineFiles(void)
{
	// first handle "loose" files (arguments before the first option)

	int p;
	const char *ps;

	for (p = 1; p < M_GetArgCount() && '-' != (ps = M_GetArgument(p))[0]; p++)
	{
		AddSingleCmdLineFile(ps);
	}

	// next handle the -file option (we allow multiple uses)

	p = M_CheckNextParm("-file", 0);
	
	while (p)
	{
		// the parms after p are wadfile/lump names,
		// go until end of parms or another '-' preceded parm

		for (p++; p < M_GetArgCount() && '-' != (ps = M_GetArgument(p))[0]; p++)
		{
			AddSingleCmdLineFile(ps);
		}

		p = M_CheckNextParm("-file", p-1);
	}

	// scripts....

	p = M_CheckNextParm("-script", 0);
	
	while (p)
	{
		// the parms after p are script filenames,
		// go until end of parms or another '-' preceded parm

		for (p++; p < M_GetArgCount() && '-' != (ps = M_GetArgument(p))[0]; p++)
		{
			std::string ext = epi::PATH_GetExtension(ps);

			// sanity check...
			if (stricmp(ext.c_str(), "wad") == 0 || 
                stricmp(ext.c_str(), "gwa") == 0 ||
			    stricmp(ext.c_str(), "hwa") == 0 ||
                stricmp(ext.c_str(), "ddf") == 0 ||
			    stricmp(ext.c_str(), "deh") == 0 ||
			    stricmp(ext.c_str(), "bex") == 0)
			{
				I_Error("Illegal filename for -script: %s\n", ps);
			}

			std::string fn = M_ComposeFileName(game_dir.c_str(), ps);

			W_AddRawFilename(fn.c_str(), FLKIND_Script);
		}

		p = M_CheckNextParm("-script", p-1);
	}


	// finally handle the -deh option(s)

	p = M_CheckNextParm("-deh", 0);
	
	while (p)
	{
		// the parms after p are Dehacked/BEX filenames,
		// go until end of parms or another '-' preceded parm

		for (p++; p < M_GetArgCount() && '-' != (ps = M_GetArgument(p))[0]; p++)
		{
			std::string ext(epi::PATH_GetExtension(ps));

			// sanity check...
			if (stricmp(ext.c_str(), "wad") == 0 || 
                stricmp(ext.c_str(), "gwa") == 0 ||
			    stricmp(ext.c_str(), "hwa") == 0 ||
                stricmp(ext.c_str(), "ddf") == 0 ||
			    stricmp(ext.c_str(), "scr") == 0)
			{
				I_Error("Illegal filename for -deh: %s\n", ps);
			}

			std::string fn = M_ComposeFileName(game_dir.c_str(), ps);

			W_AddRawFilename(fn.c_str(), FLKIND_Deh);
		}

		p = M_CheckNextParm("-deh", p-1);
	}
}

static void InitDDF(void)
{
	I_Debugf("- Initialising DDF\n");

	DDF_Init(EDGEVER);
}


void E_EngineShutdown(void)
{
	N_QuitNetGame();

	S_StopMusic();

	// Pause to allow sounds to finish
	for (int loop=0; loop < 30; loop++)
	{
		S_SoundTicker(); 
		I_Sleep(50);
	}

    S_Shutdown();
}

typedef struct
{
	int prog_time;  // rough indication of progress time
	void (*function)(void);
}
startuporder_t;

startuporder_t startcode[] =
{
	{  1, InitDDF              },
	{  1, IdentifyVersion      },
	{  1, AddCommandLineFiles  },
	{  1, CheckTurbo           },
	{  1, RAD_Init             },
	{  4, W_InitMultipleFiles  },
	{  1, V_InitPalette        },
	{  2, HU_Init              },
	{  3, W_InitFlats          },
	{ 10, W_InitTextures       },
	{  1, CON_Start            },
	{ 20, W_ReadDDF            },
	{  1, DDF_CleanUp          },
	{  1, SetLanguage          },
	{  1, ShowNotice           },
	{  1, SV_MainInit          },
	{ 10, R_ImageCreateUser    },
	{ 20, W_InitSprites        },
	{  1, W_InitModels         },
	{  1, M_Init               },
	{  3, R_Init               },
	{  1, P_Init               },
	{  1, P_MapInit            },
	{  1, P_InitSwitchList     },
	{  1, W_InitPicAnims       },
	{  1, S_Init               },
	{  1, N_InitNetwork        },
	{  1, M_CheatInit          },
	{  1, VM_InitCoal          },
	{  8, VM_LoadScripts       },
	{  0, NULL                 }
};

extern void WLF_InitMaps(void); //!!!

// Local Prototypes
static void E_Startup();
static void E_Shutdown(void);


static void E_Startup(void)
{
	int p;

	// Version check ?
	if (M_CheckParm("-version"))
	{
		// -AJA- using I_Error here, since I_Printf crashes this early on
		I_Error("\nEDGE version is " EDGEVERSTR "\n");
	}

	InitDirectories();

	SetupLogAndDebugFiles();

	CON_InitConsole();

	ShowDateAndVersion();

	CON_ResetAllVars(true);
	E_ResetAllBinds();

	M_LoadDefaults();

	CON_HandleProgramArgs();
	SetGlobalVars();

	DoSystemStartup();

	CON_CreateFont();

	if (! M_CheckParm("-nosplash"))
		E_SplashScreen();

	I_PutTitle(E_TITLE); // Needs to be done once the system is up and running

	// RGL_FontStartup();

	E_GlobalProgress(0, 0, 1);

	int total=0;
	int cur=0;

	for (p=0; startcode[p].function != NULL; p++)
		total += startcode[p].prog_time;

	// Cycle through all the startup functions
	for (p=0; startcode[p].function != NULL; p++)
	{
		E_GlobalProgress(cur, startcode[p].prog_time, total);

		startcode[p].function();

		cur += startcode[p].prog_time;
	}

	E_GlobalProgress(100, 0, 100);
}


static void E_Shutdown(void)
{
	/* TODO: E_Shutdown */
}


static void E_InitialState(void)
{
	I_Debugf("- Setting up Initial State...\n");

	const char *ps;

	// do demos and loadgames first, as they contain all of the
	// necessary state already (in the demo file / savegame).

	if (M_CheckParm("-playdemo") || M_CheckParm("-timedemo") ||
	    M_CheckParm("-record"))
	{
		I_Error("Demos are no longer supported\n");
	}

	ps = M_GetParm("-loadgame");
	if (ps)
	{
		G_DeferredLoadGame(atoi(ps));
		return;
	}

	bool warp = false;

	// get skill / episode / map from parms
	std::string warp_map;
	int warp_skill = sk_medium;
	int warp_gametype = GT_Single;

	int bots = 0;

	ps = M_GetParm("-bots");
	if (ps)
		bots = atoi(ps);

	ps = M_GetParm("-warp");
	if (ps)
	{
		warp = true;
		warp_map = std::string(ps);
	}

	// -KM- 1999/01/29 Use correct skill: 1 is easiest, not 0
	ps = M_GetParm("-skill");
	if (ps)
	{
		warp = true;
		warp_skill = atoi(ps);
	}

	// deathmatch check...
	int pp = M_CheckParm("-deathmatch");
	if (pp)
	{
		warp_gametype = GT_DeathMatch;

		if (pp + 1 < M_GetArgCount())
			warp_gametype = MAX(1, atoi(M_GetArgument(pp + 1)));
	}
	else if (M_CheckParm("-altdeath") > 0)
	{
		warp_gametype = GT_AltDeath;
	}


	if (M_GetParm("-record"))
		warp = true;

	// start the appropriate game based on parms
	if (! warp)
	{
		I_Debugf("- Startup: showing title screen.\n");
		E_StartTitle();
		return;
	}

	newgame_params_c params;

	params.skill = warp_skill;	
	params.gametype = warp_gametype;	

	if (warp_map.length() > 0)
		params.map = G_LookupMap(warp_map.c_str());
	else
		params.map = G_LookupMap("1");

	if (! params.map)
		I_Error("-warp: no such level '%s'\n", warp_map.c_str());

	SYS_ASSERT(G_MapExists(params.map));
	SYS_ASSERT(params.map->episode);

	params.random_seed = I_PureRandom();

	params.SinglePlayer(bots);

	G_DeferredNewGame(params);
}


//
// ---- MAIN ----
//
// -ACB- 1998/08/10 Removed all reference to a gamemap, episode and mission
//                  Used LanguageLookup() for lang specifics.
//
// -ACB- 1998/09/06 Removed all the unused code that no longer has
//                  relevance.    
//
// -ACB- 1999/09/04 Removed statcopy parm check - UNUSED
//
// -ACB- 2004/05/31 Moved into a namespace, the c++ revolution begins....
//
void E_Main(int argc, const char **argv)
{
	// Start the EPI Interface 
	epi::Init();

	// Implemented here - since we need to bring the memory manager up first
	// -ACB- 2004/05/31
	M_InitArguments(argc, argv);

	try
	{
		E_Startup();

		E_InitialState();

		CON_MessageColor(RGB_MAKE(255,255,0));
		I_Printf("EDGE v" EDGEVERSTR " initialisation complete.\n");

		I_Debugf("- Entering game loop...\n");

		while (! (app_state & APP_STATE_PENDING_QUIT))
		{
			// We always do this once here, although the engine may
			// makes in own calls to keep on top of the event processing
			I_ControlGetEvents(); 

			if (app_state & APP_STATE_ACTIVE)
				E_Tick();
		}
	}
	catch(...)
	{
		I_Error("Unexpected internal failure occurred!\n");
	}

	E_Shutdown();    // Shutdown whatever at this point

	// Kill the epi interface
	epi::Shutdown();
}


//
// Called when this application has lost focus (i.e. an ALT+TAB event)
//
void E_Idle(void)
{
	E_ReleaseAllKeys();
}


//
// This Function is called for a single loop in the system.
//
// -ACB- 1999/09/24 Written
// -ACB- 2004/05/31 Namespace'd
//
void E_Tick(void)
{
	// -ES- 1998/09/11 It's a good idea to frequently check the heap
#ifdef DEVELOPERS
	//Z_CheckHeap();
#endif

	G_BigStuff();

	// Update display, next frame, with current state.
	E_Display();

	bool fresh_game_tic;

	// this also runs the responder chain via E_ProcessEvents
	int counts = N_TryRunTics(&fresh_game_tic);

	SYS_ASSERT(counts > 0);

	for (; counts > 0; counts--)  // run the tics
	{
		CON_Ticker();
		M_Ticker();

		if (fresh_game_tic)
			G_Ticker();

		S_SoundTicker(); 
		S_MusicTicker(); // -ACB- 1999/11/13 Improved music update routines

		N_NetUpdate();  // check for new console commands
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
