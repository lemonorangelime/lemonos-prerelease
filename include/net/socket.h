#pragma once

#include <linked.h>
#include <net/net.h>

typedef struct socket socket_t;
typedef struct socket_packet socket_packet_t;
typedef struct socket_client socket_client_t;

typedef void (* socket_close_t)(socket_t * socket);
typedef int (* socket_onrecv_t)(socket_t * socket, socket_packet_t * packet);
typedef int * (* socket_poll_t)(socket_t * socket);

typedef socket_packet_t * (* socket_read_t)(socket_t * socket);
typedef void (* socket_write_t)(socket_t * socket, uint64_t mac, uint32_t ip, uint16_t port, void * data, size_t size);

typedef socket_t * (* socket_create_t)(uint16_t port, uint32_t op, uint32_t op2);

typedef struct socket {
	int id; // id: no purpose for us but downstream may use
	// socket_read_t read; // our equivalent of ->read() on an interface_t
	socket_write_t write; // our equivalent of ->write() on an interface_t

	socket_onrecv_t onrecv; // packet receive notification callback
	socket_poll_t poll; // todo: this
	socket_close_t close; // deleter this socket
	linked_t * clients; // client list
	linked_t * client_queue; // client queue
	int queue_clients;
	int timeout;
	void * priv; // state information or whatever you want
} socket_t;

typedef struct {
	int proto;
	socket_create_t create;
} socket_interface_t;

typedef struct socket_packet {
	void * data;
	size_t size;
	socket_client_t * client;
	uint64_t mac;
	uint32_t ip;
	uint16_t port;
	uint64_t my_mac;
	uint32_t my_ip;
	uint16_t my_port;
} socket_packet_t;

typedef struct socket_client {
	socket_t * socket; // socket
	linked_t * packets;
	uint64_t mac;
	uint32_t ip;
	uint16_t port;
	uint64_t last_packet;
} socket_client_t;

typedef struct {
	uint32_t ip;
	uint16_t port;
} socket_client_handle_t;

typedef struct {
	uint16_t sin_family;
	uint16_t sin_port;
	uint32_t sin_addr;
	char sin_zero[8];
} sockaddr_t;

enum {
	SOCKET_RAW,
	SOCKET_UDP,
	SOCKET_TCP,
};

enum {
	SOCK_STREAM = 1,
	SOCK_DGRAM,
	SOCK_RAW,
	SOCK_RDM,
	SOCK_SEQPACKET,
	SOCK_DCCP,
	SOCK_PACKET = 10,
	SOCK_CLOEXEC = 02000000,
	SOCK_NONBLOCK = 00004000,
};

enum {
	PF_UNSPEC = 0,
	PF_LOCAL = 1,
	PF_UNIX = 1,
	PF_FILE = 1,
	PF_INET = 2,
	PF_INET6 = 10,
	PF_PACKET = 17,
	PF_LLC = 26,
	PF_CAN = 29,
	PF_BLUETOOTH = 31,
};

enum {
	AF_UNSPEC = 0,
	AF_LOCAL = 1,
	AF_UNIX = 1,
	AF_FILE = 1,
	AF_INET = 2,
	AF_INET6 = 10,
	AF_PACKET = 17,
	AF_LLC = 26,
	AF_CAN = 29,
	AF_BLUETOOTH = 31,
};

enum {
	SOL_SOCKET = 1,
};

enum {
	SO_RCVTIMEO = 20,
};

socket_t * socket_open(int protocol, uint16_t port, uint32_t op, uint32_t op2);
void socket_timeout(socket_t * socket, int timeout);
void socket_set_connectionless(socket_t * socket, int state);
socket_client_t * socket_get_client(socket_t * socket, uint32_t ip, uint16_t port);
socket_client_t * socket_accept(socket_t * socket);
int socket_any_ready(socket_t * socket);
socket_packet_t * socket_any_read(socket_t * socket);
socket_packet_t * socket_read(socket_client_t * client);
socket_packet_t * socket_any_poll(socket_t * socket);
socket_packet_t * socket_poll(socket_client_t * client);
void socket_write(socket_client_t * client, void * data, size_t size);
socket_client_t * socket_connect(socket_t * socket, uint32_t ip, uint16_t port);
void socket_disconnect(socket_client_t * client);
void socket_close(socket_t * socket);

// internal API
socket_interface_t * socket_create_interface(int protocol, socket_create_t create);
socket_t * socket_create(socket_write_t write, socket_close_t close, void * priv);
void socket_queue_packet(socket_t * socket, socket_packet_t * packet);
void socket_free_packet(socket_packet_t * packet);
void socket_init();
