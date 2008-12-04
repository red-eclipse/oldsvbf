#include "pch.h"
#include "engine.h"
#include "game.h"
namespace client
{
	bool c2sinit = false, sendinfo = false, isready = false, remote = false,
		demoplayback = false, needsmap = false;
	int lastping = 0;

	VARP(centerchat, 0, 1, 1);
	VARP(colourchat, 0, 1, 1);

	SVARP(serversort, "");

	ICOMMAND(mastermode, "i", (int *val), if(remote) addmsg(SV_MASTERMODE, "ri", *val));

	ICOMMAND(getname, "", (), result(world::player1->name));
	ICOMMAND(getteam, "", (), result(teamtype[world::player1->team].name));

	void switchname(const char *name)
	{
		if(name[0])
		{
			c2sinit = false;
			s_strncpy(world::player1->name, name, MAXNAMELEN);
		}
		else conoutf("\fmyour name is: %s", world::colorname(world::player1));
	}
	ICOMMAND(name, "s", (char *s), switchname(s));

	int teamname(const char *team)
	{
		if(m_team(world::gamemode, world::mutators))
		{
			if(team[0])
			{
				int t = atoi(team);

				loopi(numteams(world::gamemode, world::mutators))
				{
					if((t && t == i+TEAM_ALPHA) || !strcasecmp(teamtype[i+TEAM_ALPHA].name, team))
					{
						return i+TEAM_ALPHA;
					}
				}
			}
			return TEAM_ALPHA;
		}
		return TEAM_NEUTRAL;
	}

	void switchteam(const char *team)
	{
		if(m_team(world::gamemode, world::mutators))
		{
			if(team[0])
			{
				int t = teamname(team);
				if(t != world::player1->team)
				{
					c2sinit = false;
					world::player1->team = t;
				}
			}
			else conoutf("\fmyour team is: %s", teamtype[world::player1->team].name);
		}
		else conoutf("\frcan only change teams in team games");
	}
	ICOMMAND(team, "s", (char *s), switchteam(s));

	int numchannels() { return 3; }

	void mapstart() { sendinfo = !needsmap; }

	void writeclientinfo(FILE *f)
	{
		fprintf(f, "name \"%s\"\n\n", world::player1->name);
	}

	void gameconnect(bool _remote)
	{
		remote = _remote;
		if(editmode) toggleedit();
	}

	void gamedisconnect(int clean)
	{
		if(editmode) toggleedit();
		needsmap = remote = isready = c2sinit = false;
		projs::remove(world::player1);
        removetrackedparticles(world::player1);
		removetrackedsounds(world::player1);
		world::player1->clientnum = -1;
		world::player1->lifesequence = 0;
		world::player1->privilege = PRIV_NONE;
		world::player1->state = CS_DEAD;
		loopv(world::players) if(world::players[i]) world::clientdisconnected(i);
		enumerate(*idents, ident, id, {
			if(id.flags&IDF_CLIENT) // reset vars
			{
				switch(id.type)
				{
					case ID_VAR:
					{
						setvar(id.name, id.def.i, true);
						break;
					}
					case ID_FVAR:
					{
						setfvar(id.name, id.def.f, true);
						break;
					}
					case ID_SVAR:
					{
						setsvar(id.name, *id.def.s ? id.def.s : "", true);
						break;
					}
					case ID_ALIAS:
					{
						worldalias(id.name, "");
						break;
					}
					default: break;
				}
			}
		});
	}

	bool allowedittoggle(bool edit)
	{
		bool allow = edit || m_edit(world::gamemode);
		if(!allow) conoutf("\fryou must be both alive and in coopedit to enter editmode");
		return allow;
	}

	void edittoggled(bool edit)
	{
		if(!edit)
		{
			world::player1->state = CS_ALIVE;
			world::player1->o.z -= world::player1->height; // entinmap wants feet pos
		}
		else
		{
			world::player1->state = CS_EDITING;
			world::resetstates(ST_DEFAULT);
		}
		physics::entinmap(world::player1, false); // find spawn closest to current floating pos
		if(m_edit(world::gamemode))
			addmsg(SV_EDITMODE, "ri", edit ? 1 : 0);
	}

	int parseplayer(const char *arg)
	{
		char *end;
		int n = strtol(arg, &end, 10);
		if(!world::players.inrange(n)) return -1;
		if(*arg && !*end) return n;
		// try case sensitive first
		loopi(world::numdynents())
		{
			gameent *o = (gameent *)world::iterdynents(i);
			if(o && !strcmp(arg, o->name)) return o->clientnum;
		}
		// nothing found, try case insensitive
		loopi(world::numdynents())
		{
			gameent *o = (gameent *)world::iterdynents(i);
			if(o && !strcasecmp(arg, o->name)) return o->clientnum;
		}
		return -1;
	}

	void clearbans()
	{
        if(!remote) return;
		addmsg(SV_CLEARBANS, "r");
	}
	ICOMMAND(clearbans, "", (char *s), clearbans());

	void kick(const char *arg)
	{
		if(!remote) return;
		int i = parseplayer(arg);
		if(i>=0 && i!=world::player1->clientnum) addmsg(SV_KICK, "ri", i);
	}
	ICOMMAND(kick, "s", (char *s), kick(s));

	void setteam(const char *arg1, const char *arg2)
	{
		if(!remote) return;
		if(m_team(world::gamemode, world::mutators))
		{
			int i = parseplayer(arg1);
			if(i>=0 && i!=world::player1->clientnum)
			{
				int t = teamname(arg2);
				if(t) addmsg(SV_SETTEAM, "ri2", i, t);
			}
		}
		else conoutf("\frcan only change teams in team games");
	}
	ICOMMAND(setteam, "ss", (char *who, char *team), setteam(who, team));

	void setmaster(const char *arg)
	{
		if(!remote || !arg[0]) return;
		int val = 1;
		const char *passwd = "";
		if(!arg[1] && isdigit(arg[0])) val = atoi(arg);
		else passwd = arg;
		addmsg(SV_SETMASTER, "ris", val, passwd);
	}
	ICOMMAND(setmaster, "s", (char *s), setmaster(s));

    void approvemaster(const char *who)
    {
        if(!remote) return;
        int i = parseplayer(who);
        if(i>=0) addmsg(SV_APPROVEMASTER, "ri", i);
    }
	ICOMMAND(approve, "s", (char *s), approvemaster(s));

    void togglespectator(int val, const char *who)
	{
        int i = who[0] ? parseplayer(who) : world::player1->clientnum;
		if(i>=0) addmsg(SV_SPECTATOR, "rii", i, val);
	}
	ICOMMAND(spectator, "is", (int *val, char *who), togglespectator(*val, who));

	// collect c2s messages conveniently
	vector<uchar> messages;

