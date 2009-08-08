// worldio.cpp: loading & saving of maps and savegames

#include "engine.h"

generic mapexts[] = {
	{ ".bgz", MAP_BFGZ },
	{ ".ogz", MAP_OCTA },
};

generic mapdirs[] = {
	{ "maps", MAP_BFGZ },
	{ "base", MAP_OCTA },
};

VAR(maptype, 1, -1, -1);
SVAR(mapfile, "");
SVAR(mapname, "");

void fixmaptitle()
{
	const char *title = maptitle, *author = strstr(title, " by ");
	if(author && *author)
	{
		char *t = newstring(title, author-title);
		author += 4; setsvar("maptitle", t); setsvar("mapauthor", author);
		delete[] t;
	}
}

SVARFW(maptitle, "", fixmaptitle());
SVARW(mapauthor, "");

void setnames(const char *fname, int type)
{
	maptype = type >= 0 || type <= MAP_MAX-1 ? type : MAP_BFGZ;

	string fn, mn, mf;
	if(fname != NULL) formatstring(fn)("%s", fname);
	else formatstring(fn)("%s/untitled", mapdirs[maptype].name);

	if(strpbrk(fn, "/\\")) copystring(mn, fn);
	else formatstring(mn)("%s/%s", mapdirs[maptype].name, fn);
	setsvar("mapname", mn);

	formatstring(mf)("%s%s", mapname, mapexts[maptype].name);
	setsvar("mapfile", mf);
}

enum { OCTSAV_CHILDREN = 0, OCTSAV_EMPTY, OCTSAV_SOLID, OCTSAV_NORMAL, OCTSAV_LODCUBE };

void savec(cube *c, stream *f, bool nolms)
{
	loopi(8)
	{
		if(c[i].children && (!c[i].ext || !c[i].ext->surfaces))
		{
			f->putchar(OCTSAV_CHILDREN);
			savec(c[i].children, f, nolms);
		}
		else
		{
			int oflags = 0;
			if(c[i].ext && c[i].ext->merged) oflags |= 0x80;
			if(c[i].children) f->putchar(oflags | OCTSAV_LODCUBE);
			else if(isempty(c[i])) f->putchar(oflags | OCTSAV_EMPTY);
			else if(isentirelysolid(c[i])) f->putchar(oflags | OCTSAV_SOLID);
			else
			{
				f->putchar(oflags | OCTSAV_NORMAL);
				f->write(c[i].edges, 12);
			}
			loopj(6) f->putlil<ushort>(c[i].texture[j]);
			uchar mask = 0;
			if(c[i].ext)
			{
				if(c[i].ext->material != MAT_AIR) mask |= 0x80;
				if(c[i].ext->normals && !nolms)
				{
					mask |= 0x40;
					loopj(6) if(c[i].ext->normals[j].normals[0] != bvec(128, 128, 128)) mask |= 1 << j;
				}
			}
			// save surface info for lighting
			if(!c[i].ext || !c[i].ext->surfaces || nolms)
			{
				f->putchar(mask);
				if(c[i].ext)
				{
					if(c[i].ext->material != MAT_AIR) f->putchar(c[i].ext->material);
					if(c[i].ext->normals && !nolms) loopj(6) if(mask & (1 << j))
					{
						loopk(sizeof(surfaceinfo)) f->putchar(0);
						f->write(&c[i].ext->normals[j], sizeof(surfacenormals));
					}
				}
			}
			else
			{
                int numsurfs = 6;
                loopj(6)
                {
                    surfaceinfo &surface = c[i].ext->surfaces[j];
                    if(surface.lmid >= LMID_RESERVED || surface.layer!=LAYER_TOP)
                    {
                        mask |= 1 << j;
                        if(surface.layer&LAYER_BLEND) numsurfs++;
                    }
                }
				f->putchar(mask);
				if(c[i].ext->material != MAT_AIR) f->putchar(c[i].ext->material);
                loopj(numsurfs) if(j >= 6 || mask & (1 << j))
                {
                    surfaceinfo tmp = c[i].ext->surfaces[j];
                    lilswap(&tmp.x, 2);
                    f->write(&tmp, sizeof(surfaceinfo));
                    if(j < 6 && c[i].ext->normals) f->write(&c[i].ext->normals[j], sizeof(surfacenormals));
                }
			}
			if(c[i].ext && c[i].ext->merged)
			{
				f->putchar(c[i].ext->merged | (c[i].ext->mergeorigin ? 0x80 : 0));
				if(c[i].ext->mergeorigin)
				{
					f->putchar(c[i].ext->mergeorigin);
					int index = 0;
					loopj(6) if(c[i].ext->mergeorigin&(1<<j))
					{
						mergeinfo tmp = c[i].ext->merges[index++];
						lilswap(&tmp.u1, 4);
						f->write(&tmp, sizeof(mergeinfo));
					}
				}
			}
			if(c[i].children) savec(c[i].children, f, nolms);
		}
	}
}

cube *loadchildren(stream *f);

