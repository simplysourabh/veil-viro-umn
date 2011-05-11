//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
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


#include <click/packet_anno.hh>
#include <click/packet.hh>
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
#define VEIL_PUBLISH_INTERVAL	25000
#define VEIL_RDV_INTERVAL		30000
#define HOST_ENTRY_EXPIRY		60000


/* value greater than hello/publish interval so entries don't expire just    
 * when hello/publish pkts need to be resent. risking temporarily invalid    
 * routes. 
 */ 
#define VEIL_TBL_ENTRY_EXPIRY	60000
#define MAX_TTL 				0x0f

#define VEIL_SPANNING_TREE_ENTRY_EXPIRY 60000 // an entry for the veil spanning tree will expire after
// 20 seconds.
#define VEIL_SPANNING_TREE_COST_BROADCAST 25000 // update the cost every 10 seconds.
#define VEIL_VID_BROADCAST_TIME 25000

#define MAX_PACKET_SIZE	1200 // maximum packet size in bytes.
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
#define VEIL_HELLO_REPLY			0x0002
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
// we use the following type to encapsulate the packets to access switch vid, when 
// there is no path for the current dest vid on the IP packet.
#define NO_VID_TO_ACCESS_SWITCH		0x0203

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

// Type: NO_VID_TO_ACCESS_SWITCH
// |Currend DST VID 6 bytes|
struct veil_payload_no_vid_to_access_switch{
	VID current_dvid;
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

/*
FROM UTILITIES.HH 
*/
//---------------------------------------------------------------------
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
//------------------------------------------------------------------------
	
	// creates the publish access info packet. returns NULL in the event of failure.
	inline
	WritablePacket* publish_access_info_packet(IPAddress ip, EtherAddress smac, VID svid, bool printDebugMessages, const char* callers_name){
		int packet_length = sizeof(click_ether) + sizeof(veil_sub_header) + sizeof(veil_payload_map_publish);
                WritablePacket *p = Packet::make(packet_length);
                if (p == 0) {
                        veil_chatter_new(true, callers_name, "[Error!] cannot make packet in publishaccessinfo");
                        return NULL;
                }
                memset(p->data(), 0, p->length());
                click_ether *e = (click_ether *) p->data();
                p->set_ether_header(e);
	        VID accessvid = calculate_access_switch(&ip);
	        memcpy(e->ether_dhost, &accessvid, 6); 
	        bzero(e->ether_shost,6);
	        memcpy(e->ether_shost, &svid, 4);
	        e->ether_type = htons(ETHERTYPE_VEIL);
                veil_sub_header *vheader = (veil_sub_header*) (e + 1);
                vheader->veil_type = htons(VEIL_MAP_PUBLISH);
	        vheader->ttl = MAX_TTL;
	        memcpy(&vheader->dvid, &accessvid, 6);
	        bzero(&vheader->svid, 6);
	        memcpy(&vheader->svid, &svid, 4);
                veil_payload_map_publish * payload_publish = (veil_payload_map_publish * )(vheader+1);
                memcpy(&payload_publish->ip, &ip, 4);
                memcpy(&payload_publish->mac, &smac, 6);
                memcpy(&payload_publish->vid, &svid, 6);

                SET_REROUTE_ANNO(p, 'r');

                veil_chatter_new(printDebugMessages, callers_name,"[Access Info Publish] HOST IP: %s  VID: %s  MAC: %s AccessSwitchVID: %s", ip.s().c_str(),  svid.vid_string().c_str(),smac.s().c_str(),accessvid.switchVIDString().c_str() );
		return p;
	}

