#include "game.h"
namespace physics
{
	FVARW(crawlspeed,		0, 20.f, 10000);	// crawl speed
	FVARW(gravity,			0, 50.f, 10000);	// gravity
	FVARW(jumpspeed,		0, 50.f, 10000);	// extra velocity to add when jumping
	FVARW(movespeed,		0, 50.f, 10000);	// speed

	FVARW(impulsespeed,		0, 30.f, 10000);	// extra velocity to add when impulsing
	VARW(impulsedelay,		0, 3000, INT_MAX-1);	// impulse delay interval

	FVARW(liquidspeed,		0, 0.85f, 1);
	FVARW(liquidcurb,		0, 10.f, 10000);
	FVARW(floorcurb,		0, 5.f, 10000);
	FVARW(aircurb,			0, 25.f, 10000);

	FVARW(stairheight,		0, 4.1f, 10000);
	FVARW(floorz,			0, 0.867f, 1);
	FVARW(slopez,			0, 0.5f, 1);
	FVARW(wallz,			0, 0.2f, 1);
	FVARW(stepspeed,		1e-3f, 1.f, 10000);
	FVARW(ladderspeed,		1e-3f, 1.f, 10000);

	FVARP(floatspeed,		1e-3f, 80.f, 10000);
	FVARP(floatcurb,        0, 1.f, 10000);

	VARP(physframetime,		5, 5, 20);
	VARP(physinterp,		0, 1, 1);

	int physsteps = 0, lastphysframe = 0;

