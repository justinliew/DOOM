// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//	plus functions to determine game mode (shareware, registered),
//	parse command line parameters, configure game parameters (turbo),
//	and call the startup functions.
//
//-----------------------------------------------------------------------------


static const char rcsid[] = "$Id: d_main.c,v 1.8 1997/02/03 22:45:09 b1 Exp $";

#define BGCOLOR 7
#define FGCOLOR 8

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/html5.h>
#include <SDL/SDL.h>

SDL_Surface *sdl_screen;
#endif

#ifdef XQD
#include "x_cache.h"
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


#include "doomdef.h"
#include "doomstat.h"

#include "dstrings.h"
#include "sounds.h"


#include "s_sound.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "f_finale.h"
#include "f_wipe.h"

#include "m_argv.h"
#include "m_menu.h"
#include "m_misc.h"

#include "i_sound.h"
#include "i_system.h"
#include "i_video.h"

#include "g_game.h"

#include "am_map.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "wi_stuff.h"

#include "p_setup.h"
#include "r_local.h"


#include "d_main.h"
#include "d_ticcmd.h"

#include "base64.h"

#include "memorywad.h"

#include "miniz.h"

#ifdef XQD
#include "xqd.h"
#endif

//
// D-DoomLoop()
// Not a globally visible function,
//  just included for source reference,
//  called by D_DoomMain, never exits.
// Manages timing and IO,
//  calls all ?_Responder, ?_Ticker, and ?_Drawer,
//  calls I_GetTime, I_StartFrame, and I_StartTic
//
void
D_DoomLoop(void);


char *wadfiles[MAXWADFILES];

boolean zipped_response = false;


boolean devparm;     // started game with -devparm
boolean nomonsters;  // checkparm of -nomonsters
boolean respawnparm; // checkparm of -respawn
boolean fastparm;    // checkparm of -fast

boolean drone;

boolean singletics = false; // debug flag to cancel adaptiveness


// extern int soundVolume;
// extern  int	sfxVolume;
// extern  int	musicVolume;

extern boolean inhelpscreens;

skill_t startskill;
int     startepisode;
int     startmap;
boolean autostart;

FILE *debugfile;

boolean advancedemo;


char wadfile[1024];     // primary wad file
char mapdir[1024];      // directory of development maps
char basedefault[1024]; // default file


void
D_CheckNetGame(int playerindex);
void
D_ProcessEvents(void);
void
G_BuildTiccmd(ticcmd_t *cmd);
void
D_DoAdvanceDemo(void);


//
// EVENT HANDLING
//
// Events are asynchronous inputs generally generated by the game user.
// Events can be discarded if no responder claims them
//
event_t events[MAXEVENTS];
int     eventhead;
int     eventtail;


//
// D_PostEvent
// Called by the I/O functions when input is detected
//
void
D_PostEvent(event_t *ev)
{
	events[eventhead] = *ev;
	eventhead = (++eventhead) & (MAXEVENTS - 1);
}


//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//
void
D_ProcessEvents(void)
{
	event_t *ev;

	// IF STORE DEMO, DO NOT ACCEPT INPUT
	if ((gamemode == commercial) && (W_CheckNumForName("map01") < 0))
		return;

	for (; eventtail != eventhead; eventtail = (++eventtail) & (MAXEVENTS - 1)) {
		ev = &events[eventtail];
		if (M_Responder(ev))
			continue; // menu ate the event
		G_Responder(ev);
	}
}


//
// D_Display
//  draw current display, possibly wiping it from the previous
//

// wipegamestate can be set to -1 to force a wipe on the next draw
gamestate_t    wipegamestate = GS_DEMOSCREEN;
extern boolean setsizeneeded;
extern int     showMessages;
void
R_ExecuteSetViewSize(void);

void
D_Display(void)
{
	static boolean     viewactivestate = false;
	static boolean     menuactivestate = false;
	static boolean     inhelpscreensstate = false;
	static boolean     fullscreen = false;
	static gamestate_t oldgamestate = -1;
	static int         borderdrawcount;
	int                nowtime;
	int                tics;
	int                wipestart;
	int                y;
	boolean            done;
	boolean            wipe;
	boolean            redrawsbar;

	if (nodrawers)
		return; // for comparative timing / profiling

	redrawsbar = false;

	// change the view size if needed
	if (setsizeneeded) {
		R_ExecuteSetViewSize();
		oldgamestate = -1; // force background redraw
		borderdrawcount = 3;
	}

	// save the current screen if about to wipe
	if (gamestate != wipegamestate) {
		wipe = true;
		wipe_StartScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);
	} else
		wipe = false;

	if (gamestate == GS_LEVEL && gametic)
		HU_Erase();

	// do buffered drawing
	switch (gamestate) {
	case GS_LEVEL:
		if (!gametic)
			break;
		if (automapactive)
			AM_Drawer();
		if (wipe || (viewheight != 200 && fullscreen))
			redrawsbar = true;
		if (inhelpscreensstate && !inhelpscreens)
			redrawsbar = true; // just put away the help screen
		ST_Drawer(viewheight == 200, redrawsbar);
		fullscreen = viewheight == 200;
		break;

	case GS_INTERMISSION:
		WI_Drawer();
		break;

	case GS_FINALE:
		F_Drawer();
		break;

	case GS_DEMOSCREEN:
		D_PageDrawer();
		break;
	}

	// draw buffered stuff to screen
