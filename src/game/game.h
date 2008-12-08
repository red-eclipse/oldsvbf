#define GAMEID				"bfa"
#define GAMEVERSION			126
#define DEMO_VERSION		GAMEVERSION

// network quantization scale
#define DMF 16.0f			// for world locations
#define DNF 100.0f			// for normalized vectors
#define DVELF 1.0f			// for playerspeed based velocity vectors

enum
{
	S_JUMP = S_GAMESPECIFIC, S_LAND, S_PAIN1, S_PAIN2, S_PAIN3, S_PAIN4, S_PAIN5, S_PAIN6, S_DIE1, S_DIE2,
	S_SPLASH1, S_SPLASH2, S_UNDERWATER,
	S_SPLAT, S_DEBRIS, S_TINK, S_RICOCHET, S_WHIZZ, S_WHIRR, S_EXPLODE, S_ENERGY, S_HUM, S_BURN, S_BURNING,
	S_RELOAD, S_SWITCH, S_PLASMA, S_SG, S_CG,
	S_GLFIRE, S_FLFIRE, S_CARBINE, S_RIFLE,
	S_ITEMPICKUP, S_ITEMSPAWN, 	S_REGEN,
	S_DAMAGE1, S_DAMAGE2, S_DAMAGE3, S_DAMAGE4, S_DAMAGE5, S_DAMAGE6, S_DAMAGE7, S_DAMAGE8,
	S_RESPAWN, S_CHAT, S_DENIED,
	S_V_FLAGSECURED, S_V_FLAGOVERTHROWN,
    S_V_FLAGPICKUP, S_V_FLAGDROP, S_V_FLAGRETURN, S_V_FLAGSCORE, S_V_FLAGRESET,
	S_V_FIGHT, S_V_CHECKPOINT, S_V_ONEMINUTE, S_V_HEADSHOT,
	S_V_SPREE1, S_V_SPREE2, S_V_SPREE3, S_V_SPREE4,
	S_V_YOUWIN, S_V_YOULOSE, S_V_MCOMPLETE, S_V_FRAGGED, S_V_OWNED,
	S_MAX
};


enum								// entity types
{
	NOTUSED = ET_EMPTY,				// 0  entity slot not in use in map
	LIGHT = ET_LIGHT,				// 1  radius, intensity or red, green, blue
	MAPMODEL = ET_MAPMODEL,			// 2  idx, yaw, pitch, roll, flags
	PLAYERSTART = ET_PLAYERSTART,	// 3  angle, team, id
	ENVMAP = ET_ENVMAP,				// 4  radius
	PARTICLES = ET_PARTICLES,		// 5  type, [others]
	MAPSOUND = ET_SOUND,			// 6  idx, maxrad, minrad, volume, flags
	SPOTLIGHT = ET_SPOTLIGHT,		// 7  radius
	WEAPON = ET_GAMESPECIFIC,		// 8  gun, flags
	TELEPORT,						// 9  yaw, pitch, push, [radius] [portal]
	RESERVED,						// 10
	TRIGGER,						// 11 idx, type, acttype, [radius]
	PUSHER,							// 12 zpush, ypush, xpush, [radius]
	FLAG,							// 13 idx, team
	CHECKPOINT,						// 14 idx
	CAMERA,							// 15 idx mindist maxdist
	WAYPOINT,						// 16 cmd
	ANNOUNCER,						// 17 maxrad, minrad, volume
	CONNECTION,						// 18
	MAXENTTYPES						// 19
};

enum { EU_NONE = 0, EU_ITEM, EU_AUTO, EU_ACT, EU_MAX };
enum { TR_NONE = 0, TR_LINK, TR_SCRIPT, TR_MAX };
enum { TA_NONE = 0, TA_AUTO, TA_ACT, TA_MAX };

struct enttypes
{
	int	type, 		links,	radius,	usetype; bool noisy;	const char *name;
};
#ifdef GAMESERVER
enttypes enttype[] = {
	{ NOTUSED,		0,		0,		EU_NONE,	true,		"none" },
	{ LIGHT,		59,		0,		EU_NONE,	false,		"light" },
	{ MAPMODEL,		58,		0,		EU_NONE,	false,		"mapmodel" },
	{ PLAYERSTART,	59,		0,		EU_NONE,	false,		"playerstart" },
	{ ENVMAP,		0,		0,		EU_NONE,	false,		"envmap" },
	{ PARTICLES,	59,		0,		EU_NONE,	false,		"particles" },
	{ MAPSOUND,		58,		0,		EU_NONE,	false,		"sound" },
	{ SPOTLIGHT,	59,		0,		EU_NONE,	false,		"spotlight" },
	{ WEAPON,		59,		16,		EU_ITEM,	false,		"weapon" },
	{ TELEPORT,		50,		12,		EU_AUTO,	false,		"teleport" },
	{ RESERVED,		59,		12,		EU_NONE,	false,		"reserved" },
	{ TRIGGER,		58,		16,		EU_AUTO,	false,		"trigger" },
	{ PUSHER,		58,		12,		EU_AUTO,	false,		"pusher" },
	{ FLAG,			48,		32,		EU_NONE,	false,		"flag" },
	{ CHECKPOINT,	48,		16,		EU_NONE,	false,		"checkpoint" }, // FIXME
	{ CAMERA,		48,		0,		EU_NONE,	false,		"camera" },
	{ WAYPOINT,		1,		8,		EU_NONE,	true,		"waypoint" },
	{ ANNOUNCER,	64,		0,		EU_NONE,	false,		"announcer" },
	{ CONNECTION,	70,		0,		EU_NONE,	false,		"connection" },
};
#else
extern enttypes enttype[];
#endif

enum
{
	ANIM_EDIT = ANIM_GAMESPECIFIC, ANIM_LAG, ANIM_SWITCH, ANIM_TAUNT, ANIM_WIN, ANIM_LOSE,
	ANIM_CROUCH, ANIM_CRAWL_FORWARD, ANIM_CRAWL_BACKWARD, ANIM_CRAWL_LEFT, ANIM_CRAWL_RIGHT,
    ANIM_PLASMA, ANIM_PLASMA_SHOOT, ANIM_PLASMA_RELOAD,
    ANIM_SHOTGUN, ANIM_SHOTGUN_SHOOT, ANIM_SHOTGUN_RELOAD,
    ANIM_CHAINGUN, ANIM_CHAINGUN_SHOOT, ANIM_CHAINGUN_RELOAD,
    ANIM_GRENADES, ANIM_GRENADES_THROW, ANIM_GREANDES_RELOAD, ANIM_GRENADES_POWER,
    ANIM_FLAMER, ANIM_FLAMER_SHOOT, ANIM_FLAMER_RELOAD,
    ANIM_CARBINE, ANIM_CARBINE_SHOOT, ANIM_CARBINE_RELOAD,
    ANIM_RIFLE, ANIM_RIFLE_SHOOT, ANIM_RIFLE_RELOAD,
    ANIM_VWEP, ANIM_SHIELD, ANIM_POWERUP,
    ANIM_MAX
};