	void addmsg(int type, const char *fmt, ...)
	{
		/*
		if(remote && spectator && (!world::player1->privilege || type<SV_MASTERMODE))
		{
			static int spectypes[] = { SV_MAPVOTE, SV_GETMAP, SV_TEXT, SV_SETMASTER };
			bool allowed = false;
			loopi(sizeof(spectypes)/sizeof(spectypes[0])) if(type==spectypes[i])
			{
				allowed = true;
				break;
			}
			if(!allowed) return;
		}
		*/
		static uchar buf[MAXTRANS];
		ucharbuf p(buf, MAXTRANS);
		putint(p, type);
		int numi = 1, nums = 0;
		bool reliable = false;
		if(fmt)
		{
			va_list args;
			va_start(args, fmt);
			while(*fmt) switch(*fmt++)
			{
				case 'r': reliable = true; break;
				case 'v':
				{
					int n = va_arg(args, int);
					int *v = va_arg(args, int *);
					loopi(n) putint(p, v[i]);
					numi += n;
					break;
				}
				case 'i':
				{
					int n = isdigit(*fmt) ? *fmt++-'0' : 1;
					loopi(n) putint(p, va_arg(args, int));
					numi += n;
					break;
				}
                case 'f':
                {
                    int n = isdigit(*fmt) ? *fmt++-'0' : 1;
                    loopi(n) putfloat(p, (float)va_arg(args, double));
                    numi += n;
                    break;
                }
				case 's': sendstring(va_arg(args, const char *), p); nums++; break;
			}
			va_end(args);
		}
		int num = nums?0:numi, msgsize = msgsizelookup(type);
        if(msgsize && num!=msgsize) { s_sprintfd(s)("inconsistent msg size for %d (%d != %d)", type, num, msgsize); fatal(s); }
		int len = p.length();
		messages.add(len&0xFF);
		messages.add((len>>8)|(reliable ? 0x80 : 0));
		loopi(len) messages.add(buf[i]);
	}

	void saytext(gameent *d, int flags, char *text)
	{
		if(!colourchat) filtertext(text, text);

		string s, t, m;
		s_sprintf(t)("%s", m_team(world::gamemode, world::mutators) && (flags & SAY_TEAM) ? teamtype[d->team].chat : teamtype[TEAM_NEUTRAL].chat);
		s_sprintf(m)("%s", world::colorname(d, NULL, "", m_team(world::gamemode, world::mutators) && (flags & SAY_TEAM) ? false : true));

		if(flags&SAY_ACTION) s_sprintf(s)("%s* \fs%s\fS \fs%s\fS", t, m, text);
		else s_sprintf(s)("%s<\fs\fw%s\fS> \fs\fw%s\fS", t, m, text);

		if(d->state != CS_DEAD && d->state != CS_SPECTATOR && d->state != CS_WAITING)
		{
			s_sprintfd(ds)("@%s", &s);
			part_text(d->abovehead(), ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 3.f);
		}

		console("%s", (centerchat ? CON_CENTER : 0)|CON_NORMAL, s);
		playsound(S_CHAT, camera1->o, camera1, SND_FORCED);
	}

	void toserver(int flags, char *text)
	{
		saytext(world::player1, flags, text);
		addmsg(SV_TEXT, "ri2s", world::player1->clientnum, flags, text);
	}
	ICOMMAND(say, "C", (char *s), toserver(SAY_NONE, s));
	ICOMMAND(me, "C", (char *s), toserver(SAY_ACTION, s));
	ICOMMAND(sayteam, "C", (char *s), toserver(SAY_TEAM, s));
	ICOMMAND(meteam, "C", (char *s), toserver(SAY_ACTION|SAY_TEAM, s));

	bool sendcmd(int nargs, char *cmd, char *arg)
	{
		if(isready && nargs > 1)
		{
			addmsg(SV_COMMAND, "ri2ss", world::player1->clientnum, nargs, cmd, arg);
			return true;
		}
		return false;
	}


	void changemapserv(char *name, int gamemode, int mutators)
	{
		if(editmode) toggleedit();
		world::gamemode = gamemode; world::mutators = mutators;
		server::modecheck(&world::gamemode, &world::mutators);
		world::nextmode = world::gamemode; world::nextmuts = world::mutators;
		world::minremain = -1;
		if(editmode && !allowedittoggle(editmode)) toggleedit();
		if(m_demo(gamemode)) return;
		if(!name || !load_world(name))
		{
			emptymap(0, true, NULL);
			needsmap = true;
		}
		if(world::player1->state != CS_SPECTATOR) world::player1->state = CS_DEAD;
		if(editmode) edittoggled(editmode);
		if(m_stf(gamemode)) stf::setupflags();
        else if(m_ctf(gamemode)) ctf::setupflags();
	}

	void receivefile(uchar *data, int len)
	{
		ucharbuf p(data, len);
		int type = getint(p);
		data += p.length();
		len -= p.length();
		switch(type)
		{
			case SV_SENDDEMO:
			{
				s_sprintfd(fname)("%d.dmo", lastmillis);
				FILE *demo = openfile(fname, "wb");
				if(!demo) return;
				conoutf("\fmreceived demo \"%s\"", fname);
				fwrite(data, 1, len, demo);
				fclose(demo);
				break;
			}

			case SV_SENDMAP:
			{
				const char *mapname = getmapname();
				if(!mapname || !*mapname) mapname = "maps/untitled";
				const char *file = findfile(mapname, "wb");
				FILE *map = fopen(file, "wb");
				if(!map) return;
				conoutf("\fmreceived map");
				fwrite(data, 1, len, map);
				fclose(map);
				if(!load_world(mapname)) emptymap(0, true, NULL);
				else needsmap = false;
				break;
			}
		}
	}

	void getmap()
	{
		conoutf("\fmgetting map...");
		addmsg(SV_GETMAP, "r");
	}
	ICOMMAND(getmap, "", (), getmap());

	void stopdemo()
	{
		if(remote) addmsg(SV_STOPDEMO, "r");
		else server::stopdemo();
	}
	ICOMMAND(stopdemo, "", (), stopdemo());

    void recorddemo(int val)
	{
        addmsg(SV_RECORDDEMO, "ri", val);
	}
	ICOMMAND(recorddemo, "i", (int *val), recorddemo(*val));

	void cleardemos(int val)
	{
        addmsg(SV_CLEARDEMOS, "ri", val);
	}
	ICOMMAND(cleardemos, "i", (int *val), cleardemos(*val));

    void getdemo(int i)
	{
		if(i<=0) conoutf("\fmgetting demo...");
		else conoutf("\fmgetting demo %d...", i);
		addmsg(SV_GETDEMO, "ri", i);
	}
	ICOMMAND(getdemo, "i", (int *val), getdemo(*val));

	void listdemos()
	{
		conoutf("\fmlisting demos...");
		addmsg(SV_LISTDEMOS, "r");
	}
	ICOMMAND(listdemos, "", (), listdemos());

	void changemap(const char *name) // request map change, server may ignore
	{
        int nextmode = world::nextmode, nextmuts = world::nextmuts; // in case stopdemo clobbers these
        if(!remote) stopdemo();
        addmsg(SV_MAPVOTE, "rsi2", name, nextmode, nextmuts);
	}
	ICOMMAND(map, "s", (char *s), changemap(s));

