#include <net/socket.h>
#include <net/net.h>
#include <net/udp.h>
#include <pit.h>
#include <memory.h>
#include <string.h>
#include <util.h>
#include <stdio.h>

static linked_t * udp_sockets = 0;

void udp_socket_write(socket_t * socket, uint64_t mac, uint32_t ip, uint16_t port, void * data, size_t size) {
	udp_packet_t * udp = malloc(sizeof(udp_packet_t) + size);
	udp_priv_t * priv = socket->priv;
	interface_t * interface;
	if (priv->if_id >= 0) {
		interface = net_find_interface(priv->if_id);
	} else if (priv->ip != 0) {
		interface = net_route_interface(priv->ip);
	} else {
		interface = net_get_default();
	}
	if (!interface) {
		return; // dropping, so sorry!
	}
	net_quick_ether(&udp->ether, mac, interface->mac, ETHER_IPv4);
	net_quick_ip(&udp->ip, sizeof(udp_header_t) + size, ticks, IP_PROTO_UDP, ip, interface->ip);
	udp->header.src_port = htonw(priv->port);
	udp->header.dest_port = htonw(port);
	udp->header.length = htonw(8 + size);
	udp->header.checksum = 0;
	memcpy(&udp->data, data, size);
	net_transmit(interface->id, udp, sizeof(udp_packet_t) + size - 1);
}

socket_packet_t * udp_make_userpacket(packet_t * packet) {
	udp_packet_t * udp = packet->data;
	udp = packet->data;

	uint8_t * data = &udp->data;
	uint16_t length = udp->header.length;
	if ((length + 42) > packet->length) {
		// kill whomever sent this packet
		length = packet->length - 42;
	}
	socket_packet_t * userpacket = malloc(sizeof(socket_packet_t));
	memset(userpacket, 0, sizeof(socket_packet_t));
	userpacket->data = malloc(length);
	userpacket->size = length;

	userpacket->ip = udp->ip.src;
	memcpy(&userpacket->mac, udp->ether.src, 6);
	userpacket->port = ntohw(udp->header.src_port);

	userpacket->my_ip = udp->ip.dest;
	memcpy(&userpacket->my_mac, udp->ether.dest, 6);
	userpacket->my_port = ntohw(udp->header.dest_port);

	memcpy(userpacket->data, data, length);
	return userpacket;
}

udp_priv_t * udp_request_socket(uint16_t port) {
	udp_priv_t * priv = malloc(sizeof(udp_priv_t));
	priv->port = port;
	priv->if_id = -1;
	priv->ip = 0;
	return priv;
}

void udp_socket_close(socket_t * socket) {
	// hi
}

socket_t * udp_socket_create(uint16_t port, uint32_t op, uint32_t op2) {
	udp_priv_t * priv = udp_request_socket(port);
	socket_t * socket = socket_create(udp_socket_write, udp_socket_close, priv);
	udp_sockets = linked_add(udp_sockets, socket);
	return socket;
}

int udp_propagate_packet(linked_t * node, void * pass) {
	packet_t * packet = pass;
	socket_t * socket = node->p;
	udp_priv_t * priv = socket->priv;
	udp_packet_t * udp = packet->data;
	if (priv->if_id >= 0) {
		if (priv->if_id != packet->if_id) {
			return 0;
		}
	}
	if (priv->ip != 0) {
		if (priv->ip != udp->ip.dest) {
			return 0;
		}
	}

	if (priv->port != ntohw(udp->header.dest_port)) {
		return 0;
	}

	socket_packet_t * userpacket = udp_make_userpacket(packet);
	if (socket->onrecv(socket, userpacket) == 0) { // return 1 to handle packet
		socket_queue_packet(socket, userpacket);
	} else {
		socket_free_packet(userpacket); // goodbye!
	}

	packet->extra = 1;
}

void udp_reject(interface_t * interface, packet_t * packet) {
	udp_packet_t * udp = packet->data;
	net_quick_icmp_packet(interface, udp->ip.src, ICMP_TYPE_UNREACHABLE, ICMP_CODE_UNREACHABLE_PORT, 0, 0, 0, &udp->ip, packet->length - sizeof(ip_packet_t));
}

void udp_sniff(packet_t * packet) {
	interface_t * interface = net_find_interface(packet->if_id);
	if (!interface) {
		return; // nope!
	}
	packet->extra = 0;
	linked_iterate(udp_sockets, udp_propagate_packet, packet);
	if (packet->extra != 0) {
		return;
	}
	udp_reject(interface, packet);
}

void udp_init() {
	net_sniffer_t * sniffer = net_create_sniffer(udp_sniff);
	sniffer->ether_type = ETHER_IPv4;
	sniffer->ip_protocol = IP_PROTO_UDP;
	sniffer->direction = SNIFFER_INCOMING;
	sniffer->mine = 1;
	net_register_sniffer(sniffer);
	socket_create_interface(SOCKET_UDP, udp_socket_create);
}
