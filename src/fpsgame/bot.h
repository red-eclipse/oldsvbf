struct botclient
{
	GAMECLIENT &cl;

	static const int BOTISNEAR		= 64;			// is near
	static const int BOTISFAR		= 128;			// too far
	static const int BOTJUMPHEIGHT	= 8;			// decides to jump
	static const int BOTJUMPIMPULSE	= 16;			// impulse to jump
	static const int BOTJUMPMAX		= 32;			// too high
	static const int BOTLOSRANGE	= 514;			// line of sight range
	static const int BOTFOVRANGE	= 128;			// field of view range

	IVAR(botskill, 1, 1, 100);
	IVAR(botdebug, 0, 2, 4);

	#define BOTRATE					(101-botskill())
	#define BOTLOSDIST				(BOTLOSRANGE-(BOTRATE*2.0f))
	#define BOTFOVX					(BOTFOVRANGE-(BOTRATE*0.5f))
	#define BOTFOVY					((BOTFOVRANGE*3/4)-(BOTRATE*0.5f))
	#define BOTWAIT(x,y)			(clamp(rnd(x*BOTRATE)+rnd(y*BOTRATE), y*BOTRATE, (x+y)*BOTRATE))

	botclient(GAMECLIENT &_cl) : cl(_cl)
	{
		CCOMMAND(addbot, "", (botclient *self), self->addbot());
		CCOMMAND(delbot, "", (botclient *self), self->delbot());
	}

	void addbot()
	{
		cl.cc.addmsg(SV_ADDBOT, "r");
	}

	void delbot()
	{
		loopvrev(cl.players) if(cl.players[i])
		{
			fpsent *d = cl.players[i];
			if (d->ownernum >= 0 && d->ownernum == cl.player1->clientnum)
			{
				cl.cc.addmsg(SV_DELBOT, "ri", d->clientnum);
				break;
			}
		}
	}

	void update()
	{
		loopv(cl.players) if(cl.players[i])
		{
			fpsent *d = cl.players[i];
			if(d->ownernum >= 0 && d->ownernum == cl.player1->clientnum && d->bot)
				think(d);
		}
	}

	bool hastarget(fpsent *d)
	{
		float cyaw = fabs(fabs(d->bot->targyaw)-fabs(d->yaw)),
			cpitch = fabs(fabs(d->bot->targpitch)-fabs(d->pitch)),
			amt = BOTRATE*0.25f;
		if (cyaw < amt && cpitch < amt) return true;
		return false;
	}

	float disttonode(fpsent *d, int node, vector<int> &route)
	{
		if(cl.et.ents.inrange(node))
		{
			vector<int> avoid;
			return cl.et.linkroute(d->lastnode, node, route, avoid, ROUTE_GTONE|ROUTE_ABS);
		}
		return 1e16f;
	}

	void doreset(fpsent *d, bool full = true)
	{
		if(full)
		{
			d->stopmoving();
			d->bot->reset();
		}
		else d->bot->removestate();
	}

	void dowait(fpsent *d, botstate &b)
	{
		d->stopmoving();

		if(d->state == CS_DEAD)
		{
			if(d->respawned != d->lifesequence && !cl.respawnwait(d))
				cl.respawnself(d);
		}
		else if(d->state == CS_ALIVE)
		{
			botstate &c = d->bot->addstate(BS_UPDATE, BOTWAIT(5, 3));
			c.airtime = BOTWAIT(1, 1);
			c.targpos = d->o;
			d->bot->targyaw = d->yaw;
			d->bot->targpitch = d->pitch;
		}
	}

	bool doupdate(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			int state, node;
			vector<int> ignore;
			ignore.setsize(0);
			d->stopmoving();

			// use these as they aren't being used
			#define resetupdatestate \
			{ \
				state = 0; \
				b.target = node = -1; \
				b.targpos = d->o; \
				b.dist = 1e16f; \
			}

			#define getclosestnode(st, nd, tr, ps) \
			{ \
				if(cl.et.ents.inrange(nd) && ignore.find(nd) < 0) \
				{ \
					vec targ; \
					float pdist = d->o.dist(ps); \
					if(getsight((physent *)d, ps, targ, BOTLOSDIST, BOTFOVX, BOTFOVY)) \
						pdist *= 0.5f; \
					if(pdist < b.dist) \
					{ \
						state = st; \
						node = nd; \
						b.target = tr; \
						b.targpos = targ; \
						b.dist = pdist; \
					} \
				} \
			}

			while(true)
			{ // keep on trying until we can get a route, story of my life :P
				resetupdatestate;

				loopv(cl.et.ents)
				{
					fpsentity &e = (fpsentity &)*cl.et.ents[i];
					switch(e.type)
					{
						case WEAPON:
						{
							if(e.spawned && e.attr1 != GUN_GL && d->ammo[e.attr1] <= 0 && e.attr1 > d->bestgun(lastmillis))
								getclosestnode(BS_INTEREST, i, i, vec(e.o).add(vec(0, 0, enttype[e.type].height)));
							break;
						}
						default: break;
					}
				}

				if(cl.player1->state == CS_ALIVE)
					getclosestnode(BS_PURSUE, cl.player1->lastnode, cl.player1->clientnum, cl.player1->o);
				loopv(cl.players)
					if(cl.players[i] && cl.players[i] != d && cl.players[i]->state == CS_ALIVE)
						getclosestnode(BS_PURSUE, cl.players[i]->lastnode, cl.players[i]->clientnum, cl.players[i]->o);

				if(!state)
				{ // we failed, go scout to the the other side of the map
					resetupdatestate;

					loopv(cl.et.ents)
					{
						fpsentity &e = (fpsentity &)*cl.et.ents[i];
						if(e.type == WAYPOINT && e.o.dist(d->o) > b.dist && ignore.find(i) < 0)
						{
							state = BS_INTEREST;
							b.target = node = i;
							b.dist = e.o.dist(d->o);
							b.targpos = vec(e.o).add(vec(0, 0, enttype[e.type].height));
						}
					}
				}

				if(state && b.target >= 0 && cl.et.ents.inrange(node))
				{
					botstate &c = d->bot->addstate(state, BOTWAIT(2, 1), BOTRATE);
					c.airtime = BOTWAIT(1, 1);
					c.target = b.target;
					c.targpos = b.targpos;
					c.dist = disttonode(d, node, c.route);
					if(c.route.length()) return true;
					else ignore.add(node); // keep going then
				}
			}
		}
		return false;
	}

	bool domove(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			d->stopmoving();
			vec off(vec(b.targpos).sub(d->o));

			if(off.z > BOTJUMPHEIGHT && !d->timeinair)
				d->jumping = true;

			d->move = 1;
			d->bot->removestate(); // bounce back to the parent
			return true;
		}
		return false;
	}

	bool doattack(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			d->stopmoving();

			if(hastarget(d))
			{
				d->attacking = true;
				d->attacktime = lastmillis;
				d->bot->removestate(); // bounce back to the parent
			}
			return true;
		}
		return false;
	}

	bool wantdefer(fpsent *d, botstate &b, fpsent *e, vec &target)
	{
		if(getsight((physent *)d, e->o, target, BOTLOSDIST, BOTFOVX, BOTFOVY) &&
			d->canshoot(d->gunselect, lastmillis)) return true;
		return false;
	}

	bool dodefer(fpsent *d, botstate &b)
	{
		int target = -1;
		float dist = 1e16f;
		vec targpos, targ;

		if(cl.player1->state == CS_ALIVE && cl.player1->o.dist(d->o) < dist && wantdefer(d, b, cl.player1, targ))
		{
			target = cl.player1->clientnum;
			targpos = targ;
		}
		loopv(cl.players)
		{
			if(cl.players[i] && cl.players[i] != d && cl.players[i]->state == CS_ALIVE && cl.players[i]->o.dist(d->o) < dist && wantdefer(d, b, cl.players[i], targ))
			{
				target = cl.players[i]->clientnum;
				targpos = targ;
			}
		}

		if(target >= 0)
		{
			botstate &c = d->bot->addstate(BS_ATTACK, BOTWAIT(3, 2), BOTRATE/10);
			c.airtime = BOTWAIT(1, 1);
			b.targpos = c.targpos = targpos;
			c.target = target;
			c.route = b.route;
			c.dist = b.dist;
			return true;
		}
		return false;
	}

	bool dohunt(fpsent *d, botstate &b)
	{
		int node = b.route.find(d->lastnode);
		if(b.route.inrange(node) && b.route.inrange(node+1))
		{
			botstate &c = d->bot->addstate(BS_MOVE);
			b.targpos = c.targpos = vec(cl.et.ents[b.route[node+1]]->o).add(vec(0, 0, d->height-1));
			c.target = b.target;
			c.route = b.route;
			c.dist = b.dist;
			return true;
		}
		return false;
	}

	bool dointerest(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			if(dodefer(d, b)) return true;

			if(cl.et.ents.inrange(b.target))
			{
				fpsentity &e = (fpsentity &)*cl.et.ents[b.target];
				float eye = d->height*0.5f;
				vec m = d->o;
				m.z -= eye;

				switch(e.type)
				{
					case WEAPON:
					{
						if(!e.spawned || d->ammo[e.attr1] > 0 || e.attr1 <= d->bestgun(lastmillis))
							return false;
						break;
					}
					default: break;
				}

				if(insidesphere(m, eye, d->radius, e.o, enttype[e.type].height, enttype[e.type].radius))
				{
					if(enttype[e.type].usetype == ETU_ITEM && d->canuse(e.type, e.attr1, e.attr2, lastmillis))
					{
						d->useaction = true;
						d->usetime = lastmillis;
					}
					d->bot->removestate();
					return true;
				}

				if(dohunt(d, b)) return true;
			}
		}
		return false;
	}

	bool dopursue(fpsent *d, botstate &b)
	{
		if(d->state == CS_ALIVE)
		{
			if(dodefer(d, b)) return true;

			fpsent *e = b.target == cl.player1->clientnum ? cl.player1 : cl.getclient(b.target);

			if(e && e->state == CS_ALIVE)
			{ // if we're here, we haven't been able to shoot
				vec target;
				if(e->o.dist(d->o) < BOTISNEAR && cl.et.ents.inrange(d->lastnode) &&
					getsight((physent *)d, e->o, target, BOTLOSDIST, BOTFOVX, BOTFOVY))
				{ // find a random node to go to, we're too close
					int node = rnd(cl.et.ents[d->lastnode]->links.length());
					if(cl.et.ents.inrange(cl.et.ents[d->lastnode]->links[node]))
					{
						fpsentity &f = (fpsentity &)*cl.et.ents[cl.et.ents[d->lastnode]->links[node]];
						if(f.type == WAYPOINT)
						{
							botstate &c = d->bot->addstate(BS_MOVE);
							b.targpos = c.targpos = vec(f.o).add(vec(0, 0, d->height-1));
							c.target = b.target;
							c.route = b.route;
							c.dist = b.dist;
							return true;
						}
					}
				}

				if(dohunt(d, b)) return true;
			}
		}
		return false;
	}

	void doaim(fpsent *d, botstate &b)
	{
		float amt = float(lastmillis-d->lastupdate)/float(BOTRATE);

		d->bot->targyaw = -(float)atan2(b.targpos.x-d->o.x, b.targpos.y-d->o.y)/PI*180+180;

		if (d->yaw < d->bot->targyaw-180.0f) d->yaw += 360.0f;
		if (d->yaw > d->bot->targyaw+180.0f) d->yaw -= 360.0f;

		float dist = d->o.dist(b.targpos);
		d->bot->targpitch = asin((b.targpos.z-d->o.z)/dist)/RAD;

		if (d->bot->targpitch > 90.f) d->bot->targpitch = 90.f;
		if (d->bot->targpitch < -90.f) d->bot->targpitch = -90.f;

		float offyaw = fabs(d->bot->targyaw-d->yaw), offpitch = fabs(d->bot->targpitch-d->pitch);

		if (d->bot->targyaw > d->yaw) // slowly turn bot towards target
		{
			d->yaw += amt*offyaw;
			if (d->bot->targyaw < d->yaw) d->yaw = d->bot->targyaw;
		}
		else if (d->bot->targyaw < d->yaw)
		{
			d->yaw -= amt*offyaw;
			if (d->bot->targyaw > d->yaw) d->yaw = d->bot->targyaw;
		}
		if (d->bot->targpitch > d->pitch)
		{
			d->pitch += amt*offpitch;
			if (d->bot->targpitch < d->pitch) d->pitch = d->bot->targpitch;
		}
		else if (d->bot->targpitch < d->pitch)
		{
			d->pitch -= amt*offpitch;
			if (d->bot->targpitch > d->pitch) d->pitch = d->bot->targpitch;
		}

		cl.fixrange(d->yaw, d->pitch);
		findorientation(d->o, d->yaw, d->pitch, d->bot->targpos);
		d->aimyaw = d->yaw;
		d->aimpitch = d->pitch;
	}

	void think(fpsent *d)
	{
		if(!d->bot->state.length()) doreset(d, true);

		// the state stack works like a chain of commands, certain commands simply replace
		// each other, others spawn new commands to the stack the ai reads the top command
		// from the stack and executes it or pops the stack and goes back along the history
		// until it finds a suitable command to execute. states can have wait times and a
		// limit can be specified on the number cycles it may try for until removed.

		botstate &b = d->bot->getstate();
		int secs = lastmillis - b.millis;

		if(d->state == CS_ALIVE && b.type != BS_WAIT)
			doaim(d, b);

		if(secs >= (d->timeinair ? b.airtime : b.waittime))
		{
			bool result = false;
			switch(b.type)
			{
				case BS_WAIT: result = true; dowait(d, b); break;
				case BS_UPDATE: result = doupdate(d, b); break;
				case BS_MOVE: result = domove(d, b); break;
				case BS_PURSUE: result = dopursue(d, b); break;
				case BS_ATTACK: result = doattack(d, b); break;
				case BS_INTEREST: result = dointerest(d, b); break;
				case BS_DEFEND: default: break;
			}
			b.millis = lastmillis;
			b.cycle++;
			if(!result || (b.cycles && b.cycle >= b.cycles))
				doreset(d, false);
		}
		cl.ph.move(d, 10, true);

		if(d->state == CS_ALIVE && b.type != BS_WAIT)
		{
			cl.et.checkitems(d);
			cl.ws.shoot(d, d->bot->targpos);
			if(d->gunselect != d->bestgun(lastmillis) && d->canswitch(d->bestgun(lastmillis), lastmillis))
			{
				d->setgun(d->bestgun(lastmillis), lastmillis);
				cl.cc.addmsg(SV_GUNSELECT, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
				cl.playsoundc(S_SWITCH, d);
			}
			else if(d->ammo[d->gunselect] <= 0 && d->canreload(d->gunselect, lastmillis))
			{
				d->gunreload(d->gunselect, guntype[d->gunselect].add, lastmillis);
				cl.cc.addmsg(SV_RELOAD, "ri3", d->clientnum, lastmillis-cl.maptime, d->gunselect);
				cl.playsoundc(S_RELOAD, d);
			}
		}
		d->lastupdate = lastmillis;
	}

	void drawstate(fpsent *d, botstate &b, bool top, int above)
	{
		const char *bnames[BS_MAX] = {
			"wait", "update", "move", "defend", "pursue", "attack", "interest"
		};
		s_sprintfd(s)("@%s%s [%d:%d] (%.2f)", top ? "\fy" : "\fw", bnames[b.type], b.goal(), b.target, max((b.millis+b.waittime-lastmillis)/1000.f, 0.f));
		particle_text(vec(d->abovehead()).add(vec(0, 0, above)), s, 14, 1);
	}

	void drawroute(fpsent *d, botstate &b)
	{
		renderprimitive(true);
		int last = -1;
		loopv(b.route)
		{
			if (b.route.inrange(last))
			{
				int index = b.route[i], prev = b.route[last];

				if (cl.et.ents.inrange(index) && cl.et.ents.inrange(prev))
				{
					fpsentity &e = (fpsentity &) *cl.et.ents[index], &f = (fpsentity &) *cl.et.ents[prev];
					renderline(vec(f.o).add(vec(0, 0, 1)), vec(e.o).add(vec(0, 0, 1)), 128.f, 64.f, 64.f, false);
				}
			}
			last = i;
		}
		if(botdebug() > 3)
			renderline(vec(d->o).sub(vec(0, 0, d->height*0.5f)), b.targpos, 64.f, 128.f, 64.f, false);
		renderprimitive(false);
	}

	void render()
	{
		if(botdebug())
		{
			loopv(cl.players) if(cl.players[i] && cl.players[i]->state == CS_ALIVE && cl.players[i]->bot)
			{
				fpsent *d = cl.players[i];
				bool top = true;
				int above = 0;
				loopvrev(d->bot->state)
				{
					botstate &b = d->bot->getstate(i);
					drawstate(d, b, top, above += 2);
					if(botdebug() > 2 && top && rendernormally && b.type != BS_WAIT)
						drawroute(d, b);
					if(top)
					{
						if(botdebug() > 1) top = false;
						else break;
					}
				}
			}
		}
	}
} bot;