	void sendmap()
	{
		conoutf("\fmsending map...");
		const char *mapname = getmapname();
		if(!mapname || !*mapname) mapname = "maps/untitled";
		save_world(mapname, m_edit(world::gamemode));
		FILE *map = openfile(mapname, "rb");
		if(map)
		{
			fseek(map, 0, SEEK_END);
			sendfile(-1, 2, map);
			fclose(map);
		}
		else conoutf("\frcould not read map: %s", mapname);
	}
	ICOMMAND(sendmap, "", (), sendmap());

	void parsecommand(gameent *d, char *cmd, char *arg)
	{
		ident *id = idents->access(cmd);
		if(id && id->flags&IDF_CLIENT)
		{
			string val;
			val[0] = 0;
			switch(id->type)
			{
				case ID_COMMAND:
				{
					string s;
					s_sprintf(s)("%s %s", cmd, arg);
					char *ret = executeret(s);
					if(ret)
					{
						conoutf("\fm%s: %s", cmd, ret);
						delete[] ret;
					}
					return;
				}
				case ID_VAR:
				{
					int ret = atoi(arg);
					*id->storage.i = ret;
					id->changed();
					s_sprintf(val)("%d", *id->storage.i);
					break;
				}
				case ID_FVAR:
				{
					float ret = atof(arg);
					*id->storage.f = ret;
					id->changed();
					s_sprintf(val)("%f", *id->storage.f);
					break;
				}
				case ID_SVAR:
				{
					delete[] *id->storage.s;
					*id->storage.s = newstring(arg);
					id->changed();
					s_sprintf(val)("%s", *id->storage.s);
					break;
				}
				default: return;
			}
			if(d || verbose)
				conoutf("\fm%s set %s to %s", d ? world::colorname(d) : "the server", cmd, val);
		}
		else conoutf("\fr%s sent unknown command: %s", d ? world::colorname(d) : "the server", cmd);
	}

	void gotoplayer(const char *arg)
	{
		if(world::player1->state!=CS_SPECTATOR && world::player1->state!=CS_WAITING && world::player1->state!=CS_EDITING) return;
		int i = parseplayer(arg);
		if(i>=0 && i!=world::player1->clientnum)
		{
			gameent *d = world::getclient(i);
			if(!d) return;
			world::player1->o = d->o;
			vec dir;
			vecfromyawpitch(world::player1->yaw, world::player1->pitch, 1, 0, dir);
			world::player1->o.add(dir.mul(-32));
            world::player1->resetinterp();
		}
	}
	ICOMMAND(goto, "s", (char *s), gotoplayer(s));

	bool ready() { return isready; }
	ICOMMAND(ready, "", (), intret(ready()));

	int state() { return world::player1->state; }
	int otherclients()
	{
		int n = 0; // ai don't count
		loopv(world::players) if(world::players[i] && world::players[i]->aitype == AI_NONE) n++;
		return n;
	}

	void editvar(ident *id, bool local)
	{
        if(id && id->flags&IDF_WORLD && local && m_edit(world::gamemode))
        {
        	switch(id->type)
        	{
        		case ID_VAR:
					addmsg(SV_EDITVAR, "risi", id->type, id->name, *id->storage.i);
					break;
        		case ID_FVAR:
					addmsg(SV_EDITVAR, "risf", id->type, id->name, *id->storage.f);
					break;
        		case ID_SVAR:
					addmsg(SV_EDITVAR, "riss", id->type, id->name, *id->storage.s);
					break;
        		case ID_ALIAS:
					addmsg(SV_EDITVAR, "riss", id->type, id->name, id->action);
					break;
				default: break;
        	}
        }
	}

	void edittrigger(const selinfo &sel, int op, int arg1, int arg2, int arg3)
	{
        if(m_edit(world::gamemode)) switch(op)
		{
			case EDIT_FLIP:
			case EDIT_COPY:
			case EDIT_PASTE:
			case EDIT_DELCUBE:
			{
				addmsg(SV_EDITF + op, "ri9i4",
					sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z, sel.grid, sel.orient,
					sel.cx, sel.cxs, sel.cy, sel.cys, sel.corner);
				break;
			}
			case EDIT_MAT:
			case EDIT_ROTATE:
			{
				addmsg(SV_EDITF + op, "ri9i5",
					sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z, sel.grid, sel.orient,
					sel.cx, sel.cxs, sel.cy, sel.cys, sel.corner,
					arg1);
				break;
			}
			case EDIT_FACE:
			case EDIT_TEX:
			case EDIT_REPLACE:
			{
				addmsg(SV_EDITF + op, "ri9i6",
					sel.o.x, sel.o.y, sel.o.z, sel.s.x, sel.s.y, sel.s.z, sel.grid, sel.orient,
					sel.cx, sel.cxs, sel.cy, sel.cys, sel.corner,
					arg1, arg2);
				break;
			}
            case EDIT_REMIP:
            {
                addmsg(SV_EDITF + op, "r");
                break;
            }
		}
	}

	void updateposition(gameent *d)
	{
        if(d->state==CS_ALIVE || d->state==CS_EDITING)
		{
			// send position updates separately so as to not stall out aiming
			ENetPacket *packet = enet_packet_create(NULL, 100, 0);
			ucharbuf q(packet->data, packet->dataLength);
			putint(q, SV_POS);
			putint(q, d->clientnum);
			putuint(q, (int)(d->o.x*DMF));			  // quantize coordinates to 1/4th of a cube, between 1 and 3 bytes
			putuint(q, (int)(d->o.y*DMF));
			putuint(q, (int)((d->o.z - d->height)*DMF));
			putuint(q, (int)d->yaw);
			putint(q, (int)d->pitch);
			putint(q, (int)d->roll);
			putint(q, (int)(d->vel.x*DVELF));		  // quantize to itself, almost always 1 byte
			putint(q, (int)(d->vel.y*DVELF));
			putint(q, (int)(d->vel.z*DVELF));
            putuint(q, d->physstate | (d->falling.x || d->falling.y ? 0x20 : 0) | (d->falling.z ? 0x10 : 0) | ((((gameent *)d)->lifesequence&1)<<6));
            if(d->falling.x || d->falling.y)
            {
                putint(q, (int)(d->falling.x*DVELF));      // quantize to itself, almost always 1 byte
                putint(q, (int)(d->falling.y*DVELF));
            }
            if(d->falling.z) putint(q, (int)(d->falling.z*DVELF));
			// pack rest in almost always 1 byte: strafe:2, move:2, crouching: 1, aimyaw/aimpitch: 1
			uint flags = (d->strafe&3) | ((d->move&3)<<2) | ((d->crouching ? 1 : 0)<<4) | ((int)d->aimyaw!=(int)d->yaw || (int)d->aimpitch!=(int)d->pitch ? 0x20 : 0);
			putuint(q, flags);
            if(flags&0x20)
            {
                putuint(q, (int)d->aimyaw);
                putint(q, (int)d->aimpitch);
            }
			enet_packet_resize(packet, q.length());
			sendpackettoserv(packet, 0);
		}
	}

