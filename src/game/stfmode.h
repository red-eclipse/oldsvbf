// server side stf manager
struct stfservmode : stfstate, servmode
{
	int scoresec;
	bool hasflaginfo;

	stfservmode() : scoresec(0), hasflaginfo(false) {}

	void reset(bool empty)
	{
		stfstate::reset();
		scoresec = 0;
		hasflaginfo = false;
	}

	void stealflag(int n, int team)
	{
		flag &b = flags[n];
		loopv(clients) if(clients[i]->state.aitype <= AI_BOT)
		{
			server::clientinfo *ci = clients[i];
			if(ci->state.state==CS_ALIVE && ci->team && ci->team == team && insideflag(b, ci->state.o))
				b.enter(ci->team);
		}
		sendflag(n);
	}

	void moveflags(int team, const vec &oldpos, const vec &newpos)
	{
		if(!team) return;
		loopv(flags)
		{
			flag &b = flags[i];
			bool leave = insideflag(b, oldpos),
				 enter = insideflag(b, newpos);
			if(leave && !enter && b.leave(team)) sendflag(i);
			else if(enter && !leave && b.enter(team)) sendflag(i);
			else if(leave && enter && b.steal(team)) stealflag(i, team);
		}
	}

	void leaveflags(int team, const vec &o)
	{
		moveflags(team, o, vec(-1e10f, -1e10f, -1e10f));
	}

	void enterflags(int team, const vec &o)
	{
		moveflags(team, vec(-1e10f, -1e10f, -1e10f), o);
	}

    int fragvalue(clientinfo *victim, clientinfo *actor)
    {
    	int value = 1;
    	loopv(flags) if(insideflag(flags[i], victim->state.o)) value++;
    	return victim==actor || victim->team == actor->team ? -value : value;
    }

	void addscore(int i, int team, int n)
	{
		if(!n) return;
		flag &b = flags[i];
		loopvk(clients) if(clients[k]->state.aitype <= AI_BOT && team == clients[k]->team && insideflag(b, clients[k]->state.o)) givepoints(clients[k], n);
		score &cs = findscore(team);
		cs.total += n;
		sendf(-1, 1, "ri3", SV_SCORE, team, cs.total);
	}

	void update()
	{
		endcheck();
		int t = (gamemillis/SCORESECS)-((gamemillis-(curtime+scoresec))/SCORESECS);
		if(t < 1) { scoresec += curtime; return; }
		else scoresec = 0;
		loopv(flags)
		{
			flag &b = flags[i];
			if(b.enemy)
			{
                if(!b.owners || !b.enemies)
                {
                	int pts = b.occupy(b.enemy, GVAR(stfpoints)*(b.enemies ? b.enemies : -(1+b.owners))*t, GVAR(stfoccupy));
                	if(pts > 0) loopvk(clients) if(clients[k]->state.aitype <= AI_BOT && b.owner == clients[k]->team && insideflag(b, clients[k]->state.o)) givepoints(clients[k], 3);
                }
				sendflag(i);
			}
			else if(b.owner)
			{
				b.securetime += t;
				int score = b.securetime/SCORESECS - (b.securetime-t)/SCORESECS;
				if(score) addscore(i, b.owner, score);
				sendflag(i);
			}
		}
	}

	void sendflag(int i)
	{
		flag &b = flags[i];
		sendf(-1, 1, "ri5", SV_FLAGINFO, i, b.enemy ? b.converted : 0, b.owner, b.enemy);
	}