	// creates the .
	inline
	WritablePacket* update_access_info_packet(IPAddress ip, EtherAddress smac, VID svid, VID dstvid, VID myvid, bool printDebugMessages, const char* callers_name){
		int packet_length = sizeof(click_ether) + sizeof(veil_sub_header) + sizeof(veil_payload_map_publish);
                WritablePacket *p = Packet::make(packet_length);
                if (p == 0) {
                        veil_chatter_new(true, callers_name, "[Error!] cannot make packet in publishaccessinfo");
                        return NULL;
                }
                memset(p->data(), 0, p->length());
                click_ether *e = (click_ether *) p->data();
                p->set_ether_header(e);
	        memcpy(e->ether_dhost, &dstvid, 6); 
	        bzero(e->ether_shost,6);
	        memcpy(e->ether_shost, &myvid, 4);
	        e->ether_type = htons(ETHERTYPE_VEIL);
                veil_sub_header *vheader = (veil_sub_header*) (e + 1);
                vheader->veil_type = htons(VEIL_MAP_UPDATE);
	        vheader->ttl = MAX_TTL;
	        memcpy(&vheader->dvid, &dstvid, 6);
	        bzero(&vheader->svid, 6);
	        memcpy(&vheader->svid, &myvid, 4);
                veil_payload_map_update * payload_update = (veil_payload_map_update * )(vheader+1);
                memcpy(&payload_update->ip, &ip, 4);
                memcpy(&payload_update->mac, &smac, 6);
                memcpy(&payload_update->vid, &svid, 6);
		
		// we don't need to reroute the map_update packets.
                //SET_REROUTE_ANNO(p, 'r');

                veil_chatter_new(printDebugMessages, callers_name,"[Access Info Update] HOST IP: %s  VID: %s  MAC: %s AccessSwitchVID: %s Old host-switch: %s", ip.s().c_str(),   svid.vid_string().c_str(), smac.s().c_str(), myvid.switchVIDString().c_str(), dstvid.vid_string().c_str() );
		return p;
	}


	inline
	WritablePacket* create_arp_reply_packet(EtherAddress smac, EtherAddress dmac, IPAddress sip, IPAddress dip){
		WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(click_ether_arp));
		if (q == 0) {
			veil_chatter_new(true, "click_veil","[Error!] in arp responder: cannot make packet!");
			return 0;
		}

		click_ether *e = (click_ether *) q->data();
		q->set_ether_header(e);
		memcpy(e->ether_dhost, dmac.data(), 6);
		memcpy(e->ether_shost, smac.data(), 6);
		e->ether_type = htons(ETHERTYPE_ARP);
		click_ether_arp *ea = (click_ether_arp *) (e + 1);
		ea->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
		ea->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
		ea->ea_hdr.ar_hln = 6;
		ea->ea_hdr.ar_pln = 4;
		ea->ea_hdr.ar_op = htons(ARPOP_REPLY);
		memcpy(ea->arp_sha, smac.data(), 6);
		memcpy(ea->arp_spa, sip.data(), 4);
		memcpy(ea->arp_tha, dmac.data(), 6);
		memcpy(ea->arp_tpa, dip.data(), 4);

