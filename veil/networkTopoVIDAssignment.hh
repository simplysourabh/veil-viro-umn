#ifndef CLICK_VEIL_NETWORKTOPOVIDASSIGNMENT_HH
#define CLICK_VEIL_NETWORKTOPOVIDASSIGNMENT_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>
#include <click/vid.hh>
#include "click_veil.hh"
#include <click/vector.hh>

CLICK_DECLS

class VEILNetworkTopoVIDAssignment : public Element {
	public:

		typedef HashTable<EtherAddress, Vector<EtherAddress> > NetworkTopo;
		typedef HashTable<EtherAddress, VID> EtherToVID;
		typedef HashTable<VID, EtherAddress> VIDtoEther;
		typedef HashTable<EtherAddress, time_t> LastNodeUpdateTime;

		VEILNetworkTopoVIDAssignment();
		~VEILNetworkTopoVIDAssignment();

		const char* class_name() const { return "VEILNetworkTopoVIDAssignment"; }
		const char* port_count() const { return PORTS_0_0; }

		int cp_spanning_tree_state(String s, ErrorHandler* errh);
		int configure(Vector<String>&, ErrorHandler*);
		int cp_an_edge(String s, ErrorHandler* errh);

		bool addAnEdge(EtherAddress fromMac, EtherAddress toMac); // adds a directed edge frommac -> tomac
		bool addAMap(VID vid, EtherAddress mac); // adds a mapping from vid to mac and reverse mapping
		int removeNode(EtherAddress mac); //removes a node from the topology.
		void lastUpdateTimeForNode(EtherAddress mac); // updates and records the last update time for the node.
		bool performVIDAssignment(); // performs the vid assignment, and return true or false
		// if the assignment has changed from the existing assignment.

		static String read_handler(Element*, void*);
		void add_handlers();

		NetworkTopo networktopo; // making it public to allow easy manipulations.
		// however, in future we'd like to make it a private member.
		EtherToVID ether2vid;
		VIDtoEther vid2ether;
		LastNodeUpdateTime lastNodeUpdateTime;
		EtherAddress vccmac;
	private:		
		bool printDebugMessages;
};

CLICK_ENDDECLS
#endif
