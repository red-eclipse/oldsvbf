// weapon.h: all shooting and effects code, projectile management
// weapon.h: all shooting and effects code, projectile management

struct projectiles
{
	gameclient &cl;

	IVARA(maxprojectiles, 0, 200, INT_MAX-1);

	projectiles(gameclient &_cl) : cl(_cl)
	{
	}

	vector<projent *> projs;

	void init(projent &proj, bool waited)
	{
		if(waited)
		{
			findorientation(proj.owner->o, proj.owner->aimyaw, proj.owner->aimpitch, proj.to);
			proj.o = cl.ws.gunorigin(proj.owner->o, proj.to, proj.owner, proj.owner != cl.player1 || cl.isthirdperson());
		}

		switch(proj.projtype)
		{
			case PRJ_SHOT:
			{
				switch(proj.attr1)
				{
					case GUN_PLASMA:
					{
						//proj.mdl = "projectiles/plasma";
						proj.aboveeye = proj.height = proj.radius = guntype[proj.attr1].offset;
						proj.elasticity = guntype[proj.attr1].elasticity;
						proj.relativity = guntype[proj.attr1].relativity;
						proj.waterfric = guntype[proj.attr1].waterfric;
						proj.weight = guntype[proj.attr1].weight;
						proj.geomcollide = proj.playercollide = 2; // explode
						proj.radial = true;
						vec v(rnd(11)-5, rnd(11)-5, rnd(11)-5);
						if(v.magnitude()>5) v.div(5);
						v.z /= 5.f;
						proj.to.add(v);
						break;
					}
					case GUN_GL:
					{
						proj.mdl = "projectiles/grenade";
						proj.aboveeye = proj.height = proj.radius = guntype[proj.attr1].offset;
						proj.elasticity = guntype[proj.attr1].elasticity;
						proj.relativity = guntype[proj.attr1].relativity;
						proj.waterfric = guntype[proj.attr1].waterfric;
						proj.weight = guntype[proj.attr1].weight;
						proj.geomcollide = proj.playercollide = 1; // bounce
						float dist = proj.o.dist(proj.to);
						proj.to.z += dist/8;
						break;
					}
					case GUN_FLAMER:
					{
						proj.aboveeye = proj.height = proj.radius = guntype[proj.attr1].offset;
						proj.elasticity = guntype[proj.attr1].elasticity;
						proj.relativity = guntype[proj.attr1].relativity;
						proj.waterfric = guntype[proj.attr1].waterfric;
						proj.weight = guntype[proj.attr1].weight;
						proj.radial = true;
						proj.extinguish = true;
						proj.geomcollide = proj.playercollide = 1; // bounce
						vec v(rnd(81)-40, rnd(81)-40, rnd(81)-50);
						if(v.magnitude()>40) v.div(40);
						v.z /= 2.f;
						proj.to.add(v);
						break;
					}
					case GUN_SG:
					case GUN_CG:
					case GUN_CARBINE:
					case GUN_RIFLE:
					default:
					{
						proj.aboveeye = proj.height = proj.radius = guntype[proj.attr1].offset;
						proj.elasticity = guntype[proj.attr1].elasticity;
						proj.relativity = guntype[proj.attr1].relativity;
						proj.waterfric = guntype[proj.attr1].waterfric;
						proj.weight = guntype[proj.attr1].weight;
						proj.geomcollide = 2; // explode
						proj.playercollide = 0; // don't
						break;
					}
				}
				break;
			}
			case PRJ_GIBS:
			{
				proj.mdl = ((int)(size_t)&proj)&0x40 ? "gibc" : "gibh";
				proj.aboveeye = 1.0f;
				proj.elasticity = 0.15f;
				proj.relativity = 1.0f;
				proj.waterfric = 2.0f;
				proj.weight = 100.f;
				proj.geomcollide = proj.playercollide = 1; // bounce
				break;
			}
			case PRJ_DEBRIS:
			{
				switch(((((int)(size_t)&proj)&0xC0)>>6)+1)
				{
					case 4: proj.mdl = "debris/debris04"; break;
					case 3: proj.mdl = "debris/debris03"; break;
					case 2: proj.mdl = "debris/debris02"; break;
					case 1: default: proj.mdl = "debris/debris01"; break;
				}
				proj.aboveeye = 1.0f;
				proj.elasticity = 0.66f;
				proj.relativity = 1.0f;
				proj.waterfric = 1.75f;
				proj.weight = 150.f;
				proj.geomcollide = proj.playercollide = 1; // bounce
				break;
			}
			case PRJ_ENT:
			{
				proj.mdl = cl.et.entmdlname(proj.ent, proj.attr1, proj.attr2, proj.attr3, proj.attr4, proj.attr5);
				proj.aboveeye = 1.f;
				proj.elasticity = 0.35f;
				proj.relativity = 0.95f;
				proj.waterfric = 1.75f;
				proj.weight = 90.f;
				proj.geomcollide = 1; // bounce
				proj.playercollide = 0; // don't
				proj.o.sub(vec(0, 0, proj.owner->height*0.2f));
				vec v(rnd(101)-50, rnd(101)-50, rnd(101)-50);
				if(v.magnitude()>50) v.div(50);
				v.z /= 2.f;
				proj.to.add(v);
				break;
			}
			default: break;
		}
		if((proj.projtype != PRJ_SHOT || proj.attr1 == GUN_GL) && proj.mdl && *proj.mdl)
		{
			setbbfrommodel(&proj, proj.mdl);
			proj.radius += 1.f;
			proj.xradius += 1.f;
			proj.yradius += 1.f;
			proj.height += proj.projtype == PRJ_ENT ? 4.f : 1.f;
		}

		vec dir(vec(vec(proj.to).sub(proj.o)).normalize());
		vectoyawpitch(dir, proj.yaw, proj.pitch);
		vec rel = dir;
		if(proj.relativity) rel.add(vec(proj.owner->vel).mul(proj.relativity));
		proj.vel = vec(rel).add(vec(dir).mul(proj.maxspeed));
		proj.spawntime = lastmillis;

		if(proj.projtype == PRJ_SHOT && proj.radial)
			proj.height = proj.radius = guntype[proj.attr1].explode;

		for(hitplayer = NULL; !plcollide(&proj) && hitplayer == proj.owner; hitplayer = NULL)
			proj.o.add(vec(dir).mul(0.1f)); // get out of the player

		if(proj.projtype == PRJ_SHOT && proj.radial)
			proj.height = proj.radius = guntype[proj.attr1].offset;

		vec orig = proj.o;
		bool found = false;
		loopi(100)
		{
			if(!collide(&proj) || inside || (hitplayer && proj.playercollide == 1))
			{
				vec pos;
				if(hitplayer) pos = vec(vec(hitplayer->o).sub(proj.o)).normalize();
				else pos = wall;

				float amt = proj.elasticity > 0.f ? proj.elasticity : 1.f;
				proj.vel.apply(pos, amt);
				if(hitplayer) proj.vel.apply(hitplayer->vel, amt);
				proj.o = vec(orig).add(vec(proj.vel).mul(0.1f));
			}
			found = true;
			break;
		}
		if(!found)
		{
			if(proj.projtype == PRJ_SHOT && proj.radial)
				proj.state = CS_DEAD; // explode
			else
			{
				proj.o = orig;
				cl.ph.entinmap(&proj, false); // failsafe
			}

		}
        proj.resetinterp();
	}

