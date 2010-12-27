#ifndef CLICK_VEILPROCESSHELLONEW_HH
#define CLICK_VEILPROCESSHELLONEW_HH
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashmap.hh>
#include "utilities.hh"
#include "interfacetable.hh"
#include "neighbortable.hh"

CLICK_DECLS

class VEILProcessHelloNew : public Element {
	public:
		VEILProcessHelloNew();
		~VEILProcessHelloNew();

		const char* class_name() const { return "VEILProcessHelloNew"; }
		const char* port_count() const { return "1/-"; }
		const char* processing() const { return PUSH; }

		int configure(Vector<String>&, ErrorHandler*);
		Packet* smaction(Packet*);
		virtual void push(int port, Packet *p);

	private:
		VEILNeighborTable *neighbor_table;		
		VEILInterfaceTable *interfaces;
		bool printDebugMessages ;
};

CLICK_ENDDECLS
#endif
