#ifndef CLICK_VEIL_HH
#define CLICK_VEIL_HH
 
#include <click/etheraddress.hh>
#include <click/ipaddress.hh>
#include <click/element.hh>
#include <click/string.hh>
#include <clicknet/ip.h>
#include <clicknet/ether.h>
#include <clicknet/udp.h>
#include <click/vid.hh>
#include <click/error.hh>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

CLICK_DECLS

//temporary solution to identify VEIL pkts
#define ETHERTYPE_VEIL		0x9876
#define ETHERTYPE_VEIL_IP   0x9878

#define VEIL_ARP_REQ		0x05	
#define VEIL_ARP_RPLY		0x06
#define VEIL_PUBLISH		0x07
#define VEIL_IP             0x08


// Intervals are in ms
#define VEIL_HELLO_INTERVAL		10000
#define VEIL_PHELLO_INTERVAL	10000
#define VEIL_PUBLISH_INTERVAL	10000
#define VEIL_RDV_INTERVAL		10000


/* value greater than hello/publish interval so entries don't expire just    
 * when hello/publish pkts need to be resent. risking temporarily invalid    
 * routes. 
 */ 
#define VEIL_TBL_ENTRY_EXPIRY	20000
#define MAX_TTL 				0x0f

#define VEIL_SPANNING_TREE_ENTRY_EXPIRY 20000 // an entry for the veil spanning tree will expire after
// 20 seconds.
#define VEIL_SPANNING_TREE_COST_BROADCAST 10000 // update the cost every 10 seconds.
#define VEIL_VID_BROADCAST_TIME 20000

#define MAX_PACKET_SIZE	1000 // maximum packet size in bytes.
// Maximum number of gateways stored in each Bucket
#define MAX_GW_PER_BUCKET		4

#define PRINT_DEBUG_MSG 0 // 1 = Print the debug messages, 0 = do not print.


// Temporarily putting VEIL_PACKET.HH FILE HERE
//-------------------------------------START----------------------------------------
// ETHERNET HEADER STRUCTURE
// |-----------------------------------------------------------|
// |SOURCE MAC (6 BYTES) | DEST MAC (6 BYTES) | TYPE (2 BYTES) |
// |-----------------------------------------------------------|
// Values for the "ether_type" in the Ethernet header:
// eth->ether_type = ETHERTYPE_IP (for the IP packet)
// eth->ether_type = ETHERTYPE_ARP (for the arp packet)
#define ETHERTYPE_VEIL          0x9876
#define ETHERTYPE_VEIL_IP   	0x9878

// (Neighbor discovery) : Exchanged between "directly connected" interfaces.
// Source and destination fields correspond to "Unique ID" for each interface
// (just like MAC address), the uniqueness is with in the "veil network".
#define VEIL_HELLO				0x0000
#define VEIL_PHELLO				0x0001

//[VEIL-Type = 0x0101] SP_AD (Spanning Tree Advertisement) : This is the packet
// broadcasted by a switch on all its outgoing interfaces. It contains the following
// information: svid = 0...0, dvid = 0...0, ttl = 1, payload contains:
// vcc_mac = mac address of the veil centralized controller,
// cost (2-bytes long) : integer containing number of physical hops to reach the vcc.
#define VEIL_VCC_SPANNING_TREE_AD 		0x0101

// [VEIL-Type = 0x0102] SP_SUB (Spanning Tree Subscription) : The veil-payload for
// this type of packet will contain the following: vcc_mac.
#define VEIL_VCC_SPANNING_TREE_SUBS 	0x0102

// [VEIL-Type = 0x0103] LTOPO (Local topology information): If the packet is
// divided into multiple chunks then "optional" field will contain the chunk id.
// The veil-payload will contain the following info: vcc_mac (the destination of
		//the packet) number of neighbors in the payload, as 2 byte short integer,
//		and then followed by the mac ids for all those interfaces.
#define VEIL_LOCAL_TOPO_TO_VCC 			0x0103

// [VEIL-Type = 0x0104] SVID_LIST (List of Switch VIDs for the all switches
// in the network: Just like other 0x08xx packets, we'll ignore the svid, dvid
// fields in the headers. The payload consists of the following info: # of
// mappings (2 bytes), <mac,vid> pairs.
#define VEIL_SWITCH_VID_FROM_VCC 	0x0104

