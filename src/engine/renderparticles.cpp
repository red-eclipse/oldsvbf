// renderparticles.cpp

#include "cube.h"
#include "engine.h"
#include "rendertarget.h"

Shader *particleshader = NULL, *particlenotextureshader = NULL;

#define MAXPARTICLES 40000

VARFP(maxparticles, 10, 4000, MAXPARTICLES, particleinit());
VARA(maxparticledistance, 128, 1024, 4096);
VARP(maxparticletrail, 1, 256, 8192);

VARP(particletext, 0, 1, 1);
VARP(maxparticletextdistance, 0, 128, 10000);
VARP(outlinemeters, 0, 0, 1);
VARP(particleglare, 0, 1, 100);
VAR(debugparticles, 0, 0, 1);

// Check emit_particles() to limit the rate that paricles can be emitted for models/sparklies
// Automatically stops particles being emitted when paused or in reflective drawing
VARP(emitmillis, 1, 17, 1000);
static int lastemitframe = 0;
static bool emit = false;

static bool emit_particles()
{
	if(reflecting || refracting) return false;
	return emit;
}

const char *partnames[] = { "part", "tape", "trail", "text", "textup", "meter", "metervs", "fireball", "lightning", "flare", "portal", "icon" };

struct partvert
{
	vec pos;
	float u, v;
	bvec color;
	uchar alpha;
};

#define COLLIDERADIUS 8.0f
#define COLLIDEERROR 1.0f

struct partrenderer
{
	Texture *tex;
	const char *texname;
	uint type;
	int grav, collide, frames;

	partrenderer(const char *texname, int type, int grav, int collide, int frames = 1)
		: tex(NULL), texname(texname), type(type), grav(grav), collide(collide), frames(frames)
	{
	}
	virtual ~partrenderer()
	{
	}

	virtual void init(int n) { }
	virtual void reset() = NULL;
	virtual void resettracked(physent *owner) { }
	virtual particle *addpart(const vec &o, const vec &d, int fade, int color, float size, physent *pl = NULL) = NULL;
	virtual int adddepthfx(vec &bbmin, vec &bbmax) { return 0; }
	virtual void update() { }
	virtual void makeflares() { }
	virtual void render() = NULL;
	virtual bool haswork() = NULL;
	virtual int count() = NULL; //for debug
	virtual bool usesvertexarray() { return false; }
	virtual void cleanup() {}

	void preload()
	{
		if(texname && (!tex || tex == notexture))
			tex = textureload(texname);
	}

	//blend = 0 => remove it
	void calc(particle *p, int &blend, int &ts, vec &o, vec &d, bool lastpass = true)
	{
		o = p->o;
		d = p->d;
		world::particletrack(p, type, ts, o, d, lastpass);
		if(p->fade <= 5)
		{
			ts = 1;
			blend = 255;
		}
		else
		{
			ts = lastmillis-p->millis;
			blend = max(255 - (ts<<8)/p->fade, 0);
			if(grav)
			{
				if(ts > p->fade) ts = p->fade;
				float secs = ts/1000.f;
				vec v = vec(d).mul(secs);
				static physent dummy;
				dummy.weight = grav;
				v.z -= physics::gravityforce(&dummy)*secs;
				v.mul(secs);
				o.add(v);
			}
			if(collide && o.z < p->val && lastpass)
			{
				vec surface;
				float floorz = rayfloor(vec(o.x, o.y, p->val), surface, RAY_CLIPMAT, COLLIDERADIUS);
				float collidez = floorz<0 ? o.z-COLLIDERADIUS : p->val - rayfloor(vec(o.x, o.y, p->val), surface, RAY_CLIPMAT, COLLIDERADIUS);
				if(o.z >= collidez+COLLIDEERROR)
					p->val = collidez+COLLIDEERROR;
				else
				{
					adddecal(collide, vec(o.x, o.y, collidez), vec(p->o).sub(o).normalize(), 2*p->size, p->color, type&PT_RND4 ? (p->flags>>5)&3 : 0);
					blend = 0;
				}
			}
		}
	}

	void makeflare(particle *p)
	{
		vec o, d;
		int blend, ts;
		calc(p, blend, ts, o, d, false);
		if(blend > 0)
		{
			extern void addlensflare(vec &o, uchar r, uchar g, uchar b, bool sparkle, float size);
			extern int flaresize;
			addlensflare(o, p->color[0], p->color[1], p->color[2], type&PT_SPARKLE, p->size*1.15f*(flaresize/100.f));
		}
	}
};

#include "depthfx.h"
#include "lensflare.h"

template<class T>
struct listparticle : particle
{
	T *next;
};

struct sharedlistparticle : listparticle<sharedlistparticle> {};

template<class T>
struct listrenderer : partrenderer
{
	static T *parempty;
	T *list;

	listrenderer(const char *texname, int type, int grav, int collide, int frames = 1)
		: partrenderer(texname, type, grav, collide, frames), list(NULL)
	{
	}

	virtual ~listrenderer()
	{
	}

	virtual void cleanup(T *p)
	{
	}

	void reset()
	{
		if(!list) return;
		T *p = list;
		for(;;)
		{
			cleanup(p);
			if(p->next) p = p->next;
			else break;
		}
		p->next = parempty;
		parempty = list;
		list = NULL;
	}

	void resettracked(physent *pl)
	{
		for(T **prev = &list, *cur = list; cur; cur = *prev)
		{
			if(cur->owner == pl)
			{
				*prev = cur->next;
				cur->next = parempty;
				parempty = cur;
			}
			else prev = &cur->next;
		}
	}

	particle *addpart(const vec &o, const vec &d, int fade, int color, float size, physent *pl = NULL)
	{
		if(!parempty)
		{
			T *ps = new T[32];
			loopi(31) ps[i].next = &ps[i+1];
			ps[31].next = parempty;
			parempty = ps;
		}
		T *p = parempty;
		parempty = p->next;
		p->next = list;
		list = p;
		p->o = o;
		p->d = d;
		p->fade = fade;
		p->millis = lastmillis;
		p->color = bvec(color>>16, (color>>8)&0xFF, color&0xFF);
		p->size = size;
		p->owner = pl;
		return p;
	}

	int count()
	{
		int num = 0;
		T *lp;
		for(lp = list; lp; lp = lp->next) num++;
		return num;
	}

	bool haswork()
	{
		return (list != NULL);
	}

	virtual void startrender() = 0;
	virtual void endrender() = 0;
	virtual void renderpart(T *p, const vec &o, const vec &d, int blend, int ts, uchar *color) = 0;