	void effect(projent &proj)
	{
		proj.lifespan = clamp((proj.lifemillis-proj.lifetime)/float(proj.lifemillis), 0.f, 1.f);

		if(proj.projtype == PRJ_SHOT)
		{
			if(guntype[proj.attr1].fsound >= 0 && !issound(proj.schan))
				playsound(guntype[proj.attr1].fsound, proj.o, &proj, 0, proj.attr1 == GUN_FLAMER ? int(255*proj.lifespan) : -1, -1, -1, &proj.schan);

			if(proj.attr1 == GUN_PLASMA)
			{
				proj.lifesize = proj.lifemillis-proj.lifetime <= 500 ? clamp((proj.lifemillis-proj.lifetime)/500.f, 0.1f, 1.f) : 1.f;
				regular_part_splash(7, rnd(2)+1, 25, proj.o, 0x00AA99, guntype[proj.attr1].size*proj.lifesize, int(proj.radius*2));
			}
			else if(proj.attr1 == GUN_FLAMER)
			{
				proj.lifesize = clamp(proj.lifespan*2.f, 0.1f, 1.f);
				int col = ((int(254*max(1.0f-proj.lifespan,0.1f))<<16)+1)|((int(64*max(1.0f-proj.lifespan,0.05f))+1)<<8),
					fade = clamp(int(proj.vel.magnitude()*5.f), 20, 100);
				regular_part_splash(4, rnd(3)+1, fade, proj.o, col, guntype[proj.attr1].size*proj.lifesize, int(proj.radius*4));
			}
			else
			{
				proj.lifesize = clamp(proj.lifespan, 0.1f, 1.f);
				regularshape(5, int(proj.radius), 0x443322, 21, rnd(5)+1, 100, proj.o, 2.f);
			}
		}
		else if(proj.projtype == PRJ_GIBS)
		{
			regular_part_splash(0, 1, 5000, proj.o, 0x66FFFF, proj.radius*0.65f, int((proj.movement < 2.f ? 32 : 4)*proj.radius), proj.movement < 2.f ? 10 : 5);
		}
	}

