namespace auth
{
    ENetSocket socket = ENET_SOCKET_NULL;
    char input[4096];
    vector<char> output;
    int inputpos = 0, outputpos = 0;
    int lastconnect = 0;
    uint nextauthreq = 0;

	extern void connect();
	extern void disconnect();

    void setmaster(clientinfo *ci, bool val, const char *pass = "", const char *authname = NULL)
	{
        if(authname && !val) return;
		const char *name = "";
		if(val)
		{
            bool haspass = adminpass[0] && checkpassword(ci, adminpass, pass);
			if(ci->privilege)
			{
				if(!adminpass[0] || haspass==(ci->privilege==PRIV_ADMIN)) return;
			}
            else if(ci->state.state==CS_SPECTATOR && !haspass) return;
            loopv(clients) if(ci!=clients[i] && clients[i]->privilege)
			{
				if(haspass) clients[i]->privilege = PRIV_NONE;
				else return;
			}
            if(haspass) ci->privilege = PRIV_ADMIN;
            else if(!authname && !(mastermask&MM_AUTOAPPROVE) && !ci->privilege && !ci->local)
            {
                sendf(ci->clientnum, 1, "ris", SV_SERVMSG, "This server requires you to use the \"/auth\" command to gain master.");
                return;
            }
            else
            {
                if(authname)
                {
                    loopv(clients) if(ci!=clients[i] && clients[i]->privilege<=PRIV_MASTER) clients[i]->privilege = PRIV_NONE;
                }
                ci->privilege = PRIV_MASTER;
            }
            name = privname(ci->privilege);
		}
		else
		{
			if(!ci->privilege) return;
			name = privname(ci->privilege);
			ci->privilege = 0;
		}
		mastermode = MM_OPEN;
        allowedips.setsize(0);
        if(val && authname) srvoutf("%s claimed %s as '\fs\fc%s\fr'", colorname(ci), name, authname);
        else srvoutf("%s %s %s", colorname(ci), val ? "claimed" : "relinquished", name);
		currentmaster = val ? ci->clientnum : -1;
		masterupdate = true;
	}

    bool isconnected() { return socket != ENET_SOCKET_NULL; }

    void addoutput(const char *str)
    {
        int len = strlen(str);
        if(len + output.length() > 64*1024) return;
        output.put(str, len);
    }

    clientinfo *findauth(uint id)
    {
        loopv(clients) if(clients[i]->authreq == id) return clients[i];
        return NULL;
    }

    void authfailed(uint id)
    {
        clientinfo *ci = findauth(id);
        if(!ci) return;
        ci->authreq = 0;
    }

    void authsucceeded(uint id)
    {
        clientinfo *ci = findauth(id);
        if(!ci) return;
        ci->authreq = 0;
		setmaster(ci, true, "", ci->authname);
    }

    void authchallenged(uint id, const char *val)
    {
        clientinfo *ci = findauth(id);
        if(!ci) return;
        sendf(ci->clientnum, 1, "riis", SV_AUTHCHAL, id, val);
    }

    void tryauth(clientinfo *ci, const char *user)
    {
        if(!isconnected())
        {
            if(!lastconnect || lastmillis - lastconnect > 60*1000) connect();
            if(!isconnected())
            {
                sendf(ci->clientnum, 1, "ris", SV_SERVMSG, "not connected to authentication server");
                return;
            }
        }
        if(!nextauthreq) nextauthreq = 1;
        ci->authreq = nextauthreq++;
        filtertext(ci->authname, user, false, 100);
        s_sprintfd(buf)("reqauth %u %s\n", ci->authreq, ci->authname);
        addoutput(buf);
    }

    void answerchallenge(clientinfo *ci, uint id, char *val)
    {
        if(ci->authreq != id) return;
        for(char *s = val; *s; s++)
        {
            if(!isxdigit(*s)) { *s = '\0'; break; }
        }
        s_sprintfd(buf)("confauth %u %s\n", id, val);
        addoutput(buf);
    }

    void processinput()
    {
        if(inputpos <= 0) return;

        char *end = (char *)memchr(input, '\n', inputpos);
        while(end)
        {
            *end++ = '\0';

            //printf("authserv: %s\n", input);

            uint id;
            string val;
            if((sscanf(input, "error %s", val) == 2) || (sscanf(input, "echo %s", val) == 2))
				conoutf("authserver: %s", val);
            else if(sscanf(input, "failauth %u", &id) == 1)
                authfailed(id);
            else if(sscanf(input, "succauth %u", &id) == 1)
                authsucceeded(id);
            else if(sscanf(input, "chalauth %u %s", &id, val) == 2)
                authchallenged(id, val);

            inputpos = &input[inputpos] - end;
            memmove(input, end, inputpos);

            end = (char *)memchr(input, '\n', inputpos);
        }

        if(inputpos >= (int)sizeof(input)) disconnect();
    }

    void connect()
    {
        if(socket != ENET_SOCKET_NULL) return;

        lastconnect = lastmillis;

        conoutf("connecting to authentication server...");
        socket = enet_socket_create(ENET_SOCKET_TYPE_STREAM);
        if(socket != ENET_SOCKET_NULL)
        {
            ENetAddress authserver = { ENET_HOST_ANY, servermasterport };
            int result = enet_socket_connect(socket, &authserver);
            if(result < 0)
            {
                enet_socket_destroy(socket);
                socket = ENET_SOCKET_NULL;
            }
            else enet_socket_set_option(socket, ENET_SOCKOPT_NONBLOCK, 1);
        }
        if(socket == ENET_SOCKET_NULL) conoutf("couldn't connect to authentication server");
        else
        {
        	conoutf("connected to authentication server");
        	s_sprintfd(msg)("server %d %d\n", serverport, serverqueryport);
        	addoutput(msg);
        }
    }

    void disconnect()
    {
        if(socket == ENET_SOCKET_NULL) return;

        enet_socket_destroy(socket);
        socket = ENET_SOCKET_NULL;

        output.setsizenodelete(0);
        outputpos = inputpos = 0;
    }

    void flushoutput()
    {
        if(output.empty()) return;

        ENetBuffer buf;
        buf.data = &output[outputpos];
        buf.dataLength = output.length() - outputpos;
        int sent = enet_socket_send(socket, NULL, &buf, 1);
        if(sent >= 0)
        {
            outputpos += sent;
            if(outputpos >= output.length())
            {
                output.setsizenodelete(0);
                outputpos = 0;
            }
        }
        else disconnect();
    }

    void flushinput()
    {
        enet_uint32 events = ENET_SOCKET_WAIT_RECEIVE;
        if(enet_socket_wait(socket, &events, 0) < 0 || !events) return;

        ENetBuffer buf;
        buf.data = &input[inputpos];
        buf.dataLength = sizeof(input) - inputpos;
        int recv = enet_socket_receive(socket, NULL, &buf, 1);

        if(recv > 0)
        {
            inputpos += recv;
            processinput();
        }
        else disconnect();
    }

    void update()
    {
        if(!isconnected()) return;

        int authreqs = 0;
        loopv(clients) if(clients[i]->authreq) authreqs++;
        if(!authreqs) return;

        flushoutput();
        flushinput();
    }
}
