#include "engine.h"

VARP(ffdynlights, 0, min(5, DYNLIGHTMASK), DYNLIGHTMASK);
VARP(maxdynlights, 0, min(3, MAXDYNLIGHTS), MAXDYNLIGHTS);
VARP(dynlightdist, 128, 1024, 10000);

struct dynlight
{
    vec o;
    float radius, initradius, curradius, dist;
    vec color, initcolor, curcolor;
    int fade, peak, expire, flags;
    physent *owner;

    void calcradius()
    {
        if(fade + peak)
        {
            int remaining = expire - lastmillis;
            if(flags&DL_EXPAND)
                curradius = initradius + (radius - initradius) * (1.0f - remaining/float(fade + peak));
            else if(remaining > fade)
                curradius = initradius + (radius - initradius) * (1.0f - float(remaining - fade)/peak);
            else if(flags&DL_SHRINK)
                curradius = (radius*remaining)/fade;
            else curradius = radius;
        }
        else curradius = radius;
    }

    void calccolor()
    {
        if(flags&DL_FLASH || !peak) curcolor = color;
        else
        {
            int peaking = expire - lastmillis - fade;
            if(peaking <= 0) curcolor = color;
            else curcolor.lerp(initcolor, color, 1.0f - float(peaking)/peak);
        }

        float intensity = 1.0f;
        if(!(flags&DL_FLASH) && fade)
        {
            int fading = expire - lastmillis;
            if(fading < fade) intensity = float(fading)/fade;
        }
        curcolor.mul(intensity);
        // KLUGE: this prevents nvidia drivers from trying to recompile dynlight fragment programs
        loopk(3) if(fmod(curcolor[k], 1.0f/256) < 0.001f) curcolor[k] += 0.001f;
    }
};

vector<dynlight> dynlights;
vector<dynlight *> closedynlights;

void adddynlight(const vec &o, float radius, const vec &color, int fade, int peak, int flags, float initradius, const vec &initcolor, physent *owner)
{
    if(renderpath==R_FIXEDFUNCTION ? !ffdynlights || maxtmus<3 : !maxdynlights) return;
    if(o.dist(camera1->o) > dynlightdist) return;

    int insert = 0, expire = fade + peak + lastmillis;
    loopvrev(dynlights) if(expire>=dynlights[i].expire) { insert = i+1; break; }
    dynlight d;
    d.o = o;
    d.radius = radius;
    d.initradius = initradius;
    d.color = color;
    d.initcolor = initcolor;
    d.fade = fade;
    d.peak = peak;
    d.expire = expire;
    d.flags = flags;
    d.owner = owner;
    dynlights.insert(insert, d);
}

void cleardynlights()
{
    int faded = -1;
    loopv(dynlights) if(lastmillis<dynlights[i].expire) { faded = i; break; }
    if(faded<0) dynlights.setsizenodelete(0);
    else if(faded>0) dynlights.remove(0, faded);
}

void removetrackeddynlights(physent *owner)
{
    loopvrev(dynlights) if(dynlights[i].owner == owner) dynlights.remove(i);
}

void updatedynlights()
{
    cleardynlights();
    game::adddynlights();

    loopv(dynlights)
    {
        dynlight &d = dynlights[i];
        if(d.owner) game::dynlighttrack(d.owner, d.o);
        d.calcradius();
        d.calccolor();
    }
}

int finddynlights()
{
    closedynlights.setsizenodelete(0);
    if(renderpath==R_FIXEDFUNCTION ? !ffdynlights || maxtmus<3 : !maxdynlights) return 0;
    physent e;
    e.type = ENT_CAMERA;
    e.collidetype = COLLIDE_AABB;
    loopvj(dynlights)
    {
        dynlight &d = dynlights[j];
        if(d.curradius <= 0) continue;
        d.dist = camera1->o.dist(d.o) - d.curradius;
        if(d.dist > dynlightdist || isfoggedsphere(d.curradius, d.o) || pvsoccluded(d.o, 2*int(d.curradius+1)))
            continue;
        if(reflecting || refracting > 0)
        {
            if(d.o.z + d.curradius < reflectz) continue;
        }
        else if(refracting < 0 && d.o.z - d.curradius > reflectz) continue;
        e.o = d.o;
        e.radius = e.height = e.aboveeye = d.curradius;
        if(collide(&e, vec(0, 0, 0), 0, false)) continue;

        int insert = 0;
        loopvrev(closedynlights) if(d.dist >= closedynlights[i]->dist) { insert = i+1; break; }
        closedynlights.insert(insert, &d);
        if(closedynlights.length() >= DYNLIGHTMASK) break;
    }
    if(renderpath==R_FIXEDFUNCTION && closedynlights.length() > ffdynlights)
        closedynlights.setsizenodelete(ffdynlights);
    return closedynlights.length();
}

