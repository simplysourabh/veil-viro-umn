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

	//first look at neighbor table and update routing table
	const VEILNeighborTable::NeighborTable *nt = neighbors->get_neighbortable_handle();
	VEILNeighborTable::NeighborTable::const_iterator iter;
	
	for(iter = nt->begin(); iter; ++iter){
		VEILNeighborTable::NeighborTableEntry nte = iter.value();
		VID neighbor = iter.key();
		VID myInterface = nte.myVid;
		uint16_t ldist = neighbor.logical_distance(&myInterface);
		if (ldist == 0){continue;}
		
		route_table->updateEntry(&myInterface, ldist, &neighbor, &myInterface);	
		
		//TODO SJ: WE SHOULD BE PUBLISHING HERE!!
		rdv_publish(myInterface, neighbor,ldist);	
	}

	const VEILInterfaceTable::InterfaceTable *it = interfaces->get_switchinterfacetable_handle();
	VEILInterfaceTable::SwitchInterfaceTable::const_iterator iiter, iiter1, iiter2;
	
	// First go through the interface list, and see if these interfaces
	// can be used by other interfaces to forward the packets for ceratain buckets. 
	VID int1, int2, nexthop1, gateway1; 
	for (iiter1 = it->begin(); iiter1; ++iiter1){
		int1 = iiter1.key();
		//click_chatter("BuildRouteTable: For Interface %s ", int1.switchVIDString().c_str());
		for (iiter2 = it->begin(); iiter2; ++iiter2){
			int2 = iiter2.key();
			uint16_t ldist = int2.logical_distance(&int1);
			if (ldist == 0){continue;}
			//click_chatter("Neighbor %s ", int2.switchVIDString().c_str());			
			route_table->updateEntry(&int1, ldist, &int2, &int1);
			//TODO SJ: WE SHOULD BE PUBLISHING HERE!!
			//TODO: Probably create a wrapper function for the RDV_PUBLISH.
			rdv_publish(int1, int2,ldist);	
		}
		//click_chatter("\n");
	}
	
	//check for each interface
	for(iiter = it->begin(); iiter; ++iiter){
		VID myinterface = iiter.key();
		// ACTIVE_VID_LEN is set to avoid sending queries for the 
		// non-existant levels in the tree. 
		for(uint16_t i = HOST_LEN*8 + 1; i <= ACTIVE_VID_LEN*8; i++){
			VID nexthop, rdvpt;
			// Just Query it everytime! Don't need to see if the entry is already there or not!
			rdv_query(myinterface,i);
		}		
	}
	
	myTimer.schedule_after_msec(VEIL_RDV_INTERVAL);
}

void
VEILBuildRouteTable::rdv_publish (VID &myinterface, VID &nexthop, uint16_t i ){
	VID rdvpt;
	myinterface.calculate_rdv_point(i, &rdvpt);
	int packet_length = sizeof(click_ether) + sizeof(veil_header) + sizeof(VID);
	WritablePacket *p = Packet::make(packet_length);
//veil_chatter(printDebugMessages,"[** BuildRouteTable **] [RDV PUBLISH] Here I am ");
	if (p == 0) {
		veil_chatter(true, "[** BuildRouteTable **] [Error!] cannot make packet in buildroutetable");
		return;
	}
//veil_chatter(printDebugMessages,"[** BuildRouteTable **] [RDV PUBLISH] Here I am 1");
	memset(p->data(), 0, p->length());
//veil_chatter(printDebugMessages,"[** BuildRouteTable **] [RDV PUBLISH] Here I am 2");
	click_ether *e = (click_ether *) p->data();
	p->set_ether_header(e);
//veil_chatter(printDebugMessages,"[** BuildRouteTable **] [RDV PUBLISH] Here I am 3");
	memcpy(e->ether_dhost, &rdvpt, 6); 
	memcpy(e->ether_shost, &myinterface, 6);
//veil_chatter(printDebugMessages,"[** BuildRouteTable **] [RDV PUBLISH] Here I am 4");	
	e->ether_type = htons(ETHERTYPE_VEIL);
//veil_chatter(printDebugMessages,"[** BuildRouteTable **] [RDV PUBLISH] Here I am 5");
	veil_header *vheader = (veil_header*) (e + 1);
//veil_chatter(printDebugMessages,"[** BuildRouteTable **] [RDV PUBLISH] Here I am 6");	
	vheader->packetType = htons(VEIL_RDV_PUBLISH);
//veil_chatter(printDebugMessages,"[** BuildRouteTable **] [RDV PUBLISH] Here I am 7");
	VID *v = (VID*) (vheader + 1);
	memcpy(v, &nexthop, 6); 
//veil_chatter(printDebugMessages,"[** BuildRouteTable **] [RDV PUBLISH] Here I am 8");
	veil_chatter(printDebugMessages,"[** BuildRouteTable **] [RDV PUBLISH] |%s| --> |%s| to RDV node at |%s|\n", myinterface.switchVIDString().c_str(), nexthop.switchVIDString().c_str(), rdvpt.switchVIDString().c_str());
//veil_chatter(printDebugMessages,"[** BuildRouteTable **] [RDV PUBLISH] Here I am 9");	
	//p-> kill();
	output(0).push(p);
}

void
VEILBuildRouteTable::rdv_query (VID &myinterface, uint16_t i){
	VID rdvpt;
	myinterface.calculate_rdv_point(i, &rdvpt);
	int packet_length = sizeof(click_ether) + sizeof(veil_header) + sizeof(uint16_t);		
	WritablePacket *p = Packet::make(packet_length);

	if (p == 0) {
		veil_chatter(true, "[** BuildRouteTable **] [Error!] cannot make packet in buildroutetable");
		return;
	}

	memset(p->data(), 0, p->length());

	click_ether *e = (click_ether *) p->data();
	p->set_ether_header(e);

	memcpy(e->ether_dhost, &rdvpt, 6); 
	memcpy(e->ether_shost, &myinterface, 6);

	e->ether_type = htons(ETHERTYPE_VEIL);

	veil_header *vheader = (veil_header*) (e + 1);
	vheader->packetType = htons(VEIL_RDV_QUERY);

	uint16_t *k = (uint16_t*) (vheader + 1);
	*k = htons(i);
	
	veil_chatter(printDebugMessages, "[** BuildRouteTable **] [RDV QUERY] For |%s| at level %d to RDV node at |%s|\n", myinterface.switchVIDString().c_str(), i, rdvpt.switchVIDString().c_str());
	output(0).push(p);
}



CLICK_ENDDECLS

EXPORT_ELEMENT(VEILBuildRouteTable)
