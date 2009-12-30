// world.cpp: core map management stuff

#include "engine.h"

bfgz hdr;
int worldscale;

VAR(octaentsize, 0, 128, 1024);
VAR(entselradius, 0, 2, 10);

bool getentboundingbox(extentity &e, ivec &o, ivec &r)
{
	switch(e.type)
	{
		case ET_EMPTY:
			return false;
		case ET_MAPMODEL:
		{
			model *m = loadmodel(NULL, e.attrs[0]);
			if(m)
			{
				vec center, radius;
				m->boundbox(0, center, radius);
				rotatebb(center, radius, e.attrs[1]);
				o = e.o;
				o.add(center);
				r = radius;
				r.add(1);
				o.sub(r);
				r.mul(2);
				break;
			}
		}
		// invisible mapmodels use entselradius
		default:
			o = e.o;
			o.sub(entselradius);
			r.x = r.y = r.z = entselradius*2;
			break;
	}
	return true;
}

enum
{
    MODOE_ADD      = 1<<0,
    MODOE_UPDATEBB = 1<<1
};

void modifyoctaentity(int flags, int id, cube *c, const ivec &cor, int size, const ivec &bo, const ivec &br, int leafsize, vtxarray *lastva = NULL)
{
	loopoctabox(cor, size, bo, br)
	{
		ivec o(i, cor.x, cor.y, cor.z, size);
		vtxarray *va = c[i].ext && c[i].ext->va ? c[i].ext->va : lastva;
		if(c[i].children != NULL && size > leafsize)
			modifyoctaentity(flags, id, c[i].children, o, size>>1, bo, br, leafsize, va);
        else if(flags&MODOE_ADD)
		{
			if(!c[i].ext || !c[i].ext->ents) ext(c[i]).ents = new octaentities(o, size);
			octaentities &oe = *c[i].ext->ents;
			switch(entities::getents()[id]->type)
			{
				case ET_MAPMODEL:
					if(loadmodel(NULL, entities::getents()[id]->attrs[0]))
					{
                        if(va)
                        {
                            va->bbmin.x = -1;
                            if(oe.mapmodels.empty())
                            {
                                if(!va->mapmodels) va->mapmodels = new vector<octaentities *>;
                                va->mapmodels->add(&oe);
                            }
                        }
						oe.mapmodels.add(id);
						loopk(3)
						{
							oe.bbmin[k] = min(oe.bbmin[k], max(oe.o[k], bo[k]));
							oe.bbmax[k] = max(oe.bbmax[k], min(oe.o[k]+size, bo[k]+br[k]));
						}
						break;
					}
					// invisible mapmodel
				default:
					oe.other.add(id);
					break;
			}

		}
		else if(c[i].ext && c[i].ext->ents)
		{
			octaentities &oe = *c[i].ext->ents;
			switch(entities::getents()[id]->type)
			{
				case ET_MAPMODEL:
					if(loadmodel(NULL, entities::getents()[id]->attrs[0]))
					{
						oe.mapmodels.removeobj(id);
                        if(va)
                        {
                            va->bbmin.x = -1;
                            if(va->mapmodels && oe.mapmodels.empty())
                            {
                                va->mapmodels->removeobj(&oe);
                                if(va->mapmodels->empty()) DELETEP(va->mapmodels);
                            }
                        }
						oe.bbmin = oe.bbmax = oe.o;
						oe.bbmin.add(oe.size);
						loopvj(oe.mapmodels)
						{
							extentity &e = *entities::getents()[oe.mapmodels[j]];
							ivec eo, er;
							if(getentboundingbox(e, eo, er)) loopk(3)
							{
								oe.bbmin[k] = min(oe.bbmin[k], eo[k]);
								oe.bbmax[k] = max(oe.bbmax[k], eo[k]+er[k]);
							}
						}
						loopk(3)
						{
							oe.bbmin[k] = max(oe.bbmin[k], oe.o[k]);
							oe.bbmax[k] = min(oe.bbmax[k], oe.o[k]+size);
						}
						break;
					}
					// invisible mapmodel
				default:
					oe.other.removeobj(id);
					break;
			}
			if(oe.mapmodels.empty() && oe.other.empty())
				freeoctaentities(c[i]);
		}
		if(c[i].ext && c[i].ext->ents) c[i].ext->ents->query = NULL;
        if(va && va!=lastva)
        {
            if(lastva)
            {
                if(va->bbmin.x < 0) lastva->bbmin.x = -1;
            }
            else if(flags&MODOE_UPDATEBB) updatevabb(va);
        }
	}
}

