#define GAMEWORLD 1
#include "game.h"
namespace game
{
	int nextmode = -1, nextmuts = -1, gamemode = -1, mutators = -1;
	bool intermission = false;
	int maptime = 0, minremain = 0, swaymillis = 0;
	vec swaydir(0, 0, 0);
    int lastcamera = 0, lastspec = 0, lastspecchg = 0, lastzoom = 0, lastmousetype = 0, lastannounce = 0;
    bool prevzoom = false, zooming = false;
	int quakewobble = 0, liquidchan = -1, fogdist = 0;

	gameent *player1 = new gameent();
	vector<gameent *> players;
	dynent fpsmodel;

	ICOMMANDG(resetvars, "", (), return); // server side

	VARW(numplayers, 0, 0, MAXCLIENTS);
	SVARW(mapmusic, "");

	VARP(mouseinvert, 0, 0, 1);
	VARP(mouseabsolute, 0, 0, 1);
	VARP(mousetype, 0, 0, 2);
	VARP(mousedeadzone, 0, 10, 100);
	VARP(mousepanspeed, 1, 30, INT_MAX-1);

	VARP(thirdperson, 0, 0, 1);
	VARP(dynlightentities, 0, 2, 2);
	FVARP(playersfade, 0, 1, 1);

	VARP(thirdpersonmodel, 0, 1, 1);
	VARP(thirdpersonfov, 90, 120, 150);
	FVARP(thirdpersonblend, 0, 0.5f, 1);
	FVARP(thirdpersondist, -100, 1.f, 100);

	VARP(firstpersonmodel, 0, 1, 1);
	VARP(firstpersonfov, 90, 100, 150);
	VARP(firstpersonsway, 0, 100, INT_MAX-1);
	FVARP(firstpersonblend, 0, 1, 1);
	FVARP(firstpersondist, -10000, -0.25f, 10000);
	FVARP(firstpersonshift, -10000, 0.25f, 10000);
	FVARP(firstpersonadjust, -10000, 0.f, 10000);

	VARP(editfov, 1, 120, 179);
	VARP(specmode, 0, 1, 1); // 0 = float, 1 = tv, 2 = follow
	VARP(spectvtime, 1000, 30000, INT_MAX-1);
	VARP(specfov, 1, 120, 179);

	FVARP(spectvspeed, 0.1f, 1.f, 1000);
	FVARP(deathcamspeed, 0.1f, 2.f, 1000);

	FVARP(sensitivity, 1e-3f, 10.0f, 1000);
	FVARP(yawsensitivity, 1e-3f, 10.0f, 1000);
	FVARP(pitchsensitivity, 1e-3f, 7.5f, 1000);
	FVARP(mousesensitivity, 1e-3f, 1.0f, 1000);
	FVARP(zoomsensitivity, 1e-3f, 3.0f, 1000);
	FVARP(pronesensitivity, 1e-3f, 5.0f, 1000);

	VARP(zoomtype, 0, 0, 1);
	VARP(zoommousetype, 0, 0, 2);
	VARP(zoommousedeadzone, 0, 25, 100);
	VARP(zoommousepanspeed, 1, 10, INT_MAX-1);
	VARP(zoomfov, 20, 20, 150);
	VARP(zoomtime, 1, 250, 10000);

	extern void checkzoom();
	VARFP(zoomlevel, 1, 4, 10, checkzoom());
	VARP(zoomlevels, 1, 4, 10);
	VARP(zoomdefault, 0, 0, 10); // 0 = last used, else defines default level

	VARP(pronetype, 0, 0, 1);
	VARP(pronemousetype, 0, 0, 2);
	VARP(pronemousedeadzone, 0, 25, 100);
	VARP(pronemousepanspeed, 1, 10, INT_MAX-1);
	VARP(pronefov, 70, 70, 150);
	VARP(pronetime, 1, 150, 10000);

	VARP(showstatusabovehead, 0, 1, 2);
	FVARP(statusaboveheadblend, 0.f, 1.f, 1.f);
	VARP(shownamesabovehead, 0, 1, 2);
	VARP(showdamageabovehead, 0, 0, 2);
	TVAR(conopentex, "textures/conopen", 3);

	VARP(showobituaries, 0, 4, 5); // 0 = off, 1 = only me, 2 = 1 + announcements, 3 = 2 + but dying bots, 4 = 3 + but bot vs bot, 5 = all
	VARP(showplayerinfo, 0, 2, 2); // 0 = none, 1 = CON_INFO, 2 = CON_CHAT
	VARP(playdamagetones, 0, 2, 2);
	VARP(announcedelay, 0, 0, INT_MAX-1); // in case you wanna clip announcements to not overlap
	VARP(announcefilter, 0, 1, 1); // 0 = don't filter, 1 = only those which effect your team

    VARP(ragdolls, 0, 1, 1);
	VARP(noblood, 0, 0, 1);
	VARP(nogibs, 0, 0, 1);
	FVARP(gibscale, 0, 1.f, 1000);
	VARP(gibexpire, 0, 5000, INT_MAX-1);

	ICOMMAND(gamemode, "", (), intret(gamemode));
	ICOMMAND(mutators, "", (), intret(mutators));
	ICOMMAND(getintermission, "", (), intret(intermission ? 1 : 0));

	ICOMMAND(specmodeswitch, "", (), {
		switch(specmode)
		{
			case 0: specmode = 1; break;
			case 1: default: specmode = 0; break;
			//case 2: default: specmode = 0; break;
		}
	});

	//ICOMMAND(specfollow, "i", (int *d), specfollower(*d));

	void start() { }

	char *gametitle() { return server::gamename(gamemode, mutators); }
	char *gametext() { return mapname; }

	bool thirdpersonview(bool viewonly)
	{
        if(!viewonly && (player1->state == CS_DEAD || player1->state == CS_WAITING)) return true;
		if(!thirdperson) return false;
		if(player1->state == CS_EDITING) return false;
		if(player1->state == CS_SPECTATOR) return false;
		if(inzoom()) return false;
		return true;
	}
	ICOMMAND(isthirdperson, "i", (int *viewonly), intret(thirdpersonview(*viewonly ? true : false) ? 1 : 0));

	int mousestyle()
	{
		if(inzoom()) return weaptype[player1->weapselect].zooms ? zoommousetype : pronemousetype;
		return mousetype;
	}

	int deadzone()
	{
		if(inzoom()) return weaptype[player1->weapselect].zooms ? zoommousedeadzone : pronemousedeadzone;
		return mousedeadzone;
	}

	int panspeed()
	{
		if(inzoom()) return weaptype[player1->weapselect].zooms ? zoommousepanspeed : pronemousepanspeed;
		return mousepanspeed;
	}

	int fov()
	{
		if(player1->state == CS_EDITING) return editfov;
		if(player1->state == CS_SPECTATOR) return specfov;
		if(thirdpersonview(true)) return thirdpersonfov;
		return firstpersonfov;
	}

	void checkzoom()
	{
		if(zoomdefault > zoomlevels) zoomdefault = zoomlevels;
		if(zoomlevel < 0) zoomlevel = zoomdefault ? zoomdefault : zoomlevels;
		if(zoomlevel > zoomlevels) zoomlevel = zoomlevels;
	}

	void setzoomlevel(int level)
	{
		checkzoom();
		zoomlevel += level;
		if(zoomlevel > zoomlevels) zoomlevel = 1;
		else if(zoomlevel < 1) zoomlevel = zoomlevels;
	}
	ICOMMAND(setzoom, "i", (int *level), setzoomlevel(*level));

	void zoomset(bool on, int millis)
	{
		if(on != zooming)
		{
			resetcursor();
			lastzoom = millis;
			prevzoom = zooming;
			if(zoomdefault && on) zoomlevel = zoomdefault;
		}
		checkzoom();
		zooming = on;
	}

	bool zoomallow()
	{
		if(allowmove(player1)) return true;
		zoomset(false, 0);
		return false;
	}
	int zoominterval() { return weaptype[player1->weapselect].zooms ? zoomtime : pronetime; }

	bool inzoom()
	{
		if(zoomallow() && (zooming || lastmillis-lastzoom < zoominterval()))
			return true;
		return false;
	}
	ICOMMAND(iszooming, "", (), intret(inzoom() ? 1 : 0));

	bool inzoomswitch()
	{
		if(zoomallow() && ((zooming && lastmillis-lastzoom > zoominterval()/2) || (!zooming && lastmillis-lastzoom < zoominterval()/2)))
			return true;
		return false;
	}

