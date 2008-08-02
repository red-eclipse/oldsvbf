// server.cpp: little more than enhanced multicaster
// runs dedicated or as client coroutine

#include "pch.h"
#include "engine.h"

VAR(version, 1, ENG_VERSION, -1); // for scripts

#ifdef STANDALONE
void conoutf(const char *s, ...) { s_sprintfdlv(str, s, s); string st; filtertext(st, str); printf("%s\n", st); }
void console(const char *s, int n, ...) { s_sprintfdlv(str, n, s); string st; filtertext(st, str); printf("%s\n", st); }
void servertoclient(int chan, uchar *buf, int len) {}
void fatal(const char *s, ...)
{
    void cleanupserver();
    cleanupserver();
    s_sprintfdlv(msg,s,s);
    printf("ERROR: %s\n", msg);
    exit(EXIT_FAILURE);
}
VAR(verbose, 0, 0, 4);
#else
VARP(verbose, 0, 0, 4);
#endif

#define DEFAULTCLIENTS 6
int servertype = 1; // 0: none, 1: private, 2: public, 3: dedicated
bool pubserv = false;
int curtime = 0, totalmillis = 0, lastmillis = 0;
int uprate = 0, maxclients = DEFAULTCLIENTS;
const char *ip = "", *master = NULL;
const char *game = "bfa", *load = NULL;

igameclient *cl = NULL;
igameserver *sv = NULL;
iclientcom *cc = NULL;
icliententities *et = NULL;

hashtable<const char *, igame *> *gamereg = NULL;

vector<char *> gameargs;

void registergame(const char *name, igame *ig)
{
    if(!gamereg) gamereg = new hashtable<const char *, igame *>;
	(*gamereg)[name] = ig;
}

void initgame(const char *type)
{
	igame **ig = gamereg->access(type);
	if(!ig) fatal("cannot start game module: %s", type);

	sv = (*ig)->newserver();
	if (sv)
	{
		cl = (*ig)->newclient();

		if (cl)
		{
			cc = cl->getcom();
			et = cl->getents();
		}

		loopv(gameargs)
		{
			if(!cl || !cl->clientoption(gameargs[i]))
			{
				if(!sv->serveroption(gameargs[i]))
					conoutf("\frunknown command-line option: %s", gameargs[i]);
			}
		}
	}
	else fatal("cannot start %s module server", type);
}

// all network traffic is in 32bit ints, which are then compressed using the following simple scheme (assumes that most values are small).

void putint(ucharbuf &p, int n)
{
	if(n<128 && n>-127) p.put(n);
	else if(n<0x8000 && n>=-0x8000) { p.put(0x80); p.put(n); p.put(n>>8); }
	else { p.put(0x81); p.put(n); p.put(n>>8); p.put(n>>16); p.put(n>>24); }
}

int getint(ucharbuf &p)
{
	int c = (char)p.get();
	if(c==-128) { int n = p.get(); n |= char(p.get())<<8; return n; }
	else if(c==-127) { int n = p.get(); n |= p.get()<<8; n |= p.get()<<16; return n|(p.get()<<24); }
	else return c;
}

// much smaller encoding for unsigned integers up to 28 bits, but can handle signed
void putuint(ucharbuf &p, int n)
{
	if(n < 0 || n >= (1<<21))
	{
		p.put(0x80 | (n & 0x7F));
		p.put(0x80 | ((n >> 7) & 0x7F));
		p.put(0x80 | ((n >> 14) & 0x7F));
		p.put(n >> 21);
	}
	else if(n < (1<<7)) p.put(n);
	else if(n < (1<<14))
	{
		p.put(0x80 | (n & 0x7F));
		p.put(n >> 7);
	}
	else
	{
		p.put(0x80 | (n & 0x7F));
		p.put(0x80 | ((n >> 7) & 0x7F));
		p.put(n >> 14);
	}
}