#define GUNSWITCHDELAY	800
#define PLAYERHEIGHT	15.f
#define EXPLOSIONSCALE	16.f

enum
{
	GUN_PLASMA = 0,
	GUN_SG,
	GUN_CG,
	GUN_FLAMER,
	GUN_CARBINE,
	GUN_RIFLE,
	GUN_GL,
	GUN_MAX
};

enum
{
	GNT_NONE = 0,
	GNT_FORCED = 1<<0, // forced spawned
};

enum
{
	GNS_IDLE = 0,
	GNS_SHOOT,
	GNS_RELOAD,
	GNS_POWER,
	GNS_SWITCH,
	GNS_PICKUP,
	GNS_MAX
};

enum
{
	IMPACT_GEOM    = 1<<0,
	BOUNCE_GEOM    = 1<<1,
	COLLIDE_GEOM   = IMPACT_GEOM | BOUNCE_GEOM,
	IMPACT_PLAYER  = 1<<2,
	BOUNCE_PLAYER  = 1<<3,
	COLLIDE_PLAYER = IMPACT_PLAYER | BOUNCE_PLAYER,
	COLLIDE_TRACE  = 1<<4
};

struct guntypes
{
	int	info, 		anim,			kick,	wobble,
			sound, 		esound, 	fsound,		rsound,
			add,	max,	adelay,	rdelay,	damage,	speed,	power,	time,
			delay,	explode,	rays,	spread,	zdiv,	collide;
	bool	radial,	extinguish,	reloads;
	float	offset,	elasticity,	reflectivity,	relativity,	waterfric,	weight;
	const char
			*name, *text,		*item,						*vwep,
			*proj;
};
#ifdef GAMESERVER
guntypes guntype[GUN_MAX] =
{
	{
		GUN_PLASMA,	ANIM_PLASMA,	-5,		5,
			S_PLASMA,	S_ENERGY,	S_HUM,		-1,
			20,		20,		200,	800,	20,		250,	0,		10000,
			0,		18,			1,		5,		0,		IMPACT_GEOM|IMPACT_PLAYER,
			false,	true,		true,
			1.0f,	0.f,		0.f,			0.1f,		1.0f,		0.f,
			"plasma",	"\fc",	"weapons/plasma/item",		"weapons/plasma/vwep",
			""
	},
	{
		GUN_SG,		ANIM_SHOTGUN,	-30,    30,
			S_SG,		S_RICOCHET,	S_WHIZZ,	S_RICOCHET,
			1,		8,		600,	1200,	10,		1000,	0,		1000,
			0,		0,			20,		40,		1,		BOUNCE_GEOM|IMPACT_PLAYER|COLLIDE_TRACE,
			false,	false,		true,
			1.0f,	0.5f,		50.f,			0.05f,		2.0f,		30.f,
			"shotgun",	"\fy",	"weapons/shotgun/item",		"weapons/shotgun/vwep",
			""
	},
	{
		GUN_CG,		ANIM_CHAINGUN,	-5,	     5,
			S_CG,		S_RICOCHET,	S_WHIZZ,	S_RICOCHET,
			40,		40,		100,    1000,	20,		1500,	0,		2000,
			0,		0,			1,		5,		4,		BOUNCE_GEOM|IMPACT_PLAYER|COLLIDE_TRACE,
			false,	false,		true,
			1.0f,	0.75f,		30.f,			0.05f,		2.0f,		0.f,
			"chaingun",	"\fo",	"weapons/chaingun/item",	"weapons/chaingun/vwep",
			""
	},
	{
		GUN_FLAMER,	ANIM_FLAMER,	-1,		 1,
			S_FLFIRE,	S_BURN,		S_BURNING,	-1,
			50,		50,		100, 	2000,	25,		100,	0,		3000,
			0,		32,			1,		5,		2,		BOUNCE_GEOM|BOUNCE_PLAYER,
			true,	true,		true,
			0.5f,	0.15f,		45.f,			0.25f,		1.5f,		50.f,
			"flamer",	"\fr",	"weapons/flamer/item",		"weapons/flamer/vwep",
			""
	},
	{
		GUN_CARBINE,ANIM_CARBINE,	-10,	10,
			S_CARBINE,	S_RICOCHET,	S_WHIZZ,	-1,
			10,		10,		500,    1000,	50,		2000,	0,		10000,
			0,		0,			1,		0,		0,		IMPACT_GEOM|IMPACT_PLAYER|COLLIDE_TRACE,
			false,	false,		true,
			1.0f,	0.f,		0.f,			0.f,		2.0f,		0.f,
			"carbine",	"\fa",	"weapons/carbine/item",		"weapons/carbine/vwep",
			"projectiles/bullet"
	},
	{
		GUN_RIFLE,	ANIM_RIFLE,		-35,  	25,
			S_RIFLE,	S_RICOCHET,	S_WHIZZ,	-1,
			1,		5,		800,	1600,	100,	2000,	0,		10000,
			0,		0,			1,		0,		0,		IMPACT_GEOM|IMPACT_PLAYER|COLLIDE_TRACE,
			false,	false,		true,
			1.0f,	0.f,		 0.f,			0.f,		2.0f,		0.f,
			"rifle",	"\fw",	"weapons/rifle/item",		"weapons/rifle/vwep",
			"projectiles/bullet"
	},
	{
		GUN_GL,		ANIM_GRENADES,	-15,    10,
			S_GLFIRE,	S_EXPLODE,	S_WHIRR,	S_TINK,
			2,		4,		1500,	3000,	200,	200,	1000,	3000,
			150,	64,			1,		0,		0,		BOUNCE_GEOM|BOUNCE_PLAYER,
			false,	false,		false,
			1.0f,	0.33f,		0.f,			0.45f,		2.0f,		75.f,
			"grenades",	"\fg",	"weapons/grenades/item",	"weapons/grenades/vwep",
			"projectiles/grenade"
	},
};
#else
extern guntypes guntype[];
#endif

#define isgun(a)		(a > -1 && a < GUN_MAX)
#define gunloads(a,b)	(a == b || guntype[a].reloads)
#define guncarry(a,b)	(a != b && guntype[a].reloads)
#define gunattr(a,b)	(b != GUN_PLASMA && a == b ? GUN_PLASMA : a)

