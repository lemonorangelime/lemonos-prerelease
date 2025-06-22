#pragma once

#include <linked.h>
#include <stdint.h>

typedef struct interface interface_t;
typedef struct packet packet_t;

typedef void (* interface_send_t)(interface_t * interface, void * packet, size_t size);
typedef int (* interface_cmd_t)(interface_t * interface, int cmd, uint32_t op);

typedef void (* net_sniff_t)(packet_t * packet);

typedef struct {
	net_sniff_t sniff;
	uint32_t src;
	uint32_t dest;
	uint32_t src_flag;
	uint32_t dest_flag;
	uint64_t hwsrc;
	uint64_t hwdest;
	uint64_t hwsrc_flag;
	uint64_t hwdest_flag;
	int ip_protocol;
	int ether_type;
	int interface;
	// what goes here?
	int direction;
	int mine;
} net_sniffer_t;

typedef struct interface {
	uint16_t * name;
	uint16_t * drvname;
	int id;
	int type;
	interface_send_t send;
	interface_cmd_t ctrl;
	uint32_t ip;
	uint64_t mac;
	uint16_t identity;
	int working;
	int mode;
	int main;
	int speed;
	int autonegotiation;
	int duplex;
	int loopback;
	void * priv; // private data
	uint32_t gateway;
	uint64_t gateway_mac;
	linked_t routes;
	int stat_sent;
	int stat_recv;
	int stat_byte_sent;
	int stat_byte_recv;
} interface_t;

typedef struct packet {
	int if_id;
	void * data;
	size_t length;
	int outgoing;
	int extra;
} packet_t;

typedef struct {
	uint8_t dest[6];
	uint8_t src[6];
	uint16_t type;
} __attribute__((packed)) ether_header_t; // 14 bytes

typedef struct {
	uint8_t hlength : 4;
	uint8_t version : 4;
	uint8_t service;
	uint16_t length;
	uint16_t identity;
	uint16_t flags;
	uint8_t ttl;
	uint8_t protocol;
	uint16_t checksum;
	uint32_t src;
	uint32_t dest;
} __attribute__((packed)) ip_header_t; // 20 bytes

typedef struct {
	uint16_t src_port;
	uint16_t dest_port;
	uint32_t seq;
	uint32_t next_seq;
	uint16_t length : 4;
	uint16_t reserved : 3;
	uint16_t flags : 9;
	uint16_t window;
	uint16_t checksum;
	uint16_t urgent;
} __attribute__((packed)) tcp_header_t; // 20 bytes

typedef struct {
	uint16_t src_port;
	uint16_t dest_port;
	uint16_t length;
	uint16_t checksum;
} __attribute__((packed)) udp_header_t; // 8 bytes

typedef struct {
	uint16_t type;
	uint16_t protocol;
	uint8_t length;
	uint8_t protocol_length;
	uint16_t operation; // only two values, why is this 16 bits?
	uint8_t hwsrc[6];
	uint32_t src;
	uint8_t hwdest[6];
	uint32_t dest;
} __attribute__((packed)) arp_header_t; // 28 bytes

typedef struct {
	uint8_t type;
	uint8_t code;
	uint16_t checksum;
	uint16_t identity;
	uint16_t sequence;
	uint8_t data;
} __attribute__((packed)) icmp_header_t; // 8 bytes

typedef struct {
	ether_header_t ether;
	ip_header_t ip;
} __attribute__((packed)) ip_packet_t; // 34 bytes


typedef struct {
	ether_header_t ether;
	ip_header_t ip;
	icmp_header_t header;
	uint8_t data;
} __attribute__((packed)) icmp_packet_t; // 42 bytes

typedef struct {
	ether_header_t ether;
	ip_header_t ip;
	udp_header_t header;
	uint8_t data;
} __attribute__((packed)) udp_packet_t; // 42 bytes

typedef struct {
	ether_header_t ether;
	ip_header_t ip;
	tcp_header_t header;
	uint8_t data;
} __attribute__((packed)) tcp_packet_t; // 64 bytes

typedef struct {
	ether_header_t ether;
	arp_header_t header;
} __attribute__((packed)) arp_packet_t; // 42 bytes

