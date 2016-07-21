#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "c2net.h"
#ifdef _WIN32
#include <WinSock2.h>
#pragma comment(lib, "wsock32.lib")
#else
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#endif

static const char* g_error;

static int error(const char *message) {
	g_error = message;

	return -1;
}

const char* get_error(void) {
	return g_error;
}

int init(void) {
#ifdef _WIN32
	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
	{
		return error("Windows Sockets failed to start");
	}

	return 0;
#else
	return 0;
#endif
}

void netshutdown(void) {
#ifdef _WIN32
	WSACleanup();
#endif
}

int get_address(netaddress *address, const char *host, unsigned short port) {
	if (host == NULL) {
		address->host = INADDR_ANY;
	}
	else {
		address->host = inet_addr(host);
		if (address->host == INADDR_NONE) {
			struct hostent *hostent = gethostbyname(host);
			if (hostent) {
				memcpy(&address->host, hostent->h_addr, hostent->h_length);
			}
			else {
				return error("Invalid host name");
			}
		}
	}

	address->port = port;

	return 0;
}

const char * host_to_str(unsigned int host) {
	struct in_addr in;
	in.s_addr = host;

	return inet_ntoa(in);
}

int udp_socket_open(netsocket* sock, unsigned int port, int non_blocking) {
	if (!sock)
		return error("Socket is NULL");

	// Create the socket
	sock->handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock->handle <= 0) {
		socket_close(sock);
		return error("Failed to create socket");
	}

	// Bind the socket to the port
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(sock->handle, (const struct sockaddr *) &address, sizeof(struct sockaddr_in)) != 0) {
		socket_close(sock);
		return error("Failed to bind socket");
	}

	// Set the socket to non-blocking if neccessary
	if (non_blocking) {
#ifdef _WIN32
		if (ioctlsocket(sock->handle, FIONBIO, &non_blocking) != 0) {
			socket_close(sock);
			return error("Failed to set socket to non-blocking");
		}
#else
		if (fcntl(sock->handle, F_SETFL, O_NONBLOCK, non_blocking) != 0) {
			socket_close(sock);
			return error("Failed to set socket to non-blocking");
		}
#endif
	}

	sock->non_blocking = non_blocking;

	return 0;
}

int tcp_socket_open(netsocket* sock, unsigned int port, int non_blocking, int listen_socket) {
	if (!sock)
		return error("Socket is NULL");

	// Create the socket
	sock->handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock->handle <= 0) {
		socket_close(sock);
		return error("Failed to create socket");
	}

	// Bind the socket to the port
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(sock->handle, (const struct sockaddr *) &address, sizeof(struct sockaddr_in)) != 0) {
		socket_close(sock);
		return error("Failed to bind socket");
	}

	// Set the socket to non-blocking if neccessary
	if (non_blocking) {
#ifdef _WIN32
		if (ioctlsocket(sock->handle, FIONBIO, &non_blocking) != 0) {
			socket_close(sock);
			return error("Failed to set socket to non-blocking");
		}
#else
		if (fcntl(sock->handle, F_SETFL, O_NONBLOCK, non_blocking) != 0) {
			socket_close(sock);
			return error("Failed to set socket to non-blocking");
		}
#endif
		sock->ready = 0;
	}

	if (listen_socket) {
#ifndef SOMAXCONN
#define SOMAXCONN 10
#endif
		if (listen(sock->handle, SOMAXCONN) != 0) {
			socket_close(sock);
			return error("Failed make socket listen");
		}
	}
	sock->non_blocking = non_blocking;

	return 0;
}

// Returns 1 if it would block, <0 if there's an error.
int check_would_block(netsocket* socket) {
	struct timeval timer;
	fd_set writefd;
	int retval;

	if (socket->non_blocking && !socket->ready) {
		writefd.fd_count = 1;
		writefd.fd_array[0] = socket->handle;
		timer.tv_sec = 0;
		timer.tv_usec = 0;
		retval = select(0, NULL, &writefd, NULL, &timer);
		if (retval == 0)
			return 1;
		else if (retval == SOCKET_ERROR) {
			socket_close(socket);
			return error("Got socket error from select()");
		}
		socket->ready = 1;
	}

	return 0;
}