	void render()
	{
		preload();
		startrender();
		if(tex) glBindTexture(GL_TEXTURE_2D, tex->id);
		bool lastpass = !reflecting && !refracting;
		for(T **prev = &list, *p = list; p; p = *prev)
		{
			vec o, d;
			int blend, ts;
			calc(p, blend, ts, o, d, lastpass);
			if(blend > 0)
			{
				renderpart(p, o, d, blend, ts, p->color.v);

				if(p->fade > 5 || !lastpass)
				{
					prev = &p->next;
					continue;
				}
			}
			//remove
			*prev = p->next;
			p->next = parempty;
			cleanup(p);
			parempty = p;
		}

		endrender();
	}

	void makeflares()
	{
		for(T **prev = &list, *p = list; p; p = *prev)
			makeflare(p);
	}
};

template<class T> T *listrenderer<T>::parempty = NULL;

typedef listrenderer<sharedlistparticle> sharedlistrenderer;

#include "explosion.h"
#include "lightning.h"

struct meterrenderer : sharedlistrenderer
{
	meterrenderer(int type)
		: sharedlistrenderer(NULL, type, 0, 0)
	{}

	void startrender()
	{
		 glDisable(GL_BLEND);
		 glDisable(GL_TEXTURE_2D);
		 particlenotextureshader->set();
	}

	void endrender()
	{
		 glEnable(GL_BLEND);
		 glEnable(GL_TEXTURE_2D);
		 if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(1, reflectz);
		 particleshader->set();
	}

	void renderpart(sharedlistparticle *p, const vec &o, const vec &d, int blend, int ts, uchar *color)
	{
		int basetype = type&0xFF;

		glPushMatrix();
		glTranslatef(o.x, o.y, o.z);
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(0, reflectz - o.z, true);
		glRotatef(camera1->yaw-180, 0, 0, 1);
		glRotatef(camera1->pitch-90, 1, 0, 0);

		float scale = p->size/80.0f;
		glScalef(-scale, scale, -scale);

		float right = 8*FONTH, left = p->progress/100.0f*right;
		glTranslatef(-right/2.0f, 0, 0);

		if(outlinemeters)
		{
			glColor3f(0, 0.8f, 0);
			glBegin(GL_TRIANGLE_STRIP);
			loopk(10)
			{
				float c = (0.5f + 0.1f)*sinf(k/9.0f*M_PI), s = 0.5f - (0.5f + 0.1f)*cosf(k/9.0f*M_PI);
				glVertex2f(-c*FONTH, s*FONTH);
				glVertex2f(right + c*FONTH, s*FONTH);
			}
			glEnd();
		}

		if(basetype==PT_METERVS) glColor3ubv(p->color2);
		else glColor3f(0, 0, 0);
		glBegin(GL_TRIANGLE_STRIP);
		loopk(10)
		{
			float c = 0.5f*sinf(k/9.0f*M_PI), s = 0.5f - 0.5f*cosf(k/9.0f*M_PI);
			glVertex2f(left + c*FONTH, s*FONTH);
			glVertex2f(right + c*FONTH, s*FONTH);
		}
		glEnd();

		if(outlinemeters)
		{
			glColor3f(0, 0.8f, 0);
			glBegin(GL_TRIANGLE_FAN);
			loopk(10)
			{
				float c = (0.5f + 0.1f)*sinf(k/9.0f*M_PI), s = 0.5f - (0.5f + 0.1f)*cosf(k/9.0f*M_PI);
				glVertex2f(left + c*FONTH, s*FONTH);
			}
			glEnd();
		}

		glColor3ubv(color);
		glBegin(GL_TRIANGLE_STRIP);
		loopk(10)
		{
			float c = 0.5f*sinf(k/9.0f*M_PI), s = 0.5f - 0.5f*cosf(k/9.0f*M_PI);
			glVertex2f(-c*FONTH, s*FONTH);
			glVertex2f(left + c*FONTH, s*FONTH);
		}
		glEnd();


		glPopMatrix();
	}
};
static meterrenderer meters(PT_METER|PT_LERP), metervs(PT_METERVS|PT_LERP);

struct textrenderer : sharedlistrenderer
{
	textrenderer(int type, int grav = 0, int frames = 1)
		: sharedlistrenderer(NULL, type, grav, 0, frames)
	{}

	void startrender()
	{
	}

	void endrender()
	{
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(1, reflectz);
	}

	void cleanup(sharedlistparticle *p)
	{
		if(p->text && p->text[0]=='@') delete[] p->text;
	}

	void renderpart(sharedlistparticle *p, const vec &o, const vec &d, int blend, int ts, uchar *color)
	{
		glPushMatrix();
		glTranslatef(o.x, o.y, o.z);
		if(fogging)
		{
			if(renderpath!=R_FIXEDFUNCTION) setfogplane(0, reflectz - o.z, true);
			else blend = (uchar)(blend * max(0.0f, min(1.0f, 1.0f - (reflectz - o.z)/waterfog)));
		}

		glRotatef(camera1->yaw-180, 0, 0, 1);
		glRotatef(camera1->pitch-90, 1, 0, 0);

		float scale = p->size/80.0f;
		glScalef(-scale, scale, -scale);

		const char *text = p->text+(p->text[0]=='@' ? 1 : 0);
		float xoff = -text_width(text)/2;
		float yoff = 0;
		if((type&0xFF)==PT_TEXTUP) { xoff += detrnd((size_t)p, 100)-50; yoff -= detrnd((size_t)p, 101); } //@TODO instead in worldspace beforehand?
		glTranslatef(xoff, yoff, 50);

		draw_text(text, 0, 0, color[0], color[1], color[2], blend);

		glPopMatrix();
	}
};
static textrenderer texts(PT_TEXT|PT_LERP), textups(PT_TEXTUP|PT_LERP, -5);

struct portal : listparticle<portal>
{
	float yaw, pitch;
};

struct portalrenderer : listrenderer<portal>
{
	portalrenderer(const char *texname)
		: listrenderer<portal>(texname, PT_PORTAL|PT_LERP, 0, 0)
	{}

	void startrender()
	{
		glDisable(GL_CULL_FACE);
	}

	void endrender()
	{
		glEnable(GL_CULL_FACE);
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(1, reflectz);
	}

	void renderpart(portal *p, const vec &o, const vec &d, int blend, int ts, uchar *color)
	{
		glPushMatrix();
		glTranslatef(o.x, o.y, o.z);
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(0, reflectz - o.z, true);
		glRotatef(p->yaw-180, 0, 0, 1);
		glRotatef(p->pitch, 1, 0, 0);
		glScalef(p->size, p->size, p->size);

		glColor4ub(color[0], color[1], color[2], blend);
		glBegin(GL_QUADS);
		glTexCoord2f(1, 0); glVertex3f(-1, 0,  1);
		glTexCoord2f(0, 0); glVertex3f( 1, 0,  1);
		glTexCoord2f(0, 1); glVertex3f( 1, 0, -1);
		glTexCoord2f(1, 1); glVertex3f(-1, 0, -1);
		glEnd();

		glPopMatrix();
	}