int getuint(ucharbuf &p)
{
	int n = p.get();
	if(n & 0x80)
	{
		n += (p.get() << 7) - 0x80;
		if(n & (1<<14)) n += (p.get() << 14) - (1<<14);
		if(n & (1<<21)) n += (p.get() << 21) - (1<<21);
		if(n & (1<<28)) n |= 0xF0000000;
	}
	return n;
}

void putfloat(ucharbuf &p, float f)
{
    uchar *c = (uchar *)&f;
    endianswap(c, sizeof(float), 1);
    loopi(sizeof(float)) p.put(c[i]);
}

float getfloat(ucharbuf &p)
{
    float f;
    uchar *c = (uchar *)&f;
    loopi(sizeof(float)) c[i] = p.get();
    endianswap(c, sizeof(float), 1);
    return f;
}

void sendstring(const char *t, ucharbuf &p)
{
	while(*t) putint(p, *t++);
	putint(p, 0);
}

void getstring(char *text, ucharbuf &p, int len)
{
	char *t = text;
	do
	{
		if(t>=&text[len]) { text[len-1] = 0; return; }
		if(!p.remaining()) { *t = 0; return; }
		*t = getint(p);
	}
	while(*t++);
}

void filtertext(char *dst, const char *src, bool whitespace, int len)
{
	for(int c = *src; c; c = *++src)
	{
        switch(c)
        {
        	case '\f': ++src; continue;
        }
		if(isspace(c) ? whitespace : isprint(c))
		{
			*dst++ = c;
			if(!--len) break;
		}
	}
	*dst = '\0';
}

vector<client *> clients;

ENetHost *serverhost = NULL;
size_t bsend = 0, brec = 0;
int laststatus = 0;
ENetSocket pongsock = ENET_SOCKET_NULL;

void cleanupserver()
{
	if(serverhost) enet_host_destroy(serverhost);
#ifdef MASTERSERVER
	cleanupmaster();
#endif
#ifdef STANDALONE
	writecfg();
#endif
}

void sendfile(int cn, int chan, FILE *file, const char *format, ...)
{
	if(cn < 0)
	{
#ifndef STANDALONE
		if(!clienthost || clienthost->peers[0].state != ENET_PEER_STATE_CONNECTED)
#endif
			return;
	}
	else if(cn >= clients.length() || clients[cn]->type != ST_TCPIP) return;

	fseek(file, 0, SEEK_END);
	int len = ftell(file);
	bool reliable = false;
	if(*format=='r') { reliable = true; ++format; }
	ENetPacket *packet = enet_packet_create(NULL, MAXTRANS+len, ENET_PACKET_FLAG_RELIABLE);
	rewind(file);

	ucharbuf p(packet->data, packet->dataLength);
	va_list args;
	va_start(args, format);
	while(*format) switch(*format++)
	{
		case 'i':
		{
			int n = isdigit(*format) ? *format++-'0' : 1;
			loopi(n) putint(p, va_arg(args, int));
			break;
		}
		case 's': sendstring(va_arg(args, const char *), p); break;
		case 'l': putint(p, len); break;
	}
	va_end(args);
	enet_packet_resize(packet, p.length()+len);

	fread(&packet->data[p.length()], 1, len, file);
	enet_packet_resize(packet, p.length()+len);

	if(cn >= 0) enet_peer_send(clients[cn]->peer, chan, packet);
#ifndef STANDALONE
	else enet_peer_send(&clienthost->peers[0], chan, packet);
#endif

	if(!packet->referenceCount) enet_packet_destroy(packet);
}

void process(ENetPacket *packet, int sender, int chan);
//void disconnect_client(int n, int reason);

void *getinfo(int i)	{ return !clients.inrange(i) || clients[i]->type==ST_EMPTY ? NULL : clients[i]->info; }
int getnumclients()	 { return clients.length(); }
uint getclientip(int n) { return clients.inrange(n) && clients[n]->type==ST_TCPIP ? clients[n]->peer->address.host : 0; }