	int sendpacketclient(ucharbuf &p, bool &reliable)
	{
		updateposition(world::player1);
		loopv(world::players)
			if(world::players[i] && world::players[i]->ai)
				updateposition(world::players[i]);

		if(sendinfo)
		{
			reliable = true;
			putint(p, SV_GAMEINFO);
			putint(p, getmapversion());
			putint(p, getmaprevision());
			entities::putitems(p);
			putint(p, -1);
			if(m_stf(world::gamemode)) stf::sendflags(p);
            else if(m_ctf(world::gamemode)) ctf::sendflags(p);
			sendinfo = false;
		}
		if(!c2sinit)	// tell other clients who I am
		{
			reliable = true;
			c2sinit = true;
			putint(p, SV_INITC2S);
			sendstring(world::player1->name, p);
			putint(p, world::player1->team);
			loopv(world::players) if(world::players[i] && world::players[i]->ai)
			{
				gameent *f = world::players[i];
				putint(p, SV_INITAI);
				putint(p, f->aitype);
				putint(p, f->ownernum);
				putint(p, f->skill);
				putint(p, f->clientnum);
				sendstring(f->name, p);
				putint(p, f->team);
			}
		}
		int i = 0;
		while(i < messages.length()) // send messages collected during the previous frames
		{
			int len = messages[i] | ((messages[i+1]&0x7F)<<8);
			if(p.remaining() < len) break;
			if(messages[i+1]&0x80) reliable = true;
			p.put(&messages[i+2], len);
			i += 2 + len;
		}
		messages.remove(0, i);
		if(p.remaining()>=10 && lastmillis-lastping>250)
		{
			putint(p, SV_PING);
			putint(p, lastmillis);
			lastping = lastmillis;
		}
		return 1;
	}

    void parsestate(gameent *d, ucharbuf &p, bool resume = false)
    {
        if(!d) { static gameent dummy; d = &dummy; }
		if(d==world::player1) getint(p);
		else d->state = getint(p);
		d->frags = getint(p);
        d->lifesequence = getint(p);
        d->health = getint(p);
        if(resume && d==world::player1)
        {
            getint(p);
            loopi(GUN_MAX) getint(p);
        }
        else
        {
            d->gunselect = getint(p);
            loopi(GUN_MAX) d->ammo[i] = getint(p);
        }
    }

	void updatepos(gameent *d)
	{
		// update the position of other clients in the game in our world
		// don't care if he's in the scenery or other players,
		// just don't overlap with our client

		const float r = world::player1->radius+d->radius;
		const float dx = world::player1->o.x-d->o.x;
		const float dy = world::player1->o.y-d->o.y;
		const float dz = world::player1->o.z-d->o.z;
		const float rz = world::player1->aboveeye+world::player1->height;
		const float fx = (float)fabs(dx), fy = (float)fabs(dy), fz = (float)fabs(dz);
		if(fx<r && fy<r && fz<rz && world::player1->state!=CS_SPECTATOR && world::player1->state!=CS_WAITING && d->state!=CS_DEAD)
		{
			if(fx<fy) d->o.y += dy<0 ? r-fy : -(r-fy);  // push aside
			else	  d->o.x += dx<0 ? r-fx : -(r-fx);
		}
		int lagtime = lastmillis-d->lastupdate;
		if(lagtime)
		{
            if(d->state!=CS_SPAWNING && d->lastupdate) d->plag = (d->plag*5+lagtime)/6;
			d->lastupdate = lastmillis;
		}
	}

	void parsepositions(ucharbuf &p)
	{
		int type;
		while(p.remaining()) switch(type = getint(p))
		{
			case SV_POS:						// position of another client
			{
				int lcn = getint(p);
				vec o, vel, falling;
				float yaw, pitch, roll, aimyaw, aimpitch;
				int physstate, f;
				o.x = getuint(p)/DMF;
				o.y = getuint(p)/DMF;
				o.z = getuint(p)/DMF;
				aimyaw = yaw = (float)getuint(p);
				aimpitch = pitch = (float)getint(p);
				roll = (float)getint(p);
				vel.x = getint(p)/DVELF;
				vel.y = getint(p)/DVELF;
				vel.z = getint(p)/DVELF;
				physstate = getuint(p);
                falling = vec(0, 0, 0);
                if(physstate&0x20)
                {
                    falling.x = getint(p)/DVELF;
                    falling.y = getint(p)/DVELF;
                }
                if(physstate&0x10) falling.z = getint(p)/DVELF;
                int seqcolor = (physstate>>6)&1;
				f = getuint(p);
                if(f&0x20)
                {
                    aimyaw = (float)getuint(p);
                    aimpitch = (float)getint(p);
                }
				gameent *d = world::getclient(lcn);
                if(!d || seqcolor!=(d->lifesequence&1) || d==world::player1 || d->ai) continue;
                float oldyaw = d->yaw, oldpitch = d->pitch, oldaimyaw = d->aimyaw, oldaimpitch = d->aimpitch;
				d->yaw = yaw;
				d->pitch = pitch;
				d->roll = roll;
				d->aimyaw = aimyaw;
				d->aimpitch = aimpitch;
				d->strafe = (f&3)==3 ? -1 : f&3;
				f >>= 2;
				d->move = (f&3)==3 ? -1 : f&3;
				f >>= 2;
				bool crouch = d->crouching;
				d->crouching = f&1 ? true : false;
				if(crouch != d->crouching) d->crouchtime = lastmillis;
                vec oldpos(d->o);
                if(world::allowmove(d))
                {
                    d->o = o;
                    d->o.z += d->height;
                    d->vel = vel;
                    d->falling = falling;
                    d->physstate = physstate & 0x0F;
                    physics::updatephysstate(d);
                    updatepos(d);
                }
                if(d->state==CS_DEAD)
                {
                    d->resetinterp();
                    d->smoothmillis = 0;
                }
                else if(physics::smoothmove && d->smoothmillis>=0 && oldpos.dist(d->o) < physics::smoothdist)
                {
                    d->newpos = d->o;
                    d->newpos.z -= d->height;
                    d->newyaw = d->yaw;
                    d->newpitch = d->pitch;
                    d->newaimyaw = d->aimyaw;
                    d->newaimpitch = d->aimpitch;

                    d->o = oldpos;
                    d->yaw = oldyaw;
                    d->pitch = oldpitch;
                    d->aimyaw = oldaimyaw;
                    d->aimpitch = oldaimpitch;

                    oldpos.z -= d->height;
                    (d->deltapos = oldpos).sub(d->newpos);

                    d->deltayaw = oldyaw - d->newyaw;
                    if(d->deltayaw > 180) d->deltayaw -= 360;
                    else if(d->deltayaw < -180) d->deltayaw += 360;
                    d->deltapitch = oldpitch - d->newpitch;

                    d->deltaaimyaw = oldaimyaw - d->newaimyaw;
                    if(d->deltaaimyaw > 180) d->deltaaimyaw -= 360;
                    else if(d->deltaaimyaw < -180) d->deltaaimyaw += 360;
                    d->deltaaimpitch = oldaimpitch - d->newaimpitch;

                    d->smoothmillis = lastmillis;
                }
                else d->smoothmillis = 0;
                if(d->state==CS_LAGGED || d->state==CS_SPAWNING) d->state = CS_ALIVE;
				break;
			}

			default:
				neterr("type");
				return;
		}
	}