void loadc(stream *f, cube &c)
{
	bool haschildren = false;
	int octsav = f->getchar();
	switch(octsav&0x7)
	{
		case OCTSAV_CHILDREN:
			c.children = loadchildren(f);
			return;

		case OCTSAV_LODCUBE: haschildren = true;	break;
		case OCTSAV_EMPTY:  emptyfaces(c);		  break;
		case OCTSAV_SOLID:  solidfaces(c);		  break;
		case OCTSAV_NORMAL: f->read(c.edges, 12); break;

		default:
			fatal("garbage in map");
	}
	loopi(6) c.texture[i] = hdr.version<14 ? f->getchar() : f->getlil<ushort>();
	if(hdr.version < 7) loopi(3) f->getchar(); //f->read(c.colour, 3);
	else
	{
		uchar mask = f->getchar();
        if(mask & 0x80)
        {
            int mat = f->getchar();
            if((maptype == MAP_OCTA && hdr.version <= 26) || (maptype == MAP_BFGZ && hdr.version <= 30))
            {
                static uchar matconv[] = { MAT_AIR, MAT_WATER, MAT_CLIP, MAT_GLASS|MAT_CLIP, MAT_NOCLIP, MAT_LAVA|MAT_DEATH, MAT_AICLIP, MAT_DEATH };
                mat = size_t(mat) < sizeof(matconv)/sizeof(matconv[0]) ? matconv[mat] : MAT_AIR;
            }
            ext(c).material = mat;
        }
		if(mask & 0x3F)
		{
			uchar lit = 0, bright = 0;
            static surfaceinfo surfaces[12];
            memset(surfaces, 0, 6*sizeof(surfaceinfo));
			if(mask & 0x40) newnormals(c);
            int numsurfs = 6;
            loopi(numsurfs)
			{
                if(i >= 6 || mask & (1 << i))
				{
                    f->read(&surfaces[i], sizeof(surfaceinfo));
                    lilswap(&surfaces[i].x, 2);
                    if(hdr.version < 10) ++surfaces[i].lmid;
                    if(hdr.version < 18)
                    {
                        if(surfaces[i].lmid >= LMID_AMBIENT1) ++surfaces[i].lmid;
                        if(surfaces[i].lmid >= LMID_BRIGHT1) ++surfaces[i].lmid;
                    }
                    if(hdr.version < 19)
                    {
                        if(surfaces[i].lmid >= LMID_DARK) surfaces[i].lmid += 2;
                    }
                    if(i < 6)
                    {
                        if(mask & 0x40) f->read(&c.ext->normals[i], sizeof(surfacenormals));
                        if(surfaces[i].layer != LAYER_TOP) lit |= 1 << i;
                        else if(surfaces[i].lmid == LMID_BRIGHT) bright |= 1 << i;
                        else if(surfaces[i].lmid != LMID_AMBIENT) lit |= 1 << i;
                        if(surfaces[i].layer&LAYER_BLEND) numsurfs++;
                    }
				}
                else surfaces[i].lmid = LMID_AMBIENT;
            }
            if(lit) newsurfaces(c, surfaces, numsurfs);
            else if(bright) brightencube(c);
		}
		if(hdr.version >= 20)
		{
			if(octsav&0x80)
			{
				int merged = f->getchar();
				ext(c).merged = merged&0x3F;
				if(merged&0x80)
				{
					c.ext->mergeorigin = f->getchar();
					int nummerges = 0;
					loopi(6) if(c.ext->mergeorigin&(1<<i)) nummerges++;
					if(nummerges)
					{
						c.ext->merges = new mergeinfo[nummerges];
						loopi(nummerges)
						{
                            mergeinfo *m = &c.ext->merges[i];
                            f->read(m, sizeof(mergeinfo));
                            lilswap(&m->u1, 4);
                            if(hdr.version <= 25)
                            {
                                int uorigin = m->u1 & 0xE000, vorigin = m->v1 & 0xE000;
                                m->u1 = (m->u1 - uorigin) << 2;
                                m->u2 = (m->u2 - uorigin) << 2;
                                m->v1 = (m->v1 - vorigin) << 2;
                                m->v2 = (m->v2 - vorigin) << 2;
                            }
						}
					}
				}
			}
		}
	}
	c.children = (haschildren ? loadchildren(f) : NULL);
}

cube *loadchildren(stream *f)
{
	cube *c = newcubes();
	loopi(8) loadc(f, c[i]);
	// TODO: remip c from children here
	return c;
}

void saveslotconfig(stream *h, Slot &s, int index)
{
    if(index >= 0)
    {
        if(s.shader)
        {
            h->printf("setshader %s\n", s.shader->name);
        }
        loopvj(s.params)
        {
            h->printf("set%sparam", s.params[j].type == SHPARAM_LOOKUP ? "shader" : (s.params[j].type == SHPARAM_UNIFORM ? "uniform" : (s.params[j].type == SHPARAM_PIXEL ? "pixel" : "vertex")));
            if(s.params[j].type == SHPARAM_LOOKUP || s.params[j].type == SHPARAM_UNIFORM) h->printf(" \"%s\"", s.params[j].name);
            else h->printf(" %d", s.params[j].index);
            loopk(4) h->printf(" %f", s.params[j].val[k]);
            h->printf("\n");
        }
    }
    loopvj(s.sts)
    {
        h->printf("texture");
        if(index >= 0) h->printf(" %s", findtexturename(s.sts[j].type));
        else if(!j) h->printf(" %s", findmaterialname(-index));
        else h->printf(" 1");
        h->printf(" \"%s\"", s.sts[j].lname);
        if(!j)
        {
            h->printf(" %d %d %d %f",
                s.rotation, s.xoffset, s.yoffset, s.scale);
            if(index >= 0) h->printf(" // %d", index);
        }
        h->printf("\n");
    }
    if(index >= 0)
    {
        if(s.scrollS != 0.f || s.scrollT != 0.f)
            h->printf("texscroll %f %f\n", s.scrollS * 1000.0f, s.scrollT * 1000.0f);
        if(s.layer != 0)
        {
            if(s.layermaskname) h->printf("texlayer %d \"%s\" %d %f\n", s.layer, s.layermaskname, s.layermaskmode, s.layermaskscale);
            else h->printf("texlayer %d\n", s.layer);
        }
        if(s.autograss) h->printf("autograss \"%s\"\n", s.autograss);
    }
    h->printf("\n");
}

static int sortidents(ident **x, ident **y) // not sure if there's a way to extern this when it needs to be static? --quin
{
    return strcmp((*x)->name, (*y)->name);
}

