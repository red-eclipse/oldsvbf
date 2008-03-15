// physics.cpp: no physics books were hurt nor consulted in the construction of this code.
// All physics computations and constants were invented on the fly and simply tweaked until
// they "felt right", and have no basis in reality. Collision detection is simplistic but
// very robust (uses discrete steps at fixed fps).

#include "pch.h"
#include "engine.h"

const int MAXCLIPPLANES = 1024;

clipplanes clipcache[MAXCLIPPLANES], *nextclip = clipcache;

static inline void setcubeclip(cube &c, int x, int y, int z, int size)
{
    if(!c.ext || !c.ext->clip || c.ext->clip->owner!=&c)
	{
        if(nextclip >= &clipcache[MAXCLIPPLANES]) nextclip = clipcache;
        ext(c).clip = nextclip;
        nextclip->owner = &c;
        genclipplanes(c, x, y, z, size, *nextclip);
        nextclip++;
	}
}

void freeclipplanes(cube &c)
{
	if(!c.ext || !c.ext->clip) return;
    if(c.ext->clip->owner==&c) c.ext->clip->owner = NULL;
	c.ext->clip = NULL;
}

/////////////////////////  ray - cube collision ///////////////////////////////////////////////

static inline void pushvec(vec &o, const vec &ray, float dist)
{
	vec d(ray);
	d.mul(dist);
	o.add(d);
}

static inline bool pointinbox(const vec &v, const vec &bo, const vec &br)
{
	return v.x <= bo.x+br.x &&
			v.x >= bo.x-br.x &&
			v.y <= bo.y+br.y &&
			v.y >= bo.y-br.y &&
			v.z <= bo.z+br.z &&
			v.z >= bo.z-br.z;
}

bool pointincube(const clipplanes &p, const vec &v)
{
	if(!pointinbox(v, p.o, p.r)) return false;
	loopi(p.size) if(p.p[i].dist(v)>1e-3f) return false;
	return true;
}

#define INTERSECTPLANES(setentry) \
    clipplanes &p = *c.ext->clip; \
    float enterdist = -1e16f, exitdist = 1e16f; \
    loopi(p.size) \
    { \
        float pdist = p.p[i].dist(o), facing = ray.dot(p.p[i]); \
        if(facing < 0) \
        { \
            pdist /= -facing; \
            if(pdist > enterdist) \
            { \
                if(pdist > exitdist) return false; \
                enterdist = pdist; \
                setentry; \
            } \
        } \
        else if(facing > 0) \
        { \
            pdist /= -facing; \
            if(pdist < exitdist) \
            { \
                if(pdist < enterdist) return false; \
                exitdist = pdist; \
            } \
        } \
        else if(pdist > 0) return false; \
    }

// optimized shadow version
bool shadowcubeintersect(const cube &c, const vec &o, const vec &ray, float &dist)
{
    INTERSECTPLANES({});
    if(exitdist < 0) return false;
    enterdist += 0.1f;
    if(enterdist < 0) enterdist = 0;
    if(!pointinbox(vec(ray).mul(enterdist).add(o), p.o, p.r)) return false;
    dist = enterdist;
    return true;
}

vec hitsurface;

bool raycubeintersect(const cube &c, const vec &o, const vec &ray, float &dist)
{
    int entry = -1, bbentry = -1;
    INTERSECTPLANES(entry = i);
    loop(i, 3)
    {
        if(ray[i])
        {
            float pdist = ((ray[i] > 0 ? p.o[i]-p.r[i] : p.o[i]+p.r[i]) - o[i]) / ray[i];
            if(pdist > enterdist)
            {
                if(pdist > exitdist) return false;
                enterdist = pdist;
                bbentry = i;
            }
            pdist += 2*p.r[i]/fabs(ray[i]);
            if(pdist < exitdist)
            {
                if(pdist < enterdist) return false;
                exitdist = pdist;
            }
         }
         else if(o[i] < p.o[i]-p.r[i] || o[i] > p.o[i]+p.r[i]) return false;
    }
    if(exitdist < 0) return false;
    dist = max(enterdist+0.1f, 0.0f);
    if(bbentry>=0) { hitsurface = vec(0, 0, 0); hitsurface[bbentry] = ray[bbentry]>0 ? -1 : 1; }
    else hitsurface = p.p[entry];
    return true;
}

