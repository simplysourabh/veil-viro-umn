#ifndef CLICK_VEIL_INTERFACETABLE_HH
#define CLICK_VEIL_INTERFACETABLE_HH

#include <click/element.hh>
#include <click/hashtable.hh>
#include <click/vid.hh>
#include "click_veil.hh"

CLICK_DECLS

class VEILInterfaceTable : public Element {
	public:
		//could've been a vector but is a hash if we decide to 
		//store more info in the future	
		typedef HashTable<VID, int> InterfaceTable;
		typedef HashTable<int, VID> ReverseInterfaceTable;
		//we need to keep track of which interfaces are connected to 
		//other switches. needed to build the routing table
		//typedef HashTable<VID, int> SwitchInterfaceTable; //SJ: Since I
		// don't plan to use it as of now, so commenting it temporarily.
		
		// it keeps the mapping from the MAC address to interface index
		typedef HashTable<EtherAddress, int> EtherAddrInterfaceTable;

		// it keeps the mapping from the interface index to the MAC addresss
		typedef HashTable<int, EtherAddress> InterfaceEtherAddr;

		VEILInterfaceTable();
		~VEILInterfaceTable();

		const char* class_name() const { return "VEILInterfaceTable"; }
		const char* port_count() const { return PORTS_0_0; }
		
		int configure(Vector<String> &, ErrorHandler*);
		bool lookupVidEntry(VID*, int*);
		bool lookupIntEntry(int, VID*);
		bool getMacAddr(int, EtherAddress*);
		bool getIntUsingMac(EtherAddress*, int*);
		int numInterfaces(){
			return interfaces.size();
		}
		bool isVIDassigned(){
			return isVIDAssignmentDone;
		}
		static String read_handler(Element*, void*);
		void add_handlers();
	
		//void deleteHostInterface(VID*);
		//SJ: commenting the follwoing for the time being.
		/*inline const SwitchInterfaceTable* get_switchinterfacetable_handle(){
			return &switch_interfaces;
		}*/

		inline const InterfaceTable* get_InterfaceTable_handle(){
			return &interfaces;
		}

		inline const EtherAddrInterfaceTable* get_EtherAddrInterfaceTable_handle(){
			return &etheraddToInterfaceIndex;
		}
		bool isVIDAssignmentDone;
	private:		
		InterfaceTable interfaces;
		ReverseInterfaceTable rinterfaces;
		//SwitchInterfaceTable switch_interfaces; //SJ: commenting it.
		EtherAddrInterfaceTable etheraddToInterfaceIndex;
		InterfaceEtherAddr interfaceIndexToEtherAddr;
		EtherAddrInterfaceTable etheraddToInterfaceIndex;
		InterfaceEtherAddr interfaceIndexToEtherAddr;
		bool printDebugMessages ;	
};

CLICK_ENDDECLS
#endif