int tcp_make_socket_ready(netsocket* socket) {
	if (!socket->non_blocking)
		return 0;
	if (socket->ready)
		return 0;

	fd_set writefd;
	int retval;

	writefd.fd_count = 1;
	writefd.fd_array[0] = socket->handle;
	retval = select(0, NULL, &writefd, NULL, NULL);
	if (retval != 1)
		return error("Failed to make non-blocking socket ready");

	socket->ready = 1;

	return 0;
}

int tcp_connect(netsocket* socket, netaddress remote_addr) {
	struct sockaddr_in address;
	int retval;

	if (!socket)
		return error("Socket is NULL");

	retval = check_would_block(socket);
	if (retval == 1)
		return 1;
	else if (retval)
		return -1;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = remote_addr.host;
	address.sin_port = htons(remote_addr.port);

	retval = connect(socket->handle, (const struct sockaddr *) &address, sizeof(address));
	if (retval == SOCKET_ERROR) {
		socket_close(socket);
		return error("Failed to connect socket");
	}

	return 0;
}

int tcp_accept(netsocket* listening_socket, netsocket* remote_socket, netaddress *remote_addr) {
	struct sockaddr_in address;
	int retval, handle;

	if (!listening_socket)
		return error("Listening socket is NULL");
	if (!remote_socket)
		return error("Remote socket is NULL");
	if (!remote_addr)
		return error("Address pointer is NULL");

	retval = check_would_block(listening_socket);
	if (retval == 1)
		return 1;
	else if (retval)
		return -1;
#ifdef _WIN32
	typedef int socklen_t;
#endif
	socklen_t addrlen = sizeof(address);
	handle = accept(listening_socket->handle, (struct sockaddr *)&address, &addrlen);

	if (handle == INVALID_SOCKET)
		return 2;

	remote_addr->host = address.sin_addr.s_addr;
	remote_addr->port = ntohs(address.sin_port);
	remote_socket->non_blocking = listening_socket->non_blocking;
	remote_socket->ready = 0;
	remote_socket->handle = handle;

	return 0;
}

void socket_close(netsocket* socket) {
	if (!socket) {
		return;
	}

	if (socket->handle) {
#ifdef _WIN32
		closesocket(socket->handle);
#else
		close(socket->handle);
#endif
	}
}

int udp_socket_send(netsocket* socket, netaddress destination, const void *data, int size) {
	if (!socket) {
		return error("Socket is NULL");
	}

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = destination.host;
	address.sin_port = htons(destination.port);

	int sent_bytes = sendto(socket->handle, (const char *)data, size, 0, (const struct sockaddr *) &address, sizeof(struct sockaddr_in));
	if (sent_bytes != size) {
		return error("Failed to send data");
	}

	return 0;
}

int udp_socket_receive(netsocket* socket, netaddress *sender, void *data, int size) {
	if (!socket) {
		return error("Socket is NULL");
	}

#ifdef _WIN32
	typedef int socklen_t;
#endif

	struct sockaddr_in from;
	socklen_t from_length = sizeof(from);

	int received_bytes = recvfrom(socket->handle, (char *)data, size, 0, (struct sockaddr *) &from, &from_length);
	if (received_bytes <= 0) {
		return 0;
	}

	sender->host = from.sin_addr.s_addr;
	sender->port = ntohs(from.sin_port);

	return received_bytes;
}

int socket_send(netsocket* remote_socket, const void *data, int size) {
	int retval;

	if (!remote_socket) {
		return error("Socket is NULL");
	}

	retval = check_would_block(remote_socket);
	if (retval == 1)
		return 1;
	else if (retval)
		return -1;

	int sent_bytes = send(remote_socket->handle, (const char *)data, size, 0);
	if (sent_bytes != size) {
		return error("Failed to send data");
	}

	return 0;
}

int tcp_socket_receive(netsocket* remote_socket, void *data, int size) {
	int retval;

	if (!remote_socket) {
		return error("Socket is NULL");
	}

	retval = check_would_block(remote_socket);
	if (retval == 1)
		return 1;
	else if (retval)
		return -1;

#ifdef _WIN32
	typedef int socklen_t;
#endif

	int received_bytes = recv(remote_socket->handle, (char *)data, size, 0);
	if (received_bytes <= 0) {
		return 0;
	}
	return received_bytes;
}