	bool move(projent &proj, int qtime)
	{
		int mat = lookupmaterial(vec(proj.o.x, proj.o.y, proj.o.z + (proj.aboveeye - proj.height)/2));

		if(isdeadly(mat&MATF_VOLUME) || proj.o.z < 0)
		{
			proj.movement = 0;
			return false; // gets destroyed
		}

		bool water = isliquid(mat&MATF_VOLUME);
		float secs = float(qtime) / 1000.0f;
		vec old(proj.o);

		if(proj.weight > 0.f)
			proj.vel.z -=  cl.ph.gravityforce(&proj)*secs;
		vec dir(proj.vel);
		if(water)
		{
			if(proj.extinguish)
			{
				proj.movement = 0;
				return false; // gets "put out"
			}
			dir.div(proj.waterfric);
		}
		dir.mul(secs);
		proj.o.add(dir);

		if(!collide(&proj, dir, 0.f, proj.playercollide > 0) || inside || (proj.playercollide && hitplayer))
		{
			proj.o = old;
			vec pos;
			if(proj.playercollide && hitplayer)
				pos = vec(vec(hitplayer->o).sub(proj.o)).normalize();
			else pos = wall;

			if((!hitplayer && proj.geomcollide == 1) || (hitplayer && proj.playercollide == 1))
			{
				if(proj.movement > 2.f)
				{
					int mag = int(proj.vel.magnitude()), vol = clamp(mag*2, 1, 255);

					if(proj.projtype == PRJ_GIBS)
					{
						part_splash(0, 1, 10000, proj.o, 0x60FFFF, proj.radius);
					}
					else if(!hitplayer && proj.projtype == PRJ_SHOT && proj.attr1 == GUN_FLAMER)
					{
						adddecal(DECAL_SCORCH, proj.o, wall, guntype[proj.attr1].size*proj.lifesize);
						adddecal(DECAL_ENERGY, proj.o, wall, guntype[proj.attr1].size*proj.lifesize*0.5f, bvec(92, 24, 8));
					}

					if(vol)
					{
						if(proj.projtype == PRJ_SHOT && guntype[proj.attr1].rsound >= 0)
							playsound(guntype[proj.attr1].rsound, proj.o, &proj, 0, vol);
						else if(proj.projtype == PRJ_GIBS) playsound(S_SPLAT, proj.o, &proj, 0, vol);
						else if(proj.projtype == PRJ_DEBRIS) playsound(S_DEBRIS, proj.o, &proj, 0, vol);
					}
				}
				if(proj.elasticity > 0.f)
				{
					proj.vel.apply(pos, proj.elasticity);
					if(hitplayer)
						proj.vel.apply(hitplayer->vel, proj.elasticity*proj.relativity*0.1f);
				}
				else proj.vel = vec(0, 0, 0);
				proj.movement = 0;
				return true; // stay alive until timeout
			}
			proj.vel = vec(pos).mul(proj.radius).neg();
			proj.movement = 0;
			return false; // die on impact
		}

		float dist = proj.o.dist(old), diff = dist/(4*RAD);
		proj.movement += dist;
		if(proj.projtype == PRJ_SHOT && proj.attr1 == GUN_GL)
		{
			proj.roll += diff;
			if(proj.roll >= 360) proj.roll = fmod(proj.roll, 360.0f);
		}
		else if(proj.projtype == PRJ_ENT && proj.pitch != 0.f)
		{
			proj.roll = 0.f;
			if(proj.pitch < 0.f) { proj.pitch += diff; if(proj.pitch > 0.f) proj.pitch = 0.f; }
			if(proj.pitch > 0.f) { proj.pitch -= diff; if(proj.pitch < 0.f) proj.pitch = 0.f; }
		}

		return true;
	}

