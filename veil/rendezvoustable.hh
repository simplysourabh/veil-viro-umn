#ifndef CLICK_VEILRENDEZVOUSTABLE_HH
#define CLICK_VEILRENDEZVOUSTABLE_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>
#include <click/error.hh>
#include <click/vid.hh>
#include "click_veil.hh"


CLICK_DECLS

class VEILRendezvousTable : public Element {
	public:
		VEILRendezvousTable();
		~VEILRendezvousTable();

		const char* class_name() const { return "VEILRendezvousTable"; }
		const char* port_count() const { return PORTS_0_0; }

		int cp_rdv_entry(String, ErrorHandler*);
	
		int configure(Vector<String>&, ErrorHandler*);
		void updateEntry(VID *src, VID *dest);
		uint8_t getRdvPoint(int, VID *src, VID *gateway);
		static void expire(Timer*, void*);
		static String read_handler(Element*, void*);
		void add_handlers();	
	
	private:
		struct RendezvousTableEntry {
			Timer *expiry;
		};

		//no real reason for this to be a hash
		typedef HashTable<RendezvousEdge, RendezvousTableEntry> RendezvousTable;
		
		RendezvousTable rdvedges;

		struct TimerData {
			RendezvousTable *rdvedges;
			RendezvousEdge *edge;
		};
		bool printDebugMessages ;
};

CLICK_ENDDECLS
#endif
