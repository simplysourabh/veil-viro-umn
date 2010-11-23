#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include <click/vid.hh>
#include "utilities.hh"
#include "buildroutetable.hh"

CLICK_DECLS

VEILBuildRouteTable::VEILBuildRouteTable () : myTimer(this)  {}

VEILBuildRouteTable::~VEILBuildRouteTable () {}

int
VEILBuildRouteTable::configure (
	Vector<String> &conf,
	ErrorHandler *errh)
{
	printDebugMessages = true;
	return cp_va_kparse(conf, this, errh,
		"NEIGHBORTABLE", cpkM+cpkP, cpElementCast, "VEILNeighborTable", &neighbors,
		"ROUTETABLE", cpkM+cpkP, cpElementCast, "VEILRouteTable", &route_table,
		"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
		"PRINTDEBUG", 0, cpBool, &printDebugMessages,
		cpEnd);
}

int
VEILBuildRouteTable::initialize (
	ErrorHandler *errh)
{
	myTimer.initialize(this);
	myTimer.schedule_now();
	return(0);
}

void
VEILBuildRouteTable::run_timer (Timer *timer) 
{
	assert(timer == &myTimer);

	if(!interfaces->isVIDAssignmentDone){
		veil_chatter_new(printDebugMessages, class_name(), "run_timer | VID assignment is not done yet. Will try after %d ms.",VEIL_RDV_INTERVAL);
		myTimer.schedule_after_msec(VEIL_RDV_INTERVAL);
		return;
	}
	//first look at neighbor table and update routing table
	const VEILNeighborTable::NeighborTable *nt = neighbors->get_neighbortable_handle();
	VEILNeighborTable::NeighborTable::const_iterator iter;
	
	for(iter = nt->begin(); iter; ++iter){
		VEILNeighborTable::NeighborTableEntry nte = iter.value();
		EtherAddress macadd = iter.key();
		VID myInterface = nte.myVid;
		veil_chatter_new(printDebugMessages, class_name(), "RDV PUBLISH | For %s", myInterface.vid_string().c_str());
		int intid = interfaces->interfaces.get(myInterface);
		if (interfaces->isvidset[interfaces->interfaces[myInterface]] == false){ continue;}
		VID neighbor = nte.neighborVID;
		uint8_t ldist = neighbor.logical_distance(&myInterface);
		if (ldist == 0){continue;}
		
		route_table->updateEntry(&myInterface, ldist, &neighbor, &myInterface);	
		
		//TODO SJ: WE SHOULD BE PUBLISHING HERE!!
		veil_chatter_new(printDebugMessages, class_name(), "RDV PUBLISH | For %s at bucket %d through %s", myInterface.vid_string().c_str(), ldist, neighbor.vid_string().c_str());
		rdv_publish(myInterface, neighbor,ldist);	
	}

	const VEILInterfaceTable::InterfaceTable *it = interfaces->get_InterfaceTable_handle();
	VEILInterfaceTable::InterfaceTable::const_iterator iiter, iiter1, iiter2;
	
	// First go through the interface list, and see if these interfaces
	// can be used by other interfaces to forward the packets for ceratain buckets. 
	VID int1, int2, nexthop1, gateway1; 
	for (iiter1 = it->begin(); iiter1; ++iiter1){
		int1 = iiter1.key();
		veil_chatter_new(printDebugMessages, class_name(), "RDV PUBLISH 2 | For %s", int1.vid_string().c_str());
		if (interfaces->isvidset[interfaces->interfaces[int1]] == false){ continue;}
		printf("BuildRouteTable: For Interface %s ", int1.switchVIDString().c_str());
		for (iiter2 = it->begin(); iiter2; ++iiter2){
			int2 = iiter2.key();
			uint8_t ldist = int2.logical_distance(&int1);
			if (ldist == 0){continue;}
			printf("Neighbor %s at distance %d", int2.switchVIDString().c_str(),ldist);
			route_table->updateEntry(&int1, ldist, &int2, &int1);
			printf("Returned!\n");
			//TODO SJ: WE SHOULD BE PUBLISHING HERE!!
			//TODO: Probably create a wrapper function for the RDV_PUBLISH.
			veil_chatter_new(printDebugMessages, class_name(), "RDV PUBLISH | For %s at bucket %d through %s", int1.vid_string().c_str(), ldist, int2.vid_string().c_str());
			printf("Returned2!\n");
			rdv_publish(int1, int2,ldist);
			printf("Returned3!\n");
		}
		printf("\n");
	}
	
	//check for each interface
	for(iiter = it->begin(); iiter; ++iiter){
		VID myinterface = iiter.key();
		veil_chatter_new(printDebugMessages, class_name(), "RDV QUERY | For %s", myinterface.vid_string().c_str());
		if (interfaces->isvidset[interfaces->interfaces[myinterface]] == false){ continue;}
		// ACTIVE_VID_LEN is set to avoid sending queries for the 
		// non-existant levels in the tree. 
		for(uint8_t i = HOST_LEN*8 + 1; i <= ACTIVE_VID_LEN*8; i++){
			VID nexthop, rdvpt;
			// Just Query it everytime! Don't need to see if the entry is already there or not!
			veil_chatter_new(printDebugMessages, class_name(), "RDV QUERY | For %s at bucket %d", myinterface.vid_string().c_str(),i);
			rdv_query(myinterface,i);
		}		
	}
	
	myTimer.schedule_after_msec(VEIL_RDV_INTERVAL);
}