enum
{
	HIT_NONE 	= 0,
	HIT_PROJ	= 1<<0,
	HIT_EXPLODE	= 1<<1,
	HIT_BURN	= 1<<2,
	HIT_MELT	= 1<<3,
	HIT_FALL	= 1<<4,
	HIT_WAVE	= 1<<5,
	HIT_PUSH	= 1<<6,
	HIT_LEGS	= 1<<7,
	HIT_TORSO	= 1<<8,
	HIT_HEAD	= 1<<9,
};

#define hithurts(x) (x&HIT_BURN || x&HIT_EXPLODE || x&HIT_PROJ || x&HIT_MELT || x&HIT_FALL)

enum
{
	G_DEMO = 0,
	G_LOBBY,
	G_EDITMODE,
	G_MISSION,
	G_DEATHMATCH,
	G_STF,
	G_CTF,
	G_MAX
};

enum
{
	G_M_NONE	= 0,
	G_M_MULTI	= 1<<0,
	G_M_TEAM	= 1<<1,
	G_M_INSTA	= 1<<2,
	G_M_DUEL	= 1<<3,
	G_M_LMS		= 1<<4,
	G_M_DM		= G_M_INSTA,
	G_M_TEAMS	= G_M_MULTI|G_M_TEAM|G_M_INSTA,
	G_M_ALL		= G_M_MULTI|G_M_TEAM|G_M_INSTA|G_M_DUEL|G_M_LMS,
};
#define G_M_NUM 4

struct gametypes
{
	int	type,			mutators,				implied;		const char *name;
};
#ifdef GAMESERVER
gametypes gametype[] = {
	{ G_DEMO,			G_M_NONE,				G_M_NONE,		"Demo" },
	{ G_LOBBY,			G_M_NONE,				G_M_NONE,		"Lobby" },
	{ G_EDITMODE,		G_M_NONE,				G_M_NONE,		"Editing" },
	{ G_MISSION,		G_M_NONE,				G_M_NONE,		"Mission" },
	{ G_DEATHMATCH,		G_M_ALL,				G_M_NONE,		"Deathmatch" },
	{ G_STF,			G_M_TEAMS,				G_M_TEAM,		"Secure-the-Flag" },
	{ G_CTF,			G_M_TEAMS,				G_M_TEAM,		"Capture-the-Flag" },
}, mutstype[] = {
	{ G_M_MULTI,		G_M_ALL,				G_M_TEAM,		"Multi-Sided" },
	{ G_M_TEAM,			G_M_TEAMS,				G_M_NONE,		"Team" },
	{ G_M_INSTA,		G_M_ALL,				G_M_NONE,		"Instagib" },
	{ G_M_DUEL,			G_M_DM|G_M_DUEL,		G_M_NONE,		"Duel" },
	{ G_M_LMS,			G_M_DM|G_M_LMS,			G_M_NONE,		"Last-Man-Standing" },
};
#else
extern gametypes gametype[], mutstype[];
#endif

#define m_game(a)			(a > -1 && a < G_MAX)

#define m_demo(a)			(a == G_DEMO)
#define m_lobby(a)			(a == G_LOBBY)
#define m_edit(a)			(a == G_EDITMODE)
#define m_mission(a)		(a == G_MISSION)
#define m_dm(a)				(a == G_DEATHMATCH)
#define m_stf(a)			(a == G_STF)
#define m_ctf(a)			(a == G_CTF)

#define m_play(a)			(a >= G_MISSION)
#define m_flag(a)			(m_stf(a) || m_ctf(a))
#define m_timed(a)			(a >= G_DEATHMATCH)

#define m_multi(a,b)		((b & G_M_MULTI) || (gametype[a].implied & G_M_MULTI))
#define m_team(a,b)			((b & G_M_TEAM) || (gametype[a].implied & G_M_TEAM))
#define m_insta(a,b)		((b & G_M_INSTA) || (gametype[a].implied & G_M_INSTA))
#define m_duel(a,b)			((b & G_M_DUEL) || (gametype[a].implied & G_M_DUEL))
#define m_lms(a,b)			((b & G_M_LMS) || (gametype[a].implied & G_M_LMS))

#define m_duke(a,b)			(a >= G_DEATHMATCH && (m_duel(a, b) || m_lms(a, b)))
#define m_regen(a,b)		(a >= G_DEATHMATCH && !m_insta(a, b) && !m_duke(a, b))

#ifdef GAMESERVER
#define m_spawngun(a,b)		(m_insta(a,b) ? sv_instaspawngun : sv_spawngun)
#define m_noitems(a,b)		(sv_itemsallowed < (m_insta(a,b) ? 2 : 1))
#else
#define m_spawngun(a,b)		(m_insta(a,b) ? instaspawngun : spawngun)
#define m_noitems(a,b)		(itemsallowed < (m_insta(a,b) ? 2 : 1))
#endif

// network messages codes, c2s, c2c, s2c
enum
{
	SV_INITS2C = 0, SV_INITC2S, SV_POS, SV_TEXT, SV_COMMAND, SV_ANNOUNCE, SV_SOUND, SV_CDIS,
	SV_SHOOT, SV_DESTROY, SV_SUICIDE, SV_DIED, SV_DAMAGE, SV_SHOTFX,
	SV_TRYSPAWN, SV_SPAWNSTATE, SV_SPAWN, SV_FORCEDEATH,
	SV_DROP, SV_GUNSELECT, SV_TAUNT,
	SV_MAPCHANGE, SV_MAPVOTE, SV_ITEMSPAWN, SV_ITEMUSE, SV_TRIGGER, SV_EXECLINK,
	SV_PING, SV_PONG, SV_CLIENTPING,
	SV_TIMEUP, SV_NEWGAME, SV_ITEMACC,
	SV_SERVMSG, SV_GAMEINFO, SV_RESUME,
    SV_EDITMODE, SV_EDITENT, SV_EDITLINK, SV_EDITVAR, SV_EDITF, SV_EDITT, SV_EDITM, SV_FLIP, SV_COPY, SV_PASTE, SV_ROTATE, SV_REPLACE, SV_DELCUBE, SV_REMIP, SV_NEWMAP,
    SV_GETMAP, SV_SENDMAP, SV_SENDMAPFILE, SV_SENDMAPSHOT, SV_SENDMAPCONFIG,
	SV_MASTERMODE, SV_KICK, SV_CLEARBANS, SV_CURRENTMASTER, SV_SPECTATOR, SV_WAITING, SV_SETMASTER, SV_SETTEAM, SV_APPROVEMASTER,
	SV_FLAGS, SV_FLAGINFO,
    SV_TAKEFLAG, SV_RETURNFLAG, SV_RESETFLAG, SV_DROPFLAG, SV_SCOREFLAG, SV_INITFLAGS,
	SV_TEAMSCORE, SV_FORCEINTERMISSION,
	SV_LISTDEMOS, SV_SENDDEMOLIST, SV_GETDEMO, SV_SENDDEMO,
	SV_DEMOPLAYBACK, SV_RECORDDEMO, SV_STOPDEMO, SV_CLEARDEMOS,
	SV_CLIENT, SV_RELOAD, SV_REGEN,
	SV_ADDBOT, SV_DELBOT, SV_INITAI
};

