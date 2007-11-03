
// network quantization scale
#define DMF 16.0f			// for world locations
#define DNF 100.0f			// for normalized vectors
#define DVELF 1.0f			// for playerspeed based velocity vectors

enum						// static entity types
{
	NOTUSED = ET_EMPTY,		// entity slot not in use in map
	LIGHT = ET_LIGHT,		// lightsource, attr1 = radius, attr2 = intensity
	MAPMODEL = ET_MAPMODEL,	// attr1 = angle, attr2 = idx
	PLAYERSTART,			// attr1 = angle
	ENVMAP = ET_ENVMAP,		// attr1 = radius
	PARTICLES = ET_PARTICLES,
	MAPSOUND = ET_SOUND,
	SPOTLIGHT = ET_SPOTLIGHT,
	WEAPON,
	TELEPORT,				// attr1 = idx
	TELEDEST,				// attr1 = angle, attr2 = idx
	MONSTER,				// attr1 = angle, attr2 = monstertype
	CARROT,					// attr1 = tag, attr2 = type
	JUMPPAD,				// attr1 = zpush, attr2 = ypush, attr3 = xpush
	BASE,
	RESPAWNPOINT,
	CAMERA,					// attr1 = yaw, attr2 = pitch, attr3 = pan (+:horiz/-:vert), attr4 = idx
	WAYPOINT,				// none?
	MAXENTTYPES
};

struct fpsentity : extentity
{
	vector<int> links;  // link list

	fpsentity()
	{
		links.setsize(0);
	}
	~fpsentity() {}
};

enum
{
	GUN_PISTOL = 0,
	GUN_SG,
	GUN_CG,
	GUN_GL,
	GUN_RL,
	GUN_RIFLE,
	NUMGUNS
};

enum { M_NONE = 0, M_SEARCH, M_HOME, M_ATTACKING, M_PAIN, M_SLEEP, M_AIMING };  // monster states

enum
{
	G_DEMO = 0,
	G_EDITMODE,
	G_SINGLEPLAYER,
	G_DEATHMATCH,
	G_CAPTURE,
	G_MAX
};

#define G_M_TEAM		0x0001
#define G_M_INSTA		0x0002
#define G_M_DUEL		0x0004

#define G_M_NUM			3

#define G_M_FRAG		G_M_TEAM|G_M_INSTA|G_M_DUEL

static struct gametypes
{
	int	type,			mutators;			char *name;
} gametype[] = {
	{ G_DEMO,			0,					"Demo Playback" },
	{ G_EDITMODE,		0,					"Coop Edit" },
	{ G_SINGLEPLAYER,	0,					"Singleplayer" },
	{ G_DEATHMATCH,		G_M_FRAG,			"Deathmatch" },
	{ G_CAPTURE,		G_M_FRAG,			"Capture" },
}, mutstype[] = {
	{ G_M_TEAM,			0,					"Team" },
	{ G_M_INSTA,		0,					"Instagib" },
	{ G_M_DUEL,			0,					"Duel" },
};

#define m_name(a) 			(a > -1 && a < G_MAX ? gametype[a].name : NULL)
#define m_mut(a) 			(a > -1 && a < G_M_NUM ? mutstype[a].name : NULL)

#define m_demo(a)			(a == G_DEMO)
#define m_edit(a)			(a == G_EDITMODE)
#define m_sp(a)				(a == G_SINGLEPLAYER)
#define m_dm(a)				(a == G_DEATHMATCH)
#define m_capture(a)		(a == G_CAPTURE)

#define m_mp(a)				(a > G_DEMO && a < G_MAX)
#define m_frag(a)			(m_dm(a) || m_capture(a))
#define m_timed(a)			(m_frag(a))

#define m_team(a,b)			((m_dm(a) && (b & G_M_TEAM)) || m_capture(a))
#define m_insta(a,b)		(m_frag(a) && (b & G_M_INSTA))
#define m_duel(a,b)			(m_frag(a) && (b & G_M_DUEL))

#define isteam(a,b)			(!strcmp(a,b))

