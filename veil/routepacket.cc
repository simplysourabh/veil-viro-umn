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
	uint16_t numinterfaces = interfaces->numInterfaces();
	
	int myport = PORT_ANNO(p);
	VID myVid;
	interfaces->lookupIntEntry(myport, &myVid);		
	const click_ether *eth = (const click_ether *) p->data();
	bool dstvidChanged = false;
	uint16_t k;
	//VEIL packets: access PUBLISH/QUERY or RDV REQ/PUB/RPLY	
	if(ntohs(eth->ether_type) == ETHERTYPE_VEIL){
		VID srcvid = VID(eth->ether_shost);
		VID dstvid = VID(eth->ether_dhost);
		click_chatter("[-x- RoutePacket] Destination: |%s| \n", dstvid.vid_string().c_str());
	
		veil_header *vhdr = (veil_header*) (eth+1);
		uint16_t pktType = ntohs(vhdr->packetType);
		
		int port = getPort(dstvid, p,k);

		while(('r' == REROUTE_ANNO(p) || pktType  == VEIL_RDV_QUERY || pktType  == VEIL_RDV_PUBLISH) && port < 0){			
			dstvid.flip_bit(k);
			port = getPort(dstvid,p,k);
			dstvidChanged = true;
			click_chatter("[-x- RoutePacket] Destination after %dth bit flip: |%s| \n",k, dstvid.vid_string().c_str());
		}
		
		//Update the packet, and overwrite the dstvid field.
		if (dstvidChanged){
			// Now overwrite the ethernet mac destination address.
			//click_ether *e = (click_ether*) p->data();
			//memcpy(e->ether_dhost, &dstvid, VID_LEN);
		}
		return (port >= 0 ? port : numinterfaces+1);
		// returns the port number if found, else to the ERROR outputport given by numinterfaces + 1
		
	}	

	//pkt is ALWAYS *destined* to one of our hosts if this is true
	if(ntohs(eth->ether_type) == ETHERTYPE_ARP ||
		ntohs(eth->ether_type) == ETHERTYPE_IP){
		//look at dest eth and find corresponding VID
		//from host table. Find interface VID from host VID
		//Find port # from interface VID.
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

int
VEILRoutePacket::getPort(VID dstvid, Packet *p, uint16_t & k){
	// returns -1 if no appropriate output port was found
	// returns numinterfaces if the packet is desined to one 
	// of the local interface
	// otherwise returns the physical output port on the veil_switch
	
	int port = -1; 
	VID myVid;
	uint16_t numinterfaces = interfaces->numInterfaces(); 
	// SJ: Use the interface VID which is closest to the destionation!
	getClosestInterfaceVID(dstvid, myVid); // return value is in myVid

	//figure out what k is	
	k = myVid.logical_distance(&dstvid);

	//if k is <= 16 then pkt is destined to us
	//i.e., myVid = dstvid
	if(k <= 16){
		//we need to set the dest field of the pkt to myVid
		click_chatter("[-x- RoutePacket] Destination is ME: |%s| \n", dstvid.vid_string().c_str());
		click_ether *e = (click_ether*) p->data();
		memcpy(e->ether_dhost, &myVid, VID_LEN);
		return numinterfaces;
	}			
	
	// see if it belongs to one of our physical neighbors,
	// lookup neighbor table
	// SJ: IMPORTANT: See if the first 32 bits are same or not!!! for the neighbor lookup
	VID swvid = dstvid.getSwitchVID();
	VID myinterface;
	if(neighbors->lookupEntry(&swvid, &myinterface)){
		interfaces->lookupVidEntry(&myinterface, &port);
		return port;
	}
	
	// otherwise lookup route table for kth bucket
	VID nexthop, gateway;
	while(routes->getRoute(&dstvid, k, myVid, &nexthop, &gateway))
	{
		int temp;
		// see if the nexthop is one of the physical neighbor?
		if (neighbors->lookupEntry(&nexthop, &myinterface)){
			interfaces->lookupVidEntry(&myinterface, &port);
			return port;
		} // otherwise it should be one of the local interfaces at the veil switch,
		// check it:
		else if (interfaces->lookupVidEntry(&nexthop, &temp)){
			memcpy(&myVid, &nexthop,6);
			k = myVid.logical_distance(&dstvid);
			// now look up in the routing table for this local interface.
			click_chatter("[-x- RoutePacket] For Dest VID: |%s|  Nexthop: |%s| is my local interface.\n",dstvid.vid_string().c_str(),nexthop.vid_string().c_str());
		}
		else{
			click_chatter("[-x- RoutePacket][ERROR][NEXTHOP IS NEITHER A PHYSICAL NEIGHBOR, NOR MY LOCAL INTERFACE] I SHOULD NEVER REACH HERE:  For Dest VID: |%s|  MyVID: |%s| NextHop: |%s| \n",dstvid.vid_string().c_str(),myVid.vid_string().c_str(), nexthop.vid_string().c_str());
			port = -1; 
			return port;
		}
	}
	
	return port;
}


void
VEILRoutePacket::push (
	int,
	Packet* pkt)
{
	int port = smaction(pkt);
	if (pkt != NULL)
	{
		click_chatter("[-x- RoutePacket] Forwarding packet on port %d.\n",port);
		output(port).push(pkt);
	}
}

void
VEILRoutePacket::getClosestInterfaceVID(VID dstvid, VID &myVID){
	//Return value is set in the dstvid
	uint16_t numinterfaces = interfaces->numInterfaces();
	VID intvid;
	uint32_t xordist = 0xFFFFFFFF;
	for (uint16_t i = 0; i < numinterfaces; i++){
		if (interfaces->lookupIntEntry(i, &intvid)){
			uint32_t newdist = VID::XOR(intvid, dstvid);
			if (newdist < xordist){
				memcpy(&myVID, &intvid,6);
				xordist = newdist;		
			}
		}
	}
	click_chatter("[-x- RoutePacket][Closest MyInerface VID] Dest VID: |%s|  MyVID: |%s| XoRDist: %d\n",dstvid.vid_string().c_str(),myVID.vid_string().c_str(), xordist);
}
CLICK_ENDDECLS

EXPORT_ELEMENT(VEILRoutePacket)