	void dozoom(bool down)
	{
		if(zoomallow())
		{
			bool on = false;
			switch(weaptype[player1->weapselect].zooms ? zoomtype : pronetype)
			{
				case 1: on = down; break;
				case 0: default:
					if(down) on = !zooming;
					else on = zooming;
					break;
			}
			zoomset(on, lastmillis);
		}
	}
	ICOMMAND(zoom, "D", (int *down), { dozoom(*down!=0); });

	void addsway(gameent *d)
	{
		if(firstpersonsway)
		{
			if(d->physstate >= PHYS_SLOPE) swaymillis += curtime;
			float k = pow(0.7f, curtime/10.0f);
			vec vel = vec(d->vel).sub(d->falling);
			swaydir.mul(k).add(vec(vel).mul((1-k)/(15*max(vel.magnitude(), physics::movevelocity(d)))));
		}
		else swaydir = vec(0, 0, 0);
	}

	void announce(int idx, int targ, const char *msg, ...)
	{
		defvformatstring(text, msg, msg);
		conoutft(targ, "%s", text);
		if(idx >= 0 && (!announcedelay || !lastannounce || lastmillis-lastannounce >= announcedelay))
			playsound(idx, camera1->o, camera1, SND_FORCED);
	}
	ICOMMAND(announce, "iis", (int *idx, int *targ, char *s), announce(*idx, *targ, "\fw%s", s));

	bool tvmode()
	{
		return !m_edit(gamemode) && player1->state == CS_SPECTATOR && specmode == 1;
	}

	/*
	gameent *following()
	{
		if(player1->state == CS_SPECTATOR && specmode == 2)
		{
			gameent *d = (gameent *)iterdynents(specfollowing);
			if(!d || d->state == CS_EDITING || d->state == CS_SPECTATOR || d == player1)
			{
				loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && d->state != CS_EDITING && d->state != CS_SPECTATOR)
				{
					specfollowing = i;
					return d;
				}
			}
			else return d;
			specmode = 1;
		}
		return NULL;
	}
	*/

    bool allowmove(physent *d)
    {
        if(d->type == ENT_PLAYER)
        {
        	if(d == player1)
        	{
        		if(UI::hascursor(true)) return false;
				if(tvmode()) return false;
        	}
			if(d->state == CS_DEAD || d->state == CS_WAITING || d->state == CS_SPECTATOR || intermission)
				return false;
        }
        return true;
    }

	void choosearenaweap(gameent *d, const char *s)
	{
		if(m_arena(gamemode, mutators))
		{
			int weap = -1;
			if(*s >= '0' && *s <= '9') weap = atoi(s);
			else
			{
				loopi(WEAPON_TOTAL) if(!strcasecmp(weaptype[i].name, s))
				{
					weap = i;
					break;
				}
			}
			if(weap < WEAPON_PISTOL || weap >= WEAPON_TOTAL) weap = WEAPON_PISTOL;
			client::addmsg(SV_ARENAWEAP, "ri2", d->clientnum, weap);
			conoutf("\fwyou will spawn with: %s%s", weaptype[weap].text, (weap != WEAPON_PISTOL ? weaptype[weap].name : "random weapons"));
		}
		else conoutf("\fronly available in arena");
	}
	ICOMMAND(arenaweap, "s", (char *s), choosearenaweap(game::player1, s));

	void respawn(gameent *d)
	{
		if(d->state == CS_DEAD && d->respawned < 0 && (!d->lastdeath || lastmillis-d->lastdeath > 500))
		{
			client::addmsg(SV_TRYSPAWN, "ri", d->clientnum);
			d->respawned = lastmillis;
		}
	}

	VARA(spawneffectnum, 1, 25, INT_MAX-1);
	void spawneffect(const vec &o, int colour, int radius, int fade, float size)
	{
		regularshape(PART_ELECTRIC, radius*2, colour, 21, spawneffectnum, m_speedtimex(fade), o, size, -5, 0, 20.f);
		adddynlight(vec(o).add(vec(0, 0, radius)), radius*2, vec(colour>>16, (colour>>8)&0xFF, colour&0xFF).mul(2.f/0xFF), m_speedtimex(fade), m_speedtimex(fade/3));
	}

	gameent *pointatplayer()
	{
		loopv(players)
		{
			gameent *o = players[i];
			if(!o) continue;
			vec pos = player1->headpos();
            float dist;
			if(intersect(o, pos, worldpos, dist)) return o;
		}
		return NULL;
	}

	void setmode(int nmode, int nmuts)
	{
		nextmode = nmode; nextmuts = nmuts;
		server::modecheck(&nextmode, &nextmuts);
	}
	ICOMMAND(mode, "ii", (int *val, int *mut), setmode(*val, *mut));

	void checkcamera()
	{
		camera1 = &camera;
		if(camera1->type != ENT_CAMERA)
		{
			camera1->reset();
			camera1->type = ENT_CAMERA;
			camera1->state = CS_ALIVE;
			camera1->height = camera1->zradius = camera1->radius = camera1->xradius = camera1->yradius = 2;
		}
		if(player1->state != CS_WAITING && (player1->state != CS_SPECTATOR || tvmode()))
		{
			camera1->vel = vec(0, 0, 0);
			camera1->move = camera1->strafe = 0;
		}
	}

	void resetcamera()
	{
		lastcamera = 0;
		zoomset(false, 0);
		resetcursor();
		checkcamera();
		camera1->o = player1->o;
		camera1->resetinterp();
	}

	void resetworld()
	{
		if(hud::sb.scoreson) hud::sb.showscores(false);
		cleargui();
	}

	void resetstate()
	{
		resetworld();
		resetcamera();
	}

	void heightoffset(gameent *d, bool local)
	{
		d->o.z -= d->height;
		if(d->state == CS_ALIVE)
		{
			if(physics::iscrouching(d))
			{
				bool crouching = d->crouching;
				float crouchoff = 1.f-CROUCHHEIGHT;
				if(!crouching)
				{
					float z = d->o.z, zoff = d->zradius*crouchoff, zrad = d->zradius-zoff, frac = zoff/10.f;
					d->o.z += zrad;
					loopi(10)
					{
						d->o.z += frac;
						if(!collide(d, vec(0, 0, 1), 0.f, false))
						{
							crouching = true;
							break;
						}
					}
					if(crouching)
					{
						if(d->crouchtime >= 0) d->crouchtime = max(CROUCHTIME-(lastmillis-d->crouchtime), 0)-lastmillis;
					}
					else if(d->crouchtime < 0)
						d->crouchtime = lastmillis-max(CROUCHTIME-(lastmillis+d->crouchtime), 0);
					d->o.z = z;
				}
				if(d->type == ENT_PLAYER)
				{
					int crouchtime = abs(d->crouchtime);
					float amt = lastmillis-crouchtime < CROUCHTIME ? clamp(float(lastmillis-crouchtime)/CROUCHTIME, 0.f, 1.f) : 1.f;
					if(!crouching) amt = 1.f-amt;
					crouchoff *= amt;
				}
				d->height = d->zradius-(d->zradius*crouchoff);
			}
			else d->height = d->zradius;
		}
		else d->height = d->zradius;
		d->o.z += d->height;
	}

	void checktags(gameent *d)
	{
        if(d->muzzle == vec(-1, -1, -1))
        { // ensure valid projection "position"
        	vec dir, right;
        	vecfromyawpitch(d->yaw, d->pitch, 1, 0, dir);
        	vecfromyawpitch(d->yaw, d->pitch, 0, -1, right);
        	dir.mul(d->radius*1.25f);
        	right.mul(d->radius);
        	dir.z -= d->height*0.1f;
			d->muzzle = vec(d->o).add(dir).add(right);
        }
        if(d->affinity == vec(-1, -1, -1))
        { // ensure valid affinity (flag, etc) "position"
        	vec dir;
        	vecfromyawpitch(d->yaw, 0, -1, 0, dir);
        	dir.mul(d->radius*1.15f);
        	dir.z -= d->height*0.5f;
			d->affinity = vec(d->o).add(dir);
        }
	}

