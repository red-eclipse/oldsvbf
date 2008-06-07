// command.cpp: implements the parsing and execution of a tiny script language which
// is largely backwards compatible with the quake console language.

#include "pch.h"
#include "engine.h"

void itoa(char *s, int i) { s_sprintf(s)("%d", i); }
char *exchangestr(char *o, const char *n) { delete[] o; return newstring(n); }

identtable *idents = NULL;		// contains ALL vars/commands/aliases

bool overrideidents = false, persistidents = true, worldidents = false, interactive = false;

void clearstack(ident &id)
{
    identstack *stack = id.stack;
	while(stack)
	{
		delete[] stack->action;
		identstack *tmp = stack;
		stack = stack->next;
		delete tmp;
	}
    id.stack = NULL;
}

void clear_command()
{
    enumerate(*idents, ident, i, if(i.type==ID_ALIAS) { DELETEA(i.name); DELETEA(i.action); if(i.stack) clearstack(i); });
	if(idents) idents->clear();
}

void clearoverrides()
{
	enumerate(*idents, ident, i,
        if(i.override!=NO_OVERRIDE)
		{
            switch(i.type)
			{
				case ID_ALIAS:
                    if(i.action[0]) i.action = exchangestr(i.action, "");
					break;
				case ID_VAR:
                    *i.storage.i = i.overrideval.i;
                    i.changed();
                    break;
                case ID_FVAR:
                    *i.storage.f = i.overrideval.f;
                    i.changed();
                    break;
                case ID_SVAR:
                    delete[] *i.storage.s;
                    *i.storage.s = i.overrideval.s;
					i.changed();
					break;
			}
            i.override = NO_OVERRIDE;
		});
}

void pushident(ident &id, char *val)
{
    if(id.type != ID_ALIAS) return;
	identstack *stack = new identstack;
    stack->action = id.isexecuting==id.action ? newstring(id.action) : id.action;
    stack->next = id.stack;
    id.stack = stack;
    id.action = val;
}

void popident(ident &id)
{
    if(id.type != ID_ALIAS || !id.stack) return;
    if(id.action != id.isexecuting) delete[] id.action;
    identstack *stack = id.stack;
    id.action = stack->action;
    id.stack = stack->next;
	delete stack;
}

ident *newident(const char *name)
{
	ident *id = idents->access(name);
	if(!id)
	{
        ident init(ID_ALIAS, newstring(name), newstring(""), persistidents, worldidents);
        id = idents->access(init.name, &init);
    }
    return id;
	}

void pusha(const char *name, char *action)
{
    pushident(*newident(name), action);
}

void push(char *name, char *action)
{
	pusha(name, newstring(action));
}

void pop(char *name)
{
	ident *id = idents->access(name);
	if(id) popident(*id);
}

COMMAND(push, "ss");
COMMAND(pop, "s");

void aliasa(const char *name, char *action)
{
	ident *b = idents->access(name);
	if(!b)
	{
        ident a(ID_ALIAS, newstring(name), action, persistidents, worldidents);
        if(overrideidents) a.override = OVERRIDDEN;
        b = idents->access(a.name, &a);
	}
    else if(b->type != ID_ALIAS)
	{
		conoutf("cannot redefine builtin %s with an alias", name);
		delete[] action;
	}
	else
	{
        if(b->action != b->isexecuting) delete[] b->action;
        b->action = action;
        if(overrideidents) b->override = OVERRIDDEN;
		else
		{
            if(b->override != NO_OVERRIDE) b->override = NO_OVERRIDE;
            if(b->persist != persistidents) b->persist = persistidents;
            if(b->world != worldidents && worldidents) b->world = worldidents;
		}
	}
#ifndef STANDALONE
	if(b && cl) cl->editvar(b, interactive);
#endif
}

void alias(const char *name, const char *action) { aliasa(name, newstring(action)); }

COMMAND(alias, "ss");

void worldalias(const char *name, const char *action)
{
	overrideidents = worldidents = true;
	persistidents = false;
	alias(name, action);
	persistidents = true;
	overrideidents = worldidents = false;
}
COMMAND(worldalias, "ss");

// variable's and commands are registered through globals, see cube.h