	portal *addportal(const vec &o, int fade, int color, float size, float yaw, float pitch)
	{
		portal *p = (portal *)listrenderer<portal>::addpart(o, vec(0, 0, 0), fade, color, size);
		p->yaw = yaw;
		p->pitch = pitch;
		return p;
	}

	// use addportal() instead
	particle *addpart(const vec &o, const vec &d, int fade, int color, float size, physent *pl = NULL) { return NULL; }
};

struct icon : listparticle<icon>
{
	Texture *tex;
	float blend;
};

struct iconrenderer : listrenderer<icon>
{
	Texture *lasttex;

	iconrenderer(int type, int grav = 0, int frames = 1)
		: listrenderer<icon>(NULL, type, grav, 0, frames)
	{}

	void startrender()
	{
		lasttex = NULL;
	}

	void endrender()
	{
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(1, reflectz);
	}

	void renderpart(icon *p, const vec &o, const vec &d, int blend, int ts, uchar *color)
	{
		if(p->tex != lasttex)
		{
			glBindTexture(GL_TEXTURE_2D, p->tex->id);
			lasttex = p->tex;
		}

		glPushMatrix();
		glTranslatef(o.x, o.y, o.z);
		if(fogging && renderpath!=R_FIXEDFUNCTION) setfogplane(0, reflectz - o.z, true);
		glRotatef(camera1->yaw-180, 0, 0, 1);
		glRotatef(camera1->pitch, 1, 0, 0);
		glScalef(p->size, p->size, p->size);

		glColor4ub(color[0], color[1], color[2], uchar(p->blend*blend));
		glBegin(GL_QUADS);
		glTexCoord2f(1, 1); glVertex3f(-1, 0, -1);
		glTexCoord2f(0, 1); glVertex3f( 1, 0, -1);
		glTexCoord2f(0, 0); glVertex3f( 1, 0,  1);
		glTexCoord2f(1, 0); glVertex3f(-1, 0,  1);
		glEnd();

		glPopMatrix();
	}

	icon *addicon(const vec &o, Texture *tex, float blend, int fade, int color, float size)
	{
		icon *p = (icon *)listrenderer<icon>::addpart(o, vec(0, 0, 0), fade, color, size);
		p->tex = tex;
		p->blend = blend;
		return p;
	}

	// use addicon() instead
	particle *addpart(const vec &o, const vec &d, int fade, int color, float size, physent *pl = NULL) { return NULL; }
};
static iconrenderer icons(PT_ICON|PT_LERP), iconups(PT_ICON|PT_LERP, -5);

template<int T>
static inline void modifyblend(const vec &o, int &blend)
{
	blend = min(blend<<2, 255);
	if(renderpath==R_FIXEDFUNCTION && fogging) blend = (uchar)(blend * max(0.0f, min(1.0f, 1.0f - (reflectz - o.z)/waterfog)));
}

template<>
inline void modifyblend<PT_TAPE>(const vec &o, int &blend)
{
}

template<int T>
static inline void genpos(const vec &o, const vec &d, float size, int grav, int ts, partvert *vs)
{
	vec udir = vec(camup).sub(camright).mul(size);
	vec vdir = vec(camup).add(camright).mul(size);
	vs[0].pos = vec(o.x + udir.x, o.y + udir.y, o.z + udir.z);
	vs[1].pos = vec(o.x + vdir.x, o.y + vdir.y, o.z + vdir.z);
	vs[2].pos = vec(o.x - udir.x, o.y - udir.y, o.z - udir.z);
	vs[3].pos = vec(o.x - vdir.x, o.y - vdir.y, o.z - vdir.z);
}

template<>
inline void genpos<PT_TAPE>(const vec &o, const vec &d, float size, int ts, int grav, partvert *vs)
{
	vec dir1 = d, dir2 = d, c;
	dir1.sub(o);
	dir2.sub(camera1->o);
	c.cross(dir2, dir1).normalize().mul(size);
	vs[0].pos = vec(d.x-c.x, d.y-c.y, d.z-c.z);
	vs[1].pos = vec(o.x-c.x, o.y-c.y, o.z-c.z);
	vs[2].pos = vec(o.x+c.x, o.y+c.y, o.z+c.z);
	vs[3].pos = vec(d.x+c.x, d.y+c.y, d.z+c.z);
}

template<>
inline void genpos<PT_TRAIL>(const vec &o, const vec &d, float size, int ts, int grav, partvert *vs)
{
	vec e = d;
	if(grav) e.z -= float(ts)/grav;
	e.div(-75.0f);
	e.add(o);
	genpos<PT_TAPE>(o, e, size, ts, grav, vs);
}

template<int T>
static inline void genrotpos(const vec &o, const vec &d, float size, int grav, int ts, partvert *vs, int rot)
{
	genpos<T>(o, d, size, grav, ts, vs);
}

#define ROTCOEFFS(n) { \
	vec(-1,  1, 0).rotate_around_z(n*2*M_PI/32.0f), \
	vec( 1,  1, 0).rotate_around_z(n*2*M_PI/32.0f), \
	vec( 1, -1, 0).rotate_around_z(n*2*M_PI/32.0f), \
	vec(-1, -1, 0).rotate_around_z(n*2*M_PI/32.0f) \
}
static const vec rotcoeffs[32][4] =
{
	ROTCOEFFS(0),  ROTCOEFFS(1),  ROTCOEFFS(2),  ROTCOEFFS(3),  ROTCOEFFS(4),  ROTCOEFFS(5),  ROTCOEFFS(6),  ROTCOEFFS(7),
	ROTCOEFFS(8),  ROTCOEFFS(9),  ROTCOEFFS(10), ROTCOEFFS(11), ROTCOEFFS(12), ROTCOEFFS(13), ROTCOEFFS(14), ROTCOEFFS(15),
	ROTCOEFFS(16), ROTCOEFFS(17), ROTCOEFFS(18), ROTCOEFFS(19), ROTCOEFFS(20), ROTCOEFFS(21), ROTCOEFFS(22), ROTCOEFFS(7),
	ROTCOEFFS(24), ROTCOEFFS(25), ROTCOEFFS(26), ROTCOEFFS(27), ROTCOEFFS(28), ROTCOEFFS(29), ROTCOEFFS(30), ROTCOEFFS(31),
};

template<>
inline void genrotpos<PT_PART>(const vec &o, const vec &d, float size, int grav, int ts, partvert *vs, int rot)
{
	const vec *coeffs = rotcoeffs[rot];
	(vs[0].pos = o).add(vec(camright).mul(coeffs[0].x*size)).add(vec(camup).mul(coeffs[0].y*size));
	(vs[1].pos = o).add(vec(camright).mul(coeffs[1].x*size)).add(vec(camup).mul(coeffs[1].y*size));
	(vs[2].pos = o).add(vec(camright).mul(coeffs[2].x*size)).add(vec(camup).mul(coeffs[2].y*size));
	(vs[3].pos = o).add(vec(camright).mul(coeffs[3].x*size)).add(vec(camup).mul(coeffs[3].y*size));
}