	void checkoften(gameent *d, bool local)
	{
		heightoffset(d, local);
		checktags(d);
		loopi(WEAPON_MAX) if(d->weapstate[i] != WPSTATE_IDLE)
		{
			if(d->state != CS_ALIVE || (d->weapstate[i] != WPSTATE_POWER && lastmillis-d->weaplast[i] >= d->weapwait[i]))
				d->setweapstate(i, WPSTATE_IDLE, 0, lastmillis);
		}
		if(d->respawned > 0 && lastmillis-d->respawned >= 3000) d->respawned = -1;
		if(d->suicided > 0 && lastmillis-d->suicided >= 3000) d->suicided = -1;
	}


	void otherplayers()
	{
		loopv(players) if(players[i])
		{
            gameent *d = players[i];
            const int lagtime = lastmillis-d->lastupdate;
            if(d->ai || !lagtime || intermission) continue;
            //else if(lagtime > 1000) continue;
			physics::smoothplayer(d, 1, false);
		}
	}

	void damaged(int weap, int flags, int damage, int health, gameent *d, gameent *actor, int millis, vec &dir)
	{
		if(d->state != CS_ALIVE || intermission) return;
		if(hithurts(flags))
		{
			d->dodamage(millis, health);
			if(actor->type == ENT_PLAYER) actor->totaldamage += damage;

			if(d == player1)
			{
				quakewobble += damage/2;
				hud::damageresidue += damage*2;
				hud::damagecompass(damage, actor->o, actor, weap);
			}

			if(d->type == ENT_PLAYER)
			{
				vec p = d->headpos();
				p.z += 0.6f*(d->height + d->aboveeye) - d->height;
				if(!kidmode && !noblood && weap != WEAPON_PAINT && !m_paint(gamemode, mutators))
					part_splash(PART_BLOOD, clamp(damage/2, 2, 10), 5000, p, 0x66FFFF, 2.f, 50, DECAL_BLOOD, int(d->radius));
				if(showdamageabovehead > (d != player1 ? 0 : 1))
				{
					defformatstring(ds)("@%d", damage);
					part_text(d->abovehead(4), ds, PART_TEXT, 2500, 0x00FFFF, 3.f, -10);
				}
				if(!issound(d->vschan))
					playsound(S_PAIN1+rnd(5), d->o, d, 0, -1, -1, -1, &d->vschan);
			}

			if(d != actor)
			{
				if(playdamagetones >= (d == player1 || actor == player1 ? 1 : 2))
				{
					int snd = 0;
					if(damage >= 200) snd = 7;
					else if(damage >= 150) snd = 6;
					else if(damage >= 100) snd = 5;
					else if(damage >= 75) snd = 4;
					else if(damage >= 50) snd = 3;
					else if(damage >= 25) snd = 2;
					else if(damage >= 10) snd = 1;
					if(!issound(actor->dschan))
						playsound(S_DAMAGE1+snd, actor->o, actor, 0, -1, -1, -1, &actor->dschan);
				}
			}
			ai::damaged(d, actor);
		}
		if(d == player1 || d->ai)
		{
			float force = (float(damage)/float(weaptype[weap].damage))*(100.f/d->weight)*weaptype[weap].hitpush;
			if(flags&HIT_WAVE || !hithurts(flags)) force *= wavepushscale;
			else if(d->health <= 0) force *= deadpushscale;
			else force *= hitpushscale;
			vec push = dir; push.z += 0.25f; push.mul(m_speedscale(force));
			d->vel.add(push);
		}
        if(d != actor && actor != game::player1)
			actor->lasthit = lastmillis;
	}

	void killed(int weap, int flags, int damage, gameent *d, gameent *actor)
	{
		if(d->type != ENT_PLAYER) return;

        d->lastregen = 0;
        d->lastpain = lastmillis;
		d->state = CS_DEAD;
		d->deaths++;

		int anc = -1, dth = -1;
		bool obliterated = false;
		if(weap == WEAPON_PAINT || m_paint(gamemode, mutators)) dth = S_SPLAT;
		else
		{
			obliterated = flags&HIT_EXPLODE || flags&HIT_MELT || damage > m_maxhealth(gamemode, mutators)*3/2;
			if(flags&HIT_MELT || flags&HIT_BURN) dth = S_BURN;
			else if(obliterated) dth = S_SPLOSH;
			else dth = S_DIE1+rnd(2);
		}

		if(d == player1) anc = S_V_FRAGGED;
		else d->resetinterp();
		formatstring(d->obit)("%s ", colorname(d));
        if(d == actor)
        {
        	if(flags&HIT_MELT) concatstring(d->obit, "melted");
			else if(flags&HIT_FALL) concatstring(d->obit, "thought they could fly");
			else if(flags&HIT_SPAWN) concatstring(d->obit, "tried to spawn inside solid matter");
			else if(flags&HIT_LOST) concatstring(d->obit, "got very, very lost");
        	else if(flags && isweap(weap))
        	{
				static const char *suicidenames[WEAPON_MAX] = {
					"pulled off a seemingly impossible stunt",
					"discovered buckshot bounces",
					"got caught up in their own crossfire",
					"barbequed themself for dinner",
					"found out what their plasma tasted like",
					"pulled off a seemingly impossible stunt",
					"decided to kick it, kamikaze style",
					"ate paint"
				};
        		concatstring(d->obit, suicidenames[weap]);
        	}
        	else if(flags&HIT_EXPLODE) concatstring(d->obit, "was obliterated");
        	else if(flags&HIT_BURN) concatstring(d->obit, "burnt up");
        	else if(m_paint(gamemode, mutators)) concatstring(d->obit, "gave up");
        	else concatstring(d->obit, "suicided");
        }
		else
		{
			static const char *obitnames[3][WEAPON_MAX] = {
				{
					"was pierced by",
					"was filled with buckshot by",
					"was riddled with holes by",
					"was char-grilled by",
					"was plasmified by",
					"was given laser burn by",
					"was blown to pieces by",
					"was tagged out by"
				},
				{
					"was given an extra orifice by",
					"was given scrambled brains cooked up by",
					"was air conditioned courtesy of",
					"was char-grilled by",
					"was plasmafied by",
					"was expertly sniped by",
					"was blown to pieces by",
					"was tagged out by"
				},
				{
					"was skewered by",
					"was turned into little chunks by",
					"was swiss-cheesed by",
					"was made the main course by order of chef",
					"was reduced to ooze by",
					"was laser-scalpeled by",
					"was obliterated by",
					"was tagged out by"
				}
			};

			int o = obliterated ? 2 : ((flags&HIT_PROJ) && (flags&HIT_HEAD) ? 1 : 0);
			concatstring(d->obit, isweap(weap) ? obitnames[o][weap] : "was killed by");
			if(m_team(gamemode, mutators) && d->team == actor->team)
			{
				concatstring(d->obit, " teammate ");
				concatstring(d->obit, colorname(actor));
			}
			else
			{
				concatstring(d->obit, " ");
				concatstring(d->obit, colorname(actor));
				if(!kidmode) switch(actor->spree)
				{
					case 5:
					{
						concatstring(d->obit, " in total carnage!");
						anc = S_V_SPREE1;
						part_text(actor->abovehead(), "@CARNAGE", PART_TEXT, 2500, 0x00FFFF, 4.f, -10);
						break;
					}
					case 10:
					{
						concatstring(d->obit, " who is slaughtering!");
						anc = S_V_SPREE2;
						part_text(actor->abovehead(), "@SLAUGHTER", PART_TEXT, 2500, 0x00FFFF, 4.f, -10);
						break;
					}
					case 15:
					{
						concatstring(d->obit, " going on a massacre!");
						anc = S_V_SPREE3;
						part_text(actor->abovehead(), "@MASSACRE", PART_TEXT, 2500, 0x00FFFF, 4.f, -10);
						break;
					}
					case 20:
					{
						concatstring(d->obit, m_paint(gamemode, mutators) ? " creating a paintbath!" : " creating a bloodbath!");
						anc = S_V_SPREE4;
						part_text(actor->abovehead(), m_paint(gamemode, mutators) ? "@PAINTBATH" : "@BLOODBATH", PART_TEXT, 2500, 0x00FFFF, 4.f, -10);
						break;
					}
					case 25:
					{
						concatstring(d->obit, " who seems unstoppable!");
						anc = S_V_SPREE4;
						part_text(actor->abovehead(), "@UNSTOPPABLE", PART_TEXT, 2500, 0x00FFFF, 4.f, -10);
						break;
					}
					default:
					{
						if((flags&HIT_PROJ) && (flags&HIT_HEAD))
						{
							anc = S_V_HEADSHOT;
							part_text(actor->abovehead(), "@HEADSHOT", PART_TEXT, 2500, 0x00FFFF, 4.f, -10);
						}
						else if(obliterated || (d->lastspawn && lastmillis-d->lastspawn <= spawnprotecttime*2000)) // double spawnprotect
							anc = S_V_OWNED;
						break;
					}
				}
			}
		}
		if(d != actor)
		{
			if(actor->state == CS_ALIVE) copystring(actor->obit, d->obit);
			actor->lastkill = lastmillis;
		}
		if(dth >= 0)
		{
			if(issound(d->vschan)) removesound(d->vschan);
			playsound(dth, d->o, d, 0, -1, -1, -1, &d->vschan);
		}
		if(showobituaries)
		{
			bool isme = (d == player1 || actor == player1), show = false;
			if(flags&HIT_LOST) show = true;
			else switch(showobituaries)
			{
				case 1: if(isme) show = true; break;
				case 2: if(isme || anc >= 0) show = true; break;
				case 3: if(isme || d->aitype == AI_NONE || anc >= 0) show = true; break;
				case 4: if(isme || d->aitype == AI_NONE || actor->aitype == AI_NONE || anc >= 0) show = true; break;
				case 5: default: show = true; break;
			}
			if(show)
			{
				if(isme) announce(anc, CON_INFO, "\fw%s", d->obit);
				else conoutft(CON_INFO, "\fw%s", d->obit);
			}
		}
		if(!kidmode && !noblood && !nogibs && !m_paint(gamemode, mutators))
		{
			vec pos = vec(d->o).sub(vec(0, 0, d->height*0.5f));
			int gibs = clamp(max(damage,5)/5, 1, 10), amt = int((rnd(gibs)+gibs+1)*gibscale);
			loopi(amt)
				projs::create(pos, vec(pos).add(d->vel), true, d, PRJ_GIBS, (gibexpire ? rnd(gibexpire)+(gibexpire/10) : 1000), 0, rnd(500)+1, 50);
		}
		if(m_team(gamemode, mutators) && d->team == actor->team && d != actor && actor == player1) hud::teamkills.add(lastmillis);
		ai::killed(d, actor);
	}

