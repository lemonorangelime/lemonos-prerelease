#include <net/net.h>
#include <pit.h>
#include <net/arp.h>
#include <net/socket.h>
#include <memory.h>
#include <util.h>
#include <multitasking.h>

static int socket_id = 0;
static linked_t * socket_interfaces = 0;
static linked_t * sockets = NULL;

// we make these linked list helpers since they dont exist in the library yet
void socket_delete_client(linked_t ** list, socket_client_t * client) {
	linked_t * node = linked_find(*list, linked_find_generic, client);
	if (!node) {
		return;
	}
	*list = linked_delete(node);
}

void socket_append_client(linked_t ** list, socket_client_t * client) {
	*list = linked_add(*list, client);
}

int socket_check_client(linked_t * node, socket_client_handle_t * handle) {
	socket_client_t * client = node->p;
	return client->ip == handle->ip && client->port == handle->port;
}

int socket_check_node(linked_t * node, void * pass) {
	socket_interface_t * interface = node->p;
	uint16_t proto = (uint16_t) (uint32_t) pass; // this is stupid
	return interface->proto == proto;
}

socket_client_t * socket_get_client(socket_t * socket, uint32_t ip, uint16_t port) {
	socket_client_handle_t handle = {ip, port};
	linked_t * node = linked_find(socket->clients, socket_check_client, &handle);
	if (!node) {
		return NULL;
	}
	return node->p;
}

int socket_onrecv_default(socket_t * socket, socket_packet_t * packet) {
	return 0;
}

int socket_request_id() {
	return socket_id++;
}

void socket_write(socket_client_t * client, void * data, size_t size) {
	if (!client) {
		return;
	}
	socket_t * socket = client->socket;
	socket->write(socket, client->mac, client->ip, client->port, data, size);
}

socket_packet_t * socket_read(socket_client_t * client) {
	if (!client) {
		return NULL;
	}
	linked_t * node;
	socket_packet_t * packet;
	int flag = interrupts_enabled();
	disable_interrupts();
	client->packets = linked_shift(client->packets, &node);
	interrupts_restore(flag);
	if (!node) {
		return NULL;
	}
	packet = node->p;
	free(node);
	return packet;
}

socket_t * socket_open(int protocol, uint16_t port, uint32_t op, uint32_t op2) {
	socket_interface_t * interface;
	linked_t * node = linked_find(socket_interfaces, socket_check_node, (void *) protocol);
	if (!node) {
		return NULL;// sorry :c
	}
	interface = node->p;
	return interface->create(port, op, op2);
}

void socket_set_connectionless(socket_t * socket, int state) {
	socket->queue_clients = !state;
}

socket_t * socket_create(socket_write_t write, socket_close_t close, void * priv) {
	socket_t * socket = malloc(sizeof(socket_t));
	socket->id = socket_request_id();
	socket->write = write;
	socket->onrecv = socket_onrecv_default;
	socket->close = close;
	socket->priv = priv;
	socket->client_queue = NULL;
	socket->clients = NULL;
	socket->queue_clients = 1;
	socket->timeout = 0;
	sockets = linked_add(sockets, socket);
	return socket;
}

socket_interface_t * socket_create_interface(int protocol, socket_create_t create) {
	socket_interface_t * interface = malloc(sizeof(socket_interface_t));
	interface->proto = protocol;
	interface->create = create;
	socket_interfaces = linked_add(socket_interfaces, interface);
	return interface;
}

void socket_free_packet(socket_packet_t * packet) {
	if (packet->client) {
		socket_client_t * client = packet->client;
		linked_t * node = linked_find(client->packets, linked_find_generic, packet);
		if (node) {
			client->packets = linked_delete(node);
		}
	}
	free(packet->data);
	free(packet);
}

int socket_packet_destructor(linked_t * node, void * pass) {
	socket_free_packet(node->p);
	return 0;
}

int socket_client_destructor(linked_t * node, void * pass) {
	socket_disconnect(node->p);
	return 0;
}

void socket_close(socket_t * socket) {
	socket->close(socket);
	linked_destroy_all(socket->clients, socket_packet_destructor, NULL);
	free(socket);
}

void socket_disconnect(socket_client_t * client) {
	socket_t * socket = client->socket;
	socket_delete_client(&socket->clients, client);
	socket_delete_client(&socket->client_queue, client);
	linked_destroy_all(client->packets, socket_packet_destructor, NULL);
	free(client);
}

// create a new client for this packet
socket_client_t * socket_create_client(socket_t * socket, uint32_t ip, uint16_t port) {
	socket_client_t * client = malloc(sizeof(socket_client_t));
	client->socket = socket;
	client->ip = ip;
	client->mac = arp_resolve(ip);
	client->port = port;
	client->packets = NULL;
	return client;
}