extern void entselectionbox(const entity &e, vec &eo, vec &es);
extern int entselradius;
float hitentdist;
int hitent, hitorient;

static float disttoent(octaentities *oc, octaentities *last, const vec &o, const vec &ray, float radius, int mode, extentity *t)
{
	vec eo, es;
	int orient;
	float dist = 1e16f, f = 0.0f;
	if(oc == last || oc == NULL) return dist;
	const vector<extentity *> &ents = et->getents();

	#define entintersect(mask, type, func) {\
		if((mode&(mask))==(mask)) \
			loopv(oc->type) \
				if(!last || last->type.find(oc->type[i])<0) \
				{ \
					extentity &e = *ents[oc->type[i]]; \
					if(!e.inoctanode || &e==t) continue; \
					func; \
					if(f<dist && f>0) { \
						hitentdist = dist = f; \
						hitent = oc->type[i]; \
						hitorient = orient; \
					} \
				} \
	}

	entintersect(RAY_POLY, mapmodels,
		if(!e.visible && (mode&RAY_ENTS)!=RAY_ENTS) continue;
		orient = 0; // FIXME, not set
		if(!mmintersect(e, o, ray, radius, mode, f)) continue;
	);

	entintersect(RAY_ENTS, other,
		entselectionbox(e, eo, es);
		if(!rayrectintersect(eo, es, o, ray, f, orient)) continue;
	);

	entintersect(RAY_ENTS, mapmodels,
		entselectionbox(e, eo, es);
		if(!rayrectintersect(eo, es, o, ray, f, orient)) continue;
	);

	return dist;
}

// optimized shadow version
static float shadowent(octaentities *oc, octaentities *last, const vec &o, const vec &ray, float radius, int mode, extentity *t)
{
	float dist = 1e16f, f = 0.0f;
	if(oc == last || oc == NULL) return dist;
	const vector<extentity *> &ents = et->getents();
	loopv(oc->mapmodels) if(!last || last->mapmodels.find(oc->mapmodels[i])<0)
	{
		extentity &e = *ents[oc->mapmodels[i]];
		if(!e.inoctanode || &e==t) continue;
		if(!e.visible) continue;
		if(!mmintersect(e, o, ray, radius, mode, f)) continue;
		if(f>0 && f<dist) dist = f;
	}
	return dist;
}

#define INITRAYCUBE \
	octaentities *oclast = NULL; \
	float dist = 0, dent = mode&RAY_BB ? 1e16f : 1e14f; \
    vec v(o), invray(ray.x ? 1/ray.x : 1e16f, ray.y ? 1/ray.y : 1e16f, ray.z ? 1/ray.z : 1e16f); \
	static cube *levels[32]; \
    levels[worldscale] = worldroot; \
    int lshift = worldscale; \
    ivec lsizemask(invray.x>0 ? 1 : 0, invray.y>0 ? 1 : 0, invray.z>0 ? 1 : 0); \

#define CHECKINSIDEWORLD \
    if(!insideworld(o)) \
    { \
        float disttoworld = 0, exitworld = 1e16f; \
        loopi(3) \
        { \
            float c = v[i]; \
            if(c<0 || c>=hdr.worldsize) \
            { \
                float d = ((invray[i]>0?0:hdr.worldsize)-c)*invray[i]; \
                if(d<0) return (radius>0?radius:-1); \
                disttoworld = max(disttoworld, 0.1f + d); \
            } \
            float e = ((invray[i]>0?hdr.worldsize:0)-c)*invray[i]; \
            exitworld = min(exitworld, e); \
        } \
        if(disttoworld > exitworld) return (radius>0?radius:-1); \
        pushvec(v, ray, disttoworld); \
        dist += disttoworld; \
    }

