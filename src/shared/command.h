// script binding functionality

enum { ID_VAR, ID_COMMAND, ID_CCOMMAND, ID_ALIAS };

enum { NO_OVERRIDE = INT_MAX, OVERRIDDEN = 0 };

enum
{
	IDC_GLOBAL = 1<<1,
	IDC_WORLD = 1<<2,
	IDC_SERVER = 1<<3,
	IDC_ALL = IDC_GLOBAL|IDC_WORLD|IDC_SERVER,
	IDC_PERSIST = 1<<4,
};

struct identstack
{
    char *action;
    identstack *next;
};

struct ident
{
    int _type;           // one of ID_* above
    char *_name;
    int _min, _max;      // ID_VAR
    int _override;       // either NO_OVERRIDE, OVERRIDDEN, or value
    int _context;
    union
    {
        void (__cdecl *_fun)(); // ID_VAR, ID_COMMAND, ID_CCOMMAND
        identstack *_stack;  // ID_ALIAS
    };
    union
    {
        char *_narg;     // ID_COMMAND, ID_CCOMMAND
        int _val;        // ID_VAR
        char *_action;   // ID_ALIAS
    };
    union
    {
        char *_isexecuting;  // ID_ALIAS
        int *_storage;       // ID_VAR
    };
    void *self;

    ident() {}
    ident(int t, char *n, int m, int c, int x, int *s, void *f = NULL, int ct = IDC_GLOBAL)
        : _type(t), _name(n), _min(m), _max(x), _override(NO_OVERRIDE), _context(ct), _fun((void (__cdecl *)(void))f), _val(c), _storage(s) {}
    ident(int t, char *n, char *a, int ct = IDC_GLOBAL)
        : _type(t), _name(n), _override(NO_OVERRIDE), _context(ct), _stack(NULL), _action(a) {}
    ident(int t, char *n, char *narg, void *f = NULL, void *_s = NULL, int ct = IDC_GLOBAL)
        : _type(t), _name(n), _context(ct), _fun((void (__cdecl *)(void))f), _narg(narg), self(_s) {}
    virtual ~ident() {}

    ident &operator=(const ident &o) { memcpy(this, &o, sizeof(ident)); return *this; }        // force vtable copy, ugh

    int operator()() { return _val; }

    virtual void changed() { if(_fun) _fun(); }
};

extern void addident(char *name, ident *id);
extern void intret(int v);
extern void result(const char *s);

// nasty macros for registering script functions, abuses globals to avoid excessive infrastructure
typedef hashtable<char *, ident> identtable;
extern identtable *idents;

enum
{
	PRIV_NONE = 0,
	PRIV_MASTER,
	PRIV_ADMIN,
	PRIV_HOST,
	PRIV_MAX
};

#define _COMMANDN(name, fun, nargs, context) \
	static bool __dummy_##fun = addcommand(#name, (void (*)())fun, nargs, context)
#define COMMANDN(name, fun, nargs) _COMMANDN(name, fun, nargs, IDC_GLOBAL)
#define COMMAND(name, nargs) _COMMANDN(name, name, nargs, IDC_GLOBAL)
#define COMMANDWN(name, fun, nargs) _COMMANDN(name, fun, nargs, IDC_WORLD)
#define COMMANDW(name, nargs) _COMMANDN(name, name, nargs, IDC_WORLD)
#define _VAR(name, global, min, cur, max, context) \
	int global = variable(#name, min, cur, max, &global, NULL, context)
#define VARN(name, global, min, cur, max) _VAR(name, global, min, cur, max, IDC_GLOBAL)
#define VARNP(name, global, min, cur, max) _VAR(name, global, min, cur, max, IDC_GLOBAL|IDC_PERSIST)
#define VAR(name, min, cur, max) _VAR(name, name, min, cur, max, IDC_GLOBAL)
#define VARP(name, min, cur, max) _VAR(name, name, min, cur, max, IDC_GLOBAL|IDC_PERSIST)
#define VARW(name, min, cur, max) _VAR(name, name, min, cur, max, IDC_WORLD)
#define _VARF(name, global, min, cur, max, body, context) \
	void var_##name(); \
	int global = variable(#name, min, cur, max, &global, var_##name, context); \
	void var_##name() { body; }
#define VARFN(name, global, min, cur, max, body) _VARF(name, global, min, cur, max, body, IDC_GLOBAL)
#define VARF(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, IDC_GLOBAL)
#define VARFP(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, IDC_GLOBAL|IDC_PERSIST)
#define VARFW(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, IDC_WORLD)

#define _COMMAND(idtype, tv, n, g, proto, b, context) \
    struct cmd_##n : ident \
    { \
        cmd_##n(void *self = NULL) : ident(idtype, #n, g, (void *)run, self, context) \
        { \
            addident(_name, this); \
        } \
        static void run proto { b; } \
    } icom_##n tv
#define ICOMMAND(n, g, proto, b) _COMMAND(ID_COMMAND, , n, g, proto, b, IDC_GLOBAL)
#define ICOMMANDW(n, g, proto, b) _COMMAND(ID_COMMAND, , n, g, proto, b, IDC_WORLD)
#define CCOMMAND(n, g, proto, b) _COMMAND(ID_CCOMMAND, (this), n, g, proto, b, IDC_GLOBAL)
#define CCOMMANDS(n, g, proto, b) _COMMAND(ID_CCOMMAND, (this), n, g, proto, b, IDC_SERVER)
#define CCOMMANDW(n, g, proto, b) _COMMAND(ID_CCOMMAND, (this), n, g, proto, b, IDC_WORLD)

#define _IVAR(n, m, c, x, b, context) \
	struct var_##n : ident \
	{ \
		var_##n() : ident(ID_VAR, #n, m, c, x, &_val, NULL, context) \
		{ \
			addident(_name, this); \
		} \
		b; \
	} n;
#define IVAR(n, m, c, x)  _IVAR(n, m, c, x, , IDC_GLOBAL)
#define IVARF(n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, IDC_GLOBAL)
#define IVARP(n, m, c, x)  _IVAR(n, m, c, x, , IDC_GLOBAL|IDC_PERSIST)
#define IVARW(n, m, c, x)  _IVAR(n, m, c, x, , IDC_WORLD)
#define IVARFP(n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, IDC_GLOBAL|IDC_PERSIST)
#define IVARFW(n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, IDC_WORLD)
