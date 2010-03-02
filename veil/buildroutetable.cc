#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include <click/vid.hh>
#include "utilities.hh"
#include "click_veil.hh"
#include "buildroutetable.hh"

CLICK_DECLS

VEILBuildRouteTable::VEILBuildRouteTable () {}

VEILBuildRouteTable::~VEILBuildRouteTable () {}

int
VEILBuildRouteTable::configure (
	Vector<String> &conf,
	ErrorHandler *errh)
{
	return cp_va_kparse(conf, this, errh,
		"NEIGHBORTABLE", cpkM+cpkP, cpElementCast, "VEILNeighborTable", &neighbors,
		"ROUTETABLE", cpkM+cpkP, cpElementCast, "VEILRouteTable", &route_table,
		"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
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
VEILBuildRouteTable::run_timer (
	Timer *timer) 
{
	assert(timer == &myTimer);

	//first look at neighbor table and update routing table
	const VEILNeighborTable::NeighborTable *nt = neighbors->get_neighbortable_handle();
	VEILNeighborTable::NeighborTable::const_iterator iter;
	
	for(iter = nt->begin(); iter; ++iter){
		VEILNeighborTable::NeighborTableEntry nte = iter.value();
		VID neighbor = iter.key();
		VID myInterface = nte.myVid;
		int ldist = neighbor.logical_distance(&myInterface);

		route_table->updateEntry(&myInterface, ldist, &neighbor, &myInterface);		
	}

	const VEILInterfaceTable::InterfaceTable *it = interfaces->get_interfacetable_handle();
	VEILInterfaceTable::InterfaceTable::const_iterator iiter;

	//send out rdv publish and queries
	for(int i = 0; i < VID_LEN*8; i++){
		VID myinterface, nexthop, gateway, rdvpt;
		//check for each interface
		for(iiter = it->begin(); iiter; ++iiter){
			myinterface = iiter.key();
			//publish
			if(route_table->lookupEntry(i, &myinterface, &nexthop, &gateway)){
				//check if nexthop = neighbor
				VID interface;			
				if(neighbors->lookupEntry(&nexthop, &interface)){
					myinterface.calculate_rdv_point(i, &rdvpt);
				
					int packet_length = sizeof(click_ether) + sizeof(veil_header) + sizeof(VID);
					WritablePacket *p = Packet::make(packet_length);

        				if (p == 0) {
                				click_chatter( "cannot make packet in buildroutetable");
                				return;
        				}

					memset(p->data(), 0, p->length());

					click_ether *e = (click_ether *) p->data();
					p->set_ether_header(e);
		
					memcpy(e->ether_dhost, &rdvpt, 6); 
					memcpy(e->ether_shost, &myinterface, 6);
					e->ether_type = htons(ETHERTYPE_VEIL);

					veil_header *vheader = (veil_header*) (e + 1);
					vheader->packetType = htons(VEIL_RDV_PUBLISH);

					VID *v = (VID*) (vheader + 1);
					memcpy(v, &nexthop, 6); 
				
					output(0).push(p);
				} else goto query;		
			} else { //query
				//send a query on all interfaces to maximize 
				//chances of getting some route
				query:
				myinterface.calculate_rdv_point(i, &rdvpt);

				int packet_length = sizeof(click_ether) + sizeof(veil_header) + sizeof(int);		
				WritablePacket *p = Packet::make(packet_length);

	        		if (p == 0) {
	                		click_chatter( "cannot make packet in buildroutetable");
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

				int *k = (int*) (vheader + 1);
				*k = htons(i);
				
				output(0).push(p);
			}
		}		
	}
	
	myTimer.schedule_after_msec(VEIL_RDV_INTERVAL);
}


CLICK_ENDDECLS

EXPORT_ELEMENT(VEILBuildRouteTable)