int variable(const char *name, int min, int cur, int max, int *storage, void (*fun)(), bool persist, bool world)
{
    if(!idents) idents = new identtable;
    ident v(ID_VAR, name, min, cur, max, storage, (void *)fun, persist, world);
    idents->access(name, &v);
    return cur;
}

float fvariable(const char *name, float cur, float *storage, void (*fun)(), bool persist, bool world)
{
    if(!idents) idents = new identtable;
    ident v(ID_FVAR, name, cur, storage, (void *)fun, persist, world);
    idents->access(name, &v);
    return cur;
}

char *svariable(const char *name, const char *cur, char **storage, void (*fun)(), bool persist, bool world)
{
    if(!idents) idents = new identtable;
    ident v(ID_SVAR, name, newstring(cur), storage, (void *)fun, persist, world);
    idents->access(name, &v);
    return v.val.s;
}

#define GETVAR(id, vartype, name, retval) \
	ident *id = idents->access(name); \
    if(!id || id->type!=vartype) return retval;
void setvar(const char *name, int i, bool dofunc)
{
	GETVAR(id, ID_VAR, name, );
    *id->storage.i = clamp(i, id->minval, id->maxval);
    if(dofunc) id->changed();
	if(verbose >= 3) conoutf("%s set to %d", id->name, *id->storage.i);
}
void setfvar(const char *name, float f, bool dofunc)
{
    GETVAR(id, ID_FVAR, name, );
    *id->storage.f = f;
    if(dofunc) id->changed();
	if(verbose >= 3) conoutf("%s set to %f", id->name, *id->storage.f);
}
void setsvar(const char *name, const char *str, bool dofunc)
{
    GETVAR(id, ID_SVAR, name, );
    *id->storage.s = exchangestr(*id->storage.s, str);
	if(dofunc) id->changed();
	if(verbose >= 3) conoutf("%s set to %s", id->name, *id->storage.s);
}
int getvar(const char *name)
{
	GETVAR(id, ID_VAR, name, 0);
    return *id->storage.i;
}
int getvarmin(const char *name)
{
	GETVAR(id, ID_VAR, name, 0);
    return id->minval;
}
int getvarmax(const char *name)
{
	GETVAR(id, ID_VAR, name, 0);
    return id->maxval;
}
bool identexists(const char *name) { return idents->access(name)!=NULL; }
ident *getident(const char *name) { return idents->access(name); }

const char *getalias(const char *name)
{
	ident *i = idents->access(name);
    return i && i->type==ID_ALIAS ? i->action : "";
}

bool addcommand(const char *name, void (*fun)(), const char *narg)
{
	if(!idents) idents = new identtable;
    ident c(ID_COMMAND, name, narg, (void *)fun);
	idents->access(name, &c);
	return false;
}

void addident(const char *name, ident *id)
{
	if(!idents) idents = new identtable;
	idents->access(name, id);
}

static vector<vector<char> *> wordbufs;
static int bufnest = 0;

char *parseexp(const char *&p, int right);

void parsemacro(const char *&p, int level, vector<char> &wordbuf)
{
	int escape = 1;
	while(*p=='@') p++, escape++;
	if(level > escape)
	{
		while(escape--) wordbuf.add('@');
		return;
	}
	if(*p=='(')
	{
		char *ret = parseexp(p, ')');
		if(ret)
		{
			for(char *sub = ret; *sub; ) wordbuf.add(*sub++);
			delete[] ret;
		}
		return;
	}
    static vector<char> ident;
    ident.setsizenodelete(0);
    while(isalnum(*p) || *p=='_') ident.add(*p++);
    ident.add(0);
    const char *alias = getalias(ident.getbuf());
	while(*alias) wordbuf.add(*alias++);
}

