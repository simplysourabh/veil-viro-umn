#ifndef CLICK_VEILNEIGHBORTABLE_HH
#define CLICK_VEILNEIGHBORTABLE_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>
#include <click/vid.hh>
#include "click_veil.hh"

CLICK_DECLS

class VEILNeighborTable : public Element {
	public:
		VEILNeighborTable();
		~VEILNeighborTable();

		const char* class_name() const { return "VEILNeighborTable"; }
		const char* port_count() const { return PORTS_0_0; }
		//const char* processing() const { return AGNOSTIC; }
	
		//int configure(Vector<String>&, ErrorHandler*);
		void updateEntry(VID*, String);
		//void printNeighborTable();
		static void expire(Timer*, void*);
		static String read_handler(Element*, void*);
		void add_handlers();	
	
	private:
		//each entry will keep track of neighbor's VID and VID of 
                //the interface the neighbor is connected to
		struct NeighborTableEntry {
			//VID myVid;
			String interface;
			Timer *expiry;
			//bool valid;
		};

		typedef HashTable<VID, NeighborTableEntry> NeighborTable;
		
		NeighborTable neighbors;

		struct TimerData {
			NeighborTable *neighbors;
			VID *vid;
		};					
};

CLICK_ENDDECLS
#endif
