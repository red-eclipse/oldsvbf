// the interface the engine uses to run the gameplay module

struct icliententities
{
    virtual ~icliententities() {}

    virtual void editent(int i) = 0;
	virtual void readent(gzFile &g, int mtype, int mver, char *gid, int gver, int id, entity &e) { return; }
	virtual void writeent(gzFile &g, int id, entity &e) { return; }
	virtual void initents(gzFile &g, int mtype, int mver, char *gid, int gver) = 0;
    virtual float dropheight(entity &e) = 0;
    virtual void fixentity(extentity &e) = 0;
    virtual const char *findname(int type) = 0;
	virtual int findtype(char *type) = 0;
	virtual bool maylink(int type, int ver = 0) = 0;
	virtual bool canlink(int index, int node, bool msg = false) = 0;
	virtual bool linkents(int index, int node, bool add, bool local, bool toggle) = 0;
    virtual extentity *newent() = 0;
    virtual vector<extentity *> &getents() = 0;
    virtual void drawparticles() = 0;
};

struct iclientcom
{
    virtual ~iclientcom() {}

    virtual void gamedisconnect(int clean) = 0;
    virtual void parsepacketclient(int chan, ucharbuf &p) = 0;
    virtual int sendpacketclient(ucharbuf &p, bool &reliable) = 0;
    virtual void gameconnect(bool _remote) = 0;
    virtual bool allowedittoggle(bool edit) = 0;
    virtual void edittoggled(bool edit) {}
    virtual void writeclientinfo(FILE *f) = 0;
    virtual void toserver(int flags, char *text) = 0;
    virtual void changemap(const char *name) = 0;
	virtual bool ready() { return true; }
	virtual int otherclients() { return 0; }
	virtual void toservcmd(char *text, bool msg) { return; }
    virtual int numchannels() { return 1; }
	virtual int servercompare(serverinfo *a, serverinfo *b) { return strcmp(a->name, b->name); }
    virtual const char *serverinfogui(g3d_gui *g, vector<serverinfo *> &servers) { return NULL; }
};

struct igameclient
{
    virtual ~igameclient() {}

    virtual icliententities *getents() = 0;
    virtual iclientcom *getcom() = 0;

    virtual bool clientoption(char *arg) { return false; }
    virtual void updateworld() = 0;
    virtual void editvar(ident *id, bool local) = 0;
    virtual void edittrigger(const selinfo &sel, int op, int arg1 = 0, int arg2 = 0, int arg3 = 0) = 0;
    virtual void resetgamestate() = 0;
    virtual void newmap(int size) = 0;
    virtual void startmap(const char *name) = 0;
    virtual void preload() {}
    virtual void drawhud(int w, int h) = 0;
    virtual bool allowmove(physent *d) { return true; }
    virtual dynent *iterdynents(int i) = 0;
    virtual int numdynents() = 0;
    virtual void render() = 0;
    virtual void g3d_gamemenus() = 0;
    virtual void lighteffects(dynent *d, vec &color, vec &dir) {}
    virtual void adddynlights() {}
    virtual void particletrack(physent *owner, vec &o, vec &d) {}

	virtual bool mousemove(int dx, int dy, int x, int y, int w, int h) = 0;
	virtual void project(int w, int h, vec &dir, float &x, float &y) = 0;
	virtual void recomputecamera(int w, int h) = 0;

	virtual bool gamethirdperson() { return false; } ;
	virtual bool gethudcolour(vec &colour) { return false; }

	virtual void loadworld(gzFile &f, int maptype) { return; };
	virtual void saveworld(gzFile &f) { return; };

	virtual int localplayers() { return 1; }
	virtual bool gui3d() { return true; }

	virtual vec feetpos(physent *d)
	{
		//if (d->type == ENT_PLAYER || d->type == ENT_AI)
		//	return vec(d->o).sub(vec(0, 0, d->height));
		return vec(d->o);
	}

	virtual void menuevent(int event) { return; }
	virtual char *gametitle() = 0;
	virtual char *gametext() = 0;
};

struct igameserver
{
    virtual ~igameserver() {}

    virtual bool serveroption(char *arg) { return false; }
    virtual void *newinfo() = 0;
    virtual void deleteinfo(void *ci) = 0;
    virtual void serverinit() = 0;
    virtual void clientdisconnect(int n) = 0;
    virtual int clientconnect(int n, uint ip) = 0;
    virtual const char *servername() = 0;
    virtual void recordpacket(int chan, void *data, int len) {}
    virtual void parsepacket(int sender, int chan, bool reliable, ucharbuf &p) = 0;
    virtual bool sendpackets() = 0;
    virtual int welcomepacket(ucharbuf &p, int n, ENetPacket *packet) = 0;
    virtual void serverinforeply(ucharbuf &req, ucharbuf &p) = 0;
    virtual void serverupdate() = 0;
    virtual int serverinfoport() = 0;
    virtual int serverport() = 0;
    virtual const char *getdefaultmaster() = 0;
    virtual void sendservmsg(const char *s) = 0;
    virtual void changemap(const char *s, int mode = 0, int muts = 0) { return; }
    virtual const char *gameid() = 0;
	virtual char *gamename(int mode, int muts) = 0;
	virtual void modecheck(int *mode, int *muts) = 0;
	virtual int gamever() = 0;
    virtual const char *defaultmap() = 0;
    virtual int defaultmode() = 0;
    virtual bool canload(char *type) = 0;
};

struct igame
{
    virtual ~igame() {}

    virtual igameclient *newclient() = 0;
    virtual igameserver *newserver() = 0;
};