#ifdef GAMESERVER
char msgsizelookup(int msg)
{
	char msgsizesl[] =				// size inclusive message token, 0 for variable or not-checked sizes
	{
		SV_INITS2C, 3, SV_INITC2S, 0, SV_POS, 0, SV_TEXT, 0, SV_COMMAND, 0,
		SV_ANNOUNCE, 0, SV_SOUND, 3, SV_CDIS, 2,
		SV_SHOOT, 0, SV_DESTROY, 0, SV_SUICIDE, 3, SV_DIED, 8, SV_DAMAGE, 10, SV_SHOTFX, 9,
		SV_TRYSPAWN, 2, SV_SPAWNSTATE, 13, SV_SPAWN, 4, SV_FORCEDEATH, 2,
		SV_DROP, 4, SV_GUNSELECT, 0, SV_TAUNT, 2,
		SV_MAPCHANGE, 0, SV_MAPVOTE, 0, SV_ITEMSPAWN, 2, SV_ITEMUSE, 0, SV_TRIGGER, 0, SV_EXECLINK, 3,
		SV_PING, 2, SV_PONG, 2, SV_CLIENTPING, 2,
		SV_TIMEUP, 2, SV_NEWGAME, 1, SV_ITEMACC, 0,
		SV_SERVMSG, 0, SV_GAMEINFO, 0, SV_RESUME, 0,
        SV_EDITMODE, 2, SV_EDITENT, 11, SV_EDITLINK, 4, SV_EDITVAR, 0, SV_EDITF, 16, SV_EDITT, 16, SV_EDITM, 15, SV_FLIP, 14, SV_COPY, 14, SV_PASTE, 14, SV_ROTATE, 15, SV_REPLACE, 16, SV_DELCUBE, 14, SV_REMIP, 1, SV_NEWMAP, 2,
        SV_GETMAP, 0, SV_SENDMAP, 0, SV_SENDMAPFILE, 0, SV_SENDMAPSHOT, 0, SV_SENDMAPCONFIG, 0,
		SV_MASTERMODE, 2, SV_KICK, 2, SV_CLEARBANS, 1, SV_CURRENTMASTER, 3, SV_SPECTATOR, 3, SV_WAITING, 2, SV_SETMASTER, 0, SV_SETTEAM, 0, SV_APPROVEMASTER, 2,
		SV_FLAGS, 0, SV_FLAGINFO, 0,
        SV_DROPFLAG, 0, SV_SCOREFLAG, 5, SV_RETURNFLAG, 3, SV_TAKEFLAG, 3, SV_RESETFLAG, 2, SV_INITFLAGS, 0,
		SV_TEAMSCORE, 0, SV_FORCEINTERMISSION, 1,
		SV_LISTDEMOS, 1, SV_SENDDEMOLIST, 0, SV_GETDEMO, 2, SV_SENDDEMO, 0,
		SV_DEMOPLAYBACK, 2, SV_RECORDDEMO, 2, SV_STOPDEMO, 1, SV_CLEARDEMOS, 2,
		SV_CLIENT, 0, SV_RELOAD, 0, SV_REGEN, 0,
		SV_ADDBOT, 0, SV_DELBOT, 0, SV_INITAI, 0,
		-1
	};
	for(char *p = msgsizesl; *p>=0; p += 2) if(*p==msg) return p[1];
	return -1;
}
#else
extern char msgsizelookup(int msg);
#endif

#define DEMO_MAGIC "BFDZ"

struct demoheader
{
	char magic[16];
	int version, gamever;
};

enum { TEAM_NEUTRAL = 0, TEAM_ALPHA, TEAM_BETA, TEAM_DELTA, TEAM_GAMMA, TEAM_ENEMY, TEAM_MAX };
struct teamtypes
{
	int	type,		colour;	const char *name,	*tpmdl,					*fpmdl,
		*flag,			*icon,			*chat;
};
#ifdef GAMESERVER
teamtypes teamtype[] = {
	{
		TEAM_NEUTRAL,	0xAAAAAA,	"neutral",	"actors/player",		"actors/player/vwep",
		"flag",			"team",			"\fw"
	},
	{
		TEAM_ALPHA,	0x2222AA,	"alpha",		"actors/player/alpha",	"actors/player/alpha/vwep",
		"flag/alpha",	"teamalpha",	"\fb"
	},
	{
		TEAM_BETA,	0xAA2222,	"beta",			"actors/player/beta",	"actors/player/beta/vwep",
		"flag/beta",	"teambeta",		"\fr"
	},
	{
		TEAM_DELTA,	0xAAAA22,	"delta",		"actors/player/delta",	"actors/player/delta/vwep",
		"flag/delta",	"teamdelta",	"\fy"
	},
	{
		TEAM_GAMMA,	0x22AA22,	"gamma",		"actors/player/gamma",	"actors/player/gamma/vwep",
		"flag/gamma",	"teamgamma",	"\fg"
	},
	{
		TEAM_ENEMY,	0xAAAAAA,	"enemy",		"actors/player",		"actors/player/vwep",
		"flag",			"team",			"\fa"
	}
};
#else
extern teamtypes teamtype[];
#endif

#define MAXNAMELEN		16
#define MAXHEALTH		100
#define MAXCARRY		2
#define MAXTEAMS		TEAM_GAMMA
#define numteams(a,b)	(m_multi(a, b) ? MAXTEAMS : MAXTEAMS/2)
#define isteam(a,b)		(a >= b && a <= MAXTEAMS)

#define REGENWAIT		3000
#define REGENTIME		1000
#define REGENHEAL		10

enum
{
	SAY_NONE	= 0,
	SAY_ACTION	= 1<<0,
	SAY_TEAM	= 1<<1,
};
#define SAY_NUM 2

enum
{
	PRIV_NONE = 0,
	PRIV_MASTER,
	PRIV_ADMIN,
	PRIV_MAX
};

