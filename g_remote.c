
#include "g_local.h"


remote_t remote;

cvar_t	*net_port;


void RA_Send(remote_cmd_t cmd, const char *fmt, ...) {

	va_list     argptr;
    char        string[MAX_STRING_CHARS];
	size_t      len;
	
	if (!remote.enabled) {
		return;
	}
	
	va_start(argptr, fmt);
    len = g_vsnprintf(string, sizeof(string), fmt, argptr);
    va_end(argptr);
	
	if (len >= sizeof(string)) {
        return;
    }
	
	gchar *final = g_strconcat(stringf("%s\\%d\\", remoteKey, cmd), string, NULL);
	gchar *encoded = g_base64_encode(final, strlen(final));
	
	if (remote.flags & REMOTE_FL_DEBUG) {
		gi.dprintf("[RA] Sending: %s\n", encoded);
	}

	int r = sendto(
		remote.socket, 
		encoded,
		strlen(final)+1, 
		MSG_DONTWAIT, 
		remote.addr->ai_addr, 
		remote.addr->ai_addrlen
	);
	
	if (r == -1) {
		gi.dprintf("[RA] error sending data: %s\n", strerror(errno));
	}
	
	g_free(final);
	g_free(encoded);
}


void RA_Init() {
	
	memset(&remote, 0, sizeof(remote));

	net_port = gi.cvar("net_port", "27910", CVAR_LATCH);
	maxclients = gi.cvar("maxclients", "64", CVAR_LATCH);
	
	if (!remoteEnabled) {
		gi.dprintf("Remote Admin is disabled in your config file.\n");
		return;
	}
	
	gi.dprintf("\[RA] Remote Admin Init...\n");
	
	struct addrinfo hints, *res = 0;
	memset(&hints, 0, sizeof(hints));
	memset(&res, 0, sizeof(res));
	
	hints.ai_family         = AF_INET;   	// either v6 or v4
	hints.ai_socktype       = SOCK_DGRAM;	// UDP
	hints.ai_protocol       = 0;
	hints.ai_flags          = AI_ADDRCONFIG;
	
	gi.dprintf("[RA] looking up %s... ", remoteAddr);

	int err = getaddrinfo(remoteAddr, stringf("%d",remotePort), &hints, &res);
	if (err != 0) {
		gi.dprintf("error, disabling\n");
		remote.enabled = 0;
		return;
	} else {
		char address[INET_ADDRSTRLEN];
		inet_ntop(res->ai_family, &((struct sockaddr_in *)res->ai_addr)->sin_addr, address, sizeof(address));
		gi.dprintf("%s\n", address);
	}
	
	int fd = socket(res->ai_family, res->ai_socktype, IPPROTO_UDP);
	if (fd == -1) {
		gi.dprintf("Unable to open socket to %s:%d...disabling remote admin\n", remoteAddr, remotePort);
		remote.enabled = 0;
		return;
	}
	
	remote.socket = fd;
	remote.addr = res;
	remote.flags = remoteFlags;
	remote.enabled = 1;
}


void RA_RunFrame() {
	
	if (!remote.enabled) {
		return;
	}

	uint8_t i;

	// report server if necessary
	if (remote.next_report <= remote.frame_number) {
		RA_Send(CMD_SHEARTBEAT, "%s\\%d\\%s\\%d\\%d", remote.mapname, remote.maxclients, remote.rcon_password, remote.port, remote.flags);
		remote.next_report = remote.frame_number + SECS_TO_FRAMES(60);
	}


	for (i=0; i<=remote.maxclients; i++) {
		if (proxyinfo[i].inuse) {

			if (proxyinfo[i].next_report <= remote.frame_number) {
				RA_Send(CMD_PHEARTBEAT, "%d\\%s", i, proxyinfo[i].userinfo);
				proxyinfo[i].next_report = remote.frame_number + SECS_TO_FRAMES(60);
			}

			/*
			if (!proxyinfo[i].remote_reported) {
				RA_Send(CMD_CONNECT, "%d\\%s", i, proxyinfo[i].userinfo);
				proxyinfo[i].remote_reported = 1;
			}

			// replace player edict's die() pointer
			if (*proxyinfo[i].ent->die != PlayerDie_Internal) {
				proxyinfo[i].die = *proxyinfo[i].ent->die;
				proxyinfo[i].ent->die = &PlayerDie_Internal;
			}
			*/
		}
	}

	remote.frame_number++;
}

void RA_Shutdown() {
	if (!remote.enabled) {
		return;
	}

	gi.dprintf("[RA] Unregistering with remote admin server\n\n");
	RA_Send(CMD_SDISCONNECT, "");
	freeaddrinfo(remote.addr);
}


void PlayerDie_Internal(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point) {
	uint8_t id = getEntOffset(self) - 1;
	uint8_t aid = getEntOffset(attacker) - 1;
	
	if (self->deadflag != DEAD_DEAD) {	
		gi.dprintf("self: %s\t inflictor: %s\t attacker %s\n", self->classname, inflictor->classname, attacker->classname);
		
		// crater, drown (water, acid, lava)
		if (g_strcmp0(attacker->classname, "worldspawn") == 0) {
			//RA_Send(CMD_FRAG,"%d\\%d\\worldspawn", id, aid);
		} else if (g_strcmp0(attacker->classname, "player") == 0 && attacker->client) {
			//gi.dprintf("Attacker: %s\n", attacker->client->pers.netname);
			/*RA_Send(CMD_FRAG, "%d\\%d\\%s", id, aid,
				attacker->client->pers.weapon->classname
			);*/
		}
	}
	
	proxyinfo[id].die(self, inflictor, attacker, damage, point);
}
