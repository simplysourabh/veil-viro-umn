#ifndef CLICK_VEIL_HH
#define CLICK_VEIL_HH
 
#include <click/etheraddress.hh>
#include <click/string.hh>
#include <clicknet/ip.h>
#include <clicknet/ether.h>
#include <clicknet/udp.h>

#include "vid.hh"

CLICK_DECLS

//temporary solution to identify VEIL pkts
#define ETHERTYPE_VEIL          0x1234

#define VEIL_HELLO 		0x01	
#define VEIL_PUBLISH		0x02	
#define VEIL_QUERY		0x04	
#define VEIL_RDVREPLY		0x08	
#define VEIL_ARP		0x10	

// Intervals are in ms
#define VEIL_HELLO_INTERVAL		20000
#define VEIL_PUBLISH_INTERVAL		20000

struct veil_header {
	uint16_t packetType;
};

CLICK_ENDDECLS
#endif