#define MM_MODE 0xF
#define MM_AUTOAPPROVE 0x1000
#define MM_DEFAULT (MM_MODE | MM_AUTOAPPROVE)

enum { MM_OPEN = 0, MM_VETO, MM_LOCKED, MM_PRIVATE };

enum
{
	CAMERA_NONE = 0,
	CAMERA_PLAYER,
	CAMERA_FOLLOW,
	CAMERA_ENTITY,
	CAMERA_MAX
};

enum
{
	SINFO_STATUS,
	SINFO_HOST,
	SINFO_DESC,
	SINFO_PING,
	SINFO_PLAYERS,
	SINFO_MAXCLIENTS,
	SINFO_GAME,
	SINFO_MAP,
	SINFO_TIME,
	SINFO_MAX
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

enum { AI_NONE = 0, AI_BOT, AI_BSOLDIER, AI_RSOLDIER, AI_YSOLDIER, AI_GSOLDIER, AI_MAX };
#define isaitype(a)	(a >= 0 && a <= AI_MAX-1)

struct aitypes
{
	int type;	const char *name,	*mdl;
};
#ifdef GAMESERVER
aitypes aitype[] = {
	{ AI_NONE,		"",				"" },
	{ AI_BOT,		"bot",			"actors/player" },
	{ AI_BSOLDIER,	"alpha",		"actors/player/alpha" },
	{ AI_RSOLDIER,	"beta",			"actors/player/beta" },
	{ AI_YSOLDIER,	"delta",		"actors/player/delta" },
	{ AI_GSOLDIER,	"gamma",		"actors/player/gamma" },
};
#else
extern aitypes aitype[];
#endif

// inherited by gameent and server clients
struct gamestate
{
	int health, ammo[GUN_MAX], entid[GUN_MAX];
	int lastgun, gunselect, gunstate[GUN_MAX], gunwait[GUN_MAX], gunlast[GUN_MAX];
	int lastdeath, lifesequence, lastspawn, lastrespawn, lastpain, lastregen;
	int aitype, ownernum, skill, spree;

	gamestate() : lifesequence(0), aitype(AI_NONE), ownernum(-1), skill(0), spree(0) {}
	~gamestate() {}

	int hasgun(int gun, int sgun, int level = 0, int exclude = -1)
	{
		if(isgun(gun) && gun != exclude)
		{
			if(ammo[gun] > 0 || (gunloads(gun, sgun) && !ammo[gun])) switch(level)
			{
				case 0: default: return true; break; // has gun at all
				case 1: if(guncarry(gun, sgun)) return true; break; // only carriable
				case 2: if(ammo[gun] > 0) return true; break; // only with actual ammo
				case 3: if(ammo[gun] > 0 && gunloads(gun, sgun)) return true; break; // only reloadable with actual ammo
				case 4: if(ammo[gun] >= (gunloads(gun, sgun) ? 0 : guntype[gun].max)) return true; break; // only reloadable or those with < max
			}
		}
		return false;
	}

	int bestgun(int sgun, bool force = true)
	{
		int best = -1;
		loopi(GUN_MAX) if(hasgun(i, sgun, 1)) best = i;
		if(!isgun(best) && force) loopi(GUN_MAX) if(hasgun(i, sgun, 0)) best = i;
		return best;
	}

	int carry(int sgun)
	{
		int carry = 0;
		loopi(GUN_MAX) if(hasgun(i, sgun, 1)) carry++;
		return carry;
	}

	int drop(int sgun, int exclude = -1)
	{
		int gun = -1;
		if(hasgun(gunselect, sgun, 1, exclude)) gun = gunselect;
		else
		{
			loopi(GUN_MAX) if(hasgun(i, sgun, 1, exclude))
			{
				gun = i;
				break;
			}
		}
		return gun;
	}

	void gunreset(bool full = false)
	{
		loopi(GUN_MAX)
		{
			if(full)
			{
				gunstate[i] = GNS_IDLE;
				gunwait[i] = gunlast[i] = 0;
				ammo[i] = -1;
			}
			entid[i] = -1;
		}
		lastgun = gunselect = -1;
	}

	void setgunstate(int gun, int state, int delay, int millis)
	{
		if(isgun(gun))
		{
			gunstate[gun] = state;
			gunwait[gun] = delay;
			gunlast[gun] = millis;
		}
	}

	void gunswitch(int gun, int millis, int state = GNS_SWITCH)
	{
		lastgun = gunselect;
		setgunstate(lastgun, GNS_SWITCH, GUNSWITCHDELAY, millis);
		gunselect = gun;
		setgunstate(gun, state, GUNSWITCHDELAY, millis);
	}

	bool gunwaited(int gun, int millis)
	{
		return !isgun(gun) || millis-gunlast[gun] >= gunwait[gun];
	}

	bool canswitch(int gun, int sgun, int millis)
	{
		if(gun != gunselect && gunwaited(gunselect, millis) && hasgun(gun, sgun) && gunwaited(gun, millis))
			return true;
		return false;
	}

	bool canshoot(int gun, int sgun, int millis)
	{
		if(hasgun(gun, sgun) && ammo[gun] > 0 && gunwaited(gun, millis))
			return true;
		return false;
	}

	bool canreload(int gun, int sgun, int millis)
	{
		if(gunwaited(gunselect, millis) && gunloads(gun, sgun) && hasgun(gun, sgun) && ammo[gun] < guntype[gun].max && gunwaited(gun, millis))
			return true;
		return false;
	}

	bool canuse(int type, int attr1, int attr2, int attr3, int attr4, int attr5, int sgun, int millis)
	{
		if((type != TRIGGER || attr3 == TA_AUTO) && enttype[type].usetype == EU_AUTO)
			return true;
		if(gunwaited(gunselect, millis)) switch(type)
		{
			case TRIGGER:
			{
				return true;
				break;
			}
			case WEAPON:
			{ // can't use when reloading or firing
				if(isgun(attr1) && !hasgun(attr1, sgun, 4) && gunwaited(attr1, millis))
					return true;
				break;
			}
			default: break;
		}
		return false;
	}

	void useitem(int id, int type, int attr1, int attr2, int attr3, int attr4, int attr5, int sgun, int millis)
	{
		switch(type)
		{
			case TRIGGER: break;
			case WEAPON:
			{
				gunswitch(attr1, millis, hasgun(attr1, sgun) ? (gunselect != attr1 ? GNS_SWITCH : GNS_RELOAD) : GNS_PICKUP);
				ammo[attr1] = clamp((ammo[attr1] > 0 ? ammo[attr1] : 0)+guntype[attr1].add, 1, guntype[attr1].max);
				entid[attr1] = id;
				break;
			}
			default: break;
		}
	}