char *parseexp(const char *&p, int right)          // parse any nested set of () or []
{
	if(bufnest++>=wordbufs.length()) wordbufs.add(new vector<char>);
	vector<char> &wordbuf = *wordbufs[bufnest-1];
	int left = *p++;
	for(int brak = 1; brak; )
	{
		int c = *p++;
		if(c=='\r') continue;				// hack
		if(left=='[' && c=='@')
		{
			parsemacro(p, brak, wordbuf);
			continue;
		}
		if(c=='\"')
		{
			wordbuf.add(c);
            const char *end = p+strcspn(p, "\"\r\n\0");
			while(p < end) wordbuf.add(*p++);
			if(*p=='\"') wordbuf.add(*p++);
			continue;
		}
		if(c=='/' && *p=='/')
		{
			p += strcspn(p, "\n\0");
			continue;
		}

		if(c==left) brak++;
		else if(c==right) brak--;
		else if(!c)
		{
			p--;
			conoutf("missing \"%c\"", right);
			wordbuf.setsize(0);
			bufnest--;
			return NULL;
		}
		wordbuf.add(c);
	}
	wordbuf.pop();
	char *s;
	if(left=='(')
	{
		wordbuf.add(0);
		char *ret = executeret(wordbuf.getbuf());					// evaluate () exps directly, and substitute result
		wordbuf.pop();
		s = ret ? ret : newstring("");
	}
	else
	{
		s = newstring(wordbuf.getbuf(), wordbuf.length());
	}
	wordbuf.setsize(0);
	bufnest--;
	return s;
}

char *lookup(char *n)							// find value of ident referenced with $ in exp
{
	ident *id = idents->access(n+1);
    if(id) switch(id->type)
	{
        case ID_VAR: { s_sprintfd(t)("%d", *id->storage.i); return exchangestr(n, t); }
        case ID_FVAR: { s_sprintfd(t)("%f", *id->storage.f); return exchangestr(n, t); }
        case ID_SVAR: return exchangestr(n, *id->storage.s);
        case ID_ALIAS: return exchangestr(n, id->action);
	}
	conoutf("unknown alias lookup: %s", n+1);
	return n;
}

char *parseword(const char *&p)                       // parse single argument, including expressions
{
	for(;;)
	{
        p += strspn(p, " \t\r");
		if(p[0]!='/' || p[1]!='/') break;
        p += strcspn(p, "\n\0");
	}
	if(*p=='\"')
	{
		p++;
		const char *word = p;
        p += strcspn(p, "\"\r\n\0");
		char *s = newstring(word, p-word);
		if(*p=='\"') p++;
		return s;
	}
    if(*p=='(') return parseexp(p, ')');
    if(*p=='[') return parseexp(p, ']');
	const char *word = p;
	for(;;)
	{
        p += strcspn(p, "/; \t\r\n\0");
		if(p[0]!='/' || p[1]=='/') break;
		else if(p[1]=='\0') { p++; break; }
		p += 2;
	}
	if(p-word==0) return NULL;
	char *s = newstring(word, p-word);
    if(*s=='$') return lookup(s);               // substitute variables
	return s;
}

char *conc(char **w, int n, bool space)
{
	int len = space ? max(n-1, 0) : 0;
	loopj(n) len += (int)strlen(w[j]);
	char *r = newstring("", len);
	loopi(n)
	{
		strcat(r, w[i]);  // make string-list out of all arguments
		if(i==n-1) break;
		if(space) strcat(r, " ");
	}
	return r;
}

VARN(numargs, _numargs, 0, 0, 25);

#define parseint(s) strtol((s), NULL, 0)

char *commandret = NULL;

