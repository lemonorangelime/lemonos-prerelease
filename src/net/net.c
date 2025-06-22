#include <stdint.h>
#include <stdio.h>
#include <memory.h>
#include <net/net.h>
#include <string.h>
#include <linked.h>
#include <util.h>
#include <net/arp.h>

static process_t * netd;
static int id = 1;
int network_enabled = 1;
linked_t * net_sniffers = 0;
linked_t * network_queue = 0;
linked_t * network_interfaces = 0;

// return next digit after the `.` seperator in an ip
static char * next_digit(char * p) {
	while (*p++ != '.' && *p != 0) {}
	return p;
}

uint16_t net_calculate_checksum(uint16_t * data, size_t length) {
	int left = length;
	int sum = 0;
	while (left > 1) {
		sum += *data++;
		left -= 2;
	}
	if (left == 1) {
		sum += *(uint8_t *) data;
	}
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return (uint16_t) (~sum);
}

void net_quick_ether(ether_header_t * ether, uint64_t dest, uint64_t src, uint16_t type) {
	memcpy(ether->dest, &dest, 6);
	memcpy(ether->src, &src, 6);
	ether->type = htonw(type);
}

void net_quick_ip(ip_header_t * ip, size_t datasize, uint16_t identity, uint8_t proto, uint32_t dest, uint32_t src) {
	ip->version = IP_VERSION_IPv4;
	ip->hlength = sizeof(ip_header_t) / 4;
	ip->service = 0;
	ip->length = htonw(sizeof(ip_header_t) + datasize);
	ip->identity = htonw(identity);
	ip->flags = 0x40;
	ip->ttl = 64;
	ip->protocol = proto;
	ip->src = src;
	ip->dest = dest;
	ip->checksum = 0;
	ip->checksum = net_calculate_checksum((void *) ip, 20);
}

void net_quick_ip_checksum(ip_header_t * ip) {
	ip->checksum = 0;
	ip->checksum = net_calculate_checksum((void *) ip, 20);	
}

int net_ip_is_local(uint32_t ip) {
	if (ip & 0xff000000 == 0x0a000000) { // ip starts with 10.
		return 1;
	}
	if (ip & 0xffff0000 == 0xc0a80000) { // ip starts with 192.168.
		return 1;
	}
	if (ip & 0xff000000 == 0xac000000) { // ip starts with 172.
		uint8_t byte2 = ip >> 16; // they should really name these things...
		return (byte2 >= 16) && (byte2 < 32); // check if second byte is between 16 and 32
	}
	return 0;
}

uint32_t net_ip_mask(uint32_t ip) {
	if (ip & 0xff000000 == 0x0a000000) { // ip starts with 10.
		return 0xff000000;
	}
	if (ip & 0xffff0000 == 0xc0a80000) { // ip starts with 192.168.
		return 0xffff0000;
	}
	if (ip & 0xff000000 == 0xac000000) { // ip starts with 172.
		return 0xffff0000;
	}
	return 0xffffff00;
}

int net_check_network(linked_t * node, void * pass) {
	interface_t * interface = node->p;
	uint32_t ip = (uint32_t) pass;
	uint32_t mask = net_ip_mask(ip);
	return (interface->ip & mask) == (ip & mask);
}

interface_t * net_route_interface(uint32_t ip) {
	// if this is a public internet IP, throw the default interface at them
	if (!net_ip_is_local(ip)) {
		return net_get_default();
	}
	return linked_find(network_interfaces, net_check_network, (void *) ip)->p;
}

uint64_t net_route_hwaddr(interface_t * interface, uint32_t ip) {
	return arp_resolve(ip);
}

void net_quick_ether_swap(ether_header_t * ether) {
	uint64_t src;
	memcpy(&src, ether->src, 6);
	memcpy(ether->src, ether->dest, 6);
	memcpy(ether->dest, &src, 6);
}

void net_quick_ip_swap(ip_header_t * ip) {
	uint32_t src = ip->src;
	ip->src = ip->dest;
	ip->dest = src;
}
int net_quick_arp_replyto(interface_t * interface, void * packet, uint32_t length) {
	if (length < sizeof(arp_packet_t)) {
		return -1;
	}
	arp_packet_t * request = packet;
	if (!interface) {
		interface = net_route_interface(request->header.dest);
		if (!interface) {
			return -1;
		}
	}
	if (ntohw(request->header.operation) != ARP_OPERATION_REQUEST) {
		return -1;
	}
	if (request->header.dest != interface->ip) {
		return -1;
	}
	arp_packet_t * response = malloc(length);
	arp_header_t * header = &response->header;
	memcpy(response, packet, length);
	memcpy(response->ether.dest, response->ether.src, 6);
	memcpy(response->ether.src, &interface->mac, 6);
	header->dest = header->src;
	header->src = interface->ip; // ?

	memcpy(header->hwdest, header->hwsrc, 6);
	memcpy(header->hwsrc, &interface->mac, 6);
	header->operation = htonw(ARP_OPERATION_RESPONSE);
	net_transmit(interface->id, response, length);
	return 0;
}

