#include "pch.h"
#include "engine.h"
#include "game.h"
#include "server.h"
#ifndef STANDALONE
struct gameclient : igameclient
{
	#include "physics.h"
	#include "projs.h"
	#include "weapon.h"
	#include "scoreboard.h"
	#include "entities.h"
	#include "client.h"
	#include "ai.h"
	#include "stf.h"
    #include "ctf.h"

	int nextmode, nextmuts, gamemode, mutators;
	bool intermission;
	int maptime, minremain, swaymillis;
	dynent fpsmodel;
	vec swaydir;
    int lasthit, lastcamera, lastspec, lastzoom, lastmousetype;
    bool prevzoom, zooming;
	int quakewobble, damageresidue;
    int liquidchan;

	gameent *player1;				// our client
	vector<gameent *> players;		// other clients
	gameent lastplayerstate;

	IVARP(titlecardtime, 0, 2000, 10000);
	IVARP(titlecardfade, 0, 3000, 10000);
	IFVARP(titlecardsize, 0, 0.3f, 1);

	IVARP(invmouse, 0, 0, 1);
	IVARP(absmouse, 0, 0, 1);

	IVARP(thirdperson, 0, 0, 1);

	IVARP(thirdpersonmouse, 0, 0, 2);
	IVARP(thirdpersondeadzone, 0, 10, 100);
	IVARP(thirdpersonpanspeed, 1, 30, INT_MAX-1);
	IVARP(thirdpersonaim, 0, 250, INT_MAX-1);
	IVARP(thirdpersonfov, 90, 120, 150);
	IVARP(thirdpersontranslucent, 0, 0, 1);
	IVARP(thirdpersondist, -100, 1, 100);
	IVARP(thirdpersonshift, -100, 4, 100);
	IVARP(thirdpersonangle, 0, 40, 360);

	IVARP(firstpersonmouse, 0, 0, 2);
	IVARP(firstpersondeadzone, 0, 10, 100);
	IVARP(firstpersonpanspeed, 1, 30, INT_MAX-1);
	IVARP(firstpersonfov, 90, 100, 150);
	IVARP(firstpersonaim, 0, 0, INT_MAX-1);
	IVARP(firstpersonsway, 0, 100, INT_MAX-1);
	IVARP(firstpersontranslucent, 0, 0, 1);
	IFVARP(firstpersondist, -10000, -0.25f, 10000);
	IFVARP(firstpersonshift, -10000, 0.25f, 10000);
	IFVARP(firstpersonadjust, -10000, 0.f, 10000);

	IVARP(editmouse, 0, 0, 2);
	IVARP(editfov, 1, 120, 360);
	IVARP(editdeadzone, 0, 10, 100);
	IVARP(editpanspeed, 1, 20, INT_MAX-1);

	IVARP(spectv, 0, 1, 1); // 0 = float, 1 = tv
	IVARP(spectvtime, 0, 10000, INT_MAX-1);
	IVARP(specmouse, 0, 0, 2);
	IVARP(specfov, 1, 120, 360);
	IVARP(specdeadzone, 0, 10, 100);
	IVARP(specpanspeed, 1, 20, INT_MAX-1);

	IFVARP(sensitivity, 1e-3f, 10.0f, 1000);
	IFVARP(yawsensitivity, 1e-3f, 10.0f, 1000);
	IFVARP(pitchsensitivity, 1e-3f, 7.5f, 1000);
	IFVARP(mousesensitivity, 1e-3f, 1.0f, 1000);
	IFVARP(snipesensitivity, 1e-3f, 3.0f, 1000);
	IFVARP(pronesensitivity, 1e-3f, 5.0f, 1000);

	IVARP(crosshairclip, 0, 1, 1);
	IVARP(crosshairhitspeed, 0, 1000, INT_MAX-1);
	IFVARP(crosshairsize, 0, 0.05f, 1);
	IFVARP(cursorsize, 0, 0.05f, 1);
	IFVARP(cursorblend, 0, 1.f, 1);

	IFVARP(crosshairblend, 0, 0.3f, 1);
	IFVARP(indicatorblend, 0, 0.5f, 1);
	IFVARP(clipbarblend, 0, 0.2f, 1);
	IFVARP(radarblend, 0, 0.9f, 1);
	IFVARP(blipblend, 0, 1.0f, 1);
	IFVARP(barblend, 0, 1.0f, 1);
	IFVARP(candinalblend, 0, 1.0f, 1);
	IFVARP(ammoblend, 0, 0.8f, 1);
	IFVARP(ammoblendinactive, 0, 0.3f, 1);
	IFVARP(infoblend, 0, 1.f, 1);

	IVARP(radardist, 0, 512, 512);
	IVARP(radarnames, 0, 1, 2);
	IFVARP(radarsize, 0, 0.25f, 1);
	IFVARP(ammosize, 0, 0.07f, 1);
	IVARP(editradardist, 0, 512, INT_MAX-1);
	IVARP(editradarnoisy, 0, 1, 2);

	IVARP(showcrosshair, 0, 1, 1);
	IVARP(showdamage, 0, 1, 1);
	IVARP(showtips, 0, 2, 3);
	IVARP(showguns, 0, 2, 2);
	IVARP(shownamesabovehead, 0, 1, 2);
	IVARP(showindicator, 0, 1, 1);

	IVARP(showstats, 0, 0, 1);
	IVARP(showenttips, 0, 1, 2);
	IVARP(showhudents, 0, 10, 100);
	IVARP(showfps, 0, 2, 2);
	IVARP(statrate, 0, 200, 1000);

	IVARP(snipetype, 0, 0, 1);
	IVARP(snipemouse, 0, 0, 2);
	IVARP(snipedeadzone, 0, 25, 100);
	IVARP(snipepanspeed, 1, 10, INT_MAX-1);
	IVARP(snipefov, 1, 35, 150);
	IVARP(snipetime, 1, 300, 10000);
	IFVARP(snipecrosshairsize, 0, 0.5f, 1);

	IVARP(pronetype, 0, 0, 1);
	IVARP(pronemouse, 0, 0, 2);
	IVARP(pronedeadzone, 0, 25, 100);
	IVARP(pronepanspeed, 1, 10, INT_MAX-1);
	IVARP(pronefov, 70, 70, 150);
	IVARP(pronetime, 1, 150, 10000);

	ITVAR(relativecursortex, "textures/cursordot", 3);
	ITVAR(guicursortex, "textures/cursor", 3);
	ITVAR(editcursortex, "textures/cursordot", 3);
	ITVAR(speccursortex, "textures/cursordot", 3);
	ITVAR(crosshairtex, "textures/crosshair", 3);
	ITVAR(teamcrosshairtex, "textures/teamcrosshair", 3);
	ITVAR(hitcrosshairtex, "textures/hitcrosshair", 3);
	ITVAR(snipecrosshairtex, "textures/snipecrosshair", 3);

	ITVAR(bliptex, "textures/blip", 3);
	ITVAR(flagbliptex, "textures/flagblip", 3);
	ITVAR(radartex, "textures/radar", 0);
	ITVAR(healthbartex, "textures/healthbar", 0);

	ITVAR(indicatortex, "textures/indicator", 3);
	ITVAR(plasmahudtex, "textures/plasmahud", 0);
	ITVAR(shotgunhudtex, "textures/shotgunhud", 0);
	ITVAR(chaingunhudtex, "textures/chaingunhud", 0);
	ITVAR(grenadeshudtex, "textures/grenadeshud", 0);
	ITVAR(flamerhudtex, "textures/flamerhud", 0);
	ITVAR(carbinehudtex, "textures/carbinehud", 0);
	ITVAR(riflehudtex, "textures/riflehud", 0);
	ITVAR(plasmacliptex, "textures/plasmaclip", 3);
	ITVAR(shotguncliptex, "textures/shotgunclip", 3);
	ITVAR(chainguncliptex, "textures/chaingunclip", 3);
	ITVAR(grenadescliptex, "textures/grenadesclip", 3);
	ITVAR(flamercliptex, "textures/flamerclip", 3);
	ITVAR(carbinecliptex, "textures/carbineclip", 3);
	ITVAR(riflecliptex, "textures/rifleclip", 3);
	ITVAR(damagetex, "textures/damage", 0);
	ITVAR(snipetex, "textures/snipe", 0);

	gameclient()
		: ph(*this), pj(*this), ws(*this), sb(*this), et(*this), cc(*this), ai(*this), stf(*this), ctf(*this),
			nextmode(G_LOBBY), nextmuts(0), gamemode(G_LOBBY), mutators(0), intermission(false),
			maptime(0), minremain(0), swaymillis(0), swaydir(0, 0, 0),
			lasthit(0), lastcamera(0), lastspec(0), lastzoom(0), lastmousetype(0),
			prevzoom(false), zooming(false),
			quakewobble(0), damageresidue(0),
			liquidchan(-1),
			player1(new gameent())
	{
        CCOMMAND(kill, "",  (gameclient *self), { self->suicide(self->player1, 0); });
		CCOMMAND(mode, "ii", (gameclient *self, int *val, int *mut), { self->setmode(*val, *mut); });
		CCOMMAND(gamemode, "", (gameclient *self), intret(self->gamemode));
		CCOMMAND(mutators, "", (gameclient *self), intret(self->mutators));
		CCOMMAND(zoom, "D", (gameclient *self, int *down), { self->dozoom(*down!=0); });
		s_strcpy(player1->name, "unnamed");
	}

	iclientcom *getcom() { return &cc; }
	icliententities *getents() { return &et; }

	char *gametitle() { return sv->gamename(gamemode, mutators); }
	char *gametext() { return getmapname(); }

	float radarrange()
	{
		float dist = float(radardist());
		if(player1->state == CS_EDITING) dist = float(editradardist());
		return dist;
	}

	bool isthirdperson()
	{
		if(!thirdperson()) return false;
		if(player1->state == CS_EDITING) return false;
		if(player1->state == CS_SPECTATOR) return false;
		if(player1->state == CS_WAITING) return false;
		if(inzoom()) return false;
		return true;
	}

	int mousestyle()
	{
		if(player1->state == CS_EDITING) return editmouse();
		if(player1->state == CS_SPECTATOR || player1->state == CS_WAITING) return specmouse();
		if(inzoom()) return player1->gunselect == GUN_RIFLE ? snipemouse() : pronemouse();
		if(isthirdperson()) return thirdpersonmouse();
		return firstpersonmouse();
	}

