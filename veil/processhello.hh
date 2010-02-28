#ifndef CLICK_VEILPROCESSHELLO_HH
#define CLICK_VEILPROCESSHELLO_HH
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashmap.hh>
#include "neighbortable.hh"

CLICK_DECLS

class VEILProcessHello : public Element {
	public:
		VEILProcessHello();
		~VEILProcessHello();

		const char* class_name() const { return "VEILProcessHello"; }
		const char* port_count() const { return PORTS_1_0; }
		const char* processing() const { return AGNOSTIC; }

		int configure(Vector<String>&, ErrorHandler*);
		Packet* simple_action(Packet*);		

	private:
		VEILNeighborTable *neighbor_table;
                /* which interface is this element connected to
		 * if we have multiple interfaces on a dev
	         * and hence multiple VEILProcessHello's
		 * the routing element can use this info
		 * to send pkts to neighbors out the right interface
		 */	
		 //String interface;
		VID myVid;
			
};

CLICK_ENDDECLS
#endif
