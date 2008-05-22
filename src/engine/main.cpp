// main.cpp: initialisation & main loop

#include "pch.h"
#include "engine.h"

void cleanup()
{
	cleanupserver();
	SDL_ShowCursor(1);
	freeocta(worldroot);
	extern void clear_command();	clear_command();
	extern void clear_console();	clear_console();
	extern void clear_mdls();		clear_mdls();
	stopsound();
	SDL_Quit();
}

void quit()					 // normal exit
{
	extern void writeinitcfg();
	writeinitcfg();
	writeservercfg();
	abortconnect();
	disconnect(1);
	writecfg();
	cleanup();
	exit(EXIT_SUCCESS);
}

void fatal(const char *s, ...)    // failure exit
{
    static int errors = 0;
    errors++;

    if(errors <= 2) // print up to one extra recursive error
    {
        s_sprintfdlv(msg,s,s);
        puts(msg);

        if(errors <= 1) // avoid recursion
        {
            SDL_ShowCursor(1);
            #ifdef WIN32
                MessageBox(NULL, msg, "Blood Frontier: Error", MB_OK|MB_SYSTEMMODAL);
            #endif
            SDL_Quit();
        }
    }

    exit(EXIT_FAILURE);
}

SDL_Surface *screen = NULL;

static int initing = NOT_INITING;
static bool restoredinits = false;

bool initwarning(const char *desc, int level, int type)
{
    if(initing < level)
    {
        addchange(desc, type);
        return true;
    }
    return false;
}

VARF(scr_w, 320, 1024, 10000, initwarning("screen resolution"));
VARF(scr_h, 200, 768, 10000, initwarning("screen resolution"));
VARF(colorbits, 0, 0, 32, initwarning("color depth"));
VARF(depthbits, 0, 0, 32, initwarning("depth-buffer precision"));
VARF(stencilbits, 0, 1, 32, initwarning("stencil-buffer precision"));
VARF(fsaa, -1, -1, 16, initwarning("anti-aliasing"));
VARF(vsync, -1, -1, 1, initwarning("vertical sync"));

void writeinitcfg()
{
    if(!restoredinits) return;
	FILE *f = openfile("init.cfg", "w");
	if(!f) return;
	fprintf(f, "// automatically written on exit, DO NOT MODIFY\n// modify settings in game\n");
    extern int fullscreen;
    fprintf(f, "fullscreen %d\n", fullscreen);
	fprintf(f, "scr_w %d\n", scr_w);
	fprintf(f, "scr_h %d\n", scr_h);
	fprintf(f, "colorbits %d\n", colorbits);
	fprintf(f, "depthbits %d\n", depthbits);
	fprintf(f, "stencilbits %d\n", stencilbits);
	fprintf(f, "fsaa %d\n", fsaa);
	fprintf(f, "vsync %d\n", vsync);
	fprintf(f, "shaders %d\n", useshaders);
	fprintf(f, "shaderprecision %d\n", shaderprecision);
	fprintf(f, "soundmono %d\n", soundmono);
	fprintf(f, "soundchans %d\n", soundchans);
	fprintf(f, "soundbufferlen %d\n", soundbufferlen);
	fprintf(f, "soundfreq %d\n", soundfreq);
	fclose(f);
}

void screenshot(char *filename)
{
	SDL_Surface *image = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 24, 0x0000FF, 0x00FF00, 0xFF0000, 0);
	if(!image) return;
	uchar *tmp = new uchar[screen->w*screen->h*3];
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, tmp);
	uchar *dst = (uchar *)image->pixels;
    loopi(screen->h)
	{
		memcpy(dst, &tmp[3*screen->w*(screen->h-i-1)], 3*screen->w);
		endianswap(dst, 3, screen->w);
		dst += image->pitch;
	}
	delete[] tmp;
	const char *name = makefile(*filename ? filename : getmapname(), ".bmp", true, true);
	const char *fname = findfile(name, "wb");
	SDL_SaveBMP(image, fname);
	SDL_FreeSurface(image);
}

COMMAND(screenshot, "s");
COMMAND(quit, "");

void setfullscreen(bool enable)
{
    if(!screen) return;
#if defined(WIN32) || defined(__APPLE__)
    initwarning(enable ? "fullscreen" : "windowed");
#else
    if(enable == !(screen->flags&SDL_FULLSCREEN)) SDL_WM_ToggleFullScreen(screen);
#endif
}

