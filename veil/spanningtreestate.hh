//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
#ifndef CLICK_VEIL_SPANNINGTREESTATE_HH
#define CLICK_VEIL_SPANNINGTREESTATE_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>
#include <click/vid.hh>
#include "click_veil.hh"


CLICK_DECLS

class VEILSpanningTreeState : public Element {
	public:
		// each entry will have the form:
		// VCC MAC -> ParentMAC (forwarding node), cost to reach VCC
		struct ParentEntry {
			EtherAddress ParentMac; // MAC address of the forwarding node
			uint16_t hopsToVcc;
			Timer *expiry;
		};

		// VCC MAC -> ParentMAC (forwarding node), cost to reach VCC
		typedef HashTable<EtherAddress, ParentEntry> ForwardingTableToVCC;

		struct TimerData {
			void *ftable;
			VEILSpanningTreeState *ststate;
			EtherAddress mac;
			bool isParentEntry;
		};

		struct ChildEntry {
			EtherAddress VccMac;
			Timer *expiry;
		};

		// ChildMAC -> VCC MAC (forwarding node)
		typedef HashTable<EtherAddress, ChildEntry> ForwardingTableFromVCC;

		VEILSpanningTreeState();
		~VEILSpanningTreeState();

		const char* class_name() const { return "VEILSpanningTreeState"; }
		const char* port_count() const { return PORTS_0_0; }

		int cp_spanning_tree_state(String s, ErrorHandler* errh);
		int configure(Vector<String>&, ErrorHandler*);

		bool updateCostToVCC(EtherAddress neighbormac, EtherAddress vccmac, uint16_t cost);
		bool updateChild(EtherAddress childmac, EtherAddress vccmac);
		bool getParentNodeToVCC(EtherAddress vccmac, EtherAddress *parent);
		inline const ForwardingTableToVCC* get_ForwardingTableToVCC_handle(){
			return &forwardingTableToVCC;
		}		

		inline const ForwardingTableFromVCC* get_ForwardingTableFromVCC_handle(){
			return &forwardingTableFromVCC;
		}

		static void expire(Timer*, void*);
		static String read_handler(Element*, void*);
		void add_handlers();

		ForwardingTableToVCC forwardingTableToVCC; // making it public to allow easy manipulations.
		// however, in future we'd like to make it a private member.
		ForwardingTableFromVCC forwardingTableFromVCC;
	private:		
		bool printDebugMessages;
};

CLICK_ENDDECLS
#endif