template<int T>
struct varenderer : partrenderer
{
	partvert *verts;
	particle *parts;
	int maxparts, numparts, lastupdate, rndmask;

	varenderer(const char *texname, int type, int grav, int collide, int frames = 1)
		: partrenderer(texname, type, grav, collide, frames),
		  verts(NULL), parts(NULL), maxparts(0), numparts(0), lastupdate(-1), rndmask(0)
	{
		if(type & PT_HFLIP) rndmask |= 0x01;
		if(type & PT_VFLIP) rndmask |= 0x02;
		if(type & PT_ROT) rndmask |= 0x1F<<2;
		if(type & PT_RND4) rndmask |= 0x03<<5;
	}

	void init(int n)
	{
		DELETEA(parts);
		DELETEA(verts);
		parts = new particle[n];
		verts = new partvert[n*4];
		maxparts = n;
		numparts = 0;
		lastupdate = -1;
	}

	void reset()
	{
		numparts = 0;
		lastupdate = -1;
	}

	void resettracked(physent *pl)
	{
		loopi(numparts)
		{
			particle *p = parts+i;
			if(p->owner == pl) p->fade = -1;
		}
		lastupdate = -1;
	}

	int count()
	{
		return numparts;
	}

	bool haswork()
	{
		return (numparts > 0);
	}

	bool usesvertexarray() { return true; }

	particle *addpart(const vec &o, const vec &d, int fade, int color, float size, physent *pl = NULL)
	{
		particle *p = parts + (numparts < maxparts ? numparts++ : rnd(maxparts)); //next free slot, or kill a random kitten
		p->o = o;
		p->d = d;
		p->fade = fade;
		p->millis = lastmillis;
		p->color = bvec(color>>16, (color>>8)&0xFF, color&0xFF);
		p->size = size;
		p->owner = pl;
		p->flags = 0x80 | (rndmask ? rnd(0x80) & rndmask : 0);
		if(frames > 1) p->frame = rnd(frames);
		lastupdate = -1;
		return p;
	}

	void genverts(particle *p, partvert *vs, bool regen)
	{
		vec o, d;
		int blend, ts;

		calc(p, blend, ts, o, d);
		if(blend <= 1 || p->fade <= 5) p->fade = -1; //mark to remove on next pass (i.e. after render)

		modifyblend<T>(o, blend);

		if(regen)
		{
			p->flags &= ~0x80;

			#define SETTEXCOORDS(u1c, u2c, v1c, v2c) \
			{ \
				float u1 = u1c, u2 = u2c, v1 = v1c, v2 = v2c; \
				if(p->flags&0x01) swap(u1, u2); \
				if(p->flags&0x02) swap(v1, v2); \
				vs[0].u = u1; \
				vs[0].v = v1; \
				vs[1].u = u2; \
				vs[1].v = v1; \
				vs[2].u = u2; \
				vs[2].v = v2; \
				vs[3].u = u1; \
				vs[3].v = v2; \
			}
			float piece = 1.f, off = 0.f;
			if(frames > 1)
			{
				piece = 1.f/float(frames);
				off = p->frame * piece;
			}
			if(type&PT_RND4)
			{
				float tx = off+(0.5f*((p->flags>>5)&1)*piece);
				float ty = 0.5f*((p->flags>>6)&1);
				SETTEXCOORDS(tx, tx+(piece*0.5f), ty, ty+0.5f);
			}
			else SETTEXCOORDS(off, off + piece, 0, 1);

			#define SETCOLOR(r, g, b, a) \
			do { \
				uchar col[4] = { r, g, b, a }; \
				loopi(4) memcpy(vs[i].color.v, col, sizeof(col)); \
			} while(0)
			#define SETMODCOLOR SETCOLOR((p->color[0]*blend)>>8, (p->color[1]*blend)>>8, (p->color[2]*blend)>>8, 255)
			if(type&PT_MOD) SETMODCOLOR;
			else SETCOLOR(p->color[0], p->color[1], p->color[2], blend);
		}
		else if(type&PT_MOD) SETMODCOLOR;
		else loopi(4) vs[i].alpha = blend;

		if(type&PT_ROT) genrotpos<T>(o, d, p->size, ts, grav, vs, (p->flags>>2)&0x1F);
		else genpos<T>(o, d, p->size, ts, grav, vs);
	}

	void update()
	{
		if(lastmillis == lastupdate) return;
		lastupdate = lastmillis;

		loopi(numparts)
		{
			particle *p = &parts[i];
			partvert *vs = &verts[i*4];
			if(p->fade < 0)
			{
				do
				{
					--numparts;
					if(numparts <= i) return;
				}
				while(parts[numparts].fade < 0);
				*p = parts[numparts];
				genverts(p, vs, true);
			}
			else genverts(p, vs, (p->flags&0x80)!=0);
		}
	}

	void makeflares()
	{
		loopi(numparts)
		{
			particle *p = &parts[i];
			makeflare(p);
		}
	}

	void render()
	{
		preload();
		if(tex) glBindTexture(GL_TEXTURE_2D, tex->id);
		glVertexPointer(3, GL_FLOAT, sizeof(partvert), &verts->pos);
		glTexCoordPointer(2, GL_FLOAT, sizeof(partvert), &verts->u);
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(partvert), &verts->color);
		glDrawArrays(GL_QUADS, 0, numparts*4);
	}
};

typedef varenderer<PT_PART> quadrenderer;
typedef varenderer<PT_TAPE> taperenderer;
typedef varenderer<PT_TRAIL> trailrenderer;

struct softquadrenderer : quadrenderer
{
	softquadrenderer(const char *texname, int type, int grav, int collide)
		: quadrenderer(texname, type|PT_SOFT, grav, collide)
	{
	}

	int adddepthfx(vec &bbmin, vec &bbmax)
	{
		if(!numparts || (!depthfxtex.highprecision() && !depthfxtex.emulatehighprecision())) return 0;
		int numsoft = 0;
		loopi(numparts)
		{
			particle &p = parts[i];
			float radius = p.size*SQRT2;
			vec o, d;
			int blend, ts;
			calc(&p, blend, ts, o, d, false);
			if(depthfxscissor==2 ? depthfxtex.addscissorbox(o, radius) : isvisiblesphere(radius, o) < VFC_FOGGED)
			{
				numsoft++;
				loopk(3)
				{
					bbmin[k] = min(bbmin[k], o[k] - radius);
					bbmax[k] = max(bbmax[k], o[k] + radius);
				}
			}
		}
		return numsoft;
	}
};