void save_config(char *mname)
{
	backup(mname, ".cfg", hdr.revision);
	defformatstring(fname)("%s.cfg", mname);
	stream *h = openfile(fname, "w");
	if(!h) { conoutf("\frcould not write config to %s", fname); return; }

	// config
	h->printf("// %s by %s (%s)\n// Config generated by Blood Frontier\n\n", *maptitle ? maptitle : "Untitled", *mapauthor ? mapauthor : "Unknown", mapname);

	int vars = 0;
	h->printf("// Variables stored in map file, may be uncommented here, or changed from editmode.\n");
    vector<ident *> ids;
    enumerate(*idents, ident, id, ids.add(&id));
    ids.sort(sortidents);
    loopv(ids)
    {
        ident &id = *ids[i];
		if(id.flags&IDF_WORLD) switch(id.type)
		{
            case ID_VAR: h->printf((id.flags&IDF_HEX ? (id.maxval==0xFFFFFF ? "// %s 0x%.6X\n" : "// %s 0x%X\n") : "// %s %d\n"), id.name, *id.storage.i); vars++;break;
            case ID_FVAR: h->printf("// %s %s\n", id.name, floatstr(*id.storage.f)); vars++; break;
            case ID_SVAR: h->printf("// %s ", id.name); writeescapedstring(h, *id.storage.s); h->putchar('\n'); vars++; break;
			default: break;
		}
	}
	if(vars) h->printf("\n");
	if(verbose >= 2) conoutf("\fdwrote %d variable values", vars);

	int aliases = 0;
    loopv(ids)
    {
        ident &id = *ids[i];
		if(id.type == ID_ALIAS && id.flags&IDF_WORLD && strlen(id.name) && strlen(id.action))
		{
			aliases++;
            h->printf("\"%s\" = [%s]\n", id.name, id.action);
		}
	}
	if(aliases) h->printf("\n");
	if(verbose >= 2) conoutf("\fdsaved %d aliases", aliases);

	// texture slots
	loopi(MAT_EDIT)
	{
		if(verbose) progress(float(i)/float(MAT_EDIT), "saving material slots...");

		if(i == MAT_WATER || i == MAT_LAVA)
		{
			saveslotconfig(h, materialslots[i], -i);
		}
	}
	if(verbose) conoutf("\fdsaved %d material slots", MAT_EDIT);

	loopv(slots)
	{
		if(verbose) progress(float(i)/float(slots.length()), "saving texture slots...");
		saveslotconfig(h, slots[i], i);
	}
	if(verbose) conoutf("\fdsaved %d texture slots", slots.length());

	loopv(mapmodels)
	{
		if(verbose) progress(float(i)/float(mapmodels.length()), "saving mapmodel slots...");
		h->printf("mmodel \"%s\"\n", mapmodels[i].name);
	}
	if(mapmodels.length()) h->printf("\n");
	if(verbose) conoutf("\fdsaved %d mapmodel slots", mapmodels.length());

	loopv(mapsounds)
	{
		if(verbose) progress(float(i)/float(mapsounds.length()), "saving mapsound slots...");
		h->printf("mapsound \"%s\" %d \"%s\" %d %d\n", mapsounds[i].sample->name, mapsounds[i].vol, findmaterialname(mapsounds[i].material), mapsounds[i].maxrad, mapsounds[i].minrad);
	}
	if(mapsounds.length()) h->printf("\n");
	if(verbose) conoutf("\fdsaved %d mapsound slots", mapsounds.length());

	delete h;
	if(verbose) conoutf("\fdsaved config %s", fname);
}
ICOMMAND(savemapconfig, "s", (char *mname), save_config(*mname ? mname : mapname));

VARFP(mapshotsize, 0, 256, INT_MAX-1, mapshotsize -= mapshotsize%2);

void save_mapshot(char *mname)
{
	backup(mname, ifmtexts[imageformat], hdr.revision);

    GLuint tex;
	glGenTextures(1, &tex);
	glViewport(0, 0, mapshotsize, mapshotsize);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
    ImageData image(mapshotsize, mapshotsize, 3);
	memset(image.data, 0, 3*mapshotsize*mapshotsize);
	glFrontFace(GL_CCW);
	drawcubemap(mapshotsize, 1, camera1->o, camera1->yaw, camera1->pitch, false, false, false);
	glReadPixels(0, 0, mapshotsize, mapshotsize, GL_RGB, GL_UNSIGNED_BYTE, image.data);

	saveimage(mname, image, imageformat, compresslevel, true);

	glDeleteTextures(1, &tex);
    glFrontFace(GL_CCW);
	glViewport(0, 0, screen->w, screen->h);
}
ICOMMAND(savemapshot, "s", (char *mname), save_mapshot(*mname ? mname : mapname));

VARP(autosaveconfig, 0, 1, 1);
VARP(autosavemapshot, 0, 1, 1);

