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
			EtherAddress myMac;
			VID neighborVID;
			Timer *expiry;
		};

		// we keep the mapping from the neighbor mac address to neighbor table entry.
		typedef HashTable<EtherAddress, NeighborTableEntry> NeighborTable;

		struct TimerData {
			NeighborTable *neighbors;
			VEILNeighborTable *ntable;
			EtherAddress *neighbormac;
		};

		VEILNeighborTable();
		~VEILNeighborTable();

		const char* class_name() const { return "VEILNeighborTable"; }
		const char* port_count() const { return PORTS_0_0; }

		int cp_neighbor(String, ErrorHandler*);

		int configure(Vector<String>&, ErrorHandler*);	
		bool updateEntry(EtherAddress *neighrbormac, VID* neighborvid, EtherAddress* mymac, VID* myvid);
		bool lookupEntry(EtherAddress neighbormac, NeighborTableEntry* entry);
		bool lookupEntry(VID *nvid, VID* myvid);

		inline const NeighborTable* get_neighbortable_handle(){
			return &neighbors;
		}		

		static void expire(Timer*, void*);
		static String read_handler(Element*, void*);
		void add_handlers();
		NeighborTable neighbors; // making it public to allow easy manipulations.
		// however, in future we'd like to make it a private member.
	private:		
		bool printDebugMessages;
};

CLICK_ENDDECLS
#endif