static void modifyoctaent(int flags, int id)
{
    vector<extentity *> &ents = entities::getents();
    if(!ents.inrange(id)) return;
    ivec o, r;
    extentity &e = *ents[id];
    if((e.inoctanode!=0)==flags || !getentboundingbox(e, o, r)) return;

    int leafsize = octaentsize, limit = max(r.x, max(r.y, r.z));
    while(leafsize < limit) leafsize *= 2;
    int diff = ~(leafsize-1) & ((o.x^(o.x+r.x))|(o.y^(o.y+r.y))|(o.z^(o.z+r.z)));
    if(diff && (limit > octaentsize/2 || diff < leafsize*2)) leafsize *= 2;

    e.inoctanode = flags&MODOE_ADD ? 1 : 0;
    modifyoctaentity(flags, id, worldroot, ivec(0, 0, 0), hdr.worldsize>>1, o, r, leafsize);
    if(e.type == ET_LIGHT || e.type == ET_SUNLIGHT) clearlightcache(id);
    else if(flags&MODOE_ADD) lightent(e);
}

static inline void addentity(int id)    { modifyoctaent(MODOE_ADD|MODOE_UPDATEBB, id); }
static inline void removeentity(int id) { modifyoctaent(MODOE_UPDATEBB, id); }

void freeoctaentities(cube &c)
{
	if(!c.ext) return;
	if(entities::getents().length())
	{
		while(c.ext->ents && !c.ext->ents->mapmodels.empty()) removeentity(c.ext->ents->mapmodels.pop());
		while(c.ext->ents && !c.ext->ents->other.empty())	 removeentity(c.ext->ents->other.pop());
	}
	if(c.ext->ents)
	{
		delete c.ext->ents;
		c.ext->ents = NULL;
	}
}

void entitiesinoctanodes()
{
    vector<extentity *> &ents = entities::getents();
    loopv(ents) modifyoctaent(MODOE_ADD, i);
}

extern bool havesel, selectcorners;
int entlooplevel = 0;
int efocus = -1, enthover = -1, entorient = -1, oldhover = -1;
bool undonext = true;

VAR(entediting, 0, 0, 1);

bool noentedit()
{
	if(!editmode) { conoutft(CON_MESG, "\froperation only allowed in edit mode"); return true; }
	return !entediting;
}

bool pointinsel(selinfo &sel, vec &o)
{
	return(o.x <= sel.o.x+sel.s.x*sel.grid
		&& o.x >= sel.o.x
		&& o.y <= sel.o.y+sel.s.y*sel.grid
		&& o.y >= sel.o.y
		&& o.z <= sel.o.z+sel.s.z*sel.grid
		&& o.z >= sel.o.z);
}

vector<int> entgroup;

bool haveselent()
{
	return entgroup.length() > 0;
}

void entcancel()
{
	entgroup.setsize(0);
}

void entadd(int id)
{
	undonext = true;
	entgroup.add(id);
}

undoblock *newundoent()
{
    int numents = entgroup.length();
    if(numents <= 0) return NULL;
    undoblock *u = (undoblock *)new uchar[sizeof(undoblock) + numents*sizeof(undoent)];
    u->numents = numents;
    undoent *e = (undoent *)(u + 1);
    loopv(entgroup)
    {
        e->i = entgroup[i];
        e->type = entities::getents()[entgroup[i]]->type;
        e->o = entities::getents()[entgroup[i]]->o;
        loopj(UNDOATTRS) e->attrs[j] = entities::getents()[entgroup[i]]->attrs.inrange(j) ? entities::getents()[entgroup[i]]->attrs[j] : 0;
        e++;
    }
    return u;
}

void makeundoent()
{
    if(!undonext) return;
    undonext = false;
    if(!editmode) return;
    oldhover = enthover;
    undoblock *u = newundoent();
    if(u) addundo(u);
}