VARF(fullscreen, 0, 0, 1, setfullscreen(fullscreen!=0));

void screenres(int *w, int *h)
{
#if !defined(WIN32) && !defined(__APPLE__)
    if(initing >= INIT_RESET)
	{
#endif
		scr_w = *w;
		scr_h = *h;
#if defined(WIN32) || defined(__APPLE__)
		initwarning("screen resolution");
#else
		return;
	}
	SDL_Surface *surf = SDL_SetVideoMode(*w, *h, 0, SDL_OPENGL|SDL_RESIZABLE|(screen->flags&SDL_FULLSCREEN));
	if(!surf) return;
	screen = surf;
	scr_w = screen->w;
	scr_h = screen->h;
	glViewport(0, 0, scr_w, scr_h);
#endif
}

COMMAND(screenres, "ii");

VARFP(gamma, 30, 100, 300,
{
	float f = gamma/100.0f;
	if(SDL_SetGamma(f,f,f)==-1)
	{
		conoutf("Could not set gamma (card/driver doesn't support it?)");
		conoutf("sdl: %s", SDL_GetError());
	}
});

void setupscreen(int &usedcolorbits, int &useddepthbits, int &usedfsaa)
{
    int flags = SDL_RESIZABLE;
    #if defined(WIN32) || defined(__APPLE__)
    flags = 0;
    #endif
    if(fullscreen) flags |= SDL_FULLSCREEN;
    SDL_Rect **modes = SDL_ListModes(NULL, SDL_OPENGL|flags);
    if(modes && modes!=(SDL_Rect **)-1)
    {
        bool hasmode = false;
        for(int i = 0; modes[i]; i++)
        {
            if(scr_w <= modes[i]->w && scr_h <= modes[i]->h) { hasmode = true; break; }
        }
        if(!hasmode) { scr_w = modes[0]->w; scr_h = modes[0]->h; }
    }
    bool hasbpp = true;
    if(colorbits && modes)
        hasbpp = SDL_VideoModeOK(modes!=(SDL_Rect **)-1 ? modes[0]->w : scr_w, modes!=(SDL_Rect **)-1 ? modes[0]->h : scr_h, colorbits, SDL_OPENGL|flags)==colorbits;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#if SDL_VERSION_ATLEAST(1, 2, 11)
    if(vsync>=0) SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, vsync);
#endif
    static int configs[] =
    {
        0x7, /* try everything */
        0x6, 0x5, 0x3, /* try disabling one at a time */
        0x4, 0x2, 0x1, /* try disabling two at a time */
        0 /* try disabling everything */
    };
    int config = 0;
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    if(!depthbits) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    if(!fsaa)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
    }
    loopi(sizeof(configs)/sizeof(configs[0]))
    {
        config = configs[i];
        if(!depthbits && config&1) continue;
        if(!stencilbits && config&2) continue;
        if(fsaa<=0 && config&4) continue;
        if(depthbits) SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, config&1 ? depthbits : 16);
        if(stencilbits)
        {
            SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, config&2 ? 1 : 0);
            hasstencil = (config&2)!=0;
        }
        if(fsaa>0)
        {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, config&4 ? 1 : 0);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, config&4 ? fsaa : 0);
        }
        screen = SDL_SetVideoMode(scr_w, scr_h, hasbpp ? colorbits : 0, SDL_OPENGL|flags);
        if(screen) break;
    }
    if(!screen) fatal("Unable to create OpenGL screen: %s", SDL_GetError());
    else
    {
        if(!hasbpp) conoutf("%d bit color buffer not supported - disabling", colorbits);
        if(depthbits && (config&1)==0) conoutf("%d bit z-buffer not supported - disabling", depthbits);
        if(stencilbits && (config&2)==0) conoutf("Stencil buffer not supported - disabling");
        if(fsaa>0 && (config&4)==0) conoutf("%dx anti-aliasing not supported - disabling", fsaa);
    }

    scr_w = screen->w;
    scr_h = screen->h;

    usedcolorbits = hasbpp ? colorbits : 0;
    useddepthbits = config&1 ? depthbits : 0;
    usedfsaa = config&4 ? fsaa : 0;
}

