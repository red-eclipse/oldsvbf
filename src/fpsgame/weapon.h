// weapon.h: all shooting and effects code, projectile management

struct weaponstate
{
	fpsclient &cl;
	fpsent *player1;

	static const int MONSTERDAMAGEFACTOR = 4;
	static const int OFFSETMILLIS = 500;
	vec sg[SGRAYS];

#ifdef BFRONTIER
	#define gunallowed(am,gn,gs) (\
			gn >= GUN_FIST && gn < NUMGUNS && (gs < 0 || gn != gs) && \
			(am[gn] || g_bf || gn == GUN_FIST) && \
			(gn != GUN_FIST || !g_bf) && \
			(gn != GUN_RIFLE || (g_bf && m_noitems)) \
		)

	IVARP(maxdebris, 0, 25, 1000);
	IVARP(weaponswitchstyle, 0, 1, 1);
#else
    IVARP(maxdebris, 10, 25, 1000);
#endif

	weaponstate(fpsclient &_cl) : cl(_cl), player1(_cl.player1)
	{
		CCOMMAND(weaponstate, weapon, "sss",
		{
			self->weaponswitch(args[0][0] ? atoi(args[0]) : -1,
								args[1][0] ? atoi(args[1]) : -1,
								args[2][0] ? atoi(args[2]) : -1);

		});
#ifdef BFRONTIER
		CCOMMAND(weaponstate, getweapon, "", self->getweapon());
#endif
	}

	void weaponswitch(int a = -1, int b = -1, int c = -1)
	{
		if(player1->state!=CS_ALIVE || a<-1 || b<-1 || c<-1 || a>=NUMGUNS || b>=NUMGUNS || c>=NUMGUNS) return;
#ifdef BFRONTIER
		int gamemode = cl.gamemode, s = player1->gunselect;
		
		if (weaponswitchstyle())
		{
			int t = -1;
			
			loopi(WSAMT) if (wsguns[i] == s) { t = i; break; }
			
			while (true)
			{
				if (a >= 0) t = a;
				else t += b;
				
				while (t >= WSAMT) t -= WSAMT;
				while (t < 0) t += WSAMT;
				
				if (!gunallowed(player1->ammo, wsguns[t], player1->gunselect))
				{
					if (a >= 0)
					{
						cl.playsoundc(S_NOAMMO, player1); 
						return; 
					}
				}
				else break;
			}
			
			if (t >= 0 && t <= WSAMT) s = wsguns[t];
		}
		else
		{
			if (gunallowed(player1->ammo, a, player1->gunselect)) s = a;
			else if (gunallowed(player1->ammo, b, player1->gunselect)) s = b;
			else if (gunallowed(player1->ammo, c, player1->gunselect)) s = c;
			else
			{
				loopi(WSAMT)
				{
					if (gunallowed(player1->ammo, wsguns[i], player1->gunselect))
					{
						s = wsguns[i];
						break;
					}
				}
			}
		}
#else
        int *ammo = player1->ammo;
        int s = player1->gunselect;

        if     (a>=0 && s!=a  && ammo[a])          s = a;
        else if(b>=0 && s!=b  && ammo[b])          s = b;
        else if(c>=0 && s!=c  && ammo[c])          s = c;
        else if(s!=GUN_CG     && ammo[GUN_CG])     s = GUN_CG;
        else if(s!=GUN_RL     && ammo[GUN_RL])     s = GUN_RL;
        else if(s!=GUN_SG     && ammo[GUN_SG])     s = GUN_SG;
        else if(s!=GUN_RIFLE  && ammo[GUN_RIFLE])  s = GUN_RIFLE;
        else if(s!=GUN_GL     && ammo[GUN_GL])     s = GUN_GL;
        else if(s!=GUN_PISTOL && ammo[GUN_PISTOL]) s = GUN_PISTOL;
        else                                       s = GUN_FIST;
#endif
		if(s!=player1->gunselect) 
		{
			cl.cc.addmsg(SV_GUNSELECT, "ri", s);
			playsound(S_WEAPLOAD, &player1->o);
		}
		player1->gunselect = s;
	}