// hardcoded sounds, defined in sounds.cfg
enum
{
	S_JUMP = 0, S_LAND, S_PAIN1, S_PAIN2, S_PAIN3, S_PAIN4, S_PAIN5, S_PAIN6, S_DIE1, S_DIE2,
	S_SPLASH1, S_SPLASH2, S_SPLAT, S_DEBRIS, S_WHIZZ, S_WHIRR, S_RUMBLE, S_TELEPORT, S_JUMPPAD,
	S_RELOAD, S_SWITCH, S_PISTOL, S_SG, S_CG,
	S_GLFIRE, S_GLEXPL, S_GLHIT, S_RLFIRE, S_RLEXPL, S_RLFLY, S_RIFLE,
	S_ITEMAMMO, S_ITEMSPAWN,
	S_V_BASECAP, S_V_BASELOST, S_V_FIGHT, S_V_RESPAWNPOINT,
	S_V_ONEMINUTE, S_V_YOUWIN, S_V_YOULOSE, S_V_FRAGGED, S_V_OWNED,
	S_V_SPREE1, S_V_SPREE2, S_V_SPREE3, S_V_SPREE4, S_REGEN,
	S_DAMAGE1, S_DAMAGE2, S_DAMAGE3, S_DAMAGE4, S_DAMAGE5, S_DAMAGE6, S_DAMAGE7, S_DAMAGE8,
	S_RESPAWN, S_CHAT, S_MENUPRESS, S_MENUBACK
};

// network messages codes, c2s, c2c, s2c
enum
{
	SV_INITS2C = 0, SV_INITC2S, SV_POS, SV_TEXT, SV_SOUND, SV_CDIS,
	SV_SHOOT, SV_EXPLODE, SV_SUICIDE,
	SV_DIED, SV_DAMAGE, SV_SHOTFX,
	SV_TRYSPAWN, SV_SPAWNSTATE, SV_SPAWN, SV_FORCEDEATH, SV_ARENAWIN,
	SV_GUNSELECT, SV_TAUNT,
	SV_MAPCHANGE, SV_MAPVOTE, SV_ITEMSPAWN, SV_ITEMPICKUP, SV_DENIED,
	SV_PING, SV_PONG, SV_CLIENTPING,
	SV_TIMEUP, SV_MAPRELOAD, SV_ITEMACC,
	SV_SERVMSG, SV_ITEMLIST, SV_RESUME,
    SV_EDITMODE, SV_EDITENT, SV_EDITF, SV_EDITT, SV_EDITM, SV_FLIP, SV_COPY, SV_PASTE, SV_ROTATE, SV_REPLACE, SV_DELCUBE, SV_REMIP, SV_NEWMAP, SV_GETMAP, SV_SENDMAP,
	SV_MASTERMODE, SV_KICK, SV_CLEARBANS, SV_CURRENTMASTER, SV_SPECTATOR, SV_SETMASTER, SV_SETTEAM, SV_APPROVEMASTER,
	SV_BASES, SV_BASEINFO, SV_TEAMSCORE, SV_FORCEINTERMISSION,
	SV_LISTDEMOS, SV_SENDDEMOLIST, SV_GETDEMO, SV_SENDDEMO,
	SV_DEMOPLAYBACK, SV_RECORDDEMO, SV_STOPDEMO, SV_CLEARDEMOS,
	SV_CLIENT,
	SV_COMMAND, SV_RELOAD, SV_REGEN,
};