socket_client_t * socket_create_client_from_packet(socket_t * socket, socket_packet_t * packet) {
	socket_client_t * client = malloc(sizeof(socket_client_t));
	client->socket = socket;
	client->ip = packet->ip;
	client->mac = packet->mac;
	client->port = packet->port;
	client->packets = linked_add(NULL, packet);
	return client;
}

socket_client_t * socket_pop_client(socket_t * socket) {
	linked_t * node;
	socket_client_t * client;
	socket->client_queue = linked_shift(socket->client_queue, &node);
	if (!node) {
		return NULL;
	}
	client = node->p;
	free(node);
	return client;
}

socket_client_t * socket_accept(socket_t * socket) {
	// poll for a new client
	if (!socket->queue_clients) {
		return NULL;
	}
	while (!socket->client_queue) {
		yield();
	}
	return socket_pop_client(socket);
}

socket_client_t * socket_connect(socket_t * socket, uint32_t ip, uint16_t port) {
	socket_client_t * client = socket_create_client(socket, ip, port);
	socket_append_client(&socket->clients, client);
	return client;
}

void socket_timeout(socket_t * socket, int timeout) {
	socket->timeout = timeout;
}

void socket_queue_packet(socket_t * socket, socket_packet_t * packet) {
	socket_client_t * client = socket_get_client(socket, packet->ip, packet->port);
	if (!client) {
		client = socket_create_client_from_packet(socket, packet);
		packet->client = client;
		client->last_packet = ticks;
		socket_append_client(&socket->clients, client);
		if (socket->queue_clients) {
			socket_append_client(&socket->client_queue, client);
		}
		return;
	}
	packet->client = client;
	client->last_packet = ticks;
	client->packets = linked_add(client->packets, packet);
}

int socket_any_ready(socket_t * socket) {
	if (!socket->clients) {
		return 0;
	}
	linked_iterator_t iterator = {socket->clients};
	linked_t * node = socket->clients;
	while (node) {
		socket_client_t * client = node->p;
		if (client->packets) {
			return 1;
		}
		node = linked_step_iterator(&iterator);
	}
	return 0;
}

socket_packet_t * socket_any_read(socket_t * socket) {
	if (!socket->clients) {
		return NULL;
	}
	linked_iterator_t iterator = {socket->clients};
	linked_t * node = socket->clients;
	while (node) {
		socket_client_t * client = node->p;
		if (client->packets) {
			return socket_read(client);
		}
		node = linked_step_iterator(&iterator);
	}
	return NULL;
}

socket_packet_t * socket_any_poll(socket_t * socket) {
	socket_packet_t * packet;
	if (socket->timeout == 0) {
		while (!packet) {
			yield();
			packet = socket_any_read(socket);
		}
		return packet;
	}
	int flags = interrupts_enabled();
	uint64_t target = ticks + (pit_freq * socket->timeout);
	enable_interrupts();
	while (!packet && (target > ticks)) {
		yield();
		packet = socket_any_read(socket);
	}
	interrupts_restore(flags);
	return NULL;
}

socket_packet_t * socket_poll(socket_client_t * client) {
	socket_packet_t * packet;
	socket_t * socket = client->socket;
	if (socket->timeout == 0) {
		while (!packet) {
			yield();
			packet = socket_read(client);
		}
		return packet;
	}
	int flags = interrupts_enabled();
	uint64_t target = ticks + (pit_freq * socket->timeout);
	enable_interrupts();
	while (!packet && (target > ticks)) {
		yield();
		packet = socket_read(client);
	}
	interrupts_restore(flags);
	return NULL;
}

void socket_clean_timed_out(socket_t * socket) {
	linked_iterator_t iterator = {socket->clients};
	linked_t * node = socket->clients;
	uint64_t max_delta = socket->timeout * pit_freq;
	while (node) {
		socket_client_t * client = node->p;
		uint64_t delta = ticks - client->last_packet;
		if (delta > max_delta) {
			socket_disconnect(client);
		}
		node = linked_step_iterator(&iterator);
	}
}

void socket_main() {
	while (1) {
		linked_iterator_t iterator = {sockets};
		linked_t * node = sockets;
		while (node) {
			socket_t * socket = node->p;
			linked_step_iterator(&iterator);

			if (socket->timeout == 0) {
				continue;
			}
			socket_clean_timed_out(socket);
		}
		sleep_ticks(pit_freq);
	}
}

void socket_init() {
	if (!network_enabled) {
		return;
	}
	//create_process(u"socketd", socket_main);
}
