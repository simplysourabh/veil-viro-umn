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
	printDebugMessages = true;
	return cp_va_kparse(conf, this, errh,
		"HOSTTABLE", cpkM+cpkP, cpElementCast, "VEILHostTable", &hosts,
		"ROUTETABLE", cpkM+cpkP, cpElementCast, "VEILRouteTable", &routes,
		"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
		"NEIGHBORTABLE", cpkM+cpkP, cpElementCast, "VEILNeighborTable", &neighbors,
		"PRINTDEBUG", 0, cpBool, &printDebugMessages,
		cpEnd);
}

int  
VEILRoutePacket::smaction(Packet* p){
	//numinterfaces is backroute and numinterfaces+1 is for error pkts
	uint8_t numinterfaces = interfaces->numInterfaces();
	
	if (p == NULL){
		veil_chatter_new(printDebugMessages, class_name()," NULL packet ");
		return numinterfaces+1;
	}
	int myport = PORT_ANNO(p);
	VID myVid;
	interfaces->lookupIntEntry(myport, &myVid);
	veil_chatter_new(printDebugMessages, class_name()," Input interface %s at port %d",myVid.switchVIDString().c_str(), myport );
	click_ether *eth = (click_ether *) p->data();
	bool dstvidChanged = false;
	uint8_t k;
	//printf("here 0 \n");
	//VEIL packets: access PUBLISH/QUERY or RDV REQ/PUB/RPLY	
	if(ntohs(eth->ether_type) == ETHERTYPE_VEIL){
		//VID srcvid = VID(eth->ether_shost);
		//VID dstvid = VID(eth->ether_dhost);
		//printf("E here 0 \n");
		veil_sub_header *vheader = (veil_sub_header *) (eth+1);
		VID srcvid, dstvid;
		memcpy(&srcvid, &vheader->svid,6);
		memcpy(&dstvid, &vheader->dvid,6);

		veil_chatter_new(printDebugMessages, class_name()," Destination: |%s| ", dstvid.switchVIDString().c_str());

		uint16_t veil_type = ntohs(vheader->veil_type);
		VID nextvid;
		int port;
		//printf("E here 1 \n");
		if (veil_type == VEIL_ENCAP_MULTIPATH_IP){
			port = getPort(dstvid, p,k, nextvid);

			VID finalvid;
			veil_payload_multipath *veil_payload = (veil_payload_multipath *)(vheader+1);
			memcpy(&finalvid, &veil_payload->final_dvid, 6);

			// Can I change the destination VID?
			// 1. vheader->dvid == veil_payload->final_dvid and final destination is not one of my interfaces, I can change the intermediate destination.
			if (finalvid == dstvid && port < numinterfaces && port >= 0 ){
				VID newgw;
				// Now choose an alternate gateway
				//printf("E here 2 \n");
				if(routes->getRoute(&dstvid,myVid, &nextvid, &newgw,false)){
					veil_chatter_new(printDebugMessages, class_name(),"[VEIL_ENCAP_MULTIPATH_IP] New GW: %s to Destination %s at Bucket %d of ME %s ",newgw.switchVIDString().c_str(), finalvid.switchVIDString().c_str(), k ,myVid.switchVIDString().c_str() );
					memcpy(&vheader->dvid, &newgw, 6);
					//printf("E here 3 \n");
					memcpy(&dstvid, &newgw, 6);
				}
			}
			//printf("E here 4 \n");
			// if dstvid == myvid then make the dstvid = finalvid
			if (port == numinterfaces){
				memcpy(&vheader->dvid, &veil_payload->final_dvid, 6);
				memcpy(&dstvid, &veil_payload->final_dvid, 6);
			}
			//printf("E here 5 \n");
		}

		//printf("E here 6.0 \n");
		port = getPort(dstvid, p,k, nextvid);
		//printf("E here 6 \n");
		while(('r' == REROUTE_ANNO(p) || veil_type  == VEIL_RDV_QUERY || veil_type  == VEIL_RDV_PUBLISH || veil_type == VEIL_MAP_PUBLISH) && port < 0){
			dstvid.flip_bit(k);
			//veil_chatter_new(printDebugMessages, class_name()," Destination after %dth bit flip: |%s| ",k, dstvid.switchVIDString().c_str());
			veil_chatter_new(printDebugMessages, class_name()," Destination after %dth bit flip: |%s| ",k, dstvid.switchVIDString().c_str());
			port = getPort(dstvid,p,k,nextvid);
			//printf("E here 7 \n");
			dstvidChanged = true;
		}
		
		//Update the packet, and overwrite the dstvid field.
		if (dstvidChanged){
			// Write the udpated destination address
			memcpy(&vheader->dvid, &dstvid, VID_LEN);
		}
		// overwrite the ethernet destination to physical nexthop vid
		// overwrite the ethernet source to my own physical nexthop vid
		memcpy(eth->ether_dhost, &nextvid,6);
		memcpy(eth->ether_shost, &myVid, 6);

		// update the ttl on the packet.
		uint8_t ttl = vheader->ttl;
		vheader->ttl = ttl-1;

		if (ttl == 0){
			// if ttl has expired
			veil_chatter_new(true, class_name(),"[TTL EXPIRED] Pkt type: Ethertype_veil Destination: |%s| at port: %d ", dstvid.switchVIDString().c_str(), port);
			return numinterfaces+1;
		}
		//printf("E here 8 \n");

		veil_chatter_new(printDebugMessages, class_name()," Pkt type: Ethertype_veil Destination: |%s| at port: %d ", dstvid.switchVIDString().c_str(), port);
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
		VID interfacevid;
		memcpy(&interfacevid, &dstvid, VID_LEN - HOST_LEN);
		
		int port;
		if(interfaces->lookupVidEntry(&(interfacevid), &port)){
		//TODO: did it return the right port here?
			veil_chatter_new(printDebugMessages, class_name()," Pkt type: Ethertype_ARP Destination MAC: %s on interface: %s at port: %d ", dst.s().c_str(),interfacevid.vid_string().c_str(), port);
			return port;
		}else{
			port = numinterfaces+1; // ERROR PORT
			veil_chatter_new(printDebugMessages, class_name()," NO ENTRY!! Pkt type: Ethertype_ARP Destination MAC: %s on interface: %s at port: %d ", dst.s().c_str(),interfacevid.vid_string().c_str(), port);
			return port;
		}
	}	
	
	if(ntohs(eth->ether_type) == ETHERTYPE_VEIL_IP){
		// TODO TTL Decrement?
		
		EtherAddress dst = EtherAddress(eth->ether_dhost);
		VID dvid, nexthop;
		memcpy(&dvid, &dst, VID_LEN);
		int port = getPort(dvid, p,k, nexthop);
		veil_chatter_new(printDebugMessages, class_name()," Pkt type: Ethertype_VEIL_IP Destination VID: %s  at port: %d ", dst.s().c_str(), port);
		return (port >= 0 ? port : numinterfaces+1);
	}
	//TODO: any error conditions here?
	return numinterfaces+1;
}

