// server-side ai manager
namespace aiman
{
	int findaiclient(int exclude)
	{
		vector<int> siblings;
		while(siblings.length() < clients.length()) siblings.add(-1);
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(ci->clientnum < 0 || ci->state.isai() || !ci->name[0] || ci->clientnum == exclude)
				siblings[i] = -1;
			else
			{
				siblings[i] = 0;
				loopvj(clients) if(clients[j]->state.isai(-1, false) && clients[j]->state.ownernum == ci->clientnum)
					siblings[i]++;
			}
		}
		while(!siblings.empty())
		{
			int q = -1;
			loopv(siblings)
				if(siblings[i] >= 0 && (!siblings.inrange(q) || siblings[i] < siblings[q]))
					q = i;
			if(siblings.inrange(q))
			{
				if(clients.inrange(q)) return clients[q]->clientnum;
				else siblings.removeunordered(q);
			}
			else break;
		}
		return -1;
	}

	bool addai(int type, int skill)
	{
		loopv(clients) if(clients[i]->state.aitype == type && (clients[i]->state.ownernum < 0 || clients[i]->state.aireinit < 0))
		{ // reuse a slot that was going to removed
			clients[i]->state.ownernum = findaiclient();
			clients[i]->state.aireinit = 1;
			return true;
		}
		int cn = addclient(ST_REMOTE);
		if(cn >= 0)
		{
			clientinfo *ci = (clientinfo *)getinfo(cn);
			if(ci)
			{
				int s = skill, m = sv_botmaxskill > sv_botminskill ? sv_botmaxskill : sv_botminskill,
					n = sv_botminskill < sv_botmaxskill ? sv_botminskill : sv_botmaxskill;
				if(skill > m || skill < n) s = (m != n ? rnd(m-n) + n + 1 : m);
				ci->clientnum = cn;
				ci->state.aitype = type;
				ci->state.ownernum = findaiclient();
				ci->state.skill = clamp(s, 1, 100);
				ci->state.state = CS_WAITING;
				ci->state.aireinit = 0;
				clients.add(ci);
				ci->state.lasttimeplayed = lastmillis;
				s_strncpy(ci->name, aitype[ci->state.aitype].name, MAXNAMELEN);
				ci->team = chooseteam(ci);
				sendf(-1, 1, "ri5si", SV_INITAI, ci->clientnum, ci->state.ownernum, ci->state.aitype, ci->state.skill, ci->name, ci->team);
				int nospawn = 0;
				if(smode && !smode->canspawn(ci, true)) { nospawn++; }
				mutate(smuts, if(!mut->canspawn(ci, true)) { nospawn++; });
				if(nospawn)
				{
					ci->state.state = CS_DEAD;
					sendf(-1, 1, "ri2", SV_FORCEDEATH, ci->clientnum);
				}
				else sendspawn(ci);
				ci->online = true;
				return true;
			}
			delclient(cn);
		}
		return false;
	}

	void refreshai()
	{
		loopv(clients) if(clients[i]->state.isai(-1, false))
		{
			clientinfo *ci = clients[i];
			int team = chooseteam(ci, ci->team);
			if(ci->team != team)
			{
				if(smode) smode->changeteam(ci, ci->team, team);
				mutate(smuts, mut->changeteam(ci, ci->team, team));
				ci->team = team;
				ci->state.aireinit = 1;
			}
		}
	}

	void deleteai(clientinfo *ci)
	{
		int cn = ci->clientnum;
		if(smode) smode->leavegame(ci, true);
		mutate(smuts, mut->leavegame(ci));
		ci->state.timeplayed += lastmillis - ci->state.lasttimeplayed;
		savescore(ci);
		dropitems(ci, true);
		sendf(-1, 1, "ri2", SV_CDIS, cn);
		clients.removeobj(ci);
		delclient(cn);
		refreshai();
	}

	bool delai(int type)
	{
		loopvrev(clients) if(clients[i]->state.isai(type, false))
		{
			deleteai(clients[i]);
			return true;
		}
		return false;
	}

	void reinitai(clientinfo *ci)
	{
		if(ci->state.aireinit < 0 || ci->state.ownernum < 0) deleteai(ci);
		else if(ci->state.aireinit >= 1)
		{
			sendf(-1, 1, "ri5si", SV_INITAI, ci->clientnum, ci->state.ownernum, ci->state.aitype, ci->state.skill, ci->name, ci->team);
			if(ci->state.aireinit >= 2)
			{
				if(smode) smode->entergame(ci);
				mutate(smuts, mut->entergame(ci));
			}
			ci->state.aireinit = 0;
		}
	}

	void shiftai(clientinfo *ci, int reinit = 1, int cn = -1)
	{
		if(cn < 0 || reinit < 0 || ci->state.aireinit < 0) ci->state.ownernum = ci->state.aireinit = -1;
		else
		{
			if(ci->state.aireinit < reinit)
			{
				if(reinit >= 2)
				{
					if(smode) smode->leavegame(ci);
					mutate(smuts, mut->leavegame(ci));
				}
				ci->state.aireinit = reinit;
			}
			ci->state.ownernum = cn;
		}
	}

	void removeai(clientinfo *ci, bool complete)
	{ // either schedules a removal, or someone else to assign to
		loopv(clients) if(clients[i]->state.isai() && clients[i]->state.ownernum == ci->clientnum)
			shiftai(clients[i], 2, complete ? -1 : findaiclient(ci->clientnum));
	}

	bool reassignai(int exclude)
	{
		vector<int> siblings;
		while(siblings.length() < clients.length()) siblings.add(-1);
		int hi = -1, lo = -1;
		loopv(clients)
		{
			clientinfo *ci = clients[i];
			if(ci->clientnum < 0 || ci->state.isai() || !ci->name[0] || ci->clientnum == exclude)
				siblings[i] = -1;
			else
			{
				siblings[i] = 0;
				loopvj(clients) if(clients[j]->state.isai(-1, false) && clients[j]->state.ownernum == ci->clientnum)
					siblings[i]++;
				if(!siblings.inrange(hi) || siblings[i] > siblings[hi]) hi = i;
				if(!siblings.inrange(lo) || siblings[i] < siblings[lo]) lo = i;
			}
		}
		if(siblings.inrange(hi) && siblings.inrange(lo) && (siblings[hi]-siblings[lo]) > 1)
		{
			clientinfo *ci = clients[hi];
			loopv(clients) if(clients[i]->state.isai(-1, false) && clients[i]->state.ownernum == ci->clientnum)
			{
				shiftai(clients[i], 1, clients[lo]->clientnum);
				return true;
			}
		}
		return false;
	}

	void checksetup()
	{
		int m = sv_botmaxskill > sv_botminskill ? sv_botmaxskill : sv_botminskill,
			n = sv_botmaxskill < sv_botminskill ? sv_botmaxskill : sv_botminskill;
		loopv(clients) if(clients[i]->state.isai(-1, false))
		{
			clientinfo *ci = clients[i];
			if(ci->state.skill > m || ci->state.skill < n)
			{ // needs re-skilling
				ci->state.skill = (m != n ? rnd(m-n) + n + 1 : m);
				if(ci->state.aireinit <= 1 && ci->state.aireinit >= 0)
					ci->state.aireinit = 1;
			}
		}
		loopvrev(clients) if(clients[i]->state.isai()) reinitai(clients[i]);
	}

	void clearai()
	{ // clear and remove all ai immediately
		loopvrev(clients) if(clients[i]->state.isai()) deleteai(clients[i]);
	}

	void checkai()
	{
		if(!m_demo(gamemode) && !m_lobby(gamemode) && numclients(-1, false, true))
		{
			if(!notgotinfo)
			{
				if(m_play(gamemode) && sv_botbalance > 0.f)
				{
					int balance = int(sv_botbalance), minamt = balance;
					if(m_team(gamemode, mutators))
					{
						balance = max(int(numplayers*2*sv_botbalance), minamt);
						int numt = numteams(gamemode, mutators), offt = balance%numt;
						if(offt) balance += numt-offt;
					}
					else balance = max(int(numplayers*sv_botbalance), minamt);
					while(numclients(-1, true, false) < balance) if(!addai(AI_BOT, -1)) break;
					while(numclients(-1, true, false) > balance) if(!delai(AI_BOT)) break;
				}
				while(true) if(!reassignai()) break;
				checksetup();
			}
		}
		else clearai();
	}

	void reqadd(clientinfo *ci, int skill)
	{
		if(haspriv(ci, PRIV_MASTER, true))
		{
			if(m_lobby(gamemode)) sendf(ci->clientnum, 1, "ri", SV_NEWGAME);
			else
			{
				if(sv_botbalance > 0.f)
				{
					setfvar("sv_botbalance", 0.f, true);
					s_sprintfd(val)("%.f", 0.f);
					sendf(-1, 1, "ri2ss", SV_COMMAND, ci->clientnum, "botbalance", val);
				}
				if(!addai(AI_BOT, skill))
					srvmsgf(ci->clientnum, "failed to create or assign bot");
			}
		}
	}

	void reqdel(clientinfo *ci)
	{
		if(haspriv(ci, PRIV_MASTER, true))
		{
			if(m_lobby(gamemode)) sendf(ci->clientnum, 1, "ri", SV_NEWGAME);
			else
			{
				if(sv_botbalance > 0.f)
				{
					setfvar("sv_botbalance", 0.f, true);
					s_sprintfd(val)("%.f", 0.f);
					sendf(-1, 1, "ri2ss", SV_COMMAND, ci->clientnum, "botbalance", val);
				}
				if(!delai(AI_BOT))
					srvmsgf(ci->clientnum, "failed to remove any bots");
			}
		}
	}
}