static char msgsizelookup(int msg)
{
	static char msgsizesl[] =				// size inclusive message token, 0 for variable or not-checked sizes
	{
		SV_INITS2C, 4, SV_INITC2S, 0, SV_POS, 0, SV_TEXT, 0, SV_SOUND, 2, SV_CDIS, 2,
		SV_SHOOT, 0, SV_EXPLODE, 0, SV_SUICIDE, 1,
		SV_DIED, 4, SV_DAMAGE, 10, SV_SHOTFX, 9,
		SV_TRYSPAWN, 1, SV_SPAWNSTATE, 9, SV_SPAWN, 3, SV_FORCEDEATH, 2, SV_ARENAWIN, 2,
		SV_GUNSELECT, 2, SV_TAUNT, 1,
		SV_MAPCHANGE, 0, SV_MAPVOTE, 0, SV_ITEMSPAWN, 2, SV_ITEMPICKUP, 2, SV_DENIED, 2,
		SV_PING, 2, SV_PONG, 2, SV_CLIENTPING, 2,
		SV_TIMEUP, 2, SV_MAPRELOAD, 1, SV_ITEMACC, 3,
		SV_SERVMSG, 0, SV_ITEMLIST, 0, SV_RESUME, 0,
        SV_EDITMODE, 2, SV_EDITENT, 10, SV_EDITF, 16, SV_EDITT, 16, SV_EDITM, 15, SV_FLIP, 14, SV_COPY, 14, SV_PASTE, 14, SV_ROTATE, 15, SV_REPLACE, 16, SV_DELCUBE, 14, SV_REMIP, 1, SV_NEWMAP, 2, SV_GETMAP, 1, SV_SENDMAP, 0,
		SV_MASTERMODE, 2, SV_KICK, 2, SV_CLEARBANS, 1, SV_CURRENTMASTER, 3, SV_SPECTATOR, 3, SV_SETMASTER, 0, SV_SETTEAM, 0, SV_APPROVEMASTER, 2,
		SV_BASES, 0, SV_BASEINFO, 0, SV_TEAMSCORE, 0, SV_FORCEINTERMISSION, 1,
		SV_LISTDEMOS, 1, SV_SENDDEMOLIST, 0, SV_GETDEMO, 2, SV_SENDDEMO, 0,
		SV_DEMOPLAYBACK, 2, SV_RECORDDEMO, 2, SV_STOPDEMO, 1, SV_CLEARDEMOS, 2,
		SV_CLIENT, 0,
		SV_COMMAND, 0, SV_RELOAD, 0, SV_REGEN, 0,
		-1
	};
	for(char *p = msgsizesl; *p>=0; p += 2) if(*p==msg) return p[1];
	return -1;
}

#define BFRONTIER_SERVER_PORT		28795
#define BFRONTIER_SERVINFO_PORT		28796
#define PROTOCOL_VERSION			BFRONTIER
#define DEMO_VERSION 1				  // bump when demo format changes
#define DEMO_MAGIC "BFDZ"

struct demoheader
{
	char magic[16];
	int version, protocol;
};

#define MAXNAMELEN 15
#define MAXTEAMLEN 4

#define MAXFOV			(isthirdperson() ? 100 : 125)
#define MINFOV			(player1->gunselect == GUN_RIFLE ? 0 : 90)

#define TPDIST			16		// thirdperson distance
#define TPHEIGHT		8		// thidperson height

#define SGRAYS			20
#define SGSPREAD		3

#define RL_DAMRAD		30
#define RL_DISTSCALE	1.5f

#define MAXCARRY		2

static struct guninfo
{
	int info, 		sound, 		esound, 	fsound,		rsound,		add,	max,	adelay,	rdelay,	damage,	speed,	time,	kick,	wobble;	char *name;
} guns[NUMGUNS] =
{
	{ GUN_PISTOL,	S_PISTOL,	-1,			S_WHIRR,	-1,			12,		12,		250,	2000,	10,		0,		0,		-10 ,	10,		"pistol" },
	{ GUN_SG,		S_SG,		-1,			S_WHIRR,	-1,			1,		8,		1000,	500,	10,		0,		0,		-30,	30, 	"shotgun" },
	{ GUN_CG,		S_CG,		-1,			S_WHIRR,	-1,			50,		50,		75,		3000,	5,		0,		0,		-5,		5,		"chaingun" },
	{ GUN_GL,		S_GLFIRE,	S_GLEXPL,	S_WHIZZ,	S_GLHIT,	2,		4,		1500,	0,		100,	100,	3000,	-15,	15,		"grenades" },
	{ GUN_RL,		S_RLFIRE,	S_RLEXPL,	S_RLFLY,	-1,			1,		1,		2500,	5000,	100,	200,	10000,	-40,	40,		"rockets" },
	{ GUN_RIFLE,	S_RIFLE,	-1,			S_WHIRR,	-1,			1,		5,		1500,	1000,	50,		500,	0,		-30,	20,		"rifle" },
};
#define isgun(gun) (gun > -1 && gun < NUMGUNS)