void save_world(const char *mname, bool nodata, bool forcesave)
{
	int savingstart = SDL_GetTicks();

	setnames(mname, MAP_BFGZ);

	backup(mapname, mapexts[MAP_BFGZ].name, hdr.revision);
	stream *f = opengzfile(mapfile, "wb");
	if(!f) { conoutf("\frerror saving %s to %s: file error", mapname, mapfile); return; }

	if(autosavemapshot || forcesave) save_mapshot(mapname);
	if(autosaveconfig || forcesave) save_config(mapname);

	progress(0, "saving map..");
	strncpy(hdr.head, "BFGZ", 4);
	hdr.version = MAPVERSION;
	hdr.headersize = sizeof(bfgz);
	hdr.gamever = server::gamever();
	hdr.numents = 0;
	hdr.revision++;
	strncpy(hdr.gameid, server::gameid(), 4);

	const vector<extentity *> &ents = entities::getents();
	loopv(ents)
	{
		if(ents[i]->type!=ET_EMPTY)
		{
			hdr.numents++;
		}
	}

    hdr.numpvs = nodata ? 0 : getnumviewcells();
    hdr.blendmap = nodata ? 0 : shouldsaveblendmap();
	hdr.lightmaps = nodata ? 0 : lightmaps.length();

	bfgz tmp = hdr;
    lilswap(&tmp.version, 7);
	f->write(&tmp, sizeof(bfgz));

	// world variables
	int numvars = 0, vars = 0;
	enumerate(*idents, ident, id, {
		if((id.type == ID_VAR || id.type == ID_FVAR || id.type == ID_SVAR) && id.flags&IDF_WORLD && strlen(id.name)) numvars++;
	});
	f->putlil<int>(numvars);
	enumerate(*idents, ident, id, {
		if((id.type == ID_VAR || id.type == ID_FVAR || id.type == ID_SVAR) && id.flags&IDF_WORLD && strlen(id.name))
		{
			vars++;
			if(verbose) progress(float(vars)/float(numvars), "saving world variables...");
			f->putlil<int>((int)strlen(id.name));
			f->write(id.name, (int)strlen(id.name)+1);
			f->putlil<int>(id.type);
			switch(id.type)
			{
				case ID_VAR:
					f->putlil<int>(*id.storage.i);
					break;
				case ID_FVAR:
					f->putlil<float>(*id.storage.f);
					break;
				case ID_SVAR:
					f->putlil<int>((int)strlen(*id.storage.s));
					f->write(*id.storage.s, (int)strlen(*id.storage.s)+1);
					break;
				default: break;
			}
		}
	});

	if(verbose) conoutf("\fdsaved %d variables", vars);

	// texture slots
	f->putlil<ushort>(texmru.length());
	loopv(texmru) f->putlil<ushort>(texmru[i]);

	// entities
	int count = 0;
	loopv(ents) // extended
	{
		if(verbose) progress(float(i)/float(ents.length()), "saving entities...");
		if(ents[i]->type!=ET_EMPTY)
		{
			entity tmp = *ents[i];
            lilswap(&tmp.o.x, 3);
            lilswap(&tmp.attr, ENTATTRS);
			f->write(&tmp, sizeof(entity));
			entities::writeent(f, i, *ents[i]);
			extentity &e = (extentity &)*ents[i];
			if(entities::maylink(e.type))
			{
				vector<int> links;
				int n = 0;
				loopvk(ents)
				{
					extentity &f = (extentity &)*ents[k];
					if(f.type != ET_EMPTY)
					{
						if(entities::maylink(f.type) && e.links.find(k) >= 0)
							links.add(n); // align to indices
						n++;
					}
				}

				f->putlil<int>(links.length());
				loopvj(links) f->putlil<int>(links[j]); // aligned index
				if(verbose >= 2) conoutf("\fdentity %s (%d) saved %d links", entities::findname(e.type), i, links.length());
			}
			count++;
		}
	}
	if(verbose) conoutf("\fdsaved %d entities", count);

	savec(worldroot, f, nodata);
    if(!nodata)
    {
        loopv(lightmaps)
        {
            if(verbose) progress(float(i)/float(lightmaps.length()), "saving lightmaps...");
            LightMap &lm = lightmaps[i];
            f->putchar(lm.type | (lm.unlitx>=0 ? 0x80 : 0));
            if(lm.unlitx>=0)
            {
                f->putlil<ushort>(ushort(lm.unlitx));
                f->putlil<ushort>(ushort(lm.unlity));
            }
            f->write(lm.data, lm.bpp*LM_PACKW*LM_PACKH);
        }
        if(verbose) conoutf("\fdsaved %d lightmaps", lightmaps.length());
        if(getnumviewcells()>0)
        {
            if(verbose) progress(0, "saving PVS...");
            savepvs(f);
            if(verbose) conoutf("\fdsaved %d PVS view cells", getnumviewcells());
        }
        if(shouldsaveblendmap())
        {
            if(verbose) progress(0, "saving blendmap...");
            saveblendmap(f);
            if(verbose) conoutf("\fdsaved blendmap");
        }
    }

	progress(0, "saving world...");
	game::saveworld(f);
	delete f;

	conoutf("\fdsaved map %s v.%d:%d (r%d) in %.1f secs", mapname, hdr.version, hdr.gamever, hdr.revision, (SDL_GetTicks()-savingstart)/1000.0f);
}

ICOMMAND(savemap, "s", (char *mname), save_world(*mname ? mname : mapname));
ICOMMAND(savecurrentmap, "", (), save_world(mapname));

void swapXZ(cube *c)
{
	loopi(8)
	{
		swap(c[i].faces[0],   c[i].faces[2]);
		swap(c[i].texture[0], c[i].texture[4]);
		swap(c[i].texture[1], c[i].texture[5]);
		if(c[i].ext && c[i].ext->surfaces)
		{
			swap(c[i].ext->surfaces[0], c[i].ext->surfaces[4]);
			swap(c[i].ext->surfaces[1], c[i].ext->surfaces[5]);
		}
		if(c[i].children) swapXZ(c[i].children);
	}
}

static void fixoversizedcubes(cube *c, int size)
{
    if(size <= VVEC_INT_MASK+1) return;
    loopi(8)
    {
        if(!c[i].children) subdividecube(c[i], true, false);
        fixoversizedcubes(c[i].children, size>>1);
    }
}

static void sanevars()
{
	setvar("fullbright", 0, false);
	setvar("blankgeom", 0, false);
}