char *executeret(const char *p)               // all evaluation happens here, recursively
{
	const int MAXWORDS = 25;					// limit, remove
	char *w[MAXWORDS];
	char *retval = NULL;
    #define setretval(v) { char *rv = v; if(rv) retval = rv; }
	for(bool cont = true; cont;)				// for each ; seperated statement
	{
		int numargs = MAXWORDS;
		loopi(MAXWORDS)						 // collect all argument values
		{
            w[i] = (char *)"";
			if(i>numargs) continue;
            char *s = parseword(p);             // parse and evaluate exps
			if(s) w[i] = s;
			else numargs = i;
		}

        p += strcspn(p, ";\n\0");
		cont = *p++!=0;						 // more statements if this isn't the end of the string
		char *c = w[0];
		if(!*c) continue;						// empty statement

		DELETEA(retval);

		if(w[1][0]=='=' && !w[1][1])
		{
			aliasa(c, numargs>2 ? w[2] : newstring(""));
			w[2] = NULL;
		}
		else
		{
			ident *id = idents->access(c);
            if(!id)
			{
                if(!isdigit(*c) && ((*c!='+' && *c!='-' && *c!='.') || !isdigit(c[1])))
                    conoutf("unknown command: %s", c);
				setretval(newstring(c));
			}
            else switch(id->type)
			{
                case ID_CCOMMAND:
				case ID_COMMAND:					 // game defined commands
				{
					void *v[MAXWORDS];
					union
					{
						int i;
						float f;
					} nstor[MAXWORDS];
					int n = 0, wn = 0;
                    char *cargs = NULL;
                    if(id->type==ID_CCOMMAND) v[n++] = id->self;
                    for(const char *a = id->narg; *a; a++) switch(*a)
					{
						case 's':								 v[n] = w[++wn];	 n++; break;
						case 'i': nstor[n].i = parseint(w[++wn]); v[n] = &nstor[n].i; n++; break;
						case 'f': nstor[n].f = atof(w[++wn]);	 v[n] = &nstor[n].f; n++; break;
#ifndef STANDALONE
						case 'D': nstor[n].i = addreleaseaction(id->name) ? 1 : 0; v[n] = &nstor[n].i; n++; break;
#endif
						case 'V': v[n++] = w+1; nstor[n].i = numargs-1; v[n] = &nstor[n].i; n++; break;
                        case 'C': if(!cargs) cargs = conc(w+1, numargs-1, true); v[n++] = cargs; break;
						default: fatal("builtin declared with illegal type");
					}
					switch(n)
					{
                        case 0: ((void (__cdecl *)()                                      )id->fun)();                             break;
                        case 1: ((void (__cdecl *)(void *)                                )id->fun)(v[0]);                         break;
                        case 2: ((void (__cdecl *)(void *, void *)                        )id->fun)(v[0], v[1]);                   break;
                        case 3: ((void (__cdecl *)(void *, void *, void *)                )id->fun)(v[0], v[1], v[2]);             break;
                        case 4: ((void (__cdecl *)(void *, void *, void *, void *)        )id->fun)(v[0], v[1], v[2], v[3]);       break;
                        case 5: ((void (__cdecl *)(void *, void *, void *, void *, void *))id->fun)(v[0], v[1], v[2], v[3], v[4]); break;
                        case 6: ((void (__cdecl *)(void *, void *, void *, void *, void *, void *))id->fun)(v[0], v[1], v[2], v[3], v[4], v[5]); break;
                        case 7: ((void (__cdecl *)(void *, void *, void *, void *, void *, void *, void *))id->fun)(v[0], v[1], v[2], v[3], v[4], v[5], v[6]); break;
                        case 8: ((void (__cdecl *)(void *, void *, void *, void *, void *, void *, void *, void *))id->fun)(v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]); break;
						default: fatal("builtin declared with too many args (use V?)");
					}
                    if(cargs) delete[] cargs;
					setretval(commandret);
                    commandret = NULL;
					break;
				}

				case ID_VAR:						// game defined variables
                    if(numargs <= 1) conoutf("%s = %d", c, *id->storage.i);      // var with no value just prints its current value
                    else if(id->minval>id->maxval) conoutf("variable %s is read-only", id->name);
					else
					{
#ifndef STANDALONE
						#define WORLDVAR \
							if (!worldidents && !editmode && id->world) \
							{ \
								conoutf("cannot set world variable %s outside editmode", id->name); \
								break; \
							}
#endif

                        #define OVERRIDEVAR(saveval, resetval) \
                            if(overrideidents) \
                            { \
                                if(id->persist && !id->world) \
                                { \
                                    conoutf("cannot override persistent variable %s", id->name); \
                                    break; \
                                } \
                                if(id->override==NO_OVERRIDE) { saveval; id->override = OVERRIDDEN; } \
                            } \
                            else if(id->override!=NO_OVERRIDE) { resetval; id->override = NO_OVERRIDE; }

#ifndef STANDALONE
						WORLDVAR;
#endif
                        OVERRIDEVAR(id->overrideval.i = *id->storage.i, )
                        int i1 = parseint(w[1]);
                        if(i1<id->minval || i1>id->maxval)
						{
                            i1 = i1<id->minval ? id->minval : id->maxval;                // clamp to valid range
                            conoutf("valid range for %s is %d..%d", id->name, id->minval, id->maxval);
						}
                        *id->storage.i = i1;
                        id->changed();                                             // call trigger function if available
#ifndef STANDALONE
						if(cl) cl->editvar(id, interactive);
#endif
					}
                    break;

                case ID_FVAR:
                    if(numargs <= 1) conoutf("%s = %f", c, *id->storage.f);
                    else
                    {
#ifndef STANDALONE
						WORLDVAR;
#endif
                        OVERRIDEVAR(id->overrideval.f = *id->storage.f, );
                        *id->storage.f = atof(w[1]);
                        id->changed();
#ifndef STANDALONE
						if(cl) cl->editvar(id, interactive);
#endif
                    }
                    break;

                case ID_SVAR:
                    if(numargs <= 1) conoutf(strchr(*id->storage.s, '"') ? "%s = [%s]" : "%s = \"%s\"", c, *id->storage.s);
                    else
					{
#ifndef STANDALONE
						WORLDVAR;
#endif
                        OVERRIDEVAR(id->overrideval.s = *id->storage.s, delete[] id->overrideval.s);
                        *id->storage.s = newstring(w[1]);
                        id->changed();
#ifndef STANDALONE
						if(cl) cl->editvar(id, interactive);
#endif
					}
					break;

				case ID_ALIAS:							  // alias, also used as functions and (global) variables
				{
                    static vector<ident *> argids;
					for(int i = 1; i<numargs; i++)
					{
                        if(i > argids.length())
                        {
                            s_sprintfd(argname)("arg%d", i);
                            argids.add(newident(argname));
                        }
                        pushident(*argids[i-1], w[i]); // set any arguments as (global) arg values so functions can access them
					}
					_numargs = numargs-1;
					bool wasoverriding = overrideidents;
                    if(id->override!=NO_OVERRIDE) overrideidents = true;
                    char *wasexecuting = id->isexecuting;
                    id->isexecuting = id->action;
                    setretval(executeret(id->action));
                    if(id->isexecuting != id->action && id->isexecuting != wasexecuting) delete[] id->isexecuting;
                    id->isexecuting = wasexecuting;
					overrideidents = wasoverriding;
                    for(int i = 1; i<numargs; i++) popident(*argids[i-1]);
                    continue;
				}
			}
		}
		loopj(numargs) if(w[j]) delete[] w[j];
	}
	return retval;
}