void resetgl()
{
    clearchanges(CHANGE_GFX);

    computescreen("resetting OpenGL");

    extern void cleanupva();
    extern void cleanupparticles();
    extern void cleanupmodels();
    extern void cleanuptextures();
    extern void cleanuplightmaps();
    extern void cleanshadowmap();
    extern void cleanreflections();
    extern void cleanupglare();
    extern void cleanupdepthfx();
    extern void cleanupshaders();
    extern void cleanupgl();
    cleanupva();
    cleanupparticles();
    cleanupmodels();
    cleanuptextures();
    cleanuplightmaps();
    cleanshadowmap();
    cleanreflections();
    cleanupglare();
    cleanupdepthfx();
    cleanupshaders();
    cleanupgl();

    SDL_SetVideoMode(0, 0, 0, 0);

    int usedcolorbits = 0, useddepthbits = 0, usedfsaa = 0;
    setupscreen(usedcolorbits, useddepthbits, usedfsaa);
    gl_init(scr_w, scr_h, usedcolorbits, useddepthbits, usedfsaa);

    extern void reloadfonts();
    extern void reloadtextures();
    extern void reloadshaders();
    inbetweenframes = false;
    if(!reloadtexture("textures/notexture") ||
       !reloadtexture("textures/logo") ||
       !reloadtexture("textures/loadback"))
        fatal("failed to reload core texture");
    reloadfonts();
    inbetweenframes = true;
    computescreen("initializing...");
    reloadshaders();
    reloadtextures();
    initlights();
    allchanged(true);
}

COMMAND(resetgl, "");

void keyrepeat(bool on)
{
	SDL_EnableKeyRepeat(on ? SDL_DEFAULT_REPEAT_DELAY : 0,
							 SDL_DEFAULT_REPEAT_INTERVAL);
}

int ignoremouse = 5;
bool activewindow = true, warpmouse = false;

vector<SDL_Event> events;

void pushevent(const SDL_Event &e)
{
    events.add(e);
}

bool interceptkey(int sym)
{
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
        case SDL_KEYDOWN:
            if(event.key.keysym.sym == SDLK_ESCAPE)
                return true;

        default:
            pushevent(event);
            break;
        }
    }
    return false;
}

void checkinput()
{
	SDL_Event event;
	int lasttype = 0, lastbut = 0;
    while(events.length() || SDL_PollEvent(&event))
    {
        if(events.length()) event = events.remove(0);

		switch (event.type)
		{
			case SDL_QUIT:
				quit();
				break;

#if !defined(WIN32) && !defined(__APPLE__)
			case SDL_VIDEORESIZE:
				screenres(&event.resize.w, &event.resize.h);
				break;
#endif

			case SDL_KEYDOWN:
			case SDL_KEYUP:
				keypress(event.key.keysym.sym, event.key.state==SDL_PRESSED, event.key.keysym.unicode);
				break;

			case SDL_ACTIVEEVENT:
			{
				if(event.active.state & SDL_APPINPUTFOCUS)
				{
					setvar("grabinput", event.active.gain ? 1 : 0, true);
					activewindow = event.active.gain ? true : false;
				}
				break;
			}
			case SDL_MOUSEMOTION:
			{
				if(ignoremouse) { ignoremouse--; break; }
				if(grabinput || activewindow)
				{
#ifdef __APPLE__
					if(event.motion.y == 0) break;  //let mac users drag windows via the title bar
#endif
					if(warpmouse && event.motion.x == screen->w/2 && event.motion.y == screen->h/2)
					{
						warpmouse = false;
						break;
					}

					if(cl->mousemove(event.motion.xrel, event.motion.yrel, event.motion.x, event.motion.y, screen->w, screen->h))
					{
						if(grabinput)
						{
							SDL_WarpMouse(screen->w/2, screen->h/2);
							warpmouse = true;
						}
					}
				}
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				if (lasttype==event.type && lastbut==event.button.button) break; // why?? get event twice without it
				keypress(-event.button.button, event.button.state!=0, 0);
				lasttype = event.type;
				lastbut = event.button.button;
				break;
			}
		}
	}
}

VARF(gamespeed, 10, 100, 1000, if(multiplayer()) gamespeed = 100);
VARF(paused, 0, 0, 1, if(multiplayer()) paused = 0);

VARP(maxfps, 0, 200, 500);

void limitfps(int &millis, int curmillis)
{
    if(!maxfps) return;
	static int fpserror = 0;
	int delay = 1000/maxfps - (millis-curmillis);
	if(delay < 0) fpserror = 0;
	else
	{
		fpserror += 1000%maxfps;
		if(fpserror >= maxfps)
		{
			++delay;
			fpserror -= maxfps;
		}
		if(delay > 0)
		{
			SDL_Delay(delay);
			millis += delay;
		}
	}
}