	void respawn(int millis)
	{
		health = MAXHEALTH;
		lastdeath = lastpain = lastregen = spree = 0;
		lastspawn = millis;
		lastrespawn = -1;
		gunreset(true);
	}

	void spawnstate(int sgun, bool others)
	{
		health = MAXHEALTH;
		lastgun = gunselect = sgun;
		loopi(GUN_MAX)
		{
			gunstate[i] = GNS_IDLE;
			gunwait[i] = gunlast[i] = 0;
			ammo[i] = (i == sgun || (others && !guntype[i].reloads)) ? guntype[i].add : -1;
			entid[i] = -1;
		}
	}

	// just subtract damage here, can set death, etc. later in code calling this
	int dodamage(int damage, int millis)
	{
		lastregen = 0;
		lastpain = millis;
		health -= damage;
		return damage;
	}
};

#if !defined(GAMESERVER) && !defined(STANDALONE)
struct gameentity : extentity
{
	int schan;
	int lastuse, lastspawn;
	bool mark;

	gameentity() : schan(-1), lastuse(0), lastspawn(0), mark(false) {}
	~gameentity()
	{
		if(issound(schan)) removesound(schan);
		schan = -1;
	}
};

enum
{
	ST_NONE		= 0,
	ST_CAMERA	= 1<<0,
	ST_CURSOR	= 1<<1,
	ST_GAME		= 1<<2,
	ST_SPAWNS	= 1<<3,
	ST_DEFAULT	= ST_CAMERA|ST_CURSOR|ST_GAME,
	ST_VIEW		= ST_CURSOR|ST_CAMERA,
	ST_ALL		= ST_CAMERA|ST_CURSOR|ST_GAME|ST_SPAWNS,
};

enum { ITEM_ENT = 0, ITEM_PROJ, ITEM_MAX };
struct actitem
{
	int type, target;
	float score;

	actitem() : type(ITEM_ENT), target(-1), score(0.f) {}
	~actitem() {}
};
#ifdef GAMEWORLD
const char *animnames[] =
{
	"dead", "dying", "idle",
	"forward", "backward", "left", "right",
	"pain", "jump", "sink", "swim",
	"mapmodel", "trigger on", "trigger off",
	"edit", "lag", "switch", "taunt", "win", "lose",
	"crouch", "crawl forward", "crawl backward", "crawl left", "crawl right",
	"plasma", "plasma shoot", "plasma reload",
	"shotgun", "shotgun shoot", "shotgun reload",
	"chaingun", "chaingun shoot", "chaingun reload",
	"grenades", "grenades throw", "grenades reload", "grenades power",
	"flamer", "flamer shoot", "flamer reload",
	"carbine", "carbine shoot", "carbine reload",
	"rifle", "rifle shoot", "rifle reload",
	"vwep", "shield", "powerup",
	""
};
#else
extern const char *animnames[];
#endif
struct serverstatuses
{
	int type,				colour;		const char *icon;
};
#ifdef GAMEWORLD
serverstatuses serverstatus[] = {
	{ SSTAT_OPEN,			0xFFFFFF,	"server" },
	{ SSTAT_LOCKED,			0xFF8800,	"serverlock" },
	{ SSTAT_PRIVATE,		0x8888FF,	"serverpriv" },
	{ SSTAT_FULL,			0xFF8888,	"serverfull" },
	{ SSTAT_UNKNOWN,		0x888888,	"serverunk" }
};
const char *serverinfotypes[] = {
	"",	"host", "desc", "ping", "pl", "max", "game", "map", "time"
};
#else
extern serverstatuses serverstatus[];
extern const char *serverinfotypes[];
#endif

// ai state information for the owner client
enum
{
	AI_S_WAIT = 0,		// waiting for next command
	AI_S_DEFEND,		// defend goal target
	AI_S_PURSUE,		// pursue goal target
	AI_S_ATTACK,		// attack goal target
	AI_S_INTEREST,		// interest in goal entity
	AI_S_MAX
};

static const int aiframetimes[AI_S_MAX] = { 250, 250, 250, 125, 250 };

enum
{
	AI_T_NODE,
	AI_T_PLAYER,
	AI_T_AFFINITY,
	AI_T_ENTITY,
	AI_T_DROP,
	AI_T_MAX
};

enum
{
	WP_NONE = 0,
	WP_CROUCH = 1<<0,
};

struct interest
{
	int state, node, target, targtype, expire;
	float tolerance, score;
	bool defers;
	interest() : state(-1), node(-1), target(-1), targtype(-1), expire(0), tolerance(0.f), score(0.f) {}
	~interest() {}
};

struct aistate
{
	int type, millis, expire, next, targtype, target, cycle;
	bool override, defers;

	aistate(int t, int m) : type(t), millis(m)
	{
		reset();
	}
	~aistate()
	{
	}

	void reset()
	{
		expire = cycle = 0;
		next = millis;
		if(type == AI_S_WAIT)
			next += rnd(1500) + 1500;
		targtype = target = -1;
		override = false;
		defers = true;
	}
};

struct aiinfo
{
	vector<aistate> state;
	vector<int> route;
	vec target, spot;
	int enemy, lastseen, gunpref;

	aiinfo() { reset(); }
	~aiinfo() { state.setsize(0); route.setsize(0); }

	void reset()
	{
		state.setsize(0);
		route.setsize(0);
		addstate(AI_S_WAIT);
		gunpref = rnd(GUN_MAX-1)+1;
		spot = target = vec(0, 0, 0);
		enemy = lastseen = -1;
	}

	aistate &addstate(int t)
	{
		aistate bs(t, lastmillis);
		return state.add(bs);
	}

	void removestate(int index = -1)
	{
		if(index < 0 && state.length()) state.pop();
		else if(state.inrange(index)) state.remove(index);
		enemy = lastseen = -1;
		if(!state.length()) addstate(AI_S_WAIT);
	}

	aistate &setstate(int t, bool pop = true)
	{
		if(pop) removestate();
		return addstate(t);
	}