enum { TEAM_BLUE = 0, TEAM_RED, TEAM_MAX };
static char *teamnames[TEAM_MAX] = {"blue", "red"};

// inherited by fpsent and server clients
struct fpsstate
{
	int health, lastspawn, lastpain, lastregen;
	int gunselect, gunwait[NUMGUNS], gunlast[NUMGUNS];
	int ammo[NUMGUNS];

	fpsstate() {}
	~fpsstate() {}

	bool canweapon(int gun, int millis)
	{
		return isgun(gun) && (gunselect != gun)	&& (millis-gunlast[gun] >= gunwait[gun]);
	}

	bool canshoot(int gun, int millis)
	{
		return isgun(gun) && (ammo[gun] > 0) && (millis-gunlast[gun] >= gunwait[gun]);
	}

	bool canreload(int gun, int millis)
	{
		return isgun(gun) && (ammo[gun] < guns[gun].max) && (guns[gun].rdelay > 0) && (millis-gunlast[gun] >= gunwait[gun]);
	}

	bool canammo(int gun, int millis)
	{
		return isgun(gun) && (ammo[gun] < guns[gun].max) && (millis-gunlast[gun] >= gunwait[gun]);
	}

	bool canpickup(int type, int attr1, int attr2, int millis)
	{
		switch (type)
		{
			case WEAPON:
			{
				return canammo(attr1, millis);
				break; // difference is here, can't pickup when reloading or firing
			}
			default:
			{
				return false;
				break;
			}
		}
	}

	void pickup(int type, int attr1, int attr2)
	{
		switch (type)
		{
			case WEAPON:
			{
				guninfo &g = guns[attr1];
				ammo[g.info] = min(ammo[g.info] + (attr2 > 0 ? attr2 : g.add), g.max);
				break;
			}
			default:
			{
				break;
			}
		}
	}

	void respawn()
	{
		health = 100;
		lastspawn = lastpain = lastregen = -1;
		loopi(NUMGUNS)
		{
			gunwait[i] = gunlast[i] = 0;
		}
		gunselect = GUN_PISTOL;
		loopi(NUMGUNS) ammo[i] = 0;
	}

	void spawnstate(int gamemode, int mutators)
	{
		if(m_insta(gamemode, mutators))
		{
			health = 1;
			gunselect = GUN_RIFLE;
			loopi(NUMGUNS)
			{
				ammo[i] = i == GUN_RIFLE ? guns[i].add : 0;
			}
		}
		else
		{
			health = 100;
			loopi(NUMGUNS)
			{
				ammo[i] = i < MAXCARRY ? guns[i].add : 0;
			}
		}
	}

	// just subtract damage here, can set death, etc. later in code calling this
	int dodamage(int damage, int millis)
	{
		lastpain = lastregen = millis;
		health -= damage;
		return damage;
	}
};

struct fpsent : dynent, fpsstate
{
	int weight;						 // affects the effectiveness of hitpush
	int clientnum, privilege, lastupdate, plag, ping;
	int lifesequence;					// sequence id for each respawn, used in damage test
	int lastattackgun;
	bool attacking;
	int lasttaunt;
	int lastpickup, lastpickupmillis;
	int superdamage;
	int frags, deaths, totaldamage, totalshots;
	editinfo *edit;
	int spree, lastimpulse;

	string name, team, info;

	fpsent() : weight(100), clientnum(-1), privilege(PRIV_NONE), lastupdate(0), plag(0), ping(0), lifesequence(0), frags(0), deaths(0), totaldamage(0), totalshots(0), edit(NULL), spree(0), lastimpulse(0)
	{
		name[0] = team[0] = info[0] = 0;
		respawn();
	}
	~fpsent() { freeeditinfo(edit); }

	void damageroll(float damage)
	{
		float damroll = 2.0f*damage;
		roll += roll>0 ? damroll : (roll<0 ? -damroll : (rnd(2) ? damroll : -damroll)); // give player a kick
	}

