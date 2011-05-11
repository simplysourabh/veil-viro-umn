//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
#ifndef CLICK_VEILPROCESSARPREQUEST_HH
#define CLICK_VEILPROCESSARPREQUEST_HH
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashmap.hh>
#include "hosttable.hh"
#include "mappingtable.hh"
#include "interfacetable.hh"

CLICK_DECLS

class VEILProcessARP: public Element {
	public:
		VEILProcessARP();
		~VEILProcessARP();

		const char* class_name() const { return "VEILProcessARP"; }
		const char* port_count() const { return PORTS_1_1; } 
		//const char* processing() const { return PUSH; }
		const char* processing() const { return "a/h"; }

		int configure(Vector<String>&, ErrorHandler*);
		Packet* smaction(Packet*);
	 	virtual void push (int, Packet*);

	private:

		VEILHostTable *host_table;
		VEILMappingTable *map;
		VEILInterfaceTable *interfaces;
		bool printDebugMessages ;
};

CLICK_ENDDECLS
#endif
