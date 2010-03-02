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
	
		inline const InterfaceTable* get_interfacetable_handle(){
			return &interfaces;
		}

	private:		
		InterfaceTable interfaces;
		ReverseInterfaceTable rinterfaces;
			
};

CLICK_ENDDECLS
#endif