int execute(const char *p)
{
    char *ret = executeret(p);
	int i = 0;
	if(ret) { i = parseint(ret); delete[] ret; }
	return i;
}

bool execfile(const char *cfgfile)
{
	string s;
	s_strcpy(s, cfgfile);
	char *buf = loadfile(s, NULL);
	if(!buf) return false;
	execute(buf);
	delete[] buf;
	if (verbose >= 3) conoutf("loaded script '%s'", cfgfile);
	return true;
}

void exec(const char *cfgfile)
{
	if(!execfile(cfgfile)) conoutf("could not read \"%s\"", cfgfile);
}

void writecfg()
{
#ifndef STANDALONE
	FILE *f = openfile("config.cfg", "w");
	if(!f) return;
	cc->writeclientinfo(f);
	fprintf(f, "if (&& (= $version %d) (= (gamever) %d)) [\n\n", ENG_VERSION, sv->gamever());
#else
	FILE *f = openfile("server.cfg", "w");
	if(!f) return;
#endif
	enumerate(*idents, ident, id,
        if(!id.persist || id.world) continue;
        switch(id.type)
		{
            case ID_VAR: fprintf(f, "%s %d\n", id.name, *id.storage.i); break;
            case ID_FVAR: fprintf(f, "%s %f\n", id.name, *id.storage.f); break;
            case ID_SVAR: fprintf(f, "%s [%s]\n", id.name, *id.storage.s); break;
			case ID_ALIAS:
			{
				if(id.override==NO_OVERRIDE && !strstr(id.name, "nextmap_") && id.action[0])
					fprintf(f, "\"%s\" = [%s]\n", id.name, id.action);
				break;
			}
		}
	);
#ifndef STANDALONE
	writebinds(f);
	writecompletions(f);
	fprintf(f, "] [ echo \"WARNING: config from different version ignored\" ]\n");
#endif
	fclose(f);
}

COMMAND(writecfg, "");

// below the commands that implement a small imperative language. thanks to the semantics of
// () and [] expressions, any control construct can be defined trivially.

void intret(int v) { s_sprintfd(b)("%d", v); commandret = newstring(b); }