static partrenderer *parts[] =
{
	new portalrenderer("textures/teleport"),
	&icons, &iconups,

	new trailrenderer("particles/entity", PT_TRAIL|PT_LERP, 0, 0),
	new softquadrenderer("particles/fireball", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_LERP, -10, 0),
	new softquadrenderer("particles/plasma", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_LERP, 0, 0),
	new taperenderer("particles/sflare", PT_TAPE|PT_LERP, 0, 0),
	new taperenderer("particles/fflare", PT_TAPE|PT_LERP, 0, 0),

	new quadrenderer("particles/smoke", PT_PART|PT_LERP|PT_RND4|PT_FLIP, 0, 0),
	new quadrenderer("particles/smoke", PT_PART|PT_LERP|PT_RND4|PT_FLIP, -10, 0),
	new softquadrenderer("particles/smoke", PT_PART|PT_LERP|PT_RND4|PT_FLIP, -10, 0),
	new quadrenderer("particles/smoke", PT_PART|PT_LERP|PT_RND4|PT_FLIP, -20, 0),
	new quadrenderer("particles/smoke", PT_PART|PT_LERP|PT_RND4|PT_FLIP, 10, 0),

	new quadrenderer("<mad:0.25/0.25/0.25>particles/smoke", PT_PART|PT_RND4|PT_FLIP, 0, 0),
	new quadrenderer("<mad:0.25/0.25/0.25>particles/smoke", PT_PART|PT_RND4|PT_FLIP, -10, 0),
	new softquadrenderer("<mad:0.25/0.25/0.25>particles/smoke", PT_PART|PT_RND4|PT_FLIP, -10, 0),
	new quadrenderer("<mad:0.25/0.25/0.25>particles/smoke", PT_PART|PT_RND4|PT_FLIP, -20, 0),
	new quadrenderer("<mad:0.25/0.25/0.25>particles/smoke", PT_PART|PT_RND4|PT_FLIP, 10, 0),

	new quadrenderer("particles/blood", PT_PART|PT_MOD|PT_RND4|PT_FLIP, 50, DECAL_BLOOD),
	new quadrenderer("particles/entity", PT_PART|PT_GLARE, 0, 0),

	new quadrenderer("particles/spark", PT_PART|PT_GLARE|PT_FLIP, 10, 0),
	new quadrenderer("particles/spark", PT_PART|PT_GLARE|PT_FLIP|PT_LENS, 10, 0),
	new quadrenderer("particles/spark", PT_PART|PT_GLARE|PT_FLIP|PT_LENS|PT_SPARKLE, 10, 0),

	new softquadrenderer("particles/fireball", PT_PART|PT_GLARE|PT_RND4|PT_FLIP, -10, 0),
	new quadrenderer("particles/fireball", PT_PART|PT_GLARE|PT_RND4|PT_FLIP, -10, 0),
	new softquadrenderer("particles/fireball", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_LENS, -10, 0),
	new quadrenderer("particles/fireball", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_LENS, -10, 0),
	new softquadrenderer("particles/fireball", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_LENS|PT_SPARKLE, -10, 0),
	new quadrenderer("particles/fireball", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_LENS|PT_SPARKLE, -10, 0),

	new softquadrenderer("particles/plasma", PT_PART|PT_GLARE|PT_RND4|PT_FLIP, 0, 0),
	new quadrenderer("particles/plasma", PT_PART|PT_GLARE|PT_RND4|PT_FLIP, 0, 0),
	new softquadrenderer("particles/plasma", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_LENS, 0, 0),
	new quadrenderer("particles/plasma", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_LENS, 0, 0),
	new softquadrenderer("particles/plasma", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_LENS|PT_SPARKLE, 0, 0),
	new quadrenderer("particles/plasma", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_LENS|PT_SPARKLE, 0, 0),

	new quadrenderer("particles/electric", PT_PART|PT_GLARE|PT_FLIP, 0, 0),
	new quadrenderer("particles/electric", PT_PART|PT_GLARE|PT_FLIP|PT_LENS, 0, 0),
	new quadrenderer("particles/electric", PT_PART|PT_GLARE|PT_FLIP|PT_LENS|PT_SPARKLE, 0, 0),

	new quadrenderer("particles/flame", PT_PART|PT_HFLIP|PT_RND4|PT_GLARE, -1, 0),
	new quadrenderer("particles/flame", PT_PART|PT_HFLIP|PT_RND4|PT_GLARE|PT_LENS, -1, 0),
	new quadrenderer("particles/flame", PT_PART|PT_HFLIP|PT_RND4|PT_GLARE|PT_LENS|PT_SPARKLE, -1, 0),

	new taperenderer("particles/sflare", PT_TAPE|PT_GLARE, 0, 0),
	new taperenderer("particles/fflare", PT_TAPE|PT_GLARE, 0, 0),

	new quadrenderer("particles/muzzle", PT_PART|PT_GLARE|PT_RND4|PT_FLIP, 0, 0),
	new quadrenderer("particles/muzzle", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_LENS, 0, 0),
	new quadrenderer("particles/muzzle", PT_PART|PT_GLARE|PT_RND4|PT_FLIP|PT_LENS|PT_SPARKLE, 0, 0),

	new taperenderer("particles/line", PT_TAPE|PT_GLARE, 0, 0),
	new quadrenderer("particles/snow", PT_PART|PT_GLARE|PT_FLIP, 100, DECAL_STAIN),

	&texts, &textups, &meters, &metervs,
	&fireballs, &noglarefireballs, &lightnings,
	&flares // must be done last!
};

void finddepthfxranges()
{
	depthfxmin = vec(1e16f, 1e16f, 1e16f);
	depthfxmax = vec(0, 0, 0);
	numdepthfxranges = fireballs.finddepthfxranges(depthfxowners, depthfxranges, MAXDFXRANGES, depthfxmin, depthfxmax);
	loopk(3)
	{
		depthfxmin[k] -= depthfxmargin;
		depthfxmax[k] += depthfxmargin;
	}
	if(depthfxparts)
	{
		loopi(sizeof(parts)/sizeof(parts[0]))
		{
			partrenderer *p = parts[i];
			if(p->type&PT_SOFT && p->adddepthfx(depthfxmin, depthfxmax))
			{
				if(!numdepthfxranges)
				{
					numdepthfxranges = 1;
					depthfxowners[0] = NULL;
					depthfxranges[0] = 0;
				}
			}
		}
	}
	if(depthfxscissor<2 && numdepthfxranges>0) depthfxtex.addscissorbox(depthfxmin, depthfxmax);
}

void particleinit()
{
	if(!particleshader) particleshader = lookupshaderbyname("particle");
	if(!particlenotextureshader) particlenotextureshader = lookupshaderbyname("particlenotexture");
	loopi(sizeof(parts)/sizeof(parts[0]))
	{
		parts[i]->init(maxparticles);
		parts[i]->preload();
	}
}

