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

#ifndef CLICK_VEILGENERATEVIDASSIGNMENTPACKETS_HH
#define CLICK_VEILGENERATEVIDASSIGNMENTPACKETS_HH
#include <click/element.hh>
#include <click/timer.hh>
#include "spanningtreestate.hh"
#include "interfacetable.hh"
#include "neighbortable.hh"
#include "networkTopoVIDAssignment.hh"
#include "click_veil.hh"


CLICK_DECLS

class VEILGenerateVIDAssignmentPackets : public Element {
	public:

		VEILGenerateVIDAssignmentPackets();
		~VEILGenerateVIDAssignmentPackets();

		const char *class_name() const { return "VEILGenerateVIDAssignmentPackets"; }
		const char *port_count() const { return "0/-"; } // 0 input ports, and any number of output ports. # of output ports
		// is same as the number of interfaces on the router.

		const char *processing() const { return PUSH; }

		int configure(Vector<String> &, ErrorHandler *);
		int initialize(ErrorHandler *);

		void run_timer(Timer *);
		void dispatch_vid();
		void dispatch_one_vid_mac_pair(EtherAddress dstmac, VID dstvid);

	private:
		Timer myTimer;
		VEILInterfaceTable *interfaces;
		VEILSpanningTreeState *ststate;
		VEILNeighborTable *neighbors;
		VEILNetworkTopoVIDAssignment *topo;
		bool printDebugMessages;

};

CLICK_ENDDECLS
#endif