	aistate &getstate(int idx = -1)
	{
		if(state.inrange(idx)) return state[idx];
		return state.last();
	}
};

enum { MDIR_FORWARD = 0, MDIR_BACKWARD, MDIR_MAX };
struct gameent : dynent, gamestate
{
	int team, clientnum, privilege, lastupdate, lastpredict, plag, ping,
		attacktime, reloadtime, usetime, lasttaunt, lastflag, frags, deaths, totaldamage,
			totalshots, smoothmillis, lastnode, respawned, suicided, wschan,
				reqswitch, reqreload, requse;
	editinfo *edit;
    float deltayaw, deltapitch, newyaw, newpitch;
    float deltaaimyaw, deltaaimpitch, newaimyaw, newaimpitch;
	aiinfo *ai;
    vec muzzle, mdir[MDIR_MAX];
	bool attacking, reloading, useaction, obliterated, k_up, k_down, k_left, k_right;

	string name, info, obit;

	gameent() : team(TEAM_NEUTRAL), clientnum(-1), privilege(PRIV_NONE), lastupdate(0), lastpredict(0), plag(0), ping(0), frags(0), deaths(0), totaldamage(0), totalshots(0), smoothmillis(-1), wschan(-1), edit(NULL), ai(NULL), muzzle(-1, -1, -1),
		k_up(false), k_down(false), k_left(false), k_right(false)
	{
		name[0] = info[0] = obit[0] = 0;
		respawn(-1);
	}
	~gameent()
	{
		freeeditinfo(edit);
		if(ai) delete ai;
		removetrackedparticles(this);
		removetrackedsounds(this);
		if(issound(wschan)) removesound(wschan);
		wschan = -1;
	}

	void hitpush(int damage, int flags, const vec &dir)
	{
		int force = damage;
		if(flags&HIT_WAVE || !hithurts(flags)) force /= 2;
		vel.add(vec(dir).mul(damage*100.f/weight));
	}

	void stopactions()
	{
		attacking = reloading = useaction = false;
		attacktime = reloadtime = usetime = 0;
	}

	void stopmoving()
	{
		dynent::stopmoving();
		stopactions();
	}

	void respawn(int millis)
	{
		stopmoving();
		dynent::reset();
		gamestate::respawn(millis);
		obliterated = false;
		lasttaunt = 0;
		lastflag = respawned = suicided = lastnode = reqswitch = reqreload = requse = -1;
		obit[0] = 0;
	}

	void resetstate(int millis)
	{
		respawn(millis);
		frags = deaths = totaldamage = totalshots = 0;
		if(state != CS_SPECTATOR) state = CS_DEAD;
	}
};

enum { PRJ_SHOT = 0, PRJ_GIBS, PRJ_DEBRIS, PRJ_ENT };

struct projent : dynent
{
	vec from, to, norm;
	int addtime, lifetime, lifemillis, waittime, spawntime, lastradial, lasteffect, lastbounce;
	float movement, roll, lifespan, lifesize;
	bool local, beenused, radial, extinguish;
	int projtype, projcollide;
	float elasticity, reflectivity, relativity, waterfric;
	int ent, attr1, attr2, attr3, attr4, attr5;
	int schan, id;
	entitylight light;
	gameent *owner;
	physent *hit;
	const char *mdl;

	projent() : norm(0, 0, 1), projtype(PRJ_SHOT), id(-1), owner(NULL), hit(NULL), mdl(NULL) { reset(); }
	~projent()
	{
		removetrackedparticles(this);
		removetrackedsounds(this);
		if(issound(schan)) removesound(schan);
		schan = -1;
	}

	void reset()
	{
		physent::reset();
		type = ENT_PROJ;
		state = CS_ALIVE;
		norm = vec(0, 0, 1);
		addtime = lifetime = lifemillis = waittime = spawntime = lastradial = lasteffect = lastbounce = 0;
		ent = attr1 = attr2 = attr3 = attr4 = attr5 = 0;
		schan = id = -1;
		movement = roll = lifespan = lifesize = 0.f;
		beenused = radial = extinguish = false;
		projcollide = BOUNCE_GEOM|BOUNCE_PLAYER;
	}

	bool ready()
	{
		if(owner && !beenused && waittime <= 0 && state != CS_DEAD)
			return true;
		return false;
	}
};

namespace entities
{
	extern vector<extentity *> ents;

	struct avoidset
	{
		struct obstacle
		{
			dynent *ent;
			int numentities;
			bool avoid;

			obstacle(dynent *ent) : ent(ent), numentities(0) {}
		};

		vector<obstacle> obstacles;
		vector<int> entities;

		void clear()
		{
			obstacles.setsizenodelete(0);
			entities.setsizenodelete(0);
		}

		void add(dynent *ent, int entity)
		{
			if(obstacles.empty() || ent!=obstacles.last().ent) obstacles.add(obstacle(ent));
			obstacles.last().numentities++;
			entities.add(entity);
		}

		#define loopavoid(v, d, body) \
			if(!(v).obstacles.empty()) \
			{ \
				int cur = 0; \
				loopv((v).obstacles) \
				{ \
					entities::avoidset::obstacle &ob = (v).obstacles[i]; \
					int next = cur + ob.numentities; \
					if(ob.ent && ob.ent != (d)) \
					{ \
						for(; cur < next; cur++) \
						{ \
							int ent = (v).entities[cur]; \
							body; \
						} \
					} \
					cur = next; \
				} \
			}

		bool find(int entity, gameent *d)
		{
			loopavoid(*this, d, { if(ent == entity) return true; });
			return false;
		}
	};

	extern bool route(gameent *d, int node, int goal, vector<int> &route, avoidset &obstacles, float tolerance, bool retry = false, float *score = NULL);
	extern int entitynode(const vec &v, bool dist = true, int type = WAYPOINT);
	extern bool collateitems(gameent *d, vector<actitem> &actitems);
	extern void checkitems(gameent *d);
	extern void putitems(ucharbuf &p);
	extern void announce(int idx, const char *msg = "", bool force = false);
	extern void execlink(gameent *d, int index, bool local);
	extern void setspawn(int n, bool on);
	extern void findplayerspawn(dynent *d, int forceent = -1, int tag = -1, int retries = 0);
	extern const char *entinfo(int type, int attr1 = 0, int attr2 = 0, int attr3 = 0, int attr4 = 0, int attr5 = 0, bool full = false);
	extern void useeffects(gameent *d, int n, bool s, int g, int r);
	extern const char *entmdlname(int type, int attr1 = 0, int attr2 = 0, int attr3 = 0, int attr4 = 0, int attr5 = 0);
	extern void preload();
	extern void mapstart();
	extern const char *findname(int type);
	extern void adddynlights();
	extern void render();
	extern void start();
	extern void update();
}

namespace server
{
	extern void stopdemo();
}

namespace client
{
	extern bool demoplayback;
	extern void addmsg(int type, const char *fmt = NULL, ...);
	extern void mapstart();
}

namespace physics
{
	extern int smoothmove, smoothdist;
	extern bool canimpulse(physent *d);
	extern void smoothplayer(gameent *d, int res, bool local);
	extern void update();
}

namespace projs
{
	extern vector<projent *> projs;