// RENDEZVOUS PACKETS: PUBLISH, QUERY, REPLY
#define VEIL_RDV_REPLY			0x0402
#define VEIL_RDV_PUBLISH		0x0403
#define VEIL_RDV_QUERY			0x0404

// From a "host switch" to the "access-switch", it contains the IP,MAC to VID mapping
#define VEIL_MAP_PUBLISH		0x0201
//From the "acces-switch" to "old-host-switch", when a switch receives a mapping from
// IP,MAC to VID for which it already has a mapping then it informs the old-host-switch
// that host has moved.  Upon receiving this packet host will inform all the host-devices
// connected to it by sending an ARP Reply packet to all of them.
#define VEIL_MAP_UPDATE			0x0202


// An encapsulated IP packet. source and dest will be the VIDs of the end-hosts.
#define VEIL_ENCAP_IP			0x0601
// An encapsulated ARP packet, source and dest will be the VIDs of the end-hosts.
#define VEIL_ENCAP_ARP			0x0602
// ENCAP_REDIRECTION: A data packet which has been "redirected" by the "old-host-switch".
#define VEIL_ENCAP_REDIRECTION	0x0603

// TYPE for the MULTIPATH enabled packet forwarding:
#define VEIL_ENCAP_MULTIPATH_IP	0x0801

//Different forwarding options for the IP packets, however it can
// be over-ridden at command line when VEILProcessIP is instantiated.
#define IP_FORWARDING_TYPE							1 // can be 0 or 1 or 2
// set it to 1 if we want to enable the ENACAPSULATION of
// IP packets as IP packets.
#define IP_FORWARDING_TYPE_ENCAP 					1
// else set it to 0:
// When set as 0, We simply overwrite the MAC addresses on the
// Ethernet header, and it won't allow VEIL_SWITCHES to be interconnected using ETHERNET SWITCHES.
// i.e. VEIL_SWITCHES must be WIRED directly to each other.
#define IP_FORWARDING_TYPE_ADD_OVERWRITE 			0
// if it is set to 2: then "MULTIPATH ROUTING" is enabled for the IP packets.
#define IP_FORWARDING_TYPE_MULTIPATH				2
// REMOVE THIS LATER START

#define min(x,y) (x < y ? x : y)
#define max(x,y) (x > y ? x : y)

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

// REMOVE THIS LATER END

// VEIL sub HEADER STRUCTURE
//              |------------------- 4 BYTE LONG------------------|
// offset =  0  |           first 4 bytes of src vid              |
// offset =  4  |   last 2 bytes of svid : first 2 bytes dvid     |
// offset =  8  |   last 4 bytes of dvid : first 2 bytes dvid     |
// offset = 12  | 2 bytes veil-type : 1 byte ttl: 1 byte optional |
//              |-------------------------------------------------|
struct veil_sub_header{
	VID svid;
	VID dvid;
	uint16_t veil_type;
	uint8_t ttl;
	uint8_t optional;
};


// VEIL PAYLOAD STRUCTURE
// Type: VEIL_ENCAP_IP/ARP/REDIRECTION
// Payload: ARP/IP packet

// Type: VEIL_HELLO
// Payload: None

// Type: VEIL_RDV_PUBLISH
// |Bucket 1 byte : Optional 1 byte : Neighbor switch vid 4 bytes|
struct veil_payload_rdv_publish{
	uint8_t bucket;
	uint8_t optional ;
	uint8_t neighbor_vid[4];
};

// Type: VEIL_RDV_QUERY
// |Bucket 1 byte : Optional 1 byte|
struct veil_payload_rdv_query{
	uint8_t bucket;
	uint8_t optional;
};

// Type: VEIL_RDV_REPLY
// GW_COUNT = 1   (if only default gateway is supplied)
// GW_COUNT = k+1 (if additional k gateways are also supplied)
// |Bucket 1 byte : GW_COUNT 1 byte : Gateway1 switch vid 4 bytes : Gateway2 switch vid 4 bytes :...: Gateway(k+1) switch vid 4 bytes|
struct veil_payload_rdv_reply{
	uint8_t bucket;
	uint8_t gw_count ;
	uint8_t gw_vid[MAX_GW_PER_BUCKET][4];
};

// Type: VEIL_RDV_PUBLISH
// |Bucket 1 byte : Optional 1 byte : Neighbor switch vid 4 bytes|
struct veil_payload_map_publish{
	IPAddress ip;
	EtherAddress mac;
	VID vid;
};