// convenience macros implicitly define:
// e		 entity, currently edited ent
// n		 int,	index to currently edited ent
#define addimplicit(f)  { if(entgroup.empty() && enthover>=0) { entadd(enthover); undonext = (enthover != oldhover); f; entgroup.drop(); } else f; }
#define entedit(i, f) \
{ \
	entfocus(i, \
	removeentity(n);  \
	f; \
	if(e.type!=ET_EMPTY) { addentity(n); } \
	entities::editent(n)); \
}
#define addgroup(exp)	{ loopv(entities::getents()) entfocus(i, if(exp) entadd(n)); }
#define setgroup(exp)	{ entcancel(); addgroup(exp); }
#define groupeditloop(f){ entlooplevel++; int _ = efocus; loopv(entgroup) entedit(entgroup[i], f); efocus = _; entlooplevel--; }
#define groupeditpure(f){ if(entlooplevel>0) { entedit(efocus, f); } else groupeditloop(f); }
#define groupeditundo(f){ makeundoent(); groupeditpure(f); }
#define groupedit(f)	{ addimplicit(groupeditundo(f)); }

undoblock *copyundoents(undoblock *u)
{
    entcancel();
    undoent *e = u->ents();
    loopi(u->numents)
        entadd(e[i].i);
    undoblock *c = newundoent();
    loopi(u->numents) if(e[i].type==ET_EMPTY)
        entgroup.removeobj(e[i].i);
    return c;
}

void pasteundoents(undoblock *u)
{
    undoent *ue = u->ents();
    loopi(u->numents)
        entedit(ue[i].i, { e.type = ue[i].type; e.o = ue[i].o; while(e.attrs.length() < UNDOATTRS) e.attrs.add(0); loopj(UNDOATTRS) e.attrs[j] = ue[i].attrs[j]; });
}

void entflip()
{
	if(noentedit()) return;
	int d = dimension(sel.orient);
	float mid = sel.s[d]*sel.grid/2+sel.o[d];
	groupeditundo(e.o[d] -= (e.o[d]-mid)*2);
}

void entrotate(int *cw)
{
	if(noentedit()) return;
	int d = dimension(sel.orient);
	int dd = (*cw<0) == dimcoord(sel.orient) ? R[d] : C[d];
	float mid = sel.s[dd]*sel.grid/2+sel.o[dd];
	vec s(sel.o.v);
	groupeditundo(
		e.o[dd] -= (e.o[dd]-mid)*2;
		e.o.sub(s);
        swap(e.o[R[d]], e.o[C[d]]);
		e.o.add(s);
	);
}

void entselectionbox(const extentity &e, vec &eo, vec &es)
{
	model *m = NULL;
	if(e.type == ET_MAPMODEL && (m = loadmodel(NULL, e.attrs[0])))
	{
		m->collisionbox(0, eo, es);
		rotatebb(eo, es, e.attrs[1]);
		if(m->collide)
			eo.z -= camera1->aboveeye; // wacky but true. see physics collide
		else
			es.div(2);  // cause the usual bb is too big...
		eo.add(e.o);
	}
	else
	{
		es = vec(entselradius);
		eo = e.o;
	}
	eo.sub(es);
	es.mul(2);
}

VAR(entselsnap, 0, 1, 1);
VAR(entmovingshadow, 0, 1, 1);

extern void boxs(int orient, vec o, const vec &s);
extern void boxs3D(const vec &o, vec s, int g);
extern void editmoveplane(const vec &o, const vec &ray, int d, float off, vec &handle, vec &dest, bool first);

bool initentdragging = true;

void entdrag(const vec &ray)
{
	if(noentedit() || !haveselent()) return;

	float r = 0, c = 0;
	static vec v, handle;
	vec eo, es;
	int d = dimension(entorient),
		dc= dimcoord(entorient);

	entfocus(entgroup.last(),
		entselectionbox(e, eo, es);

		editmoveplane(e.o, ray, d, eo[d] + (dc ? es[d] : 0), handle, v, initentdragging);

		ivec g(v);
		int z = g[d]&(~(sel.grid-1));
		g.add(sel.grid/2).mask(~(sel.grid-1));
		g[d] = z;

		r = (entselsnap ? g[R[d]] : v[R[d]]) - e.o[R[d]];
		c = (entselsnap ? g[C[d]] : v[C[d]]) - e.o[C[d]];
	);

	if(initentdragging) makeundoent();
	groupeditpure(e.o[R[d]] += r; e.o[C[d]] += c);
	initentdragging = false;
}