ICOMMAND(if, "sss", (char *cond, char *t, char *f), commandret = executeret(cond[0]!='0' ? t : f));
ICOMMAND(loop, "sis", (char *var, int *n, char *body),
{
    if(*n<=0) return;
    ident *id = newident(var);
    if(id->type!=ID_ALIAS) return;
    loopi(*n)
    {
        if(i) sprintf(id->action, "%d", i);
        else pushident(*id, newstring("0", 16));
        execute(body);
    }
    popident(*id);
});
ICOMMAND(while, "ss", (char *cond, char *body), while(execute(cond)) execute(body));    // can't get any simpler than this :)

void concat(const char *s) { commandret = newstring(s); }
void result(const char *s) { commandret = newstring(s); }

void concatword(char **args, int *numargs)
{
	commandret = conc(args, *numargs, false);
}

void format(char **args, int *numargs)
{
	vector<char> s;
	char *f = args[0];
	while(*f)
	{
		int c = *f++;
		if(c == '%')
		{
			int i = *f++;
			if(i >= '1' && i <= '9')
			{
				i -= '0';
				const char *sub = i < *numargs ? args[i] : "";
				while(*sub) s.add(*sub++);
			}
			else s.add(i);
		}
		else s.add(c);
	}
	s.add('\0');
	result(s.getbuf());
}

#define whitespaceskip s += strspn(s, "\n\t ")
#define elementskip *s=='"' ? (++s, s += strcspn(s, "\"\n\0"), s += *s=='"') : s += strcspn(s, "\n\t \0")

void explodelist(const char *s, vector<char *> &elems)
{
    whitespaceskip;
    while(*s)
    {
        const char *elem = s;
        elementskip;
        elems.add(*elem=='"' ? newstring(elem+1, s-elem-(s[-1]=='"' ? 2 : 1)) : newstring(elem, s-elem));
        whitespaceskip;
    }
}

char *indexlist(const char *s, int pos)
{
    whitespaceskip;
    loopi(pos) elementskip, whitespaceskip;
    const char *e = s;
    elementskip;
    if(*e=='"')
    {
        e++;
        if(s[-1]=='"') --s;
    }
    return newstring(e, s-e);
}

void listlen(char *s)
{
	int n = 0;
	whitespaceskip;
	for(; *s; n++) elementskip, whitespaceskip;
	intret(n);
}

void at(char *s, int *pos)
{
    commandret = indexlist(s, *pos);
}

void substr(char *s, int *start, int *count)
{
    int len = strlen(s), offset = clamp(*start, 0, len);
    commandret = newstring(&s[offset], *count <= 0 ? len - offset : min(*count, len - offset));
}

void getalias_(char *s)
{
	result(getalias(s));
}

COMMAND(exec, "s");
COMMAND(concat, "C");
COMMAND(result, "s");
COMMAND(concatword, "V");
COMMAND(format, "V");
COMMAND(at, "si");
COMMAND(substr, "sii");
COMMAND(listlen, "s");
COMMANDN(getalias, getalias_, "s");

void add  (int *a, int *b) { intret(*a + *b); }		  COMMANDN(+, add, "ii");
void mul  (int *a, int *b) { intret(*a * *b); }		  COMMANDN(*, mul, "ii");
void sub  (int *a, int *b) { intret(*a - *b); }		  COMMANDN(-, sub, "ii");
void divi (int *a, int *b) { intret(*b ? *a / *b : 0); } COMMANDN(div, divi, "ii");
void mod  (int *a, int *b) { intret(*b ? *a % *b : 0); } COMMAND(mod, "ii");
void equal(int *a, int *b) { intret((int)(*a == *b)); }  COMMANDN(=, equal, "ii");
void nequal(int *a, int *b) { intret((int)(*a != *b)); } COMMANDN(!=, nequal, "ii");
void lt	(int *a, int *b) { intret((int)(*a < *b)); }	COMMANDN(<, lt, "ii");
void gt	(int *a, int *b) { intret((int)(*a > *b)); }	COMMANDN(>, gt, "ii");
void lte	(int *a, int *b) { intret((int)(*a <= *b)); } COMMANDN(<=, lte, "ii");
void gte	(int *a, int *b) { intret((int)(*a >= *b)); } COMMANDN(>=, gte, "ii");
void xora (int *a, int *b) { intret(*a ^ *b); }		  COMMANDN(^, xora, "ii");
void nota (int *a)		 { intret(*a == 0); }		  COMMANDN(!, nota, "i");
void mina (int *a, int *b) { intret(min(*a, *b)); }      COMMANDN(min, mina, "ii");
void maxa (int *a, int *b) { intret(max(*a, *b)); }      COMMANDN(max, maxa, "ii");