#ifndef HEADLESS
	I_UpdateNoBlit();
#endif

	// draw the view directly
	if (gamestate == GS_LEVEL && !automapactive && gametic)
		R_RenderPlayerView(&players[displayplayer]);

	if (gamestate == GS_LEVEL && gametic)
		HU_Drawer();

		// clean up border stuff
#ifndef HEADLESS
	if (gamestate != oldgamestate && gamestate != GS_LEVEL)
		I_SetPalette(W_CacheLumpName("PLAYPAL", PU_CACHE));
#endif
	// see if the border needs to be initially drawn
	if (gamestate == GS_LEVEL && oldgamestate != GS_LEVEL) {
		viewactivestate = false; // view was not active
		R_FillBackScreen();      // draw the pattern into the back screen
	}

	// see if the border needs to be updated to the screen
	if (gamestate == GS_LEVEL && !automapactive && scaledviewwidth != 320) {
		if (menuactive || menuactivestate || !viewactivestate)
			borderdrawcount = 3;
		if (borderdrawcount) {
			R_DrawViewBorder(); // erase old menu stuff
			borderdrawcount--;
		}
	}

	menuactivestate = menuactive;
	viewactivestate = viewactive;
	inhelpscreensstate = inhelpscreens;
	oldgamestate = wipegamestate = gamestate;

	// draw pause pic
	if (paused) {
		if (automapactive)
			y = 4;
		else
			y = viewwindowy + 4;
		V_DrawPatchDirect(viewwindowx + (scaledviewwidth - 68) / 2, y, 0, W_CacheLumpName("M_PAUSE", PU_CACHE));
	}


	// menus go directly to the screen
	M_Drawer();  // menu is drawn even on top of everything
	NetUpdate(false); // send out any new accumulation


	// normal update
	if (!wipe) {
#ifndef HEADLESS
		I_FinishUpdate(); // page flip or blit buffer
#endif
		return;
	}

	// wipe update
	wipe_EndScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);

	wipestart = I_GetTime() - 1;

#ifndef HEADLESS
	do {
		do {
			nowtime = I_GetTime();
			tics = nowtime - wipestart;
		} while (!tics);
		wipestart = nowtime;
		done = wipe_ScreenWipe(wipe_Melt, 0, 0, SCREENWIDTH, SCREENHEIGHT, tics);
		I_UpdateNoBlit();
		M_Drawer(); // menu is drawn even on top of wipes
		I_FinishUpdate(); // page flip or blit buffer
	} while (!done);
#endif
}


//
//  D_DoomLoop
//
extern boolean demorecording;

#ifdef __EMSCRIPTEN__
EM_BOOL
#else
void
#endif
D_OneLoop(void)
{
	// frame syncronous IO operations
#ifndef HEADLESS
	I_StartFrame();
#endif

	// process one or more tics
	if (singletics) {
#ifndef HEADLESS
		I_StartTic();
#endif
		D_ProcessEvents();
		G_BuildTiccmd(&netcmds[consoleplayer][maketic % BACKUPTICS]);
		if (advancedemo)
			D_DoAdvanceDemo();
		M_Ticker();
		G_Ticker();
		gametic++;
		maketic++;
	} else {
		clock_t ticsstart = clock();
		TryRunTics(); // will run at least one tic
		clock_t ticend = clock();
		printf("TryRunTics took  %f\n", 1000.0*(double)(ticend-ticsstart) / CLOCKS_PER_SEC);
	}

	S_UpdateSounds(players[consoleplayer].mo); // move positional sounds

	// Update display, next frame, with current state.
	clock_t dispstart = clock();
	D_Display();
	clock_t dispend = clock();
	printf("D_display took  %f\n", 1000.0*(double)(dispend-dispstart) / CLOCKS_PER_SEC);

#ifdef __EMSCRIPTEN__
	if (SDL_MUSTLOCK(sdl_screen)) SDL_LockSurface(sdl_screen);

    byte*	linear;
    // munge planar buffer to linear
    linear = screens[2];
    I_ReadScreen (linear);

	for (int i=0;i<SCREENWIDTH;++i) {
		for (int j=0;j<SCREENHEIGHT;++j) {
			int alpha = (i+j) % 255;
			int val = linear[i*SCREENHEIGHT + j];

			*((Uint32*)sdl_screen->pixels + i * SCREENHEIGHT + j) = val;
		}
	}
	if (SDL_MUSTLOCK(sdl_screen)) SDL_UnlockSurface(sdl_screen);
	SDL_Flip(sdl_screen);
#endif

#ifndef SNDSERV
	// Sound mixing for the buffer is snychronous.
	I_UpdateSound();
#endif
	// Synchronous sound output is explicitly called.
#ifndef SNDINTR
	// Update sound output.
#ifndef HEADLESS
	I_SubmitSound();
#endif // HEADLESS
#endif // SNDSERV
#ifdef __EMSCRIPTEN__
return EM_TRUE;
#endif
}

unsigned long
hash(byte *data, int len)
{
	unsigned long hash = 5381;

	for (int i=0;i<len;++i) {
		byte b = *data++;
		hash = ((hash << 5) + hash) + b; /* hash * 33 + c */
	}

	return hash;
}

