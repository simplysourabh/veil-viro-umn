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
		// Routing Table Entry for an interface. 
		// Each entry has three tuples: NextHop, Gateway and Expiry Time
		struct InnerRouteTableEntry {
			VID nextHop;
			VID gateway;
			Timer *expiry;
		};

		// Routing Table for an interface is a <key, value> pair
		// Key = Bucket Number (int), Value = Nexthop, gateway, timer (InnerRouteTableEntry)
		typedef HashTable<uint16_t, InnerRouteTableEntry> InnerRouteTable;
		
		// Full Routing Table, a <key, value> pair
		// Key = VID of the interface, Value = InnerRouteTable
		typedef HashTable<VID, InnerRouteTable> OuterRouteTable;

		struct TimerData {
			OuterRouteTable *routes;
			VID *interface;
			uint16_t bucket;
		};
		
		VEILRouteTable();
		~VEILRouteTable();

		const char* class_name() const { return "VEILRouteTable"; }
		const char* port_count() const { return PORTS_0_0; }
	
		int cp_viro_route(String, ErrorHandler*);

		int configure(Vector<String>&, ErrorHandler*);
		void updateEntry(VID*, uint16_t, VID*, VID*);
		bool getRoute(VID*, uint16_t, VID, VID*, VID*);
		bool  getBucket(uint16_t, VID*, VID*);
		static void expire(Timer*, void*);
		static String read_handler(Element*, void*);
		void add_handlers();	
	
	private:	
		OuterRouteTable routes;
							
};

CLICK_ENDDECLS
#endif


				