#ifdef BFRONTIER
	void getweapon()
	{
		int t = -1;
		
		if (weaponswitchstyle())
		{
			loopi(WSAMT) if (wsguns[i] == player1->gunselect) { t = i; break; }
		}
		else
			t = player1->gunselect;
		
		s_sprintfd(s)("%d", t);
		result(s);
	}
#else
	int reloadtime(int gun) { return guns[gun].attackdelay; }
#endif	

	void offsetray(vec &from, vec &to, int spread, vec &dest)
	{
		float f = to.dist(from)*spread/1000;
		for(;;)
		{
			#define RNDD rnd(101)-50
			vec v(RNDD, RNDD, RNDD);
			if(v.magnitude()>50) continue;
			v.mul(f);
			v.z /= 2;
			dest = to;
			dest.add(v);
			vec dir = dest;
			dir.sub(from);
			raycubepos(from, dir, dest, 0, RAY_CLIPMAT|RAY_POLY);
			return;
		}
	}

	void createrays(vec &from, vec &to)			 // create random spread of rays for the shotgun
	{
		loopi(SGRAYS) offsetray(from, to, SGSPREAD, sg[i]);
	}

	enum { BNC_GRENADE, BNC_GIBS, BNC_DEBRIS };

	struct bouncent : physent
	{
		int lifetime;
		float lastyaw, roll;
		bool local;
		fpsent *owner;
		int bouncetype;
		vec offset;
		int offsetmillis;
#ifdef BFRONTIER
		float elasticity, waterfric;
		
		bouncent() : elasticity(0.6f), waterfric(3.0f) {}
		~bouncent() {}
#endif
	};

	vector<bouncent *> bouncers;

	void newbouncer(const vec &from, const vec &to, bool local, fpsent *owner, int type, int lifetime, int speed)
	{
		bouncent &bnc = *(bouncers.add(new bouncent));
		bnc.reset();
		bnc.type = ENT_BOUNCE;
		bnc.o = from;
		bnc.radius = type==BNC_DEBRIS ? 0.5f : 1.5f;
		bnc.eyeheight = bnc.radius;
		bnc.aboveeye = bnc.radius;
		bnc.lifetime = lifetime;
		bnc.roll = 0;
		bnc.local = local;
		bnc.owner = owner;
		bnc.bouncetype = type;
#ifdef BFRONTIER
		switch(bnc.bouncetype)
		{
			case BNC_GRENADE:
				if (g_bf)
				{
					bnc.elasticity = 0.2f;
					bnc.waterfric = 2.0f;
				}
				else
				{
					bnc.elasticity = 0.6f;
					bnc.waterfric = 3.0f;
				}
				break;
			default:
				bnc.elasticity = 0.6f;
				bnc.waterfric = 3.0f;
				break;
		}
#endif

		vec dir(to);
		dir.sub(from).normalize();
		bnc.vel = dir;
		bnc.vel.mul(speed);

		avoidcollision(&bnc, dir, owner, 0.1f);

		bnc.offset = hudgunorigin(type==BNC_GRENADE ? GUN_GL : -1, from, to, owner);
		bnc.offset.sub(bnc.o);
		bnc.offsetmillis = OFFSETMILLIS;
	}

	void bounceupdate(int time)
	{
		loopv(bouncers)
		{
			bouncent &bnc = *(bouncers[i]);
            if(bnc.bouncetype==BNC_GRENADE && vec(bnc.vel).add(bnc.gravity).magnitude() > 50.0f) 
            {
                vec pos(bnc.o);
                pos.add(vec(bnc.offset).mul(bnc.offsetmillis/float(OFFSETMILLIS)));
                regular_particle_splash(5, 1, 150, pos);
            }
			vec old(bnc.o);
			int rtime = time;
			while(rtime > 0)
			{
				int qtime = min(bnc.bouncetype==BNC_GRENADE ? 10 : 30, rtime);
				rtime -= qtime;
#ifdef BFRONTIER
				if ((bnc.lifetime -= qtime) < 0 || bounce(&bnc, qtime/1000.0f, bnc.elasticity, bnc.waterfric))
#else
				if((bnc.lifetime -= qtime)<0 || bounce(&bnc, qtime/1000.0f, 0.6f))
#endif
				{
					if(bnc.bouncetype==BNC_GRENADE)
					{
#ifdef BFRONTIER
						extern physent *hitplayer;
						if (g_bf && bnc.lifetime > 0 && hitplayer != NULL) continue;
						int qdam = getgun(GUN_GL).damage*(bnc.owner->quadmillis ? 4 : 1);
#else
						int qdam = guns[GUN_GL].damage*(bnc.owner->quadmillis ? 4 : 1);
#endif
						hits.setsizenodelete(0);
						explode(bnc.local, bnc.owner, bnc.o, NULL, qdam, GUN_GL);					
                        if(bnc.local)
                            cl.cc.addmsg(SV_EXPLODE, "ri2iv", cl.lastmillis-cl.maptime, GUN_GL,
									hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
					}
					delete &bnc;
					bouncers.remove(i--);
					goto next;
				}
			}
			bnc.roll += old.sub(bnc.o).magnitude()/(4*RAD);
			bnc.offsetmillis = max(bnc.offsetmillis-time, 0);
			next:;
		}
	}

	void removebouncers(fpsent *owner)
	{
		loopv(bouncers) if(bouncers[i]->owner==owner) { delete bouncers[i]; bouncers.remove(i--); }
	}

	struct projectile { vec o, to, offset; float speed; fpsent *owner; int gun; bool local; int offsetmillis; };
	vector<projectile> projs;

	void projreset() { projs.setsize(0); bouncers.deletecontentsp(); bouncers.setsize(0); }

	void newprojectile(const vec &from, const vec &to, float speed, bool local, fpsent *owner, int gun)
	{
		projectile &p = projs.add();
		p.o = from;
		p.to = to;
		p.offset = hudgunorigin(gun, from, to, owner);
		p.offset.sub(from);
		p.speed = speed;
		p.local = local;
		p.owner = owner;
		p.gun = gun;
		p.offsetmillis = OFFSETMILLIS;
	}
	
	void removeprojectiles(fpsent *owner) 
	{ 
		loopv(projs) if(projs[i].owner==owner) projs.remove(i--);
	}

	IVARP(blood, 0, 1, 1);

	void damageeffect(int damage, fpsent *d)
	{
		vec p = d->o;
		p.z += 0.6f*(d->eyeheight + d->aboveeye) - d->eyeheight;
		if(blood()) particle_splash(3, damage/10, 1000, p);
		if(d!=cl.player1)
		{
			s_sprintfd(ds)("@%d", damage);
			particle_text(d->abovehead(), ds, 8);
		}
	}
	
	void spawnbouncer(vec &p, vec &vel, fpsent *d, int type)
	{
		vec to(rnd(100)-50, rnd(100)-50, rnd(100)-50);
		to.normalize();
		to.add(p);
		newbouncer(p, to, true, d, type, rnd(1000)+1000, rnd(100)+20);
	}	

	void superdamageeffect(vec &vel, fpsent *d)
	{
		if(!d->superdamage) return;
		vec from = d->abovehead();
		from.y -= 16;
		loopi(min(d->superdamage/25, 40)+1) spawnbouncer(from, vel, d, BNC_GIBS);
	}

	struct hitmsg
	{
		int target, lifesequence, info;
		ivec dir;
	};
	vector<hitmsg> hits;

	void hit(int damage, fpsent *d, fpsent *at, const vec &vel, int gun, int info = 1)
	{
		d->lastpain = cl.lastmillis;
		at->totaldamage += damage;
		d->superdamage = 0;

#ifdef BFRONTIER
		hitmsg &h = hits.add();
		h.target = d->clientnum;
		h.lifesequence = d->lifesequence;
		h.info = info;
		h.dir = ivec(int(vel.x*DNF), int(vel.y*DNF), int(vel.z*DNF));
		if (d == player1) d->hitpush(damage, vel, at, gun);
		if (at == player1)
		{
			int snd;
			if (damage > 200) snd = 0;
			else if (damage > 175) snd = 1;
			else if (damage > 150) snd = 2;
			else if (damage > 125) snd = 3;
			else if (damage > 100) snd = 4;
			else if (damage > 50) snd = 5;
			else snd = 6;
			playsound(S_DAMAGE1+snd);
		}
#else
		if(d->type==ENT_AI || !m_mp(cl.gamemode) || d==player1) d->hitpush(damage, vel, at, gun);

        if(d->type==ENT_AI) ((monsterset::monster *)d)->monsterpain(damage, at); 
        else if(!m_mp(cl.gamemode)) cl.damaged(damage, d, at);
		else 
		{ 
			hitmsg &h = hits.add();
			h.target = d->clientnum;
			h.lifesequence = d->lifesequence;
			h.info = info;
            damageeffect(damage, d);
			if(d==player1)
			{
				h.dir = ivec(0, 0, 0);
				d->damageroll(damage);
				damageblend(damage);
				playsound(S_PAIN6);
			}
			else 
			{
				h.dir = ivec(int(vel.x*DNF), int(vel.y*DNF), int(vel.z*DNF));
				playsound(S_PAIN1+rnd(5), &d->o); 
			}
		}
#endif
	}

	void hitpush(int damage, fpsent *d, fpsent *at, vec &from, vec &to, int gun, int rays)
	{
		vec v(to);
		v.sub(from);
		v.normalize();
		hit(damage, d, at, v, gun, rays);
	}

	void radialeffect(fpsent *o, vec &v, int qdam, fpsent *at, int gun)
	{
        if(o->state!=CS_ALIVE) return;
		vec dir;
		float dist = rocketdist(o, dir, v);
		if(dist<RL_DAMRAD) 
		{
			int damage = (int)(qdam*(1-dist/RL_DISTSCALE/RL_DAMRAD));
			if(gun==GUN_RL && o==at) damage /= RL_SELFDAMDIV; 
			hit(damage, o, at, dir, gun, int(dist*DMF));
		}
	}

    float rocketdist(fpsent *o, vec &dir, const vec &v)
	{
		vec middle = o->o;
		middle.z += (o->aboveeye-o->eyeheight)/2;
		float dist = middle.dist(v, dir);
		dir.div(dist);
		if(dist<0) dist = 0;
		return dist;
	}

#ifdef BFRONTIER
	void wobeffect(vec &v, int qdam)
	{
		vec dir;
		float dist = rocketdist(cl.player1, dir, v);
		if (dist < RL_DAMRAD*5.f) cl.camerawobble = max(cl.camerawobble, int(qdam*(1-dist/RL_DISTSCALE/(RL_DAMRAD*5.f))));
	}
#endif
	void explode(bool local, fpsent *owner, vec &v, dynent *notthis, int qdam, int gun)
	{
		particle_splash(0, 200, 300, v);
		playsound(S_RLHIT, &v);
		particle_fireball(v, RL_DAMRAD, gun==GUN_RL ? 22 : 23);
        adddynlight(v, 1.15f*RL_DAMRAD, vec(1, 0.75f, 0.5f), 600, 400);
#ifdef BFRONTIER
        if (maxdebris())
        {
#endif
			int numdebris = rnd(maxdebris()-5)+5;
			vec debrisvel = owner->o==v ? vec(0, 0, 0) : vec(owner->o).sub(v).normalize(), debrisorigin(v);
			if(gun==GUN_RL) debrisorigin.add(vec(debrisvel).mul(8));
			loopi(numdebris) spawnbouncer(debrisorigin, debrisvel, owner, BNC_DEBRIS);
#ifdef BFRONTIER
        }
        wobeffect(v, qdam);
#endif
		if(!local) return;
		loopi(cl.numdynents())
		{
			fpsent *o = (fpsent *)cl.iterdynents(i);
			if(!o || o==notthis) continue;
			radialeffect(o, v, qdam, owner, gun);
		}
	}

	void splash(projectile &p, vec &v, dynent *notthis, int qdam)
	{
        if(guns[p.gun].part)
		{
			particle_splash(0, 100, 200, v);
			playsound(S_FEXPLODE, &v);
			// no push?
		}
		else
		{
			explode(p.local, p.owner, v, notthis, qdam, GUN_RL);
		}
	}

	bool projdamage(fpsent *o, projectile &p, vec &v, int qdam)
	{
        if(o->state!=CS_ALIVE) return false;
		if(!intersect(o, p.o, v)) return false;
#ifdef BFRONTIER
		splash(p, v, NULL, qdam);
#else
		splash(p, v, o, qdam);
#endif
		vec dir;
        if(guns[p.gun].part) { dir = v; dir.normalize(); }
        else rocketdist(o, dir, v);
		hit(qdam, o, p.owner, dir, p.gun, 0);
		return true;
	}

	void moveprojectiles(int time)
	{
		loopv(projs)
		{
			projectile &p = projs[i];
			p.offsetmillis = max(p.offsetmillis-time, 0);
#ifdef BFRONTIER
			int qdam = getgun(p.gun).damage*(p.owner->quadmillis ? 4 : 1);
#else
			int qdam = guns[p.gun].damage*(p.owner->quadmillis ? 4 : 1);
#endif
			if(p.owner->type==ENT_AI) qdam /= MONSTERDAMAGEFACTOR;
			vec v;
			float dist = p.to.dist(p.o, v);
			float dtime = dist*1000/p.speed; 
			if(time > dtime) dtime = time;
			v.mul(time/dtime);
			v.add(p.o);
			bool exploded = false;
			hits.setsizenodelete(0);
			if(p.local)
			{
				loopj(cl.numdynents())
				{
					fpsent *o = (fpsent *)cl.iterdynents(j);
					if(!o || p.owner==o || o->o.reject(v, 10.0f)) continue;
                    if(projdamage(o, p, v, qdam)) { exploded = true; break; }
				}
			}
			if(!exploded)
			{
				if(dist<4)
				{
					if(p.o!=p.to && raycubepos(p.o, vec(p.to).sub(p.o), p.to, 0, RAY_CLIPMAT|RAY_POLY)>=4) 
						continue; // if original target was moving, reevaluate endpoint
					splash(p, v, NULL, qdam);
					exploded = true;
				}
				else 
				{	
                    vec pos(v);
                    pos.add(vec(p.offset).mul(p.offsetmillis/float(OFFSETMILLIS)));
                    if(guns[p.gun].part)
					{
                         regular_particle_splash(1, 2, 300, pos);
                         particle_splash(guns[p.gun].part, 1, 1, pos);
					}
					else 
					{
                        regular_particle_splash(5, 2, 300, pos);
					}
				}	
			}
			if(exploded) 
			{
                if(p.local)
                    cl.cc.addmsg(SV_EXPLODE, "ri2iv", cl.lastmillis-cl.maptime, p.gun,
							hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
				projs.remove(i--);
			}
			else p.o = v;
		}
	}

	vec hudgunorigin(int gun, const vec &from, const vec &to, fpsent *d)
	{
		vec offset(from);
        if(d!=player1 || isthirdperson()) 
        {
            vec front, right;
            vecfromyawpitch(d->yaw, 0, 1, 0, front);
            offset.add(front.mul(d->radius));
            if(d->type!=ENT_AI || cl.ms.monstertypes[((monsterset::monster *)d)->mtype].vwepname)
            {
                offset.z -= d->eyeheight/2;
                vecfromyawpitch(d->yaw, 0, 0, -1, right);
                offset.add(right.mul(0.5f*d->radius));
            }
            return offset;
        }
        offset.add(vec(to).sub(from).normalize().mul(2));
		if(cl.hudgun())
		{
            offset.sub(vec(camup).mul(1.0f));
			offset.add(vec(camright).mul(0.8f));
		}
		return offset;
	}

	void shootv(int gun, vec &from, vec &to, fpsent *d, bool local)	 // create visual effect from a shot
	{
#ifdef BFRONTIER
		playsound(getgun(gun).sound, d==player1 ? NULL : &d->o);
#else
		playsound(guns[gun].sound, d==player1 ? NULL : &d->o);
#endif
		int pspeed = 25;
		switch(gun)
		{
			case GUN_FIST:
				break;

			case GUN_SG:
			{
				loopi(SGRAYS)
				{
					particle_splash(0, 20, 250, sg[i]);
                    particle_flare(hudgunorigin(gun, from, sg[i], d), sg[i], 300, 10);
				}
				break;
			}

			case GUN_CG:
			case GUN_PISTOL:
			{
				particle_splash(0, 200, 250, to);
				//particle_trail(1, 10, from, to);
                particle_flare(hudgunorigin(gun, from, to, d), to, 600, 10);
				break;
			}

			case GUN_RL:
			case GUN_FIREBALL:
			case GUN_ICEBALL:
			case GUN_SLIMEBALL:
#ifdef BFRONTIER
				pspeed = getgun(gun).projspeed*4;
#else
				pspeed = guns[gun].projspeed*4;
#endif
				if(d->type==ENT_AI) pspeed /= 2;
				newprojectile(from, to, (float)pspeed, local, d, gun);
				break;

			case GUN_GL:
			{
				float dist = from.dist(to);
				vec up = to;
				up.z += dist/8;
				newbouncer(from, up, local, d, BNC_GRENADE, 2000, 200);
				break;
			}

			case GUN_RIFLE: 
				particle_splash(0, 200, 250, to);
				particle_trail(21, 500, hudgunorigin(gun, from, to, d), to);
				break;
		}
	}

	fpsent *intersectclosest(vec &from, vec &to, fpsent *at)
	{
		fpsent *best = NULL;
		float bestdist = 1e16f;
		loopi(cl.numdynents())
		{
			fpsent *o = (fpsent *)cl.iterdynents(i);
            if(!o || o==at || o->state!=CS_ALIVE) continue;
			if(!intersect(o, from, to)) continue;
			float dist = at->o.dist(o->o);
			if(dist<bestdist)
			{
				best = o;
				bestdist = dist;
			}
		}
		return best;
	}

	void shorten(vec &from, vec &to, vec &target)
	{
		target.sub(from).normalize().mul(from.dist(to)).add(from);
	}

	void raydamage(vec &from, vec &to, fpsent *d)
	{
#ifdef BFRONTIER
		int qdam = getgun(d->gunselect).damage;
#else
		int qdam = guns[d->gunselect].damage;
#endif
		if(d->quadmillis) qdam *= 4;
		if(d->type==ENT_AI) qdam /= MONSTERDAMAGEFACTOR;
		fpsent *o, *cl;
		if(d->gunselect==GUN_SG)
		{
			bool done[SGRAYS];
			loopj(SGRAYS) done[j] = false;
			for(;;)
			{
				bool raysleft = false;
				int hitrays = 0;
				o = NULL;
				loop(r, SGRAYS) if(!done[r] && (cl = intersectclosest(from, sg[r], d)))
				{
					if((!o || o==cl) /*&& (damage<cl->health+cl->armour || cl->type!=ENT_AI)*/)
					{
						hitrays++;
						o = cl;
						done[r] = true;
						shorten(from, o->o, sg[r]);
					}
					else raysleft = true;
				}
				if(hitrays) hitpush(hitrays*qdam, o, d, from, to, d->gunselect, hitrays);
				if(!raysleft) break;
			}
		}
		else if((o = intersectclosest(from, to, d)))
		{
			hitpush(qdam, o, d, from, to, d->gunselect, 1);
			shorten(from, o->o, to);
		}
	}

	void shoot(fpsent *d, vec &targ)
	{
#ifdef BFRONTIER
		int rtime = gunvar(d->gunwait, d->gunselect);
		if(cl.lastmillis - gunvar(d->gunlast, d->gunselect) < rtime) return;
		gunvar(d->gunwait, d->gunselect) = 0;
		
		if(d == player1 && !d->attacking) return;
		
		gunvar(d->gunlast, d->gunselect) = cl.lastmillis;
		d->lastattackgun = d->gunselect;
		
		int gamemode = cl.gamemode;
		
		if (!d->ammo[d->gunselect])
		{ 
			if(d == player1)
			{
				cl.playsoundc(S_NOAMMO, d); 
				d->lastattackgun = -1; 
				
				if (g_bf)
				{
					if (!gunallowed(d->ammo, d->gunselect, -1))
					{
						weaponswitch();
					}
					else
					{
						gunvar(d->gunwait, d->gunselect) = getgun(d->gunselect).reloaddelay;
						d->ammo[d->gunselect] = getitem(d->gunselect).max;
					}
				}
				else
				{
					gunvar(d->gunwait, d->gunselect) = 600; 
					weaponswitch();
				}
			}
			return; 
		}
#else
		int attacktime = cl.lastmillis-d->lastaction;
		if(attacktime<d->gunwait) return;
		d->gunwait = 0;
		if(d==player1 && !d->attacking) return;
		d->lastaction = cl.lastmillis;
		d->lastattackgun = d->gunselect;
        if(!d->ammo[d->gunselect]) 
		{ 
			if(d==player1)
			{
				cl.playsoundc(S_NOAMMO, d); 
				d->gunwait = 600; 
				d->lastattackgun = -1; 
				weaponswitch(); 
			}
			return; 
		}
#endif
		if(d->gunselect) d->ammo[d->gunselect]--;
		vec from = d->o;
		vec to = targ;

		vec unitv;
		float dist = to.dist(from, unitv);
		unitv.div(dist);
		vec kickback(unitv);
#ifdef BFRONTIER
		kickback.mul(getgun(d->gunselect).kickamount*-2.5f);
		if (g_bf)
		{
			kickback.mul(getgun(d->gunselect).kickamount*0.005f);
		}
		d->vel.add(kickback);

		if(d->pitch<80.0f) d->pitch += getgun(d->gunselect).kickamount*0.05f;
		float shorten = 0.0f;
		if(dist>MAXRANGE) shorten = MAXRANGE;
#else
		kickback.mul(guns[d->gunselect].kickamount*-2.5f);
		d->vel.add(kickback);
		if(d->pitch<80.0f) d->pitch += guns[d->gunselect].kickamount*0.05f;
		float shorten = 0.0f;
        
		if(dist>1024) shorten = 1024;
#endif
		if(d->gunselect==GUN_FIST || d->gunselect==GUN_BITE) shorten = 12;
		float barrier = raycube(d->o, unitv, dist, RAY_CLIPMAT|RAY_POLY);
		if(barrier < dist && (!shorten || barrier < shorten))
			shorten = barrier;
		if(shorten)
		{
			to = unitv;
			to.mul(shorten);
			to.add(from);
		}
		
		if(d->gunselect==GUN_SG) createrays(from, to);
		else if(d->gunselect==GUN_CG) offsetray(from, to, 1, to);
			
#ifdef BFRONTIER
		if(d->quadmillis && rtime > 200) cl.playsoundc(S_ITEMPUP, d);

		hits.setsizenodelete(0);

		if(!getgun(d->gunselect).projspeed) raydamage(from, to, d);
#else
		if(d->quadmillis && attacktime>200) cl.playsoundc(S_ITEMPUP, d);

		hits.setsizenodelete(0);

		if(!guns[d->gunselect].projspeed) raydamage(from, to, d);
#endif

		shootv(d->gunselect, from, to, d, true);

		if(d==player1)
		{
            cl.cc.addmsg(SV_SHOOT, "ri2i6iv", cl.lastmillis-cl.maptime, d->gunselect,
							(int)(from.x*DMF), (int)(from.y*DMF), (int)(from.z*DMF), 
							(int)(to.x*DMF), (int)(to.y*DMF), (int)(to.z*DMF),
							hits.length(), hits.length()*sizeof(hitmsg)/sizeof(int), hits.getbuf());
		}

#ifdef BFRONTIER
		gunvar(d->gunwait,d->gunselect) = getgun(d->gunselect).attackdelay;
		d->totalshots += getgun(d->gunselect).damage*(d->quadmillis ? 4 : 1)*(d->gunselect==GUN_SG ? SGRAYS : 1);
#else
		d->gunwait = guns[d->gunselect].attackdelay;

		d->totalshots += guns[d->gunselect].damage*(d->quadmillis ? 4 : 1)*(d->gunselect==GUN_SG ? SGRAYS : 1);
#endif
	}

	void renderprojectiles()
	{
		vec color, dir;
		float yaw, pitch;
		loopv(bouncers)
		{
			bouncent &bnc = *(bouncers[i]);
			vec pos(bnc.o);
			pos.add(vec(bnc.offset).mul(bnc.offsetmillis/float(OFFSETMILLIS)));
			lightreaching(pos, color, dir);
			vec vel(bnc.vel);
			vel.add(bnc.gravity);
			if(vel.magnitude() <= 25.0f) yaw = bnc.lastyaw;
			else
			{
				vectoyawpitch(vel, yaw, pitch);
				yaw += 90;
				bnc.lastyaw = yaw;
			}
			pitch = -bnc.roll;
			const char *mdl = "projectiles/grenade";
			string debrisname;
			int cull = MDL_CULL_VFC|MDL_CULL_DIST;
            if(bnc.bouncetype==BNC_GIBS) { mdl = ((int)(size_t)&bnc)&0x40 ? "gibc" : "gibh"; cull |= MDL_DYNSHADOW; }
			else if(bnc.bouncetype==BNC_DEBRIS) { s_sprintf(debrisname)("debris/debris0%d", ((((int)(size_t)&bnc)&0xC0)>>6)+1); mdl = debrisname; }
			else cull = MDL_CULL_VFC|MDL_DYNSHADOW;
				
			rendermodel(color, dir, mdl, ANIM_MAPMODEL|ANIM_LOOP, 0, 0, pos, yaw, pitch, 0, 0, NULL, cull);
		}
		loopv(projs)
		{
			projectile &p = projs[i];
			if(p.gun!=GUN_RL) continue;
			vec pos(p.o);
			pos.add(vec(p.offset).mul(p.offsetmillis/float(OFFSETMILLIS)));
			if(p.to==pos) continue;
			vec v(p.to);
			v.sub(pos);
			v.normalize();
			// the amount of distance in front of the smoke trail needs to change if the model does
			vectoyawpitch(v, yaw, pitch);
			yaw += 90;
			v.mul(3);
			v.add(pos);
			lightreaching(v, color, dir);
			rendermodel(color, dir, "projectiles/rocket", ANIM_MAPMODEL|ANIM_LOOP, 0, 0, v, yaw, pitch, 0, 0, NULL, MDL_CULL_VFC|MDL_DYNSHADOW);
		}
	}  
#ifdef BFRONTIER
	void dynlightprojectiles()
	{
		loopv(projs)
		{
			projectile &p = projs[i];
			
			switch (p.gun)
			{
				case GUN_RL:
					adddynlight(p.o, RL_DAMRAD/2, vec(0.75f, 0.5f, 0.25f), 0, 0);
					break;
				case GUN_FIREBALL:
					adddynlight(p.o, RL_DAMRAD/2, vec(0.75f, 0.25f, 0.25f), 0, 0);
					break;
				case GUN_ICEBALL:
					adddynlight(p.o, RL_DAMRAD/2, vec(0.25f, 0.25f, 0.75f), 0, 0);
					break;
				case GUN_SLIMEBALL:
					adddynlight(p.o, RL_DAMRAD/2, vec(0.25f, 0.75f, 0.25f), 0, 0);
					break;
				default:
					break;
			}
		}
	}
#endif
};