void sendpacket(int n, int chan, ENetPacket *packet, int exclude)
{
	if(n<0)
	{
		sv->recordpacket(chan, packet->data, packet->dataLength);
		loopv(clients) if(i!=exclude) sendpacket(i, chan, packet);
		return;
	}
	switch(clients[n]->type)
	{
		case ST_TCPIP:
		{
			enet_peer_send(clients[n]->peer, chan, packet);
			bsend += packet->dataLength;
			break;
		}
		default: break;
	}
}

void sendf(int cn, int chan, const char *format, ...)
{
	int exclude = -1;
	bool reliable = false;
	if(*format=='r') { reliable = true; ++format; }
	ENetPacket *packet = enet_packet_create(NULL, MAXTRANS, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
	ucharbuf p(packet->data, packet->dataLength);
	va_list args;
	va_start(args, format);
	while(*format) switch(*format++)
	{
		case 'x':
			exclude = va_arg(args, int);
			break;

		case 'v':
		{
			int n = va_arg(args, int);
			int *v = va_arg(args, int *);
			loopi(n) putint(p, v[i]);
			break;
		}

		case 'i':
		{
			int n = isdigit(*format) ? *format++-'0' : 1;
			loopi(n) putint(p, va_arg(args, int));
			break;
		}

        case 'f':
        {
            int n = isdigit(*format) ? *format++-'0' : 1;
            loopi(n) putfloat(p, (float)va_arg(args, double));
            break;
        }

		case 's': sendstring(va_arg(args, const char *), p); break;

		case 'm':
		{
			int n = va_arg(args, int);
			enet_packet_resize(packet, packet->dataLength+n);
			p.buf = packet->data;
			p.maxlen += n;
			p.put(va_arg(args, uchar *), n);
			break;
		}
	}
	va_end(args);
	enet_packet_resize(packet, p.length());
	sendpacket(cn, chan, packet, exclude);
	if(packet->referenceCount==0) enet_packet_destroy(packet);
}

const char *disc_reasons[] = { "normal", "end of packet", "client num", "kicked/banned", "tag type", "ip is banned", "server is in private mode", "server FULL (maxclients)" };

void disconnect_client(int n, int reason)
{
	if(clients[n]->type!=ST_TCPIP) return;
	enet_peer_disconnect(clients[n]->peer, reason);
	sv->clientdisconnect(n);
	clients[n]->type = ST_EMPTY;
	clients[n]->peer->data = NULL;
	sv->deleteinfo(clients[n]->info);
	clients[n]->info = NULL;
	s_sprintfd(s)("client (%s) disconnected because: %s", clients[n]->hostname, disc_reasons[reason]);
	conoutf("\fr%s", s);
	sv->srvoutf(-1, "%s", s);
}

void process(ENetPacket *packet, int sender, int chan)	// sender may be -1
{
	ucharbuf p(packet->data, (int)packet->dataLength);
	sv->parsepacket(sender, chan, (packet->flags&ENET_PACKET_FLAG_RELIABLE)!=0, p);
	if(p.overread()) { disconnect_client(sender, DISC_EOP); return; }
}

void send_welcome(int n)
{
	ENetPacket *packet = enet_packet_create (NULL, MAXTRANS, ENET_PACKET_FLAG_RELIABLE);
	ucharbuf p(packet->data, packet->dataLength);
	int chan = sv->welcomepacket(p, n, packet);
	enet_packet_resize(packet, p.length());
	sendpacket(n, chan, packet);
	if(packet->referenceCount==0) enet_packet_destroy(packet);
}

void clienttoserver(int chan, ENetPacket *packet)
{
	process(packet, 0, chan);
	if(packet->referenceCount==0) enet_packet_destroy(packet);
}

void delclient(int n)
{
	if (clients.inrange(n))
	{
		if(clients[n]->type==ST_TCPIP) clients[n]->peer->data = NULL;
		clients[n]->type = ST_EMPTY;
		sv->deleteinfo(clients[n]->info);
		clients[n]->info = NULL;
	}
}

int addclient(int type)
{
	int n = -1;
	loopv(clients) if(clients[i]->type==ST_EMPTY)
	{
		n = i;
		break;
	}
	if (!clients.inrange(n))
	{
		client *c = new client;
		n = c->num = clients.length();
		clients.add(c);
	}
	clients[n]->info = sv->newinfo();
	clients[n]->type = type;
	return n;
}

int nonlocalclients = 0;

bool hasnonlocalclients() { return nonlocalclients!=0; }

static ENetAddress pongaddr;

void sendserverinforeply(ucharbuf &p)
{
    ENetBuffer buf;
    buf.data = p.buf;
    buf.dataLength = p.length();
    enet_socket_send(pongsock, &pongaddr, &buf, 1);
}

void sendpongs()		// reply all server info requests
{
	ENetBuffer buf;
	uchar pong[MAXTRANS];
	int len;
	enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
	buf.data = pong;
	while(enet_socket_wait(pongsock, &events, 0) >= 0 && events)
	{
		buf.dataLength = sizeof(pong);
        len = enet_socket_receive(pongsock, &pongaddr, &buf, 1);
		if(len < 0) return;
        ucharbuf req(pong, len), p(pong, sizeof(pong));
        p.len += len;
        sv->serverinforeply(req, p);
	}
}

#ifdef STANDALONE
bool resolverwait(const char *name, ENetAddress *address)
{
	return enet_address_set_host(address, name) >= 0;
}

int connectwithtimeout(ENetSocket sock, const char *hostname, ENetAddress &remoteaddress)
{
	int result = enet_socket_connect(sock, &remoteaddress);
	if(result<0) enet_socket_destroy(sock);
	return result;
}
#endif

ENetSocket mastersend(ENetAddress &remoteaddress, const char *hostname, const char *req, ENetAddress *localaddress = NULL)
{
	if(remoteaddress.host==ENET_HOST_ANY)
	{
		conoutf("\fwlooking up %s...", hostname);
		if(!resolverwait(hostname, &remoteaddress)) return ENET_SOCKET_NULL;
	}
	ENetSocket sock = enet_socket_create(ENET_SOCKET_TYPE_STREAM, localaddress);
	if(sock==ENET_SOCKET_NULL || connectwithtimeout(sock, hostname, remoteaddress)<0)
	{
		conoutf(sock==ENET_SOCKET_NULL ? "could not open socket" : "could not connect");
		return ENET_SOCKET_NULL;
	}
	ENetBuffer buf;
	s_sprintfd(mget)("%s", req);
	buf.data = mget;
	buf.dataLength = strlen((char *)buf.data);
	conoutf("\fwsending request to %s...", hostname);
	enet_socket_send(sock, NULL, &buf, 1);
	return sock;
}

bool masterreceive(ENetSocket sock, ENetBuffer &buf, int timeout = 0)
{
	if(sock==ENET_SOCKET_NULL) return false;
	enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
	if(enet_socket_wait(sock, &events, timeout) >= 0 && events)
	{
		int len = enet_socket_receive(sock, NULL, &buf, 1);
		if(len<=0)
		{
			enet_socket_destroy(sock);
			return false;
		}
		buf.data = ((char *)buf.data)+len;
		((char*)buf.data)[0] = 0;
		buf.dataLength -= len;
	}
	return true;
}

ENetSocket mssock = ENET_SOCKET_NULL;
ENetAddress msaddress = { ENET_HOST_ANY, ENET_PORT_ANY };
ENetAddress masteradr = { ENET_HOST_ANY, MASTER_PORT };
int lastupdatemaster = 0;
string masterserv;
uchar masterrep[MAXTRANS];
ENetBuffer masterb;

void updatemaster()
{
	if(mssock!=ENET_SOCKET_NULL) enet_socket_destroy(mssock);
	mssock = mastersend(masteradr, masterserv, "add", &msaddress);
	masterrep[0] = 0;
	masterb.data = masterrep;
	masterb.dataLength = MAXTRANS-1;
}

void checkmasterreply()
{
	if(mssock!=ENET_SOCKET_NULL && !masterreceive(mssock, masterb))
	{
		mssock = ENET_SOCKET_NULL;
		conoutf("\fymaster reply: %s", masterrep);
	}
}

#ifndef STANDALONE
#define RETRIEVELIMIT 20000
uchar *retrieveservers(uchar *buf, int buflen)
{
	buf[0] = '\0';
	ENetAddress address = masteradr;
	ENetSocket sock = mastersend(address, masterserv, "list");
	if(sock==ENET_SOCKET_NULL) return buf;
	/* only cache this if connection succeeds */
	masteradr = address;

	s_sprintfd(text)("retrieving servers from %s... (esc to abort)", masterserv);
	renderprogress(0, text);

	ENetBuffer eb;
	eb.data = buf;
	eb.dataLength = buflen-1;

	int starttime = SDL_GetTicks(), timeout = 0;
	while(masterreceive(sock, eb, 250))
	{
		timeout = SDL_GetTicks() - starttime;
        renderprogress(min(float(timeout)/RETRIEVELIMIT, 1.0f), text);
        if(interceptkey(SDLK_ESCAPE)) timeout = RETRIEVELIMIT + 1;
		if(timeout > RETRIEVELIMIT)
		{
			buf[0] = '\0';
			enet_socket_destroy(sock);
			return buf;
		}
	}

	return buf;
}
#endif

void serverslice()	// main server update, called from main loop in sp, or from below in dedicated server
{
	if (!serverhost) return;

	nonlocalclients = 0;

	loopv(clients) switch(clients[i]->type)
	{
		case ST_TCPIP: nonlocalclients++; break;
		default: break;
	}

	sv->serverupdate();

	sendpongs();

	if (pubserv)
	{
		if (*masterserv) checkmasterreply();

		if (totalmillis-lastupdatemaster > 60*60*1000 && *masterserv)		// send alive signal to master every hour of uptime
		{
			updatemaster();
			lastupdatemaster = totalmillis;
		}

		if (totalmillis-laststatus > 60*1000)	// display bandwidth stats, useful for server ops
		{
			laststatus = totalmillis;
			if (nonlocalclients || bsend || brec) conoutf("\fmstatus: %d remote clients, %.1f send, %.1f rec (K/sec)", nonlocalclients, bsend/60.0f/1024, brec/60.0f/1024);
			bsend = brec = 0;
		}
	}

	ENetEvent event;
    bool serviced = false;
    while(!serviced)
    {
        if(enet_host_check_events(serverhost, &event) <= 0)
        {
            if(enet_host_service(serverhost, &event, 0) <= 0) break;
            serviced = true;
        }
		switch(event.type)
		{
			case ENET_EVENT_TYPE_CONNECT:
			{
				int cn = addclient(ST_TCPIP);
				client &c = *clients[cn];
				c.peer = event.peer;
				c.peer->data = &c;
				char hn[1024];
				s_strcpy(c.hostname, (enet_address_get_host_ip(&c.peer->address, hn, sizeof(hn))==0) ? hn : "unknown");
				conoutf("\fgclient connected (%s)", c.hostname);
				int reason = DISC_MAXCLIENTS;
				if(nonlocalclients<maxclients && !(reason = sv->clientconnect(c.num, c.peer->address.host)))
				{
					nonlocalclients++;
					send_welcome(c.num);
				}
				else disconnect_client(c.num, reason);
				break;
			}
			case ENET_EVENT_TYPE_RECEIVE:
			{
				brec += event.packet->dataLength;
				client *c = (client *)event.peer->data;
				if(c) process(event.packet, c->num, event.channelID);
				if(event.packet->referenceCount==0) enet_packet_destroy(event.packet);
				break;
			}
			case ENET_EVENT_TYPE_DISCONNECT:
			{
				client *c = (client *)event.peer->data;
				if(!c) break;
				conoutf("\fodisconnected client (%s)", c->hostname);
				sv->clientdisconnect(c->num);
				nonlocalclients--;
				c->type = ST_EMPTY;
				event.peer->data = NULL;
				sv->deleteinfo(c->info);
				c->info = NULL;
				break;
			}
			default:
				break;
		}
    }
	if(sv->sendpackets()) enet_host_flush(serverhost);
}

void serverloop()
{
	#ifdef WIN32
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	#endif
	conoutf("\fydedicated server started, waiting for clients... [Ctrl-C to exit]");
	for(;;)
	{
		int _lastmillis = lastmillis;
		lastmillis = totalmillis = (int)enet_time_get();
		curtime = lastmillis-_lastmillis;

		#ifdef MASTERSERVER
		checkmaster();
		#endif
		if(servertype) serverslice();

		if((int)enet_time_get()-lastmillis <= 0)
		{
			#ifdef WIN32
			Sleep(1);
			#else
			usleep(1000);
			#endif
		}
	}
	exit(EXIT_SUCCESS);
}

void setupserver()
{
	conoutf("\fminit: server");
	pubserv = servertype >= 2 ? true : false;
	if(!master) master = sv->getdefaultmaster();
	s_strcpy(masterserv, master);

	ENetAddress address = { ENET_HOST_ANY, sv->serverport() };
	if (*ip)
	{
		if (enet_address_set_host(&address, ip) < 0) conoutf("\frWARNING: server ip not resolved");
		else msaddress.host = address.host;
	}
	serverhost = enet_host_create(&address, maxclients+1, 0, uprate);
	if (!serverhost) { conoutf("\frcould not create server socket"); return; }
	loopi (maxclients) serverhost->peers[i].data = NULL;

	address.port = sv->serverinfoport();
	pongsock = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM, &address);
	if (pongsock == ENET_SOCKET_NULL)
	{
		conoutf("\frcould not create server info socket, publicity disabled");
		pubserv = false;
	}
	else
	{
		enet_socket_set_option(pongsock, ENET_SOCKOPT_NONBLOCK, 1);
	}
	sv->changemap(load && *load ? load : NULL, -1, -1);

	if (pubserv && *masterserv)
	{
		updatemaster();
	}
	if(verbose) conoutf("\fygame server for %s started", game);

#ifndef STANDALONE
	if(servertype >= 3) serverloop();
#endif
}

