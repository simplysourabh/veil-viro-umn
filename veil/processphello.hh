#ifndef CLICK_VEILPROCESSPHELLO_HH
#define CLICK_VEILPROCESSPHELLO_HH
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashmap.hh>
#include "utilities.hh"
#include "interfacetable.hh"
#include "neighbortable.hh"

CLICK_DECLS

class VEILProcessPHello : public Element {
	public:
		VEILProcessPHello();
		~VEILProcessPHello();

		const char* class_name() const { return "VEILProcessPHello"; }
		const char* port_count() const { return PORTS_1_0; }
		const char* processing() const { return AGNOSTIC; }

		int configure(Vector<String>&, ErrorHandler*);
		Packet* simple_action(Packet*);		

	private:
		VEILNeighborTable *neighbor_table;		
		VEILInterfaceTable *interfaces;
		bool printDebugMessages ;
};

CLICK_ENDDECLS
#endif