bool load_world(const char *mname, bool temp)		// still supports all map formats that have existed since the earliest cube betas!
{
	int loadingstart = SDL_GetTicks();
	loop(tempfile, 2) if(tempfile || temp)
	{
		loop(format, MAP_MAX)
		{
			setnames(mname, format);
			if(!tempfile) loopk(2)
			{
				defformatstring(s)("temp/%s", k ? mapfile : mapname);
				copystring(k ? mapfile : mapname, s);
			}

			stream *f = opengzfile(mapfile, "rb");
			if(!f) continue;

			bool samegame = true;
			int eif = 0;
			bfgz newhdr;
			if(f->read(&newhdr, sizeof(binary))!=(int)sizeof(binary))
			{
				conoutf("\frerror loading %s: malformatted universal header", mapname);
				delete f;
				return false;
			}
			lilswap(&newhdr.version, 2);

			clearworldvars();
			if(strncmp(newhdr.head, "BFGZ", 4) == 0)
			{
				// this check removed below due to breakage: (size_t)newhdr.headersize > sizeof(chdr) || f->read(&chdr.worldsize, newhdr.headersize-sizeof(binary))!=newhdr.headersize-(int)sizeof(binary)
				#define BFGZCOMPAT(ver) \
					bfgzcompat##ver chdr; \
					memcpy(&chdr, &newhdr, sizeof(binary)); \
					if(f->read(&chdr.worldsize, sizeof(chdr)-sizeof(binary))!=int(sizeof(chdr)-sizeof(binary))) \
					{ \
						conoutf("\frerror loading %s: malformatted bfgz v%d[%d] header", mapname, newhdr.version, ver); \
						delete f; \
						return false; \
					}
				if(newhdr.version <= 25)
				{
					BFGZCOMPAT(25);
					lilswap(&chdr.worldsize, 5);
					memcpy(&newhdr.worldsize, &chdr.worldsize, sizeof(int)*2);
					newhdr.numpvs = 0;
					newhdr.lightmaps = chdr.lightmaps;
					newhdr.blendmap = 0;
					memcpy(&newhdr.gamever, &chdr.gamever, sizeof(int)*2);
                    memcpy(&newhdr.gameid, &chdr.gameid, 4);
					setsvar("maptitle", chdr.maptitle, true);
				}
				else if(newhdr.version <= 32)
				{
					BFGZCOMPAT(32);
					lilswap(&chdr.worldsize, 6);
					memcpy(&newhdr.worldsize, &chdr.worldsize, sizeof(int)*4);
					newhdr.blendmap = 0;
					memcpy(&newhdr.gamever, &chdr.gamever, sizeof(int)*2);
                    memcpy(&newhdr.gameid, &chdr.gameid, 4);
					setsvar("maptitle", chdr.maptitle, true);
				}
				else if(newhdr.version <= 33)
				{
					BFGZCOMPAT(33);
					lilswap(&chdr.worldsize, 7);
					memcpy(&newhdr.worldsize, &chdr.worldsize, sizeof(int)*7);
					memcpy(&newhdr.gameid, &chdr.gameid, 4);
					setsvar("maptitle", chdr.maptitle, true);
				}
				else
				{
					if((size_t)newhdr.headersize > sizeof(newhdr) || f->read(&newhdr.worldsize, newhdr.headersize-sizeof(binary))!=newhdr.headersize-(int)sizeof(binary))
					{
						conoutf("\frerror loading %s: malformatted bfgz v%d header", mapname, newhdr.version);
						delete f;
						return false;
					}
					lilswap(&newhdr.worldsize, 7);
				}

				if(newhdr.version > MAPVERSION)
				{
					conoutf("\frerror loading %s: requires a newer version of Blood Frontier", mapname);
					delete f;
					return false;
				}

                resetmap(false);
				hdr = newhdr;
				progress(0, "please wait...");
				maptype = MAP_BFGZ;

				if(hdr.version <= 24) copystring(hdr.gameid, "bfa", 4); // all previous maps were bfa-fps
				if(verbose) conoutf("\fdloading v%d map from %s game v%d", hdr.version, hdr.gameid, hdr.gamever);

				if(hdr.version >= 25 || (hdr.version == 24 && hdr.gamever >= 44))
				{
					int numvars = hdr.version >= 25 ? f->getlil<int>() : f->getchar(), vars = 0;
					overrideidents = worldidents = true;
					persistidents = false;
					progress(0, "loading variables...");
					loopi(numvars)
					{
						if(verbose) progress(float(i)/float(numvars), "loading variables...");
						int len = hdr.version >= 25 ? f->getlil<int>() : f->getchar();
						if(len)
						{
							string name;
							f->read(name, len+1);
							if(hdr.version <= 34 && !strcmp(name, "cloudcolour")) copystring(name, "cloudlayercolour");
							ident *id = idents->access(name);
							bool proceed = true;
							int type = hdr.version >= 28 ? f->getlil<int>()+(hdr.version >= 29 ? 0 : 1) : (id ? id->type : ID_VAR);
							if(!id || type != id->type)
							{
								if(id && hdr.version <= 28 && id->type == ID_FVAR && type == ID_VAR)
									type = ID_FVAR;
								else proceed = false;
							}
							if(!id || !(id->flags&IDF_WORLD)) proceed = false;

							switch(type)
							{
								case ID_VAR:
								{
									int val = hdr.version >= 25 ? f->getlil<int>() : f->getchar();
									if(proceed)
									{
										if(val > id->maxval) val = id->maxval;
										else if(val < id->minval) val = id->minval;
										setvar(name, val, true);
									}
									break;
								}
								case ID_FVAR:
								{
									float val = hdr.version >= 29 ? f->getlil<float>() : float(f->getlil<int>())/100.f;
									if(proceed)
									{
										if(val > id->maxvalf) val = id->maxvalf;
										else if(val < id->minvalf) val = id->minvalf;
										setfvar(name, val, true);
									}
									break;
								}
								case ID_SVAR:
								{
									int slen = f->getlil<int>();
									string val;
									f->read(val, slen+1);
									if(proceed && slen) setsvar(name, val, true);
									break;
								}
								default:
								{
									if(hdr.version <= 27)
									{
										if(hdr.version >= 25) f->getlil<int>();
										else f->getchar();
									}
									proceed = false;
									break;
								}
							}
							if(!proceed)
							{
								if(verbose) conoutf("\frWARNING: ignoring variable %s stored in map", name);
							}
							else vars++;
						}
					}
					persistidents = true;
					overrideidents = worldidents = false;
					if(verbose) conoutf("\fdloaded %d variables", vars);
				}
				sanevars();

				if(!server::canload(hdr.gameid))
				{
					if(verbose) conoutf("\frWARNING: loading map from %s game type in %s, ignoring game specific data", hdr.gameid, server::gameid());
					samegame = false;
				}
			}
			else if(strncmp(newhdr.head, "OCTA", 4) == 0)
			{
				octa ohdr;
				memcpy(&ohdr, &newhdr, sizeof(binary));

				// this check removed due to breakage: (size_t)ohdr.headersize > sizeof(chdr) || f->read(&chdr.worldsize, ohdr.headersize-sizeof(binary))!=ohdr.headersize-(int)sizeof(binary)
				#define OCTACOMPAT(ver) \
					octacompat##ver chdr; \
					memcpy(&chdr, &ohdr, sizeof(binary)); \
					if(f->read(&chdr.worldsize, sizeof(chdr)-sizeof(binary))!=int(sizeof(chdr)-sizeof(binary))) \
					{ \
						conoutf("\frerror loading %s: malformatted octa v%d[%d] header", mapname, ver, ohdr.version); \
						delete f; \
						return false; \
					}

				#define OCTAVARS \
					if(chdr.lightprecision) setvar("lightprecision", chdr.lightprecision); \
					if(chdr.lighterror) setvar("lighterror", chdr.lighterror); \
					if(chdr.bumperror) setvar("bumperror", chdr.bumperror); \
					setvar("lightlod", chdr.lightlod); \
					if(chdr.ambient) setvar("ambient", chdr.ambient); \
					setvar("skylight", (int(chdr.skylight[0])<<16) | (int(chdr.skylight[1])<<8) | int(chdr.skylight[2])); \
					setvar("watercolour", (int(chdr.watercolour[0])<<16) | (int(chdr.watercolour[1])<<8) | int(chdr.watercolour[2]), true); \
					setvar("waterfallcolour", (int(chdr.waterfallcolour[0])<<16) | (int(chdr.waterfallcolour[1])<<8) | int(chdr.waterfallcolour[2])); \
					setvar("lavacolour", (int(chdr.lavacolour[0])<<16) | (int(chdr.lavacolour[1])<<8) | int(chdr.lavacolour[2])); \
					setvar("fullbright", 0, true); \
					if(chdr.lerpsubdivsize || chdr.lerpangle) setvar("lerpangle", chdr.lerpangle); \
					if(chdr.lerpsubdivsize) \
					{ \
						setvar("lerpsubdiv", chdr.lerpsubdiv); \
						setvar("lerpsubdivsize", chdr.lerpsubdivsize); \
					} \
					setsvar("maptitle", chdr.maptitle); \
					ohdr.numvars = 0;

				if(ohdr.version <= 25)
				{
					OCTACOMPAT(25);
					lilswap(&chdr.worldsize, 7);
					memcpy(&ohdr.worldsize, &chdr.worldsize, sizeof(int)*2);
					ohdr.numpvs = 0;
					memcpy(&ohdr.lightmaps, &chdr.lightmaps, sizeof(int)*3);
					OCTAVARS;
				}
				else if(ohdr.version <= 28)
				{
					OCTACOMPAT(28);
					lilswap(&chdr.worldsize, 7);
					memcpy(&ohdr.worldsize, &chdr.worldsize, sizeof(int)*6);
					OCTAVARS;
					ohdr.blendmap = chdr.blendmap;
				}
				else
				{
					if(f->read(&ohdr.worldsize, sizeof(octa)-sizeof(binary))!=sizeof(octa)-(int)sizeof(binary))
					{
						conoutf("\frerror loading %s: malformatted octa v%d header", mapname, ohdr.version);
						delete f;
						return false;
					}
					lilswap(&ohdr.worldsize, 6);
				}

				if(ohdr.version > OCTAVERSION)
				{
					conoutf("\frerror loading %s: requires a newer version of Cube 2 support", mapname);
					delete f;
					return false;
				}

                resetmap(false);
				hdr = newhdr;
				progress(0, "please wait...");
				maptype = MAP_OCTA;

				strncpy(hdr.head, ohdr.head, 4);
				hdr.gamever = 0; // sauer has no gamever
				hdr.worldsize = ohdr.worldsize;
				if(hdr.worldsize > 1<<18) hdr.worldsize = 1<<18;
				hdr.numents = ohdr.numents;
				hdr.numpvs = ohdr.numpvs;
				hdr.lightmaps = ohdr.lightmaps;
				hdr.blendmap = ohdr.blendmap;
				hdr.revision = 1;

				if(ohdr.version >= 29) loopi(ohdr.numvars)
				{
					int type = f->getchar(), ilen = f->getlil<ushort>();
					string name;
					f->read(name, min(ilen, OCTASTRLEN-1));
					name[min(ilen, OCTASTRLEN-1)] = '\0';
					if(ilen >= OCTASTRLEN) f->seek(ilen - (OCTASTRLEN-1), SEEK_CUR);
					if(!strcmp(name, "cloudalpha")) copystring(name, "cloudblend");
					if(!strcmp(name, "cloudcolour")) copystring(name, "cloudlayercolour");
					ident *id = getident(name);
					bool exists = id && id->type == type;
					switch(type)
					{
						case ID_VAR:
						{
							int val = f->getlil<int>();
							if(exists && id->minval <= id->maxval) setvar(name, val);
							break;
						}

						case ID_FVAR:
						{
							float val = f->getlil<float>();
							if(exists && id->minvalf <= id->maxvalf) setfvar(name, val);
							break;
						}

						case ID_SVAR:
						{
							int slen = f->getlil<ushort>();
							string val;
							f->read(val, min(slen, OCTASTRLEN-1));
							val[min(slen, OCTASTRLEN-1)] = '\0';
							if(slen >= OCTASTRLEN) f->seek(slen - (OCTASTRLEN-1), SEEK_CUR);
							if(exists) setsvar(name, val);
							break;
						}
					}
				}
				sanevars();

				string gameid;
				if(hdr.version >= 16)
				{
					int len = f->getchar();
					f->read(gameid, len+1);
				}
				else copystring(gameid, "fps");
				strncpy(hdr.gameid, gameid, 4);

				if(!server::canload(hdr.gameid))
				{
					if(verbose) conoutf("\frWARNING: loading OCTA v%d map from %s game, ignoring game specific data", hdr.version, hdr.gameid);
					samegame = false;
				}
				else if(verbose) conoutf("\fdloading OCTA v%d map from %s game", hdr.version, hdr.gameid);

				if(hdr.version>=16)
				{
					eif = f->getlil<ushort>();
					int extrasize = f->getlil<ushort>();
					loopj(extrasize) f->getchar();
				}

				if(hdr.version<25) hdr.numpvs = 0;
				if(hdr.version<28) hdr.blendmap = 0;
			}
			else
			{
				delete f;
				continue;
			}

			progress(0, "clearing world...");

			texmru.setsize(0);
			if(hdr.version<14)
			{
				uchar oldtl[256];
				f->read(oldtl, sizeof(oldtl));
				loopi(256) texmru.add(oldtl[i]);
			}
			else
			{
				ushort nummru = f->getlil<ushort>();
				loopi(nummru) texmru.add(f->getlil<ushort>());
			}

			freeocta(worldroot);
			worldroot = NULL;

			progress(0, "loading entities...");

			vector<extentity *> &ents = entities::getents();
			loopi(hdr.numents)
			{
				if(verbose) progress(float(i)/float(hdr.numents), "loading entities...");
				extentity &e = *entities::newent();
				ents.add(&e);
				f->read(&e, sizeof(entity));
				lilswap(&e.o.x, 3);
				lilswap(&e.attr, ENTATTRS);
				if((maptype == MAP_OCTA && hdr.version <= 27) || (maptype == MAP_BFGZ && hdr.version <= 31))
					e.attr[4] = 0; // init ever-present attr5
				if(maptype == MAP_OCTA) loopj(eif) f->getchar();
				// sauerbraten version increments
				if(hdr.version <= 10 && e.type >= 7) e.type++;
				if(hdr.version <= 12 && e.type >= 8) e.type++;
				if(hdr.version <= 14 && e.type >= ET_MAPMODEL && e.type <= 16)
				{
					if(e.type == 16) e.type = ET_MAPMODEL;
					else e.type++;
				}
				if(hdr.version <= 20 && e.type >= ET_ENVMAP) e.type++;
				if(hdr.version <= 21 && e.type >= ET_PARTICLES) e.type++;
				if(hdr.version <= 22 && e.type >= ET_SOUND) e.type++;
				if(hdr.version <= 23 && e.type >= ET_LIGHTFX) e.type++;
				if(!samegame && (e.type>=ET_GAMESPECIFIC || hdr.version<=14))
				{
					entities::deleteent(ents.pop());
					continue;
				}
				entities::readent(f, maptype, hdr.version, hdr.gameid, hdr.gamever, i, e);
				if(maptype == MAP_BFGZ && entities::maylink(hdr.gamever <= 49 && e.type >= 10 ? e.type-1 : e.type, hdr.gamever))
				{
					int links = f->getlil<int>();
					loopk(links)
					{
						int ln = f->getlil<int>();
						e.links.add(ln);
					}
					if(verbose >= 2) conoutf("\fdentity %s (%d) loaded %d link(s)", entities::findname(e.type), i, links);
				}
				if(maptype == MAP_OCTA && e.type == ET_PARTICLES && e.attr[0] >= 11)
				{
					if(e.attr[0] <= 12) e.attr[0] += 3;
					else e.attr[0] = 0; // bork it up
				}
				if(hdr.version <= 14 && e.type == ET_MAPMODEL)
				{
					e.o.z += e.attr[2];
					if(e.attr[3] && verbose) conoutf("\frWARNING: mapmodel ent (index %d) uses texture slot %d", i, e.attr[3]);
					e.attr[2] = e.attr[3] = 0;
				}
				if(hdr.version <= 31 && e.type == ET_MAPMODEL)
				{
					int angle = e.attr[0];
					e.attr[0] = e.attr[1];
					e.attr[1] = angle;
					e.attr[2] = e.attr[3] = e.attr[4] = 0;
				}
				if(verbose && !insideworld(e.o) && e.type != ET_LIGHT && e.type != ET_LIGHTFX)
					conoutf("\frWARNING: ent outside of world: enttype[%s] index %d (%f, %f, %f)", entities::findname(e.type), i, e.o.x, e.o.y, e.o.z);
			}
			if(verbose) conoutf("\fdloaded %d entities", hdr.numents);

			progress(0, "loading octree...");
			worldroot = loadchildren(f);

			if(hdr.version <= 11)
				swapXZ(worldroot);

			if(hdr.version <= 8)
				converttovectorworld();

			if(hdr.worldsize > VVEC_INT_MASK+1 && hdr.version <= 25)
				fixoversizedcubes(worldroot, hdr.worldsize>>1);

			progress(0, "validating...");
			validatec(worldroot, hdr.worldsize>>1);

			worldscale = 0;
			while(1<<worldscale < hdr.worldsize) worldscale++;

			if(hdr.version >= 7) loopi(hdr.lightmaps)
			{
				if(verbose) progress(i/(float)hdr.lightmaps, "loading lightmaps...");
				LightMap &lm = lightmaps.add();
				if(hdr.version >= 17)
				{
					int type = f->getchar();
					lm.type = type&0x7F;
					if(hdr.version >= 20 && type&0x80)
					{
						lm.unlitx = f->getlil<ushort>();
						lm.unlity = f->getlil<ushort>();
					}
				}
				if(lm.type&LM_ALPHA && (lm.type&LM_TYPE)!=LM_BUMPMAP1) lm.bpp = 4;
				lm.data = new uchar[lm.bpp*LM_PACKW*LM_PACKH];
				f->read(lm.data, lm.bpp * LM_PACKW * LM_PACKH);
				lm.finalize();
			}

			if(hdr.numpvs > 0) loadpvs(f);
			if(hdr.blendmap) loadblendmap(f);

			if(verbose) conoutf("\fdloaded %d lightmaps", hdr.lightmaps);

			progress(0, "loading world...");
			game::loadworld(f, maptype);
			entities::initents(f, maptype, hdr.version, hdr.gameid, hdr.gamever);

			overrideidents = worldidents = true;
			persistidents = false;
			defformatstring(cfgname)("%s.cfg", mapname);
			if(maptype == MAP_OCTA)
			{
				execfile("octa.cfg"); // for use with -pSAUER_DIR
				execfile(cfgname);
			}
			else if(!execfile(cfgname, false)) execfile("map.cfg");
			if(maptype == MAP_OCTA || (maptype == MAP_BFGZ && hdr.version <= 34))
			{
				extern float cloudblend;
				setfvar("cloudlayerblend", cloudblend, true);
			}

			persistidents = true;
			overrideidents = worldidents = false;

			vector<int> mapmodels;
			loopv(ents)
			{
				extentity &e = *ents[i];

				if((maptype == MAP_OCTA || (maptype == MAP_BFGZ && hdr.version <= 29)) && ents[i]->type == ET_LIGHTFX)
				{
					int closest = -1;
					float closedist = 1e10f;
					loopvk(ents) if(ents[k]->type == ET_LIGHT)
					{
						extentity &a = *ents[k];
						float dist = e.o.dist(a.o);
						if(dist < closedist)
						{
							closest = k;
							closedist = dist;
						}
					}
					if(ents.inrange(closest) && closedist <= 100)
					{
						extentity &a = *ents[closest];
						if(e.links.find(closest) < 0) e.links.add(closest);
						if(a.links.find(i) < 0) a.links.add(i);
						if(verbose) conoutf("\frWARNING: auto linked spotlight %d to light %d", i, closest);
					}
				}
				if(e.type == ET_MAPMODEL && e.attr[0] >= 0)
				{
					if(mapmodels.find(e.attr[0]) < 0) mapmodels.add(e.attr[0]);
				}
			}

			loopv(mapmodels)
			{
				loadprogress = float(i+1)/mapmodels.length();
				int mmindex = mapmodels[i];
				mapmodelinfo &mmi = getmminfo(mmindex);
				if(!&mmi) conoutf("\frcould not find map model: %d", mmindex);
				else if(!loadmodel(NULL, mmindex, true))
					conoutf("\frcould not load model: %s", mmi.name);
			}
			loadprogress = 0;

			delete f;
			conoutf("\fdloaded map %s v.%d:%d (r%d) in %.1f secs", mapname, hdr.version, hdr.gamever, hdr.revision, (SDL_GetTicks()-loadingstart)/1000.0f);

			if((maptype == MAP_OCTA && hdr.version <= 25) || (maptype == MAP_BFGZ && hdr.version <= 26))
				fixlightmapnormals();

			entitiesinoctanodes();
			initlights();
			allchanged(true);

			progress(0, "starting world...");
			game::startmap(mapname);
			return true;
		}
	}
	conoutf("\frunable to load %s", mname);
	return false;
}

