#ifndef CLICK_VEILROUTEPACKET_HH
#define CLICK_VEILROUTEPACKET_HH
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashmap.hh>
#include "neighbortable.hh"
#include "hosttable.hh"
#include "routetable.hh"
#include "interfacetable.hh"

CLICK_DECLS

class VEILRoutePacket: public Element {
	public:
		VEILRoutePacket();
		~VEILRoutePacket();

		const char* class_name() const { return "VEILRoutePacketRequest"; }
		const char* port_count() const { return "1/-"; } 
		const char* processing() const { return PUSH; }

		int configure(Vector<String>&, ErrorHandler*);
		//this method returns the output port #		
		int smaction(Packet*);
	 	virtual void push (int, Packet*);

	private:
		VEILHostTable *hosts;
		VEILRouteTable *routes;
		VEILInterfaceTable *interfaces;
		VEILNeighborTable *neighbors;
			
};

CLICK_ENDDECLS
#endif