#ifndef CLICK_VEIL_HH
#define CLICK_VEIL_HH
 
#include <click/etheraddress.hh>
#include <click/ipaddress.hh>
#include <click/string.hh>
#include <clicknet/ip.h>
#include <clicknet/ether.h>
#include <clicknet/udp.h>
#include <click/vid.hh>

CLICK_DECLS

//temporary solution to identify VEIL pkts
#define ETHERTYPE_VEIL          0x9876

#define VEIL_HELLO 		0x01	
#define VEIL_RDV_PUBLISH	0x02	
#define VEIL_RDV_QUERY		0x03	
#define VEIL_RDV_REPLY		0x04	
#define VEIL_ARP_REQ		0x05	
#define VEIL_ARP_RPLY		0x06
#define VEIL_PUBLISH		0x07
#define VEIL_IP                 0x08

// Intervals are in ms
#define VEIL_HELLO_INTERVAL		20000
#define VEIL_PUBLISH_INTERVAL		50000
#define VEIL_RDV_INTERVAL		60000
/* value greater than hello/publish interval so entries don't expire just    
 * when hello/publish pkts need to be resent. risking temporarily invalid    
 * routes. 
 */ 
#define VEIL_TBL_ENTRY_EXPIRY		21000


struct veil_header {
	uint16_t packetType;
};

struct access_info{
	IPAddress ip;
	VID vid;
};

struct rdv_reply{
	int k;
	int gatewayvid;
};

CLICK_ENDDECLS
#endif
