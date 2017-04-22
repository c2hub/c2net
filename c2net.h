//c2net is a modified version of ZedZull's zed_net library. The library is in public domain

#ifndef C2NET_H
#define C2NET_H
typedef struct {
	unsigned int host;
	unsigned short port;
} netaddress;
typedef struct {
	int handle;
	int non_blocking;
	int ready;
} netsocket;

const char* get_error(void);
int init(void);
void netshutdown(void);
int get_address(netaddress *address, const char *host, unsigned short port);
const char* host_to_str(unsigned int host);

void socket_close(netsocket* socket);

int udp_socket_open(netsocket* socket, unsigned int port, int non_blocking);
int udp_socket_send(netsocket* socket, netaddress destination, const void *data, int size);
int udp_socket_receive(netsocket* socket, netaddress* sender, void* data, int size);

int tcp_socket_open(netsocket* socket, unsigned int port, int non_blocking, int listen_socket);
int tcp_connect(netsocket* socket, netaddress remote_addr);
int tcp_accept(netsocket* listening_socket, netsocket* remote_socket, netaddress* remote_addr);
int tcp_socket_send(netsocket* remote_socket, const void *data, int size);
int tcp_socket_receive(netsocket* remote_socket, void *data, int size);
int tcp_make_socket_ready(netsocket* socket);


#endif