void net_quick_icmp_redirect() {}
int net_quick_icmp_replyto(interface_t * interface, void * packet, uint32_t length) {
	if (length < sizeof(icmp_packet_t)) {
		return -1;
	}
	icmp_packet_t * request = packet;
	if (!interface) {
		interface = net_route_interface(request->ip.dest);
		if (!interface) {
			return -1;
		}
	}
	if (request->header.type != ICMP_TYPE_ECHO_REQUEST) {
		return -1;
	}
	icmp_packet_t * response = malloc(length);
	memcpy(response, packet, length);
	memcpy(response->ether.dest, response->ether.src, 6);
	memcpy(response->ether.src, &interface->mac, 6);
	net_quick_ip_swap(&response->ip);
	response->header.type = ICMP_TYPE_ECHO_REPLY;
	response->header.code = 0;
	response->header.checksum = 0;
	response->header.checksum = net_calculate_checksum((void *) &response->header, length - sizeof(ip_packet_t));
	net_transmit(interface->id, response, length);
	return 0;
}
void net_quick_icmp_unreachable() {}

void net_quick_icmp_packet(interface_t * interface, uint32_t dest, uint8_t type, uint8_t code, uint16_t icmp_identity, uint16_t seq, uint16_t identity, void * data, size_t length) {
	if (!interface) {
		interface = net_route_interface(dest);
	}
	icmp_packet_t * packet = malloc(sizeof(icmp_packet_t) + length);
	uint64_t hwdest = net_route_hwaddr(interface, dest);
	net_quick_ether(&(packet->ether), hwdest, interface->mac, ETHER_IPv4);
	net_quick_ip(&(packet->ip), 8 + length, identity, IP_PROTO_ICMP, dest, interface->ip);
	packet->header.type = type;
	packet->header.code = code;
	packet->header.identity = htonw(icmp_identity);
	packet->header.sequence = htonw(seq);
	packet->header.checksum = 0;
	memcpy(&packet->data, data, length);
	packet->header.checksum = net_calculate_checksum((void *) &packet->header, 8 + length);
	net_transmit(interface->id, packet, sizeof(icmp_packet_t) + length - 1);
}

uint32_t net_ip_to_int(char * ip) {
	uint32_t ret = 0;
	// convert each number in the xxx.xxx.xxx.xxx to an integer, and then shove it in the result
	ret |= atoi(ip) << 24;
	ret |= atoi(ip = next_digit(ip)) << 16;
	ret |= atoi(ip = next_digit(ip)) << 8;
	ret |= atoi(ip = next_digit(ip));
	return ret;
}

void net_int_to_ip(uint32_t ip, char * str) {
	itoa((ip >> 24) & 0xff, str, 10); str += strlen(str); // omg this is so dumb
	*str++ = '.';
	itoa((ip >> 16) & 0xff, str, 10); str += strlen(str);
	*str++ = '.';
	itoa((ip >> 8) & 0xff, str, 10); str += strlen(str);
	*str++ = '.';
	itoa(ip & 0xff, str, 10);
}

net_sniffer_t * net_create_sniffer(net_sniff_t sniff) {
	net_sniffer_t * sniffer = malloc(sizeof(net_sniffer_t));
	memset(sniffer, 0, sizeof(net_sniffer_t));
	sniffer->ip_protocol = -1;
	sniffer->ether_type = -1;
	sniffer->interface = -1;
	sniffer->direction = -1;
	sniffer->sniff = sniff;
	return sniffer;
}

void net_register_sniffer(net_sniffer_t * sniffer) {
	net_sniffers = linked_add(net_sniffers, sniffer);
}

int net_ip_too_small(uint8_t protocol, size_t length) {
	switch (protocol) { // includes ether and ip
		case IP_PROTO_ICMP:
			return length < 42;
		case IP_PROTO_UDP:
			return length < 42;
		case IP_PROTO_TCP:
			return length < 54;
	}
	return 0; // :shrug:
}

int net_ether_too_small(uint16_t type, size_t length) {
	switch (type) {
		case ETHER_IPv4: // includes ether
			return length < 34;
		case ETHER_ARP:
			return length < 32;
		case ETHER_IPX:
			return length < 44;
	}
	return 0; // :shrug:
}