// Type: VEIL_MAP_PUBLISH or VEIL_MAP_UPDATE
// |IP address 4 bytes : MAC address 6 bytes : VID 6 bytes|
struct veil_payload_map_update{
	IPAddress ip;
	EtherAddress mac;
	VID vid;
};

// Type: VEIL_ENCAP_MULTIPATH_IP
// |Fianl VID 6 bytes|
struct veil_payload_multipath{
	VID final_dvid;
};

// payload structure for the VEIL_VCC_SPANNING_TREE_AD
// |MAC 6 bytes : # of hops to VCC 2 bytes|
struct veil_payload_vcc_sp_tree_ad{
	EtherAddress vccmac;
	uint16_t cost;
};

// payload structure for the VEIL_VCC_SPANNING_TREE_SUBS
// |MAC 6 bytes|
struct veil_payload_vcc_sp_tree_sub{
	EtherAddress vccmac;
};

// payload structure for the VEIL_LOCAL_TOPO_TO_VCC
// |# of neighbors. : neighbor 1: neighbor 2: ... : neighbor n|
struct veil_payload_ltopo_to_vcc{
	uint16_t neighbor_count;
};

// payload structure for the VEIL_SWITCH_VID_FROM_VCC
// |vid|
struct veil_payload_svid_mappings{
	VID vid;
};



// List of functions that we might be interested in:


// ETHER HEADER START -----------------------------------------
//------------------- GET METHODS ----------------------------
	inline EtherAddress getEtherSrcAddress(const Packet *p){
		click_ether *eth = (click_ether*) p->data();
		EtherAddress smac = EtherAddress(eth->ether_shost);
		return smac;
	}

	inline EtherAddress getEtherDstAddress(const Packet *p){
		click_ether *eth = (click_ether*) p->data();
		EtherAddress dmac = EtherAddress(eth->ether_dhost);
		return dmac;
	}

	inline uint16_t getEtherType(const Packet *p){
		click_ether *eth = (click_ether*) p->data();
		uint16_t type = ntohs(eth->ether_type);
		return type;
	}

//------------------- SET METHODS ----------------------------
	inline Packet* setEtherSrcAddress (Packet *p, EtherAddress smac){
		click_ether *eth = (click_ether*) p->data();
		memcpy(eth->ether_shost, &smac, 6);
		return p;
	}

	inline Packet* setEtherDstAddress (Packet *p, EtherAddress dmac){
		click_ether *eth = (click_ether*) p->data();
		memcpy(eth->ether_dhost, &dmac, 6);
		return p;
	}

	inline Packet* setEtherType(Packet *p, uint16_t type){
		click_ether *eth = (click_ether*) p->data();
		uint16_t type1 = htons(type);
		eth->ether_type = type1;
		return p;
	}

	inline bool isZeroVID(VID vid){
		VID zerovid;
		memset(&zerovid,0,6);
		return zerovid == vid;
	}

// ETHER HEADER END -----------------------------------------


// VEIL sub HEADER START ------------------------------------
//------------------- GET METHODS ----------------------------
	inline VID getSrcVID(const Packet *p){
		const click_ether *eth = (const click_ether *) p->data();
		const veil_sub_header *veil_sub = (const veil_sub_header *) (eth+1);
		VID svid;
		memcpy(&svid, &(veil_sub->svid),6);
		return svid;
	}

	inline VID getDstVID(const Packet *p){
		const click_ether *eth = (const click_ether *) p->data();
		const veil_sub_header *veil_sub = (const veil_sub_header *) (eth+1);
		VID dvid;
		memcpy(&dvid, &(veil_sub->dvid),6);
		return dvid;
	}

	inline uint16_t getVEILType(const Packet *p){
		uint16_t type;
		const click_ether *eth = (const click_ether *) p->data();
		const veil_sub_header *veil_sub = (const veil_sub_header *) (eth+1);
		type = ntohs(veil_sub->veil_type);
		return type;
	}

	inline uint8_t getVEILTTL(const Packet *p){
		const click_ether *eth = (const click_ether *) p->data();
		const veil_sub_header *veil_sub = (const veil_sub_header *) (eth+1);
		return veil_sub->ttl;
	}
