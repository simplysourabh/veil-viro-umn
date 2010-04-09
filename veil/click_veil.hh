#ifndef CLICK_VEIL_HH
#define CLICK_VEIL_HH
 
#include <click/etheraddress.hh>
#include <click/ipaddress.hh>
#include <click/string.hh>
#include <clicknet/ip.h>
#include <clicknet/ether.h>
#include <clicknet/udp.h>
#include <click/vid.hh>
#include <click/error.hh>
#include <stdarg.h>
#include <stdio.h>

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
#define VEIL_HELLO_INTERVAL		10000
#define VEIL_PUBLISH_INTERVAL		20000
#define VEIL_RDV_INTERVAL		20000
/* value greater than hello/publish interval so entries don't expire just    
 * when hello/publish pkts need to be resent. risking temporarily invalid    
 * routes. 
 */ 
#define VEIL_TBL_ENTRY_EXPIRY		40000

#define PRINT_DEBUG_MSG 0 // 1 = Print the debug messages, 0 = do not print.


struct veil_header {
	uint16_t packetType;
};

struct access_info{
	IPAddress ip;
	VID vid;
};

struct rdv_reply{
	uint16_t k;
	VID gatewayvid;
};

/*inline static void veil_chatter(const char *fmt,...){
	va_list val;
	va_start(val, fmt);
	if (PRINT_DEBUG_MSG == 1){
		click_chatter(fmt,val);
	}
	va_end(val);
}*/

inline 
void veil_chatter(bool printit, const char *fmt, ...){
	if (!printit) return;
	
	va_list val;
	va_start(val, fmt);
	vprintf(fmt,val);
	va_end(val);
}
CLICK_ENDDECLS
#endif