	void timeupdate(int timeremain)
	{
		minremain = timeremain;
		if(!timeremain && !intermission)
		{
			player1->stopmoving(true);
			hud::sb.showscores(true, true);
			intermission = true;
			smartmusic(true, false);
		}
	}

	gameent *newclient(int cn)
	{
		if(cn < 0 || cn >= MAXPLAYERS)
		{
			neterr("clientnum");
			return NULL;
		}

		if(cn == player1->clientnum) return player1;

		while(cn >= players.length()) players.add(NULL);

		if(!players[cn])
		{
			gameent *d = new gameent();
			d->clientnum = cn;
			players[cn] = d;
		}

		return players[cn];
	}

	gameent *getclient(int cn)
	{
		if(cn == player1->clientnum) return player1;
		if(players.inrange(cn)) return players[cn];
		return NULL;
	}

	void clientdisconnected(int cn)
	{
		if(!players.inrange(cn)) return;
		gameent *d = players[cn];
		if(!d) return;
		if(d->name[0] && showplayerinfo && (d->aitype == AI_NONE || ai::showaiinfo))
			conoutft(showplayerinfo > 1 ? int(CON_CHAT) : int(CON_INFO), "\fo%s left the game", colorname(d));
		projs::remove(d);
        if(m_ctf(gamemode)) ctf::removeplayer(d);
        if(m_stf(gamemode)) stf::removeplayer(d);
		DELETEP(players[cn]);
		players[cn] = NULL;
		cleardynentcache();
	}

    void preload()
    {
    	int n = m_team(gamemode, mutators) ? numteams(gamemode, mutators)+1 : 1;
    	loopi(n)
    	{
			loadmodel(teamtype[i].tpmdl, -1, true);
			loadmodel(teamtype[i].fpmdl, -1, true);
    	}
    	weapons::preload();
		projs::preload();
        entities::preload();
		if(m_edit(gamemode) || m_stf(gamemode)) stf::preload();
        if(m_edit(gamemode) || m_ctf(gamemode)) ctf::preload();
    }

	void resetmap(bool empty) // called just before a map load
	{
		if(!empty) smartmusic(true, false);
	}