#define DOWNOCTREE(disttoent, earlyexit) \
        cube *lc = levels[lshift]; \
		for(;;) \
		{ \
            lshift--; \
            lc += octastep(x, y, z, lshift); \
			if(lc->ext && lc->ext->ents && dent > 1e15f) \
			{ \
				dent = disttoent(lc->ext->ents, oclast, o, ray, radius, mode, t); \
				if(dent < 1e15f earlyexit) return min(dent, dist); \
				oclast = lc->ext->ents; \
			} \
			if(lc->children==NULL) break; \
			lc = lc->children; \
            levels[lshift] = lc; \
		}

#define FINDCLOSEST(xclosest, yclosest, zclosest) \
        float dx = (lo.x+(lsizemask.x<<lshift)-v.x)*invray.x, \
              dy = (lo.y+(lsizemask.y<<lshift)-v.y)*invray.y, \
              dz = (lo.z+(lsizemask.z<<lshift)-v.z)*invray.z; \
        float disttonext = dx; \
        xclosest; \
        if(dy < disttonext) { disttonext = dy; yclosest; } \
        if(dz < disttonext) { disttonext = dz; zclosest; } \
		disttonext += 0.1f; \
		pushvec(v, ray, disttonext); \
        dist += disttonext;

#define UPOCTREE(exitworld) \
		x = int(v.x); \
		y = int(v.y); \
		z = int(v.z); \
        uint diff = uint(lo.x^x)|uint(lo.y^y)|uint(lo.z^z); \
        if(diff >= uint(hdr.worldsize)) exitworld; \
        diff >>= lshift; \
        if(!diff) exitworld; \
        do \
		{ \
            lshift++; \
            diff >>= 1; \
        } while(diff);

float raycube(const vec &o, const vec &ray, float radius, int mode, int size, extentity *t)
{
	if(ray.iszero()) return 0;

    INITRAYCUBE;
    CHECKINSIDEWORLD;

    int closest = -1, x = int(v.x), y = int(v.y), z = int(v.z);
	for(;;)
	{
		DOWNOCTREE(disttoent, && (mode&RAY_SHADOW));

        int lsize = 1<<lshift;

		cube &c = *lc;
		if((dist>0 || !(mode&RAY_SKIPFIRST)) &&
			(((mode&RAY_CLIPMAT) && c.ext && isclipped(c.ext->material) && c.ext->material != MAT_CLIP) ||
			((mode&RAY_EDITMAT) && c.ext && c.ext->material != MAT_AIR) ||
			(!(mode&RAY_PASS) && lsize==size && !isempty(c)) ||
			isentirelysolid(c) ||
            dent < dist))
		{
            if(closest >= 0) { hitsurface = vec(0, 0, 0); hitsurface[closest] = ray[closest]>0 ? -1 : 1; }
			return min(dent, dist);
		}

        ivec lo(x&(~0<<lshift), y&(~0<<lshift), z&(~0<<lshift));

        if(!isempty(c))
		{
			float f = 0;
			setcubeclip(c, lo.x, lo.y, lo.z, lsize);
			if(raycubeintersect(c, v, ray, f) && (dist+f>0 || !(mode&RAY_SKIPFIRST)))
				return min(dent, dist+f);
		}

        FINDCLOSEST(closest = 0, closest = 1, closest = 2);

		if(radius>0 && dist>=radius) return min(dent, dist);

        UPOCTREE(return min(dent, radius>0 ? radius : dist));
	}
}

// optimized version for lightmap shadowing... every cycle here counts!!!
float shadowray(const vec &o, const vec &ray, float radius, int mode, extentity *t)
{
    INITRAYCUBE;
    CHECKINSIDEWORLD;

	int x = int(v.x), y = int(v.y), z = int(v.z);
	for(;;)
	{
		DOWNOCTREE(shadowent, );

		cube &c = *lc;
		if(isentirelysolid(c)) return dist;

        ivec lo(x&(~0<<lshift), y&(~0<<lshift), z&(~0<<lshift));

        if(!isempty(c))
		{
			float f = 0;
            setcubeclip(c, lo.x, lo.y, lo.z, 1<<lshift);
            if(shadowcubeintersect(c, v, ray, f)) return dist+f;
		}

        FINDCLOSEST( , , );

		if(dist>=radius) return dist;

        UPOCTREE(return radius);
	}
}

