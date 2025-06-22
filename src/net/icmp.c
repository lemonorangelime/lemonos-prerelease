#include <net/net.h>
#include <stdio.h>

// reply to ICMP packets
void icmp_responder(packet_t *packet) {
	interface_t *interface = net_find_interface(packet->if_id);
	if (!interface) { // ???
		return;
	}
	net_quick_icmp_replyto(interface, packet->data, packet->length);
}

void icmp_init() {
	if (!network_enabled) {
		return; // no network driver loaded :shrug:
	}

	// sniff all ICMP packets
	net_sniffer_t *sniffer = net_create_sniffer(icmp_responder);
	sniffer->ether_type = ETHER_IPv4;
	sniffer->ip_protocol = IP_PROTO_ICMP;
	sniffer->direction = SNIFFER_INCOMING; // only incoming packets
	sniffer->mine = 1;
	net_register_sniffer(sniffer);
}