	void parsemessages(int cn, gameent *d, ucharbuf &p)
	{
		static char text[MAXTRANS];
		int type = -1, prevtype = -1;
		bool mapchanged = false;

		while(p.remaining())
		{
			prevtype = type;
			type = getint(p);
			//conoutf("\fo[client] msg: %d, prev: %d", type, prevtype);
			switch(type)
			{
				case SV_INITS2C:					// welcome messsage from the server
				{
					int mycn = getint(p), gver = getint(p);
					if(gver!=GAMEVERSION)
					{
						conoutf("\fryou are using a different game version (you: %d, server: %d)", GAMEVERSION, gver);
						disconnect();
						return;
					}
					world::player1->clientnum = mycn;	  // we are now fully ready
					isready = true;
					break;
				}

				case SV_CLIENT:
				{
					int lcn = getint(p), len = getuint(p);
					ucharbuf q = p.subbuf(len);
					parsemessages(lcn, world::getclient(lcn), q);
					break;
				}

				case SV_ANNOUNCE:
				{
					int snd = getint(p);
					getstring(text, p);
					entities::announce(snd, text, false);
					break;
				}

				case SV_SOUND:
				{
					int tcn = getint(p), snd = getint(p);
					if(snd < 0 || snd >= S_MAX) break;
					gameent *t = world::getclient(tcn);
					if(!t) break;
					playsound(snd, t->o, t, SND_FORCED);
					break;
				}

				case SV_TEXT:
				{
					int tcn = getint(p);
					gameent *t = world::getclient(tcn);
					int flags = getint(p);
					getstring(text, p);
					if(!t) break;
					saytext(t, flags, text);
					break;
				}

				case SV_COMMAND:
				{
					int lcn = getint(p);
					gameent *f = world::getclient(lcn);
					string cmd;
					getstring(cmd, p);
					getstring(text, p);
					parsecommand(f ? f : NULL, cmd, text);
					break;
				}

				case SV_EXECLINK:
				{
					int tcn = getint(p), index = getint(p);
					gameent *t = world::getclient(tcn);
					if(!t || !d || (t->clientnum != d->clientnum && t->ownernum != d->clientnum)) break;
					entities::execlink(t, index, false);
					break;
				}

				case SV_MAPCHANGE:
				{
					int hasmap = getint(p);
					if(hasmap) getstring(text, p);
					int mode = getint(p), muts = getint(p);
					changemapserv(hasmap ? text : NULL, mode, muts);
					mapchanged = true;
					break;
				}

				case SV_FORCEDEATH:
				{
					int lcn = getint(p);
					gameent *f = world::newclient(lcn);
					if(!f) break;
					if(f==world::player1)
					{
						if(editmode) toggleedit();
						hud::sb.showscores(true);
					}
                    else f->resetinterp();
					f->state = CS_DEAD;
					break;
				}

				case SV_GAMEINFO:
				{
					int ver = getint(p), rev = getint(p), n;
					while((n = getint(p))!=-1) entities::setspawn(n, getint(p));
					if(!needsmap && ver >= 0 && rev >= 0 && (ver != getmapversion() || rev != getmaprevision()))
						getmap();
					break;
				}

				case SV_NEWGAME: // server requests next game
				{
					hud::sb.showscores(false);
					if(!guiactive()) showgui("game");
					break;
				}

				case SV_INITC2S: // another client either connected or changed name/team
				{
					d = world::newclient(cn);
					if(!d)
					{
						getstring(text, p);
						getint(p);
						break;
					}
					getstring(text, p);
					if(!text[0]) s_strcpy(text, "unnamed");
					if(d->name[0])		  // already connected
					{
						if(strcmp(d->name, text))
						{
							string oldname, newname;
							s_strcpy(oldname, world::colorname(d, NULL, "", false));
							s_strcpy(newname, world::colorname(d, text));
							conoutf("\fm%s is now known as %s", oldname, newname);
						}
					}
					else					// new client
					{
						c2sinit = false;	// send new players my info again
						conoutf("\fg%s has joined the game", world::colorname(d, text, "", false));
						loopv(world::players)	// clear copies since new player doesn't have them
							if(world::players[i]) freeeditinfo(world::players[i]->edit);
						freeeditinfo(localedit);
					}
					s_strncpy(d->name, text, MAXNAMELEN);
					d->team = getint(p);
					break;
				}

				case SV_CDIS:
					world::clientdisconnected(getint(p));
					break;

				case SV_SPAWN:
				{
					int lcn = getint(p);
					gameent *f = world::newclient(lcn);
					f->respawn(lastmillis);
					parsestate(f, p);
					f->state = CS_SPAWNING;
					playsound(S_RESPAWN, f->o, f);
					world::spawneffect(vec(f->o).sub(vec(0, 0, f->height/2.f)), teamtype[f->team].colour, int(f->height/2.f));
					break;
				}

				case SV_SPAWNSTATE:
				{
					int lcn = getint(p);
					gameent *f = world::newclient(lcn);
					if(f == world::player1 && editmode) toggleedit();
					f->respawn(lastmillis);
					parsestate(f, p);
					f->state = CS_ALIVE;
					if(f == world::player1 || f->ai)
					{
						entities::findplayerspawn(f, m_stf(world::gamemode) ? stf::pickspawn(f->team) : -1, m_team(world::gamemode, world::mutators) ? f->team : -1);
						addmsg(SV_SPAWN, "ri3", f->clientnum, f->lifesequence, f->gunselect);
						playsound(S_RESPAWN, f->o, f);
						world::spawneffect(vec(f->o).sub(vec(0, 0, f->height/2.f)), teamtype[f->team].colour, int(f->height/2.f));
					}
					ai::spawned(f);
					if(f == world::player1) world::resetstates(ST_DEFAULT);
					break;
				}

				case SV_SHOTFX:
				{
					int scn = getint(p), gun = getint(p), power = clamp(getint(p), 0, 200);
					vec from;
					vector<vec> locs;
					locs.setsize(0);
					loopk(3) from[k] = getint(p)/DMF;
					int ls = getint(p);
					if(ls) loopj(ls)
					{
						vec to = locs.add();
						loopk(3) to[k] = getint(p)/DMF;
					}
					gameent *s = world::getclient(scn);
					if(!s || !isgun(gun)) break;
					s->setgunstate(gun, GNS_SHOOT, guntype[gun].adelay, lastmillis);
					projs::shootv(gun, power, from, locs, s, false);
					break;
				}

				case SV_DAMAGE:
				{
					int tcn = getint(p),
						acn = getint(p),
						gun = getint(p),
						flags = getint(p),
						damage = getint(p),
						health = getint(p);
					vec dir;
					loopk(3) dir[k] = getint(p)/DNF;
					dir.normalize();
					gameent *target = world::getclient(tcn), *actor = world::getclient(acn);
					if(!target || !actor) break;
					world::damaged(gun, flags, damage, health, target, actor, lastmillis, dir);
					break;
				}

				case SV_RELOAD:
				{
					int trg = getint(p), gun = getint(p), amt = getint(p);
					gameent *target = world::getclient(trg);
					if(!target || !isgun(gun)) break;
					target->setgunstate(gun, GNS_RELOAD, guntype[gun].rdelay, lastmillis);
					target->ammo[gun] = amt;
					target->reqreload = -1;
					playsound(S_RELOAD, target->o, target);
					break;
				}

				case SV_REGEN:
				{
					int trg = getint(p), amt = getint(p);
					gameent *target = world::getclient(trg);
					if(!target) break;
					target->health = amt;
					target->lastregen = lastmillis;
					playsound(S_REGEN, target->o, target); // maybe only player1?
					break;
				}

				case SV_DIED:
				{
					int vcn = getint(p), acn = getint(p), frags = getint(p), spree = getint(p),
						gun = getint(p), flags = getint(p), damage = getint(p);
					gameent *victim = world::getclient(vcn), *actor = world::getclient(acn);
					if(!actor) break;
					actor->frags = frags;
					actor->spree = spree;
					if(!victim) break;
					world::killed(gun, flags, damage, victim, actor);
					victim->lastdeath = lastmillis;
					victim->gunreset(true);
					break;
				}

				case SV_DROP:
				{
					int trg = getint(p), ds = getint(p);
					gameent *target = world::getclient(trg);
					bool local = target && (target == world::player1 || target->ai);
					if(ds) loopj(ds)
					{
						int gs = getint(p), drop = getint(p);
						if(target) projs::drop(target, gs, drop, local);
					}
					break;
				}

				case SV_GUNSELECT:
				{
					int trg = getint(p), gun = getint(p);
					gameent *target = world::getclient(trg);
					if(!target || !isgun(gun)) break;
					target->gunswitch(gun, lastmillis);
					target->reqswitch = -1;
					playsound(S_SWITCH, target->o, target);
					break;
				}

				case SV_TAUNT:
				{
					int lcn = getint(p);
					gameent *f = world::getclient(lcn);
					if(!f) break;
					f->lasttaunt = lastmillis;
					break;
				}

				case SV_RESUME:
				{
					for(;;)
					{
						int lcn = getint(p);
						if(p.overread() || lcn < 0) break;
						gameent *f = world::newclient(lcn);
						if(f!=world::player1) f->respawn(0);
						parsestate(f, p, true);
					}
					break;
				}

				case SV_ITEMSPAWN:
				{
					int ent = getint(p);
					if(!entities::ents.inrange(ent)) break;
					entities::setspawn(ent, true);
					playsound(S_ITEMSPAWN, entities::ents[ent]->o);
					const char *item = entities::entinfo(entities::ents[ent]->type, entities::ents[ent]->attr1, entities::ents[ent]->attr2, entities::ents[ent]->attr4, entities::ents[ent]->attr4, entities::ents[ent]->attr5, false);
					if(item)
					{
						s_sprintfd(ds)("@%s", item);
						part_text(entities::ents[ent]->o, ds, PART_TEXT_RISE, 5000, 0xFFFFFF, 3.f);
					}
					world::spawneffect(entities::ents[ent]->o, 0x221188, enttype[entities::ents[ent]->type].radius);
					break;
				}

				case SV_TRIGGER:
				{
					int ent = getint(p), st = getint(p);
					if(!entities::ents.inrange(ent)) break;
					entities::setspawn(ent, st ? true : false);
					break;
				}

				case SV_ITEMACC:
				{ // uses a specific drop so the client knows what to replace
					int lcn = getint(p), ent = getint(p), spawn = getint(p), gun = getint(p), drop = getint(p);
					gameent *target = world::getclient(lcn);
					if(!target) break;
					entities::useeffects(target, ent, spawn, gun, drop);
					target->requse = -1;
					break;
				}

				case SV_EDITVAR:
				{
					int t = getint(p);
					bool commit = true;
					getstring(text, p);
					ident *id = idents->access(text);
					if(!d || !id || !(id->flags&IDF_WORLD) || id->type != t) commit = false;
					switch(t)
					{
						case ID_VAR:
						{
							int val = getint(p);
							if(commit)
							{
								if(id->minval > id->maxval || val < id->minval || val > id->maxval)
									commit = false;
								setvar(text, val, true);
								conoutf("\fm%s set variable %s to %d", world::colorname(d), id->name, *id->storage.i);
							}
							break;
						}
						case ID_FVAR:
						{
							float val = getfloat(p);
							if(commit)
							{
								setfvar(text, val, true);
								conoutf("\fm%s set float variable %s to %f", world::colorname(d), id->name, *id->storage.f);
							}
							break;
						}
						case ID_SVAR:
						{
							string val;
							getstring(val, p);
							if(commit)
							{
								setsvar(text, val, true);
								conoutf("\fm%s set string variable %s to %s", world::colorname(d), id->name, *id->storage.s);
							}
							break;
						}
						case ID_ALIAS:
						{
							string val;
							getstring(val, p);
							if(commit || !id) // set aliases anyway
							{
								worldalias(text, val);
								conoutf("\fm%s set world alias %s to %s", world::colorname(d), text, val);
							}
							break;
						}
						default: break;
					}
					break;
				}

				case SV_EDITF:			  // coop editing messages
				case SV_EDITT:
				case SV_EDITM:
				case SV_FLIP:
				case SV_COPY:
				case SV_PASTE:
				case SV_ROTATE:
				case SV_REPLACE:
				case SV_DELCUBE:
				{
					if(!d) return;
					selinfo s;
					s.o.x = getint(p); s.o.y = getint(p); s.o.z = getint(p);
					s.s.x = getint(p); s.s.y = getint(p); s.s.z = getint(p);
					s.grid = getint(p); s.orient = getint(p);
					s.cx = getint(p); s.cxs = getint(p); s.cy = getint(p), s.cys = getint(p);
					s.corner = getint(p);
					int dir, mode, tex, newtex, mat, allfaces;
					ivec moveo;
					switch(type)
					{
						case SV_EDITF: dir = getint(p); mode = getint(p); mpeditface(dir, mode, s, false); break;
						case SV_EDITT: tex = getint(p); allfaces = getint(p); mpedittex(tex, allfaces, s, false); break;
						case SV_EDITM: mat = getint(p); mpeditmat(mat, s, false); break;
						case SV_FLIP: mpflip(s, false); break;
						case SV_COPY: if(d) mpcopy(d->edit, s, false); break;
						case SV_PASTE: if(d) mppaste(d->edit, s, false); break;
						case SV_ROTATE: dir = getint(p); mprotate(dir, s, false); break;
						case SV_REPLACE: tex = getint(p); newtex = getint(p); mpreplacetex(tex, newtex, s, false); break;
						case SV_DELCUBE: mpdelcube(s, false); break;
					}
					break;
				}
				case SV_REMIP:
				{
					if(!d) return;
					conoutf("\fm%s remipped", world::colorname(d));
					mpremip(false);
					break;
				}
				case SV_EDITENT:			// coop edit of ent
				{
					if(!d) return;
					int i = getint(p);
					float x = getint(p)/DMF, y = getint(p)/DMF, z = getint(p)/DMF;
					int type = getint(p);
					int attr1 = getint(p), attr2 = getint(p), attr3 = getint(p), attr4 = getint(p), attr5 = getint(p);

					mpeditent(i, vec(x, y, z), type, attr1, attr2, attr3, attr4, attr5, false);
					break;
				}

				case SV_EDITLINK:
				{
					if(!d) return;
					int b = getint(p), index = getint(p), node = getint(p);
					entities::linkents(index, node, b!=0, false, false);
				}

				case SV_PONG:
					addmsg(SV_CLIENTPING, "i", world::player1->ping = (world::player1->ping*5+lastmillis-getint(p))/6);
					break;

				case SV_CLIENTPING:
					if(!d) return;
					d->ping = getint(p);
					break;

				case SV_TIMEUP:
					world::timeupdate(getint(p));
					break;

				case SV_SERVMSG:
					getstring(text, p);
					conoutf("%s", text);
					break;

				case SV_SENDDEMOLIST:
				{
					int demos = getint(p);
					if(!demos) conoutf("\frno demos available");
					else loopi(demos)
					{
						getstring(text, p);
						conoutf("\fw%d. %s", i+1, text);
					}
					break;
				}

				case SV_DEMOPLAYBACK:
				{
					int on = getint(p);
					if(on) world::player1->state = CS_SPECTATOR;
					else
                    {
                        loopv(world::players) if(world::players[i]) world::clientdisconnected(i);
                    }
					demoplayback = on!=0;
					break;
				}

				case SV_CURRENTMASTER:
				{
					int mn = getint(p), priv = getint(p);
					world::player1->privilege = PRIV_NONE;
					loopv(world::players) if(world::players[i]) world::players[i]->privilege = PRIV_NONE;
					if(mn>=0)
					{
						gameent *m = world::getclient(mn);
						if(m) m->privilege = priv;
					}
					break;
				}

				case SV_EDITMODE:
				{
					int val = getint(p);
					if(!d) break;
					if(val) d->state = CS_EDITING;
					else d->state = CS_ALIVE;
					break;
				}

				case SV_SPECTATOR:
				{
					int sn = getint(p), val = getint(p);
					gameent *s = world::newclient(sn);
					if(!s) break;
					if(val)
					{
						if(s==world::player1 && editmode) toggleedit();
						s->state = CS_SPECTATOR;
					}
					else if(s->state==CS_SPECTATOR)
					{
						s->state = CS_DEAD;
						if(s==world::player1) hud::sb.showscores(true);
                        else s->resetinterp();
					}
					break;
				}

				case SV_WAITING:
				{
					int sn = getint(p);
					gameent *s = world::newclient(sn);
					if(!s) break;
					if(s==world::player1 && editmode) toggleedit();
					s->state = CS_WAITING;
					break;
				}

				case SV_SETTEAM:
				{
					int wn = getint(p), tn = getint(p);
					gameent *w = world::getclient(wn);
					if(!w) return;
					w->team = tn;
					break;
				}

				case SV_FLAGINFO:
				{
					int flag = getint(p), converted = getint(p),
							owner = getint(p), enemy = getint(p);
					if(m_stf(world::gamemode)) stf::updateflag(flag, owner, enemy, converted);
					break;
				}

				case SV_FLAGS:
				{
					int flag = 0, converted;
					while((converted = getint(p))>=0)
					{
						int owner = getint(p), enemy = getint(p);
						stf::st.initflag(flag++, owner, enemy, converted);
					}
					break;
				}

				case SV_TEAMSCORE:
				{
					int team = getint(p), total = getint(p);
					if(m_stf(world::gamemode)) stf::setscore(team, total);
					break;
				}

				case SV_INITFLAGS:
				{
					ctf::parseflags(p, m_ctf(world::gamemode));
					break;
				}

				case SV_DROPFLAG:
				{
					int ocn = getint(p), flag = getint(p);
					vec droploc;
					loopk(3) droploc[k] = getint(p)/DMF;
					gameent *o = world::newclient(ocn);
					if(o && m_ctf(world::gamemode)) ctf::dropflag(o, flag, droploc);
					break;
				}

				case SV_SCOREFLAG:
				{
					int ocn = getint(p), relayflag = getint(p), goalflag = getint(p), score = getint(p);
					gameent *o = world::newclient(ocn);
					if(o && m_ctf(world::gamemode)) ctf::scoreflag(o, relayflag, goalflag, score);
					break;
				}

				case SV_RETURNFLAG:
				{
					int ocn = getint(p), flag = getint(p);
					gameent *o = world::newclient(ocn);
					if(o && m_ctf(world::gamemode)) ctf::returnflag(o, flag);
					break;
				}

				case SV_TAKEFLAG:
				{
					int ocn = getint(p), flag = getint(p);
					gameent *o = world::newclient(ocn);
					if(o && m_ctf(world::gamemode)) ctf::takeflag(o, flag);
					break;
				}

				case SV_RESETFLAG:
				{
					int flag = getint(p);
					if(m_ctf(world::gamemode)) ctf::resetflag(flag);
					break;
				}

				case SV_GETMAP:
				{
					if(!needsmap)
					{
						conoutf("\fmserver has requested we send the map..");
						sendmap();
					}
					break;
				}

				case SV_SENDMAP:
				{
					int ocn = getint(p);
					gameent *o = world::newclient(ocn);
					conoutf("\fmmap uploaded by %s..", o ? world::colorname(o) : "server");
					if(needsmap) getmap();
					break;
				}

				case SV_NEWMAP:
				{
					int size = getint(p);
					if(size>=0) emptymap(size, true);
					else enlargemap(true);
					if(d && d!=world::player1)
					{
						int newsize = 0;
						while(1<<newsize < getworldsize()) newsize++;
						conoutf(size>=0 ? "%s started a new map of size %d" : "%s enlarged the map to size %d", world::colorname(d), newsize);
					}
					break;
				}

				case SV_INITAI:
				{
					int at = getint(p), on = getint(p), sk = clamp(getint(p), 1, 100), bn = getint(p);
					getstring(text, p);
					int tm = getint(p);
					gameent *b = world::newclient(bn);
					if(!b) break;
					ai::init(b, at, on, sk, bn, text, tm);
					break;
				}

				default:
				{
					neterr("type");
					return;
				}
			}
		}
	}