    bool move(projent &proj)
    {
        if(cl.ph.physsteps <= 0)
        {
            cl.ph.interppos(&proj);
            return true;
        }

        bool alive = true;
        proj.o = proj.newpos;
        proj.o.z += proj.height;
        loopi(cl.ph.physsteps-1)
        {
            if((proj.lifetime -= cl.ph.physframetime()) <= 0 || !move(proj, cl.ph.physframetime()))
            {
                alive = false;
                break;
            }
        }
        proj.deltapos = proj.o;
        if(alive && ((proj.lifetime -= cl.ph.physframetime()) <= 0 || !move(proj, cl.ph.physframetime()))) alive = false;
        proj.newpos = proj.o;
        proj.deltapos.sub(proj.newpos);
        proj.newpos.z -= proj.height;
        cl.ph.interppos(&proj);
        return alive;
    }

	void create(vec &from, vec &to, bool local, gameent *d, int type, int lifetime, int waittime, int speed, int id = 0, int ent = 0, int attr1 = 0, int attr2 = 0, int attr3 = 0, int attr4 = 0, int attr5 = 0)
	{
		if(!d || !lifetime || !speed) return;

		projent &proj = *(new projent());
		proj.o = proj.from = from;
		proj.to = to;
		proj.local = local;
		proj.projtype = type;
		proj.addtime = lastmillis;
		proj.lifetime = proj.lifemillis = lifetime;
		proj.waittime = waittime;
		proj.ent = ent;
		proj.attr1 = attr1;
		proj.attr2 = attr2;
		proj.attr3 = attr3;
		proj.attr4 = attr4;
		proj.attr5 = attr5;
		proj.maxspeed = speed;
		if(id) proj.id = id;
		else proj.id = lastmillis;
		proj.lastradial = lastmillis;
		proj.movement = 0;
		proj.owner = d;
		if(!waittime) init(proj, false);
		projs.add(&proj);
	}

	void drop(gameent *d, int g, int n, int delay = 0)
	{
		if(n >= 0)
		{
			if(cl.et.ents.inrange(n) && !m_noitems(cl.gamemode, cl.mutators))
			{
				gameentity &e = (gameentity &)*cl.et.ents[n];
				create(d->o, d->o, d == cl.player1 || d->ai, d, PRJ_ENT, itemspawntime*1000, delay, 20, n, e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5);
			}
		}
		else if(g == GUN_GL)
			create(d->o, d->o, d == cl.player1 || d->ai, d, PRJ_SHOT, 500, 50, 5, -1, WEAPON, d->gunselect);
	}