	void sendflags()
	{
		ENetPacket *packet = enet_packet_create(NULL, MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
		ucharbuf p(packet->data, packet->dataLength);
		initclient(NULL, p, false);
		enet_packet_resize(packet, p.length());
		sendpacket(-1, 1, packet);
		if(!packet->referenceCount) enet_packet_destroy(packet);
	}

	void initclient(clientinfo *ci, ucharbuf &p, bool connecting)
	{
		if(connecting)
        {
            loopv(scores)
		    {
			    score &cs = scores[i];
			    putint(p, SV_SCORE);
			    putint(p, cs.team);
			    putint(p, cs.total);
		    }
        }
		putint(p, SV_FLAGS);
		putint(p, flags.length());
		loopv(flags)
		{
			flag &b = flags[i];
			putint(p, b.converted);
			putint(p, b.owner);
			putint(p, b.enemy);
		}
	}

	void winner(int team, int score)
	{
		sendf(-1, 1, "ri3", SV_SCORE, team, score);
		startintermission();
	}

	void endcheck()
	{
		int maxscore = GVAR(stflimit) ? GVAR(stflimit) : INT_MAX-1;
		loopi(numteams(gamemode, mutators))
		{
			int lastteam = i+TEAM_FIRST;
			if(findscore(lastteam).total >= maxscore)
			{
				findscore(lastteam).total = maxscore;
				sendf(-1, 1, "ri3s", SV_ANNOUNCE, S_GUIBACK, CON_SELF, "\fcsecure limit has been reached!");
				winner(lastteam, maxscore);
				return;
			}
		}
		if(GVAR(stffinish))
		{
			int lastteam = TEAM_NEUTRAL;
			loopv(flags)
			{
				flag &b = flags[i];
				if(b.owner)
				{
					if(!lastteam) lastteam = b.owner;
					else if(lastteam != b.owner)
					{
						lastteam = TEAM_NEUTRAL;
						break;
					}
				}
				else
				{
					lastteam = TEAM_NEUTRAL;
					break;
				}
			}
			if(lastteam)
			{
				findscore(lastteam).total = INT_MAX-1;
				sendf(-1, 1, "ri3s", SV_ANNOUNCE, S_GUIBACK, CON_SELF, "\fcall flags have been secured!");
				winner(lastteam, INT_MAX-1);
				return;
			}
		}
	}

	void entergame(clientinfo *ci)
	{
        if(!hasflaginfo || ci->state.state!=CS_ALIVE || ci->state.aitype >= AI_START) return;
		enterflags(ci->team, ci->state.o);
	}

	void spawned(clientinfo *ci)
	{
		if(!hasflaginfo || ci->state.aitype >= AI_START) return;
		enterflags(ci->team, ci->state.o);
	}

    void leavegame(clientinfo *ci, bool disconnecting = false)
	{
        if(!hasflaginfo || ci->state.state!=CS_ALIVE || ci->state.aitype >= AI_START) return;
		leaveflags(ci->team, ci->state.o);
	}

	void died(clientinfo *ci, clientinfo *actor)
	{
		if(!hasflaginfo || ci->state.aitype >= AI_START) return;
		leaveflags(ci->team, ci->state.o);
	}

	void moved(clientinfo *ci, const vec &oldpos, const vec &newpos)
	{
		if(!hasflaginfo || ci->state.aitype >= AI_START) return;
		moveflags(ci->team, oldpos, newpos);
	}

	void regen(clientinfo *ci, int &total, int &amt, int &delay)
	{
		if(hasflaginfo) loopv(flags)
		{
			flag &b = flags[i];
			if(b.owner == ci->team && !b.enemy && insideflag(b, ci->state.o, 2.f))
			{
				if(GVAR(extrahealth)) total = max(GVAR(extrahealth), total);
				if(ci->state.lastregen && GVAR(regenguard)) delay = GVAR(regenguard);
				if(GVAR(regenextra)) amt = GVAR(regenextra);
				return;
			}
		}
	}

	void parseflags(ucharbuf &p)
	{
		int numflags = getint(p);
		if(numflags)
		{
			loopi(numflags)
			{
				vec o;
				o.x = getint(p)/DMF;
				o.y = getint(p)/DMF;
				o.z = getint(p)/DMF;
				if(!hasflaginfo) addflag(o);
			}
			if(!hasflaginfo)
			{
				hasflaginfo = true;
				sendflags();
				loopv(clients) if(clients[i]->state.state==CS_ALIVE) entergame(clients[i]);
			}
		}
	}
} stfmode;