void
VEILBuildRouteTable::rdv_publish (VID &myinterface, VID &nexthop, uint8_t i ){
	VID rdvpt;
	myinterface.calculate_rdv_point(i, &rdvpt);
	int packet_length = sizeof(click_ether) + sizeof(veil_sub_header) + sizeof(veil_payload_rdv_publish);
	WritablePacket *p = Packet::make(packet_length);
	if (p == 0) {
		veil_chatter_new(true, class_name(),"[Error!] cannot make packet in buildroutetable");
		return;
	}

	memset(p->data(), 0, p->length());

	click_ether *e = (click_ether *) p->data();
	p->set_ether_header(e);

	//TODO: Make sure routepacket overwrites the destination address before sending it out to the wire.
	memcpy(e->ether_dhost, &rdvpt, 6);
	memcpy(e->ether_shost, &myinterface, 6);

	e->ether_type = htons(ETHERTYPE_VEIL);

	veil_sub_header *vheader = (veil_sub_header*) (e + 1);
	memcpy (&vheader->dvid, &rdvpt, 6);
	memcpy (&vheader->svid, &myinterface, 6);
	vheader->veil_type = htons(VEIL_RDV_PUBLISH);
	vheader->ttl = MAX_TTL;

	veil_payload_rdv_publish *vpayload = (veil_payload_rdv_publish*)(vheader+1);
	vpayload->bucket = i;
	memcpy(&vpayload->neighbor_vid, &nexthop, 4);

	veil_chatter_new(printDebugMessages,class_name(),"[RDV PUBLISH] |%s| --> |%s| to RDV node at |%s|", myinterface.switchVIDString().c_str(), nexthop.switchVIDString().c_str(), rdvpt.switchVIDString().c_str());
	output(0).push(p);
}

void
VEILBuildRouteTable::rdv_query (VID &myinterface, uint8_t i){
	VID rdvpt;
	myinterface.calculate_rdv_point(i, &rdvpt);
	int packet_length = sizeof(click_ether) + sizeof(veil_sub_header) + sizeof(veil_payload_rdv_query);
	WritablePacket *p = Packet::make(packet_length);

	if (p == 0) {
		veil_chatter_new(true, class_name(),"[Error!] cannot make packet in buildroutetable");
		return;
	}

	memset(p->data(), 0, p->length());

	click_ether *e = (click_ether *) p->data();
	p->set_ether_header(e);

	//TODO: Make sure routepacket overwrites the destination address before sending it out to the wire.
	memcpy(e->ether_dhost, &rdvpt, 6); 
	memcpy(e->ether_shost, &myinterface, 6);

	e->ether_type = htons(ETHERTYPE_VEIL);

	veil_sub_header *vheader = (veil_sub_header*) (e + 1);
	memcpy (&vheader->dvid, &rdvpt, 6);
	memcpy (&vheader->svid, &myinterface, 6);
	vheader->veil_type = htons(VEIL_RDV_QUERY);
	vheader->ttl = MAX_TTL;

	veil_payload_rdv_query *vpayload = (veil_payload_rdv_query*)(vheader+1);
	vpayload->bucket = i;
	
	veil_chatter_new(printDebugMessages, class_name(),"[RDV QUERY] For |%s| at level %d to RDV node at |%s|", myinterface.switchVIDString().c_str(), i, rdvpt.switchVIDString().c_str());
	output(0).push(p);
}



CLICK_ENDDECLS

EXPORT_ELEMENT(VEILBuildRouteTable)