void writeobj(char *name)
{
    defformatstring(fname)("%s.obj", name);
    stream *f = openfile(fname, "w");
    if(!f) return;
    f->printf("# obj file of sauerbraten level\n");
    extern vector<vtxarray *> valist;
    loopv(valist)
    {
        vtxarray &va = *valist[i];
        ushort *edata = NULL;
        uchar *vdata = NULL;
        if(!readva(&va, edata, vdata)) continue;
        int vtxsize = VTXSIZE;
        uchar *vert = vdata;
        loopj(va.verts)
        {
            vec v;
            if(floatvtx) (v = *(vec *)vert).div(1<<VVEC_FRAC);
            else v = ((vvec *)vert)->tovec(va.o).add(0x8000>>VVEC_FRAC);
            if(v.x != floor(v.x)) f->printf("v %.3f ", v.x); else f->printf("v %d ", int(v.x));
            if(v.y != floor(v.y)) f->printf("%.3f ", v.y); else f->printf("%d ", int(v.y));
            if(v.z != floor(v.z)) f->printf("%.3f\n", v.z); else f->printf("%d\n", int(v.z));
            vert += vtxsize;
        }
        ushort *tri = edata;
        loopi(va.tris)
        {
            f->printf("f");
            for(int k = 0; k<3; k++) f->printf(" %d", tri[k]-va.verts-va.voffset);
            tri += 3;
            f->printf("\n");
        }
        delete[] edata;
        delete[] vdata;
    }
    delete f;
}

COMMAND(writeobj, "s");

int getworldsize() { return hdr.worldsize; }
int getmapversion() { return hdr.version; }
ICOMMAND(mapversion, "", (void), intret(getmapversion()));
int getmaprevision() { return hdr.revision; }
ICOMMAND(maprevision, "", (void), intret(getmaprevision()));