	void startmap(const char *name, bool empty)	// called just after a map load
	{
		intermission = false;
		maptime = 0;
		projs::reset();
		resetworld();

		if(*name)
		{
			if(*maptitle) conoutf("%s", maptitle);
			preload();
		}
		// reset perma-state
		gameent *d;
		loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && d->type == ENT_PLAYER)
			d->resetstate(lastmillis, m_maxhealth(gamemode, mutators));
		entities::spawnplayer(player1, -1, true, false); // prevent the player from being in the middle of nowhere
		resetcamera();
		if(!empty) client::sendinfo = true;
		fogdist = max(getvar("fog")-16, 64);
	}

	gameent *intersectclosest(vec &from, vec &to, gameent *at)
	{
		gameent *best = NULL, *o;
		float bestdist = 1e16f;
		loopi(numdynents()) if((o = (gameent *)iterdynents(i)))
		{
            if(!o || o==at || o->state!=CS_ALIVE || !physics::issolid(o)) continue;
            float dist;
			if(intersect(o, from, to, dist) && dist < bestdist)
			{
				best = o;
				bestdist = dist;
			}
		}
		return best;
	}

	int numdynents() { return 1+players.length(); }
	dynent *iterdynents(int i)
	{
		if(!i) return player1;
		i--;
		if(i<players.length()) return players[i];
		i -= players.length();
		return NULL;
	}

	bool duplicatename(gameent *d, char *name = NULL)
	{
		if(!name) name = d->name;
		if(d!=player1 && !strcmp(name, player1->name)) return true;
		loopv(players) if(players[i] && d!=players[i] && !strcmp(name, players[i]->name)) return true;
		return false;
	}

	char *colorname(gameent *d, char *name, const char *prefix, bool team, bool dupname)
	{
		if(!name) name = d->name;
		static string cname;
		const char *chat = team && m_team(gamemode, mutators) ? teamtype[d->team].chat : teamtype[TEAM_NEUTRAL].chat;
		formatstring(cname)("%s\fs%s%s", *prefix ? prefix : "", chat, name);
		if(!name[0] || d->aitype != AI_NONE || (dupname && duplicatename(d, name)))
		{
			defformatstring(s)(" [\fs%s%d\fS]", d->aitype != AI_NONE ? "\fc" : "\fm", d->clientnum);
			concatstring(cname, s);
		}
		concatstring(cname, "\fS");
		return cname;
	}

	void suicide(gameent *d, int flags)
	{
		if((d == player1 || d->ai) && d->state == CS_ALIVE && d->suicided < 0)
		{
			client::addmsg(SV_SUICIDE, "ri2", d->clientnum, flags);
			d->suicided = lastmillis;
		}
	}
	ICOMMAND(kill, "",  (), { suicide(player1, 0); });

	void lighteffects(dynent *e, vec &color, vec &dir)
	{
	}

    void particletrack(particle *p, uint type, int &ts, vec &o, vec &d, bool lastpass)
    {
        if(!p || !p->owner || p->owner->type != ENT_PLAYER) return;

        switch(type&0xFF)
        {
        	case PT_TAPE: case PT_LIGHTNING:
        	{
        		float dist = o.dist(d);
				d = o = ((gameent *)p->owner)->muzzle;
        		vec dir; vecfromyawpitch(p->owner->yaw, p->owner->pitch, 1, 0, dir);
        		d.add(dir.mul(dist));
				break;
        	}
        	case PT_PART: case PT_FIREBALL: case PT_FLARE:
        	{
				o = ((gameent *)p->owner)->muzzle;
				break;
        	}
        	default: break;
        }
    }

    void dynlighttrack(physent *owner, vec &o)
    {
    }

	void newmap(int size)
	{
		client::addmsg(SV_NEWMAP, "ri", size);
	}

	void loadworld(stream *f, int maptype) {}
	void saveworld(stream *f) {}

	void fixfullrange(float &yaw, float &pitch, float &roll, bool full)
	{
		if(full)
		{
			while(pitch < -180.0f) pitch += 360.0f;
			while(pitch >= 180.0f) pitch -= 360.0f;
			while(roll < -180.0f) roll += 360.0f;
			while(roll >= 180.0f) roll -= 360.0f;
		}
		else
		{
			if(pitch > 89.9f) pitch = 89.9f;
			if(pitch < -89.9f) pitch = -89.9f;
			if(roll > 89.9f) roll = 89.9f;
			if(roll < -89.9f) roll = -89.9f;
		}
		while(yaw < 0.0f) yaw += 360.0f;
		while(yaw >= 360.0f) yaw -= 360.0f;
	}

	void fixrange(float &yaw, float &pitch)
	{
		float r = 0.f;
		fixfullrange(yaw, pitch, r, false);
	}

	void fixview(int w, int h)
	{
		if(inzoom())
		{
			int frame = lastmillis-lastzoom, f = weaptype[player1->weapselect].zooms ? zoomfov : pronefov, t = zoominterval();
			checkzoom();
			if(zoomlevels > 1 && zoomlevel < zoomlevels) f = fov()-(((fov()-zoomfov)/zoomlevels)*zoomlevel);
			float diff = float(fov()-f), amt = frame < t ? clamp(float(frame)/float(t), 0.f, 1.f) : 1.f;
			if(!zooming) amt = 1.f-amt;
			curfov = fov()-(amt*diff);
		}
		else curfov = float(fov());
        aspect = w/float(h);
        fovy = 2*atan2(tan(curfov/2*RAD), aspect)/RAD;
		hud::hudwidth = int(hud::hudsize*aspect);
	}

	bool mousemove(int dx, int dy, int x, int y, int w, int h)
	{
		bool hascursor = UI::hascursor(true);
		#define mousesens(a,b,c) ((float(a)/float(b))*c)
		if(hascursor || (mousestyle() >= 1 && player1->state != CS_WAITING && player1->state != CS_SPECTATOR))
		{
			if(mouseabsolute) // absolute positions, unaccelerated
			{
				cursorx = clamp(float(x)/float(w), 0.f, 1.f);
				cursory = clamp(float(y)/float(h), 0.f, 1.f);
				return false;
			}
			else
			{
				cursorx = clamp(cursorx+mousesens(dx, w, mousesensitivity), 0.f, 1.f);
				cursory = clamp(cursory+mousesens(dy, h, mousesensitivity*(!hascursor && mouseinvert ? -1.f : 1.f)), 0.f, 1.f);
				return true;
			}
		}
		else if(!tvmode())
		{
			if(player1->state == CS_WAITING || player1->state == CS_SPECTATOR)
			{
				camera1->yaw += mousesens(dx, w, yawsensitivity*sensitivity);
				camera1->pitch -= mousesens(dy, h, pitchsensitivity*sensitivity*(!hascursor && mouseinvert ? -1.f : 1.f));
				fixfullrange(camera1->yaw, camera1->pitch, camera1->roll, false);
			}
			else if(allowmove(player1))
			{
				float scale = inzoom() ?
						(weaptype[player1->weapselect].zooms? zoomsensitivity : pronesensitivity)
					: sensitivity;
				player1->yaw += mousesens(dx, w, yawsensitivity*scale);
				player1->pitch -= mousesens(dy, h, pitchsensitivity*scale*(!hascursor && mouseinvert ? -1.f : 1.f));
				fixfullrange(player1->yaw, player1->pitch, player1->roll, false);
			}
			return true;
		}
		return false;
	}

	void project(int w, int h)
	{
		int style = UI::hascursor() ? -1 : mousestyle();
		if(style != lastmousetype)
		{
			resetcursor();
			lastmousetype = style;
		}
		if(style >= 0) vecfromcursor(cursorx, cursory, 1.f, cursordir);
	}

	struct camstate
	{
		int ent, idx;
		vec pos, dir;
		vector<int> cansee;
		float mindist, maxdist, score;
		bool alter;

		camstate() : idx(-1), mindist(32.f), maxdist(512.f), alter(false) { reset(); }
		~camstate() {}

		void reset()
		{
			cansee.setsize(0);
			dir = vec(0, 0, 0);
			score = 0.f;
			alter = false;
		}

		static int camsort(const camstate *a, const camstate *b)
		{
			int asee = a->cansee.length(), bsee = b->cansee.length();
			if(a->alter && asee) asee = 1;
			if(b->alter && bsee) bsee = 1;
			if(asee > bsee) return -1;
			if(asee < bsee) return 1;
			if(a->score < b->score) return -1;
			if(a->score > b->score) return 1;
			return 0;
		}
	};
	vector<camstate> cameras;

	void getyawpitch(const vec &from, const vec &pos, float &yaw, float &pitch)
	{
		float dist = from.dist(pos);
		yaw = -(float)atan2(pos.x-from.x, pos.y-from.y)/PI*180+180;
		pitch = asin((pos.z-from.z)/dist)/RAD;
	}

	void scaleyawpitch(float &yaw, float &pitch, float targyaw, float targpitch, float frame, float scale)
	{
		if(yaw < targyaw-180.0f) yaw += 360.0f;
		if(yaw > targyaw+180.0f) yaw -= 360.0f;
		float offyaw = fabs(targyaw-yaw)*frame, offpitch = fabs(targpitch-pitch)*frame*scale;
		if(targyaw > yaw)
		{
			yaw += offyaw;
			if(targyaw < yaw) yaw = targyaw;
		}
		else if(targyaw < yaw)
		{
			yaw -= offyaw;
			if(targyaw > yaw) yaw = targyaw;
		}
		if(targpitch > pitch)
		{
			pitch += offpitch;
			if(targpitch < pitch) pitch = targpitch;
		}
		else if(targpitch < pitch)
		{
			pitch -= offpitch;
			if(targpitch > pitch) pitch = targpitch;
		}
		fixrange(yaw, pitch);
	}

	void cameraplayer()
	{
		if(player1->state != CS_WAITING && player1->state != CS_SPECTATOR && player1->state != CS_DEAD && !tvmode())
		{
			player1->aimyaw = camera1->yaw;
			player1->aimpitch = camera1->pitch;
			fixrange(player1->aimyaw, player1->aimpitch);
			if(lastcamera && mousestyle() >= 1 && !UI::hascursor())
			{
				physent *d = mousestyle() != 2 ? player1 : camera1;
				float amt = clamp(float(lastmillis-lastcamera)/100.f, 0.f, 1.f)*panspeed();
				float zone = float(deadzone())/200.f, cx = cursorx-0.5f, cy = 0.5f-cursory;
				if(cx > zone || cx < -zone) d->yaw += ((cx > zone ? cx-zone : cx+zone)/(1.f-zone))*amt;
				if(cy > zone || cy < -zone) d->pitch += ((cy > zone ? cy-zone : cy+zone)/(1.f-zone))*amt;
				fixfullrange(d->yaw, d->pitch, d->roll, false);
			}
		}
	}

	void cameratv()
	{
		if(cameras.empty()) loopk(2)
		{
			physent d = *player1;
			d.radius = d.height = 4.f;
			d.state = CS_ALIVE;
			loopv(entities::ents) if(entities::ents[i]->type == CAMERA || (k && !enttype[entities::ents[i]->type].noisy))
			{
				gameentity &e = *(gameentity *)entities::ents[i];
				vec pos(e.o);
				if(e.type == MAPMODEL)
				{
					mapmodelinfo &mmi = getmminfo(e.attr[0]);
					vec center, radius;
					mmi.m->collisionbox(0, center, radius);
					if(!mmi.m->ellipsecollide) rotatebb(center, radius, int(e.attr[1]));
					pos.z += ((center.z-radius.z)+radius.z*2*mmi.m->height)*3.f;
				}
				else if(enttype[e.type].radius) pos.z += enttype[e.type].radius;
				d.o = pos;
				if(physics::entinmap(&d, false))
				{
					camstate &c = cameras.add();
					c.pos = pos;
					c.ent = i;
					if(!k)
					{
						c.idx = e.attr[0];
						if(e.attr[1]) c.mindist = e.attr[1];
						if(e.attr[2]) c.maxdist = e.attr[2];
					}
				}
			}
			lastspec = lastspecchg = 0;
			if(!cameras.empty()) break;
		}
		#define unsetspectv(q) \
		{ \
			if(q) \
			{ \
				camera1->o.x = camera1->o.y = camera1->o.z = getworldsize(); \
				camera1->o.x *= 0.5f; camera1->o.y *= 0.5f; \
			} \
			camera1->resetinterp(); \
			setvar("specmode", 0, true); \
			return; \
		}

		if(!cameras.empty())
		{
			camstate *cam = &cameras[0];
			int entidx = cam->ent;
			bool alter = cam->alter, renew = !lastspec || lastmillis-lastspec >= spectvtime,
				override = renew || !lastspec || lastmillis-lastspec >= max(spectvtime/10, 1500);
			#define addcamentity(q,p) \
			{ \
				vec trg, pos = p; \
				float dist = c.pos.dist(pos); \
				if(dist >= c.mindist && dist <= min(c.maxdist, float(fogdist)) && raycubelos(c.pos, pos, trg)) \
				{ \
					c.cansee.add(q); \
					avg.add(pos); \
				} \
			}
			#define updatecamorient \
			{ \
				if((k || j) && c.cansee.length()) \
				{ \
					vec dir = vec(avg).div(c.cansee.length()).sub(c.pos).normalize(); \
					vectoyawpitch(dir, yaw, pitch); \
				} \
			}
			#define dircamentity(q,p) \
			{ \
				vec trg, pos = p; \
				if(getsight(c.pos, yaw, pitch, pos, trg, min(c.maxdist, float(fogdist)), curfov, fovy)) \
				{ \
					c.dir.add(pos); \
					c.score += c.alter ? rnd(32) : c.pos.dist(pos); \
				} \
				else \
				{ \
					avg.sub(pos); \
					c.cansee.remove(q); \
					updatecamorient; \
				} \
			}
			loopk(4)
			{
				int found = 0;
				loopvj(cameras)
				{
					camstate &c = cameras[j];
					vec avg(0, 0, 0);
					c.reset();
					switch(k)
					{
						case 0: case 1: default:
						{
							gameent *d;
							loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && (d->state == CS_ALIVE || d->state == CS_DEAD || d->state == CS_WAITING))
								addcamentity(i, d->feetpos());
							break;
						}
						case 2: case 3:
						{
							c.alter = true;
							loopv(entities::ents) if((k == 3 && !enttype[entities::ents[i]->type].noisy) || entities::ents[i]->type == WEAPON)
								addcamentity(i, entities::ents[i]->o);
							break;
						}
					}
					float yaw = camera1->yaw, pitch = camera1->pitch;
					updatecamorient;
					switch(k)
					{
						case 0: case 1: default:
						{
							gameent *d;
							loopvrev(c.cansee) if((d = (gameent *)iterdynents(c.cansee[i])))
								dircamentity(i, d->feetpos());
							break;
						}
						case 2: case 3:
						{
							loopvrev(c.cansee) if(entities::ents.inrange(c.cansee[i]))
								dircamentity(i, entities::ents[c.cansee[i]]->o);
							break;
						}
					}
					if(!c.cansee.empty())
					{
						float amt = float(c.cansee.length());
						c.dir.div(amt);
						c.score /= amt;
						found++;
					}
					else
					{
						c.score = 0;
						if(override && !k && !j && !alter) renew = true; // quick scotty, get a new cam
					}
					if(!renew || !override) break;
				}
				if(override && !found && (k || !alter))
				{
					if(k < 3) renew = true;
					else unsetspectv(lastspec ? false : true);
				}
				else break;
			}
			if(renew)
			{
				cameras.sort(camstate::camsort);
				cam = &cameras[0];
				lastspec = lastmillis;
				if(!lastspecchg || cam->ent != entidx) lastspecchg = lastmillis;
			}
			else if(alter && !cam->cansee.length()) cam->alter = true;
			camera1->o = cam->pos;
			if(cam->ent != entidx || !cam->alter)
			{
				vec dir = vec(cam->dir).sub(camera1->o).normalize();
				vectoyawpitch(dir, camera1->aimyaw, camera1->aimpitch);
			}
			if(cam->ent != entidx || cam->alter) { camera1->yaw = camera1->aimyaw; camera1->pitch = camera1->aimpitch; }
			else scaleyawpitch(camera1->yaw, camera1->pitch, camera1->aimyaw, camera1->aimpitch, (float(curtime)/1000.f)*spectvspeed, 0.25f);
			camera1->resetinterp();
		}
		else unsetspectv(true);
	}

	void updateworld()		// main game update loop
	{
		if(!curtime) return;
        if(!maptime && connected())
        {
        	maptime = lastmillis;
			if(m_lobby(gamemode)) smartmusic(true, false);
			else if(*mapmusic && (!music || !Mix_PlayingMusic() || strcmp(mapmusic, musicfile))) playmusic(mapmusic, "");
			else musicdone(false);
			RUNWORLD("on_start");
        	return;
        }

       	if(!*game::player1->name && !guiactive()) showgui("name");
        if(connected())
        {
        	game::player1->conopen = commandmillis > 0 || UI::hascursor(true);
            // do shooting/projectile update here before network update for greater accuracy with what the player sees
			if(allowmove(player1)) cameraplayer();
			else player1->stopmoving(player1->state != CS_WAITING && player1->state != CS_SPECTATOR);

            gameent *d = NULL;
            loopi(numdynents()) if((d = (gameent *)iterdynents(i)) != NULL && d->type == ENT_PLAYER)
				checkoften(d, d == player1 || d->ai);

            physics::update();
            projs::update();
			ai::update();
            if(!intermission)
            {
				entities::update();
				if(player1->state == CS_ALIVE) weapons::shoot(player1, worldpos);
            }
            otherplayers();
        }
        else if(!guiactive()) showgui("main");

		gets2c();

		if(connected())
		{
			#define adjustscaled(t,n) \
				if(n > 0) { n = (t)(n/(1.f+sqrtf((float)curtime)/100.f)); if(n <= 0) n = (t)0; }

			adjustscaled(float, player1->roll);
			adjustscaled(int, quakewobble);
			adjustscaled(int, hud::damageresidue);

			if(player1->state == CS_DEAD || player1->state == CS_WAITING)
			{
				if(player1->ragdoll) moveragdoll(player1, true);
				else if(lastmillis-player1->lastpain < 2000)
					physics::move(player1, 10, false);
			}
			else
            {
                if(player1->ragdoll) cleanragdoll(player1);
				if(player1->state == CS_EDITING) physics::move(player1, 10, true);
				else if(!intermission && player1->state == CS_ALIVE)
				{
					physics::move(player1, 10, true);
					addsway(player1);
					entities::checkitems(player1);
					weapons::reload(player1);
				}
            }
			checkcamera();
			if(player1->state == CS_DEAD)
			{
				vec dir = vec(ragdollcenter(player1)).sub(camera1->o).normalize();
				float yaw = camera1->yaw, pitch = camera1->pitch;
				vectoyawpitch(dir, yaw, pitch);
				scaleyawpitch(camera1->yaw, camera1->pitch, yaw, pitch, (float(curtime)/1000.f)*deathcamspeed, 4.f);
				camera1->aimyaw = camera1->yaw;
				camera1->aimpitch = camera1->pitch;
			}
			else if(tvmode()) cameratv();
			else if(player1->state == CS_WAITING || player1->state == CS_SPECTATOR)
			{
				camera1->move = player1->move;
				camera1->strafe = player1->strafe;
				physics::move(camera1, 10, true);
			}
			if(player1->state == CS_SPECTATOR)
			{
				player1->aimyaw = player1->yaw = camera1->yaw;
				player1->aimpitch = player1->pitch = camera1->pitch;
				player1->o = camera1->o;
				player1->resetinterp();
			}
            if(hud::sb.canshowscores()) hud::sb.showscores(true);
		}

		if(player1->clientnum >= 0) client::c2sinfo();
	}

	void recomputecamera(int w, int h)
	{
		fixview(w, h);
		checkcamera();
		if(client::ready())
		{
			if(!lastcamera)
			{
				resetcursor();
				cameras.setsize(0);
				if(mousestyle() == 2 && player1->state != CS_WAITING && player1->state != CS_SPECTATOR)
				{
					camera1->yaw = player1->aimyaw = player1->yaw;
					camera1->pitch = player1->aimpitch = player1->pitch;
				}
			}

			if(player1->state == CS_DEAD || player1->state == CS_WAITING || player1->state == CS_SPECTATOR)
			{
				camera1->aimyaw = camera1->yaw;
				camera1->aimpitch = camera1->pitch;
			}
			else
			{
				camera1->o = player1->headpos();
				if(mousestyle() <= 1)
					findorientation(camera1->o, player1->yaw, player1->pitch, worldpos);

				camera1->aimyaw = mousestyle() <= 1 ? player1->yaw : player1->aimyaw;
				camera1->aimpitch = mousestyle() <= 1 ? player1->pitch : player1->aimpitch;
				if(thirdpersonview(true) && thirdpersondist)
				{
					vec dir;
					vecfromyawpitch(camera1->aimyaw, camera1->aimpitch, thirdpersondist > 0 ? -1 : 1, 0, dir);
					physics::movecamera(camera1, dir, fabs(thirdpersondist), 1.0f);
				}
                camera1->resetinterp();

				switch(mousestyle())
				{
					case 0:
					case 1:
					{
						camera1->yaw = player1->yaw;
						camera1->pitch = player1->pitch;
						if(mousestyle())
						{
							camera1->aimyaw = camera1->yaw;
							camera1->aimpitch = camera1->pitch;
						}
						break;
					}
					case 2:
					{
						float yaw, pitch;
						vectoyawpitch(cursordir, yaw, pitch);
						fixrange(yaw, pitch);
						findorientation(camera1->o, yaw, pitch, worldpos);
						if(allowmove(player1))
						{
							player1->yaw = yaw;
							player1->pitch = pitch;
						}
						break;
					}
				}
				fixfullrange(camera1->yaw, camera1->pitch, camera1->roll, false);
				fixrange(camera1->aimyaw, camera1->aimpitch);
			}

			if(quakewobble > 0)
				camera1->roll = float(rnd(15)-7)*(float(min(quakewobble, 100))/100.f);
			else camera1->roll = 0;

			vecfromyawpitch(camera1->yaw, camera1->pitch, 1, 0, camdir);
			vecfromyawpitch(camera1->yaw, 0, 0, -1, camright);
			vecfromyawpitch(camera1->yaw, camera1->pitch+90, 1, 0, camup);

			camera1->inmaterial = lookupmaterial(camera1->o);
			camera1->inliquid = isliquid(camera1->inmaterial&MATF_VOLUME);

			switch(camera1->inmaterial)
			{
				case MAT_WATER:
				{
					if(!issound(liquidchan))
						playsound(S_UNDERWATER, camera1->o, camera1, SND_LOOP|SND_NOATTEN|SND_NODELAY|SND_NOCULL, -1, -1, -1, &liquidchan);
					break;
				}
				default:
				{
					if(issound(liquidchan)) removesound(liquidchan);
					liquidchan = -1;
					break;
				}
			}

			lastcamera = lastmillis;
		}
	}

	float showtranslucent(gameent *d, bool third = true, bool full = false)
	{
		float total = full ? 1.f : (d == player1 ? (third ? thirdpersonblend : firstpersonblend) : playersfade);
		if(d->state == CS_ALIVE)
		{
			int len = spawnprotecttime*1000, millis = d->protect(lastmillis, len); // protect returns time left
			if(millis > 0) return (1.f-(float(millis)/float(len)))*total;
			else return total;
		}
		else if(d->state == CS_DEAD || d->state == CS_WAITING)
		{
			int len = m_spawndelay(gamemode, mutators), interval = full ? len : min(len/3, 1000), over = full ? 0 : max(len-interval, 0), millis = lastmillis-d->lastdeath;
			if(millis < len)
			{
				if(millis > over) return (1.f-(float(millis-over)/float(interval)))*total;
				else return total;
			}
			else return 0.f;
		}
		return total;
	}

	void adddynlights()
	{
		if(dynlightentities)
		{
			projs::adddynlights();
			entities::adddynlights();
			if(dynlightentities > 1)
			{
				if(m_ctf(gamemode)) ctf::adddynlights();
				if(m_stf(gamemode)) stf::adddynlights();
			}
		}
	}