enum {
	IF_UNKNOWN = 0,
	IF_ETHER,
	IF_WLAN,
	IF_VLAN,
	IF_TELEPHONE,
	IF_BLUETOOTH,
	IF_COM,
	IF_LOOPBACK,
	IF_VIRTUAL,
	IF_PRINTER,
	IF_CAN,
};

enum {
	MODE_OFF, // turn it off / disable receiving packets
	MODE_MANAGED, // receive only packets meant for us
	MODE_MONITOR, // receive all packets
};

enum {
	SNIFFER_INCOMING, // self explainitory
	SNIFFER_OUTGOING,
};

enum {
	NET_CMD_SET_MODE,
	NET_CMD_SET_MAC_LOW,
	NET_CMD_SET_MAC_HIGH,
	NET_CMD_SET_IP,
	NET_CMD_ELECT,
	NET_CMD_SET_SPEED,
	NET_CMD_DISABLE,
	NET_CMD_ENABLE,
	NET_CMD_CLEAR_ROUTES,
	NET_CMD_SET_GATEWAY,
	NET_CMD_SET_IDENTITY,
	NET_CMD_CLEAR_STATS,
	NET_CMD_AUTO_NEGOTIATE,
	NET_CMD_SET_DUPLEX,
};

enum {
	ETHER_IPv4 = 0x0800,
	ETHER_ARP  = 0x0806,
	ETHER_RING = 0x0842, // ring ring, ring ring, hello? hello??
	ETHER_IPX  = 0x8137,
	ETHER_IPv6 = 0x86dd,
	ETHER_SCSI = 0x889a, // wireless SCSI anything, cool!
	ETHER_ATA  = 0x88a2, // wireless ATA hard drive, cool!
	ETHER_LLDP = 0x88cc,
};

enum {
	ARP_OPERATION_REQUEST  = 1,
	ARP_OPERATION_RESPONSE = 2,
};

enum {
	IP_VERSION_IPv4 = 4,
	IP_VERSION_IPv6 = 6,

	IP_PROTO_ICMP  = 1,
	IP_PROTO_GTG   = 2,
	IP_PROTO_TCP   = 6,
	IP_PROTO_CHAOS = 16,
	IP_PROTO_UDP   = 17,
};

enum {
	ICMP_TYPE_ECHO_REPLY      = 0,
	ICMP_TYPE_UNREACHABLE     = 3,
	ICMP_TYPE_SRC_QUENCH      = 4,
	ICMP_TYPE_REDIRECT        = 5,
	ICMP_TYPE_ECHO_REQUEST    = 8,
	ICMP_TYPE_ROUTER_ANNOUNCE = 9,
	ICMP_TYPE_TIMEOUT         = 10,
	ICMP_TYPE_TRACE           = 30,

	ICMP_CODE_UNREACHABLE_NETWORK      = 0,
	ICMP_CODE_UNREACHABLE_HOST         = 1,
	ICMP_CODE_UNREACHABLE_PROTOCOL     = 2,
	ICMP_CODE_UNREACHABLE_PORT         = 3,
	ICMP_CODE_UNREACHABLE_ROUTING_FAIL = 5,
	ICMP_CODE_UNREACHABLE_NET_UNKNOWN  = 6,
	ICMP_CODE_UNREACHABLE_HOST_UNKNOWN = 7,
	ICMP_CODE_UNREACHABLE_AIRGAP       = 8,
	ICMP_CODE_UNREACHABLE_NET_ILLEGAL  = 9,
	ICMP_CODE_UNREACHABLE_HOST_ILLEGAL = 10,
	ICMP_CODE_UNREACHABLE_ILLEGAL      = 13,

	ICMP_CODE_REDIRECT_NETWORK = 0,
	ICMP_CODE_REDIRECT_HOST    = 1,

	ICMP_CODE_ROUTER_ANNOUNCE             = 0,
	ICMP_CODE_ROUTER_ANNOUNCE_NON_ROUTING = 1, // fuck is point of a non-routing router

	ICMP_CODE_TIMEOUT_TTL        = 0,
	ICMP_CODE_TIMEOUT_REASSEMBLY = 1,
};