	int deadzone()
	{
		if(player1->state == CS_EDITING) return editdeadzone();
		if(player1->state == CS_SPECTATOR || player1->state == CS_WAITING) return specdeadzone();
		if(inzoom()) return player1->gunselect == GUN_RIFLE ? snipedeadzone() : pronedeadzone();
		if(isthirdperson()) return thirdpersondeadzone();
		return firstpersondeadzone();
	}

	int panspeed()
	{
		if(inzoom()) return player1->gunselect == GUN_RIFLE ? snipepanspeed() : pronepanspeed();
		if(player1->state == CS_EDITING) return editpanspeed();
		if(player1->state == CS_SPECTATOR || player1->state == CS_WAITING) return specpanspeed();
		if(isthirdperson()) return thirdpersonpanspeed();
		return firstpersonpanspeed();
	}

	int fov()
	{
		if(player1->state == CS_EDITING) return editfov();
		if(player1->state == CS_SPECTATOR || player1->state == CS_WAITING) return specfov();
		if(isthirdperson()) return thirdpersonfov();
		return firstpersonfov();
	}

	void zoomset(bool on, int millis)
	{
		if(on != zooming)
		{
			resetcursor();
			lastzoom = millis;
			prevzoom = zooming;
		}
		zooming = on;
	}

	bool zoomallow()
	{
		if(allowmove(player1)) return true;
		zoomset(false, 0);
		return false;
	}

	int zoomtime()
	{
		return player1->gunselect == GUN_RIFLE ? snipetime() : pronetime();
	}

	bool inzoom()
	{
		if(zoomallow() && (zooming || lastmillis-lastzoom < zoomtime()))
			return true;
		return false;
	}

	bool inzoomswitch()
	{
		if(zoomallow() && ((zooming && lastmillis-lastzoom > zoomtime()/2) || (!zooming && lastmillis-lastzoom < zoomtime()/2)))
			return true;
		return false;
	}

