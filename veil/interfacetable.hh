#ifndef CLICK_VEIL_INTERFACETABLE_HH
#define CLICK_VEIL_INTERFACETABLE_HH

#include <click/element.hh>
#include <click/hashtable.hh>
#include <click/vid.hh>
#include "click_veil.hh"

CLICK_DECLS

class VEILInterfaceTable : public Element {
	public:
		VEILInterfaceTable();
		~VEILInterfaceTable();

		const char* class_name() const { return "VEILInterfaceTable"; }
		const char* port_count() const { return PORTS_0_0; }
		
		int configure(Vector<String> &, ErrorHandler*);
		bool lookupEntry(VID*, int*);
		static String read_handler(Element*, void*);
		void add_handlers();	
	
	private:
		//could've been a vector but is a hash if we decide to 
		//store more info in the future	
		typedef HashTable<VID, int> InterfaceTable;
		
		InterfaceTable interfaces;		
};

CLICK_ENDDECLS
#endif