int net_handle_packet(interface_t * interface, void * p, size_t length) {
	if (length < sizeof(ether_header_t)) {
		return 1; // drop this for sure !
	}

	void * packet = malloc(length);
	memcpy(packet, p, length);

	// we might want to drop this packet depending on how fucked it is
	ether_header_t * ether = packet;
	uint16_t type = ntohw(ether->type);
	if (net_ether_too_small(type, length)) {
		return 1; // drop this !
	}
	if (ether->type == ETHER_IPv4) {
		ip_packet_t * ip = packet;
		uint8_t protocol = ip->ip.protocol;
		if (net_ip_too_small(protocol, length)) {
			return 1; // erhm... i thinks we drop this !
		}
	}
	// someone else can handle all the other cases !
	net_receive(interface->id, packet, length);
	return 0;
}

void net_transmit(int if_id, void * p, size_t size) {
	packet_t * packet = malloc(sizeof(packet_t));
	packet->if_id = if_id;
	packet->data = p;
	packet->length = size;
	packet->outgoing = 1;
	network_queue = linked_add(network_queue, packet);
	netd->killed = 0; // wake up netd to handle this
}

void net_receive(int if_id, void * p, size_t size) {
	packet_t * packet = malloc(sizeof(packet_t));
	packet->if_id = if_id;
	packet->data = p;
	packet->length = size;
	packet->outgoing = 0;
	network_queue = linked_add(network_queue, packet);
	netd->killed = 0; // wake up netd to handle this
}

int net_request_id() {
	return id++;
}

static int ephemeral = 32768;
int net_request_ephemeral() {
	int port = ephemeral++;
	if (port == 65535) {
		ephemeral = 32768;
	}
	return port;
}

interface_t * net_create_interface(uint16_t * drvname, uint16_t * name, interface_send_t send, interface_cmd_t ctrl, int type, void * priv) {
	interface_t * interface = malloc(sizeof(interface_t));
	interface->name = NULL;
	interface->id = net_request_id();
	interface->send = send;
	interface->ctrl = ctrl;
	interface->ip = htond(0);
	interface->mac = 0;
	interface->working = 0;
	interface->mode = MODE_OFF;
	interface->priv = priv;
	interface->speed = 0;
	interface->type = type;
	if (name) {
		interface->name = ustrdup(name);
	} else {
		net_autoname_interface(interface);
	}
	if (drvname) {
		interface->drvname = ustrdup(drvname);
	}
	return interface;
}

uint16_t * net_getname(int type, int id) {
	uint16_t * name = malloc(64);
	switch (type) {
		case IF_UNKNOWN:
			ustrcpy(name, u"if");
			break;
		case IF_ETHER:
			ustrcpy(name, u"eth");
			break;
		case IF_WLAN:
			ustrcpy(name, u"wlan");
			break;
		case IF_VLAN:
			ustrcpy(name, u"vlan");
			break;
		case IF_TELEPHONE:
			ustrcpy(name, u"tel");
			break;
		case IF_BLUETOOTH:
			ustrcpy(name, u"bl");
			break;
		case IF_COM:
			ustrcpy(name, u"com");
			break;
		case IF_LOOPBACK:
			ustrcpy(name, u"loop");
			break;
		case IF_VIRTUAL:
			ustrcpy(name, u"virt");
			break;
		case IF_PRINTER:
			ustrcpy(name, u"print");
			break;
		case IF_CAN:
			ustrcpy(name, u"can");
			break;
	}

	ulldtoustr(id, name + ustrlen(name), 10);
	return name;
}

void net_autoname_interface(interface_t * interface) {
	if (interface->name) {
		free(interface->name);
	}
	interface->name = net_getname(interface->type, interface->id);
}

int net_check_interface(linked_t * node, void * pass) {
	interface_t * interface = node->p;
	return interface->id == (int) pass;
}

int net_default_check_interface(linked_t * node, void * pass) {
	interface_t * interface = node->p;
	return interface->main;
}

interface_t * net_get_default() {
	linked_t * node = linked_find(network_interfaces, net_default_check_interface, 0);
	if (!node) {
		return NULL;
	}
	return node->p;
}

interface_t * net_find_interface(int id) {
	if (id == 0) {
		return net_get_default();
	}
	linked_t * node = linked_find(network_interfaces, net_check_interface, (void *) id);
	if (!node) {
		return NULL;
	}
	return node->p;
}

static int first = 1;
void net_register_interface(interface_t * interface) {
	// printf(u"Registered interface \"%s\" - %dMbps - %s-duplex\n", interface->name, interface->speed, interface->duplex ? u"Full" : u"Half");
	network_interfaces = linked_add(network_interfaces, interface);
	if (first) {
		interface->main = 1;
		first = 0;
	}
}