	#define imov(name,v,d,s,os) \
		void do##name(bool down) \
		{ \
			game::player1->s = down; \
			game::player1->v = game::player1->s ? d : (game::player1->os ? -(d) : 0); \
		} \
		ICOMMAND(name, "D", (int *down), { do##name(*down!=0); });

	imov(backward, move,   -1, k_down,  k_up);
	imov(forward,  move,    1, k_up,    k_down);
	imov(left,     strafe,  1, k_left,  k_right);
	imov(right,    strafe, -1, k_right, k_left);

	// inputs
	#define iput(x,y,t,q,z,a,f) \
		void do##x(bool down) \
		{ \
			if(game::allowmove(game::player1)) \
			{ \
				if(q) \
				{ \
					if(a > 0) \
					{ \
						if(game::player1->y != down) \
						{ \
							if(game::player1->t >= 0) game::player1->t = lastmillis-max(a-(lastmillis-game::player1->t), 0); \
							else if(down) game::player1->t = -game::player1->t; \
						} \
					} \
					else if(a < 0 && !down) return; \
					else if(down) \
					{ \
						if(a < 0 && game::player1->y) down = false; \
						else game::player1->t = lastmillis; \
					} \
				} \
				game::player1->y = down; \
				f; \
			} \
			else \
			{ \
				if(q && a > 0 && game::player1->y && game::player1->t >= 0) \
					game::player1->t = lastmillis-max(a-(lastmillis-game::player1->t), 0); \
				game::player1->y = false; \
				if(z && down) game::respawn(game::player1); \
			} \
		} \
		ICOMMAND(x, "D", (int *n), { do##x(*n!=0); });

	iput(crouch,	crouching,	crouchtime,	true,						false,	CROUCHTIME, );
	iput(impulse,	impulsing,	impulsetime,true,						false,	0, );
	iput(jump,		jumping,	jumptime,	!game::player1->timeinair,	false,	0, );
	iput(attack,	attacking,	attacktime,	true,						true,	0,	{ if(down) game::player1->reloading = game::player1->useaction = false; });
	iput(reload,	reloading,	reloadtime,	true,						false,	-1,	{ if(down) game::player1->useaction = game::player1->attacking = false; });
	iput(action,	useaction,	usetime,	true,						false,	0,	{ if(down) game::player1->reloading = game::player1->attacking = false; });

	void taunt(gameent *d)
	{
        if(d->state!=CS_ALIVE || d->physstate<PHYS_SLOPE) return;
		if(lastmillis-d->lasttaunt<1000) return;
		d->lasttaunt = lastmillis;
		client::addmsg(SV_TAUNT, "ri", d->clientnum);
	}
	ICOMMAND(taunt, "", (), { taunt(game::player1); });

	bool issolid(physent *d)
	{
		if(d->state == CS_ALIVE)
		{
			if(d->type == ENT_PLAYER && ((gameent *)d)->protect(lastmillis, spawnprotecttime*1000))
				return false;
			return true;
		}
        return d->state == CS_DEAD || d->state == CS_WAITING;
	}

	bool iscrouching(physent *d) { return d->crouching || d->crouchtime < 0 || lastmillis-d->crouchtime <= 200; }
	bool liquidcheck(physent *d) { return d->inliquid && d->submerged > 0.8f; }

	float liquidmerge(physent *d, float from, float to)
	{
		if(d->inliquid)
		{
			if(d->physstate >= PHYS_SLIDE && d->submerged < 1.f)
				return from-((from-to)*d->submerged);
			else return to;
		}
		return from;
	}

	float jumpforce(physent *d, bool liquid) { return m_speedscale(jumpspeed)*(d->weight/100.f)*(liquid ? liquidmerge(d, 1.f, liquidspeed) : 1.f)*jumpscale; }
	float impulseforce(physent *d) { return m_speedscale(impulsespeed)*(d->weight/100.f)*jumpscale; }
	float gravityforce(physent *d) { return m_speedscale(m_speedscale(gravity))*(d->weight/100.f)*gravityscale; }

	float stepforce(physent *d, bool up)
	{
		if(up && d->onladder) return ladderspeed;
		if(d->physstate > PHYS_FALL) return stepspeed;
		return 1.f;
	}

	bool canimpulse(physent *d)
	{
		if(impulsedelay > 0) return lastmillis-d->lastimpulse > m_speedtime(impulsedelay);
		return false;
	}

	float movevelocity(physent *d)
	{
		if(d->type == ENT_CAMERA) return game::player1->maxspeed*(game::player1->weight/100.f)*(floatspeed/100.0f);
		else if(d->type == ENT_PLAYER)
		{
			if(d->state == CS_EDITING || d->state == CS_SPECTATOR) return d->maxspeed*(d->weight/100.f)*(floatspeed/100.0f);
			else
			{
				float speed = iscrouching(d) ? crawlspeed : movespeed;
				if(impulsedelay > 0 && d->lastimpulse && d->impulsing)
				{
					int millis = lastmillis-d->lastimpulse;
					if(millis < impulsedelay) speed += impulsespeed*(1.f-clamp(millis/float(impulsedelay), 0.f, 1.f));
				}
				return m_speedscale(d->maxspeed)*(d->weight/100.f)*(speed/100.f);
			}
		}
		return m_speedscale(d->maxspeed);
	}

	bool movepitch(physent *d) { return d->type == ENT_CAMERA || d->state == CS_EDITING || d->state == CS_SPECTATOR; }

    void recalcdir(physent *d, const vec &oldvel, vec &dir)
    {
        float speed = oldvel.magnitude();
        if(speed > 1e-6f)
        {
            float step = dir.magnitude();
            dir = d->vel;
            dir.add(d->falling);
            dir.mul(step/speed);
        }
    }

    void slideagainst(physent *d, vec &dir, const vec &obstacle, bool foundfloor, bool slidecollide)
    {
        vec wall(obstacle);
        if(foundfloor ? wall.z > 0 : slidecollide)
        {
            wall.z = 0;
            if(!wall.iszero()) wall.normalize();
        }
        vec oldvel(d->vel);
        oldvel.add(d->falling);
        d->vel.project(wall);
        d->falling.project(wall);
        recalcdir(d, oldvel, dir);
    }

    void switchfloor(physent *d, vec &dir, const vec &floor)
    {
        if(floor.z >= floorz) d->falling = vec(0, 0, 0);

        vec oldvel(d->vel);
        oldvel.add(d->falling);
        if(dir.dot(floor) >= 0)
        {
            if(d->physstate < PHYS_SLIDE || fabs(dir.dot(d->floor)) > 0.01f*dir.magnitude()) return;
            d->vel.projectxy(floor, 0.0f);
        }
        else d->vel.projectxy(floor);
        d->falling.project(floor);
        recalcdir(d, oldvel, dir);
    }

    bool trystepup(physent *d, vec &dir, const vec &obstacle, float maxstep, const vec &floor)
    {
        vec old(d->o), stairdir = (obstacle.z >= 0 && obstacle.z < slopez ? vec(-obstacle.x, -obstacle.y, 0) : vec(dir.x, dir.y, 0)).rescale(1);
        float force = stepforce(d, true);
		if(d->onladder)
		{
			vec laddir = vec(stairdir).add(vec(0, 0, maxstep)).mul(0.1f*force);
            loopi(2)
            {
				d->o.add(laddir);
            	if(collide(d))
				{
					if(d->physstate == PHYS_FALL || d->floor != floor)
					{
						d->timeinair = 0;
						d->floor = vec(0, 0, 1);
						switchfloor(d, dir, d->floor);
					}
					d->physstate = PHYS_STEP_UP;
					return true;
				}
				d->o = old; // try again, but only up
				laddir.x = laddir.y = 0;
            }
            return false;
		}
        bool cansmooth = true;
        d->o = old;
        /* check if there is space atop the stair to move to */
        if(d->physstate != PHYS_STEP_UP)
        {
            vec checkdir = stairdir;
            checkdir.mul(0.1f);
            checkdir.z += maxstep + 0.1f;
            checkdir.mul(force);
            d->o.add(checkdir);
            if(!collide(d))
            {
                d->o = old;
                if(collide(d, vec(0, 0, -1), slopez)) return false;
                cansmooth = false;
            }
        }

        if(cansmooth)
        {
            d->o = old;
            vec checkdir = stairdir;
            checkdir.z += 1;
            checkdir.mul(maxstep*force);
            d->o.add(checkdir);
            if(!collide(d, checkdir))
            {
                if(collide(d, vec(0, 0, -1), slopez))
                {
                    d->o = old;
                    return false;
                }
            }
			/* try stepping up half as much as forward */
			d->o = old;
			vec smoothdir = vec(dir.x, dir.y, 0).mul(force);
			float magxy = smoothdir.magnitude();
			if(magxy > 1e-9f)
			{
				if(magxy > 2*dir.z)
				{
					smoothdir.mul(1/magxy);
					smoothdir.z = 0.5f;
					smoothdir.mul(dir.magnitude()*force/smoothdir.magnitude());
				}
				else smoothdir.z = dir.z;
				d->o.add(smoothdir);
				d->o.z += maxstep*force + 0.1f*force;
				if(collide(d, smoothdir))
				{
					d->o.z -= maxstep*force + 0.1f*force;
					if(d->physstate == PHYS_FALL || d->floor != floor)
					{
						d->timeinair = 0;
						d->floor = floor;
						switchfloor(d, dir, d->floor);
					}
					d->physstate = PHYS_STEP_UP;
					return true;
				}
			}
        }

        /* try stepping up */
        d->o = old;
        d->o.z += dir.magnitude()*force;
        if(collide(d, vec(0, 0, 1)))
        {
            if(d->physstate == PHYS_FALL || d->floor != floor)
            {
                d->timeinair = 0;
                d->floor = floor;
                switchfloor(d, dir, d->floor);
            }
            if(cansmooth) d->physstate = PHYS_STEP_UP;
            return true;
        }
        d->o = old;
        return false;
    }

	bool trystepdown(physent *d, vec &dir, float step, float xy, float z)
	{
		vec old(d->o);
		vec dv(dir.x*xy, dir.y*xy, -step*z), v = vec(dv).mul(stairheight/(step*z));
		d->o.add(v);
		if(!collide(d, vec(0, 0, -1), slopez))
		{
			d->o = old;
			d->o.add(dv);
			if(collide(d, vec(0, 0, -1))) return true;
		}
		d->o = old;
		return false;
	}


    void falling(physent *d, vec &dir, const vec &floor)
	{
		if(d->type == ENT_PLAYER && d->physstate >= PHYS_FLOOR && !d->onladder && !liquidcheck(d))
		{
			vec moved(d->o);
			d->o.z -= stairheight+0.1f;
			if(!collide(d, vec(0, 0, -1), slopez))
			{
				d->o = moved;
				d->timeinair = 0;
				d->physstate = PHYS_STEP_DOWN;
				return;
			}
			else d->o = moved;
		}

        if(floor.z > 0.0f && floor.z < slopez)
        {
            if(floor.z >= wallz) switchfloor(d, dir, floor);
            d->timeinair = 0;
            d->physstate = PHYS_SLIDE;
            d->floor = floor;
        }
        else if(d->onladder)
        {
            d->timeinair = 0;
            d->physstate = PHYS_FLOOR;
            d->floor = vec(0, 0, 1);
        }
		else d->physstate = PHYS_FALL;
	}

    void landing(physent *d, vec &dir, const vec &floor, bool collided)
	{
#if 0
        if(d->physstate == PHYS_FALL)
        {
            d->timeinair = 0;
            if(dir.z < 0.0f) dir.z = d->vel.z = 0.0f;
        }
#endif
        switchfloor(d, dir, floor);
        d->timeinair = 0;
        if((d->physstate!=PHYS_STEP_UP && d->physstate!=PHYS_STEP_DOWN) || !collided)
            d->physstate = floor.z >= floorz ? PHYS_FLOOR : PHYS_SLOPE;
		d->floor = floor;
	}

	bool findfloor(physent *d, bool collided, const vec &obstacle, bool &slide, vec &floor)
	{
		bool found = false;
		vec moved(d->o);
		d->o.z -= 0.1f;
		if(d->onladder)
		{
			floor = vec(0, 0, 1);
			found = true;
		}
		else if(d->physstate != PHYS_FALL && !collide(d, vec(0, 0, -1), d->physstate == PHYS_SLOPE ? slopez : floorz))
		{
			floor = wall;
			found = true;
		}
		else if(collided && obstacle.z >= slopez)
		{
			floor = obstacle;
			found = true;
			slide = false;
		}
        else if(d->physstate == PHYS_STEP_UP || d->physstate == PHYS_SLIDE)
        {
            if(!collide(d, vec(0, 0, -1)) && wall.z > 0.0f)
            {
                floor = wall;
                if(floor.z >= slopez) found = true;
            }
        }
        else if(d->physstate >= PHYS_SLOPE && d->floor.z < 1.0f)
        {
            if(!collide(d, vec(d->floor).neg(), 0.95f) || !collide(d, vec(0, 0, -1)))
            {
                floor = wall;
                if(floor.z >= slopez && floor.z < 1.0f) found = true;
            }
        }
        if(collided && (!found || obstacle.z > floor.z))
        {
            floor = obstacle;
            slide = !found && (floor.z < wallz || floor.z >= slopez);
        }
		d->o = moved;
		return found;
	}

	bool move(physent *d, vec &dir)
	{
		vec old(d->o);

		if(d->type == ENT_PLAYER && d->physstate == PHYS_STEP_DOWN && !d->onladder && !liquidcheck(d) && !d->jumping && !d->impulsing)
		{
			float step = dir.magnitude();
			if(trystepdown(d, dir, step, 0.75f, 0.25f)) return true;
			if(trystepdown(d, dir, step, 0.5f, 0.5f)) return true;
			if(trystepdown(d, dir, step, 0.25f, 0.75f)) return true;
			d->o.z -= step;
			if(collide(d, vec(0, 0, -1))) return true;
			d->o = old;
			if(!collide(d, vec(0, 0, -1))) d->physstate = PHYS_FALL;
		}

		bool collided = false, slidecollide = false;
		vec obstacle;
		d->o.add(dir);
		if(!collide(d, dir))
		{
            obstacle = wall;
            /* check to see if there is an obstacle that would prevent this one from being used as a floor */
            if(d->type==ENT_PLAYER && ((wall.z>=slopez && dir.z<0) || (wall.z<=-slopez && dir.z>0)) && (dir.x || dir.y) && !collide(d, vec(dir.x, dir.y, 0)))
            {
                if(wall.dot(dir) >= 0) slidecollide = true;
                obstacle = wall;
            }

            d->o = old;
            d->o.z -= stairheight;
            d->zmargin = -stairheight;
            if(d->physstate == PHYS_SLOPE || d->physstate == PHYS_FLOOR || (!collide(d, vec(0, 0, -1), slopez) && (d->physstate==PHYS_STEP_UP || wall.z>=floorz || d->onladder)))
            {
                d->o = old;
                d->zmargin = 0;
                if(trystepup(d, dir, obstacle, stairheight, d->physstate == PHYS_SLOPE || d->physstate == PHYS_FLOOR || d->onladder ? d->floor : vec(wall))) return true;
            }
            else
            {
                d->o = old;
                d->zmargin = 0;
            }

			/* can't step over the obstacle, so just slide against it */
			collided = true;
		}
        else if(d->physstate == PHYS_STEP_UP || d->onladder)
        {
            if(!collide(d, vec(0, 0, -1), slopez))
            {
                d->o = old;
                if(trystepup(d, dir, vec(0, 0, 1), stairheight, d->onladder ? d->floor : vec(wall))) return true;
                d->o.add(dir);
            }
        }

		vec floor(0, 0, 0);
		bool slide = collided, found = findfloor(d, collided, obstacle, slide, floor);
        if(slide || (!collided && floor.z > 0 && floor.z < wallz))
        {
            slideagainst(d, dir, slide ? obstacle : floor, found, slidecollide);
		}
		if(found) landing(d, dir, floor, collided);
		else falling(d, dir, floor);
		return !collided;
	}

	void modifyvelocity(physent *pl, bool local, bool floating, int millis)
	{
		if(floating)
		{
			if(game::allowmove(pl) && pl->jumping)
			{
				pl->vel.z += jumpforce(pl, false);
				pl->jumping = false;
				if(local && pl->type == ENT_PLAYER) client::addmsg(SV_PHYS, "ri2", ((gameent *)pl)->clientnum, SPHY_JUMP);
			}
		}
        else if(pl->physstate >= PHYS_SLOPE || liquidcheck(pl))
		{
			if(game::allowmove(pl))
			{
				if(pl->jumping)
				{
					pl->falling = vec(0, 0, 0);
					pl->physstate = PHYS_FALL; // cancel out other physstate
					pl->vel.z += jumpforce(pl, true);
					if(pl->inliquid)
					{
						float scale = liquidmerge(pl, 1.f, liquidspeed);
						pl->vel.x *= scale; pl->vel.y *= scale;
					}
					pl->jumping = false;
					if(local && pl->type == ENT_PLAYER)
					{
						playsound(S_JUMP, pl->o, pl);
						regularshape(PART_SMOKE, int(pl->radius), 0x222222, 21, 20, 250, pl->feetpos(), 1.f, -10, 0, 10.f);
						client::addmsg(SV_PHYS, "ri2", ((gameent *)pl)->clientnum, SPHY_JUMP);
					}
				}
				if(pl->impulsing && canimpulse(pl) && (pl->move || pl->strafe))
				{
					vec dir; vecfromyawpitch(pl->aimyaw, max(pl->aimpitch+50.f, 90.f), pl->move, pl->strafe, dir); dir.normalize().mul(impulseforce(pl));
					pl->vel.add(dir);
					pl->lastimpulse = lastmillis;
					if(local && pl->type == ENT_PLAYER)
					{
						playsound(S_IMPULSE, pl->o, pl);
						regularshape(PART_SMOKE, int(pl->radius), 0x222222, 21, 20, 250, pl->feetpos(), 1.f, -10, 0, 10.f);
						client::addmsg(SV_PHYS, "ri2", ((gameent *)pl)->clientnum, SPHY_IMPULSE);
					}
				}
			}
		}
		else if(game::allowmove(pl) && (pl->impulsing || pl->jumping) && canimpulse(pl))
		{
			bool q = (pl->move || pl->strafe) && !pl->jumping && pl->impulsing;
			vec dir; vecfromyawpitch(pl->aimyaw, q ? pl->aimpitch : 90.f, q ? pl->move : 1, pl->strafe, dir); dir.normalize().mul(impulseforce(pl));
			if(q) pl->vel.z = 0;
			pl->vel.add(dir);
			pl->falling = vec(0, 0, 0);
			pl->lastimpulse = lastmillis;
			pl->jumping = false;
			if(local && pl->type == ENT_PLAYER)
			{
				playsound(S_IMPULSE, pl->o, pl);
				regularshape(PART_SMOKE, int(pl->radius), 0x222222, 21, 20, 250, pl->feetpos(), 1.f, -10, 0, 10.f);
				client::addmsg(SV_PHYS, "ri2", ((gameent *)pl)->clientnum, SPHY_IMPULSE);
			}
		}
        if(pl->physstate == PHYS_FALL && !pl->onladder) pl->timeinair += millis;

		vec m(0, 0, 0);
        bool wantsmove = game::allowmove(pl) && (pl->move || pl->strafe);
		if(wantsmove)
		{
			vecfromyawpitch(pl->aimyaw, floating || (pl->inliquid && (liquidcheck(pl) || pl->aimpitch < 0.f)) || movepitch(pl) ? pl->aimpitch : 0, pl->move, pl->strafe, m);
            if(pl->type == ENT_PLAYER && !floating && pl->physstate >= PHYS_SLOPE)
			{ // move up or down slopes in air but only move up slopes in liquid
				float dz = -(m.x*pl->floor.x + m.y*pl->floor.y)/pl->floor.z;
                m.z = liquidcheck(pl) ? max(m.z, dz) : dz;
			}
			m.normalize();
		}

		vec d = vec(m).mul(movevelocity(pl));
        if(pl->type == ENT_PLAYER && !floating && !pl->inliquid)
			d.mul((wantsmove ? 1.3f : 1.0f) * (pl->physstate == PHYS_FALL || pl->physstate == PHYS_STEP_DOWN ? 1.3f : 1.0f));
		if(floating || pl->type==ENT_CAMERA) pl->vel.lerp(d, pl->vel, pow(max(1.0f - 1.0f/floatcurb, 0.0f), millis/20.0f));
		else
		{
			float curb = pl->physstate >= PHYS_SLOPE ? floorcurb : aircurb;
			if(impulsedelay > 0 && pl->lastimpulse && pl->impulsing && pl->physstate >= PHYS_SLOPE)
			{
				int millis = lastmillis-pl->lastimpulse;
				if(millis < impulsedelay) curb += (aircurb-floorcurb)*(1.f-clamp(millis/float(impulsedelay), 0.f, 1.f));
			}
			float fric = pl->inliquid ? liquidmerge(pl, curb, liquidcurb) : curb;
			pl->vel.lerp(d, pl->vel, pow(max(1.0f - 1.0f/fric, 0.0f), millis/20.0f*speedscale));
		}
	}

    void modifygravity(physent *pl, int curtime)
    {
        float secs = curtime/1000.0f;
        vec g(0, 0, 0);
        if(pl->physstate == PHYS_FALL) g.z -= gravityforce(pl)*secs;
        else if(pl->floor.z > 0 && pl->floor.z < floorz)
        {
            g.z = -1;
            g.project(pl->floor);
            g.normalize();
            g.mul(gravityforce(pl)*secs);
        }
        if(!liquidcheck(pl) || (!pl->move && !pl->strafe)) pl->falling.add(g);

        if(liquidcheck(pl) || pl->physstate >= PHYS_SLOPE)
        {
            float fric = liquidcheck(pl) ? liquidmerge(pl, aircurb, liquidcurb) : floorcurb,
                  c = liquidcheck(pl) ? 1.0f : clamp((pl->floor.z - slopez)/(floorz-slopez), 0.0f, 1.0f);
            pl->falling.mul(pow(max(1.0f - c/fric, 0.0f), curtime/20.0f*speedscale));
        }
    }

    void updatematerial(physent *pl, const vec &center, float radius, const vec &bottom, bool local, bool floating)
    {
		int matid = lookupmaterial(bottom), curmat = matid&MATF_VOLUME, flagmat = matid&MATF_FLAGS,
			oldmat = pl->inmaterial&MATF_VOLUME;

		if(!floating && curmat != oldmat)
		{
			#define mattrig(mo,mcol,ms,mt,mz,mq,mp,mw) \
			{ \
				int col = (int(mcol[2]*mq) + (int(mcol[1]*mq) << 8) + (int(mcol[0]*mq) << 16)); \
				regularshape(mp, mt, col, 21, 20, m_speedtime(mz), mo, ms, 10, 0, 20.f); \
				if(mw >= 0) playsound(mw, mo, pl); \
			}
			if(curmat == MAT_WATER || oldmat == MAT_WATER)
				mattrig(bottom, watercol, 0.5f, int(radius), 250, 0.25f, PART_SPARK, curmat != MAT_WATER ? S_SPLASH2 : S_SPLASH1);
			if(curmat == MAT_LAVA) mattrig(vec(bottom).add(vec(0, 0, radius)), lavacol, 2.f, int(radius), 500, 1.f, PART_FIREBALL, S_BURNING);
		}
		if(local && pl->type == ENT_PLAYER && pl->state == CS_ALIVE && flagmat == MAT_DEATH)
			game::suicide((gameent *)pl, (curmat == MAT_LAVA ? HIT_MELT : 0)|HIT_FULL);
		pl->inmaterial = matid;
		if((pl->inliquid = !floating && isliquid(curmat)) != false)
		{
			float frac = float(center.z-bottom.z)/10.f, sub = pl->submerged;
			vec tmp = bottom;
			int found = 0;
			loopi(10)
			{
				tmp.z += frac;
				if(!isliquid(lookupmaterial(tmp)&MATF_VOLUME))
				{
					found = i+1;
					break;
				}
			}
			pl->submerged = found ? found/10.f : 1.f;
			if(local && pl->physstate < PHYS_SLIDE && sub >= 0.5f && pl->submerged < 0.5f && pl->vel.z > 1e-16f)
				pl->vel.z = max(pl->vel.z, max(jumpforce(pl, false), max(gravityforce(pl), 50.f)));
		}
		else pl->submerged = 0;
		pl->onladder = !floating && flagmat == MAT_LADDER;
        if(pl->onladder && pl->physstate < PHYS_SLIDE) pl->floor = vec(0, 0, 1);
    }

    void updatematerial(physent *pl, bool local, bool floating)
    {
        updatematerial(pl, pl->o, pl->height/2.f, pl->type == ENT_PLAYER ? pl->feetpos(1) : pl->o, local, floating);
    }

    void updateragdoll(dynent *d, const vec &center, float radius)
    {
        vec bottom(center);
        bottom.z -= radius/2.f;
        updatematerial(d, center, radius, bottom, false, false);
    }

	// main physics routine, moves a player/monster for a time step
	// moveres indicated the physics precision (which is lower for monsters and multiplayer prediction)
	// local is false for multiplayer prediction

	bool moveplayer(physent *pl, int moveres, bool local, int millis)
	{
		bool floating = pl->type == ENT_CAMERA || (pl->type == ENT_PLAYER && pl->state == CS_EDITING);
		float secs = millis/1000.f;

		if(pl->type==ENT_PLAYER)
        {
            updatematerial(pl, local, floating);
            if(!floating && !pl->onladder) modifygravity(pl, millis); // apply gravity
        }
		modifyvelocity(pl, local, floating, millis); // apply any player generated changes in velocity

		vec d(pl->vel);
        if(pl->type==ENT_PLAYER && !floating && pl->inliquid) d.mul(liquidmerge(pl, 1.f, liquidspeed));
        d.add(pl->falling);
		d.mul(secs);

		if(floating)				// just apply velocity
		{
			if(pl->physstate != PHYS_FLOAT)
			{
				pl->physstate = PHYS_FLOAT;
				pl->timeinair = 0;
                pl->falling = vec(0, 0, 0);
			}
			pl->o.add(d);
		}
		else						// apply velocity with collision
		{
			const float f = 1.0f/moveres;
			int collisions = 0, timeinair = pl->timeinair;
			vec vel(pl->vel);

			d.mul(f);
			loopi(moveres) if(!move(pl, d)) { if(++collisions<5) i--; } // discrete steps collision detection & sliding
			if(pl->type == ENT_PLAYER && !pl->timeinair && timeinair > 1000) // if we land after long time must have been a high jump, make thud sound
				playsound(S_LAND, pl->o, pl);
		}

        if(pl->type==ENT_PLAYER && pl->state==CS_ALIVE)
        {
		    updatedynentcache(pl);

            if(local && pl->o.z < 0)
            {
                game::suicide((gameent *)pl, HIT_FALL|HIT_FULL);
                return false;
            }
        }

		return true;
	}

    bool movecamera(physent *pl, const vec &dir, float dist, float stepdist)
    {
        int steps = (int)ceil(dist/stepdist);
        if(steps <= 0) return true;

        vec d(dir);
        d.mul(dist/steps);
        loopi(steps)
        {
            vec oldpos(pl->o);
            pl->o.add(d);
            if(!collide(pl, vec(0, 0, 0), 0, false))
            {
                pl->o = oldpos;
                return false;
            }
        }
        return true;
    }

    void interppos(physent *d)
    {
        d->o = d->newpos;
        d->o.z += d->height;

        int diff = lastphysframe - lastmillis;
        if(diff <= 0 || !physinterp) return;

        vec deltapos(d->deltapos);
        deltapos.mul(min(diff, physframetime)/float(physframetime));
        d->o.add(deltapos);
    }

	void move(physent *d, int moveres, bool local)
	{
        if(physsteps <= 0)
        {
            if(local) interppos(d);
            return;
        }

        if(local)
        {
            d->o = d->newpos;
            d->o.z += d->height;
        }
        loopi(physsteps-1) moveplayer(d, moveres, local, physframetime);
        if(local) d->deltapos = d->o;
        moveplayer(d, moveres, local, physframetime);
        if(local)
        {
            d->newpos = d->o;
            d->deltapos.sub(d->newpos);
            d->newpos.z -= d->height;
            interppos(d);
        }
	}

	void avoidcollision(physent *d, const vec &dir, physent *obstacle, float space)
	{
		float rad = obstacle->radius+d->radius;
		vec bbmin(obstacle->o);
		bbmin.x -= rad;
		bbmin.y -= rad;
		bbmin.z -= obstacle->height+d->aboveeye;
		bbmin.sub(space);
		vec bbmax(obstacle->o);
		bbmax.x += rad;
		bbmax.y += rad;
		bbmax.z += obstacle->aboveeye+d->height;
		bbmax.add(space);

		loopi(3) if(d->o[i] <= bbmin[i] || d->o[i] >= bbmax[i]) return;

		float mindist = 1e16f;
		loopi(3) if(dir[i] != 0)
		{
			float dist = ((dir[i] > 0 ? bbmax[i] : bbmin[i]) - d->o[i]) / dir[i];
			mindist = min(mindist, dist);
		}
		if(mindist >= 0.0f && mindist < 1e15f) d->o.add(vec(dir).mul(mindist));
	}

	void updatephysstate(physent *d)
	{
		if(d->physstate == PHYS_FALL && !d->onladder) return;
		d->timeinair = 0;
		vec old(d->o);
		/* Attempt to reconstruct the floor state.
		 * May be inaccurate since movement collisions are not considered.
		 * If good floor is not found, just keep the old floor and hope it's correct enough.
		 */
        bool foundfloor = false;
		switch(d->physstate)
		{
			case PHYS_SLOPE:
			case PHYS_FLOOR:
			case PHYS_STEP_DOWN:
				d->o.z -= 0.15f;
				if(!collide(d, vec(0, 0, -1), d->physstate == PHYS_SLOPE ? slopez : floorz))
                {
					d->floor = wall;
                    foundfloor = true;
                }
				break;

			case PHYS_STEP_UP:
				d->o.z -= stairheight+0.15f;
				if(!collide(d, vec(0, 0, -1), slopez))
                {
					d->floor = wall;
                    foundfloor = true;
                }
				break;

			case PHYS_SLIDE:
				d->o.z -= 0.15f;
				if(!collide(d, vec(0, 0, -1)) && wall.z < slopez)
                {
					d->floor = wall;
                    foundfloor = true;
                }
				break;
			default: break;
		}
		if(d->physstate > PHYS_FALL && d->floor.z <= 0) d->floor = vec(0, 0, 1);
        else if(d->onladder && !foundfloor) d->floor = vec(0, 0, 1);
		d->o = old;
	}

	bool entinmap(physent *d, bool avoidplayers)
	{
		if(d->state != CS_ALIVE) { d->resetinterp(); return insideworld(d->o); }
		vec orig = d->o;
		#define inmapchk(x,y) \
		{ \
			loopi(x) \
			{ \
				if(i) { y; } \
				if(collide(d) && !inside) \
				{ \
					if(avoidplayers && hitplayer && issolid(hitplayer)) continue; \
                    d->resetinterp(); \
					return true; \
				} \
				d->o = orig; \
			} \
		}
		vec dir; vecfromyawpitch(d->yaw, d->pitch, 1, 0, dir);
		inmapchk(100, d->o.add(vec(dir).mul(i/10.f)));
		inmapchk(100, d->o.add(vec((rnd(21)-10)*i/10.f, (rnd(21)-10)*i/10.f, (rnd(21)-10)*i/10.f)));
		d->o = orig;
        d->resetinterp();
		return false;
	}

    VARP(smoothmove, 0, 90, 1000);
    VARP(smoothdist, 0, 64, 1024);

    void predictplayer(gameent *d, bool domove, int res = 0, bool local = false)
    {
        d->o = d->newpos;
        d->o.z += d->height;

        d->yaw = d->newyaw;
        d->pitch = d->newpitch;

        d->aimyaw = d->newaimyaw;
        d->aimpitch = d->newaimpitch;

        if(domove)
        {
            move(d, res, local);
            d->newpos = d->o;
            d->newpos.z -= d->height;
        }

        float k = 1.0f - float(lastmillis - d->smoothmillis)/float(smoothmove);
        if(k>0)
        {
            d->o.add(vec(d->deltapos).mul(k));

            d->yaw += d->deltayaw*k;
            if(d->yaw<0) d->yaw += 360;
            else if(d->yaw>=360) d->yaw -= 360;
            d->pitch += d->deltapitch*k;

            d->aimyaw += d->deltaaimyaw*k;
            if(d->aimyaw<0) d->aimyaw += 360;
            else if(d->aimyaw>=360) d->aimyaw -= 360;
            d->aimpitch += d->deltaaimpitch*k;
        }
    }

	void smoothplayer(gameent *d, int res, bool local)
	{
		if(d->state == CS_ALIVE || d->state == CS_EDITING)
		{
			if(smoothmove && d->smoothmillis>0) predictplayer(d, true, res, local);
			else move(d, res, local);
		}
		else if(d->state==CS_DEAD || d->state == CS_WAITING)
        {
            if(d->ragdoll) moveragdoll(d, true);
            else if(lastmillis-d->lastpain<2000) move(d, res, local);
        }
	}

	bool droptofloor(vec &o, float radius, float height)
	{
		if(!insideworld(o)) return false;
		vec v(0.0001f, 0.0001f, -1);
		v.normalize();
		if(raycube(o, v, hdr.worldsize) >= hdr.worldsize) return false;
		physent d;
		d.type = ENT_PROJ;
		d.o = o;
		d.radius = radius;
		d.height = height;
		d.aboveeye = radius;
        if(!movecamera(&d, vec(0, 0, -1), hdr.worldsize, 1))
        {
            o = d.o;
            return true;
        }
        return false;
	}

	void update()
	{
        int diff = lastmillis - lastphysframe;
        if(diff <= 0) physsteps = 0;
        else
        {
            physsteps = (diff + physframetime - 1)/physframetime;
            lastphysframe += physsteps * physframetime;
        }
		cleardynentcache();
	}
}