	void dozoom(bool down)
	{
		if(zoomallow())
		{
			bool on = false;
			switch(player1->gunselect == GUN_RIFLE ? snipetype() : pronetype())
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

	void addsway(gameent *d)
	{
		if(firstpersonsway())
		{
			if(d->physstate >= PHYS_SLOPE) swaymillis += curtime;

			float k = pow(0.7f, curtime/10.0f);
			swaydir.mul(k);
			vec vel(d->vel);
			vel.add(d->falling);
			swaydir.add(vec(vel).mul((1-k)/(15*max(vel.magnitude(), ph.maxspeed(d)))));
		}
		else swaydir = vec(0, 0, 0);
	}

	int respawnwait(gameent *d)
	{
		int wait = 0;
		if(m_stf(gamemode)) wait = stf.respawnwait(d);
		else if(m_ctf(gamemode)) wait = ctf.respawnwait(d);
		return wait;
	}

	void respawn(gameent *d)
	{
		if(d->state == CS_DEAD && !respawnwait(d))
			respawnself(d);
	}

	bool tvmode()
	{
		return player1->state == CS_SPECTATOR && spectv();
	}

    bool allowmove(physent *d)
    {
        if(d->type == ENT_PLAYER)
        {
        	if(d == player1)
        	{
        		if(g3d_active(true, true)) return false;
				if(tvmode()) return false;
        	}
			if(d->state == CS_DEAD) return false;
			if(intermission) return false;
        }
        return true;
    }

	void respawnself(gameent *d)
	{
		d->stopmoving();

		if(d->respawned != d->lifesequence)
		{
			cc.addmsg(SV_TRYSPAWN, "ri", d->clientnum);
			d->respawned = d->lifesequence;
		}
	}

	gameent *pointatplayer()
	{
		loopv(players)
		{
			gameent *o = players[i];
			if(!o) continue;
			vec pos = headpos(player1, 0.f);
			if(intersect(o, pos, worldpos)) return o;
		}
		return NULL;
	}

	void setmode(int mode, int muts)
	{
		nextmode = mode; nextmuts = muts;
		sv->modecheck(&nextmode, &nextmuts);
	}

	void resetstates(int types)
	{
		if(types & ST_CAMERA)
		{
			lastcamera = 0;
			zoomset(false, 0);
		}
		if(types & ST_CURSOR) resetcursor();
		if(types & ST_GAME)
		{
			sb.showscores(false);
			lasthit = 0;
		}
		if(types & ST_SPAWNS)
		{
			pj.reset();
			// reset perma-state
			gameent *d;
			loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && d->type == ENT_PLAYER)
				d->resetstate(lastmillis);
		}
	}

	void checkoften(gameent *d)
	{
		heightoffset(d, d == player1 || d->ai);
		loopi(GUN_MAX) if(d->gunstate[i] != GNS_IDLE)
		{
			if(d->state != CS_ALIVE || (d->gunstate[i] != GNS_POWER && lastmillis-d->gunlast[i] >= d->gunwait[i]))
				d->setgunstate(i, GNS_IDLE, 0, lastmillis);
		}

		if(d->reqswitch > 0 && lastmillis-d->reqswitch > GUNSWITCHDELAY*2)
			d->reqswitch = -1;
		if(d->reqreload > 0 && lastmillis-d->reqreload > guntype[d->gunselect].rdelay*2)
			d->reqreload = -1;
		if(d->requse > 0 && lastmillis-d->requse > GUNSWITCHDELAY*2)
			d->requse = -1;
	}


	void otherplayers()
	{
		loopv(players) if(players[i])
		{
            gameent *d = players[i];
            const int lagtime = lastmillis-d->lastupdate;
            checkoften(d);
            if(d->ai || !lagtime || intermission) continue;
            else if(lagtime>1000 && d->state==CS_ALIVE)
			{
                d->state = CS_LAGGED;
				continue;
			}
			ph.smoothplayer(d, 1, false);
		}
	}

	void updateworld()		// main game update loop
	{
		if(!curtime) return;
        if(!maptime)
        {
        	maptime = lastmillis;
        	return;
        }

        if(connected())
        {
            // do shooting/projectile update here before network update for greater accuracy with what the player sees

            ph.update();
            pj.update();
            et.update();
            ai.update();

            if(player1->state == CS_ALIVE) ws.shoot(player1, worldpos);

            otherplayers();
        }

		gets2c();

		if(connected())
		{
			if(!allowmove(player1)) player1->stopmoving();
            checkoften(player1);

			#define adjustscaled(t,n) \
				if(n > 0) { n = (t)(n/(1.f+sqrtf((float)curtime)/100.f)); if(n <= 0) n = (t)0; }

			adjustscaled(float, player1->roll);
			adjustscaled(int, quakewobble);
			adjustscaled(int, damageresidue);

			if(player1->state == CS_DEAD)
			{
				if(lastmillis-player1->lastpain < 2000)
					ph.move(player1, 10, false);
			}
			else if(player1->state == CS_ALIVE)
			{
				ph.move(player1, 10, true);
				addsway(player1);
				et.checkitems(player1);
				//ws.shoot(player1, worldpos);
				ws.reload(player1);
			}
			else ph.move(player1, 10, true);
		}

		if(player1->clientnum >= 0) c2sinfo(40);
	}

	void damaged(int gun, int flags, int damage, int health, gameent *d, gameent *actor, int millis, vec &dir)
	{
		if(d->state != CS_ALIVE || intermission) return;

        d->lastregen = d->lastpain = lastmillis;
        d->health = health;

		if(actor->type == ENT_PLAYER) actor->totaldamage += damage;

		if(d == player1)
		{
			quakewobble += damage/2;
			damageresidue += damage*2;
		}
		if(d == player1 || d->ai) d->hitpush(damage, dir);

		if(d->type == ENT_PLAYER)
		{
			vec p = headpos(d);
			p.z += 0.6f*(d->height + d->aboveeye) - d->height;
			part_splash(PART_BLOOD, max(damage/3, 3), REGENWAIT, p, 0x66FFFF, 3.f);
			s_sprintfd(ds)("@%d", damage);
			part_text(vec(d->abovehead()).sub(vec(0, 0, 3)), ds, PART_TEXT_RISE, 3000, 0xFFFFFF, 3.f);
			playsound(S_PAIN1+rnd(5), d->o, d);
		}

		if(d != actor)
		{
			int snd = 0;
			if(damage >= 200) snd = 7;
			else if(damage >= 150) snd = 6;
			else if(damage >= 100) snd = 5;
			else if(damage >= 75) snd = 4;
			else if(damage >= 50) snd = 3;
			else if(damage >= 25) snd = 2;
			else if(damage >= 10) snd = 1;
			playsound(S_DAMAGE1+snd, actor->o, actor);
			if(actor == player1) lasthit = lastmillis;
		}

		ai.damaged(d, actor, gun, flags, damage, health, millis, dir);
	}

	void killed(int gun, int flags, int damage, gameent *d, gameent *actor)
	{
		if(d->type != ENT_PLAYER) return;

		d->obliterated = d == actor || flags&HIT_EXPLODE || flags&HIT_MELT || damage > MAXHEALTH;
        d->lastregen = d->lastpain = lastmillis;
		d->state = CS_DEAD;
		d->deaths++;

		int anc = -1, dth = S_DIE1+rnd(2);
		if(flags&HIT_MELT || flags&HIT_BURN) dth = S_BURN;
		else if(d->obliterated) dth = S_SPLAT;

		if(d == player1)
		{
			anc = S_V_FRAGGED;
			sb.showscores(true);
			lastplayerstate = *player1;
			d->stopmoving();
			d->pitch = 0;
			d->roll = 0;
		}
		else
        {
            d->move = d->strafe = 0;
            d->resetinterp();
        }

		s_strcpy(d->obit, "rests in pieces");
        if(d == actor)
        {
        	if(flags&HIT_MELT) s_strcpy(d->obit, "melted");
			else if(flags&HIT_FALL) s_strcpy(d->obit, "thought they could fly");
        	else if(flags && isgun(gun))
        	{
				static const char *suicidenames[GUN_MAX] = {
					"found out what their plasma tasted like",
					"discovered buckshot bounces",
					"got caught up in their own crossfire",
					"barbequed themselves for dinner",
					"pulled off a seemingly impossible stunt",
					"pulled off a seemingly impossible stunt",
					"decided to kick it, kamakaze style",
				};
        		s_strcpy(d->obit, suicidenames[gun]);
        	}
        	else if(flags&HIT_EXPLODE) s_strcpy(d->obit, "was obliterated");
        	else if(flags&HIT_BURN) s_strcpy(d->obit, "burnt up");
        	else s_strcpy(d->obit, "suicided");
        }
		else
		{
			static const char *obitnames[3][GUN_MAX] = {
				{
					"was plasmified by",
					"was filled with buckshot by",
					"was riddled with holes by",
					"was char-grilled by",
					"was skewered by",
					"was pierced by",
					"was blown to pieces by",
				},
				{
					"was plasmafied by",
					"was given scrambled brains cooked up by",
					"was air conditioned courtesy of",
					"was char-grilled by",
					"was given an extra orifice by",
					"was expertly sniped by",
					"was blown to pieces by",
				},
				{
					"was reduced to ooze by",
					"was turned into little chunks by",
					"was swiss-cheesed by",
					"was made the main course by order of chef",
					"was spliced by",
					"had their head blown clean off by",
					"was obliterated by",
				}
			};

			int o = d->obliterated ? 2 : (flags&HIT_HEAD && !guntype[gun].explode ? 1 : 0);
			const char *oname = isgun(gun) ? obitnames[o][gun] : "was killed by";
			if(m_team(gamemode, mutators) && d->team == actor->team)
				s_sprintf(d->obit)("%s teammate %s", oname, colorname(actor));
			else
			{
				s_sprintf(d->obit)("%s %s", oname, colorname(actor));
				switch(actor->spree)
				{
					case 5:
					{
						s_strcat(d->obit, " in total carnage!");
						anc = S_V_SPREE1;
						s_sprintfd(ds)("@\fgCARNAGE");
						part_text(actor->abovehead(), ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 4.f);
						break;
					}
					case 10:
					{
						s_strcat(d->obit, " who is slaughtering!");
						anc = S_V_SPREE2;
						s_sprintfd(ds)("@\fgSLAUGHTER");
						part_text(actor->abovehead(), ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 4.f);
						break;
					}
					case 25:
					{
						s_strcat(d->obit, " going on a massacre!");
						anc = S_V_SPREE3;
						s_sprintfd(ds)("@\fgMASSACRE");
						part_text(actor->abovehead(), ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 4.f);
						break;
					}
					case 50:
					{
						s_strcat(d->obit, " creating a bloodbath!");
						anc = S_V_SPREE4;
						s_sprintfd(ds)("@\fgBLOODBATH");
						part_text(actor->abovehead(), ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 4.f);
						break;
					}
					default:
					{
						if(flags&HIT_HEAD)
						{
							anc = S_V_HEADSHOT;
							s_sprintfd(ds)("@\fgHEADSHOT");
							part_text(actor->abovehead(), ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 4.f);
						}
						else if(d->obliterated || lastmillis-d->lastspawn <= REGENWAIT*3)
						{
							anc = S_V_OWNED;
						}
						break;
					}
				}
			}
		}
		bool af = (d == player1 || actor == player1);
		if(dth >= 0) playsound(dth, d->o, d);
		s_sprintfd(a)("\fy%s %s", colorname(d), d->obit);
		et.announce(anc, a, af);
		s_sprintfd(da)("@%s", a);
		part_text(vec(d->abovehead()).add(vec(0, 0, 4)), da, PART_TEXT_RISE, 5000, 0xFFFFFF, 3.f);

		vec pos = headpos(d);
		int gdiv = d->obliterated ? 2 : 4, gibs = clamp((damage+gdiv)/gdiv, 1, 20);
		loopi(rnd(gibs)+1)
			pj.create(pos, vec(pos).add(d->vel), true, d, PRJ_GIBS, rnd(2000)+1000, rnd(100)+1, 50);

		ai.killed(d, actor, gun, flags, damage);
	}

	void timeupdate(int timeremain)
	{
		minremain = timeremain;
		if(!timeremain)
		{
			if(!intermission)
			{
				player1->stopmoving();
				sb.showscores(true, true);
				intermission = true;
			}
		}
		else if(timeremain > 0)
		{
			console("\f2time remaining: %d %s", CON_NORMAL|CON_CENTER, timeremain, timeremain==1 ? "minute" : "minutes");
		}
	}

	gameent *newclient(int cn)	// ensure valid entity
	{
		if(cn < 0 || cn >= MAXCLIENTS)
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

	gameent *getclient(int cn)	// ensure valid entity
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
		if(d->name[0]) conoutf("\fo%s left the game", colorname(d));
		pj.remove(d);
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
    	ws.preload();
		pj.preload();
        et.preload();
		if(m_edit(gamemode) || m_stf(gamemode)) stf.preload();
        if(m_edit(gamemode) || m_ctf(gamemode)) ctf.preload();
    }

	void startmap(const char *name)	// called just after a map load
	{
		const char *title = getmaptitle();
		if(*title) console("%s", CON_CENTER|CON_NORMAL, title);
		intermission = false;
        player1->respawned = player1->suicided = maptime = 0;
        preload();
        et.mapstart();
		cc.mapstart();
        resetstates(ST_ALL);

        // prevent the player from being in the middle of nowhere if he doesn't get spawned
        et.findplayerspawn(player1);
	}

	void playsoundc(int n, gameent *d = NULL)
	{
		if(n < 0 || n >= S_MAX) return;
		gameent *c = d ? d : player1;
		if(c == player1 || c->ai) cc.addmsg(SV_SOUND, "i2", c->clientnum, n);
		playsound(n, c->o, c);
	}

	gameent *intersectclosest(vec &from, vec &to, gameent *at)
	{
		gameent *best = NULL, *o;
		float bestdist = 1e16f;
		loopi(numdynents()) if((o = (gameent *)iterdynents(i)))
		{
            if(!o || o==at || o->state!=CS_ALIVE || lastmillis-o->lastspawn <= REGENWAIT) continue;
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

	char *colorname(gameent *d, char *name = NULL, const char *prefix = "", bool team = true, bool dupname = true)
	{
		if(!name) name = d->name;
		static string cname;
		s_sprintf(cname)("%s\fs%s\fS", *prefix ? prefix : "", name);
		if(!name[0] || d->aitype != AI_NONE || (dupname && duplicatename(d, name)))
		{
			s_sprintfd(s)(" [\fs%s%d\fS]", d->aitype != AI_NONE ? "\fc" : "\fm", d->clientnum);
			s_strcat(cname, s);
		}
		if(team && m_team(gamemode, mutators))
		{
			s_sprintfd(s)(" (\fs%s%s\fS)", teamtype[d->team].chat, teamtype[d->team].name);
			s_strcat(cname, s);
		}
		return cname;
	}

	void suicide(gameent *d, int flags)
	{
		if(d == player1 || d->ai)
		{
			if(d->state!=CS_ALIVE) return;
			if(d->suicided!=d->lifesequence)
			{
				cc.addmsg(SV_SUICIDE, "ri2", d->clientnum, flags);
				d->suicided = d->lifesequence;
			}
		}
	}

	enum
	{
		POINTER_NONE = 0,
		POINTER_RELATIVE,
		POINTER_GUI,
		POINTER_EDIT,
		POINTER_SPEC,
		POINTER_HAIR,
		POINTER_TEAM,
		POINTER_HIT,
		POINTER_SNIPE,
		POINTER_MAX
	};

    const char *getpointer(int index)
    {
        switch(index)
        {
            case POINTER_RELATIVE: default: return relativecursortex(); break;
            case POINTER_GUI: return guicursortex(); break;
            case POINTER_EDIT: return editcursortex(); break;
            case POINTER_SPEC: return speccursortex(); break;
            case POINTER_HAIR: return crosshairtex(); break;
            case POINTER_TEAM: return teamcrosshairtex(); break;
            case POINTER_HIT: return hitcrosshairtex(); break;
            case POINTER_SNIPE: return snipecrosshairtex(); break;
        }
        return NULL;
    }

    void drawclip(int gun, float x, float y, float size, float blend, Texture *pointer = NULL)
    {
        const char *cliptexs[GUN_MAX] = {
            plasmacliptex(), shotguncliptex(), chainguncliptex(),
            flamercliptex(), carbinecliptex(), riflecliptex(), grenadescliptex(),
        };
        float px = x, py = y, psize = size;
        Texture *t = textureload(cliptexs[gun], 3);
        if(pointer)
        {
            psize *= t->w/float(pointer->w);
            px -= psize/2.0f;
            py -= psize/2.0f;
        }
        int ammo = player1->ammo[gun], maxammo = guntype[gun].max;
        glBindTexture(GL_TEXTURE_2D, t->retframe(ammo, maxammo));
        glColor4f(1.f, 1.f, 1.f, blend);
        float cx = px + psize/2.0f, cy = py + psize/2.0f;

        if(t->frames.length()>1) drawtex(px, py, psize, psize);
        else switch(gun)
        {
            case GUN_FLAMER:
                drawslice(0,
                          max(ammo-min(maxammo - ammo, 2), 0)/float(maxammo),
                          cx, cy, psize/2.0f);
                if(player1->ammo[gun] < guntype[gun].max)
                {
                    drawfadedslice(max(ammo-min(maxammo - ammo, 2), 0)/float(maxammo),
                                   min(min(maxammo - ammo, ammo), 2) /float(maxammo),
                                   cx, cy, psize/2.0f,
                                   blend);
                }
                break;

            default:
                drawslice(0.5f/maxammo,
                          ammo/float(maxammo),
                          cx, cy, psize/2.0f);
                break;
        }
		if(showindicator() && guntype[gun].power && player1->gunselect == gun && player1->gunstate[gun] == GNS_POWER)
		{
			px = x;
			py = y;
			psize = size;
			t = textureload(indicatortex(), 3);
			if(pointer)
			{
				psize *= t->w/float(pointer->w);
				px -= psize/2.0f;
				py -= psize/2.0f;
			}

			if(t->bpp == 32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			else glBlendFunc(GL_ONE, GL_ONE);

			cx = px + psize/2.0f;
			cy = py + psize/2.0f;

			float amt = clamp(float(lastmillis-player1->gunlast[gun])/float(guntype[gun].power), 0.f, 1.f);
			glBindTexture(GL_TEXTURE_2D, t->retframe(lastmillis-player1->gunlast[gun], guntype[gun].power));
			glColor4f(clamp(amt, 0.3f, 1.f), clamp(amt, 0.3f, 1.f), 0.f, indicatorblend());

			if(t->frames.length() > 1) drawtex(px, py, psize, psize);
			else drawslice(0, amt, cx, cy, psize/2.0f);
		}
    }

	void drawpointer(int w, int h, int index, float x, float y, float r, float g, float b)
	{
		Texture *pointer = textureload(getpointer(index), 3, true);
		if(pointer)
		{
			float chsize = crosshairsize()*h*3.f, blend = crosshairblend();
			if(index == POINTER_GUI)
			{
				chsize = cursorsize()*h*3.f;
				blend = cursorblend();
			}
			else if(index == POINTER_SNIPE)
			{
				chsize = snipecrosshairsize()*h*3.f;
				if(inzoom() && player1->gunselect == GUN_RIFLE)
				{
					int frame = lastmillis-lastzoom;
					float amt = frame < zoomtime() ? clamp(float(frame)/float(zoomtime()), 0.f, 1.f) : 1.f;
					if(!zooming) amt = 1.f-amt;
					chsize *= amt;
				}
			}

			if(pointer->bpp == 32) glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			else glBlendFunc(GL_ONE, GL_ONE);
			glColor4f(r, g, b, blend);

			float ox = x*w*3.f, oy = y*h*3.f, os = index != POINTER_GUI ? chsize/2.0f : 0,
				cx = ox-os, cy = oy-os;
			glBindTexture(GL_TEXTURE_2D, pointer->id);
			glBegin(GL_QUADS);
			glTexCoord2f(0.0f, 0.0f); glVertex2f(cx, cy);
			glTexCoord2f(1.0f, 0.0f); glVertex2f(cx + chsize, cy);
			glTexCoord2f(1.0f, 1.0f); glVertex2f(cx + chsize, cy + chsize);
			glTexCoord2f(0.0f, 1.0f); glVertex2f(cx, cy + chsize);
			glEnd();

			if(index > POINTER_GUI && player1->state == CS_ALIVE && isgun(player1->gunselect) && player1->hasgun(player1->gunselect) && crosshairclip())
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				drawclip(player1->gunselect, ox, oy, chsize, clipbarblend(), pointer);
			}
		}
	}

	void drawpointers(int w, int h)
	{
        int index = POINTER_NONE;

		if(g3d_active(true, false))
		{
			if(g3d_active()) index = POINTER_GUI;
			else return;
		}
        else if(hidehud || !showcrosshair() || player1->state == CS_DEAD || !connected()) return;
        else if(player1->state == CS_EDITING) index = POINTER_EDIT;
        else if(player1->state == CS_SPECTATOR || player1->state == CS_WAITING) index = POINTER_SPEC;
        else if(inzoom() && player1->gunselect == GUN_RIFLE) index = POINTER_SNIPE;
        else if(lastmillis-lasthit <= crosshairhitspeed()) index = POINTER_HIT;
        else if(m_team(gamemode, mutators))
        {
            vec pos = headpos(player1, 0.f);
            dynent *d = intersectclosest(pos, worldpos, player1);
            if(d && d->type == ENT_PLAYER && ((gameent *)d)->team == player1->team)
				index = POINTER_TEAM;
			else index = POINTER_HAIR;
        }
        else index = POINTER_HAIR;

		float r = 1.f, g = 1.f, b = 1.f;
		if(index >= POINTER_HAIR)
		{
			if(r && g && b && player1->state == CS_ALIVE)
			{
				if(player1->health<=25) { r = 1; g = b = 0; }
				else if(player1->health<=50) { r = 1; g = 0.5f; b = 0; }
			}
            if(!player1->canshoot(player1->gunselect, lastmillis)) { r *= 0.5f; g *= 0.5f; b *= 0.5f; }
		}

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, w*3, h*3, 0, -1, 1);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);

		float curx = aimx, cury = aimy;
		if(index < POINTER_EDIT || mousestyle() == 2)
		{
			curx = cursorx;
			cury = cursory;
		}
		else if(isthirdperson() ? thirdpersonaim() : firstpersonaim())
			curx = cury = 0.5f;

		drawpointer(w, h, index, curx, cury, r, g, b);

		if(index > POINTER_GUI && mousestyle() >= 1)
		{
			curx = mousestyle() == 1 ? cursorx : 0.5f;
			cury = mousestyle() == 1 ? cursory : 0.5f;
			drawpointer(w, h, POINTER_RELATIVE, curx, cury, r, g, b);
		}
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
	}

	void drawtex(float x, float y, float w, float h)
	{
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(x,	y);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(x+w, y);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(x+w, y+h);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(x,	y+h);
		glEnd();
	}
	void drawsized(float x, float y, float s) { drawtex(x, y, s, s); }

	void drawplayerblip(gameent *d, int x, int y, int s)
	{
		vec dir = headpos(d);
		dir.sub(camera1->o);
		float dist = dir.magnitude();
		if(dist < radarrange())
		{
			dir.rotate_around_z(-camera1->yaw*RAD);
			int colour = teamtype[d->team].colour;
			float cx = x + s*0.5f*(1.0f+dir.x/radarrange()),
				cy = y + s*0.5f*(1.0f+dir.y/radarrange()),
				cs = (d->crouching ? 0.025f : 0.05f)*s,
				r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f,
				fmag = clamp(d->vel.magnitude()/ph.maxspeed(d), 0.f, 1.f),
				fade = clamp(1.f-(dist/radarrange()), 0.f, 1.f)*blipblend()*fmag;
			if(lastmillis-d->lastspawn <= REGENWAIT)
				fade *= clamp(float(lastmillis-d->lastspawn)/float(REGENWAIT), 0.f, 1.f);
			settexture(bliptex(), 3);
			glColor4f(r, g, b, fade);
			drawsized(cx-cs*0.5f, cy-cs*0.5f, cs);
			int ty = int(cy+cs);
			if(radarnames())
				ty += draw_textx("%s", int(cx), ty, 255, 255, 255, int(fade*255.f), false, AL_CENTER, -1, -1, colorname(d, NULL, "", false));
			if(radarnames() == 2 && m_team(gamemode, mutators))
				ty += draw_textx("(\fs%s%s\fS)", int(cx), ty, 255, 255, 255, int(fade*255.f), false, AL_CENTER, -1, -1, teamtype[d->team].chat, teamtype[d->team].name);
		}
	}

	void drawcardinalblips(int x, int y, int s)
	{
		pushfont("emphasis");
		loopi(4)
		{
			const char *card = "";
			vec dir(camera1->o);
			switch(i)
			{
				case 0: dir.sub(vec(0, radarrange(), 0)); card = "N"; break;
				case 1: dir.add(vec(radarrange(), 0, 0)); card = "E"; break;
				case 2: dir.add(vec(0, radarrange(), 0)); card = "S"; break;
				case 3: dir.sub(vec(radarrange(), 0, 0)); card = "W"; break;
				default: break;
			}
			dir.sub(camera1->o);
			dir.rotate_around_z(-camera1->yaw*RAD);

			float cx = x + (s-FONTW)*0.5f*(1.0f+dir.x/radarrange()),
				cy = y + (s-FONTH)*0.5f*(1.0f+dir.y/radarrange());

			draw_textx("%s", int(cx), int(cy), 255, 255, 255, int(255*candinalblend()), true, AL_LEFT, -1, -1, card);
		}
		popfont();
	}

	void drawentblip(int x, int y, int s, int n, vec &o, int type, int attr1, int attr2, int attr3, int attr4, int attr5, bool spawned, int lastspawn)
	{
		if(type > NOTUSED && type < MAXENTTYPES && ((enttype[type].usetype == EU_ITEM && spawned) || player1->state == CS_EDITING))
		{
			bool insel = player1->state == CS_EDITING && et.ents.inrange(n) && (enthover == n || entgroup.find(n) >= 0);
			float inspawn = spawned && lastspawn && lastmillis-lastspawn <= 1000 ? float(lastmillis-lastspawn)/1000.f : 0.f;
			if(enttype[type].noisy && (player1->state != CS_EDITING || !editradarnoisy() || (editradarnoisy() < 2 && !insel)))
				return;
			vec dir(o);
			dir.sub(camera1->o);
			float dist = dir.magnitude();
			if(dist >= radarrange())
			{
				if(insel || inspawn) dir.mul(radarrange()/dist);
				else return;
			}
			dir.rotate_around_z(-camera1->yaw*RAD);
			float cx = x + s*0.5f*0.95f*(1.0f+dir.x/radarrange()), cy = y + s*0.5f*0.95f*(1.0f+dir.y/radarrange()),
				cs = (inspawn > 0.f ? (2.0f-inspawn)*0.025f : (insel ? 0.033f : 0.025f))*s,
					range = (inspawn > 0.f ? 2.f-inspawn : 1.f)-(insel ? 1.f : (dist/radarrange())),
						fade = clamp(range, 0.f, 1.f)*blipblend();
			settexture(bliptex(), 3);
			if(inspawn > 0.f)
			{
				glColor4f(1.f, 1.f, 1.f, fade*(1.f-inspawn));
				drawsized(cx-(cs*0.5f)-(inspawn*cs), cy-(cs*0.5f)-(inspawn*cs), cs+(inspawn*cs*2.f));
			}
			glColor4f(1.f, insel ? 0.5f : 1.f, inspawn > 0.f ? inspawn : 0.f, fade);
			drawsized(cx-(cs*0.5f), cy-(cs*0.5f), cs);
		}
	}

	void drawentblips(int x, int y, int s)
	{
		loopv(et.ents)
		{
			gameentity &e = *(gameentity *)et.ents[i];
			drawentblip(x, y, s, i, e.o, e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, e.spawned, e.lastspawn);
		}

		loopv(pj.projs) if(pj.projs[i]->projtype == PRJ_ENT && pj.projs[i]->ready())
		{
			projent &proj = *pj.projs[i];
			if(et.ents.inrange(proj.id))
				drawentblip(x, y, s, -1, proj.o, proj.ent, proj.attr1, proj.attr2, proj.attr3, proj.attr4, proj.attr5, true, proj.spawntime);
		}
	}

	void drawtitlecard(int w, int h)
	{
		int ox = w*3, oy = h*3;

		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);
		pushfont("emphasis");

		int bs = int(oy*titlecardsize()), bp = int(oy*0.01f), bx = ox-bs-bp, by = bp,
			secs = maptime ? lastmillis-maptime : 0;
		float fade = hudblend, amt = 1.f;

		if(secs < titlecardtime())
		{
			amt = clamp(float(secs)/float(titlecardtime()), 0.f, 1.f);
			fade = clamp(amt*fade, 0.f, 1.f);
		}
		else if(secs < titlecardtime()+titlecardfade())
			fade = clamp(fade*(1.f-(float(secs-titlecardtime())/float(titlecardfade()))), 0.f, 1.f);

		const char *title = getmaptitle();
		if(!*title) title = getmapname();

		int rs = int(bs*amt), rx = bx+(bs-rs), ry = by;
		glColor4f(1.f, 1.f, 1.f, fade*0.9f);
		if(!rendericon(getmapname(), rx, ry, rs, rs))
			rendericon("textures/emblem", rx, ry, rs, rs);
		glColor4f(1.f, 1.f, 1.f, fade);
		rendericon(guioverlaytex, rx, ry, rs, rs);

		int tx = bx + bs, ty = by + bs + FONTH/2, ts = int(tx*(1.f-amt));
		ty += draw_textx("%s", tx-ts, ty, 255, 255, 255, int(255.f*fade), false, AL_RIGHT, -1, tx-FONTH, sv->gamename(gamemode, mutators));
		ty += draw_textx("%s", tx-ts, ty, 255, 255, 255, int(255.f*fade), false, AL_RIGHT, -1, tx-FONTH, title);
		popfont();
	}