int
VEILRoutePacket::getPort(VID dstvid, Packet *p, uint8_t & k, VID &nexthop){
	// returns -1 if no appropriate output port was found
	// returns numinterfaces if the packet is desined to one 
	// of the local interface
	// otherwise returns the physical output port on the veil_switch
	
	int port = -1; 
	VID myVid;
	//printf("getPort 1 \n");
	uint8_t numinterfaces = interfaces->numInterfaces();
	// SJ: Use the interface VID which is closest to the destionation!
	getClosestInterfaceVID(dstvid, myVid); // return value is in myVid
	//printf("getPort 2 \n");
	//figure out what k is	
	k = myVid.logical_distance(&dstvid);
	//printf("1 Myvid is %s\n", myVid.vid_string().c_str());
	//if k is <= 16 then pkt is destined to us
	//i.e., myVid = dstvid
	if(k <= 16){
		//printf("getPort 3 \n");
		//we need to set the dest field of the pkt to myVid
		veil_chatter_new(printDebugMessages, class_name()," Destination is ME: |%s| ", dstvid.switchVIDString().c_str());
		click_ether *e = (click_ether*) p->data();
		memcpy(e->ether_dhost, &myVid, VID_LEN);
		memcpy(&nexthop, &myVid, 6);
		return numinterfaces;
	}
	//printf("2 Myvid is %s\n", myVid.vid_string().c_str());
	//printf("getPort 4 \n");
	// see if it belongs to one of our physical neighbors,
	// lookup neighbor table
	// SJ: IMPORTANT: See if the first 32 bits are same or not!!! for the neighbor lookup
	VID swvid = dstvid.getSwitchVID();
	VID myInterfacevid;
	//printf("getPort 5 \n");
	if(neighbors->lookupEntry(&swvid, &myInterfacevid)){
		//printf("getPort 6 \n");
		memcpy(&nexthop, &swvid,6);
		interfaces->lookupVidEntry(&myInterfacevid, &port);
		//printf("getPort 7 \n");
		return port;
	}
	//printf("3 Myvid is %s\n", myVid.vid_string().c_str());
	//printf("getPort 8 \n");
	// otherwise lookup route table for kth bucket
	VID gateway;
	//return -1;
	//printf("4 Myvid is %s\n", myVid.vid_string().c_str());
	while(routes->getRoute(&dstvid,myVid, &nexthop, &gateway))
	{
		int temp;
		//printf("getPort 9 \n");
		// see if the nexthop is one of the physical neighbor?
		if (neighbors->lookupEntry(&nexthop, &myInterfacevid)){
			//printf("getPort 10 \n");
			interfaces->lookupVidEntry(&myInterfacevid, &port);
			//printf("getPort 11 \n");
			return port;
		}

		// otherwise it should be one of the local interfaces at the veil switch,
		// check it:
		else if (interfaces->lookupVidEntry(&nexthop, &temp)){
			memcpy(&myVid, &nexthop,6);
			k = myVid.logical_distance(&dstvid);
			// now look up in the routing table for this local interface.
			veil_chatter_new(printDebugMessages, class_name()," For Dest VID: |%s|  Nexthop: |%s| is my local interface.",dstvid.switchVIDString().c_str(),nexthop.switchVIDString().c_str());
		}
		else{
			veil_chatter_new(true, class_name(),"[ERROR][NEXTHOP IS NEITHER A PHYSICAL NEIGHBOR, NOR MY LOCAL INTERFACE] I SHOULD NEVER REACH HERE:  For Dest VID: |%s|  MyVID: |%s| NextHop: |%s| ",dstvid.switchVIDString().c_str(),myVid.switchVIDString().c_str(), nexthop.switchVIDString().c_str());
			port = -1; 
			return port;
		}
	}
	//printf("getPort 9 port is %d \n", port);
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
		//veil_chatter_new(printDebugMessages, class_name()," Forwarding packet on port %d.",port);
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
				//printf("VEILRoutePacket::getClosestInterfaceVID myvid %s intvid %s \n", myVID.vid_string().c_str(), intvid.vid_string().c_str());
				memcpy(&myVID, &intvid,6);
				xordist = newdist;		
			}
		}
	}
	veil_chatter_new(printDebugMessages, class_name(),"[Closest MyInerface VID] Dest VID: |%s|  MyVID: |%s| XoRDist: %d",dstvid.switchVIDString().c_str(),myVID.switchVIDString().c_str(), xordist);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILRoutePacket)
