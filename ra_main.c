#include "ra_main.h"
#include "g_local.h"


remote_t remote;

// remote admin specific cvars
cvar_t		*remote_enabled;
cvar_t		*remote_server;
cvar_t		*remote_port;
cvar_t		*remote_key;


void RA_Send(const char *format, ...) {
	static char strings[8][MAX_STRING_CHARS];
	static uint16_t index;

	char *string = strings[index++ % 8];

	va_list args;

	va_start(args, format);
	vsnprintf(string, MAX_STRING_CHARS, format, args);
	va_end(args);

	//gi.dprintf("RA: Debug: sending '%s'\n", string);
	int r = sendto(
				remote.socket, 
				string, 
				strlen(string)+1, 
				MSG_DONTWAIT, 
				remote.addr->ai_addr, 
				remote.addr->ai_addrlen
			);
	if (r == -1) {
		gi.dprintf("RA: error sending data: %s\n", strerror(errno));
	}
}


void RA_Init() {
	
	remote_enabled = gi.cvar("remote_enabled", "1", 0);
	remote_server = gi.cvar("remote_server", "packetflinger.com", 0);
	remote_port = gi.cvar("remote_port", "9999", 0);
	
	if (g_strcmp0(remote_enabled->string, "0") == 0)
		return;

	remote.enabled = 1;
	
	gi.dprintf("\nRA: Remote Admin Init\n");
	
	struct addrinfo hints, *res = 0;
	memset(&hints, 0, sizeof(hints));
	memset(&res, 0, sizeof(res));
	
	//hints.ai_family			= AF_UNSPEC;    // ipv6 then v4
	hints.ai_family         = AF_INET;   	// ipv4 only
	hints.ai_socktype       = SOCK_DGRAM;	// UDP
	hints.ai_protocol       = 0;
	hints.ai_flags          = AI_ADDRCONFIG;
	
	gi.dprintf("RA: looking up %s... ", remote_server->string);
	int err = getaddrinfo(remote_server->string, remote_port->string, &hints, &res);
	if (err != 0) {
		gi.dprintf("error, disabling\n");
		remote.enabled = 0;
		return;
	} else {
		char address[INET_ADDRSTRLEN];
		inet_ntop(res->ai_family, &((struct sockaddr_in *)res->ai_addr)->sin_addr, address, sizeof(address));
		gi.dprintf("done [%s].\n", address);
	}
	
	int fd = socket(res->ai_family, res->ai_socktype, IPPROTO_UDP);
	if (fd == -1) {
		gi.dprintf("Unable to open socket to %s:%s...disabling remote admin\n", remote_server->string, remote_port->string);
		remote.enabled = 0;
		return;
	}
	
	remote.socket = fd;
	remote.addr = res;
	
	gi.dprintf("RA: Registering with remote admin server\n\n");
	RA_Send("REG %s", rcon_password->string);
}

void RA_Shutdown() {
}

// run every frame (1/10 second)
void RA_RunFrame() {
}

char *pfva(const char *format, ...) {
	static char strings[8][MAX_STRING_CHARS];
	static uint16_t index;

	char *string = strings[index++ % 8];

	va_list args;

	va_start(args, format);
	vsnprintf(string, MAX_STRING_CHARS, format, args);
	va_end(args);

	return string;
}
