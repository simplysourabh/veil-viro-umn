#ifndef CLICK_VEILPROCESSACCESSINFO_HH
#define CLICK_VEILPROCESSACCESSINFO_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashmap.hh>
#include "utilities.hh"
#include "click_veil.hh"
#include "mappingtable.hh"
#include "interfacetable.hh"
#include "hosttable.hh"

CLICK_DECLS

class VEILProcessAccessInfo : public Element {
	public:
		VEILProcessAccessInfo();
		~VEILProcessAccessInfo();

		const char* class_name() const { return "VEILProcessAccessInfo"; }
		const char* port_count() const { return PORTS_1_1; }
		const char* processing() const { return PUSH; }

		int configure(Vector<String>&, ErrorHandler*);
		Packet* smaction(Packet*);
	 	virtual void push (int, Packet*);		

	private:
		VEILInterfaceTable *interfaces;
		VEILMappingTable *map;
		VEILHostTable *hosts;
		bool printDebugMessages;
			
};

CLICK_ENDDECLS
#endif