void printDiff(const char* type, byte* data, byte* last, int len ) {
	// the idea here is that delta is small and maybe similar, so we can ideally compress this data using sometihng like Huffman
	int sum = 0;
	int numzero = 0;
	for (int i=0; i < len; ++i) {
		byte delta = data[i] - last[i];
		if (delta == 0) {
			numzero++;
		}
		sum += (int)delta;
	}
	float avg = (float)sum / (float)len;
	printf("%s - avg sum is %f, %f zero\n", type, avg, (float)numzero/(float)len);
}

#ifdef XQD
int sessionid=0;
bool global=false;
int
X_ProcessIncoming(void)
{
	RequestHandle reqhandle;
	BodyHandle bodyhandle;
	int ret = xqd_req_body_downstream_get(&reqhandle, &bodyhandle);

	char uribuf[200];
	size_t nread=0;

	char* bodybuf = Z_Malloc(1000,PU_STATIC,0);
	int bodyindex = 0;
	do {
		ret = xqd_body_read(bodyhandle, bodybuf+bodyindex, 1000-bodyindex, &nread);
		bodyindex += nread;
	} while (nread > 0 && bodyindex < 1000);

	int num_frames = 0;
	int playerindex=0;

	memcpy(&sessionid, bodybuf, sizeof(int));
	sessionid = ntohl(sessionid);

	memcpy(&global, &bodybuf[4], 1);

	int cache_len = 0;
	byte* cache_data = X_GetStateFromCache(global, sessionid, &cache_len);

	G_DoDeserialize(cache_data, cache_len);

	memcpy(&playerindex, &bodybuf[5], sizeof(int));
	playerindex = ntohl(playerindex);

	int num_events = 0;
	memcpy(&num_events, &bodybuf[9], sizeof(int));
	num_events = ntohl(num_events);
	printf("We got %d events\n", num_events);

	for (int e=0;e<num_events;++e) {
		byte event = bodybuf[13+e];
		event_t es;
		es.type = ev_keydown;
		es.data1 = event;
		D_PostEvent(&es);
	}
	memcpy(&num_frames, &bodybuf[13+num_events], sizeof(int));
	num_frames = ntohl(num_frames);
	printf("We are requesting %d frames\n", num_frames);
	doomcom->numplayers=4;
	doomcom->consoleplayer=playerindex;
	Z_Free(bodybuf);
	return num_frames;
}

void
X_RunAndSendResponse(int num_frames)
{
	const expected_ss_len = SCREENWIDTH*SCREENHEIGHT+768;
	if (num_frames > 10 || num_frames < 1) {
		num_frames = 3;
	}

	byte* frame_ptrs[10];
	for (int i=0;i<num_frames;++i) {
		D_OneLoop();

		// get framebuffer
		int ss_len;
		byte* ss = M_InMemoryScreenShot(&ss_len);
		if (ss_len != expected_ss_len) {
			I_Error("We got a bad framebuffer of %d bytes\n", ss_len);
		}
		frame_ptrs[i] = ss;
	}

	// get gamestate
	int gs_len;
	byte* gs_data = G_DoSerialize(&gs_len);
	printf("serialize is len %d\n", gs_len);

	const char* pop = getenv("FASTLY_POP");
	int poplen = strlen(pop);
	const char* hostname = getenv("FASTLY_HOSTNAME");
	int hostlen = strlen(hostname);

	// framebuffer * num_frames, num_frames, stateid
	int buflen = expected_ss_len*num_frames + 4*sizeof(int) + poplen + hostlen;
	byte* finalbuffer = Z_Malloc(buflen, PU_LEVEL, 0);
	byte* fbp = finalbuffer;

	memcpy(fbp, &num_frames, sizeof(int));
	fbp += sizeof(int);

	for (int i=0;i<num_frames;++i) {
		memcpy(fbp,frame_ptrs[i],expected_ss_len);
		free(frame_ptrs[i]);
		fbp += expected_ss_len;
	}

	memcpy(fbp, &sessionid, sizeof(int));
	fbp += sizeof(int);


	printf("pop and hostname: %s, %s\n", pop, hostname);

	memcpy(fbp, &poplen, sizeof(int));
	fbp += 4;
	memcpy(fbp, pop, strlen(pop));
	fbp += strlen(pop);
	memcpy(fbp, &hostlen, sizeof(int));
	fbp += 4;
	memcpy(fbp, hostname, strlen(hostname));
	fbp += strlen(hostname);

	ResponseHandle resphandle;
	BodyHandle respbodyhandle;

	printf("Body size is %d\n", buflen);

	xqd_resp_new(&resphandle);
	xqd_body_new(&respbodyhandle);

	if (zipped_response) {
		clock_t zipstart = clock();
		byte* dest = Z_Malloc(buflen, PU_STATIC,0);
		int dest_len = buflen;
		int ret = mz_compress2(dest, &dest_len, finalbuffer, buflen, MZ_BEST_SPEED);
		clock_t zipend = clock();
		printf("	Compression took %f\n", 1000.0 * (double)(zipend-zipstart) / CLOCKS_PER_SEC);
		int nwritten=0;
		ret = xqd_body_write(respbodyhandle, dest, dest_len, BodyWriteEndBack, &nwritten);

		const char* deflate_header_name = "Content-Encoding";
		const char* deflate_header_value = "deflate";
		xqd_resp_header_append(resphandle, deflate_header_name, strlen(deflate_header_name), deflate_header_value, strlen(deflate_header_value) );
	} else {
		int nwritten=0;
		int ret = xqd_body_write(respbodyhandle, finalbuffer, buflen, BodyWriteEndBack, &nwritten);
	}

	const char* cors_header_name = "Access-Control-Allow-Origin";
	const char* cors_header_value = "*";
	xqd_resp_header_append(resphandle, cors_header_name, strlen(cors_header_name), cors_header_value, strlen(cors_header_value) );

	const char* vary_header_name = "Vary";
	const char* vary_header_value = "Origin";
	xqd_resp_header_append(resphandle, vary_header_name, strlen(vary_header_name), vary_header_value, strlen(vary_header_value) );

	int response_res = xqd_resp_send_downstream(resphandle, respbodyhandle, 0);
	X_WriteStateToCache(global, sessionid, gs_data, gs_len);
}
#endif