#if defined(WIN32) && !defined(_DEBUG) && !defined(__GNUC__)
void stackdumper(unsigned int type, EXCEPTION_POINTERS *ep)
{
	if(!ep) fatal("unknown type");
	EXCEPTION_RECORD *er = ep->ExceptionRecord;
	CONTEXT *context = ep->ContextRecord;
	string out, t;
	s_sprintf(out)("Blood Frontier Win32 Exception: 0x%x [0x%x]\n\n", er->ExceptionCode, er->ExceptionCode==EXCEPTION_ACCESS_VIOLATION ? er->ExceptionInformation[1] : -1);
	STACKFRAME sf = {{context->Eip, 0, AddrModeFlat}, {}, {context->Ebp, 0, AddrModeFlat}, {context->Esp, 0, AddrModeFlat}, 0};
	SymInitialize(GetCurrentProcess(), NULL, TRUE);

	while(::StackWalk(IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentThread(), &sf, context, NULL, ::SymFunctionTableAccess, ::SymGetModuleBase, NULL))
	{
		struct { IMAGEHLP_SYMBOL sym; string n; } si = { { sizeof( IMAGEHLP_SYMBOL ), 0, 0, 0, sizeof(string) } };
		IMAGEHLP_LINE li = { sizeof( IMAGEHLP_LINE ) };
		DWORD off;
		if(SymGetSymFromAddr(GetCurrentProcess(), (DWORD)sf.AddrPC.Offset, &off, &si.sym) && SymGetLineFromAddr(GetCurrentProcess(), (DWORD)sf.AddrPC.Offset, &off, &li))
		{
			char *del = strrchr(li.FileName, '\\');
			s_sprintf(t)("%s - %s [%d]\n", si.sym.Name, del ? del + 1 : li.FileName, li.LineNumber);
			s_strcat(out, t);
		}
	}
	fatal(out);
}
#endif

#define MAXFPSHISTORY 60

int fpspos = 0, fpshistory[MAXFPSHISTORY];

void resetfpshistory()
{
	loopi(MAXFPSHISTORY) fpshistory[i] = 1;
	fpspos = 0;
}

void updatefpshistory(int millis)
{
	fpshistory[fpspos++] = max(1, min(1000, millis));
	if(fpspos>=MAXFPSHISTORY) fpspos = 0;
}

void getfps(int &fps, int &bestdiff, int &worstdiff)
{
	int total = fpshistory[MAXFPSHISTORY-1], best = total, worst = total;
	loopi(MAXFPSHISTORY-1)
	{
		int millis = fpshistory[i];
		total += millis;
		if(millis < best) best = millis;
		if(millis > worst) worst = millis;
	}

	fps = (1000*MAXFPSHISTORY)/total;
	bestdiff = 1000/best-fps;
	worstdiff = fps-1000/worst;
}

void getfps_()
{
    int fps, bestdiff, worstdiff;
    getfps(fps, bestdiff, worstdiff);
    intret(fps);
}

COMMANDN(getfps, getfps_, "");

bool inbetweenframes = false;

static bool findarg(int argc, char **argv, const char *str)
{
	for(int i = 1; i<argc; i++) if(strstr(argv[i], str)==argv[i]) return true;
	return false;
}

static int clockrealbase = 0, clockvirtbase = 0;
static void clockreset() { clockrealbase = SDL_GetTicks(); clockvirtbase = totalmillis; }
VARFP(clockerror, 990000, 1000000, 1010000, clockreset());
VARFP(clockfix, 0, 0, 1, clockreset());

VAR(version, 1, ENG_VERSION, -1); // for scripts
VARP(verbose, 0, 0, 4); // be more or less expressive to console

void _grabinput(int n)
{
	SDL_WM_GrabInput(n ? SDL_GRAB_ON : SDL_GRAB_OFF);
	keyrepeat(n ? false : true);
	SDL_ShowCursor(n ? 0 : 1);
}
VARF(grabinput, 0, 1, 1, _grabinput(grabinput););

VAR(curfps, 1, 0, -1);
VAR(bestfps, 1, 0, -1);
VAR(worstfps, 1, 0, -1);

#if 0
int lastperf = 0;

VARP(perfauto, 0, 1, 1);		// auto performance adjust
VARP(perfmin, 0, 0, 100);		// lowest perf level to go to
VARP(perfmax, 0, 100, 100);		// highest perf level to go to
VARFP(perffps, 0, 25, 100,		// aim for this fps or higher
	perffps = min(perffps, maxfps-1));