	void hitpush(int damage, const vec &dir, fpsent *actor, int gun)
	{
		vec push(dir);
		push.mul(damage/weight);
		vel.add(push);
	}

	void respawn()
	{
		dynent::reset();
		fpsstate::respawn();
		lastattackgun = gunselect;
		attacking = false;
		lasttaunt = 0;
		lastpickup = -1;
		lastpickupmillis = 0;
		superdamage = 0;
		spree = lastimpulse = 0;
	}
};

enum
{
	SINFO_ICON = 0,
	SINFO_STATUS,
	SINFO_HOST,
	SINFO_DESC,
	SINFO_PING,
	SINFO_PLAYERS,
	SINFO_MAXCLIENTS,
	SINFO_MODE,
	SINFO_MAP,
	SINFO_TIME,
	SINFO_MAX
};

static char *serverinfotypes[] = {
	"",
	"status",
	"host",
	"description",
	"ping",
	"pl",
	"max",
	"game mode",
	"map name",
	"time left"
};

enum
{
	SSTAT_OPEN = 0,
	SSTAT_LOCKED,
	SSTAT_PRIVATE,
	SSTAT_FULL,
	SSTAT_UNKNOWN,
	SSTAT_MAX
};
static char *serverstatustypes[] = {
	"\fs\fgopen\fS",
	"\fs\fblocked\fS",
	"\fs\fmprivate\fS",
	"\fs\frfull\fS",
	"\fs\fb?\fS"
};

#ifndef STANDALONE
#define FPSMODHUDMAX		((h*3/FONTH)*FONTH)-(FONTH*5)		// max hud length
#define ILLUMINATE			48.f
#define ENTPART				4.f

enum {
	HD_OLD = 0,
	HD_LEFT,
	HD_RIGHT,
	HD_MAX
};

#endif
#if 0
static char *msgnames[] = {
	"SV_INITS2C",
	"SV_INITC2S",
	"SV_POS",
	"SV_TEXT",
	"SV_SOUND",
	"SV_CDIS",
	"SV_SHOOT",
	"SV_EXPLODE",
	"SV_SUICIDE",
	"SV_DIED",
	"SV_DAMAGE",
	"SV_SHOTFX",
	"SV_TRYSPAWN",
	"SV_SPAWNSTATE",
	"SV_SPAWN",
	"SV_FORCEDEATH",
	"SV_ARENAWIN",
	"SV_GUNSELECT",
	"SV_TAUNT",
	"SV_MAPCHANGE",
	"SV_MAPVOTE",
	"SV_ITEMSPAWN",
	"SV_ITEMPICKUP",
	"SV_DENIED",
	"SV_PING",
	"SV_PONG",
	"SV_CLIENTPING",
	"SV_TIMEUP",
	"SV_MAPRELOAD",
	"SV_ITEMACC",
	"SV_SERVMSG",
	"SV_ITEMLIST",
	"SV_RESUME",
	"SV_EDITMODE",
	"SV_EDITENT",
	"SV_EDITF",
	"SV_EDITT",
	"SV_EDITM",
	"SV_FLIP",
	"SV_COPY",
	"SV_PASTE",
	"SV_ROTATE",
	"SV_REPLACE",
	"SV_DELCUBE",
	"SV_REMIP",
	"SV_NEWMAP",
	"SV_GETMAP",
	"SV_SENDMAP",
	"SV_MASTERMODE",
	"SV_KICK",
	"SV_CLEARBANS",
	"SV_CURRENTMASTER",
	"SV_SPECTATOR",
	"SV_SETMASTER",
	"SV_SETTEAM",
	"SV_BASES",
	"SV_BASEINFO",
	"SV_TEAMSCORE",
	"SV_FORCEINTERMISSION",
	"SV_LISTDEMOS",
	"SV_SENDDEMOLIST",
	"SV_GETDEMO",
	"SV_SENDDEMO",
	"SV_DEMOPLAYBACK",
	"SV_RECORDDEMO",
	"SV_STOPDEMO",
	"SV_CLEARDEMOS",
	"SV_CLIENT",
	"SV_COMMAND",
	"SV_RELOAD",
	"SV_REGEN",
};
#endif
