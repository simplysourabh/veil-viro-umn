#ifndef CLICK_VEILBUILDROUTETABLE_HH
#define CLICK_VEILBUILDROUTETABLE_HH
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashmap.hh>
#include "neighbortable.hh"
#include "routetable.hh"
#include "interfacetable.hh"

//there is one instance of this element per switch

CLICK_DECLS

class VEILBuildRouteTable: public Element {
	public:
		VEILBuildRouteTable();
		~VEILBuildRouteTable();

		const char* class_name() const { return "VEILBuildRouteTableRequest"; }
		const char* port_count() const { return PORTS_0_1; } 
		const char* processing() const { return "a/h"; }

		int configure(Vector<String> &, ErrorHandler *);
		int initialize(ErrorHandler *);
		void run_timer(Timer *);

	private:
		Timer myTimer;
	
		VEILNeighborTable *neighbors;
		VEILRouteTable *route_table;
		VEILInterfaceTable *interfaces;
			
};

CLICK_ENDDECLS
#endif