void initruntime()
{
	initgame(game);

#ifdef MASTERSERVER
    setupmaster();
#endif
	if(servertype) setupserver();
}

bool serveroption(char *opt)
{
	switch(opt[1])
	{
		case 'g': game = opt+2; return true;
		case 'h': printf("Using home directory: %s\n", &opt[2]); sethomedir(&opt[2]); return true;
		case 'p': printf("Adding package directory: %s\n", &opt[2]); addpackagedir(&opt[2]); return true;
		case 'v': setvar("verbose", atoi(opt+2)); return true;
		case 's':
		{
			switch(opt[2])
			{
				case 'u': uprate = atoi(opt+3); return true;
				case 'c':
				{
					int clients = atoi(opt+3);
					if(clients > 0) maxclients = min(clients, MAXCLIENTS);
					else maxclients = DEFAULTCLIENTS;
					return true;
				}
				case 'i': ip = opt+3; return true;
				case 'm': master = opt+3; return true;
				case 'l': load = opt+3; return true;
				case 's': servertype = atoi(opt+3); return true;
			}
		}
#ifdef MASTERSERVER
		case 'm':
		{
			switch(opt[2])
			{
				case 's': setvar("masterserver", atoi(opt+3)); return true;
				case 'i': setsvar("masterip", opt+3); return true;
				case 'p': setvar("masterport", atoi(opt+3)); return true;
				default: return false;
			}
			return false;
		}
#endif
		default: return false;
	}
	return false;
}

#ifdef STANDALONE
int main(int argc, char* argv[])
{
	servertype = 3;
	for(int i = 1; i<argc; i++) if(argv[i][0]!='-' || !serveroption(argv[i])) gameargs.add(argv[i]);
	if(enet_initialize()<0) fatal("Unable to initialise network module");
	atexit(enet_deinitialize);
	atexit(cleanupserver);
	enet_time_set(0);
	initruntime();
	execfile("server.cfg");
	serverloop();
	return 0;
}
#endif