float rayent(const vec &o, vec &ray, vec &hitpos, float radius, int mode, int size, int &orient, int &ent)
{
	hitent = -1;
	float d = raycubepos(o, ray, hitpos, hitentdist = radius, mode, size);
	orient = hitorient;
	ent = (hitentdist == d) ? hitent : -1;
	return d;
}

float raycubepos(const vec &o, vec &ray, vec &hitpos, float radius, int mode, int size)
{
	ray.normalize();
	hitpos = ray;
	float dist = raycube(o, ray, radius, mode, size);
	hitpos.mul(dist);
	hitpos.add(o);
	return dist;
}

bool raycubelos(vec &o, vec &dest, vec &hitpos)
{
	vec ray(dest);
	ray.sub(o);
	float mag = ray.magnitude();
	float distance = raycubepos(o, ray, hitpos, mag, RAY_CLIPMAT|RAY_POLY);
	return distance >= mag;
}

float rayfloor(const vec &o, vec &floor, int mode, float radius)
{
	if(o.z<=0) return -1;
    hitsurface = vec(0, 0, 1);
	float dist = raycube(o, vec(0, 0, -1), radius, mode);
	if(dist<0 || (radius>0 && dist>=radius)) return dist;
    floor = hitsurface;
	return dist;
}

/////////////////////////  entity collision  ///////////////////////////////////////////////

// info about collisions
bool inside; // whether an internal collision happened
physent *hitplayer; // whether the collection hit a player
vec wall; // just the normal vectors.
float walldistance;

bool ellipsecollide(physent *d, const vec &dir, const vec &o, float yaw, float xr, float yr,  float hi, float lo)
{
    float below = (o.z-lo) - (d->o.z+d->height),
          above = (d->o.z-d->height) - (o.z+hi);
	if(below>=0 || above>=0) return true;
    float x = o.x - d->o.x, y = o.y - d->o.y;
    float angle = atan2f(y, x), dangle = angle-(d->yaw+90)*RAD, eangle = angle-(yaw+90)*RAD;
	float dx = d->xradius*cosf(dangle), dy = d->yradius*sinf(dangle);
    float ex = xr*cosf(eangle), ey = yr*sinf(eangle);
	float dist = sqrtf(x*x + y*y) - sqrtf(dx*dx + dy*dy) - sqrtf(ex*ex + ey*ey);
	if(dist < 0)
	{
        if(dir.iszero() || ((d->type>=ENT_INANIMATE || below >= -(d->height+d->aboveeye)/4.0f) && dir.z > 0))
		{
			wall = vec(0, 0, -1);
			return false;
		}
        if(dir.iszero() || ((d->type>=ENT_INANIMATE || above >= -(d->height+d->aboveeye)/3.0f) && dir.z < 0))
		{
			wall = vec(0, 0, 1);
			return false;
		}
		if(dir.iszero() || -x*dir.x + -y*dir.y < 0)
		{
			wall = vec(-x, -y, 0);
			wall.normalize();
			return false;
		}
	}
	return true;
}