enum {
	UDP_PORT_DYNAMIC = 0,
	UDP_PORT_LEMONOS = 4,
	UDP_PORT_SHELL   = 6,
	UDP_PORT_ECHO    = 7,
	UDP_PORT_WoL     = 9,
	UDP_PORT_DAYTIME = 13,
	UDP_PORT_QOTD    = 17,
	UDP_PORT_TELNET  = 23,
	UDP_PORT_TIME    = 37,
	UDP_PORT_DNS     = 53,
	UDP_PORT_TFTP    = 69,
	UDP_PORT_HTTP    = 80,
	UDP_PORT_NTP     = 123,
	UDP_PORT_IRC     = 194,
	UDP_PORT_SYSLOG  = 514,
	UDP_PORT_TALK    = 517,
	UDP_PORT_DOOM    = 666,

	TCP_PORT_DYNAMIC = 0,
	TCP_PORT_ECHO    = 7,
	TCP_PORT_DAYTIME = 13,
	TCP_PORT_QOTD    = 17,
	TCP_PORT_FTP     = 20,
	TCP_PORT_FTPC    = 21,
	TCP_PORT_SSH     = 22,
	TCP_PORT_TELNET  = 23,
	TCP_PORT_SMTP    = 25,
	TCP_PORT_TIME    = 37,
	TCP_PORT_HTTP    = 80,
	TCP_PORT_IRC     = 194,
	TCP_PORT_REXEC   = 512,
	TCP_PORT_RLOGIN  = 513,
	TCP_PORT_DOOM    = 666,
	TCP_PORT_iSCSI   = 860,
};

extern linked_t * net_sniffers;
extern linked_t * network_queue;
extern linked_t * network_interfaces;
extern int network_enabled;

uint16_t net_calculate_checksum(uint16_t * data, size_t length);
void net_quick_ether(ether_header_t * ether, uint64_t dest, uint64_t src, uint16_t type);
void net_quick_ip(ip_header_t * ip, size_t datasize, uint16_t identity, uint8_t proto, uint32_t dest, uint32_t src);
int net_ip_is_local(uint32_t ip);
uint32_t net_ip_mask(uint32_t ip);
void net_quick_ip_checksum(ip_header_t * ip);
int net_check_network(linked_t * node, void * pass);
interface_t * net_route_interface(uint32_t ip);
uint64_t net_route_hwaddr(interface_t * interface, uint32_t ip);
void net_quick_ether_swap(ether_header_t * ether);
void net_quick_ip_swap(ip_header_t * ip);
int net_quick_arp_replyto(interface_t * interface, void * packet, uint32_t length);
void net_quick_icmp_redirect();
int net_quick_icmp_replyto(interface_t * interface, void * packet, uint32_t length);
void net_quick_icmp_unreachable();
void net_quick_icmp_packet(interface_t * interface, uint32_t dest, uint8_t type, uint8_t code, uint16_t icmp_identity, uint16_t seq, uint16_t identity, void * data, size_t length);
uint32_t net_ip_to_int(char * ip);
void net_int_to_ip(uint32_t ip, char * str);
net_sniffer_t * net_create_sniffer(net_sniff_t sniff);
void net_register_sniffer(net_sniffer_t * sniffer);
int net_ip_too_small(uint8_t protocol, size_t length);
int net_ether_too_small(uint16_t type, size_t length);
int net_handle_packet(interface_t * interface, void * p, size_t length);
void net_transmit(int if_id, void * p, size_t size);
void net_receive(int if_id, void * p, size_t size);
int net_request_id();
int net_request_ephemeral();
interface_t * net_create_interface(uint16_t * drvname, uint16_t * name, interface_send_t send, interface_cmd_t ctrl, int type, void * priv);
interface_t * net_get_default();
interface_t * net_find_interface(int id);
void net_autoname_interface(interface_t * interface);
void net_register_interface(interface_t * interface);
void net_clear_route(interface_t * interface);
void net_set_gateway(interface_t * interface, uint32_t gateway);
int net_ctrl(int if_id, int cmd, uint32_t op);
uint64_t net_read_mac(uint8_t * d);
int net_sniffer_match(int if_id, ip_packet_t * packet, net_sniffer_t * sniffer);
int net_propagate_packet(linked_t * node, void * pass);
void net_init();