	int serverstat(serverinfo *a)
	{
		if(a->attr.length() > 4 && a->numplayers >= a->attr[4])
		{
			return SSTAT_FULL;
		}
		else if(a->attr.length() > 5) switch(a->attr[5])
		{
			case MM_LOCKED:
			{
				return SSTAT_LOCKED;
			}
			case MM_PRIVATE:
			{
				return SSTAT_PRIVATE;
			}
			default:
			{
				return SSTAT_OPEN;
			}
		}
		return SSTAT_UNKNOWN;
	}

	void resetserversort()
	{
		s_sprintfd(u)("serversort [%d %d %d]", SINFO_STATUS, SINFO_PLAYERS, SINFO_PING);
		execute(u);
	}
	ICOMMAND(serversortreset, "", (), resetserversort());

	int servercompare(serverinfo *a, serverinfo *b)
	{
		int ac = 0, bc = 0;

		if(a->address.host != ENET_HOST_ANY && a->ping < 999 &&
			a->attr.length() && a->attr[0] == GAMEVERSION) ac = 1;

		if(b->address.host != ENET_HOST_ANY && b->ping < 999 &&
			b->attr.length() && b->attr[0] == GAMEVERSION) bc = 1;

		if(ac > bc) return -1;
		if(ac < bc) return 1;

		#define retcp(c) if(c) { return c; }
		#define retsw(c,d,e) \
			if(c != d) \
			{ \
				if(e) { return c > d ? 1 : -1; } \
				else { return c < d ? 1 : -1; } \
			}

		int len = execute("listlen $serversort");

		loopi(len)
		{
			s_sprintfd(s)("at $serversort %d", i);

			int style = execute(s);
			serverinfo *aa = a, *ab = b;

			if(style < 0)
			{
				style = 0-style;
				aa = b;
				ab = a;
			}

			switch (style)
			{
				case SINFO_STATUS:
				{
					retsw(serverstat(aa), serverstat(ab), true);
					break;
				}
				case SINFO_HOST:
				{
					retcp(strcmp(aa->name, ab->name));
					break;
				}
				case SINFO_DESC:
				{
					retcp(strcmp(aa->sdesc, ab->sdesc));
					break;
				}
				case SINFO_PING:
				{
					retsw(aa->ping, ab->ping, true);
					break;
				}
				case SINFO_PLAYERS:
				{
					retsw(aa->numplayers, ab->numplayers, false);
					break;
				}
				case SINFO_MAXCLIENTS:
				{
					if(aa->attr.length() > 4) ac = aa->attr[4];
					else ac = 0;

					if(ab->attr.length() > 4) bc = ab->attr[4];
					else bc = 0;

					retsw(ac, bc, false);
					break;
				}
				case SINFO_GAME:
				{
					if(aa->attr.length() > 1) ac = aa->attr[1];
					else ac = 0;

					if(ab->attr.length() > 1) bc = ab->attr[1];
					else bc = 0;

					retsw(ac, bc, true);

					if(aa->attr.length() > 2) ac = aa->attr[2];
					else ac = 0;

					if(ab->attr.length() > 2) bc = ab->attr[2];
					else bc = 0;

					retsw(ac, bc, true);
					break;
				}
				case SINFO_MAP:
				{
					retcp(strcmp(aa->map, ab->map));
					break;
				}
				case SINFO_TIME:
				{
					if(aa->attr.length() > 3) ac = aa->attr[3];
					else ac = 0;

					if(ab->attr.length() > 3) bc = ab->attr[3];
					else bc = 0;

					retsw(ac, bc, false);
					break;
				}
				default:
					break;
			}
		}
		return strcmp(a->name, b->name);
	}