void net_clear_route(interface_t * interface) {

}

// todo
void net_set_gateway(interface_t * interface, uint32_t gateway) {

}

int net_ctrl(int if_id, int cmd, uint32_t op) {
	interface_t * interface = net_find_interface(if_id);
	if (!interface) {
		return -0xff;
	}

	switch (cmd) {
		case NET_CMD_SET_IP:
			interface->ip = op;
			return 0;
		case NET_CMD_ELECT:
			interface->main = 1;
			net_get_default()->main = 0;
			return 0;
		case NET_CMD_CLEAR_ROUTES:
			net_clear_route(interface);
			return 0;
		case NET_CMD_SET_GATEWAY:
			net_set_gateway(interface, op);
			return 0;
		case NET_CMD_SET_IDENTITY:
			interface->identity = op;
			return 0;
		case NET_CMD_CLEAR_STATS:
			interface->stat_byte_recv = 0;
			interface->stat_byte_sent = 0;
			interface->stat_sent = 0;
			interface->stat_recv = 0;
			return 0;
		case NET_CMD_AUTO_NEGOTIATE:
			if (!interface->autonegotiation) {
				return -1;
			}
			break;
		case NET_CMD_DISABLE:
		case NET_CMD_ENABLE:
			break;
	}

	return interface->ctrl(interface, cmd, op);
}

uint64_t net_read_mac(uint8_t * d) {
	uint64_t mac = 0;
	memcpy(&mac, d, 6);
	return mac;
}

int net_sniffer_match(int if_id, ip_packet_t * packet, net_sniffer_t * sniffer) {
	if (sniffer->interface >= 0) {
		if (sniffer->interface != if_id) {
			return 0;
		}
	}

	uint64_t hwsrc = net_read_mac(packet->ether.src) & sniffer->hwsrc_flag;
	uint64_t hwdest = net_read_mac(packet->ether.dest) & sniffer->hwdest_flag;
	uint16_t type = ntohw(packet->ether.type);
	interface_t * interface;
	if (sniffer->mine) {
		interface = net_find_interface(if_id);
		if (memcmp(packet->ether.dest, &interface->mac, 6) != 0) {
			return 0;
		}
	}
	if (hwsrc != (sniffer->hwsrc & sniffer->hwsrc_flag) || hwdest != (sniffer->hwdest & sniffer->hwdest_flag)) {
		return 0;
	}
	if (sniffer->ether_type >= 0) {
		if (type != sniffer->ether_type) {
			return 0;
		}
	}
	if (type == ETHER_IPv4) {
		uint32_t src = packet->ip.src & sniffer->src_flag;
		uint32_t dest = packet->ip.dest & sniffer->dest_flag;
		if (sniffer->mine) {
			if (packet->ip.dest != interface->ip) {
				return 0;
			}
		}
		if (sniffer->ip_protocol >= 0) {
			if (packet->ip.protocol != sniffer->ip_protocol) {
				return 0;
			}
		}
		if (src != (sniffer->src & sniffer->src_flag) || dest != (sniffer->dest & sniffer->dest_flag)) {
			return 0;
		}
	}
	return 1;
}

int net_propagate_packet(linked_t * node, void * pass) {
	packet_t * packet = pass;
	net_sniffer_t * sniffer = node->p;
	if (sniffer->direction != -1) {
		if (sniffer->direction != packet->outgoing) {
			return 0;
		}
	}
	if (!net_sniffer_match(packet->if_id, packet->data, sniffer)) {
		return 0;
	}
	sniffer->sniff(packet);
}

void netd_main() {
	while (1) {
		linked_t * node = 0;
		disable_interrupts();
		network_queue = linked_shift(network_queue, &node);
		if (!node) {
			netd->killed = 1;
			enable_interrupts();
			continue;
		}
		enable_interrupts();
		packet_t * packet = node->p;
		interface_t * interface = net_find_interface(packet->if_id);
		if (packet->outgoing) {
			if (!interface) {
				// have to drop this... sorry...
				free(packet->data);
				free(packet);
				free(node);
				continue;
			}
			interface->send(interface, packet->data, packet->length);
			interface->stat_sent++;
			interface->stat_byte_sent += packet->length;
		} else {
			interface->stat_recv++;
			interface->stat_byte_recv += packet->length;
		}
		linked_iterate(net_sniffers, net_propagate_packet, packet);
		free(packet->data); // dont need the packet anymore actually
		free(packet);
		free(node);
	}
}

void net_init() {
	if (!network_enabled) {
		return;
	}
	netd = create_process(u"netd", netd_main);
}