bool getdynlight(int n, vec &o, float &radius, vec &color)
{
    if(!closedynlights.inrange(n)) return false;
    dynlight &d = *closedynlights[n];
    o = d.o;
    radius = d.curradius;
    color = d.curcolor;
    return true;
}

void dynlightreaching(const vec &target, vec &color, vec &dir)
{
    vec dyncolor(0, 0, 0);//, dyndir(0, 0, 0);
    loopv(dynlights)
    {
        dynlight &d = dynlights[i];
        if(d.curradius<=0) continue;

        vec ray(d.o);
        ray.sub(target);
        float mag = ray.magnitude();
        if(mag >= d.curradius) continue;

        vec color = d.curcolor;
        color.mul(1 - mag/d.curradius);
        dyncolor.add(color);
        //dyndir.add(ray.mul(intensity/mag));
    }
#if 0
    if(!dyndir.iszero())
    {
        dyndir.normalize();
        float x = dyncolor.magnitude(), y = color.magnitude();
        if(x+y>0)
        {
            dir.mul(x);
            dyndir.mul(y);
            dir.add(dyndir).div(x+y);
            if(dir.iszero()) dir = vec(0, 0, 1);
            else dir.normalize();
        }
    }
#endif
    color.add(dyncolor);
}

void calcdynlightmask(vtxarray *va)
{
    uint mask = 0;
    int offset = 0;
    loopv(closedynlights)
    {
        dynlight &d = *closedynlights[i];
        if(d.o.dist_to_bb(va->geommin, va->geommax) >= d.curradius) continue;

        mask |= (i+1)<<offset;
        offset += DYNLIGHTBITS;
        if(offset >= maxdynlights*DYNLIGHTBITS) break;
    }
    va->dynlightmask = mask;
}

int setdynlights(vtxarray *va, const ivec &vaorigin)
{
    if(closedynlights.empty() || !va->dynlightmask) return 0;

    static string vertexparams[MAXDYNLIGHTS] = { "" }, pixelparams[MAXDYNLIGHTS] = { "" };
    if(!*vertexparams[0]) loopi(MAXDYNLIGHTS)
    {
        formatstring(vertexparams[i])("dynlight%dpos", i);
        formatstring(pixelparams[i])("dynlight%dcolor", i);
    }

    int index = 0;
    float scale0 = 1;
    vec origin0(0, 0, 0);
    for(uint mask = va->dynlightmask; mask; mask >>= DYNLIGHTBITS, index++)
    {
        dynlight &d = *closedynlights[(mask&DYNLIGHTMASK)-1];

        float scale = 1.0f/d.curradius;
        vec origin = vaorigin.tovec().sub(d.o).mul(scale);
        setenvparamf(vertexparams[index], SHPARAM_VERTEX, 10+index, origin.x, origin.y, origin.z, scale/(1<<VVEC_FRAC));

        if(index<=0) { scale0 = scale; origin0 = origin; }
        else
        {
            scale /= scale0;
            origin.sub(vec(origin0).mul(scale));
            setenvparamf(vertexparams[index], SHPARAM_PIXEL, index-1, origin.x, origin.y, origin.z, scale);
        }

        setenvparamf(pixelparams[index], SHPARAM_PIXEL, 10+index, d.curcolor.x, d.curcolor.y, d.curcolor.z);
    }
    return index;
}

void makelightfx(extentity &e, extentity &f)
{
	if(f.attrs[0] && e.attrs[0] != LFX_SPOTLIGHT)
	{
		vec colour = vec(f.attrs[1], f.attrs[2], f.attrs[3]).div(255.f);
		float radius = f.attrs[0]; int millis = lastmillis-e.emit[2], effect = e.attrs[0], interval = e.emit[0]+e.emit[1];
		if(!e.emit[2] || millis >= interval) loopi(2)
		{
			e.emit[i] = e.attrs[i+2] ? e.attrs[i+2] : 750;
			if(e.attrs[4]&(1<<i)) e.emit[i] = rnd(e.emit[i]);
			millis -= interval; e.emit[2] = lastmillis-millis;
		}
		if(millis >= e.emit[0]) loopi(LFX_MAX-1) if(e.attrs[4]&(1<<(LFX_S_MAX+i))) { effect = i+1; break; }
		#define lightskew float skew = clamp(millis < e.emit[0] ? 1.f-(float(millis)/float(e.emit[0])) : float(millis-e.emit[0])/float(e.emit[1]), 0.f, 1.f);
		switch(effect)
		{
			case LFX_FLICKER:
			{
				if(millis >= e.emit[0]) radius -= (e.attrs[1] ? e.attrs[1] : radius);
				break;
			}
			case LFX_PULSE:
			{
				lightskew; radius -= (e.attrs[1] ? e.attrs[1] : radius)*skew;
				break;
			}
			case LFX_GLOW:
			{
				lightskew; colour.mul(skew); radius -= e.attrs[1]*skew;
				break;
			}
			default: break;
		}
		if(radius > 0) adddynlight(f.o, radius, colour);
	}
}
