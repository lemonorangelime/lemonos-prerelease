#include <net/net.h>
#include <string.h>
#include <memory.h>
#include <util.h>
#include <net/arp.h>
#include <stdio.h>
#include <pit.h>
#include <graphics/graphics.h>

arp_entry_t arp_table[ARP_TABLE_SIZE] = {};
int arp_table_top = 0;

void arp_responder(packet_t * packet) {
	arp_packet_t * request = packet->data;
	arp_header_t * header = &request->header;
	interface_t * interface = net_find_interface(packet->if_id);
	if (!interface) { // ???
		return;
	}
	if (interface->ip == 0) {
		return; // we COULD respond without an IP, but I don't think we should, dropped!
	}
	if ((packet->length < sizeof(arp_packet_t)) || (request->header.dest != interface->ip)) {
		return; // bad packet / not for us
	}
	if (ntohw(request->header.operation) == ARP_OPERATION_REQUEST) {
		net_quick_arp_replyto(interface, packet->data, packet->length);
		return;
	}
	// response to us, add to table
	arp_entry_t * entry = &arp_table[arp_table_top++];
	entry->address = header->src;
	memcpy(&entry->mac, header->hwsrc, 6);

	if (arp_table_top == ARP_TABLE_SIZE) {
		arp_table_top = 0;
	}
}

void arp_request(uint32_t address) {
	interface_t * interface = net_route_interface(address);
	arp_packet_t * response = malloc(sizeof(arp_packet_t));
	arp_header_t * header = &response->header;
	response->ether.type = htonw(ETHER_ARP);
	memset(response->ether.dest, 0xff, 6);
	memcpy(response->ether.src, &interface->mac, 6);
	header->type = htonw(0x0001);
	header->protocol = htonw(ETHER_IPv4);
	header->length = 6;
	header->protocol_length = 4;
	header->operation = htonw(ARP_OPERATION_REQUEST);
	header->dest = address;
	header->src = interface->ip;

	memset(header->hwdest, 0x00, 6); // why 00:00:00:00:00:00 ??
	memcpy(header->hwsrc, &interface->mac, 6);
	net_transmit(interface->id, response, sizeof(arp_packet_t));
}

uint64_t arp_lookup(uint32_t address) {
	for (int i = 0; i < ARP_TABLE_SIZE; i++) {
		arp_entry_t * entry = &arp_table[i];
		if (entry->address == 0) {
			return 0;
		}
		if (entry->address == address) {
			return entry->mac;
		}
	}
	return 0;
}

uint64_t arp_wait_timeout(uint32_t address) {
	uint64_t mac = 0;
	uint64_t target = ticks + pit_freq;
	while (!mac && (target > ticks)) {
		mac = arp_lookup(address);
		yield();
	}
	return mac;
}

uint64_t arp_resolve(uint32_t address) {
	uint64_t mac = arp_lookup(address);
	if (mac != 0) {
		return mac; // cached
	}
	for (int i = 0; i < 6; i++) {
		arp_request(address);
		mac = arp_wait_timeout(address);
		if (mac) {
			break;
		}
	}
	return mac;
}

void arp_init() {
	if (!network_enabled) {
		return; // no network driver loaded :shrug:
	}
	memset(arp_table, 0, sizeof(arp_table));

	// sniff all ARP packets
	net_sniffer_t * sniffer = net_create_sniffer(arp_responder);
	sniffer->ether_type = ETHER_ARP;
	sniffer->direction = SNIFFER_INCOMING; // only incoming packets
	net_register_sniffer(sniffer);
}
