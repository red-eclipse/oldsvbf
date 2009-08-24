struct gameent;

enum { AI_BOT, AI_TURRET, AI_GUARD, AI_ZOMBIE, AI_MAX, AI_START = AI_TURRET };
enum { AI_F_NONE = 0, AI_F_RANDWEAP = 1<<0 };
#define isaitype(a)	(a >= 0 && a <= AI_MAX-1)

struct aitypes
{
	int	type,			weap,			health,	maxspeed,	frame;	float	skill,	xradius,	yradius,	height,		weight;
	bool	canmove,	canfight,	useweap;	const char	*name,		*mdl;
};
#ifdef GAMESERVER
aitypes aitype[] = {
	{
		AI_BOT,			-1, 			0,		50,			1,				1.f,	0,			0, 			0,			200,
			true,		true,		true,					"bot",		"actors/player"
	},
	{
		AI_TURRET,		WEAP_SMG,	 	200,	0,			2,				1.f,	3,			3,			4,			0,
			false,		true,		true,					"turret",	"actors/player/vwep"
	},
	{
		AI_GUARD,		WEAP_PISTOL, 	100,	50,			2,				0.5f,	3,			3,			14,			100,
			true,		false,		true,					"guard",	"actors/player/beta"
	},
	{
		AI_ZOMBIE,		WEAP_RIFLE, 	200,	25,			10,				0.25f,	3,			3,			14,			50,
			true,		false,		true,					"zombie",	"actors/player/gamma"
	},
};
#else
extern aitypes aitype[];
#endif

enum
{
	WP_F_NONE = 0,
	WP_F_CROUCH = 1<<0,
};

namespace ai
{
	const float CLOSEDIST		= float(enttype[WAYPOINT].radius);	// is close
	const float NEARDIST		= CLOSEDIST*8.f;					// is near
	const float FARDIST			= CLOSEDIST*24.f;					// too far
	const float JUMPMIN			= CLOSEDIST*0.25f;					// decides to jump
	const float JUMPMAX			= CLOSEDIST*1.5f;					// max jump
	const float SIGHTMIN		= CLOSEDIST*2.f;					// minimum line of sight
	const float SIGHTMAX		= CLOSEDIST*64.f;					// maximum line of sight
	const float VIEWMIN			= 70.f;								// minimum field of view
	const float VIEWMAX			= 150.f;							// maximum field of view

	// ai state information for the owner client
	enum
	{
		AI_S_WAIT = 0,		// waiting for next command
		AI_S_DEFEND,		// defend goal target
		AI_S_PURSUE,		// pursue goal target
		AI_S_INTEREST,		// interest in goal entity
		AI_S_MAX
	};

	enum
	{
		AI_T_NODE,
		AI_T_PLAYER,
		AI_T_AFFINITY,
		AI_T_ENTITY,
		AI_T_DROP,
		AI_T_MAX
	};

	struct interest
	{
		int state, node, target, targtype;
		float score;
		interest() : state(-1), node(-1), target(-1), targtype(-1), score(0.f) {}
		~interest() {}
	};

	struct aistate
	{
		int type, millis, next, targtype, target, idle;
		bool override;

		aistate(int m, int t, int r = -1, int v = -1) : type(t), millis(m), targtype(r), target(v)
		{
			reset();
		}
		~aistate() {}

		void reset()
		{
			next = millis;
			idle = 0;
			override = false;
		}
	};

	const int NUMPREVNODES = 8;

	struct aiinfo
	{
		vector<aistate> state;
		vector<int> route;
		vec target, spot;
		int enemy, enemyseen, enemymillis, prevnodes[NUMPREVNODES],
			lastrun, lasthunt, lastaction, jumpseed, jumprand;
		float targyaw, targpitch, views[3];
		bool suspended, dontmove, becareful, tryreset, trywipe;

		aiinfo()
		{
			reset();
			loopk(3) views[k] = 0.f;
		}
		~aiinfo() {}

		void clear(bool prev = true)
		{
            if(prev) memset(prevnodes, -1, sizeof(prevnodes));
            route.setsizenodelete(0);
		}

		void wipe()
		{
			clear(true);
			state.setsizenodelete(0);
			addstate(AI_S_WAIT);
			trywipe = becareful = false;
			suspended = true;
		}

		void reset(bool tryit = false)
		{
			wipe();
			if(!tryit)
			{
				spot = target = vec(0, 0, 0);
				enemy = -1;
				lastaction = lasthunt = enemyseen = enemymillis = 0;
				lastrun = jumpseed = lastmillis;
				jumprand = lastmillis+5000;
				dontmove = false;
			}
			targyaw = rnd(360);
			targpitch = 0.f;
			tryreset = tryit;
		}

		bool hasprevnode(int n) const
		{
			loopi(NUMPREVNODES) if(prevnodes[i] == n) return true;
			return false;
		}

		void addprevnode(int n)
		{
			if(prevnodes[0] != n)
			{
				memmove(&prevnodes[1], prevnodes, sizeof(prevnodes) - sizeof(prevnodes[0]));
				prevnodes[0] = n;
			}
		}

		aistate &addstate(int t, int r = -1, int v = -1)
		{
			return state.add(aistate(lastmillis, t, r, v));
		}

		void removestate(int index = -1)
		{
			if(index < 0) state.pop();
			else if(state.inrange(index)) state.remove(index);
			if(!state.length()) addstate(AI_S_WAIT);
		}

		aistate &getstate(int idx = -1)
		{
			if(state.inrange(idx)) return state[idx];
			return state.last();
		}

		aistate &switchstate(aistate &b, int t, int r = -1, int v = -1)
		{
			if(b.type == t && b.targtype == r)
			{
				b.millis = lastmillis;
				b.target = v;
				b.reset();
				return b;
			}
			return addstate(t, r, v);
		}
	};

	extern vec aitarget;
	extern int aidebug, showaiinfo;

	extern float viewdist(int x = 101);
	extern float viewfieldx(int x = 101);
	extern float viewfieldy(int x = 101);
	extern bool targetable(gameent *d, gameent *e, bool z = true);
	extern bool cansee(gameent *d, vec &x, vec &y, vec &targ = aitarget);
	extern int owner(gameent *d);

	extern void init(gameent *d, int at, int et, int on, int sk, int bn, char *name, int tm);

	extern bool badhealth(gameent *d);
	extern bool checkothers(vector<int> &targets, gameent *d = NULL, int state = -1, int targtype = -1, int target = -1, bool teams = false);
	extern bool makeroute(gameent *d, aistate &b, int node, bool changed = true, int retries = 0);
	extern bool makeroute(gameent *d, aistate &b, const vec &pos, bool changed = true, int retries = 0);
	extern bool randomnode(gameent *d, aistate &b, const vec &pos, float guard = NEARDIST, float wander = FARDIST);
	extern bool randomnode(gameent *d, aistate &b, float guard = NEARDIST, float wander = FARDIST);
	extern bool violence(gameent *d, aistate &b, gameent *e, bool pursue = false);
	extern bool patrol(gameent *d, aistate &b, const vec &pos, float guard = NEARDIST, float wander = FARDIST, int walk = 1, bool retry = false);
	extern bool defend(gameent *d, aistate &b, const vec &pos, float guard = NEARDIST, float wander = FARDIST, int walk = 0);

	extern void spawned(gameent *d, int ent);
	extern void damaged(gameent *d, gameent *e);
	extern void killed(gameent *d, gameent *e);
	extern void update();
	extern void avoid();
	extern void think(gameent *d, bool run);
	extern void render();
};