void renderentselection(const vec &o, const vec &ray, bool entmoving)
{
	if(noentedit()) return;
	vec eo, es;

	glColor3ub(0, 40, 0);
	loopv(entgroup) entfocus(entgroup[i],
		entselectionbox(e, eo, es);
		boxs3D(eo, es, 1);
	);

	if(enthover >= 0)
	{
		entfocus(enthover, entselectionbox(e, eo, es)); // also ensures enthover is back in focus
		boxs3D(eo, es, 1);
		if(entmoving && entmovingshadow==1)
		{
			vec a, b;
			glColor3ub(20, 20, 20);
			(a=eo).x=0; (b=es).x=hdr.worldsize; boxs3D(a, b, 1);
			(a=eo).y=0; (b=es).y=hdr.worldsize; boxs3D(a, b, 1);
			(a=eo).z=0; (b=es).z=hdr.worldsize; boxs3D(a, b, 1);
		}
		glColor3ub(150,0,0);
		glLineWidth(5);
		boxs(entorient, eo, es);
		glLineWidth(1);
	}
}

bool enttoggle(int id)
{
	undonext = true;
	int i = entgroup.find(id);
	if(i < 0)
		entadd(id);
	else
		entgroup.remove(i);
	return i < 0;
}

bool hoveringonent(int ent, int orient)
{
	if(noentedit()) return false;
	entorient = orient;
	if((efocus = enthover = ent) >= 0)
		return true;
	efocus	= entgroup.empty() ? -1 : entgroup.last();
	enthover = -1;
	return false;
}

VAR(entitysurf, 0, 0, 1);
VARF(entmoving, 0, 0, 2,
	if(enthover < 0 || noentedit())
		entmoving = 0;
	else if(entmoving == 1)
		entmoving = enttoggle(enthover);
	else if(entmoving == 2 && entgroup.find(enthover) < 0)
		entadd(enthover);
	if(entmoving > 0)
		initentdragging = true;
);

void entpush(int *dir)
{
	if(noentedit()) return;
	int d = dimension(entorient);
	int s = dimcoord(entorient) ? -*dir : *dir;
	if(entmoving)
	{
		groupeditpure(e.o[d] += float(s*sel.grid)); // editdrag supplies the undo
	}
	else
		groupedit(e.o[d] += float(s*sel.grid));
	if(entitysurf==1)
	{
		physent *player = (physent *)game::iterdynents(0);
		if(!player) player = camera1;
		player->o[d] += s*sel.grid;
	}
}

VAR(entautoviewdist, 0, 25, 100);
void entautoview(int *dir)
{
	if(!haveselent()) return;
	static int s = 0;
	physent *player = (physent *)game::iterdynents(0);
	if(!player) player = camera1;
	vec v(player->o);
	v.sub(worldpos);
	v.normalize();
	v.mul(entautoviewdist);
	int t = s + *dir;
	s = abs(t) % entgroup.length();
	if(t<0 && s>0) s = entgroup.length() - s;
	entfocus(entgroup[s],
		v.add(e.o);
		player->o = v;
        player->resetinterp();
	);
}

COMMAND(entautoview, "i");
COMMAND(entflip, "");
COMMAND(entrotate, "i");
COMMAND(entpush, "i");

void delent()
{
	if(noentedit()) return;
	groupedit(e.type = ET_EMPTY;);
	entcancel();
}

VAR(entdrop, 0, 2, 3);

void dropenttofloor(extentity *e)
{
	if(!insideworld(e->o)) return;
	vec v(0.0001f, 0.0001f, -1);
	v.normalize();
	if(raycube(e->o, v, hdr.worldsize) >= hdr.worldsize) return;
	physent d;
	d.type = ENT_CAMERA;
	d.o = e->o;
	d.vel = vec(0, 0, -1);
	d.radius = 1.0f;
	d.height = entities::dropheight(*e);
	d.aboveeye = 1.0f;
	while (!collide(&d, v) && d.o.z > 0.f) d.o.z -= 0.1f;
	e->o = d.o;
}

