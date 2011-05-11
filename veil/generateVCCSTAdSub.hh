//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
/*
 * This click element generates the Spanning Tree Advertisement, and Spanning Tree Subscription
 * Packets periodically.
 *
 * Sourabh Jain (sourj@cs.umn.edu)
 */

#ifndef CLICK_VEILGENERATEVCCSTADSUB_HH
#define CLICK_VEILGENERATEVCCSTADSUB_HH
#include <click/element.hh>
#include <click/timer.hh>
#include "spanningtreestate.hh"
#include "interfacetable.hh"
#include "neighbortable.hh"
#include "click_veil.hh"


CLICK_DECLS

class VEILGenerateVCCSTAdSub : public Element {
	public:

		VEILGenerateVCCSTAdSub();
		~VEILGenerateVCCSTAdSub();

		const char *class_name() const { return "VEILGenerateVCCSTAdSub"; }
		const char *port_count() const { return "0/-"; } // 0 input ports, and any number of output ports. # of output ports
		// is same as the number of interfaces on the router.

		const char *processing() const { return PUSH; }

		int configure(Vector<String> &, ErrorHandler *);
		int initialize(ErrorHandler *);

		void run_timer(Timer *);
		void sendNeighborInfotoVCC();
		void sendVCCAdandSubPackets();

	private:
		Timer myTimer;
		VEILInterfaceTable *interfaces;
		VEILSpanningTreeState *ststate;
		VEILNeighborTable *neighbors;
		bool printDebugMessages;

};

CLICK_ENDDECLS
#endif
