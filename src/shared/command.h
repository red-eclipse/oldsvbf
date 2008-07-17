// script binding functionality


enum { ID_VAR, ID_FVAR, ID_SVAR, ID_COMMAND, ID_CCOMMAND, ID_ALIAS };

enum { NO_OVERRIDE = INT_MAX, OVERRIDDEN = 0 };

enum { IDF_PERSIST = 1<<0, IDF_OVERRIDE = 1<<1, IDF_WORLD = 1<<2, IDF_COMPLETE = 1<<3, IDF_TEXTURE = 1<<4 };

struct identstack
{
    char *action;
    identstack *next;
};

union identval
{
    int i;      // ID_VAR
    float f;    // ID_FVAR
    char *s;    // ID_SVAR
};

union identvalptr
{
    int *i;   // ID_VAR
    float *f; // ID_FVAR
    char **s; // ID_SVAR
};

struct ident
{
    int type;           // one of ID_* above
    const char *name;
    int minval, maxval; // ID_VAR
    int override; // either NO_OVERRIDE, OVERRIDDEN, or value
    union
    {
        void (__cdecl *fun)(); // ID_VAR, ID_COMMAND, ID_CCOMMAND
        identstack *stack;     // ID_ALIAS
    };
    union
    {
        const char *narg; // ID_COMMAND, ID_CCOMMAND
        char *action;     // ID_ALIAS
        identval val;// ID_VAR, ID_FVAR, ID_SVAR
    };
    union
    {
        void *self;        // ID_COMMAND, ID_CCOMMAND
        char *isexecuting; // ID_ALIAS
        identval overrideval; // ID_VAR, ID_FVAR, ID_SVAR
    };
    identval def;
    identvalptr storage; // ID_VAR, ID_FVAR, ID_SVAR
    int flags;

    ident() {}
    // ID_VAR
    ident(int t, const char *n, int m, int c, int x, int *s, void *f = NULL, int flags = IDF_COMPLETE)
        : type(t), name(n), minval(m), maxval(x), override(NO_OVERRIDE), fun((void (__cdecl *)())f), flags(flags)
    { val.i = def.i = c; storage.i = s; }
    // ID_FVAR
    ident(int t, const char *n, float c, float *s, void *f = NULL, int flags = IDF_COMPLETE)
        : type(t), name(n), override(NO_OVERRIDE), fun((void (__cdecl *)())f), flags(flags)
    { val.f = def.f = c; storage.f = s; }
    // ID_SVAR
    ident(int t, const char *n, const char *c, char **s, void *f = NULL, int flags = IDF_COMPLETE)
        : type(t), name(n), override(NO_OVERRIDE), fun((void (__cdecl *)())f), flags(flags)
    { val.s = newstring(*c ? c : ""); def.s = newstring(*c ? c : ""); storage.s = s; }
    // ID_ALIAS
    ident(int t, const char *n, char *a, int flags)
        : type(t), name(n), override(NO_OVERRIDE), stack(NULL), action(a), flags(flags) {}
    // ID_COMMAND, ID_CCOMMAND
    ident(int t, const char *n, const char *narg, void *f = NULL, void *s = NULL, int flags = IDF_COMPLETE)
        : type(t), name(n), fun((void (__cdecl *)(void))f), narg(narg), self(s), flags(flags) {}

    virtual ~ident() {}

    ident &operator=(const ident &o) { memcpy(this, &o, sizeof(ident)); return *this; }        // force vtable copy, ugh

    virtual void changed() { if(fun) fun(); }
};

extern void addident(const char *name, ident *id);
extern void intret(int v);
extern void result(const char *s);

typedef hashtable<const char *, ident> identtable;
extern identtable *idents;

extern int variable(const char *name, int min, int cur, int max, int *storage, void (*fun)(), int flags);
extern float fvariable(const char *name, float cur, float *storage, void (*fun)(), int flags);
extern char *svariable(const char *name, const char *cur, char **storage, void (*fun)(), int flags);
extern void setvar(const char *name, int i, bool dofunc = false);
extern void setfvar(const char *name, float f, bool dofunc = false);
extern void setsvar(const char *name, const char *str, bool dofunc = false);
extern void touchvar(const char *name);
extern int getvar(const char *name);
extern int getvarmin(const char *name);
extern int getvarmax(const char *name);
extern bool identexists(const char *name);
extern ident *getident(const char *name);
extern bool addcommand(const char *name, void (*fun)(), const char *narg);
extern int execute(const char *p);
extern char *executeret(const char *p);
extern void exec(const char *cfgfile);
extern bool execfile(const char *cfgfile);
extern void alias(const char *name, const char *action);
extern void worldalias(const char *name, const char *action);
extern const char *getalias(const char *name);

extern bool overrideidents, persistidents, worldidents, interactive;

extern char *parseword(char *&p);
extern void explodelist(const char *s, vector<char *> &elems);

extern void clearoverrides();
extern void writecfg();

extern void checksleep(int millis);
extern void clearsleep(bool clearoverrides = true, bool clearworlds = false);

