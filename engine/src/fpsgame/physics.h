struct physics : iphysics
{
	fpsclient &cl;
	
	physics(fpsclient &_cl) : cl(_cl)
	{
		CVAR(physics, gravity,		1,			200,		INT_MAX-1);	// gravity
		CVAR(physics, speed,		1,			100,		INT_MAX-1);	// speed
		CVAR(physics, jumpvel,		0,			200,		INT_MAX-1);	// extra velocity to add when jumping
		CVAR(physics, watertype,	WT_WATER,	WT_WATER,	WT_MAX-1);	// water type (0: water, 1: slime, 2: lava)
		CVAR(physics, watervel,		0,			200,		1024);		// extra water velocity
	}

	float stairheight(physent *d)
	{
		return 4.1f;
	}
	float floorz(physent *d)
	{
		return 0.867f;
	}
	float slopez(physent *d)
	{
		return 0.5f;
	}
	float wallz(physent *d)
	{
		return 0.2f;
	}
	float jumpvel(physent *d)
	{
		return EXTENSIONS ? (d->inwater ? float(getvar("watervel")) : float(getvar("jumpvel"))) : 125.f;
	}
	float gravity(physent *d)
	{
		return EXTENSIONS ? float(getvar("gravity")) : 200.f;
	}
	float speed(physent *d)
	{
		if (EXTENSIONS && d->state != CS_SPECTATOR && d->state != CS_EDITING)
		{
			return d->maxspeed * (float(getvar("speed"))/100.f);
		}
		return d->maxspeed;
	}
	float stepspeed(physent *d)
	{
		return 1.0f;
	}
	float watergravscale(physent *d)
	{
		return d->move || d->strafe ? 0.f : 4.f;
	}
	float waterdampen(physent *d)
	{
		return 8.f;
	}
	float waterfric(physent *d)
	{
		return 20.f;
	}
	float floorfric(physent *d)
	{
		return 6.f;
	}
	float airfric(physent *d)
	{
		return 30.f;
	}

	bool movepitch(physent *d)
	{
		return d->type == ENT_CAMERA || d->state == CS_SPECTATOR || d->state == CS_EDITING;
	}

	void updateroll(physent *d)
	{
		extern int maxroll, curtime;
		
		if(d->strafe==0)
		{
			d->roll = d->roll/(1+(float)sqrtf((float)curtime)/25);
		}
		else
		{
			d->roll += d->strafe*curtime/-30.0f;
			if(d->roll>maxroll) d->roll = (float)maxroll;
			if(d->roll<-maxroll) d->roll = (float)-maxroll;
		}
	}

	void updatewater(fpsent *d, int waterlevel)
	{
		vec v(d->o.x, d->o.y, d->o.z-d->eyeheight);
		int mat = getmatvec(v);
			
		if (waterlevel || mat == MAT_WATER || mat == MAT_LAVA)
		{
			if (waterlevel)
			{
				uchar col[3] = { 255, 255, 255 };

				if (mat == MAT_WATER) getwatercolour(col);
				else if (mat == MAT_LAVA) getlavacolour(col);
					
				int wcol = (col[2] + (col[1] << 8) + (col[0] << 16));
				
				part_spawn(v, vec(d->xradius, d->yradius, ENTPART), 0, 19, 100, 200, wcol);
			}
			
			if (waterlevel || d->inwater)
			{
				int water = getvar("watertype");
				
				if (waterlevel < 0 && (mat == MAT_WATER && water == WT_WATER))
				{
					playsound(S_SPLASH1, d != cl.player1 ? &d->o : NULL);
				}
				else if (waterlevel > 0 && (mat == MAT_WATER && water == WT_WATER))
				{
					playsound(S_SPLASH2, d != cl.player1 ? &d->o : NULL);
				}
				else if (waterlevel < 0 && ((mat == MAT_WATER && (water == WT_KILL || water == WT_HURT)) || mat == MAT_LAVA))
				{
					part_spawn(v, vec(d->xradius, d->yradius, ENTPART), 0, 5, 200, 500, COL_WHITE);
					if (!g_sauer && mat != MAT_LAVA && (d == cl.player1 || cl.bc.isbot(d))) cl.suicide(d);
				}
			}
		}
	}
	
	void trigger(physent *d, bool local, int floorlevel, int waterlevel)
	{
		if (waterlevel) updatewater((fpsent *)d, waterlevel);
			
		if (floorlevel > 0)
		{
			playsound(S_JUMP, d != cl.player1 ? &d->o : NULL);
		}
		else if (floorlevel < 0)
		{
			playsound(S_LAND, d != cl.player1 ? &d->o : NULL);
		}
	}
	
	bool move(physent *d, int moveres = 20, bool local = true, int secs = 0, int repeat = 0)
	{
		loopi(physicsrepeat)
		if (!secs) secs = curtime;
		if (!repeat) repeat = physicsrepeat;
		
		loopi(repeat)
		{
			if (!moveplayer(d, moveres, local, min(secs, minframetime))) return false;
			if (d->o.z < 0 && d->state == CS_ALIVE)
			{
				cl.suicide(d);
				return false;
			}
		}
		return true;
	}
};