void anda (char *a, char *b) { intret(execute(a)!=0 && execute(b)!=0); }
void ora  (char *a, char *b) { intret(execute(a)!=0 || execute(b)!=0); }

COMMANDN(&&, anda, "ss");
COMMANDN(||, ora, "ss");

void rndn(int *a)		  { intret(*a>0 ? rnd(*a) : 0); }  COMMANDN(rnd, rndn, "i");

void strcmpa(char *a, char *b) { intret(strcmp(a,b)==0); }  COMMANDN(strcmp, strcmpa, "ss");

ICOMMAND(echo, "C", (char *s), conoutf("\fs\fw%s\fS", s));

void strstra(char *a, char *b) { char *s = strstr(a, b); intret(s ? s-a : -1); } COMMANDN(strstr, strstra, "ss");

char *strreplace(const char *s, const char *oldval, const char *newval)
{
    vector<char> buf;

    int oldlen = strlen(oldval);
    for(;;)
    {
        const char *found = strstr(s, oldval);
        if(found)
        {
            while(s < found) buf.add(*s++);
            for(const char *n = newval; *n; n++) buf.add(*n);
            s = found + oldlen;
        }
        else
        {
            while(*s) buf.add(*s++);
            buf.add('\0');
            return newstring(buf.getbuf(), buf.length());
        }
    }
}

void strreplacea(char *s, char *o, char *n) { commandret = strreplace(s, o, n); } COMMANDN(strreplace, strreplacea, "sss");

struct sleepcmd
{
	int millis;
	char *command;
    bool override, world;
};
vector<sleepcmd> sleepcmds;

void addsleep(int *msec, char *cmd)
{
	sleepcmd &s = sleepcmds.add();
	s.millis = *msec+lastmillis;
	s.command = newstring(cmd);
    s.override = overrideidents;
    s.world = worldidents;
}

COMMANDN(sleep, addsleep, "is");

void checksleep(int millis)
{
	loopv(sleepcmds)
	{
		sleepcmd &s = sleepcmds[i];
		if(s.millis && millis>s.millis)
		{
			char *cmd = s.command; // execute might create more sleep commands
			execute(cmd);
			delete[] cmd;
			sleepcmds.remove(i--);
		}
	}
}

void clearsleep(bool clearoverrides, bool clearworlds)
{
    int len = 0;
    loopv(sleepcmds)
    {
        if(clearoverrides && !sleepcmds[i].override) sleepcmds[len++] = sleepcmds[i];
        else delete[] sleepcmds[i].command;
    }
    sleepcmds.setsize(len);
}

void clearsleep_(int *clearoverrides, int *clearworlds)
{
    clearsleep(*clearoverrides!=0 || overrideidents, *clearworlds!=0 || worldidents);
}

COMMANDN(clearsleep, clearsleep_, "ii");

ICOMMAND(exists, "ss", (char *a, char *b), intret(fileexists(a, *b ? b : "r")));

string _gettime;
char *gettime(char *format)
{
	time_t ltime;
	struct tm *t;

	ltime = time (NULL);
	t = localtime (&ltime);

	strftime (_gettime, sizeof (_gettime) - 1, format, t);

	return _gettime;
}
ICOMMAND(gettime, "s", (char *a), result(gettime(a)));

int gzgetint(gzFile f)
{
	int t;
	gzread(f, &t, sizeof(int));
	endianswap(&t, sizeof(int), 1);
	return t;
}

void gzputint(gzFile f, int x)
{
	int t = (int)x;
	endianswap(&t, sizeof(int), 1);
	gzwrite(f, &t, sizeof(int));
}

float gzgetfloat(gzFile f)
{
	float t;
	gzread(f, &t, sizeof(float));
	endianswap(&t, sizeof(float), 1);
	return t;
}

void gzputfloat(gzFile f, float x)
{
	float t = (float)x;
	endianswap(&t, sizeof(float), 1);
	gzwrite(f, &t, sizeof(float));
}
