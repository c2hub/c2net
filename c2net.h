/////////////////////////////////////////////////////////////////////////////////////////
//
// zed_net - v0.19 - public domain networking library
// (inspired by the excellent stb libraries: https://github.com/nothings/stb)
//
// This library is intended primarily for use in games and provides a simple wrapper
// around BSD sockets (Winsock 2.2 on Windows). Sockets can be set to be blocking or
// non-blocking. Both UDP and TCP sockets are supported.
//
// VERSION HISTORY
//
//    0.19 (3/4/2016) TCP added and malloc/free calls removed.
//                     Not backwards compatible. - Ian T. Jacobsen (itjac.me)
//    0.18 (9/13/2015) minor polishing
//    0.17 (8/8/2015) initial release
//
// LICENSE
//
//    This software is in the public domain. Where that dedication is not recognized, you
//    are granted a perpetual, irrevocable license to copy, distribute, and modify this
//    file as you see fit.
//
// USAGE
//
//    #define the symbol ZED_NET_IMPLEMENTATION in *one* C/C++ file before the #include
//    of this file; the implementation will be generated in that file.
//
//    If you define the symbol ZED_NET_STATIC, then the implementation will be private to
//    that file.
//
//    Immediately after this block comment is the "header file" section. This section
//    includes documentation for each API function.
//

#ifndef INCLUDE_ZED_NET_H
#define INCLUDE_ZED_NET_H


	/////////////////////////////////////////////////////////////////////////////////////////
	//
	// INITIALIZATION AND netshutdown
	//

	// Get a brief reason for failure
	const char* get_error(void);

	// Perform platform-specific socket initialization;
	// *must* be called before using any other function
	//
	// Returns 0 on success, -1 otherwise (call 'zed_net_get_error' for more info)
	int init(void);

	// Perform platform-specific socket de-initialization;
	// *must* be called when finished using the other functions
	void netshutdown(void);

	/////////////////////////////////////////////////////////////////////////////////////////
	//
	// INTERNET ADDRESS API
	//

	// Represents an internet address usable by sockets
	typedef struct {
		unsigned int host;
		unsigned short port;
	} netaddress;

	// Obtain an address from a host name and a port
	//
	// 'host' may contain a decimal formatted IP (such as "127.0.0.1"), a human readable
	// name (such as "localhost"), or NULL for the default address
	//
	// Returns 0 on success, -1 otherwise (call 'zed_net_get_error' for more info)
	int get_address(netaddress *address, const char *host, unsigned short port);

	// Converts an address's host name into a decimal formatted string
	//
	// Returns NULL on failure (call 'zed_net_get_error' for more info)
	const char* host_to_str(unsigned int host);

	/////////////////////////////////////////////////////////////////////////////////////////
	//
	// UDP SOCKETS API
	//

	// Wraps the system handle for a UDP/TCP socket
	typedef struct {
		int handle;
		int non_blocking;
		int ready;
	} netsocket;

	// Closes a previously opened socket
	void socket_close(netsocket* socket);

	// Opens a UDP socket and binds it to a specified port
	// (use 0 to select a random open port)
	//
	// Socket will not block if 'non-blocking' is non-zero
	//
	// Returns 0 on success
	// Returns -1 on failure (call 'zed_net_get_error' for more info)
	int udp_socket_open(netsocket* socket, unsigned int port, int non_blocking);

	// Closes a previously opened socket
	void socket_close(netsocket* socket);

	// Sends a specific amount of data to 'destination'
	//
	// Returns 0 on success, -1 otherwise (call 'zed_net_get_error' for more info)
	int udp_socket_send(netsocket* socket, netaddress destination, const void *data, int size);

	// Receives a specific amount of data from 'sender'
	//
	// Returns the number of bytes received, -1 otherwise (call 'zed_net_get_error' for more info)
	int udp_socket_receive(netsocket* socket, netaddress* sender, void* data, int size);

	/////////////////////////////////////////////////////////////////////////////////////////
	//
	// TCP SOCKETS API
	//

	// Opens a TCP socket and binds it to a specified port
	// (use 0 to select a random open port)
	//
	// Socket will not block if 'non-blocking' is non-zero
	//
	// Returns NULL on failure (call 'get_error' for more info)
	// Socket will listen for incoming connections if 'listen_socket' is non-zero
	// Returns 0 on success
	// Returns -1 on failure (call 'get_error' for more info)
	int tcp_socket_open(netsocket* socket, unsigned int port, int non_blocking, int listen_socket);

	// Connect to a remote endpoint
	// Returns 0 on success.
	//  if the socket is non-blocking, then this can return 1 if the socket isn't ready
	//  returns -1 otherwise. (call 'get_error' for more info)
	int tcp_connect(netsocket* socket, netaddress remote_addr);

	// Accept connection
	// New remote_socket inherits non-blocking from listening_socket
	// Returns 0 on success.
	//  if the socket is non-blocking, then this can return 1 if the socket isn't ready
	//  if the socket is non_blocking and there was no connection to accept, returns 2
	//  returns -1 otherwise. (call 'get_error' for more info)
	int tcp_accept(netsocket* listening_socket, netsocket* remote_socket, netaddress* remote_addr);

	// Returns 0 on success.
	// if the socket is non-blocking, then this can return 1 if the socket isn't ready
	// returns -1 otherwise. (call 'get_error' for more info)
	int tcp_socket_send(netsocket* remote_socket, const void *data, int size);

	// Returns 0 on success.
	// if the socket is non-blocking, then this can return 1 if the socket isn't ready
	// returns -1 otherwise. (call 'get_error' for more info)
	int tcp_socket_receive(netsocket* remote_socket, void *data, int size);

	// Blocks until the TCP socket is ready. Only makes sense for non-blocking socket.
	// Returns 0 on success.
	// returns -1 otherwise. (call 'get_error' for more info)
	int tcp_make_socket_ready(netsocket* socket);


#endif // INCLUDE_ZED_NET_H