bool rectcollide(physent *d, const vec &dir, const vec &o, float xr, float yr,  float hi, float lo, uchar visible, bool collideonly, float cutoff)
{
	if(collideonly && !visible) return true;
	vec s(d->o);
	s.sub(o);
    float dxr = d->collidetype==COLLIDE_ELLIPSE ? d->radius : d->xradius, dyr = d->collidetype==COLLIDE_ELLIPSE ? d->radius : d->yradius;
    xr += dxr;
    yr += dyr;
	walldistance = -1e10f;
	float zr = s.z>0 ? d->height+hi : d->aboveeye+lo;
	float ax = fabs(s.x)-xr;
	float ay = fabs(s.y)-yr;
	float az = fabs(s.z)-zr;
	if(ax>0 || ay>0 || az>0) return true;
	wall.x = wall.y = wall.z = 0;
#define TRYCOLLIDE(dim, ON, OP, N, P) \
	{ \
        if(s.dim<0) { if(visible&(1<<ON) && (dir.iszero() || (dir.dim>0 && (d->type>=ENT_INANIMATE || (N))))) { walldistance = a ## dim; wall.dim = -1; return false; } } \
        else if(visible&(1<<OP) && (dir.iszero() || (dir.dim<0 && (d->type>=ENT_INANIMATE || (P))))) { walldistance = a ## dim; wall.dim = 1; return false; } \
	}
    if(ax>ay && ax>az) TRYCOLLIDE(x, O_LEFT, O_RIGHT, ax > -dxr, ax > -dxr);
    if(ay>az) TRYCOLLIDE(y, O_BACK, O_FRONT, ay > -dyr, ay > -dyr);
	TRYCOLLIDE(z, O_BOTTOM, O_TOP,
         az >= d->zmargin-(d->height+d->aboveeye)/4.0f,
         az >= d->zmargin-(d->height+d->aboveeye)/3.0f);
	if(collideonly) inside = true;
	return collideonly;
}

#define DYNENTCACHESIZE 1024

static uint dynentframe = 0;

static struct dynentcacheentry
{
	int x, y;
	uint frame;
	vector<physent *> dynents;
} dynentcache[DYNENTCACHESIZE];

void cleardynentcache()
{
	dynentframe++;
	if(!dynentframe || dynentframe == 1) loopi(DYNENTCACHESIZE) dynentcache[i].frame = 0;
	if(!dynentframe) dynentframe = 1;
}

VARF(dynentsize, 4, 7, 12, cleardynentcache());

#define DYNENTHASH(x, y) (((((x)^(y))<<5) + (((x)^(y))>>5)) & (DYNENTCACHESIZE - 1))

const vector<physent *> &checkdynentcache(int x, int y)
{
	dynentcacheentry &dec = dynentcache[DYNENTHASH(x, y)];
	if(dec.x == x && dec.y == y && dec.frame == dynentframe) return dec.dynents;
	dec.x = x;
	dec.y = y;
	dec.frame = dynentframe;
	dec.dynents.setsize(0);
	int numdyns = cl->numdynents(), dsize = 1<<dynentsize, dx = x<<dynentsize, dy = y<<dynentsize;
	loopi(numdyns)
	{
		dynent *d = cl->iterdynents(i);
		if(!d || d->state != CS_ALIVE ||
			d->o.x+d->radius <= dx || d->o.x-d->radius >= dx+dsize ||
			d->o.y+d->radius <= dy || d->o.y-d->radius >= dy+dsize)
			continue;
		dec.dynents.add(d);
	}
	return dec.dynents;
}

#define loopdynentcache(curx, cury, o, radius) \
    for(int curx = max(int(o.x-radius), 0)>>dynentsize, endx = min(int(o.x+radius), hdr.worldsize-1)>>dynentsize; curx <= endx; curx++) \
    for(int cury = max(int(o.y-radius), 0)>>dynentsize, endy = min(int(o.y+radius), hdr.worldsize-1)>>dynentsize; cury <= endy; cury++)

void updatedynentcache(physent *d)
{
    loopdynentcache(x, y, d->o, d->radius)
	{
		dynentcacheentry &dec = dynentcache[DYNENTHASH(x, y)];
		if(dec.x != x || dec.y != y || dec.frame != dynentframe || dec.dynents.find(d) >= 0) continue;
		dec.dynents.add(d);
	}
}

bool overlapsdynent(const vec &o, float radius)
{
    loopdynentcache(x, y, o, radius)
	{
		const vector<physent *> &dynents = checkdynentcache(x, y);
		loopv(dynents)
		{
			physent *d = dynents[i];
			if(o.dist(d->o)-d->radius < radius) return true;
		}
	}
	return false;
}

bool plcollide(physent *d, const vec &dir)	// collide with player or monster
{
	if(d->type==ENT_CAMERA || d->state!=CS_ALIVE) return true;
    loopdynentcache(x, y, d->o, d->radius)
	{
		const vector<physent *> &dynents = checkdynentcache(x, y);
		loopv(dynents)
		{
			physent *o = dynents[i];
			if(o==d || d->o.reject(o->o, d->radius+o->radius)) continue;
            if(d->collidetype!=COLLIDE_ELLIPSE || o->collidetype!=COLLIDE_ELLIPSE)
            {
                if(!rectcollide(d, dir, o->o, o->collidetype==COLLIDE_ELLIPSE ? o->radius : o->xradius, o->collidetype==COLLIDE_ELLIPSE ? o->radius : o->yradius, o->aboveeye, o->height))
                {
                    hitplayer = o;
                    if((d->type==ENT_AI || d->type==ENT_INANIMATE) && wall.z>0) d->onplayer = o;
                    return false;
                }
            }
            else if(!ellipsecollide(d, dir, o->o, o->yaw, o->xradius, o->yradius, o->aboveeye, o->height))
			{
				hitplayer = o;
                if((d->type==ENT_AI || d->type==ENT_INANIMATE) && wall.z>0) d->onplayer = o;
				return false;
			}
		}
	}
	return true;
}

void rotatebb(vec &center, vec &radius, int yaw)
{
    yaw = (yaw+7)-(yaw+7)%15;
    yaw = 180-yaw;
    if(yaw<0) yaw += 360;
    switch(yaw)
    {
        case 0: break;
        case 180:
            center.x = -center.x;
            center.y = -center.y;
            break;
        case 90:
            swap(radius.x, radius.y);
            swap(center.x, center.y);
            center.x = -center.x;
            break;
        case 270:
            swap(radius.x, radius.y);
            swap(center.x, center.y);
            center.y = -center.y;
            break;
        default:
            radius.x = radius.y = max(radius.x, radius.y) + max(fabs(center.x), fabs(center.y));
            center.x = center.y = 0.0f;
            break;
    }
}

bool mmcollide(physent *d, const vec &dir, octaentities &oc)               // collide with a mapmodel
{
    const vector<extentity *> &ents = et->getents();
    loopv(oc.mapmodels)
    {
        extentity &e = *ents[oc.mapmodels[i]];
        if(!e.visible) continue;
        model *m = loadmodel(NULL, e.attr2);
        if(!m || !m->collide) continue;
        vec center, radius;
        m->collisionbox(0, center, radius);
        if(!m->ellipsecollide || d->collidetype!=COLLIDE_ELLIPSE)
        {
            rotatebb(center, radius, e.attr1);
            if(!rectcollide(d, dir, center.add(e.o), radius.x, radius.y, radius.z, radius.z)) return false;
        }
        else if(!ellipsecollide(d, dir, center.add(e.o), float((e.attr1+7)-(e.attr1+7)%15), radius.x, radius.y, radius.z, radius.z)) return false;
    }
    return true;
}

bool cubecollide(physent *d, const vec &dir, float cutoff, cube &c, int x, int y, int z, int size, bool solid) // collide with cube geometry
{
    if(solid || isentirelysolid(c))
	{
		int s2 = size>>1;
		vec o = vec(x+s2, y+s2, z+s2);
		vec r = vec(s2, s2, s2);
		return rectcollide(d, dir, o, r.x, r.y, r.z, r.z, isentirelysolid(c) ? (c.ext ? c.ext->visible : 0) : 0xFF, true, cutoff);
	}

	setcubeclip(c, x, y, z, size);
	clipplanes &p = *c.ext->clip;

	float r = d->radius,
		  zr = (d->aboveeye+d->height)/2;
	vec o(d->o), *w = &wall;
	o.z += zr - d->height;

	if(rectcollide(d, dir, p.o, p.r.x, p.r.y, p.r.z, p.r.z, c.ext->visible, !p.size, cutoff)) return true;

	if(p.size)
	{
		if(!wall.iszero())
		{
			vec wo(o), wrad(r, r, zr);
			loopi(3) if(wall[i]) { wo[i] = p.o[i]+wall[i]*p.r[i]; wrad[i] = 0; break; }
			loopi(p.size)
			{
				plane &f = p.p[i];
				if(!wall.dot(f)) continue;
				if(f.dist(wo) >= vec(f.x*wrad.x, f.y*wrad.y, f.z*wrad.z).magnitude())
				{
					wall = vec(0, 0, 0);
					walldistance = -1e10f;
					break;
				}
			}
		}
		float m = walldistance;
		loopi(p.size)
		{
			plane &f = p.p[i];
			float dist = f.dist(o) - vec(f.x*r, f.y*r, f.z*zr).magnitude();
			if(dist > 0) return true;
			if(dist > m)
			{
                if(!dir.iszero())
                {
                    if(f.dot(dir) >= -cutoff*dir.magnitude()) continue;
                    if(d->type<ENT_CAMERA &&
                        dist < (dir.z*f.z < 0 ?
                            d->zmargin-(d->height+d->aboveeye)/(dir.z < 0 ? 3.0f : 4.0f) :
                            ((dir.x*f.x < 0 || dir.y*f.y < 0) ? -r : 0)))
                        continue;
                }
                if(f.x && (f.x>0 ? o.x-p.o.x : p.o.x-o.x) + p.r.x - r < dist &&
                    f.dist(vec(p.o.x + (f.x>0 ? -p.r.x : p.r.x), o.y, o.z)) >= vec(0, f.y*r, f.z*zr).magnitude())
                    continue;
                if(f.y && (f.y>0 ? o.y-p.o.y : p.o.y-o.y) + p.r.y - r < dist &&
                    f.dist(vec(o.x, p.o.y + (f.y>0 ? -p.r.y : p.r.y), o.z)) >= vec(f.x*r, 0, f.z*zr).magnitude())
                    continue;
                if(f.z && (f.z>0 ? o.z-p.o.z : p.o.z-o.z) + p.r.z - zr < dist &&
                    f.dist(vec(o.x, o.y, p.o.z + (f.z>0 ? -p.r.z : p.r.z))) >= vec(f.x*r, f.y*r, 0).magnitude())
                    continue;
                w = &f;
				m = dist;
			}
		}
		wall = *w;
		if(wall.iszero())
		{
			inside = true;
			return true;
		}
	}
	return false;
}

static inline bool octacollide(physent *d, const vec &dir, float cutoff, const ivec &bo, const ivec &bs, cube *c, const ivec &cor, int size) // collide with octants
{
	loopoctabox(cor, size, bo, bs)
	{
        if(c[i].ext && c[i].ext->ents) if(!mmcollide(d, dir, *c[i].ext->ents)) return false;
		ivec o(i, cor.x, cor.y, cor.z, size);
		if(c[i].children)
		{
			if(!octacollide(d, dir, cutoff, bo, bs, c[i].children, o, size>>1)) return false;
		}
        else
        {
            bool solid = false;
            if(c[i].ext) switch(c[i].ext->material)
			{
                case MAT_NOCLIP: continue;
                case MAT_AICLIP: if(d->type==ENT_AI) solid = true; break;
                case MAT_CLIP: if(d->type<ENT_CAMERA) solid = true; break;
                case MAT_GLASS: solid = true; break;
            }
            if(!solid && isempty(c[i])) continue;
            if(!cubecollide(d, dir, cutoff, c[i], o.x, o.y, o.z, size, solid)) return false;
		}
	}
	return true;
}

static inline bool octacollide(physent *d, const vec &dir, float cutoff, const ivec &bo, const ivec &bs)
{
    int diff = (bo.x^(bo.x+bs.x)) | (bo.y^(bo.y+bs.y)) | (bo.z^(bo.z+bs.z)),
        scale = worldscale-1;
    if(diff&~((1<<scale)-1) || uint(bo.x|bo.y|bo.z|bo.x+bs.x|bo.y+bs.y|bo.z+bs.z) >= uint(hdr.worldsize))
       return octacollide(d, dir, cutoff, bo, bs, worldroot, ivec(0, 0, 0), hdr.worldsize>>1);
    cube *c = &worldroot[octastep(bo.x, bo.y, bo.z, scale)];
    if(c->ext && c->ext->ents && !mmcollide(d, dir, *c->ext->ents)) return false;
    scale--;
    while(c->children && !(diff&(1<<scale)))
    {
        c = &c->children[octastep(bo.x, bo.y, bo.z, scale)];
        if(c->ext && c->ext->ents && !mmcollide(d, dir, *c->ext->ents)) return false;
        scale--;
    }
    if(c->children) return octacollide(d, dir, cutoff, bo, bs, c->children, ivec(bo).mask(~((2<<scale)-1)), 1<<scale);
    bool solid = false;
    if(c->ext) switch(c->ext->material)
    {
        case MAT_NOCLIP: return true;
        case MAT_AICLIP: if(d->type==ENT_AI) solid = true; break;
        case MAT_CLIP: if(d->type<ENT_CAMERA) solid = true; break;
        case MAT_GLASS: solid = true; break;
    }
    if(!solid && isempty(*c)) return true;
    int csize = 2<<scale, cmask = ~(csize-1);
    return cubecollide(d, dir, cutoff, *c, bo.x&cmask, bo.y&cmask, bo.z&cmask, csize, solid);
}

// all collision happens here
bool collide(physent *d, const vec &dir, float cutoff, bool playercol)
{
	inside = false;
	hitplayer = NULL;
	wall.x = wall.y = wall.z = 0;
	ivec bo(int(d->o.x-d->radius), int(d->o.y-d->radius), int(d->o.z-d->height)),
		 bs(int(d->radius)*2, int(d->radius)*2, int(d->height+d->aboveeye));
	bs.add(2);  // guard space for rounding errors
    if(!octacollide(d, dir, cutoff, bo, bs)) return false;//, worldroot, ivec(0, 0, 0), hdr.worldsize>>1)) return false; // collide with world
    return !playercol || plcollide(d, dir);
}

void phystest()
{
	static const char *states[] = {"float", "fall", "slide", "slope", "floor", "step up", "step down", "bounce"};
//    printf ("PHYS(pl): %s, air %d, floor: (%f, %f, %f), vel: (%f, %f, %f), g: (%f, %f, %f)\n", states[player->physstate], player->timeinair, player->floor.x, player->floor.y, player->floor.z, player->vel.x, player->vel.y, player->vel.z, player->falling.x, player->falling.y, player->falling.z);
    printf ("PHYS(cam): %s, air %d, floor: (%f, %f, %f), vel: (%f, %f, %f), g: (%f, %f, %f)\n", states[camera1->physstate], camera1->timeinair, camera1->floor.x, camera1->floor.y, camera1->floor.z, camera1->vel.x, camera1->vel.y, camera1->vel.z, camera1->falling.x, camera1->falling.y, camera1->falling.z);
}

COMMAND(phystest, "");

void vecfromyawpitch(float yaw, float pitch, int move, int strafe, vec &m)
{
	if(move)
	{
		m.x = move*sinf(RAD*yaw);
		m.y = move*-cosf(RAD*yaw);
	}
	else m.x = m.y = 0;

	if(pitch)
	{
		m.x *= cosf(RAD*pitch);
		m.y *= cosf(RAD*pitch);
		m.z = move*sinf(RAD*pitch);
	}
	else m.z = 0;

	if(strafe)
	{
		m.x += strafe*-cosf(RAD*yaw);
		m.y += strafe*-sinf(RAD*yaw);
	}
}

void vectoyawpitch(const vec &v, float &yaw, float &pitch)
{
	yaw = -(float)atan2(v.x, v.y)/RAD + 180;
	pitch = asin(v.z/v.magnitude())/RAD;
}

bool intersect(physent *d, vec &from, vec &to)	// if lineseg hits entity bounding box
{
	vec v = to, w = d->o, *p;
	v.sub(from);
	w.sub(from);
	float c1 = w.dot(v);

	if(c1<=0) p = &from;
	else
	{
		float c2 = v.dot(v);
		if(c2<=c1) p = &to;
		else
		{
			float f = c1/c2;
			v.mul(f);
			v.add(from);
			p = &v;
		}
	}

	return p->x <= d->o.x+d->radius
		&& p->x >= d->o.x-d->radius
		&& p->y <= d->o.y+d->radius
		&& p->y >= d->o.y-d->radius
		&& p->z <= d->o.z+d->aboveeye - (d->crouching ? (1-CROUCHHEIGHT)*d->height : 0)
		&& p->z >= d->o.z-d->height;
}

bool insidesphere(vec &d, float h1, float r1, vec &v, float h2, float r2)
{
	return d.x <= v.x+r2+r1 && d.x >= v.x-r2-r1 &&
	d.y <= v.y+r2+r1 && d.y >= v.y-r2-r1 &&
	d.z <= v.z+h2+h1 && d.z >= v.z-h2-h1;
}