void
D_DoomLoop(void)
{
	if (demorecording)
		G_BeginRecording();

	if (M_CheckParm("-debugfile")) {
		char filename[20];
		sprintf(filename, "debug%i.txt", consoleplayer);
		printf("debug output to: %s\n", filename);
		debugfile = fopen(filename, "w");
	}

#ifndef HEADLESS
	I_InitGraphics();
#endif


#ifdef __EMSCRIPTEN__
	emscripten_request_animation_frame_loop(D_OneLoop, 0);
#elif defined(XQD)

	clock_t start=clock();
	int num_frames = X_ProcessIncoming();
	clock_t end=clock();
	printf("X_ProcessIncoming took %f\n", 1000.0 * (double)(end-start) / CLOCKS_PER_SEC);

	printf("D_CheckNetGame: Checking network game status.\n");
	D_CheckNetGame(doomcom->consoleplayer);

	// we need to do this here so we get the hud for the proper player
	ST_Start ();
	// wake up the heads up text
	HU_Start ();

	// this runs the setup frame
	D_OneLoop();

	start = clock();
	X_RunAndSendResponse(num_frames);
	end = clock();
	printf("X_RunAndSendResponse took %f\n", 1000.0 * (double)(end-start) / CLOCKS_PER_SEC);
#else

	D_CheckNetGame(0);

//	for (int i=0;i<10;++i)
	byte* last_state = NULL;
	int state_len;
	int frame = 0;
	D_OneLoop();
	D_OneLoop();
	while(1)
	{
		if (last_state != NULL) {
			G_DoDeserialize(last_state, state_len);

			if (frame > 3) {
				consoleplayer=displayplayer=1;
	 			D_CheckNetGame(1);
			} else {
				consoleplayer=displayplayer=0;
	 			D_CheckNetGame(0);
			}
		}

		D_OneLoop();
		++frame;
		int out=0;
		byte* ss = M_InMemoryScreenShot(&out);
		Z_Free(ss);

		last_state = G_DoSerialize(&state_len);
	}
#endif
}


//
//  DEMO LOOP
//
int   demosequence;
int   pagetic;
char *pagename;


//
// D_PageTicker
// Handles timing for warped projection
//
void
D_PageTicker(void)
{
	if (--pagetic < 0)
		D_AdvanceDemo();
}


//
// D_PageDrawer
//
void
D_PageDrawer(void)
{
	V_DrawPatch(0, 0, 0, W_CacheLumpName(pagename, PU_CACHE));
}


//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
void
D_AdvanceDemo(void)
{
	advancedemo = true;
}


//
// This cycles through the demo sequences.
// FIXME - version dependend demo numbers?
//
void
D_DoAdvanceDemo(void)
{
	players[consoleplayer].playerstate = PST_LIVE; // not reborn
	advancedemo = false;
	usergame = false; // no save / end game here
	paused = false;
	gameaction = ga_nothing;

	if (gamemode == retail)
		demosequence = (demosequence + 1) % 7;
	else
		demosequence = (demosequence + 1) % 6;

	switch (demosequence) {
	case 0:
		if (gamemode == commercial)
			pagetic = 35 * 11;
		else
			pagetic = 170;
		gamestate = GS_DEMOSCREEN;
		pagename = "TITLEPIC";
		if (gamemode == commercial)
			S_StartMusic(mus_dm2ttl);
		else
			S_StartMusic(mus_intro);
		break;
	case 1:
		G_DeferedPlayDemo("demo1");
		break;
	case 2:
		pagetic = 200;
		gamestate = GS_DEMOSCREEN;
		pagename = "CREDIT";
		break;
	case 3:
		G_DeferedPlayDemo("demo2");
		break;
	case 4:
		gamestate = GS_DEMOSCREEN;
		if (gamemode == commercial) {
			pagetic = 35 * 11;
			pagename = "TITLEPIC";
			S_StartMusic(mus_dm2ttl);
		} else {
			pagetic = 200;

			if (gamemode == retail)
				pagename = "CREDIT";
			else
				pagename = "HELP2";
		}
		break;
	case 5:
		G_DeferedPlayDemo("demo3");
		break;
		// THE DEFINITIVE DOOM Special Edition demo
	case 6:
		G_DeferedPlayDemo("demo4");
		break;
	}
}


//
// D_StartTitle
//
void
D_StartTitle(void)
{
	gameaction = ga_nothing;
	demosequence = -1;
	D_AdvanceDemo();
}


