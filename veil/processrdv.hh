//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
#ifndef CLICK_VEILPROCESSRDV_HH
#define CLICK_VEILPROCESSRDV_HH
#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashmap.hh>
#include "rendezvoustable.hh"
#include "routetable.hh"
#include "interfacetable.hh"

CLICK_DECLS

class VEILProcessRDV: public Element {
	public:
		VEILProcessRDV();
		~VEILProcessRDV();

		const char* class_name() const { return "VEILProcessRDV"; }
		const char* port_count() const { return PORTS_1_1; } 
		const char* processing() const { return "a/h"; }

		int configure(Vector<String>&, ErrorHandler*);
		Packet* smaction(Packet*);
	 	virtual void push (int, Packet*);

	private:

		VEILRouteTable *routes;
		VEILRendezvousTable *rdvs;
		VEILInterfaceTable *interfaces;
		bool printDebugMessages ;
};

CLICK_ENDDECLS
#endif