//------------------- SET METHODS ----------------------------
	inline Packet * setSrcVID(Packet *p, VID svid){
		click_ether *eth = (click_ether *) p->data();
		veil_sub_header *veil_sub = (veil_sub_header *) (eth+1);
		memcpy(&(veil_sub->svid), &svid, 6);
		return p;
	}

	inline Packet * setDstVID(Packet *p, VID dvid){
		click_ether *eth = (click_ether *) p->data();
		veil_sub_header *veil_sub = (veil_sub_header *) (eth+1);
		memcpy(&(veil_sub->dvid), &dvid, 6);
		return p;
	}

	inline Packet * setVEILType(Packet *p, uint16_t type){
		click_ether *eth = (click_ether *) p->data();
		veil_sub_header *veil_sub = (veil_sub_header *) (eth+1);
		veil_sub->veil_type = htons(type);
		return p;
	}

	inline Packet * setVEILTTL(Packet *p, uint8_t ttl){
		click_ether *eth = (click_ether *) p->data();
		veil_sub_header *veil_sub = (veil_sub_header *) (eth+1);
		veil_sub->ttl=ttl;
		return p;
	}
	inline Packet * decrementVEILTTL(Packet *p){
		// decrements the ttl on the packet by one;
		click_ether *eth = (click_ether *) p->data();
		veil_sub_header *veil_sub = (veil_sub_header *) (eth+1);
		(veil_sub->ttl)--;
		return p;
	}


// VEIL sub HEADER END ---------------------------------------

//------------------------------------ END -----------------------------------------

/*
	inline
	void veil_chatter(bool printit, const char *fmt, ...){
		if (!printit) return;
		char buf[2000];
		bzero(buf,2000);
		time_t myt; time(&myt);
		va_list val;
		va_start(val, fmt);
		vsprintf(buf,fmt,val);
		//printf("\nAt %s:: %s", ctime(&myt), buf);
		printf("%d | %s", (int)myt,buf);
		//printf("%s", myt,buf);
		va_end(val);
	}
*/

	inline
	void veil_chatter_new(bool printit, const char * nametag , const char *fmt, ...){
		if (!printit) return;
		timeval tim;
		gettimeofday(&tim, NULL);
		char buf[2400];
		bzero(buf,2400);
		double t1 = tim.tv_sec + (tim.tv_usec/1000000.0);
		va_list val;
		va_start(val, fmt);
		vsprintf(buf,fmt,val);
		//printf("\nAt %s:: %s", ctime(&myt), buf);
		printf("%3lf | %s | %s\n", t1, nametag, buf);
		va_end(val);
	}

	inline
	const char * get_packet_type_str(uint32_t type){
		switch(type){
			case ETHERTYPE_VEIL: return "EtherType_VEIL";
			case ETHERTYPE_VEIL_IP: return "ETHERTYPE_VEIL_IP";
			case VEIL_ARP_REQ: return "VEIL_ARP_REQ";	
			case VEIL_ARP_RPLY: return "VEIL_ARP_RPLY";
			case VEIL_PUBLISH: return "VEIL_PUBLISH";
			case VEIL_IP: return "VEIL_IP";
			case VEIL_HELLO: return "VEIL_HELLO";
			case VEIL_PHELLO: return "VEIL_PHELLO";
			case VEIL_VCC_SPANNING_TREE_AD: return "VEIL_VCC_SPANNING_TREE_AD";
			case VEIL_VCC_SPANNING_TREE_SUBS: return "VEIL_VCC_SPANNING_TREE_SUBS";
			case VEIL_LOCAL_TOPO_TO_VCC: return "VEIL_LOCAL_TOPO_TO_VCC";
			case VEIL_SWITCH_VID_FROM_VCC: return "VEIL_SWITCH_VID_FROM_VCC";							
			case VEIL_RDV_REPLY: return "VEIL_RDV_REPLY";							
			case VEIL_RDV_PUBLISH: return "VEIL_RDV_PUBLISH";							
			case VEIL_RDV_QUERY: return "VEIL_RDV_QUERY";							
			case VEIL_MAP_PUBLISH: return "VEIL_MAP_PUBLISH";							
			case VEIL_MAP_UPDATE: return "VEIL_MAP_UPDATE";							
			case VEIL_ENCAP_IP: return "VEIL_ENCAP_IP";							
			case VEIL_ENCAP_ARP: return "VEIL_ENCAP_ARP";							
			case VEIL_ENCAP_REDIRECTION: return "VEIL_ENCAP_REDIRECTION";							
			case VEIL_ENCAP_MULTIPATH_IP: return "VEIL_ENCAP_MULTIPATH_IP";								
																										
			default: return "Undefined_Packet_Type";
		}
		return "Error";
	}

CLICK_ENDDECLS
#endif
