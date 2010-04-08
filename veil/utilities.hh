#ifndef UTILITIES_HH
#define UTILITIES_HH
 
#include <clicknet/ip.h>
#include <click/vid.hh>
#include <click/ipaddress.hh>
#include <click/packet_anno.hh>
#include <click/packet.hh>
#include <click/error.hh>
#include "click_veil.hh"

CLICK_DECLS

//this is for access and rdv point rerouting
#define REROUTE_ANNO_OFFSET       20
#define REROUTE_ANNO(p)           ((p)->anno_u8(Packet::user_anno_offset + REROUTE_ANNO_OFFSET))
#define SET_REROUTE_ANNO(p, v)    ((p)->set_anno_u8(Packet::user_anno_offset + REROUTE_ANNO_OFFSET, (v)))

//set port number annotation for all incoming packets
#define PORT_ANNO_OFFSET          10
#define PORT_ANNO(p)              ((p)->anno_u32(Packet::user_anno_offset + PORT_ANNO_OFFSET))
#define SET_PORT_ANNO(p, v)       ((p)->set_anno_u32(Packet::user_anno_offset + PORT_ANNO_OFFSET, (v)))

//TODO: find a good hash function. 
//shouldn't get an all 0's vid or something equally crazy
//we don't want hash clashes either
static VID calculate_access_switch(IPAddress *ip){
	unsigned char* data = ip->data();
	unsigned char val[VID_LEN];
	memset (val, 0, sizeof(val));	
	val[0] = data[0] >> 4 & val[1];
	val[1] = data[1] << 2;
	val[2] = data[2] >> 1 ^ val[3];
	val[3] = data[3] << 5; 
	return VID(static_cast<const unsigned char*>(val));
}

CLICK_ENDDECLS
#endif