//      print title for every printed line
char title[128];


//
// D_AddFile
//
void
D_AddFile(char *file)
{
	int   numwadfiles;
	char *newfile;

	for (numwadfiles = 0; wadfiles[numwadfiles]; numwadfiles++)
		;

	newfile = malloc(strlen(file) + 1);
	strcpy(newfile, file);

	wadfiles[numwadfiles] = newfile;
}

//
// IdentifyVersion
// Checks availability of IWAD files by name,
// to determine whether registered/commercial features
// should be executed (notably loading PWAD's).
//
void
IdentifyVersion(void)
{

	char *doom1wad;
	char *doomwad;
	char *doomuwad;
	char *doom2wad;

	char *doom2fwad;
	char *plutoniawad;
	char *tntwad;

#ifdef NORMALUNIX
	char *home;
	char *doomwaddir;
	doomwaddir = getenv("DOOMWADDIR");
	if (!doomwaddir)
		doomwaddir = ".";

	// Commercial.
	doom2wad = malloc(strlen(doomwaddir) + 1 + 9 + 1);
	sprintf(doom2wad, "%s/doom2.wad", doomwaddir);

	// Retail.
	doomuwad = malloc(strlen(doomwaddir) + 1 + 8 + 1);
	sprintf(doomuwad, "%s/doomu.wad", doomwaddir);

	// Registered.
	doomwad = malloc(strlen(doomwaddir) + 1 + 8 + 1);
	sprintf(doomwad, "%s/doom.wad", doomwaddir);

	// Shareware.
	doom1wad = malloc(strlen(doomwaddir) + 1 + 9 + 1);
	sprintf(doom1wad, "%s/doom1.wad", doomwaddir);

	// Bug, dear Shawn.
	// Insufficient malloc, caused spurious realloc errors.
	plutoniawad = malloc(strlen(doomwaddir) + 1 + /*9*/ 12 + 1);
	sprintf(plutoniawad, "%s/plutonia.wad", doomwaddir);

	tntwad = malloc(strlen(doomwaddir) + 1 + 9 + 1);
	sprintf(tntwad, "%s/tnt.wad", doomwaddir);


	// French stuff.
	doom2fwad = malloc(strlen(doomwaddir) + 1 + 10 + 1);
	sprintf(doom2fwad, "%s/doom2f.wad", doomwaddir);

	home = getenv("HOME");
	if (!home)
		I_Error("Please set $HOME to your home directory");
	sprintf(basedefault, "%s/.doomrc", home);
#endif

#ifndef WASISDK
	D_AddFile("data/doom1.wad");
#endif

	if (M_CheckParm("-shdev")) {
		gamemode = shareware;
		devparm = true;
		D_AddFile(DEVDATA "doom1.wad");
		D_AddFile(DEVMAPS "data_se/texture1.lmp");
		D_AddFile(DEVMAPS "data_se/pnames.lmp");
		strcpy(basedefault, DEVDATA "default.cfg");
		return;
	}

	if (M_CheckParm("-regdev")) {
		gamemode = registered;
		devparm = true;
		D_AddFile(DEVDATA "doom.wad");
		D_AddFile(DEVMAPS "data_se/texture1.lmp");
		D_AddFile(DEVMAPS "data_se/texture2.lmp");
		D_AddFile(DEVMAPS "data_se/pnames.lmp");
		strcpy(basedefault, DEVDATA "default.cfg");
		return;
	}

	if (M_CheckParm("-comdev")) {
		gamemode = commercial;
		devparm = true;
		/* I don't bother
		if(plutonia)
		    D_AddFile (DEVDATA"plutonia.wad");
		else if(tnt)
		    D_AddFile (DEVDATA"tnt.wad");
		else*/
		D_AddFile(DEVDATA "doom2.wad");

		D_AddFile(DEVMAPS "cdata/texture1.lmp");
		D_AddFile(DEVMAPS "cdata/pnames.lmp");
		strcpy(basedefault, DEVDATA "default.cfg");
		return;
	}

	if (!access(doom2fwad, R_OK)) {
		gamemode = commercial;
		// C'est ridicule!
		// Let's handle languages in config files, okay?
		language = french;
		printf("French version\n");
		D_AddFile(doom2fwad);
		return;
	}

	if (!access(doom2wad, R_OK)) {
		gamemode = commercial;
		D_AddFile(doom2wad);
		return;
	}

	if (!access(plutoniawad, R_OK)) {
		gamemode = commercial;
		D_AddFile(plutoniawad);
		return;
	}

	if (!access(tntwad, R_OK)) {
		gamemode = commercial;
		D_AddFile(tntwad);
		return;
	}

	if (!access(doomuwad, R_OK)) {
		gamemode = retail;
		D_AddFile(doomuwad);
		return;
	}

	if (!access(doomwad, R_OK)) {
		gamemode = registered;
		D_AddFile(doomwad);
		return;
	}

	if (!access(doom1wad, R_OK)) {
		gamemode = shareware;
		D_AddFile(doom1wad);
		return;
	}

	printf("Game mode indeterminate.\n");
	gamemode = indetermined;

	// We don't abort. Let's see what the PWAD contains.
	// exit(1);
	// I_Error ("Game mode indeterminate\n");
}