		return q;
	}

	inline
	WritablePacket * create_veil_encap_arp_reply_packet(IPAddress dstip, IPAddress srcip, VID srcvid, VID dstvid, VID myvid){
	        VID accvid = calculate_access_switch(&dstip);

		WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(veil_sub_header) + sizeof(click_ether_arp));
	        if (q == 0) {
	                veil_chatter_new(true, "CLICK_VEIL "," ERROR create_veil_encap_arp_reply_packet: cannot make packet!");
	                return NULL;
	        }

	        click_ether *e = (click_ether *) q->data();
	        q->set_ether_header(e);
	
	        memcpy(e->ether_dhost, &myvid, 6);
	        memcpy(e->ether_shost, &myvid, 6);
	        e->ether_type = htons(ETHERTYPE_VEIL);
	
	        veil_sub_header *vheader = (veil_sub_header*) (e+1);
	        memcpy(&vheader->dvid, &dstvid, 6);
	        memcpy(&vheader->svid, &srcvid, 6);
	        vheader->veil_type = htons(VEIL_ENCAP_ARP);
	        vheader->ttl = MAX_TTL;
	
	        click_ether_arp *arp_payload = (click_ether_arp *) (vheader + 1);
	        arp_payload->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
	        arp_payload->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
	        arp_payload->ea_hdr.ar_hln = 6;
	        arp_payload->ea_hdr.ar_pln = 4;
	        arp_payload->ea_hdr.ar_op = htons(ARPOP_REPLY);
	        memcpy(arp_payload->arp_sha, &srcvid, 6);
	        memcpy(arp_payload->arp_spa, &srcip, 4);
	        memcpy(arp_payload->arp_tha, &dstvid, 6);
	        memcpy(arp_payload->arp_tpa, &dstip, 4);

		return q;
	}

	inline
	WritablePacket * create_veil_encap_arp_query_packet(IPAddress dstip, IPAddress srcip, VID srcvid, VID myvid){
	        VID accvid = calculate_access_switch(&dstip);

		WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(veil_sub_header) + sizeof(click_ether_arp));
	        if (q == 0) {
	                veil_chatter_new(true, "CLICK_VEIL ","[Error!] in processarp: cannot make packet!");
	                return NULL;
	        }

	        click_ether *e = (click_ether *) q->data();
	        q->set_ether_header(e);
	
	        memcpy(e->ether_dhost, &myvid, 6);
	        memcpy(e->ether_shost, &myvid, 6);
	        e->ether_type = htons(ETHERTYPE_VEIL);
	
	        veil_sub_header *vheader = (veil_sub_header*) (e+1);
	        memcpy(&vheader->dvid, &accvid, 6);
	        memcpy(&vheader->svid, &srcvid, 6);
	        vheader->veil_type = htons(VEIL_ENCAP_ARP);
	        vheader->ttl = MAX_TTL;
	
	        click_ether_arp *arp_payload = (click_ether_arp *) (vheader + 1);
	        arp_payload->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
	        arp_payload->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
	        arp_payload->ea_hdr.ar_hln = 6;
	        arp_payload->ea_hdr.ar_pln = 4;
	        arp_payload->ea_hdr.ar_op = htons(ARPOP_REQUEST);
	        memcpy(arp_payload->arp_sha, &srcvid, 6);
	        memcpy(arp_payload->arp_spa, &srcip, 4);
	        memset(arp_payload->arp_tha, 0, 6);
	        memcpy(arp_payload->arp_tpa, &dstip, 4);

		return q;
	
	}

	inline
	Packet* create_an_encapsulated_packet_to_access_switch(Packet *p){
	        // We will encapsualte and send this packet to the access switch. 
	
	        // first extract the header info.
	        IPAddress srcip, dstip;
	        VID srcvid, dstvid;
	        uint16_t veil_type;
	        EtherAddress smac, dmac;
	        uint8_t ttl;
	
	        click_ether *eth = (click_ether*) p->data();
	        memcpy(&smac, eth->ether_shost,6);
	        memcpy(&dmac, eth->ether_dhost,6);
	
	        // Assumes that ether_type on the packet is ETHER_VEIL
	        assert(ntohs(eth->ether_type) == ETHERTYPE_VEIL);
	
	        veil_sub_header *vheader = (veil_sub_header*) (eth+1);
	        memcpy(&srcvid, &vheader->svid, 6);
	        memcpy(&dstvid, &vheader->dvid, 6);
	        veil_type = ntohs(vheader->veil_type);
	        ttl = vheader->ttl;
	
	        // veil_type must be either VEIL_ENCAP_IP or VEIL_MULTIPATH_IP
	        assert(veil_type == VEIL_ENCAP_IP || veil_type == VEIL_ENCAP_MULTIPATH_IP);
	        click_ip * ip_header; veil_payload_multipath* veil_payload;
	        if(veil_type == VEIL_ENCAP_MULTIPATH_IP){
	                veil_payload = (veil_payload_multipath*) (vheader+1);
	                memcpy(&dstvid, &veil_payload->final_dvid, 6);
	
	                ip_header = (click_ip*) (veil_payload+1);
	        }else{
	                ip_header = (click_ip*) (vheader+1);
	        }
	
	        srcip = IPAddress(ip_header->ip_src);
		dstip = IPAddress(ip_header->ip_dst);
	
	        //calculate access switch VID
	        VID accvid = calculate_access_switch(&dstip);
	        if(veil_type == VEIL_ENCAP_MULTIPATH_IP){
			
	                // we do not need to push any new data on the packet in this case
	                // first overwrite the dstvid on the packet by the accvid.
	                memcpy(&vheader->dvid, &accvid, 6);
	
	                // change the packet type to NO_VID_TO_ACCESS_SWITCH
	                vheader->veil_type = htons(NO_VID_TO_ACCESS_SWITCH);
	
	                // packet is ready now.
			veil_chatter_new(true, "CLICK_VEIL in create_an_encapulated_packet_to_access_switch: VEIL_ENCAP_MULTIPATH_IP", "sip %s srcvid %s => dip %s dstvid %s via accvid %s", srcip.s().c_str(), srcvid.vid_string().c_str(), dstip.s().c_str(), dstvid.vid_string().c_str(), accvid.vid_string().c_str());
	                return p;
	        }
	
	        // we will have to insert a 6 byte long payload in this case.
	        Packet * q = p->push(sizeof(veil_payload_no_vid_to_access_switch));
	        click_ether *q_eth = (click_ether*)(q->data());
	        veil_sub_header * q_vheader = (veil_sub_header*)(q_eth+1);
	        veil_payload_no_vid_to_access_switch* q_veil_payload = (veil_payload_no_vid_to_access_switch*)(q_vheader+1);
	
	        //shift the ethernet header
	        memcpy(q_eth, eth, sizeof(click_ether));
	
	        // shift the veil header.
	        memcpy(q_vheader, vheader, sizeof(veil_sub_header));
	
	        // now update the values.
	        q_vheader->veil_type = htons(NO_VID_TO_ACCESS_SWITCH);
	        memcpy(&q_vheader->dvid, &accvid, 6);
	        memcpy(&q_veil_payload->current_dvid, &dstvid, 6);

		veil_chatter_new(true, "CLICK_VEIL in create_an_encapulated_packet_to_access_switch: VEIL_ENCAP_IP", "sip %s srcvid %s => dip %s dstvid %s via accvid %s", srcip.s().c_str(), srcvid.vid_string().c_str(), dstip.s().c_str(), dstvid.vid_string().c_str());
	        return q;
	}

	inline
	Packet * update_vid_on_no_vid_to_access_switch_packet(Packet* p, VID mydstvid){
		// constructs a VEIL_ENCAP_IP type of packet using the initial embedded packet
		// and the updated destination VID.
		click_ether* eth = (click_ether*) p->data();
		veil_sub_header* vheader = (veil_sub_header*) (eth+1);
		veil_payload_no_vid_to_access_switch* veil_payload = (veil_payload_no_vid_to_access_switch*) (vheader+1);

		//now modify the destination vid and the packet type
		memcpy(&vheader->dvid, &mydstvid, 6);
		vheader->veil_type = htons(VEIL_ENCAP_IP);

		// now shift the header by 6 bytes. starting from the veil_sub_header, and then eth.
		// memcpy((char*)(vheader) + sizeof(veil_payload_no_vid_to_access_switch), vheader, sizeof(veil_sub_header));
		// shift the eth header by size(...) bytes.
		// memcpy((char*)(eth) + sizeof(veil_payload_no_vid_to_access_switch), eth, sizeof(click_ether));
		int totalBytesToShift = sizeof(veil_sub_header) + sizeof(click_ether);
		int offset = sizeof(veil_payload_no_vid_to_access_switch);
		char * dataptr = (char*) p->data();
		for (int i = totalBytesToShift-1; i >= 0; i--){
			dataptr[i+offset] = dataptr[i];
		}

		// now pull out the initial sizeof(..) bytes.
		p->pull(sizeof(veil_payload_no_vid_to_access_switch));

		// doing some checking to make sure that everything is correct.
		eth = (click_ether*) p->data();
		vheader = (veil_sub_header*) (eth+1);
		uint16_t veil_type = ntohs(vheader->veil_type);
		if(veil_type != VEIL_ENCAP_IP){
			// something went wrong.
			veil_chatter_new(true, "CLICK_VEIL","ERROR: Packet should have veil_type = %d(VEIL_ENCAP_IP), but found %d", VEIL_ENCAP_IP, veil_type);
		}

		VID tvid;
		memcpy(&tvid, &vheader->dvid,6);
		if(tvid != mydstvid){
			//error
			veil_chatter_new(true,"CLICK_VEIL", "ERROR: Packet should have dstvid = %s, but found %s", mydstvid.vid_string().c_str(), tvid.vid_string().c_str());
		}
		return p;
	}

CLICK_ENDDECLS
#endif