	extern void reset();
	extern void update();
	extern void create(vec &from, vec &to, bool local, gameent *d, int type, int lifetime, int lifemillis, int waittime, int speed, int id = 0, int ent = -1, int attr1 = 0, int attr2 = 0, int attr3 = 0, int attr4 = 0, int attr5 = 0);
	extern void preload();
	extern void remove(gameent *owner);
	extern void shootv(int gun, int power, vec &from, vector<vec> &locs, gameent *d, bool local);
	extern void drop(gameent *d, int g, int n, bool local = true);
	extern void adddynlights();
	extern void render();
}

namespace weapons
{
	extern int autoreload;
	extern void reload(gameent *d);
	extern void shoot(gameent *d, vec &targ, int force = 0);
	extern bool doautoreload(gameent *d);
	extern void preload(int gun = -1);
}

namespace ai
{
	const float AIISNEAR			= 64.f;			// is near
	const float AIISFAR				= 128.f;		// too far
	const float AIJUMPHEIGHT		= 6.f;			// decides to jump
	const float AIJUMPIMPULSE		= 16.f;			// impulse to jump
	const float AILOSMIN			= 64.f;			// minimum line of sight
	const float AILOSMAX			= 4096.f;		// maximum line of sight
	const float AIFOVMIN			= 90.f;			// minimum field of view
	const float AIFOVMAX			= 130.f;		// maximum field of view

	#define AILOSDIST(x)			clamp((AILOSMIN+(AILOSMAX-AILOSMIN))/100.f*float(x), float(AILOSMIN), float(getvar("fog")+AILOSMIN))
	#define AIFOVX(x)				clamp((AIFOVMIN+(AIFOVMAX-AIFOVMIN))/100.f*float(x), float(AIFOVMIN), float(AIFOVMAX))
	#define AIFOVY(x)				AIFOVX(x)*3.f/4.f
    #define AIMAYTARG(y)           (y->state == CS_ALIVE && lastmillis-y->lastspawn > REGENWAIT)
	#define AITARG(x,y,z)			(y != x && AIMAYTARG(y) && (!z || !m_team(world::gamemode, world::mutators) || (x)->team != (y)->team))
	#define AICANSEE(x,y,z)			getsight(x, z->yaw, z->pitch, y, targ, AILOSDIST(z->skill), AIFOVX(z->skill), AIFOVY(z->skill))

	extern void spawned(gameent *d);
	extern void init(gameent *d, int at, int on, int sk, int bn, char *name, int tm);
	extern bool checkothers(vector<int> &targets, gameent *d = NULL, int state = -1, int targtype = -1, int target = -1, bool teams = false);
	extern bool makeroute(gameent *d, aistate &b, int node, float tolerance = AIISNEAR, bool retry = false);
	extern bool makeroute(gameent *d, aistate &b, vec &pos, float tolerance = AIISNEAR, bool dist = false);
	extern bool violence(gameent *d, aistate &b, gameent *e, bool pursue = false);
	extern bool patrol(gameent *d, aistate &b, vec &pos, float radius = AIISNEAR, float wander = AIISFAR, bool retry = false);
	extern bool randomnode(gameent *d, aistate &b, vec &from, vec &to, float radius = AIISNEAR, float wander = AIISFAR);
	extern bool randomnode(gameent *d, aistate &b, float radius = AIISNEAR, float wander = AIISFAR);
	extern bool defer(gameent *d, aistate &b, bool pursue = false);
	extern void update();
	extern void avoid();
	extern void think(gameent *d, int idx);
	extern void damaged(gameent *d, gameent *e, int gun, int flags, int damage, int health, int millis, vec &dir);
	extern void killed(gameent *d, gameent *e, int gun, int flags, int damage);
	extern void render();
}

namespace hud
{
	extern int hudwidth, hudsize;
	extern float radarblipblend, radarnameblend, inventoryblend, inventoryskew;
	extern void drawquad(int x, int y, int w, int h, float tx1 = 0, float ty1 = 0, float tx2 = 1, float ty2 = 1);
	extern void drawtex(int x, int y, int w, int h, float tx = 0, float ty = 0, float tw = 1, float th = 1);
	extern void drawsized(int x, int y, int s);
	extern void drawblip(int w, int h, int s, float blend, int idx, vec &dir, float r = 1.f, float g = 1.f, float b = 1.f, const char *font = "radar", float fade = -1.f, const char *text = NULL, ...);
	extern int drawitem(const char *tex, int x, int y, float size, float fade, float skew, const char *font = NULL, float blend = 1.f, const char *text = NULL, ...);
	extern const char *flagtex(int team = TEAM_NEUTRAL);
	extern float radarrange();
}

namespace world
{
	extern int gamemode, mutators, nextmode, nextmuts, minremain, maptime,
		quakewobble, damageresidue, lasthit, lastzoom, lastspec, spectvtime,
			thirdpersonaim, firstpersonaim;
	extern bool intermission, zooming;
	extern gameent *player1;
	extern vector<gameent *> players;

	extern gameent *newclient(int cn);
	extern gameent *getclient(int cn);
	extern gameent *intersectclosest(vec &from, vec &to, gameent *at);
	extern void clientdisconnected(int cn);
	extern char *colorname(gameent *d, char *name = NULL, const char *prefix = "", bool team = true, bool dupname = true);
	extern int respawnwait(gameent *d);
	extern void respawn(gameent *d);
	extern void respawnself(gameent *d);
	extern void spawneffect(const vec &o, int colour = 0xFFFFFF, int radius = 4, float size = 2.f, int num = 100, int fade = 250, float vel = 15.f);
	extern void suicide(gameent *d, int flags);
	extern void fixrange(float &yaw, float &pitch);
	extern void fixfullrange(float &yaw, float &pitch, float &roll, bool full);
	extern bool allowmove(physent *d);
	extern int mousestyle();
	extern int deadzone();
	extern int panspeed();
	extern bool inzoom();
	extern bool inzoomswitch();
	extern int zoomtime();
	extern bool tvmode();
	extern void resetstates(int types);
	extern void damaged(int gun, int flags, int damage, int health, gameent *d, gameent *actor, int millis, vec &dir);
	extern void killed(int gun, int flags, int damage, gameent *d, gameent *actor);
	extern void timeupdate(int timeremain);
}
#endif
#include "ctf.h"
#include "stf.h"
#include "vars.h"
#ifndef GAMESERVER
#include "scoreboard.h"
#endif