	void parsepacketclient(int chan, ucharbuf &p)	// processes any updates from the server
	{
		switch(chan)
		{
			case 0:
				parsepositions(p);
				break;

			case 1:
				parsemessages(-1, NULL, p);
				break;

			case 2:
				receivefile(p.buf, p.maxlen);
				break;
		}
	}

    void serverstartcolumn(g3d_gui *g, int i)
    {
		g->pushlist();

		if(g->buttonf("%s ", GUI_BUTTON_COLOR, NULL, serverinfotypes[i]) & G3D_UP)
		{
			string st; st[0] = 0;
			bool invert = false;
			int len = execute("listlen $serversort");
			loopk(len)
			{
				s_sprintfd(s)("at $serversort %d", k);

				int n = execute(s);
				if(abs(n) != i)
				{
					s_sprintfd(t)("%s%d", st[0] ? " " : "", n);
					s_sprintf(st)("%s%s", st[0] ? st : "", t);
				}
				else if(!k) invert = true;
			}
			s_sprintfd(u)("serversort [%d%s%s]",
				invert ? 0-i : i, st[0] ? " " : "", st[0] ? st : "");
			execute(u);
		}

		g->mergehits(true);
    }

    void serverendcolumn(g3d_gui *g, int i)
    {
        g->mergehits(false);
        g->poplist();
    }

