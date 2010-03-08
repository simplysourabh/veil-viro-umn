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
		typedef HashTable<VID, int> SwitchInterfaceTable;
		
		VEILInterfaceTable();
		~VEILInterfaceTable();

		const char* class_name() const { return "VEILInterfaceTable"; }
		const char* port_count() const { return PORTS_0_0; }
		
		int configure(Vector<String> &, ErrorHandler*);
		bool lookupVidEntry(VID*, int*);
		bool lookupIntEntry(int, VID*);
		int numInterfaces(){
			return interfaces.size();
		}
		static String read_handler(Element*, void*);
		void add_handlers();
	
		void deleteHostInterface(VID*);
		inline const SwitchInterfaceTable* get_switchinterfacetable_handle(){
			return &switch_interfaces;
		}

	private:		
		InterfaceTable interfaces;
		ReverseInterfaceTable rinterfaces;
		SwitchInterfaceTable switch_interfaces;
			
};

CLICK_ENDDECLS
#endif