#if 0
	vector<gameent *> bestplayers;
    vector<int> bestteams;
#endif

	VAR(animoverride, -1, 0, ANIM_MAX-1);
	VAR(testanims, 0, 0, 1);

	int numanims() { return ANIM_MAX; }

	void findanims(const char *pattern, vector<int> &anims)
	{
		loopi(sizeof(animnames)/sizeof(animnames[0]))
			if(*animnames[i] && matchanim(animnames[i], pattern))
				anims.add(i);
	}

	void renderclient(gameent *d, bool third, float trans, int team, modelattach *attachments, bool secondary, int animflags, int animdelay, int lastaction, bool early)
	{
		string mdl;
		if(third) copystring(mdl, teamtype[team].tpmdl);
		else copystring(mdl, teamtype[team].fpmdl);

		float yaw = d->yaw, pitch = d->pitch, roll = d->roll;
		vec o = vec(third ? d->feetpos() : d->headpos());
		if(!third)
		{
			vec dir;
			if(firstpersonsway != 0.f)
			{
				vecfromyawpitch(d->yaw, d->pitch, 1, 0, dir);
				float swayspeed = min(4.f, sqrtf(d->vel.x*d->vel.x + d->vel.y*d->vel.y));
				dir.mul(swayspeed);
				float swayxy = sinf(swaymillis/115.0f)/float(firstpersonsway),
					  swayz = cosf(swaymillis/115.0f)/float(firstpersonsway);
				swap(dir.x, dir.y);
				dir.x *= -swayxy;
				dir.y *= swayxy;
				dir.z = -fabs(swayspeed*swayz);
				dir.add(swaydir);
				o.add(dir);
			}
			if(firstpersondist != 0.f)
			{
				vecfromyawpitch(yaw, pitch, 1, 0, dir);
				dir.mul(player1->radius*firstpersondist);
				o.add(dir);
			}
			if(firstpersonshift != 0.f)
			{
				vecfromyawpitch(yaw, pitch, 0, -1, dir);
				dir.mul(player1->radius*firstpersonshift);
				o.add(dir);
			}
			if(firstpersonadjust != 0.f)
			{
				vecfromyawpitch(yaw, pitch+90.f, 1, 0, dir);
				dir.mul(player1->height*firstpersonadjust);
				o.add(dir);
			}
		}

		int anim = animflags, basetime = lastaction, basetime2 = 0;
		if(animoverride)
		{
			anim = (animoverride<0 ? ANIM_ALL : animoverride)|ANIM_LOOP;
			basetime = 0;
		}
		else
		{
			if(secondary)
			{
				if(physics::liquidcheck(d) && d->physstate <= PHYS_FALL)
					anim |= (((allowmove(d) && (d->move || d->strafe)) || d->vel.z+d->falling.z>0 ? int(ANIM_SWIM) : int(ANIM_SINK))|ANIM_LOOP)<<ANIM_SECONDARY;
				else if(d->timeinair && d->jumptime && lastmillis-d->jumptime <= 1000) { anim |= ANIM_JUMP<<ANIM_SECONDARY; basetime2 = d->jumptime; }
				else if(d->timeinair && d->lastimpulse && lastmillis-d->lastimpulse <= 1000) { anim |= ANIM_IMPULSE<<ANIM_SECONDARY; basetime2 = d->lastimpulse; }
				else if(d->timeinair > 1000) anim |= (ANIM_JUMP|ANIM_END)<<ANIM_SECONDARY;
				else if(d->crouching || d->crouchtime<0)
				{
					if(d->move>0)		anim |= (ANIM_CRAWL_FORWARD|ANIM_LOOP)<<ANIM_SECONDARY;
					else if(d->strafe)	anim |= ((d->strafe>0 ? ANIM_CRAWL_LEFT : ANIM_CRAWL_RIGHT)|ANIM_LOOP)<<ANIM_SECONDARY;
					else if(d->move<0)	anim |= (ANIM_CRAWL_BACKWARD|ANIM_LOOP)<<ANIM_SECONDARY;
					else				anim |= (ANIM_CROUCH|ANIM_LOOP)<<ANIM_SECONDARY;
				}
				else if(d->move>0) anim |= (ANIM_FORWARD|ANIM_LOOP)<<ANIM_SECONDARY;
				else if(d->strafe) anim |= ((d->strafe>0 ? ANIM_LEFT : ANIM_RIGHT)|ANIM_LOOP)<<ANIM_SECONDARY;
				else if(d->move<0) anim |= (ANIM_BACKWARD|ANIM_LOOP)<<ANIM_SECONDARY;
			}

			if((anim>>ANIM_SECONDARY)&ANIM_INDEX) switch(anim&ANIM_INDEX)
			{
				case ANIM_IDLE: case ANIM_PISTOL: case ANIM_SHOTGUN: case ANIM_SMG:
				case ANIM_GRENADE: case ANIM_FLAMER: case ANIM_PLASMA: case ANIM_RIFLE: case ANIM_PAINTGUN:
				{
                    anim = (anim>>ANIM_SECONDARY) | ((anim&((1<<ANIM_SECONDARY)-1))<<ANIM_SECONDARY);
                    swap(basetime, basetime2);
					break;
				}
				default: break;
			}
		}

        if(third && testanims && d == player1) yaw = 0;
        else yaw += 90;
        if(anim == ANIM_DYING) pitch *= max(1.0f - (lastmillis-basetime)/500.0f, 0.0f);

        if(d->ragdoll && (!ragdolls || anim!=ANIM_DYING)) cleanragdoll(d);

		if(!((anim>>ANIM_SECONDARY)&ANIM_INDEX)) anim |= (ANIM_IDLE|ANIM_LOOP)<<ANIM_SECONDARY;

		int flags = MDL_LIGHT;
#if 0 // breaks linkpos
		if(d != player1 && !(anim&ANIM_RAGDOLL)) flags |= MDL_CULL_VFC|MDL_CULL_OCCLUDED|MDL_CULL_QUERY;
#endif
        if(d->type == ENT_PLAYER)
        {
            if(!early && third) flags |= MDL_FULLBRIGHT;
        }
		else flags |= MDL_CULL_DIST;
        if(early) flags |= MDL_NORENDER;
		else if(third && (anim&ANIM_INDEX)!=ANIM_DEAD) flags |= MDL_DYNSHADOW;
		dynent *e = third ? (dynent *)d : (dynent *)&fpsmodel;
		rendermodel(NULL, mdl, anim, o, yaw, pitch, roll, flags, e, attachments, basetime, basetime2, trans);
	}

	void renderplayer(gameent *d, bool third, float trans, bool early = false)
	{
		if(d->state == CS_SPECTATOR) return;
#if 0 // not working great?
		if(trans <= 0.f || (d == player1 && (third ? thirdpersonmodel : firstpersonmodel) < 1))
		{
			if(d->state == CS_ALIVE && rendernormally && (early || d != player1))
				trans = 1e-16f; // we need tag_muzzle/tag_affinity
			else return; // screw it, don't render them
		}
#endif

        modelattach a[4];
		int ai = 0, team = m_team(gamemode, mutators) ? d->team : TEAM_NEUTRAL,
			weap = d->weapselect, lastaction = 0, animflags = ANIM_IDLE|ANIM_LOOP, animdelay = 0;
		bool secondary = false, showweap = isweap(weap);


		if(d->state == CS_DEAD || d->state == CS_WAITING)
		{
			showweap = false;
			animflags = ANIM_DYING;
			lastaction = d->lastpain;
            if(ragdolls)
            {
                if(!validragdoll(d, lastaction)) animflags |= ANIM_RAGDOLL;
            }
            else
            {
			    int t = lastmillis-lastaction;
			    if(t < 0) return;
			    if(t > 1000) animflags = ANIM_DEAD|ANIM_LOOP;
            }
        }
		else if(d->state == CS_EDITING)
		{
			animflags = ANIM_EDIT|ANIM_LOOP;
			showweap = false;
		}
#if 0
		else if(intermission)
		{
			lastaction = lastmillis;
			animflags = ANIM_LOSE|ANIM_LOOP;
			animdelay = 1000;
			if(m_team(gamemode, mutators))
			{
				loopv(bestteams) if(bestteams[i] == d->team)
				{
					animflags = ANIM_WIN|ANIM_LOOP;
					break;
				}
			}
			else if(bestplayers.find(d) >= 0) animflags = ANIM_WIN|ANIM_LOOP;
		}
		else if(third && d->lasttaunt && lastmillis-d->lasttaunt <= 1000)
		{
			lastaction = d->lasttaunt;
			animflags = ANIM_TAUNT;
			animdelay = 1000;
		}
#endif
		else if(third && lastmillis-d->lastpain <= 300)
		{
			secondary = third;
			lastaction = d->lastpain;
			animflags = ANIM_PAIN;
			animdelay = 300;
		}
		else
		{
			secondary = third;
			if(showweap)
			{
				lastaction = d->weaplast[weap];
				animdelay = d->weapwait[weap];
				switch(d->weapstate[weap])
				{
					case WPSTATE_SWITCH:
					case WPSTATE_PICKUP:
					{
						if(lastmillis-d->weaplast[weap] <= d->weapwait[weap]/3)
						{
							if(!d->hasweap(d->lastweap, m_spawnweapon(gamemode, mutators))) showweap = false;
							else weap = d->lastweap;
						}
						else if(!d->hasweap(weap, m_spawnweapon(gamemode, mutators))) showweap = false;
						animflags = ANIM_SWITCH;
						break;
					}
					case WPSTATE_POWER:
					{
						if(weaptype[weap].power) animflags = weaptype[weap].anim+d->weapstate[weap];
						else animflags = weaptype[weap].anim|ANIM_LOOP;
						break;
					}
					case WPSTATE_SHOOT:
					{
						if(!d->hasweap(weap, m_spawnweapon(gamemode, mutators)) || (!weaptype[weap].reloads && lastmillis-d->weaplast[weap] <= d->weapwait[weap]/3))
							showweap = false;
						animflags = weaptype[weap].anim+d->weapstate[weap];
						break;
					}
					case WPSTATE_RELOAD:
					{
						if(!d->hasweap(weap, m_spawnweapon(gamemode, mutators)) || (!weaptype[weap].reloads && lastmillis-d->weaplast[weap] <= d->weapwait[weap]/3))
							showweap = false;
						animflags = weaptype[weap].anim+d->weapstate[weap];
						break;
					}
					case WPSTATE_IDLE: case WPSTATE_WAIT: default:
					{
						if(!d->hasweap(weap, m_spawnweapon(gamemode, mutators))) showweap = false;
						animflags = weaptype[weap].anim|ANIM_LOOP;
						break;
					}
				}
			}
		}

		if(third && !shadowmapping && !envmapping && d->o.squaredist(camera1->o) < maxparticledistance*maxparticledistance)
		{
			vec pos = d->abovehead(2);
			if(shownamesabovehead > (d != player1 ? 0 : 1))
			{
				const char *name = colorname(d, NULL, "@");
				if(name && *name && (*name != '@' || name[1]))
				{
					part_text(pos, name);
					pos.z += 2;
				}
			}
			if(showstatusabovehead > (d != player1 ? 0 : 1) && d->conopen && (d->state == CS_ALIVE || d->state == CS_EDITING))
			{
                part_icon(pos, textureload(conopentex, 3), statusaboveheadblend, 2);
				pos.z += 2;
			}
		}
		if(showweap) a[ai++] = modelattach("tag_weapon", weaptype[weap].vwep, ANIM_VWEP|ANIM_LOOP, 0); // we could probably animate this too now..
        if(rendernormally && (early || d != player1))
        {
        	a[ai++] = modelattach("tag_muzzle", &d->muzzle);
        	if(third) a[ai++] = modelattach("tag_affinity", &d->affinity);
        }
        renderclient(d, third, trans, team, a[0].tag ? a : NULL, secondary, animflags, animdelay, lastaction, early);
	}

	void render()
	{
#if 0
		if(intermission)
		{
			if(m_team(gamemode, mutators)) { bestteams.setsize(0); hud::sb.bestteams(bestteams); }
			else { bestplayers.setsize(0); hud::sb.bestplayers(bestplayers); }
		}
#endif
		gameent *d;
        loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && d != player1)
        {
        	if(rendernormally) d->muzzle = d->affinity = vec(-1, -1, -1);
			renderplayer(d, true, showtranslucent(d, true));
			if(rendernormally) checktags(d);
        }

		startmodelbatches(); // two batch passes
		entities::render();
		projs::render();
		if(m_stf(gamemode)) stf::render();
        if(m_ctf(gamemode)) ctf::render();
        ai::render();
		endmodelbatches();
	}

    void renderavatar(bool early)
    {
    	if(rendernormally && early) player1->muzzle = player1->affinity = vec(-1, -1, -1);
        if((thirdpersonview() || !rendernormally))
			renderplayer(player1, true, showtranslucent(player1, thirdpersonview(true)), early);
        else if(!thirdpersonview() && player1->state == CS_ALIVE)
            renderplayer(player1, false, showtranslucent(player1, false), early);
		if(rendernormally && early) checktags(player1);
    }

	bool clientoption(char *arg) { return false; }
}
#undef GAMEWORLD