// nasty macros for registering script functions, abuses globals to avoid excessive infrastructure
#define COMMANDN(name, fun, nargs) static bool __dummy_##fun = addcommand(#name, (void (*)())fun, nargs)
#define COMMAND(name, nargs) COMMANDN(name, name, nargs)

#define _VAR(name, global, min, cur, max, persist)  int global = variable(#name, min, cur, max, &global, NULL, persist)
#define VARN(name, global, min, cur, max) _VAR(name, global, min, cur, max, IDF_COMPLETE)
#define VARNP(name, global, min, cur, max) _VAR(name, global, min, cur, max, IDF_PERSIST|IDF_COMPLETE)
#define VARNR(name, global, min, cur, max) _VAR(name, global, min, cur, max, IDF_OVERRIDE|IDF_COMPLETE)
#define VARNW(name, global, min, cur, max) _VAR(name, global, min, cur, max, IDF_WORLD|IDF_COMPLETE)
#define VAR(name, min, cur, max) _VAR(name, name, min, cur, max, IDF_COMPLETE)
#define VARP(name, min, cur, max) _VAR(name, name, min, cur, max, IDF_PERSIST|IDF_COMPLETE)
#define VARR(name, min, cur, max) _VAR(name, name, min, cur, max, IDF_OVERRIDE|IDF_COMPLETE)
#define VARW(name, min, cur, max) _VAR(name, name, min, cur, max, IDF_WORLD|IDF_COMPLETE)
#define _VARF(name, global, min, cur, max, body, persist)  void var_##name(); int global = variable(#name, min, cur, max, &global, var_##name, persist); void var_##name() { body; }
#define VARFN(name, global, min, cur, max, body) _VARF(name, global, min, cur, max, body, IDF_COMPLETE)
#define VARF(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, IDF_COMPLETE)
#define VARFP(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, IDF_PERSIST|IDF_COMPLETE)
#define VARFR(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, IDF_OVERRIDE|IDF_COMPLETE)
#define VARFW(name, min, cur, max, body) _VARF(name, name, min, cur, max, body, IDF_WORLD|IDF_COMPLETE)

#define _FVAR(name, global, cur, persist) float global = fvariable(#name, cur, &global, NULL, persist)
#define FVARN(name, global, cur) _FVAR(name, global, cur, IDF_COMPLETE)
#define FVARNP(name, global, cur) _FVAR(name, global, cur, IDF_PERSIST|IDF_COMPLETE)
#define FVARNR(name, global, cur) _FVAR(name, global, cur, IDF_OVERRIDE|IDF_COMPLETE)
#define FVARNW(name, global, cur) _FVAR(name, global, cur, IDF_WORLD|IDF_COMPLETE)
#define FVAR(name, cur) _FVAR(name, name, cur, IDF_COMPLETE)
#define FVARP(name, cur) _FVAR(name, name, cur, IDF_PERSIST|IDF_COMPLETE)
#define FVARR(name, cur) _FVAR(name, name, cur, IDF_OVERRIDE|IDF_COMPLETE)
#define FVARW(name, cur) _FVAR(name, name, cur, IDF_WORLD|IDF_COMPLETE)
#define _FVARF(name, global, cur, body, persist) void var_##name(); float global = fvariable(#name, cur, &global, var_##name, persist); void var_##name() { body; }
#define FVARFN(name, global, cur, body) _FVARF(name, global, cur, body, IDF_COMPLETE)
#define FVARF(name, cur, body) _FVARF(name, name, cur, body, IDF_COMPLETE)
#define FVARFP(name, cur, body) _FVARF(name, name, cur, body, IDF_PERSIST|IDF_COMPLETE)
#define FVARFR(name, cur, body) _FVARF(name, name, cur, body, IDF_OVERRIDE|IDF_COMPLETE)
#define FVARFW(name, cur, body) _FVARF(name, name, cur, body, IDF_WORLD|IDF_COMPLETE)

#define _SVAR(name, global, cur, persist) char *global = svariable(#name, cur, &global, NULL, persist)
#define SVARN(name, global, cur) _SVAR(name, global, cur, IDF_COMPLETE)
#define SVARNP(name, global, cur) _SVAR(name, global, cur, IDF_PERSIST|IDF_COMPLETE)
#define SVARNR(name, global, cur) _SVAR(name, global, cur, IDF_OVERRIDE|IDF_COMPLETE)
#define SVARNW(name, global, cur) _SVAR(name, global, cur, IDF_WORLD|IDF_COMPLETE)
#define SVAR(name, cur) _SVAR(name, name, cur, IDF_COMPLETE)
#define SVARP(name, cur) _SVAR(name, name, cur, IDF_PERSIST|IDF_COMPLETE)
#define SVARR(name, cur) _SVAR(name, name, cur, IDF_OVERRIDE|IDF_COMPLETE)
#define SVARW(name, cur) _SVAR(name, name, cur, IDF_WORLD|IDF_COMPLETE)
#define _SVARF(name, global, cur, body, persist) void var_##name(); char *global = svariable(#name, cur, &global, var_##name, persist); void var_##name() { body; }
#define SVARFN(name, global, cur, body) _SVARF(name, global, cur, body, IDF_COMPLETE)
#define SVARF(name, cur, body) _SVARF(name, name, cur, body, IDF_COMPLETE)
#define SVARFP(name, cur, body) _SVARF(name, name, cur, body, IDF_PERSIST|IDF_COMPLETE)
#define SVARFR(name, cur, body) _SVARF(name, name, cur, body, IDF_OVERRIDE|IDF_COMPLETE)
#define SVARFW(name, cur, body) _SVARF(name, name, cur, body, IDF_WORLD|IDF_COMPLETE)

