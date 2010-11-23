#ifndef CLICK_VEILROUTETABLE_HH
#define CLICK_VEILROUTETABLE_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/vector.hh>
#include <click/hashtable.hh>
#include <click/vid.hh>
#include "click_veil.hh"
#include "interfacetable.hh"

CLICK_DECLS

class VEILRouteTable : public Element {
	public:
		// Routing Table Entry for an interface. 
		// Each entry has three tuples: NextHop, Gateway and Expiry Time
		struct RoutingEntry {
			VID nexthop;
			VID gateway;
			//Timer *expiry;
			bool isDefault;
			bool isValid;
		};

		struct Bucket{
			RoutingEntry buckets[MAX_GW_PER_BUCKET];
			Timer *expiry;
		};

		// Routing Table for an interface is a <key, value> pair
		// Key = Bucket Number (int), Value = Nexthop, gateway, timer (InnerRouteTableEntry)
		//typedef HashTable<uint8_t, InnerRouteTableEntry> InnerRouteTable;
		typedef HashTable<uint8_t, Bucket> InnerRouteTable;
		
		// Full Routing Table, a <key, value> pair
		// Key = VID of the interface, Value = InnerRouteTable
		// SJ Changing the OuterRouteTable to key = int and value = inner_route_table. on Nov 22.
		typedef HashTable<int, InnerRouteTable> OuterRouteTable;

		struct TimerData {
			OuterRouteTable *routes;
			int interface_id;
			uint8_t bucket;
			VID *gateway;
		};
		
		VEILRouteTable();
		~VEILRouteTable();

		const char* class_name() const { return "VEILRouteTable"; }
		const char* port_count() const { return PORTS_0_0; }
	
		int cp_viro_route(String, ErrorHandler*);

		int configure(Vector<String>&, ErrorHandler*);
		void updateEntry(VID *i, uint8_t b, VID *nh, VID *g, bool isDefault=true);
		bool getRoute(VID*, VID, VID*, VID*, bool isDefault=true);
		bool  getBucket(uint8_t, VID*, VID*);
		static void expire(Timer*, void*);
		static String read_handler(Element*, void*);
		void add_handlers();	
		OuterRouteTable routes;
	private:	
		bool printDebugMessages ;
		VEILInterfaceTable *interfaces;
							
};

CLICK_ENDDECLS
#endif


				
