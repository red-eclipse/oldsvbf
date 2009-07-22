#include "game.h"
namespace stf
{
	stfstate st;

    bool insideflag(const stfstate::flag &b, gameent *d)
    {
        return st.insideflag(b, d->feetpos());
    }

    void preload()
    {
        loopi(TEAM_MAX) loadmodel(teamtype[i].flag, -1, true);
    }

	void render()
	{
		loopv(st.flags)
		{
			stfstate::flag &b = st.flags[i];
            if(!b.ent) continue;
			const char *flagname = teamtype[b.owner].flag;
            rendermodel(&b.ent->light, flagname, ANIM_MAPMODEL|ANIM_LOOP, b.o, b.ent->attr[2], b.ent->attr[3], 0, MDL_SHADOW|MDL_CULL_VFC|MDL_CULL_OCCLUDED);
			int attack = b.enemy ? b.enemy : b.owner, defend = b.owner ? b.owner : b.enemy;
			if(b.enemy && b.owner)
				formatstring(b.info)("\fs%s%s\fS vs. \fs%s%s\fS", teamtype[b.owner].chat, teamtype[b.owner].name, teamtype[b.enemy].chat, teamtype[b.enemy].name);
			else formatstring(b.info)("\fs%s%s\fS", teamtype[defend].chat, teamtype[defend].name);
			float occupy = attack ? (!b.owner || b.enemy ? clamp(b.converted/float((b.owner?2:1) * st.OCCUPYLIMIT), 0.f, 1.f) : 1.f) : 0.f;
			vec above = b.o;
			above.z += enttype[FLAG].radius*2/3;
			part_text(above, b.info);
			above.z += 2.5f;
			if(occupy > 0 && occupy < 1)
			{
				part_icon(above, textureload("textures/progress", 3), 1, 4, 0, 0, 1, teamtype[attack].colour, 0, occupy);
				part_icon(above, textureload("textures/progress", 3), 1, 4, 0, 0, 1, teamtype[b.owner ? b.owner : TEAM_NEUTRAL].colour, occupy, 1-occupy);
			}
			else part_icon(above, textureload("textures/progress", 3), 1, 4, 0, 0, 1, teamtype[b.owner ? b.owner : TEAM_NEUTRAL].colour);
			defformatstring(str)("@%d%%", int(occupy*100.f)); part_text(above, str);
		}
	}


    void adddynlights()
    {
        loopv(st.flags)
        {
            stfstate::flag &f = st.flags[i];
            if(!f.ent) continue;
			adddynlight(vec(f.o).add(vec(0, 0, enttype[FLAG].radius)), enttype[FLAG].radius*1.5f,
				vec((teamtype[f.owner].colour>>16), ((teamtype[f.owner].colour>>8)&0xFF), (teamtype[f.owner].colour&0xFF)).div(255.f));
        }
    }

	void drawblips(int w, int h, float blend)
	{
		loopv(st.flags)
		{
			stfstate::flag &f = st.flags[i];
			vec dir(f.o); dir.sub(camera1->o);
			int colour = teamtype[f.enemy && lastmillis%600 >= 400 ? f.enemy : f.owner].colour;
			float r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f, fade = blend*hud::radarflagblend;
			if(f.owner != game::player1->team && f.enemy != game::player1->team)
			{
				float dist = dir.magnitude(),
					diff = dist <= hud::radarrange() ? clamp(1.f-(dist/hud::radarrange()), 0.f, 1.f) : 0.f;
				fade *= diff*0.5f;
			}
			dir.rotate_around_z(-camera1->yaw*RAD); dir.normalize();
			if(hud::radarflagnames)
			{
				float occupy = !f.owner || f.enemy ? clamp(f.converted/float((f.owner?2:1) * st.OCCUPYLIMIT), 0.f, 1.f) : 1.f;
				if(occupy < 1.f)
					hud::drawblip(hud::flagtex, 3, w, h, hud::radarflagsize, fade, dir, r, g, b, "radar", "%s%d%%", teamtype[f.owner].chat, int(occupy*100.f));
				else hud::drawblip(hud::flagtex, 3, w, h, hud::radarflagsize, fade, dir, r, g, b, "radar", "%s%s", teamtype[f.owner].chat, teamtype[f.owner].name);
			}
			else hud::drawblip(hud::flagtex, 3, w, h, hud::radarflagsize, fade, dir, r, g, b);
		}
	}