bool dropentity(extentity &e, int drop = -1)
{
	vec radius(4.0f, 4.0f, 4.0f);
	if(drop<0) drop = entdrop;
	if(e.type == ET_MAPMODEL)
	{
		model *m = loadmodel(NULL, e.attrs[0]);
		if(m)
		{
			vec center;
			m->boundbox(0, center, radius);
			rotatebb(center, radius, e.attrs[1]);
			radius.x += fabs(center.x);
			radius.y += fabs(center.y);
		}
		radius.z = 0.0f;
	}
	switch(drop)
	{
	case 1:
		if(e.type != ET_LIGHT && e.type != ET_LIGHTFX && e.type != ET_SUNLIGHT) dropenttofloor(&e);
		break;
	case 2:
	case 3:
		int cx = 0, cy = 0;
		if(sel.cxs == 1 && sel.cys == 1)
		{
			cx = (sel.cx ? 1 : -1) * sel.grid / 2;
			cy = (sel.cy ? 1 : -1) * sel.grid / 2;
		}
		e.o = sel.o.tovec();
		int d = dimension(sel.orient), dc = dimcoord(sel.orient);
		e.o[R[d]] += sel.grid / 2 + cx;
		e.o[C[d]] += sel.grid / 2 + cy;
		if(!dc)
			e.o[D[d]] -= radius[D[d]];
		else
			e.o[D[d]] += sel.grid + radius[D[d]];

		if(drop == 3)
			dropenttofloor(&e);
		break;
	}
	return true;
}

void dropent()
{
	if(noentedit()) return;
	groupedit(dropentity(e));
}

extentity *newentity(bool local, const vec &o, int type, vector<int> &attrs)
{
	extentity &e = *entities::newent();
	e.o = o;
	while(e.attrs.length() < max(5, attrs.length())) e.attrs.add(0);
	loopv(attrs) e.attrs[i] = attrs[i];
	e.type = type;
	e.spawned = false;
	e.inoctanode = false;
    e.light.color = vec(1, 1, 1);
    e.light.dir = vec(0, 0, 1);
	entities::getents().add(&e);
	if(local)
	{
		int n = entities::getents().find(&e);
		if(entities::getents().inrange(n)) entities::fixentity(n, true, true);
	}
	return &e;
}

void newentity(int type, vector<int> &attrs)
{
	extentity *t = newentity(true, camera1->o, type, attrs);
	dropentity(*t);
	int i = entities::getents().length()-1;
	t->type = ET_EMPTY;
	enttoggle(i);
	makeundoent();
	entedit(i, e.type = type);
}

void entattrs(const char *str, vector<int> &attrs)
{
	int num = listlen(str);
	loopk(num)
	{
		char *a = indexlist(str, k);
		if(a)
		{
			attrs.add(atoi(a));
			DELETEA(a);
		}
	}
}

void newent(char *what, char *attr)
{
	if(noentedit()) return;
	int type = entities::findtype(what);
	vector<int> attrs;
	entattrs(attr, attrs);
	if(type != ET_EMPTY) newentity(type, attrs);
}

int entcopygrid;
vector<entity> entcopybuf;

void entcopy()
{
	if(noentedit()) return;
	entcopygrid = sel.grid;
	entcopybuf.setsize(0);
	loopv(entgroup)
		entfocus(entgroup[i], entcopybuf.add(e).o.sub(sel.o.tovec()));
}

void entpaste()
{
	if(noentedit()) return;
	if(entcopybuf.length()==0) return;
	entcancel();
	int last = entities::getents().length()-1;
	float m = float(sel.grid)/float(entcopygrid);
	loopv(entcopybuf)
	{
		entity &c = entcopybuf[i];
		vec o(c.o);
		o.mul(m).add(sel.o.tovec());
		extentity *e = newentity(true, o, ET_EMPTY, c.attrs);
		loopvk(c.links) e->links.add(c.links[k]);
		entadd(++last);
	}
	int j = 0;
	groupeditundo(e.type = entcopybuf[j++].type;);
}