void clearparticles()
{
	loopi(sizeof(parts)/sizeof(parts[0])) parts[i]->reset();
}

void cleanupparticles()
{
	loopi(sizeof(parts)/sizeof(parts[0])) parts[i]->cleanup();
}

void removetrackedparticles(physent *pl)
{
	loopi(sizeof(parts)/sizeof(parts[0])) parts[i]->resettracked(pl);
}

void render_particles(int time)
{
	//want to debug BEFORE the lastpass render (that would delete particles)
	if(debugparticles && !glaring && !reflecting && !refracting)
	{
		int n = sizeof(parts)/sizeof(parts[0]);
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(0, FONTH*n*2, FONTH*n*2, 0, -1, 1); //squeeze into top-left corner
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		defaultshader->set();
		loopi(n)
		{
			int type = parts[i]->type;
			const char *title = parts[i]->texname ? strrchr(parts[i]->texname, '/')+1 : NULL;
			string info = "";
			if(type&PT_GLARE) s_strcat(info, "g,");
			if(type&PT_SOFT) s_strcat(info, "s,");
			if(type&PT_LERP) s_strcat(info, "l,");
			if(type&PT_MOD) s_strcat(info, "m,");
			if(type&PT_RND4) s_strcat(info, "r,");
			if(type&PT_FLIP) s_strcat(info, "f,");
			if(parts[i]->collide) s_strcat(info, "c,");
			s_sprintfd(ds)("%d\t%s(%s%d) %s", parts[i]->count(), partnames[type&0xFF], info, parts[i]->grav, (title?title:""));
			draw_text(ds, FONTH, (i+n/2)*FONTH);
		}
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

	if(glaring && !particleglare) return;

	loopi(sizeof(parts)/sizeof(parts[0]))
	{
		if(glaring && !(parts[i]->type&PT_GLARE)) continue;
		parts[i]->update();
	}

	static float zerofog[4] = { 0, 0, 0, 1 };
	float oldfogc[4];
	bool rendered = false;
	uint lastflags = PT_LERP, flagmask = PT_LERP|PT_MOD;

	if(binddepthfxtex()) flagmask |= PT_SOFT;

	loopi(sizeof(parts)/sizeof(parts[0]))
	{
		partrenderer *p = parts[i];
		if(glaring && !(p->type&PT_GLARE)) continue;
		if(!p->haswork() && (!p->tex || p->tex->frames.length() <= 1)) continue;

		if(!rendered)
		{
			rendered = true;
			glDepthMask(GL_FALSE);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			if(glaring) setenvparamf("colorscale", SHPARAM_VERTEX, 4, particleglare, particleglare, particleglare, 1);
			else setenvparamf("colorscale", SHPARAM_VERTEX, 4, 1, 1, 1, 1);

			particleshader->set();
			glGetFloatv(GL_FOG_COLOR, oldfogc);
		}

		uint flags = p->type & flagmask;
		if(p->usesvertexarray()) flags |= 0x01; //0x01 = VA marker
		uint changedbits = (flags ^ lastflags);
		if(changedbits != 0x0000)
		{
			if(changedbits&0x01)
			{
				if(flags&0x01)
				{
					glEnableClientState(GL_VERTEX_ARRAY);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glEnableClientState(GL_COLOR_ARRAY);
				}
				else
				{
					glDisableClientState(GL_VERTEX_ARRAY);
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					glDisableClientState(GL_COLOR_ARRAY);
				}
			}
			if(changedbits&PT_LERP) glFogfv(GL_FOG_COLOR, (flags&PT_LERP) ? oldfogc : zerofog);
			if(changedbits&(PT_LERP|PT_MOD))
			{
				if(flags&PT_LERP) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				else if(flags&PT_MOD) glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
				else glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			}
			if(changedbits&PT_SOFT)
			{
				if(flags&PT_SOFT)
				{
					if(depthfxtex.target==GL_TEXTURE_RECTANGLE_ARB)
					{
						if(!depthfxtex.highprecision()) SETSHADER(particlesoft8rect);
						else SETSHADER(particlesoftrect);
					}
					else
					{
						if(!depthfxtex.highprecision()) SETSHADER(particlesoft8);
						else SETSHADER(particlesoft);
					}

					binddepthfxparams(depthfxpartblend);
				}
				else particleshader->set();
			}
			lastflags = flags;
		}
		p->render();
	}

	if(rendered)
	{
		if(lastflags&(PT_LERP|PT_MOD)) glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		if(!(lastflags&PT_LERP)) glFogfv(GL_FOG_COLOR, oldfogc);
		if(lastflags&0x01)
		{
			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			glDisableClientState(GL_COLOR_ARRAY);
		}
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
}

static inline particle *newparticle(const vec &o, const vec &d, int fade, int type, int color, float size, physent *pl = NULL)
{
	return parts[type]->addpart(o, d, fade, color, size, pl);
}

static void create(int type, int color, int fade, const vec &p, float size, physent *pl)
{
	if(camera1->o.dist(p) > maxparticledistance) return;
	float collidez = parts[type]->collide ? p.z - raycube(p, vec(0, 0, -1), COLLIDERADIUS, RAY_CLIPMAT) + COLLIDEERROR : -1;
	int fmin = 1;
	int fmax = fade*3;
	int f = fmin + rnd(fmax); //help deallocater by using fade distribution rather than random
	newparticle(p, vec(0, 0, 0), f, type, color, size, pl)->val = collidez;
}

static void regularcreate(int type, int color, int fade, const vec &p, float size, physent *pl, int delay=0)
{
	if(!emit_particles() || (delay > 0 && rnd(delay) != 0)) return;
	create(type, color, fade, p, size, pl);
}

static void splash(int type, int color, int radius, int num, int fade, const vec &p, float size)
{
	if(camera1->o.dist(p) > maxparticledistance) return;
	float collidez = parts[type]->collide ? p.z - raycube(p, vec(0, 0, -1), COLLIDERADIUS, RAY_CLIPMAT) + COLLIDEERROR : -1;
	int fmin = 1;
	int fmax = fade*3;
	loopi(num)
	{
		vec tmp(rnd(max(radius*2,1))-radius, rnd(max(radius*2,1))-radius, rnd(max(radius*2,1))-radius);
		int f = (num < 10) ? (fmin + rnd(fmax)) : (fmax - (i*(fmax-fmin))/(num-1)); //help deallocater by using fade distribution rather than random
		newparticle(p, tmp, f, type, color, size)->val = collidez;
	}
}

static void regularsplash(int type, int color, int radius, int num, int fade, const vec &p, float size, int delay=0)
{
	if(!emit_particles() || (delay > 0 && rnd(delay) != 0)) return;
	splash(type, color, radius, num, fade, p, size);
}

void regular_part_create(int type, int fade, const vec &p, int color, float size, physent *pl, int delay)
{
	if(shadowmapping || renderedgame) return;
	regularcreate(type, color, fade, p, size, pl, delay);
}

void part_create(int type, int fade, const vec &p, int color, float size, physent *pl)
{
	if(shadowmapping || renderedgame) return;
	create(type, color, fade, p, size, pl);
}

void regular_part_splash(int type, int num, int fade, const vec &p, int color, float size, int radius, int delay)
{
	if(shadowmapping || renderedgame) return;
	regularsplash(type, color, radius, num, fade, p, size, delay);
}

void part_splash(int type, int num, int fade, const vec &p, int color, float size, int radius)
{
	if(shadowmapping || renderedgame) return;
	splash(type, color, radius, num, fade, p, size);
}

void part_trail(int ptype, int fade, const vec &s, const vec &e, int color, float size)
{
	if(shadowmapping || renderedgame) return;
	vec v;
	float d = e.dist(s, v);
	int steps = clamp(int(d*2), 1, maxparticletrail);
	v.div(steps);
	vec p = s;
	loopi(steps)
	{
		p.add(v);
		vec tmp = vec(float(rnd(11)-5), float(rnd(11)-5), float(rnd(11)-5));
		newparticle(p, tmp, rnd(fade)+fade, ptype, color, size);
	}
}

void part_text(const vec &s, const char *t, int type, int fade, int color, float size)
{
	if(shadowmapping || renderedgame) return;
	if(!particletext || camera1->o.dist(s) > maxparticledistance) return;
	if(t[0]=='@') t = newstring(t);
	newparticle(s, vec(0, 0, 1), fade, type, color, size)->text = t;
}

void part_meter(const vec &s, float val, int type, int fade, int color, int color2, float size)
{
	if(shadowmapping || renderedgame) return;
	particle *p = newparticle(s, vec(0, 0, 1), fade, type, color, size);
	p->color2[0] = color2>>16;
	p->color2[1] = (color2>>8)&0xFF;
	p->color2[2] = color2&0xFF;
	p->progress = clamp(int(val*100), 0, 100);
}

void part_flare(const vec &p, const vec &dest, int fade, int type, int color, float size, physent *pl)
{
	if(shadowmapping || renderedgame) return;
	newparticle(p, dest, fade, type, color, size, pl);
}

void part_fireball(const vec &dest, float maxsize, int type, int fade, int color, float size)
{
	if(shadowmapping || renderedgame) return;
	float growth = maxsize - size;
	if(fade < 0) fade = int(growth*25);
	newparticle(dest, vec(0, 0, 1), fade, type, color, size)->val = growth;
}

void regular_part_fireball(const vec &dest, float maxsize, int type, int fade, int color, float size)
{
	if(shadowmapping || renderedgame || !emit_particles()) return;
	part_fireball(dest, maxsize, type, fade, color, size);
}

void part_spawn(const vec &o, const vec &v, float z, uchar type, int amt, int fade, int color, float size)
{
	if(shadowmapping || renderedgame) return;
	loopi(amt)
	{
		vec w(rnd(int(v.x*2))-int(v.x), rnd(int(v.y*2))-int(v.y), rnd(int(v.z*2))-int(v.z)+z);
		w.add(o);
		part_splash(type, 1, fade, w, color, size, 1);
	}
}

void part_flares(const vec &o, const vec &v, float z1, const vec &d, const vec &w, float z2, uchar type, int amt, int fade, int color, float size, physent *pl)
{
	if(shadowmapping || renderedgame) return;
	loopi(amt)
	{
		vec from(rnd(int(v.x*2))-int(v.x), rnd(int(v.y*2))-int(v.y), rnd(int(v.z*2))-int(v.z)+z1);
		from.add(o);

		vec to(rnd(int(w.x*2))-int(w.x), rnd(int(w.y*2))-int(w.y), rnd(int(w.z*2))-int(w.z)+z1);
		to.add(d);

		newparticle(from, to, fade, type, color, size, pl);
	}
}

void part_portal(const vec &o, float size, float yaw, float pitch, int type, int fade, int color)
{
	if(shadowmapping || renderedgame) return;
	portalrenderer *p = dynamic_cast<portalrenderer *>(parts[type]);
	if(p) p->addportal(o, fade, color, size, yaw, pitch);
}

void part_icon(const vec &o, Texture *tex, float blend, float size, int fade, int color, int type)
{
	if(shadowmapping || renderedgame) return;
	iconrenderer *p = dynamic_cast<iconrenderer *>(parts[type]);
	if(p) p->addicon(o, tex, blend, fade, color, size);
}

//dir = 0..6 where 0=up
static inline vec offsetvec(vec o, int dir, int dist)
{
	vec v = vec(o);
	v[(2+dir)%3] += (dir>2)?(-dist):dist;
	return v;
}

//converts a 16bit color to 24bit
static inline int colorfromattr(int attr)
{
	return (((attr&0xF)<<4) | ((attr&0xF0)<<8) | ((attr&0xF00)<<12)) + 0x0F0F0F;
}

/* Experiments in shapes...
 * dir: (where dir%3 is similar to offsetvec with 0=up)
 * 0..2 circle
 * 3.. 5 cylinder shell
 * 6..11 cone shell
 * 12..14 plane volume
 * 15..20 line volume, i.e. wall
 * 21 sphere
 * +32 to inverse direction
 */
void regularshape(int type, int radius, int color, int dir, int num, int fade, const vec &p, float size, float vel)
{
	if(!emit_particles()) return;

	int basetype = parts[type]->type&0xFF;
	bool flare = (basetype == PT_TAPE) || (basetype == PT_LIGHTNING),
		 inv = (dir&0x20)!=0, taper = (dir&0x40)!=0;
	dir &= 0x1F;
	loopi(num)
	{
		vec to, from;
		if(dir < 12)
		{
			float a = PI2*float(rnd(1000))/1000.0;
			to[dir%3] = sinf(a)*radius;
			to[(dir+1)%3] = cosf(a)*radius;
			to[(dir+2)%3] = 0.0;
			to.add(p);
			if(dir < 3) //circle
				from = p;
			else if(dir < 6) //cylinder
			{
				from = to;
				to[(dir+2)%3] += radius;
				from[(dir+2)%3] -= radius;
			}
			else //cone
			{
				from = p;
				to[(dir+2)%3] += (dir < 9)?radius:(-radius);
			}
		}
		else if(dir < 15) //plane
		{
			to[dir%3] = float(rnd(radius<<4)-(radius<<3))/8.0;
			to[(dir+1)%3] = float(rnd(radius<<4)-(radius<<3))/8.0;
			to[(dir+2)%3] = radius;
			to.add(p);
			from = to;
			from[(dir+2)%3] -= 2*radius;
		}
		else if(dir < 21) //line
		{
			if(dir < 18)
			{
				to[dir%3] = float(rnd(radius<<4)-(radius<<3))/8.0;
				to[(dir+1)%3] = 0.0;
			}
			else
			{
				to[dir%3] = 0.0;
				to[(dir+1)%3] = float(rnd(radius<<4)-(radius<<3))/8.0;
			}
			to[(dir+2)%3] = 0.0;
			to.add(p);
			from = to;
			to[(dir+2)%3] += radius;
		}
		else //sphere
		{
			to = vec(PI2*float(rnd(1000))/1000.0, PI*float(rnd(1000)-500)/1000.0).mul(radius);
			to.add(p);
			from = p;
		}

		if(taper)
		{
			vec o = inv ? to : from;
			o.sub(camera1->o);
			float dist = clamp(sqrtf(o.x*o.x + o.y*o.y)/maxparticledistance, 0.0f, 1.0f);
			if(dist > 0.2f)
			{
				dist = 1 - (dist - 0.2f)/0.8f;
				if(rnd(0x10000) > dist*dist*0xFFFF) continue;
			}
		}

		if(flare)
			newparticle(inv?to:from, inv?from:to, rnd(fade*3)+1, type, color, size);
		else
		{
			vec d = vec(to).sub(from).normalize().mul(inv ? -vel : vel);
			particle *np = newparticle(inv?to:from, d, rnd(fade*3)+1, type, color, size);
			if(parts[type]->collide)
				np->val = (inv ? to.z : from.z) - raycube(inv ? to : from, vec(0, 0, -1), COLLIDERADIUS, RAY_CLIPMAT) + COLLIDEERROR;
		}
	}
}

void regularflame(int type, const vec &p, float radius, float height, int color, int density, int fade, float size, float vel)
{
	if(!emit_particles()) return;

	float s = size*min(radius, height);
	vec v(0, 0, min(1.0f, height)*vel);
	loopi(density)
	{
		vec q = vec(p).add(vec(rndscale(radius*2.f)-radius, rndscale(radius*2.f)-radius, 0));
		newparticle(q, v, rnd(max(int(fade*height), 1))+1, type, color, s);
	}
}

void makeparticle(const vec &o, int attr1, int attr2, int attr3, int attr4, int attr5)
{
	switch(attr1)
	{
		case 0: //fire
			regularsplash(PART_FIREBALL, 0xFFC8C8, 10, 1, 40, o, 4.8f);
			regularsplash(PART_SMOKE_LERP_SRISE, 0x897661, 2, 1, 200,  vec(o.x, o.y, o.z+3.0), 2.4f, 3);
			break;
		case 1: //smoke vent - <dir>
			regularsplash(PART_SMOKE_LERP_SRISE, 0x897661, 2, 1, 200,  offsetvec(o, attr2, rnd(10)), 2.4f);
			break;
		case 2: //water fountain - <dir>
		{
			uchar col[3];
			getwatercolour(col);
			int color = (col[0]<<16) | (col[1]<<8) | col[2];
			regularsplash(PART_WATER, color, 10, 4, 200, offsetvec(o, attr2, rnd(10)), 0.6f);
			break;
		}
		case 3: //fire ball - <size> <rgb>
			newparticle(o, vec(0, 0, 1), 1, PART_EXPLOSION, colorfromattr(attr3), 4.0f)->val = 1+attr2;
			break;
		case 4:  //tape - <dir> <length> <rgb>
		case 7:  //lightning
		case 8:  //fire
		case 9:  //smoke
		case 10: //water
		case 11: //plasma
		case 12: //snow
		case 13: //sparks
		{
			const int typemap[] = { PART_SFLARE, -1, -1, PART_LIGHTNING, PART_FIREBALL, PART_SMOKE_LERP_SRISE, PART_WATER, PART_PLASMA, PART_SNOW, PART_SPARK };
			const float sizemap[] = { 0.28f, 0.0f, 0.0f, 0.28f, 4.8f, 2.4f, 0.60f, 4.8f, 0.5f, 0.28f }, velmap[] = { 100, 0, 0, 200, 200, 200, 200, 200, 40, 200 };
			int type = typemap[attr1-4];
			float size = sizemap[attr1-4], vel = velmap[attr1-4];
			if(attr2 >= 256) regularshape(type, 1+attr3, colorfromattr(attr4), attr2-256, 5, attr5 > 0 ? attr5 : 200, o, size, vel);
			else newparticle(o, offsetvec(o, attr2, 1+attr3), attr5 > 0 ? attr5 : 1, type, colorfromattr(attr4), size);
			break;
		}
		case 14: // flames <radius> <height> <rgb>
		case 15: // smoke plume
		{
			const int typemap[] = { PART_FLAME, PART_SMOKE_SRISE }, fademap[] = { 500, 1000 }, densitymap[] = { 3, 1 };
			const float sizemap[] = { 2.f, 4.f }, velmap[] = { 100.f, 150.f };
			int type = typemap[attr1-14], density = densitymap[attr1-14], fade = attr5 > 0 ? attr5 : fademap[attr1-14];
			float size = sizemap[attr1-14], vel = velmap[attr1-14];
			regularflame(type, o, float(attr2)/100.0f, float(attr3)/100.0f, colorfromattr(attr4), density, fade, size, vel);
			break;
		}
		case 5: //meter, metervs - <percent> <rgb> <rgb2>
		case 6:
		{
			particle *p = newparticle(o, vec(0, 0, 1), 1, attr1==5 ? PART_METER : PART_METER_VS, colorfromattr(attr3), 2.f);
			int color2 = colorfromattr(attr4);
			p->color2[0] = color2>>16;
			p->color2[1] = (color2>>8)&0xFF;
			p->color2[2] = color2&0xFF;
			p->progress = clamp(attr2, 0, 100);
			break;
		}
		case 32: //lens flares - plain/sparkle/sun/sparklesun <red> <green> <blue>
		case 33:
		case 34:
		case 35:
			flares.addflare(o, attr2, attr3, attr4, (attr1&0x02)!=0, (attr1&0x01)!=0);
			break;
		default:
			s_sprintfd(ds)("@%d?", attr1);
			part_text(o, ds);
			break;
	}
}

void makeparticles(const entity &e)
{
	makeparticle(e.o, (int)e.attr[0], (int)e.attr[1], (int)e.attr[2], (int)e.attr[3], (int)e.attr[4]);
}

void updateparticles()
{
	if(lastmillis - lastemitframe >= emitmillis)
	{
		emit = true;
		lastemitframe = lastmillis - (lastmillis%emitmillis);
	}
	else emit = false;

	flares.setupflares();
	entities::drawparticles();
	if(flareparts) loopi(sizeof(parts)/sizeof(parts[0]))
	{
		if(!(parts[i]->type&PT_LENS)) continue;
		parts[i]->makeflares();
	}
	flares.drawflares(); // do after drawparticles so that we can make flares for them too
}