	void drawlast(int w, int h, int &tx, int &ty, float blend)
	{
		if(game::player1->state == CS_ALIVE)
		{
			loopv(st.flags) if(insideflag(st.flags[i], game::player1) && (st.flags[i].owner == game::player1->team || st.flags[i].enemy == game::player1->team))
			{
				stfstate::flag &f = st.flags[i];
				pushfont("super");
				float occupy = !f.owner || f.enemy ? clamp(f.converted/float((f.owner?2:1) * st.OCCUPYLIMIT), 0.f, 1.f) : 1.f;
				bool overthrow = f.owner && f.enemy == game::player1->team;
				ty += draw_textx("%s \fs%s%d%%\fS complete", tx, ty, 255, 255, 255, int(255*blend), TEXT_CENTERED, -1, -1, overthrow ? "Overthrow" : "Secure", overthrow ? "\fo" : (occupy < 1.f ? "\fy" : "\fg"), int(occupy*100.f));
				popfont();
				break;
			}
		}
	}

    int drawinventory(int x, int y, int s, float blend)
    {
		int sy = 0;
		loopv(st.flags)
		{
			stfstate::flag &f = st.flags[i];
			bool hasflag = game::player1->state == CS_ALIVE && insideflag(f, game::player1);
			if(f.hasflag != hasflag) { f.hasflag = hasflag; f.lasthad = lastmillis-max(1000-(lastmillis-f.lasthad), 0); }
			int millis = lastmillis-f.lasthad;
			if(game::player1->state == CS_SPECTATOR || hud::inventorygame >= 2 || f.hasflag || millis < 1000)
			{
				int prevsy = sy, colour = teamtype[f.owner].colour;
				float skew = game::player1->state == CS_SPECTATOR || hud::inventorygame >= 2 ? hud::inventoryskew : 0.f, fade = blend*hud::inventoryblend,
					occupy = f.enemy ? clamp(f.converted/float((f.owner ? 2 : 1)*st.OCCUPYLIMIT), 0.f, 1.f) : (f.owner ? 1.f : 0.f),
					r = (colour>>16)/255.f, g = ((colour>>8)&0xFF)/255.f, b = (colour&0xFF)/255.f;
				if(f.hasflag) skew += (millis < 1000 ? clamp(float(millis)/1000.f, 0.f, 1.f)*(1.f-skew) : 1.f-skew);
				else if(millis < 1000) skew += (1.f-skew)-(clamp(float(millis)/1000.f, 0.f, 1.f)*(1.f-skew));
				sy += hud::drawitem(hud::flagtex, x, y-sy, s, false, r, g, b, fade, skew, "default", "%s%d%%", hasflag ? (f.owner && f.enemy == game::player1->team ? "\fo" : (occupy < 1.f ? "\fy" : "\fg")) : "\fw", int(occupy*100.f));
				if(f.enemy) hud::drawitem(hud::teamtex(f.enemy), x, y-prevsy, int(s*0.5f), false, 1.f, 1.f, 1.f, fade, skew);
			}
		}
        return sy;
    }

	void setupflags()
	{
		st.reset();
		loopv(entities::ents)
		{
			extentity *e = entities::ents[i];
			if(e->type != FLAG || !chkflagmode(game::gamemode, game::mutators, e->attr[3])) continue;
			stfstate::flag &b = st.flags.add();
			b.o = e->o;
			defformatstring(alias)("flag_%d", e->attr[0]);
			const char *name = getalias(alias);
			if(name[0]) copystring(b.name, name);
			else formatstring(b.name)("flag %d", st.flags.length());
			b.ent = e;
		}
	}

	void sendflags(ucharbuf &p)
	{
		putint(p, SV_FLAGS);
		putint(p, st.flags.length());
		loopv(st.flags)
		{
			stfstate::flag &b = st.flags[i];
			putint(p, int(b.o.x*DMF));
			putint(p, int(b.o.y*DMF));
			putint(p, int(b.o.z*DMF));
		}
	}