	void drawgamehud(int w, int h)
	{
		Texture *t;
		int ox = w*3, oy = h*3;

		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);
		pushfont("emphasis");

		int secs = maptime ? lastmillis-maptime : 0;
		float fade = hudblend;

		if(secs < titlecardtime()+titlecardfade()+titlecardfade())
		{
			float amt = clamp(float(secs-titlecardtime()-titlecardfade())/float(titlecardfade()), 0.f, 1.f);
			fade = clamp(fade*amt, 0.f, 1.f);
		}

		if(player1->state == CS_ALIVE && inzoom() && player1->gunselect == GUN_RIFLE)
		{
			t = textureload(snipetex());
			int frame = lastmillis-lastzoom;
			float pc = frame < zoomtime() ? float(frame)/float(zoomtime()) : 1.f;
			if(!zooming) pc = 1.f-pc;

			glBindTexture(GL_TEXTURE_2D, t->id);
			glColor4f(1.f, 1.f, 1.f, pc);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(0, 0);
			glTexCoord2f(1, 0); glVertex2f(ox, 0);
			glTexCoord2f(1, 1); glVertex2f(ox, oy);
			glTexCoord2f(0, 1); glVertex2f(0, oy);
			glEnd();
		}

		if(showdamage() && ((player1->state == CS_ALIVE && damageresidue > 0) || player1->state == CS_DEAD))
		{
			t = textureload(damagetex());
			int dam = player1->state == CS_DEAD ? 100 : min(damageresidue, 100);
			float pc = float(dam)/100.f;

			glBindTexture(GL_TEXTURE_2D, t->id);
			glColor4f(1.f, 1.f, 1.f, pc);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0); glVertex2f(0, 0);
			glTexCoord2f(1, 0); glVertex2f(ox, 0);
			glTexCoord2f(1, 1); glVertex2f(ox, oy);
			glTexCoord2f(0, 1); glVertex2f(0, oy);
			glEnd();
		}

		int bs = int(oy*radarsize()), bo = int(bs/16.f), bp = int(oy*0.01f), bx = ox-bs-bp, by = bp,
			colour = teamtype[player1->team].colour,
				r = (colour>>16), g = ((colour>>8)&0xFF), b = (colour&0xFF);

		pushfont("radar");
		settexture(radartex());
		glColor4f((r/255.f)*0.25f, (g/255.f)*0.25f, (b/255.f)*0.25f, fade*radarblend());
		drawsized(float(bx), float(by), float(bs));

		drawentblips(bx+bo, by+bo, bs-(bo*2));

		if(m_stf(gamemode)) stf.drawblips(ox, oy, bx+bo, by+bo, bs-(bo*2));
		else if(m_ctf(gamemode)) ctf.drawblips(ox, oy, bx+bo, by+bo, bs-(bo*2));

		loopv(players)
			if(players[i] && players[i]->state == CS_ALIVE)
				drawplayerblip(players[i], bx+bo, by+bo, bs-(bo*2));
		popfont(); // radar

		int tp = by + bs + FONTH/2;
		if(player1->state == CS_ALIVE)
		{
			t = textureload(healthbartex());
			float amt = max(player1->health, 0)/float(MAXHEALTH),
				glow = 1.f, pulse = fade, cr = 1.f*amt, cg = 0.3f*amt, cb = 0.f;

			if(lastmillis-player1->lastregen < 500)
			{
				float regen = clamp((lastmillis-player1->lastregen)/500.f, 0.f, 1.f);
				pulse = clamp(pulse*regen, 0.3f, max(fade, 0.33f))*barblend();
				glow = clamp(glow*regen, 0.3f, 1.f);
			}

			glBindTexture(GL_TEXTURE_2D, t->retframe(player1->health, MAXHEALTH));
			glColor4f(clamp(cr, 0.5f, 1.f)*glow, clamp(cg, 0.f, 1.f)*glow, clamp(cb, 0.f, 1.f)*glow, pulse);
			if(t->frames.length() > 1) drawsized(float(bx), float(by), float(bs));
			else
			{
				float hs = float(bs)/2.0f, hx = float(bx)+hs, hy = float(by)+hs;
				drawslice(0, amt, hx, hy, hs);
			}

			if(showguns())
			{
				int ta = int(oy*ammosize()), tb = ta*3, tv = bx + bs - tb,
					to = ta/16, tr = ta/2, tq = tr - FONTH/2;
				const char *hudtexs[GUN_MAX] = {
					plasmahudtex(), shotgunhudtex(), chaingunhudtex(),
					flamerhudtex(), carbinehudtex(), riflehudtex(), grenadeshudtex(),
				};
				loopi(GUN_MAX) if(player1->hasgun(i) && (i == player1->gunselect || showguns() > 1))
				{
					float blend = fade * (i == player1->gunselect ? ammoblend() : ammoblendinactive());
					settexture(hudtexs[i], 0);
					glColor4f(1.f, 1.f, 1.f, blend);
					drawtex(float(tv), float(tp), float(tb), float(ta));
                    drawclip(i, tv+to/2, tp+to/2, ta-to, blend);
					if(i != player1->gunselect) pushfont("hud");
					int ts = tv + tr, tt = tp + tq;
					draw_textx("%s%d", ts, tt, 255, 255, 255, int(255.f*blend), false, AL_CENTER, -1, -1, player1->canshoot(i, lastmillis) ? "\fw" : "\fr", player1->ammo[i]);
					if(i != player1->gunselect) popfont();
					tp += ta;
				}
				tp += FONTH/2;
			}
			if(showtips())
			{
				tp = oy-FONTH;
				if(showtips() > 1)
				{
					if(player1->hasgun(player1->gunselect))
					{
						const char *a = retbindaction("zoom", keym::ACTION_DEFAULT, 0);
						s_sprintfd(actkey)("%s", a && *a ? a : "ZOOM");
						tp -= draw_textx("Press [ \fs\fg%s\fS ] to %s", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, actkey, player1->gunselect == GUN_RIFLE ? "zoom" : "prone");
					}
					if(player1->canshoot(player1->gunselect, lastmillis))
					{
						const char *a = retbindaction("attack", keym::ACTION_DEFAULT, 0);
						s_sprintfd(actkey)("%s", a && *a ? a : "ATTACK");
						tp -= draw_textx("Press [ \fs\fg%s\fS ] to attack", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, actkey);
					}

					if(player1->canreload(player1->gunselect, lastmillis))
					{
						const char *a = retbindaction("reload", keym::ACTION_DEFAULT, 0);
						s_sprintfd(actkey)("%s", a && *a ? a : "RELOAD");
						tp -= draw_textx("Press [ \fs\fg%s\fS ] to load ammo", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, actkey);
						if(ws.autoreload() > 1 && lastmillis-player1->gunlast[player1->gunselect] <= 1000)
							tp -= draw_textx("Autoreload in [ \fs\fg%.01f\fS ] second(s)", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, float(1000-(lastmillis-player1->gunlast[player1->gunselect]))/1000.f);
					}
				}

				vector<actitem> actitems;
				if(et.collateitems(player1, actitems))
				{
					bool found = false;
					while(!actitems.empty())
					{
						actitem &t = actitems.last();
						int ent = -1;
						switch(t.type)
						{
							case ITEM_ENT:
							{
								if(!et.ents.inrange(t.target)) break;
								ent = t.target;
								break;
							}
							case ITEM_PROJ:
							{
								if(!pj.projs.inrange(t.target)) break;
								projent &proj = *pj.projs[t.target];
								if(!et.ents.inrange(proj.id)) break;
								ent = proj.id;
								break;
							}
							default: break;
						}
						if(et.ents.inrange(ent))
						{
							const char *a = retbindaction("action", keym::ACTION_DEFAULT, 0);
							s_sprintfd(actkey)("%s", a && *a ? a : "ACTION");

							extentity &e = *et.ents[ent];
							if(enttype[e.type].usetype == EU_ITEM)
							{
								if(!found)
								{
									int drop = -1;
									if(e.type == WEAPON && guntype[player1->gunselect].carry &&
										player1->ammo[e.attr1] < 0 && guntype[e.attr1].carry &&
											player1->carry() >= MAXCARRY) drop = player1->drop(e.attr1);
									if(isgun(drop))
									{
										s_sprintfd(dropgun)("%s", et.entinfo(WEAPON, drop, player1->ammo[drop], 0, 0, 0, true));
										tp -= draw_textx("Press [ \fs\fg%s\fS ] to swap [ \fs%s\fS ] for [ \fs%s\fS ]", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, actkey, dropgun, et.entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, true));
									}
									else tp -= draw_textx("Press [ \fs\fg%s\fS ] to pickup [ \fs%s\fS ]", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, actkey, et.entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, true));
									if(showtips() < 3) break;
									else found = true;
								}
								else tp -= draw_textx("Nearby [ \fs%s\fS ]", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, et.entinfo(e.type, e.attr1, e.attr2, e.attr3, e.attr4, e.attr5, true));
							}
							else if(e.type == TRIGGER && e.attr3 == TA_ACT)
							{
								if(!found)
								{
									tp -= draw_textx("Press [ \fs\fg%s\fS ] to interact", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, actkey);
									if(showtips() < 3) break;
									else found = true;
								}
								else
									tp -= draw_textx("Nearby interactive item", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1);
							}
						}
						actitems.pop();
					}
				}
			}
		}
		else if(player1->state == CS_DEAD)
		{
			if(showtips())
			{
				tp = oy-FONTH;
				int wait = respawnwait(player1);
				if(wait)
					tp -= draw_textx("Fragged! Respawn available in [ \fs\fg%.01f\fS ] second(s)", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, float(wait)/1000.f);
				else
				{
					const char *a = retbindaction("attack", keym::ACTION_DEFAULT, 0);
					s_sprintfd(actkey)("%s", a && *a ? a : "ACTION");
					tp -= draw_textx("Fragged! Press [ \fs\fg%s\fS ] to respawn", bx+bs, tp, 255, 255, 255, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1, actkey);
				}
			}
		}
		else if(player1->state == CS_EDITING)
		{
			tp = oy-FONTH;
			if(showenttips()) loopi(clamp(entgroup.length()+1, 0, showhudents()))
			{
				int n = i ? entgroup[i-1] : enthover;
				if((!i || n != enthover) && et.ents.inrange(n))
				{
					gameentity &f = (gameentity &)*et.ents[n];
					if(showenttips() <= 2 && n != enthover) pushfont("hud");
					tp -= draw_textx("entity %d, %s", bx+bs, tp,
						n == enthover ? 255 : 196, 196, 196, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1,
							n, et.findname(f.type));
					if(showenttips() <= 2 && n != enthover) popfont();
					if(showenttips() > 1 || n == enthover)
					{
						tp -= draw_textx("%s (%d %d %d %d %d)", bx+bs, tp,
							255, 196, 196, int(255.f*fade*infoblend()), false, AL_RIGHT, -1, -1,
								et.entinfo(f.type, f.attr1, f.attr2, f.attr3, f.attr4, f.attr5, true),
									f.attr1, f.attr2, f.attr3, f.attr4, f.attr5);
					}
				}
			}
		}
		popfont(); // emphasis

		drawcardinalblips(bx+bo, by+bo, bs-(bo*2));
	}

	void drawhudelements(int w, int h)
	{
		int ox = w*3, oy = h*3, hoff = oy;
		glLoadIdentity();
		glOrtho(0, ox, oy, 0, -1, 1);

		renderconsole(ox, oy);

		static int laststats = 0, prevstats[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, curstats[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

		if(totalmillis-laststats >= statrate())
		{
			memcpy(prevstats, curstats, sizeof(prevstats));
			laststats = totalmillis-(totalmillis%statrate());
		}

		int nextstats[12] =
		{
			vtris*100/max(wtris, 1),
			vverts*100/max(wverts, 1),
			xtraverts/1024,
			xtravertsva/1024,
			glde,
			gbatches,
			getnumqueries(),
			rplanes,
			curfps,
			bestfpsdiff,
			worstfpsdiff,
			autoadjustlevel
		};

		loopi(12) if(prevstats[i] == curstats[i]) curstats[i] = nextstats[i];

		if(showstats())
		{
			hoff -= draw_textx("ond:%d va:%d gl:%d(%d) oq:%d lm:%d rp:%d pvs:%d", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, allocnodes*8, allocva, curstats[4], curstats[5], curstats[6], lightmaps.length(), curstats[7], getnumviewcells());
			hoff -= draw_textx("wtr:%dk(%d%%) wvt:%dk(%d%%) evt:%dk eva:%dk", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, wtris/1024, curstats[0], wverts/1024, curstats[1], curstats[2], curstats[3]);
		}

		if(showfps()) switch(showfps())
		{
			case 2:
				if(autoadjust) hoff -= draw_textx("fps:%d (%d/%d) +%d-%d [\fs%s%d%%\fS]", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, curstats[8], autoadjustfps, maxfps, curstats[9], curstats[10], curstats[11]<100?(curstats[11]<50?(curstats[11]<25?"\fr":"\fo"):"\fy"):"\fg", curstats[11]);
				else hoff -= draw_textx("fps:%d (%d) +%d-%d", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, curstats[8], maxfps, curstats[9], curstats[10]);
				break;
			case 1:
				if(autoadjust) hoff -= draw_textx("fps:%d (%d/%d) [\fs%s%d%%\fS]", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, curstats[8], autoadjustfps, maxfps, curstats[11]<100?(curstats[11]<50?(curstats[11]<25?"\fr":"\fo"):"\fy"):"\fg", curstats[11]);
				else hoff -= draw_textx("fps:%d (%d)", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1, curstats[8], maxfps);
				break;
			default: break;
		}

		if(getcurcommand())
			hoff -= rendercommand(FONTH/4, hoff-FONTH, w*3-FONTH);

		if(connected() && maptime)
		{
			if(player1->state == CS_EDITING)
			{
				hoff -= draw_textx("sel:%d,%d,%d %d,%d,%d (%d,%d,%d,%d)", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1,
						sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z,
							sel.cx, sel.cxs, sel.cy, sel.cys);
				hoff -= draw_textx("corner:%d orient:%d grid:%d", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1,
								sel.corner, sel.orient, sel.grid);
				hoff -= draw_textx("cube:%s%d ents:%d", FONTH/4, hoff-FONTH, 255, 255, 255, int(255*hudblend), false, AL_LEFT, -1, -1,
					selchildcount<0 ? "1/" : "", abs(selchildcount), entgroup.length());
			}

			render_texture_panel(w, h);
		}
	}

	void drawhud(int w, int h)
	{
		if(!hidehud)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			if(maptime && connected())
			{
				if(lastmillis-maptime < titlecardtime()+titlecardfade())
					drawtitlecard(w, h);
				else drawgamehud(w, h);
			}
			drawhudelements(w, h);
			glDisable(GL_BLEND);
		}
		g3d_render();
		drawpointers(w, h); // gets done last!
	}

	void lighteffects(dynent *e, vec &color, vec &dir)
	{
	}

    void particletrack(particle *p, uint type, int &ts, vec &o, vec &d, bool lastpass)
    {
        if(!p->owner || p->owner->type != ENT_PLAYER) return;

        switch(type&0xFF)
        {
        	case PT_PART: case PT_TAPE: case PT_FIREBALL: case PT_LIGHTNING: case PT_FLARE:
        	{
				o = ws.gunorigin(p->owner->o, d, (gameent *)p->owner, p->owner != player1 || isthirdperson());
				break;
        	}
        	default: break;
        }
    }

	void newmap(int size)
	{
		cc.addmsg(SV_NEWMAP, "ri", size);
	}

	void g3d_gamemenus() { sb.show(); }

	void loadworld(gzFile &f, int maptype)
	{
	}

	void saveworld(gzFile &f)
	{
	}

	bool gethudcolour(vec &colour)
	{
		if(!maptime || lastmillis-maptime < titlecardtime())
		{
			float fade = maptime ? float(lastmillis-maptime)/float(titlecardtime()) : 0.f;
			if(fade < 1.f)
			{
				colour = vec(fade, fade, fade);
				return true;
			}
		}
		if(tvmode())
		{
			float fade = 1.f;
			int millis = spectvtime() ? min(spectvtime()/10, 500) : 500, interval = lastmillis-lastspec;
			if(!lastspec || interval < millis)
				fade = lastspec ? float(interval)/float(millis) : 0.f;
			else if(spectvtime() && interval > spectvtime()-millis)
				fade = float(spectvtime()-interval)/float(millis);
			if(fade < 1.f)
			{
				colour = vec(fade, fade, fade);
				return true;
			}
		}
		return false;
	}

	void heightoffset(gameent *d, bool local)
	{
		d->o.z -= d->height;
		if(d->state == CS_ALIVE)
		{
			if(ph.iscrouching(d))
			{
				float crouchoff = 1.f-CROUCHHEIGHT;
				if(d->type == ENT_PLAYER)
				{
					float amt = clamp(float(lastmillis-d->crouchtime)/200.f, 0.f, 1.f);
					if(!d->crouching) amt = 1.f-amt;
					crouchoff *= amt;
				}
				d->height = PLAYERHEIGHT-(PLAYERHEIGHT*crouchoff);
			}
			else d->height = PLAYERHEIGHT;
		}
		else if(d->state == CS_DEAD)
		{
			if(d->obliterated) d->height = PLAYERHEIGHT;
			else
			{
				int t = lastmillis-d->lastpain;
				if(t < 0) d->height = PLAYERHEIGHT;
				float amt = t > 1000 ? 0.9f : clamp(float(lastmillis-d->crouchtime)/1000.f, 0.f, 0.9f);
				d->height = PLAYERHEIGHT-(PLAYERHEIGHT*amt);
			}
		}
		else d->height = PLAYERHEIGHT;
		d->o.z += d->height;
	}

	vec headpos(physent *d, float off = 0.f)
	{
		vec pos(d->o);
		pos.z -= off;
		return pos;
	}

	vec feetpos(physent *d, float off = 0.f)
	{
 		vec pos(d->o);
		if(d->type == ENT_PLAYER) pos.z += off-d->height;
		return pos;
	}

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
		curfov = float(fov());

		if(inzoom())
		{
			int frame = lastmillis-lastzoom,
				t = player1->gunselect == GUN_RIFLE ? snipetime() : pronetime(),
				f = player1->gunselect == GUN_RIFLE ? snipefov() : pronefov();
			float diff = float(fov()-f),
				amt = frame < t ? clamp(float(frame)/float(t), 0.f, 1.f) : 1.f;
			if(!zooming) amt = 1.f-amt;
			curfov -= amt*diff;
		}

        aspect = w/float(h);
        fovy = 2*atan2(tan(curfov/2*RAD), aspect)/RAD;
	}

	bool mousemove(int dx, int dy, int x, int y, int w, int h)
	{
		bool hit = g3d_active();

		#define mousesens(a,b,c) ((float(a)/float(b))*c)

		if(hit || mousestyle() >= 1)
		{
			if(absmouse()) // absolute positions, unaccelerated
			{
				cursorx = clamp(float(x)/float(w), 0.f, 1.f);
				cursory = clamp(float(y)/float(h), 0.f, 1.f);
				return false;
			}
			else
			{
				cursorx = clamp(cursorx+mousesens(dx, w, mousesensitivity()), 0.f, 1.f);
				cursory = clamp(cursory+mousesens(dy, h, mousesensitivity()*(!hit && invmouse() ? -1.f : 1.f)), 0.f, 1.f);
				return true;
			}
		}
		else
		{
			if(allowmove(player1))
			{
				float scale = inzoom() ?
						(player1->gunselect == GUN_RIFLE ? snipesensitivity() : pronesensitivity())
					: sensitivity();
				player1->yaw += mousesens(dx, w, yawsensitivity()*scale);
				player1->pitch -= mousesens(dy, h, pitchsensitivity()*scale*(!hit && invmouse() ? -1.f : 1.f));
				fixfullrange(player1->yaw, player1->pitch, player1->roll, false);
			}
			return true;
		}
		return false;
	}

	void project(int w, int h)
	{
		int style = g3d_active() ? -1 : mousestyle();
		if(style != lastmousetype)
		{
			resetcursor();
			lastmousetype = style;
		}
		if(!g3d_active())
		{
			int aim = isthirdperson() ? thirdpersonaim() : firstpersonaim();
			if(aim)
			{
				float ax, ay, az;
				vectocursor(worldpos, ax, ay, az);
				float amt = float(curtime)/float(aim),
					  offx = ax-aimx, offy = ay-aimy;
				aimx += offx*amt;
				aimy += offy*amt;
			}
			else
			{
				aimx = cursorx;
				aimy = cursory;
			}
			float vx = mousestyle() <= 1 ? aimx : cursorx,
				vy = mousestyle() <= 1 ? aimy : cursory;
			vecfromcursor(vx, vy, 1.f, cursordir);
		}
	}

	void scaleyawpitch(float &yaw, float &pitch, float newyaw, float newpitch, int frame, int speed)
	{
		if(speed && frame)
		{
			float amt = float(lastmillis-frame)/float(speed),
				offyaw = newyaw-yaw, offpitch = newpitch-pitch;
			if(offyaw > 180.f) offyaw -= 360.f;
			if(offyaw < -180.f) offyaw += 360.f;
			yaw += offyaw*amt;
			pitch += offpitch*amt;
		}
		else
		{
			yaw = newyaw;
			pitch = newpitch;
		}
		fixrange(yaw, pitch);
	}

	struct camstate
	{
		int ent, idx;
		vec pos, dir;
		vector<int> cansee;
		float mindist, maxdist, score;

		camstate() : idx(-1), mindist(32.f), maxdist(512.f) { reset(); }
		~camstate() {}

		void reset()
		{
			cansee.setsize(0);
			dir = vec(0, 0, 0);
			score = 0.f;
		}
	};
	vector<camstate> cameras;

	static int camerasort(const camstate *a, const camstate *b)
	{
		int asee = a->cansee.length(), bsee = b->cansee.length();
		if(asee > bsee) return -1;
		if(asee < bsee) return 1;
		if(a->score < b->score) return -1;
		if(a->score > b->score) return 1;
		return 0;
	}

	void recomputecamera(int w, int h)
	{
		fixview(w, h);

		camera1 = &camera;

		if(camera1->type != ENT_CAMERA)
		{
			camera1->reset();
			camera1->type = ENT_CAMERA;
			camera1->state = CS_ALIVE;
			camera1->height = camera1->radius = camera1->xradius = camera1->yradius = 1;
		}

		if(connected() && maptime)
		{
			if(!lastcamera)
			{
				resetcursor();
				cameras.setsize(0);
				lastspec = 0;
				if(mousestyle() == 2)
				{
					camera1->yaw = player1->aimyaw = player1->yaw;
					camera1->pitch = player1->aimpitch = player1->pitch;
				}
			}
			if(tvmode())
			{
				if(cameras.empty()) loopk(2)
				{
					physent d = *camera1;
					loopv(et.ents) if(et.ents[i]->type == (k ? LIGHT : CAMERA))
					{
						d.o = et.ents[i]->o;
						if(ph.entinmap(&d, false))
						{
							camstate &c = cameras.add();
							c.pos = et.ents[i]->o;
							c.ent = i;
							if(!k)
							{
								c.idx = et.ents[i]->attr1;
								if(et.ents[i]->attr2) c.mindist = et.ents[i]->attr2;
								if(et.ents[i]->attr3) c.maxdist = et.ents[i]->attr3;
							}
						}
					}
					lastspec = 0;
					if(!cameras.empty()) break;
				}
				if(!cameras.empty())
				{
					bool renew = !lastspec || (spectvtime() && lastmillis-lastspec >= spectvtime());
					loopvj(cameras)
					{
						camstate &c = cameras[j];
						loopk(2)
						{
							vec avg(0, 0, 0);
							gameent *d;
							c.reset();
							loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && (d->state == CS_ALIVE || d->state == CS_DEAD))
							{
								vec trg, pos = feetpos(d);
								float dist = c.pos.dist(pos);
								if((k || dist >= c.mindist) && dist <= c.maxdist && raycubelos(c.pos, pos, trg))
								{
									c.cansee.add(i);
									avg.add(pos);
								}
							}
							float yaw = camera1->yaw, pitch = camera1->pitch;
							#define updatecamorient \
							{ \
								if((k || j) && c.cansee.length()) \
								{ \
									vec dir = vec(avg).div(c.cansee.length()).sub(c.pos).normalize(); \
									vectoyawpitch(dir, yaw, pitch); \
								} \
							}
							updatecamorient;
							loopvrev(c.cansee) if((d = (gameent *)iterdynents(c.cansee[i])))
							{
								vec trg, pos = feetpos(d);
								if(getsight(c.pos, yaw, pitch, pos, trg, c.maxdist, curfov, fovy))
								{
									c.dir.add(pos);
									c.score += c.pos.dist(pos);
								}
								else
								{
									avg.sub(pos);
									c.cansee.removeunordered(i);
									updatecamorient;
								}
							}
							if(!j || !c.cansee.empty()) break;
						}
						if(!c.cansee.empty())
						{
							float amt = float(c.cansee.length());
							c.dir.div(amt);
							c.score /= amt;
						}
						else if(!j) renew = true; // quick scotty, get a new cam
						if(!renew) break; // only update first camera then
					}
					camstate *cam = &cameras[0], *oldcam = cam;
					if(renew)
					{
						cameras.sort(camerasort);
						lastspec = lastmillis;
						cam = &cameras[0];
					}
					player1->o = camera1->o = cam->pos;
					vec dir = vec(cam->dir).sub(camera1->o).normalize();
					vectoyawpitch(dir, camera1->yaw, camera1->pitch);
					if(cam == oldcam)
					{
						float amt = float(curtime)/1000.f,
							offyaw = fabs(camera1->yaw-camera1->aimyaw)*amt, offpitch = fabs(camera1->pitch-camera1->aimpitch)*amt*0.25f;

						if(camera1->yaw > camera1->aimyaw)
						{
							camera1->aimyaw += offyaw;
							if(camera1->yaw < camera1->aimyaw) camera1->aimyaw = camera1->yaw;
						}
						else if(camera1->yaw < camera1->aimyaw)
						{
							camera1->aimyaw -= offyaw;
							if(camera1->yaw > camera1->aimyaw) camera1->aimyaw = camera1->yaw;
						}

						if(camera1->pitch > camera1->aimpitch)
						{
							camera1->aimpitch += offpitch;
							if(camera1->pitch < camera1->aimpitch) camera1->aimpitch = camera1->pitch;
						}
						else if(camera1->pitch < camera1->aimpitch)
						{
							camera1->aimpitch -= offpitch;
							if(camera1->pitch > camera1->aimpitch) camera1->aimpitch = camera1->pitch;
						}
						camera1->yaw = camera1->aimyaw;
						camera1->pitch = camera1->aimpitch;
					}
					player1->yaw = player1->aimyaw = camera1->aimyaw = camera1->yaw;
					player1->pitch = player1->aimpitch = camera1->aimpitch = camera1->pitch;
				}
			}
			else
			{
				vec pos = headpos(player1, 0.f);

				if(mousestyle() <= 1)
					findorientation(pos, player1->yaw, player1->pitch, worldpos);

				camera1->o = pos;

				if(isthirdperson())
				{
					float angle = thirdpersonangle() ? 0-thirdpersonangle() : player1->pitch;
					camera1->aimyaw = mousestyle() <= 1 ? player1->yaw : player1->aimyaw;
					camera1->aimpitch = mousestyle() <= 1 ? angle : player1->aimpitch;

					#define cameramove(d,s) \
						if(d) \
						{ \
							camera1->move = !s ? (d > 0 ? -1 : 1) : 0; \
							camera1->strafe = s ? (d > 0 ? -1 : 1) : 0; \
							loopi(10) if(!ph.moveplayer(camera1, 10, true, abs(d))) break; \
						}
					cameramove(thirdpersondist(), false);
					cameramove(thirdpersonshift(), true);
				}
				else
				{
					camera1->aimyaw = mousestyle() <= 1 ? player1->yaw : player1->aimyaw;
					camera1->aimpitch = mousestyle() <= 1 ? player1->pitch : player1->aimpitch;
				}

				switch(mousestyle())
				{
					case 0:
					case 1:
					{
						if(!thirdpersonaim() && isthirdperson())
						{
							vec dir(worldpos);
							dir.sub(camera1->o);
							dir.normalize();
							vectoyawpitch(dir, camera1->yaw, camera1->pitch);
							fixfullrange(camera1->yaw, camera1->pitch, camera1->roll, false);
						}
						else
						{
							camera1->yaw = player1->yaw;
							camera1->pitch = player1->pitch;
						}
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
						findorientation(isthirdperson() ? camera1->o : pos, yaw, pitch, worldpos);
						if(allowmove(player1))
						{
							if(isthirdperson())
							{
								vec dir(worldpos);
								dir.sub(camera1->o);
								dir.normalize();
								vectoyawpitch(dir, player1->yaw, player1->pitch);
							}
							else
							{
								player1->yaw = yaw;
								player1->pitch = pitch;
							}
						}
						break;
					}
				}

				fixfullrange(camera1->yaw, camera1->pitch, camera1->roll, false);
				fixrange(camera1->aimyaw, camera1->aimpitch);

				if(allowmove(player1))
				{
					if(isthirdperson())
					{
						vec dir(worldpos);
						dir.sub(pos);
						dir.normalize();
						vectoyawpitch(dir, player1->aimyaw, player1->aimpitch);
					}
					else
					{
						player1->aimyaw = camera1->yaw;
						player1->aimpitch = camera1->pitch;
					}
					fixrange(player1->aimyaw, player1->aimpitch);

					if(lastcamera && mousestyle() >= 1 && !g3d_active())
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

			if(quakewobble > 0)
				camera1->roll = float(rnd(21)-10)*(float(min(quakewobble, 100))/100.f);
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

	void adddynlights()
	{
		pj.adddynlights();
		et.adddynlights();
	}

	vector<gameent *> bestplayers;
    vector<int> bestteams;

	IVAR(animoverride, -1, 0, ANIM_MAX-1);
	IVAR(testanims, 0, 0, 1);

	int numanims() { return ANIM_MAX; }

	void findanims(const char *pattern, vector<int> &anims)
	{
		loopi(sizeof(animnames)/sizeof(animnames[0]))
			if(*animnames[i] && matchanim(animnames[i], pattern))
				anims.add(i);
	}

	void renderclient(gameent *d, bool third, bool trans, int team, modelattach *attachments, bool secondary, int animflags, int animdelay, int lastaction, float speed, bool early)
	{
		string mdl;
		if(third) s_strcpy(mdl, teamtype[team].tpmdl);
		else s_strcpy(mdl, teamtype[team].fpmdl);

		float yaw = d->yaw, pitch = d->pitch, roll = d->roll;
		vec o = vec(third ? feetpos(d) : headpos(d));
		if(!third)
		{
			vec dir;
			if(firstpersonsway() != 0.f)
			{
				vecfromyawpitch(d->yaw, d->pitch, 1, 0, dir);
				float swayspeed = min(4.f, sqrtf(d->vel.x*d->vel.x + d->vel.y*d->vel.y));
				dir.mul(swayspeed);
				float swayxy = sinf(swaymillis/115.0f)/float(firstpersonsway()),
					  swayz = cosf(swaymillis/115.0f)/float(firstpersonsway());
				swap(dir.x, dir.y);
				dir.x *= -swayxy;
				dir.y *= swayxy;
				dir.z = -fabs(swayspeed*swayz);
				dir.add(swaydir);
				o.add(dir);
			}
			if(firstpersondist() != 0.f)
			{
				vecfromyawpitch(yaw, pitch, 1, 0, dir);
				dir.mul(player1->radius*firstpersondist());
				o.add(dir);
			}
			if(firstpersonshift() != 0.f)
			{
				vecfromyawpitch(yaw, pitch, 0, -1, dir);
				dir.mul(player1->radius*firstpersonshift());
				o.add(dir);
			}
			if(firstpersonadjust() != 0.f)
			{
				vecfromyawpitch(yaw, pitch+90.f, 1, 0, dir);
				dir.mul(player1->height*firstpersonadjust());
				o.add(dir);
			}
		}

		int anim = animflags, basetime = lastaction;
		if(animoverride())
		{
			anim = (animoverride()<0 ? ANIM_ALL : animoverride())|ANIM_LOOP;
			basetime = 0;
		}
		else
		{
			if(secondary)
			{
				if(d->inliquid && d->physstate <= PHYS_FALL)
					anim |= (((allowmove(d) && (d->move || d->strafe)) || d->vel.z+d->falling.z>0 ? ANIM_SWIM : ANIM_SINK)|ANIM_LOOP)<<ANIM_SECONDARY;
				else if(d->timeinair > 1000)
					anim |= (ANIM_JUMP|ANIM_END)<<ANIM_SECONDARY;
				else if(d->crouching)
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
				case ANIM_IDLE: case ANIM_PLASMA: case ANIM_SHOTGUN: case ANIM_CHAINGUN:
				case ANIM_GRENADES: case ANIM_FLAMER: case ANIM_CARBINE: case ANIM_RIFLE:
				{
					anim >>= ANIM_SECONDARY;
					break;
				}
				default: break;
			}
		}

		if(!((anim>>ANIM_SECONDARY)&ANIM_INDEX)) switch(anim&ANIM_INDEX)
		{
			case ANIM_IDLE: case ANIM_PLASMA: case ANIM_SHOTGUN: case ANIM_CHAINGUN:
			case ANIM_GRENADES: case ANIM_FLAMER: case ANIM_CARBINE: case ANIM_RIFLE:
			{
				anim |= ((anim&ANIM_INDEX)|ANIM_LOOP)<<ANIM_SECONDARY;
				break;
			}
			default:
			{
				anim |= (ANIM_IDLE|ANIM_LOOP)<<ANIM_SECONDARY;
				break;
			}
		}

		int flags = MDL_LIGHT;
		if(d != player1) flags |= MDL_CULL_VFC | MDL_CULL_OCCLUDED | MDL_CULL_QUERY;
		if(d->type != ENT_PLAYER) flags |= MDL_CULL_DIST;
        if(early) flags |= MDL_NORENDER;
		else if(trans) flags |= MDL_TRANSLUCENT;
		else if(third && (anim&ANIM_INDEX)!=ANIM_DEAD) flags |= MDL_DYNSHADOW;
		dynent *e = third ? (dynent *)d : (dynent *)&fpsmodel;
		rendermodel(NULL, mdl, anim, o, !third && testanims() && d == player1 ? 0 : yaw+90, pitch, roll, flags, e, attachments, basetime, speed);
	}

	void renderplayer(gameent *d, bool third, bool trans, bool early = false)
	{
        modelattach a[4];
		int ai = 0, team = m_team(gamemode, mutators) ? d->team : TEAM_NEUTRAL,
			gun = d->gunselect, lastaction = lastmillis,
			animflags = ANIM_IDLE|ANIM_LOOP, animdelay = 0;
		bool secondary = false, showgun = isgun(gun);

		if(d->state == CS_SPECTATOR || d->state == CS_WAITING) return;
		else if(d->state == CS_DEAD)
		{
			if(d->obliterated) return; // not shown at all
			showgun = false;
			animflags = ANIM_DYING;
			lastaction = d->lastpain;
			int t = lastmillis-lastaction;
			if(t < 0) return;
			if(t > 1000) animflags = ANIM_DEAD|ANIM_LOOP;
        }
		else if(d->state == CS_EDITING)
		{
			animflags = ANIM_EDIT|ANIM_LOOP;
		}
		else if(d->state == CS_LAGGED)
		{
			animflags = ANIM_LAG|ANIM_LOOP;
		}
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
		else if(d->lasttaunt && lastmillis-d->lasttaunt <= 1000)
		{
			lastaction = d->lasttaunt;
			animflags = ANIM_TAUNT;
			animdelay = 1000;
		}
		else if(lastmillis-d->lastpain <= 300)
		{
			secondary = third && allowmove(d);
			lastaction = d->lastpain;
			animflags = ANIM_PAIN;
			animdelay = 300;
		}
		else
		{
			secondary = third && allowmove(d);
			if(showgun)
			{
				int gunstate = GNS_IDLE;
				if(lastmillis-d->gunlast[gun] <= d->gunwait[gun])
				{
					gunstate = d->gunstate[gun];
					lastaction = d->gunlast[gun];
					animdelay = d->gunwait[gun];
				}
				switch(gunstate)
				{
					case GNS_SWITCH:
					{
						if(lastmillis-d->gunlast[gun] <= d->gunwait[gun]/2)
						{
							if(d->hasgun(d->lastgun)) gun = d->lastgun;
							else showgun = false;
						}
						animflags = ANIM_SWITCH;
						break;
					}
					case GNS_POWER:
					{
						if(!guntype[gun].power) gunstate = GNS_SHOOT;
						animflags = (guntype[gun].anim+gunstate);
						break;
					}
					case GNS_SHOOT:
					{
						if(guntype[gun].power) showgun = false;
						animflags = (guntype[gun].anim+gunstate);
						break;
					}
					case GNS_RELOAD:
					{
						if(guntype[gun].power) showgun = false;
						animflags = (guntype[gun].anim+gunstate);
						break;
					}
					case GNS_IDLE:	default:
					{
						if(!d->hasgun(gun)) showgun = false;
						else animflags = (guntype[gun].anim+gunstate)|ANIM_LOOP;
						break;
					}
				}
			}
		}

		if(shownamesabovehead() && third && d != player1)
		{
			s_sprintfd(s)("@%s", colorname(d));
			part_text(d->abovehead(), s);
		}


		if(showgun)
		{ // we could probably animate the vwep too now..
			a[ai].name = guntype[gun].vwep;
			a[ai].tag = "tag_weapon";
			a[ai].anim = ANIM_VWEP|ANIM_LOOP;
			a[ai].basetime = 0;
			ai++;
		}
		if(third)
		{
			if(m_ctf(gamemode))
			{
				loopv(ctf.flags) if(ctf.flags[i].owner == d && !ctf.flags[i].droptime)
				{
					a[ai].name = teamtype[ctf.flags[i].team].flag;
					a[ai].tag = "tag_flag";
					a[ai].anim = ANIM_MAPMODEL|ANIM_LOOP;
					a[ai].basetime = 0;
					ai++;
				}
			}
		}

        if(rendernormally && (early || d != player1))
        {
            d->muzzle = vec(-1, -1, -1);
            a[ai].tag = "tag_muzzle";
            a[ai].pos = &d->muzzle;
            ai++;
        }

        renderclient(d, third, trans, team, a[0].name ? a : NULL, secondary, animflags, animdelay, lastaction, 0.f, early);
	}

	void render()
	{
		if(intermission)
		{
			if(m_team(gamemode, mutators)) { bestteams.setsize(0); sb.bestteams(bestteams); }
			else { bestplayers.setsize(0); sb.bestplayers(bestplayers); }
		}

		startmodelbatches();

		gameent *d;
        loopi(numdynents()) if((d = (gameent *)iterdynents(i)) && d != player1)
        {
			if(d->state!=CS_SPECTATOR && d->state!=CS_WAITING && d->state!=CS_SPAWNING && (d->state!=CS_DEAD || !d->obliterated))
				renderplayer(d, true, d->state == CS_LAGGED || (d->state == CS_ALIVE && lastmillis-d->lastspawn <= REGENWAIT));
        }

		et.render();
		pj.render();
		if(m_stf(gamemode)) stf.render();
        else if(m_ctf(gamemode)) ctf.render();
        ai.render();

		endmodelbatches();
	}

    void renderavatar(bool early)
    {
        if(inzoomswitch() && player1->gunselect == GUN_RIFLE) return;
        if(isthirdperson() || !rendernormally)
        {
            if(player1->state!=CS_SPECTATOR && player1->state!=CS_WAITING && (player1->state!=CS_DEAD || !player1->obliterated))
                renderplayer(player1, true, (player1->state == CS_ALIVE && lastmillis-player1->lastspawn <= REGENWAIT) || thirdpersontranslucent(), early);
        }
        else if(player1->state == CS_ALIVE)
        {
            renderplayer(player1, false, (lastmillis-player1->lastspawn <= REGENWAIT) || firstpersontranslucent(), early);
        }
    }
};
REGISTERGAME(GAMEID, new gameclient(), new gameserver());
#else
REGISTERGAME(GAMEID, NULL, new gameserver());
#endif
