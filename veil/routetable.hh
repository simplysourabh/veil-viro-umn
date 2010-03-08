#ifndef CLICK_VEILROUTETABLE_HH
#define CLICK_VEILROUTETABLE_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/vector.hh>
#include <click/hashtable.hh>
#include <click/vid.hh>
#include "click_veil.hh"

CLICK_DECLS

class VEILRouteTable : public Element {
	public:
		struct InnerRouteTableEntry {
			VID nextHop;
			VID gateway;
			Timer *expiry;
		};

		typedef HashTable<int, InnerRouteTableEntry> InnerRouteTable;

		typedef HashTable<VID, InnerRouteTable> OuterRouteTable;

		struct TimerData {
			OuterRouteTable *routes;
			VID *interface;
			int bucket;
		};
		
		VEILRouteTable();
		~VEILRouteTable();

		const char* class_name() const { return "VEILRouteTable"; }
		const char* port_count() const { return PORTS_0_0; }
	
		int cp_viro_route(String, ErrorHandler*);

		int configure(Vector<String>&, ErrorHandler*);
		void updateEntry(VID*, int, VID*, VID*);
		bool getRoute(VID*, int, VID*, VID*, VID*);
		bool  getBucket(int, VID*, VID*);
		static void expire(Timer*, void*);
		static String read_handler(Element*, void*);
		void add_handlers();	
	
	private:	
		OuterRouteTable routes;
							
};

CLICK_ENDDECLS
#endif


				