    bool serverentry(g3d_gui *g, int i, serverinfo *si)
    {
		string text; text[0] = 0;
		int colour = serverstatus[serverstat(si)].colour;
		switch(i)
		{
			case SINFO_STATUS:
			{
				if(g->button("", colour, serverstatus[serverstat(si)].icon) & G3D_UP)
					return true;
				break;
			}
			case SINFO_HOST:
			{
				if(g->buttonf("%s:%d ", colour, NULL, si->name, si->port) & G3D_UP) return true;
				break;
			}
			case SINFO_DESC:
			{
				s_strncpy(text, si->sdesc, 18);
				if(g->buttonf("%s ", colour, NULL, text) & G3D_UP) return true;
				break;
			}
			case SINFO_PING:
			{
				s_sprintf(text)("%d", si->ping);
				if(g->buttonf("%s ", colour, NULL, text) & G3D_UP) return true;
				break;
			}
			case SINFO_PLAYERS:
			{
				s_sprintf(text)("%d", si->numplayers);
				if(g->buttonf("%s ", colour, NULL, text) & G3D_UP) return true;
				break;
			}
			case SINFO_MAXCLIENTS:
			{
				if(si->attr.length() > 4 && si->attr[5] >= 0)
					s_sprintf(text)("%d", si->attr[5]);
				if(g->buttonf("%s ", colour, NULL, text) & G3D_UP) return true;
				break;
			}
			case SINFO_GAME:
			{
				if(si->attr.length() > 1)
					s_sprintf(text)("%s", server::gamename(si->attr[1], si->attr[2]));
				if(g->buttonf("%s ", colour, NULL, text) & G3D_UP) return true;
				break;
			}
			case SINFO_MAP:
			{
				s_strncpy(text, si->map, 18);
				if(g->buttonf("%s ", colour, NULL, text) & G3D_UP) return true;
				break;
			}
			case SINFO_TIME:
			{
				if(si->attr.length() > 3 && si->attr[4] >= 0)
					s_sprintf(text)("%d %s", si->attr[4], si->attr[4] == 1 ? "min" : "mins");
				if(g->buttonf("%s ", colour, NULL, text) & G3D_UP) return true;
				break;
			}
			default:
				break;
		}
		return false;
    }

	int serverbrowser(g3d_gui *g)
	{
		if(servers.empty())
		{
			g->pushlist();
			g->text("no servers, press update to see some..", GUI_TEXT_COLOR);
			g->poplist();
			return -1;
		}
		int n = -1;
		for(int start = 0; start < servers.length();)
		{
			if(start > 0) g->tab();
			int end = servers.length();
			g->pushlist();
			loopi(SINFO_MAX)
			{
				serverstartcolumn(g, i);
				for(int j = start; j < end; j++)
				{
					if(!i && g->shouldtab()) { end = j; break; }
					serverinfo *si = servers[j];
					if(si->ping >= 0 && si->attr.length() && si->attr[0]==GAMEVERSION)
					{
						if(serverentry(g, i, si)) n = j;
					}
				}
				serverendcolumn(g, i);
			}
			g->poplist();
			start = end;
		}
		return n;
	}
}