	void updateflag(int i, int owner, int enemy, int converted)
	{
		if(!st.flags.inrange(i)) return;
		stfstate::flag &b = st.flags[i];
		if(owner)
		{
			if(b.owner != owner)
			{
				int idx = !game::announcefilter || game::player1->state == CS_SPECTATOR || owner == game::player1->team || enemy == game::player1->team || b.owner == game::player1->team || b.enemy == game::player1->team ? S_V_FLAGSECURED : -1;
				game::announce(idx, CON_INFO, "\foteam \fs%s%s\fS secured %s", teamtype[owner].chat, teamtype[owner].name, b.name);
				game::spawneffect(PART_FIREBALL, vec(b.o).add(vec(0, 0, enttype[FLAG].radius/2)), teamtype[owner].colour, enttype[FLAG].radius);
			}
		}
		else if(b.owner)
		{
			int idx = !game::announcefilter || game::player1->state == CS_SPECTATOR || owner == game::player1->team || enemy == game::player1->team || b.owner == game::player1->team || b.enemy == game::player1->team ? S_V_FLAGOVERTHROWN : -1;
			game::announce(idx, CON_INFO, "\foteam \fs%s%s\fS overthrew %s", teamtype[enemy].chat, teamtype[enemy].name, b.name);
			game::spawneffect(PART_FIREBALL, vec(b.o).add(vec(0, 0, enttype[FLAG].radius/2)), teamtype[enemy].colour, enttype[FLAG].radius);
		}
		b.owner = owner;
		b.enemy = enemy;
		b.converted = converted;
	}

	void setscore(int team, int total)
	{
		st.findscore(team).total = total;
	}

	bool aicheck(gameent *d, ai::aistate &b)
	{
		return false;
	}

	void aifind(gameent *d, ai::aistate &b, vector<ai::interest> &interests)
	{
		vec pos = d->feetpos();
		loopvj(st.flags)
		{
			stfstate::flag &f = st.flags[j];
			static vector<int> targets; // build a list of others who are interested in this
			targets.setsizenodelete(0);
			ai::checkothers(targets, d, ai::AI_S_DEFEND, ai::AI_T_AFFINITY, j, true);
			gameent *e = NULL;
			bool regen = !m_regen(game::gamemode, game::mutators) || !overctfhealth || d->health >= overctfhealth;
			loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && ai::targetable(d, e, false) && !e->ai && d->team == e->team)
			{ // try to guess what non ai are doing
				vec ep = e->feetpos();
				if(targets.find(e->clientnum) < 0 && ep.squaredist(f.o) <= (enttype[FLAG].radius*enttype[FLAG].radius))
					targets.add(e->clientnum);
			}
			if((!regen && f.owner == d->team) || (targets.empty() && (f.owner != d->team || f.enemy)))
			{
				ai::interest &n = interests.add();
				n.state = ai::AI_S_DEFEND;
				n.node = entities::closestent(WAYPOINT, f.o, ai::NEARDIST, false);
				n.target = j;
				n.targtype = ai::AI_T_AFFINITY;
				n.score = pos.squaredist(f.o)/(!regen ? 100.f : 1.f);
			}
		}
	}

	bool aidefend(gameent *d, ai::aistate &b)
	{
		if(st.flags.inrange(b.target))
		{
			stfstate::flag &f = st.flags[b.target];
			bool regen = !m_regen(game::gamemode, game::mutators) || !overctfhealth || d->health >= overctfhealth;
			int walk = 0;
			if(regen && !f.enemy && f.owner == d->team)
			{
				static vector<int> targets; // build a list of others who are interested in this
				targets.setsizenodelete(0);
				ai::checkothers(targets, d, ai::AI_S_DEFEND, ai::AI_T_AFFINITY, b.target, true);
				gameent *e = NULL;
				loopi(game::numdynents()) if((e = (gameent *)game::iterdynents(i)) && ai::targetable(d, e, false) && !e->ai && d->team == e->team)
				{ // try to guess what non ai are doing
					vec ep = e->feetpos();
					if(targets.find(e->clientnum) < 0 && (ep.squaredist(f.o) <= (enttype[FLAG].radius*enttype[FLAG].radius*4)))
						targets.add(e->clientnum);
				}
				if(!targets.empty())
				{
					if(lastmillis-b.millis >= m_speedtime((201-d->skill)*33))
					{
						d->ai->trywipe = true; // re-evaluate so as not to herd
						return true;
					}
					else walk = 2;
				}
				else walk = 1;
			}
			return ai::defend(d, b, f.o, float(enttype[FLAG].radius), float(enttype[FLAG].radius*(2+(walk*2))), walk);
		}
		return false;
	}

	bool aipursue(gameent *d, ai::aistate &b)
	{
		b.type = ai::AI_S_DEFEND;
		return aidefend(d, b);
	}

    void removeplayer(gameent *d)
    {
    }
}
