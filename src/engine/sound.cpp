#include "pch.h"
#include "engine.h"

hashtable<const char *, soundsample> soundsamples;
vector<soundslot> gamesounds, mapsounds;
vector<sound> sounds;

bool nosound = true;
Mix_Music *music = NULL;
char *musicdonecmd = NULL;
int soundsatonce = 0, lastsoundmillis = 0;

void setmusicvol(int musicvol)
{
	if (!nosound)
	{
		if (music) Mix_VolumeMusic((musicvol*MIX_MAX_VOLUME)/255);
	}
}


VARP(soundvol, 0, 255, 255);
VARFP(musicvol, 0, 255, 255, setmusicvol(musicvol));
VARF(soundmono, 0, 0, 1, initwarning());
VARF(soundchans, 0, 1024, INT_MAX-1, initwarning());
VARF(soundbufferlen, 0, 1024, INT_MAX-1, initwarning());
VARF(soundfreq, 0, 44100, 48000, initwarning());
VARP(maxsoundsatonce, 0, 24, INT_MAX-1);

void initsound()
{
	if (!nosound) return;

	if (Mix_OpenAudio(soundfreq, MIX_DEFAULT_FORMAT, soundmono ? 1 : 2, soundbufferlen) == -1)
	{
		conoutf("sound initialisation failed: %s", Mix_GetError());
		return;
	}

	Mix_AllocateChannels(soundchans);
	nosound = false;
}

void musicdone(bool docmd)
{
	if (Mix_PlayingMusic()) Mix_HaltMusic();

	if (music)
	{
		Mix_FreeMusic(music);
		music = NULL;
	}

	if (musicdonecmd != NULL)
	{
		char *cmd = musicdonecmd;
		musicdonecmd = NULL;
		if (docmd) execute(cmd);
		delete[] cmd;
	}
}

void stopsound()
{
	if (nosound) return;
	nosound = true;
	musicdone(false);
	clearsound();
	soundsamples.clear();
	gamesounds.setsizenodelete(0);
	Mix_CloseAudio();
}

void removesound(int c)
{
	if (Mix_Playing(c)) Mix_HaltChannel(c);
	if (sounds.inrange(c)) sounds[c].inuse = false;
}

void clearsound()
{
	loopv(sounds) removesound(i);
	mapsounds.setsizenodelete(0);
}

void playmusic(char *name, char *cmd)
{
	if (nosound || !musicvol)

	musicdone(false);

	if (soundvol && musicvol && *name)
	{
		if (cmd[0]) musicdonecmd = newstring(cmd);
		s_sprintfd(sn)("%s", name);
		const char *file = findfile(sn, "rb");

		if ((music = Mix_LoadMUS(file)))
		{
			Mix_PlayMusic(music, cmd[0] ? 0 : -1);
			Mix_VolumeMusic((musicvol*MIX_MAX_VOLUME)/255);
		}
		else
		{
			conoutf("could not play music: %s", sn);
		}
	}
}

COMMANDN(music, playmusic, "ss");

int findsound(const char *name, int vol, vector<soundslot> &sounds)
{
	loopv(sounds)
	{
		if (!strcmp(sounds[i].sample->name, name) && (!vol || sounds[i].vol == vol)) return i;
	}
	return -1;
}

int addsound(const char *name, int vol, vector<soundslot> &sounds)
{
	soundsample *sample = soundsamples.access(name);
	if (!sample)
	{
		char *n = newstring(name);
		sample = &soundsamples[n];
		sample->name = n;
		sample->sound = NULL;
	}
	if (!sample->sound)
	{
		const char *exts[] = { "", ".wav", ".ogg" };
		string buf;
		loopi(sizeof(exts)/sizeof(exts[0]))
		{
			s_sprintf(buf)("sounds/%s%s", sample->name, exts[i]);
			const char *file = findfile(path(buf), "rb");
			if ((sample->sound = Mix_LoadWAV(file)) != NULL) break;
		}

		if (!sample->sound) { conoutf("failed to load sample: %s", sample->name); return -1; }
	}
	soundslot &slot = sounds.add();
	slot.sample = sample;
	slot.vol = vol > 0 ? min(vol, 255) : 255;
	return sounds.length()-1;
}

ICOMMAND(registersound, "si", (char *n, int *v), intret(addsound(n, *v, gamesounds)));
ICOMMAND(mapsound, "si", (char *n, int *v), intret(addsound(n, *v, mapsounds)));

void checksound()
{
	if (nosound) return;

	loopv(sounds)
	{
		if (sounds[i].inuse)
		{
			if (Mix_Playing(i))
			{
				vec v;
				sounds[i].curvol = soundvol*sounds[i].vol*sounds[i].slot->vol*MIX_MAX_VOLUME/255/255/255;
				sounds[i].dist = camera1->o.dist(*sounds[i].pos, v);
	
				if (sounds[i].dist > 6.f) // only if it is within our radius
				{
					if (!soundmono && (v.x != 0 || v.y != 0))
					{
						float yaw = -atan2f(v.x, v.y) - camera1->yaw*RAD; // relative angle of sound along X-Y axis
						sounds[i].curpan = int(255.9f*(0.5f*sinf(yaw)+0.5f)); // range is from 0 (left) to 255 (right)
					}
					
					sounds[i].curvol -= (int)(sounds[i].dist*sounds[i].curvol/MIX_MAX_VOLUME);
				}
				else sounds[i].curpan = 127;
	
				sounds[i].curvol = clamp(sounds[i].curvol, 0, MIX_MAX_VOLUME);

				Mix_Volume(i, sounds[i].curvol);
				Mix_SetPanning(i, 255-sounds[i].curpan, sounds[i].curpan);
			}
			else sounds[i].inuse = false;
		}
	}
	if (music && !Mix_PlayingMusic()) musicdone(true);
}

int playsound(int n, vec *pos, int vol, int loop, bool copy, bool mapsnd)
{
	if (nosound || !soundvol || !camera1 || !cc->ready()) return -1;

	if (!mapsnd)
	{
		if (totalmillis == lastsoundmillis) soundsatonce++;
		else soundsatonce = 1;
		lastsoundmillis = totalmillis;
		if (maxsoundsatonce && soundsatonce > maxsoundsatonce) return -1;
	}

	vec *p = pos != NULL ? pos : &camera1->o;

	vector<soundslot> &soundset = mapsnd ? mapsounds : gamesounds;

	if (soundset.inrange(n) && soundset[n].sample->sound)
	{
		int chan = Mix_PlayChannel(-1, soundset[n].sample->sound, loop);

		if (chan >= 0)
		{
			while(chan >= sounds.length()) sounds.add().inuse = false;
			sounds[chan].slot = &soundset[n];
			sounds[chan].vol = vol >= 1 && vol <= 255 ? vol : 255;
			sounds[chan].inuse = true;
			sounds[chan].map = mapsnd;

			if (copy)
			{
				sounds[chan].posval = vec(*p);
				sounds[chan].pos = &sounds[chan].posval;
			}
			else sounds[chan].pos = p;

			return chan;
		}
		else conoutf("cannot play sound %d (%s): %s", n, soundset[n].sample->name, Mix_GetError());
	}
	else conoutf("unregistered sound: %d", n);

	return -1;
}

void sound(int *n) { intret(playsound(*n, NULL, false, false)); }
COMMAND(sound, "i");