COMMAND(newent, "ss");
COMMAND(delent, "");
COMMAND(dropent, "");
COMMAND(entcopy, "");
COMMAND(entpaste, "");

void entlink()
{
	if(entgroup.length() > 1)
	{
		const vector<extentity *> &ents = entities::getents();
		int index = entgroup[0];
		if(ents.inrange(index))
		{
			loopi(entgroup.length()-1)
			{
				int node = entgroup[i+1];

				if(verbose >= 2) conoutf("\faattempting to link %d and %d (%d)", index, node, i+1);
				if(ents.inrange(node))
				{
					if(!entities::linkents(index, node) && !entities::linkents(node, index))
						conoutf("\frfailed linking %d and %d (%d)", index, node, i+1);
				}
				else conoutf("\fr%d (%d) is not in range", node, i+1);
			}
		}
		else conoutf("\fr%d (%d) is not in range", index, 0);
	}
	else conoutft(CON_MESG, "\frmore than one entity must be selected to link");
}
COMMAND(entlink, "");


void entset(char *what, char *attr)
{
	if(noentedit()) return;
	int type = entities::findtype(what);
	static vector<int> attrs; attrs.setsizenodelete(0);
	entattrs(attr, attrs);
	groupedit({
		e.type = type;
		while(e.attrs.length() < max(5, attrs.length())) e.attrs.add(0);
		loopvk(attrs) e.attrs[k] = attrs[k];
	});
}

ICOMMAND(enthavesel,"", (), addimplicit(intret(entgroup.length())));
ICOMMAND(entselect, "s", (char *body), if(!noentedit()) addgroup(e.type != ET_EMPTY && entgroup.find(n)<0 && execute(body)>0));
ICOMMAND(entloop, "s", (char *body), if(!noentedit()) addimplicit(groupeditloop(((void)e, execute(body)))));
ICOMMAND(enttype, "s", (char *s), entfocus(efocus, intret((!*s || !strcmp(s, entities::findname(e.type))))));
ICOMMAND(insel, "", (), entfocus(efocus, intret(pointinsel(sel, e.o))));
ICOMMAND(entget, "", (), entfocus(efocus, {
	defformatstring(s)("%s", entities::findname(e.type));
	loopv(e.attrs)
	{
		defformatstring(str)(" %d", e.attrs[i]);
		concatstring(s, str);
	}
	result(s);
}));
ICOMMAND(entindex, "", (), intret(efocus));
COMMAND(entset, "ss");


int findentity(int type, int index, vector<int> &attr)
{
    const vector<extentity *> &ents = entities::getents();
    for(int i = index; i<ents.length(); i++)
    {
        extentity &e = *ents[i];
        if(e.type==type)
        {
        	bool find = true;
        	loopvk(attr) if(!e.attrs.inrange(k) || e.attrs[k] != attr[k])
        	{
        		find = false;
        		break;
       		}
       		if(find) return i;
        }
    }
    loopj(min(index, ents.length()))
    {
        extentity &e = *ents[j];
        if(e.type==type)
        {
        	bool find = true;
        	loopvk(attr) if(!e.attrs.inrange(k) || e.attrs[k] != attr[k])
        	{
        		find = false;
        		break;
       		}
       		if(find) return j;
        }
    }
    return -1;
}

void splitocta(cube *c, int size)
{
	if(size <= VVEC_INT_MASK+1) return;
	loopi(8)
	{
		if(!c[i].children) c[i].children = newcubes(isempty(c[i]) ? F_EMPTY : F_SOLID);
		splitocta(c[i].children, size>>1);
	}
}

void clearworldvars(bool msg)
{
	overrideidents = worldidents = true;
	persistidents = false;
	enumerate(*idents, ident, id, {
		if(id.flags&IDF_WORLD) // reset world vars
		{
			switch (id.type)
			{
				case ID_VAR:
				{
					setvar(id.name, id.def.i, true);
					break;
				}
				case ID_FVAR:
				{
					setfvar(id.name, id.def.f, true);
					break;
				}
				case ID_SVAR:
				{
					setsvar(id.name, id.def.s && *id.def.s ? id.def.s : "", true);
					break;
				}
				case ID_ALIAS:
				{
					worldalias(id.name, "");
					break;
				}
				default: break;
			}
		}
	});
	overrideidents = worldidents = false;
	persistidents = true;
	if(msg) conoutf("world variables reset");
}

