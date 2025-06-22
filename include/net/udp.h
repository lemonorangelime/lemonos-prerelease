#pragma once

typedef struct {
	uint16_t port;
	uint32_t ip;
	int if_id;
} udp_priv_t;

void udp_socket_write(socket_t * socket, uint64_t mac, uint32_t ip, uint16_t port, void * data, size_t size);
udp_priv_t * udp_request_socket(uint16_t port);
socket_t * udp_socket_create(uint16_t port, uint32_t op, uint32_t op2);
void udp_init();