	void update()
	{
		int numprojs = projs.length();
		if(numprojs > maxprojectiles())
		{
			vector<projent *> canremove;
			loopvrev(projs)
				if(projs[i]->ready() && (projs[i]->projtype == PRJ_DEBRIS || projs[i]->projtype == PRJ_GIBS))
					canremove.add(projs[i]);

			while(!canremove.empty() && numprojs > maxprojectiles())
			{
				int oldest = 0;
				loopv(canremove)
					if(lastmillis-canremove[i]->addtime > lastmillis-canremove[oldest]->addtime)
						oldest = i;
				if(canremove.inrange(oldest))
				{
					canremove[oldest]->state = CS_DEAD;
					canremove.removeunordered(oldest);
					numprojs--;
				}
				else break;
			}
		}

		loopv(projs)
		{
			projent &proj = *projs[i];
			if(!proj.owner || proj.state == CS_DEAD)
			{
				proj.state = CS_DEAD;
				if(proj.projtype == PRJ_ENT)
				{
					if(!proj.beenused)
					{
						regularshape(7, int(proj.radius), 0x888822, 21, 50, 250, proj.o, 1.f);
						if(proj.local)
							cl.cc.addmsg(SV_EXPLODE, "ri6", proj.owner->clientnum, lastmillis-cl.maptime, -1, proj.id, 0, 0);
					}
				}
				else if(proj.projtype == PRJ_SHOT && guntype[proj.attr1].explode)
					cl.ws.explode(proj.owner, proj.o, proj.vel, proj.id, proj.attr1, proj.local);
			}
			else
			{
				if(proj.waittime > 0)
				{
					if((proj.waittime -= curtime) <= 0)
					{
						proj.waittime = 0;
						init(proj, true);
					}
					else continue;
				}

				effect(proj);

				if(proj.projtype == PRJ_SHOT || proj.projtype == PRJ_ENT)
				{
                    if(!move(proj))
                    {
                        switch(proj.projtype)
                        {
                            case PRJ_SHOT:
							    if(guntype[proj.attr1].explode)
								    cl.ws.explode(proj.owner, proj.o, proj.vel, proj.id, proj.attr1, proj.local);
                                break;

                            case PRJ_ENT:
                                if(!proj.beenused)
                                {
                                    regularshape(7, int(proj.radius), 0x888822, 21, 50, 250, proj.o, 1.f);
                                    if(proj.local)
                                        cl.cc.addmsg(SV_EXPLODE, "ri6", proj.owner->clientnum, lastmillis-cl.maptime, -1, proj.id, 0, 0);
                                }
                                break;
                        }
						proj.state = CS_DEAD;
					}
				}
				else for(int rtime = curtime; proj.state != CS_DEAD && rtime > 0;)
				{
					int qtime = min(rtime, 30);
					rtime -= qtime;

					if((proj.lifetime -= qtime) <= 0 || !move(proj, qtime))
					{
						proj.state = CS_DEAD;
						break;
					}
				}
			}

			if(proj.state == CS_DEAD)
			{
				delete &proj;
				projs.removeunordered(i--);
			}
			else if(proj.radial && proj.local) cl.ws.radiate(proj);
		}
	}

	void remove(gameent *owner)
	{
		loopv(projs) if(projs[i]->owner==owner)
        {
            delete projs[i];
            projs.removeunordered(i--);
        }
	}

	void reset()
    {
        projs.deletecontentsp();
        projs.setsize(0);
    }

	void spawn(vec &p, vec &vel, gameent *d, int type)
	{
		vec to(rnd(100)-50, rnd(100)-50, rnd(100)-50);
		to.normalize();
		to.add(p);
		create(p, to, true, d, type, rnd(2000)+2000, 0, rnd(30)+10, 0, -1);
	}

	void preload()
	{
		const char *mdls[] = {
			"projectiles/grenade",
			"gibc", "gibh",
			"debris/debris01", "debris/debris02",
			"debris/debris03", "debris/debris04",
			""
		};
		for(int i = 0; *mdls[i]; i++) loadmodel(mdls[i], -1, true);
	}

	void render()
	{
		loopv(projs) if(projs[i]->ready() && projs[i]->mdl && *projs[i]->mdl)
		{
			projent &proj = *projs[i];
            if(proj.projtype == PRJ_ENT && !cl.et.ents.inrange(proj.id)) continue;
			rendermodel(&proj.light, proj.mdl, ANIM_MAPMODEL|ANIM_LOOP, proj.o, proj.yaw+90, proj.pitch, proj.roll, MDL_CULL_VFC|MDL_CULL_OCCLUDED|MDL_DYNSHADOW|MDL_LIGHT|MDL_CULL_DIST);
		}
	}

	void adddynlights()
	{
		loopv(projs) if(projs[i]->ready() && projs[i]->projtype == PRJ_SHOT)
		{
			projent &proj = *projs[i];

			switch(proj.attr1)
			{
				case GUN_PLASMA:
				{
					vec col(0.f, 0.7f, 0.6f);
					adddynlight(proj.o, guntype[proj.attr1].explode*proj.lifesize, col);
					break;
				}
				case GUN_FLAMER:
				{
					vec col(1.1f*max(1.0f-proj.lifespan,0.1f), 0.25f*max(1.0f-proj.lifespan,0.05f), 0.00f);
					adddynlight(proj.o, guntype[proj.attr1].explode*proj.lifesize, col);
					break;
				}
				default:
					break;
			}
		}
	}
} pj;