ICOMMAND(resetworldvars, "", (), if(editmode || worldidents) clearworldvars(true));

void resetmap(bool empty)
{
	progress(0, "resetting map...");
	resetmaterials();
	resettextures();
	resetmapmodels();
	clearoverrides();
	clearsound();
	cleanreflections();
    resetblendmap();
	resetlightmaps();
    clearpvs();
	clearparticles();
    cleardecals();
	clearsleep();
	cancelsel();
	pruneundos();
    entities::clearents();
	game::resetmap(empty);
}

bool emptymap(int scale, bool force, char *mname, bool nocfg)	// main empty world creation routine
{
	if(!force && !editmode)
	{
		conoutft(CON_MESG, "\frnewmap only allowed in edit mode");
		return false;
	}

	clearworldvars();
	resetmap(nocfg);
	setnames(mname, MAP_BFGZ);
	strncpy(hdr.head, "BFGZ", 4);

	hdr.version = MAPVERSION;
	hdr.gamever = server::gamever();
	hdr.headersize = sizeof(bfgz);
    worldscale = scale<10 ? 10 : (scale>20 ? 20 : scale);
    hdr.worldsize = 1<<worldscale;
	hdr.revision = 0;
    hdr.numpvs = 0;
    hdr.blendmap = 0;
	hdr.lightmaps = 0;

	copystring(hdr.gameid, server::gameid(), 4);

	texmru.setsize(0);
	freeocta(worldroot);
	worldroot = newcubes(F_EMPTY);
	loopi(4) solidfaces(worldroot[i]);

	if(hdr.worldsize > VVEC_INT_MASK+1) splitocta(worldroot, hdr.worldsize>>1);

	clearlights();
	allchanged();

    if(!nocfg)
    {
	    overrideidents = worldidents = true;
	    persistidents = false;
	    execfile("map.cfg");
	    persistidents = true;
	    overrideidents = worldidents = false;
    }
	game::startmap(nocfg ? "" : "maps/untitled", NULL, true);
	return true;
}

bool enlargemap(bool force)
{
	if(!force && !editmode)
	{
		conoutft(CON_MESG, "\frmapenlarge only allowed in edit mode");
		return false;
	}
	if(hdr.worldsize >= 1<<20) return false;

    worldscale++;
	hdr.worldsize *= 2;
	cube *c = newcubes(F_EMPTY);
	c[0].children = worldroot;
	loopi(3) solidfaces(c[i+1]);
	worldroot = c;

	if(hdr.worldsize > VVEC_INT_MASK+1) splitocta(worldroot, hdr.worldsize>>1);

    enlargeblendmap();

	allchanged();

	return true;
}

ICOMMAND(newmap, "is", (int *i), if(emptymap(*i, false)) game::newmap(::max(*i, 0)));
ICOMMAND(mapenlarge, "", (), if(enlargemap(false)) game::newmap(-1));
ICOMMAND(mapsize, "", (void),
{
    int size = 0;
    while(1<<size < hdr.worldsize) size++;
    intret(size);
});

void mpeditent(int i, const vec &o, int type, vector<int> &attr, bool local)
{
	if(entities::getents().length()<=i)
	{
		while(entities::getents().length()<i) entities::getents().add(entities::newent())->type = ET_EMPTY;
		newentity(local, o, type, attr);
		addentity(i);
	}
	else
	{
		extentity &e = *entities::getents()[i];
		removeentity(i);
		e.type = type;
		e.o = o;
		while(e.attrs.length() < max(5, attr.length())) e.attrs.add(0);
		loopvk(attr) e.attrs[k] = attr[k];
		addentity(i);
	}
}

void newentity(vec &v, int type, vector<int> &attrs)
{
	extentity *t = newentity(true, v, type, attrs);
	int i = entities::getents().length()-1;
	t->type = ET_EMPTY;
	enttoggle(i);
	makeundoent();
	entedit(i, e.type = type);
}
