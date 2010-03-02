#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include <click/vid.hh>
#include "utilities.hh"
#include "click_veil.hh"
#include "routepacket.hh"

CLICK_DECLS

VEILRoutePacket::VEILRoutePacket () {}

VEILRoutePacket::~VEILRoutePacket () {}

int
VEILRoutePacket::configure (
	Vector<String> &conf,
	ErrorHandler *errh)
{
	return cp_va_kparse(conf, this, errh,
		"HOSTTABLE", cpkM+cpkP, cpElementCast, "VEILHostTable", &hosts,
		"ROUTETABLE", cpkM+cpkP, cpElementCast, "VEILRouteTable", &routes,
		"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
		"NEIGHBORTABLE", cpkM+cpkP, cpElementCast, "VEILNeighborTable", &neighbors,
		cpEnd);
}

int  
VEILRoutePacket::smaction(Packet* p){
	//numinterfaces is backroute and numinterfaces+1 is for error pkts
	int numinterfaces = interfaces->numInterfaces();
	
	int myport = PORT_ANNO(p);
	VID myVid;
	interfaces->lookupIntEntry(myport, &myVid);		
	
	const click_ether *eth = (const click_ether *) p->mac_header();

	//VEIL packets: access PUBLISH/QUERY or RDV REQ/PUB/RPLY	
	if(ntohs(eth->ether_type) == ETHERTYPE_VEIL){
		VID srcvid = VID(eth->ether_shost);
		VID dstvid = VID(eth->ether_dhost);
		
		veil_header *vhdr = (veil_header*) (eth+1);
		uint16_t pktType = ntohs(vhdr->packetType);

		int port;
		VID myinterface;

		if('r' == REROUTE_ANNO(p) || pktType  == VEIL_RDV_QUERY || 
		   pktType  == VEIL_RDV_PUBLISH){			
		reroute:
			//figure out what k is	
			int k = myVid.logical_distance(&dstvid);
			
			//if k is 0 then pkt is destined to us
			//i.e., myVid = dstvid
			if(0 == k){				
				return numinterfaces;
			}			
			
			//packet is meant for one of our interfaces
			if(interfaces->lookupVidEntry(&dstvid, &port))
				return port;			
			
			//lookup neighbor table
			if(neighbors->lookupEntry(&dstvid, &myinterface)){
				interfaces->lookupVidEntry(&myinterface, &port);
				return port;
			}
			
			//lookup route table for kth bucket
			VID nexthop, gateway;
			if(routes->lookupEntry(k, &myinterface, &nexthop, &gateway))
			{
				interfaces->lookupVidEntry(&myinterface, &port);
				return port;
			}
			
			dstvid.flip_bit(k);
			goto reroute;
		}

		int k = myVid.logical_distance(&dstvid);		
		
		//packet is meant for one of our interfaces
		if(interfaces->lookupVidEntry(&dstvid, &port))
			return port;			
			
		//lookup neighbor table
		if(neighbors->lookupEntry(&dstvid, &myinterface)){
			interfaces->lookupVidEntry(&myinterface, &port);
			return port;
		}
			
		//lookup route table for kth bucket
		VID nexthop, gateway;
		if(routes->lookupEntry(k, &myinterface, &nexthop, &gateway))
		{
			interfaces->lookupVidEntry(&myinterface, &port);
			return port;
		}

		//we now don't know what to do with the packet
		return numinterfaces+1;				
	} 
	
	//pkt is ALWAYS *destined* to one of our hosts if this is true
	if(ntohs(eth->ether_type) == ETHERTYPE_ARP ||
		ntohs(eth->ether_type) == ETHERTYPE_IP){
		//look at dest eth and find corresponding VID
		//from host table. Find interface VID from host VID
		//Find port # from interface VID.
		const click_ether *eth = (const click_ether *) p->mac_header();
		EtherAddress dst = EtherAddress(eth->ether_dhost);
		VID dstvid;
		hosts->lookupMAC(&dst, &dstvid);
		unsigned char interfacevid[VID_LEN];
		memcpy(interfacevid, &dstvid, VID_LEN - HOST_LEN);
		
		int port;
		interfaces->lookupVidEntry(&VID(interfacevid), &port);
		//TODO: did it return the right port here?
		return port;
	}	

	//TODO: any error conditions here?
	return numinterfaces+1;
}

void
VEILRoutePacket::push (
	int,
	Packet* pkt)
{
	int port = smaction(pkt);
	output(port).push(pkt);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILRoutePacket)