// new style macros, have the body inline, and allow binds to happen anywhere, even inside class constructors, and access the surrounding class
#define _COMMAND(idtype, tv, n, g, proto, b) \
    struct cmd_##n : ident \
    { \
        cmd_##n(void *self = NULL) : ident(idtype, #n, g, (void *)run, self) \
        { \
            addident(name, this); \
        } \
        static void run proto { b; } \
    } icom_##n tv
#define ICOMMAND(n, g, proto, b) _COMMAND(ID_COMMAND, , n, g, proto, b)
#define CCOMMAND(n, g, proto, b) _COMMAND(ID_CCOMMAND, (this), n, g, proto, b)

#define _IVAR(n, m, c, x, b, p) \
	struct var_##n : ident \
	{ \
        var_##n() : ident(ID_VAR, #n, m, c, x, &val.i, NULL, p) \
		{ \
            addident(name, this); \
		} \
        int operator()() { return val.i; } \
        b \
    } n
#define IVAR(n, m, c, x)  _IVAR(n, m, c, x, , IDF_COMPLETE)
#define IVARF(n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, IDF_COMPLETE)
#define IVARP(n, m, c, x)  _IVAR(n, m, c, x, , IDF_PERSIST|IDF_COMPLETE)
#define IVARR(n, m, c, x)  _IVAR(n, m, c, x, , IDF_OVERRIDE|IDF_COMPLETE)
#define IVARW(n, m, c, x)  _IVAR(n, m, c, x, , IDF_WORLD|IDF_COMPLETE)
#define IVARFP(n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, IDF_PERSIST|IDF_COMPLETE)
#define IVARFR(n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, IDF_OVERRIDE|IDF_COMPLETE)
#define IVARFW(n, m, c, x, b) _IVAR(n, m, c, x, void changed() { b; }, IDF_WORLD|IDF_COMPLETE)

#define _IFVAR(n, c, b, p) \
	struct var_##n : ident \
	{ \
        var_##n() : ident(ID_FVAR, #n, c, &val.f, NULL, p) \
		{ \
            addident(name, this); \
		} \
        float operator()() { return val.f; } \
        b \
    } n
#define IFVAR(n, c)  _IFVAR(n, c, , IDF_COMPLETE)
#define IFVARF(n, c, b) _IFVAR(n, c, void changed() { b; }, IDF_COMPLETE)
#define IFVARP(n, c)  _IFVAR(n, c, , IDF_PERSIST|IDF_COMPLETE)
#define IFVARR(n, c)  _IFVAR(n, c, , IDF_OVERRIDE|IDF_COMPLETE)
#define IFVARW(n, c)  _IFVAR(n, c, , IDF_WORLD|IDF_COMPLETE)
#define IFVARFP(n, c, b) _IFVAR(n, c, void changed() { b; }, IDF_PERSIST|IDF_COMPLETE)
#define IFVARFR(n, c, b) _IFVAR(n, c, void changed() { b; }, IDF_OVERRIDE|IDF_COMPLETE)
#define IFVARFW(n, c, b) _IFVAR(n, c, void changed() { b; }, IDF_WORLD|IDF_COMPLETE)

#define _ISVAR(n, c, b, p) \
	struct var_##n : ident \
	{ \
        var_##n() : ident(ID_SVAR, #n, c, &val.s, NULL, p) \
		{ \
            addident(name, this); \
		} \
        char *operator()() { return val.s; } \
        b \
    } n
#define ISVAR(n, c)  _ISVAR(n, c, , IDF_COMPLETE)
#define ISVARF(n, c, b) _ISVAR(n, c, void changed() { b; }, IDF_COMPLETE)
#define ISVARP(n, c)  _ISVAR(n, c, , IDF_PERSIST|IDF_COMPLETE)
#define ISVARR(n, c)  _ISVAR(n, c, , IDF_OVERRIDE|IDF_COMPLETE)
#define ISVARW(n, c)  _ISVAR(n, c, , IDF_WORLD|IDF_COMPLETE)
#define ISVARFP(n, c, b) _ISVAR(n, c, void changed() { b; }, IDF_PERSIST|IDF_COMPLETE)
#define ISVARFR(n, c, b) _ISVAR(n, c, void changed() { b; }, IDF_OVERRIDE|IDF_COMPLETE)
#define ISVARFW(n, c, b) _ISVAR(n, c, void changed() { b; }, IDF_WORLD|IDF_COMPLETE)

#define RUNWORLD(n) { ident *wid = idents->access(n); if(wid && wid->action && wid->flags&IDF_WORLD) execute(wid->action); }
