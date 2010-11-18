/*
 * This click element processes the Spanning Tree Advertisement, and Spanning Tree Subscription
 * Packets.
 * It does not react to any incoming packets by sending packets out of the
 * network, it simply stores them in the relevant tables.
 * Sourabh Jain (sourj@cs.umn.edu)
 */

#ifndef CLICK_VEILPROCESSVCCSTADSUB_HH
#define CLICK_VEILPROCESSVCCSTADSUB_HH
#include <click/element.hh>
#include "spanningtreestate.hh"
#include "interfacetable.hh"
#include "neighbortable.hh"
#include "networkTopoVIDAssignment.hh"
#include "click_veil.hh"

#include <click/hashmap.hh>
#include <click/vid.hh>
#include "utilities.hh"
#include "click_veil.hh"
#include "hosttable.hh"
#include "mappingtable.hh"

CLICK_DECLS

class VEILProcessVCCSTAdSub : public Element {
	public:

		VEILProcessVCCSTAdSub();
		~VEILProcessVCCSTAdSub();

		const char *class_name() const { return "VEILProcessVCCSTAdSub"; }
		const char *port_count() const { return "1/-"; } // 1 input ports, and any number of output ports. # of output ports
				// is same as the number of interfaces on the router.

		const char *processing() const { return PUSH; }
		int configure(Vector<String>&, ErrorHandler*);

		int smaction(Packet*);
	 	virtual void push (int, Packet*);

	 	void processAD(Packet*p);
	 	void processSUBS(Packet*p);
	 	int processLocalTopo(Packet*p);
	 	int processSwitchVID(Packet*p);

	private:

		VEILInterfaceTable *interfaces;
		VEILSpanningTreeState *ststate;
		VEILNeighborTable *neighbors;
		VEILNetworkTopoVIDAssignment *topo;
		bool printDebugMessages;

};

CLICK_ENDDECLS
#endif