//
// Find a Response File
//
void
FindResponseFile(void)
{
	int i;
#define MAXARGVS 100

	for (i = 1; i < myargc; i++)
		if (myargv[i][0] == '@') {
			FILE *handle;
			int   size;
			int   k;
			int   index;
			int   indexinfile;
			char *infile;
			char *file;
			char *moreargs[20];
			char *firstargv;

			// READ THE RESPONSE FILE INTO MEMORY
			handle = fopen(&myargv[i][1], "rb");
			if (!handle) {
				printf("\nNo such response file!");
				exit(1);
			}
			printf("Found response file %s!\n", &myargv[i][1]);
			fseek(handle, 0, SEEK_END);
			size = ftell(handle);
			fseek(handle, 0, SEEK_SET);
			file = malloc(size);
			fread(file, size, 1, handle);
			fclose(handle);

			// KEEP ALL CMDLINE ARGS FOLLOWING @RESPONSEFILE ARG
			for (index = 0, k = i + 1; k < myargc; k++)
				moreargs[index++] = myargv[k];

			firstargv = myargv[0];
			myargv = malloc(sizeof(char *) * MAXARGVS);
			memset(myargv, 0, sizeof(char *) * MAXARGVS);
			myargv[0] = firstargv;

			infile = file;
			indexinfile = k = 0;
			indexinfile++; // SKIP PAST ARGV[0] (KEEP IT)
			do {
				myargv[indexinfile++] = infile + k;
				while (k < size && ((*(infile + k) >= ' ' + 1) && (*(infile + k) <= 'z')))
					k++;
				*(infile + k) = 0;
				while (k < size && ((*(infile + k) <= ' ') || (*(infile + k) > 'z')))
					k++;
			} while (k < size);

			for (k = 0; k < index; k++)
				myargv[indexinfile++] = moreargs[k];
			myargc = indexinfile;

			// DISPLAY ARGS
			printf("%d command-line args:\n", myargc);
			for (k = 1; k < myargc; k++)
				printf("%s\n", myargv[k]);

			break;
		}
}

#ifdef XQD
boolean
X_HandleUrl(const char* uri)
{
	if (strstr(uri, "favicon.ico")) {
		return true;
	}
	if (strstr(uri, "sessions")) {
		// TODO - hit our lobby endpoint to get sessions
//		const char* sessions_json = X_GetSessions(false);

		RequestHandle reqHandle;
		BodyHandle bodyHandle, respBodyHandle;
		int res = xqd_req_new(&reqHandle);
		res = xqd_body_new(&bodyHandle);

		const char* uri = "https://loudly-verified-elephant.edgecompute.app";
		const char* name = "lobby";

		res = xqd_req_uri_set(reqHandle, uri, strlen(uri));

		ResponseHandle respHandle;
		res = xqd_req_send(reqHandle, bodyHandle, name, strlen(name), &respHandle, &respBodyHandle );

		byte* buf = (byte*)malloc(1000);
		int bodyindex=0;
		int nread;
		do {
			int ret = xqd_body_read(respBodyHandle, buf+bodyindex, 1000-bodyindex, &nread);
			bodyindex += nread;
		} while (nread > 0 && bodyindex < 1000);


		return true;
	}

	if (strstr(uri, ".well-known/fastly/demo-manifest"))
	{
		ResponseHandle resphandle;
		BodyHandle respbodyhandle;

		xqd_resp_new(&resphandle);
		xqd_body_new(&respbodyhandle);

		const char* data = "---\n\
schemaVersion: 1\n\
id: doom\n\
title: DOOM\n\
image:\n\
  href: /images/screenshot.png\n\
  alt: DOOM on Compute @ Edge Demo\n\
description: |\n\
  A port of the original DOOM to Compute@Edge\n\
  This demo was created to push the boundaries of the \n\
  platform and inspire new ideas!\n\
views:\n\
  endUser:\n\
    mode: frame\n\
    href: /\n\
    height: 600\n\
---\n\
Key bindings:\n\
W to move forward\n\
S to move backward\n\
A to turn left\n\
D to turn right\n\
Q to strafe left\n\
E to strafe right\n\
F to fire gun\n\
U to use\n";

		int nwritten=0;
		int ret = xqd_body_write(respbodyhandle, data, strlen(data), BodyWriteEndBack, &nwritten);

		const char* cors_header_name = "Access-Control-Allow-Origin";
		const char* cors_header_value = "*";
		xqd_resp_header_append(resphandle, cors_header_name, strlen(cors_header_name), cors_header_value, strlen(cors_header_value) );

		const char* vary_header_name = "Vary";
		const char* vary_header_value = "Origin";
		xqd_resp_header_append(resphandle, vary_header_name, strlen(vary_header_name), vary_header_value, strlen(vary_header_value) );

		int response_res = xqd_resp_send_downstream(resphandle, respbodyhandle, 0);
		return true;
	}


	if (!strstr(uri, "doomframe")) {
		printf("Handle root serving\n");
		// we need to serve the html here
		ResponseHandle resphandle;
		BodyHandle respbodyhandle;

		xqd_resp_new(&resphandle);
		xqd_body_new(&respbodyhandle);

		char* data = D_GetIndex();

		int nwritten=0;
		int ret = xqd_body_write(respbodyhandle, data, strlen(data), BodyWriteEndBack, &nwritten);

		const char* cors_header_name = "Access-Control-Allow-Origin";
		const char* cors_header_value = "*";
		xqd_resp_header_append(resphandle, cors_header_name, strlen(cors_header_name), cors_header_value, strlen(cors_header_value) );

		const char* vary_header_name = "Vary";
		const char* vary_header_value = "Origin";
		xqd_resp_header_append(resphandle, vary_header_name, strlen(vary_header_name), vary_header_value, strlen(vary_header_value) );

		int response_res = xqd_resp_send_downstream(resphandle, respbodyhandle, 0);
		return true;
	}
	return false;
}
#endif