VARP(perfrate, 0, 500, 10000);	// only adjust this often
VARFP(perflimit, 0, 10, 100,	// going below this automatically scales to minimum
	perflimit = min(perflimit, perffps-1));

void perfset(int level)
{
	if (level >= 0 && level <= 100)
	{
		#define perfvarscl(s,v,n0,n1) \
			{ \
				int _##s = int((float(n0-n1)/float(v))*float(level))+n1; \
				setvar(#s, clamp(_##s, n1, n0), true); \
			} \

		perfvarscl(decalfade,			100,	60000,		1);
		perfvarscl(dynlightdist,		100,	10000,		128);
		perfvarscl(emitfps,				100,	60,			1);
		perfvarscl(grassanimdist,		100,	10000,		0);
		perfvarscl(grassdist,			100,	10000,		0);
		perfvarscl(grassfalloff,		100,	1000,		0);
		perfvarscl(grasslod,			100,	1000,		0);
		perfvarscl(grasslodz,			100,	10000,		0);
		perfvarscl(grasstaper,			100,	200,		0);
		perfvarscl(maxparticledistance,	100,	1024,		256);
		perfvarscl(maxreflect,			100,	8,			1);
		perfvarscl(reflectdist,			100,	10000,		128);

		lastperf = lastmillis;
	}
}

VARFP(perflevel, 0, 100, 100, perfset(perflevel));

void perfcheck()
{
	extern void getfps(int &fps, int &bestdiff, int &worstdiff);
	int fps, bestdiff, worstdiff;
	getfps(fps, bestdiff, worstdiff);

	setvar("curfps", fps);
	setvar("bestfps", fps+bestdiff);
	setvar("worstfps", fps-worstdiff);

	if (perfauto && (lastmillis-lastperf > perfrate || fps-worstdiff < perflimit))
	{
		float amt = float(fps-worstdiff)/float(perffps);

		if (fps-worstdiff < perflimit && perflevel > perfmin) setvar("perflevel", perfmin, true);
		else if (amt < 1.f) setvar("perflevel", max(perflevel-int((1.f-amt)*10.f), perfmin), true);
		else if (amt > 1.f) setvar("perflevel", min(perflevel+int(amt), perfmax), true);
	}
}
#endif

void rehash(bool reload)
{
	if (reload)
	{
		writeservercfg();
		writecfg();
	}

	persistidents = false;

	if (!execfile("stdlib.cfg"))
		fatal("cannot find data files");
	if (!execfile("font.cfg"))
		fatal("cannot find font definitions");

	exec("defaults.cfg");
	execfile("servers.cfg");

	persistidents = true;

    initing = INIT_LOAD;
	execfile("config.cfg");
	execfile("autoexec.cfg");
    initing = NOT_INITING;
}
ICOMMAND(rehash, "i", (int *nosave), rehash(*nosave ? false : true));

void setcaption(const char *text)
{
	s_sprintfd(caption)("%s [v%.2f] %s%s%s", ENG_NAME, float(ENG_VERSION)/100.f, ENG_RELEASE, text ? ": " : "", text ? text : "");
	SDL_WM_SetCaption(caption, NULL);
}

int frames = 0, ignore = 5;

int main(int argc, char **argv)
{
	#ifdef WIN32
	//atexit((void (__cdecl *)(void))_CrtDumpMemoryLeaks);
	#ifndef _DEBUG
	#ifndef __GNUC__
	__try {
	#endif
	#endif
	#endif

	char *initscript = NULL;
	colorpos = 0; // last but not least.
	colorstack[0] = 'c'; //indicate user color

	initing = INIT_RESET;
	addpackagedir("data");
	for(int i = 1; i<argc; i++)
	{
		if(argv[i][0]=='-') switch(argv[i][1])
		{
            case 'q': printf("Using home directory: %s\n", &argv[i][2]); sethomedir(&argv[i][2]); break;
            case 'k': printf("Adding package directory: %s\n", &argv[i][2]); addpackagedir(&argv[i][2]); break;
			case 'r': execfile(argv[i][2] ? &argv[i][2] : "init.cfg"); restoredinits = true; break;
            case 'w': scr_w = atoi(&argv[i][2]); if(scr_w<320) scr_w = 320; if(!findarg(argc, argv, "-h")) scr_h = (scr_w*3)/4; break;
            case 'h': scr_h = atoi(&argv[i][2]); if(scr_h<200) scr_h = 200; if(!findarg(argc, argv, "-w")) scr_w = (scr_h*4)/3; break;
			case 'z': depthbits = atoi(&argv[i][2]); break;
			case 'b': colorbits = atoi(&argv[i][2]); break;
			case 'a': fsaa = atoi(&argv[i][2]); break;
			case 'v': vsync = atoi(&argv[i][2]); break;
            case 't': fullscreen = atoi(&argv[i][2]); break;
			case 'e': stencilbits = atoi(&argv[i][2]); break;
			case 'f':
			{
				extern int useshaders, shaderprecision;
				int n = atoi(&argv[i][2]);
				useshaders = n ? 1 : 0;
				shaderprecision = min(max(n - 1, 0), 3);
				break;
			}
			case 'x': initscript = &argv[i][2]; break;
			default: if(!serveroption(argv[i])) gameargs.add(argv[i]); break;
		}
		else gameargs.add(argv[i]);
	}
	initing = NOT_INITING;

	conoutf("init: sdl");

    int par = 0;
	#ifdef _DEBUG
	par = SDL_INIT_NOPARACHUTE;
	#ifdef WIN32
	SetEnvironmentVariable("SDL_DEBUG", "1");
	#endif
	#endif

	//#ifdef WIN32
	//SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	//#endif

	par |= SDL_INIT_TIMER|SDL_INIT_VIDEO|SDL_INIT_JOYSTICK;
	if (SDL_Init(par) < 0) fatal("Unable to initialize SDL: %s", SDL_GetError());

	conoutf("init: video: mode");
    int usedcolorbits = 0, useddepthbits = 0, usedfsaa = 0;
    setupscreen(usedcolorbits, useddepthbits, usedfsaa);

	conoutf("init: video: misc");
	setcaption("loading..");
	setvar("grabinput", 1, true);

	conoutf("init: gl");
    gl_checkextensions();
    gl_init(scr_w, scr_h, usedcolorbits, useddepthbits, usedfsaa);
    notexture = textureload("textures/notexture");
    blanktexture = textureload("textures/blank");
    if(!notexture) fatal("could not find core textures");

	conoutf("init: sound");
	initsound();

	conoutf("init: enet");
	if(enet_initialize()<0) fatal("Unable to initialise network module");

	conoutf("init: runtime");
	initruntime();
	camera1 = cl->iterdynents(0);
	emptymap(0, true, NULL, true);

	conoutf("init: config");
	rehash(false);
	if(!setfont("default")) fatal("no default font specified");

	computescreen("loading...");
	inbetweenframes = true;

    conoutf("init: gl effects");
    loadshaders();
	particleinit();
    initdecals();

	conoutf("init: client");
	cl->initclient();

	conoutf("init: mainloop");
	if (initscript) execute(initscript);

	resetfpshistory();
	for(;;)
	{
		int millis = SDL_GetTicks() - clockrealbase;
		if (clockfix) millis = int(millis*(double(clockerror)/1000000));
        millis += clockvirtbase;
        if(millis<totalmillis) millis = totalmillis;
        limitfps(millis, totalmillis);
		int elapsed = millis-totalmillis;

		if (paused) curtime = 0;
		else curtime = (elapsed*gamespeed)/100;

		checkinput();

		if (frames && lastmillis) cl->updateworld();

		menuprocess();
		lastmillis += curtime;
		totalmillis = millis;

		checksleep(lastmillis);

		serverslice(0);

		if (frames)
		{
			updatefpshistory(elapsed);
			//perfcheck();

			if (cc->ready())
			{
                cl->recomputecamera(screen->w, screen->h);
                setviewcell(camera1->o);
				entity_particles();
				checksound();

				inbetweenframes = false;
				if(frames>2) gl_drawframe(screen->w, screen->h);
				SDL_GL_SwapBuffers();
				inbetweenframes = true;

				s_sprintfd(cap)("%s - %s", cl->gametitle(), cl->gametext());
				setcaption(cap);
			}
		}
		frames++;
		colorpos = 0; // last but not least.
		colorstack[0] = 'c'; //indicate user color
	}

	ASSERT(0);
	return EXIT_FAILURE;

#if defined(WIN32) && !defined(_DEBUG) && !defined(__GNUC__)
	} __except(stackdumper(0, GetExceptionInformation()), EXCEPTION_CONTINUE_SEARCH) { return 0; }
#endif
}
