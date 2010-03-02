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
		//each entry will keep track of neighbor's VID and VID of 
                //the interface the neighbor is connected to
		struct NeighborTableEntry {
			VID myVid;
			//String interface;
			Timer *expiry;
		};

		typedef HashTable<VID, NeighborTableEntry> NeighborTable;

		struct TimerData {
			NeighborTable *neighbors;
			VID *vid;
		};

		VEILNeighborTable();
		~VEILNeighborTable();

		const char* class_name() const { return "VEILNeighborTable"; }
		const char* port_count() const { return PORTS_0_0; }
	
		void updateEntry(VID*, VID*);
		bool lookupEntry(VID*, VID*);

		inline const NeighborTable* get_neighbortable_handle(){
			return &neighbors;
		}		

		static void expire(Timer*, void*);
		static String read_handler(Element*, void*);
		void add_handlers();

	private:		
		NeighborTable neighbors;
						
};

CLICK_ENDDECLS
#endif