//
// D_DoomMain
//
void
D_DoomMain(void)
{
	// very first thing we do is check URL so we can either bail or serve html/js if necessary
#ifdef XQD
	int ret = xqd_init(XQD_ABI_VERSION);

	RequestHandle reqhandle;
	BodyHandle bodyhandle;
	ret = xqd_req_body_downstream_get(&reqhandle, &bodyhandle);
	char uribuf[200];
	size_t nread=0;

	ret = xqd_req_uri_get(reqhandle, uribuf, 200, &nread);
	printf("Read url, length %zu, %s\n", nread, uribuf);
	if (X_HandleUrl(uribuf)) {
		return;
	}
#endif

	int  p;
	char file[256];

	FindResponseFile();

	IdentifyVersion();

	setbuf(stdout, NULL);
	modifiedgame = false;

	nomonsters = M_CheckParm("-nomonsters");
	respawnparm = M_CheckParm("-respawn");
	fastparm = M_CheckParm("-fast");
	devparm = M_CheckParm("-devparm");
	if (M_CheckParm("-altdeath"))
		deathmatch = 2;
	else if (M_CheckParm("-deathmatch"))
		deathmatch = 1;
// forcing deathmatch on
#ifdef WASISDK
	deathmatch = 1;
#endif

	switch (gamemode) {
	case retail:
		sprintf(title,
			"                         "
			"The Ultimate DOOM Startup v%i.%i"
			"                           ",
			VERSION / 100, VERSION % 100);
		break;
	case shareware:
		sprintf(title,
			"                            "
			"DOOM Shareware Startup v%i.%i"
			"                           ",
			VERSION / 100, VERSION % 100);
		break;
	case registered:
		sprintf(title,
			"                            "
			"DOOM Registered Startup v%i.%i"
			"                           ",
			VERSION / 100, VERSION % 100);
		break;
	case commercial:
		sprintf(title,
			"                         "
			"DOOM 2: Hell on Earth v%i.%i"
			"                           ",
			VERSION / 100, VERSION % 100);
		break;
		/*FIXME
		       case pack_plut:
			sprintf (title,
				 "                   "
				 "DOOM 2: Plutonia Experiment v%i.%i"
				 "                           ",
				 VERSION/100,VERSION%100);
			break;
		      case pack_tnt:
			sprintf (title,
				 "                     "
				 "DOOM 2: TNT - Evilution v%i.%i"
				 "                           ",
				 VERSION/100,VERSION%100);
			break;
		*/
	default:
		sprintf(title,
			"                     "
			"Public DOOM - v%i.%i"
			"                           ",
			VERSION / 100, VERSION % 100);
		break;
	}

	printf("%s\n", title);

	if (devparm)
		printf(D_DEVSTR);

	if (M_CheckParm("-cdrom")) {
		printf(D_CDROM);
		mkdir("c:\\doomdata", 0);
		strcpy(basedefault, "c:/doomdata/default.cfg");
	}

	// turbo option
	if ((p = M_CheckParm("-turbo"))) {
		int        scale = 200;
		extern int forwardmove[2];
		extern int sidemove[2];

		if (p < myargc - 1)
			scale = atoi(myargv[p + 1]);
		if (scale < 10)
			scale = 10;
		if (scale > 400)
			scale = 400;
		printf("turbo scale: %i%%\n", scale);
		forwardmove[0] = forwardmove[0] * scale / 100;
		forwardmove[1] = forwardmove[1] * scale / 100;
		sidemove[0] = sidemove[0] * scale / 100;
		sidemove[1] = sidemove[1] * scale / 100;
	}

	// add any files specified on the command line with -file wadfile
	// to the wad list
	//
	// convenience hack to allow -wart e m to add a wad file
	// prepend a tilde to the filename so wadfile will be reloadable
	p = M_CheckParm("-wart");
	if (p) {
		myargv[p][4] = 'p'; // big hack, change to -warp

		// Map name handling.
		switch (gamemode) {
		case shareware:
		case retail:
		case registered:
			sprintf(file, "~" DEVMAPS "E%cM%c.wad", myargv[p + 1][0], myargv[p + 2][0]);
			printf("Warping to Episode %s, Map %s.\n", myargv[p + 1], myargv[p + 2]);
			break;

		case commercial:
		default:
			p = atoi(myargv[p + 1]);
			if (p < 10)
				sprintf(file, "~" DEVMAPS "cdata/map0%i.wad", p);
			else
				sprintf(file, "~" DEVMAPS "cdata/map%i.wad", p);
			break;
		}
		D_AddFile(file);
	}

	p = M_CheckParm("-file");
	if (p) {
		// the parms after p are wadfile/lump names,
		// until end of parms or another - preceded parm
		modifiedgame = true; // homebrew levels
		while (++p != myargc && myargv[p][0] != '-')
			D_AddFile(myargv[p]);
	}

	p = M_CheckParm("-playdemo");

	if (!p)
		p = M_CheckParm("-timedemo");

	if (p && p < myargc - 1) {
		sprintf(file, "%s.lmp", myargv[p + 1]);
		D_AddFile(file);
		printf("Playing demo %s.lmp.\n", myargv[p + 1]);
	}

	// get skill / episode / map from parms
	startskill = sk_medium;
	startepisode = 1;
	startmap = 1;
	autostart = false;


	p = M_CheckParm("-skill");
	if (p && p < myargc - 1) {
		startskill = myargv[p + 1][0] - '1';
		autostart = true;
	}

	p = M_CheckParm("-episode");
	if (p && p < myargc - 1) {
		startepisode = myargv[p + 1][0] - '0';
		startmap = 1;
		autostart = true;
	}
	// TODO - for now we are forcing a SP load
	startepisode = 1;
	startmap = 1;
	autostart = true;

	p = M_CheckParm("-timer");
	if (p && p < myargc - 1 && deathmatch) {
		int time;
		time = atoi(myargv[p + 1]);
		printf("Levels will end after %d minute", time);
		if (time > 1)
			printf("s");
		printf(".\n");
	}

	p = M_CheckParm("-avg");
	if (p && p < myargc - 1 && deathmatch)
		printf("Austin Virtual Gaming: Levels will end after 20 minutes\n");

	p = M_CheckParm("-warp");
	if (p && p < myargc - 1) {
		if (gamemode == commercial)
			startmap = atoi(myargv[p + 1]);
		else {
			startepisode = myargv[p + 1][0] - '0';
			startmap = myargv[p + 2][0] - '0';
		}
		autostart = true;
	}

	// init subsystems
	printf("V_Init: allocate screens.\n");
	V_Init();

	printf("M_LoadDefaults: Load system defaults.\n");
	M_LoadDefaults(); // load before initing other systems

	printf("Z_Init: Init zone memory allocation daemon. \n");
	Z_Init();

	printf("W_Init: Init WADfiles.\n");
#ifdef WASISDK
#include "memorywad.h"
	W_InitFromMemory(D_GetInMemoryWad());
#else
	W_InitMultipleFiles(wadfiles);
#endif

	// Check and print which version is executed.
	switch (gamemode) {
	case shareware:
	case indetermined:
		printf("===========================================================================\n"
		       "                                Shareware!\n"
		       "===========================================================================\n");
		break;
	case registered:
	case retail:
	case commercial:
		printf("===========================================================================\n"
		       "                 Commercial product - do not distribute!\n"
		       "         Please report software piracy to the SPA: 1-800-388-PIR8\n"
		       "===========================================================================\n");
		break;

	default:
		// Ouch.
		break;
	}

	printf("M_Init: Init miscellaneous info.\n");
	M_Init();

	printf("R_Init: Init DOOM refresh daemon - ");
	R_Init();

	printf("\nP_Init: Init Playloop state.\n");
	P_Init();

	printf("I_Init: Setting up machine state.\n");
	I_Init();

	//gotta leave this here so the HUD renders
	// for some reason...
	// we init properly once we have the player index...
	D_CheckNetGame(0);

	printf("S_Init: Setting up sound.\n");
	S_Init(snd_SfxVolume /* *8 */, snd_MusicVolume /* *8*/);

	printf("HU_Init: Setting up heads up display.\n");
	HU_Init();

	printf("ST_Init: Init status bar.\n");
	ST_Init();

	// check for a driver that wants intermission stats
	p = M_CheckParm("-statcopy");
	if (p && p < myargc - 1) {
		// for statistics driver
		extern void *statcopy;

		statcopy = (void *)atoi(myargv[p + 1]);
		printf("External statistics registered.\n");
	}

	// start the apropriate game based on parms
	p = M_CheckParm("-record");

	if (p && p < myargc - 1) {
		G_RecordDemo(myargv[p + 1]);
		autostart = true;
	}

	p = M_CheckParm("-playdemo");
	if (p && p < myargc - 1) {
		singledemo = true; // quit after one demo
		G_DeferedPlayDemo(myargv[p + 1]);
		D_DoomLoop(); // never returns
	}

	p = M_CheckParm("-timedemo");
	if (p && p < myargc - 1) {
		G_TimeDemo(myargv[p + 1]);
		D_DoomLoop(); // never returns
	}

	p = M_CheckParm("-loadgame");
	if (p && p < myargc - 1) {
		if (M_CheckParm("-cdrom"))
			sprintf(file, "c:\\doomdata\\" SAVEGAMENAME "%c.dsg", myargv[p + 1][0]);
		else
			sprintf(file, SAVEGAMENAME "%c.dsg", myargv[p + 1][0]);
		G_LoadGame(file);
	}


	if (gameaction != ga_loadgame) {
		if (autostart || netgame)
			G_InitNew(startskill, startepisode, startmap);
		else
			D_StartTitle(); // start up intro loop
	}

#ifdef __EMSCRIPTEN__
	SDL_Init(SDL_INIT_VIDEO);
	sdl_screen = SDL_SetVideoMode(SCREENWIDTH, SCREENHEIGHT, 8, SDL_SWSURFACE);
#endif

	D_DoomLoop(); // never returns
}
