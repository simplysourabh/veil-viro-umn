#ifndef CLICK_VEILRENDEZVOUSTABLE_HH
#define CLICK_VEILRENDEZVOUSTABLE_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>
#include <click/vid.hh>
#include "click_veil.hh"

CLICK_DECLS

class VEILRendezvousTable : public Element {
	public:
		VEILRendezvousTable();
		~VEILRendezvousTable();

		const char* class_name() const { return "VEILRendezvousTable"; }
		const char* port_count() const { return PORTS_0_0; }
	
		void updateEntry(VID*, VID*);
		bool getRdvPoint(int, VID*, VID*);
		static void expire(Timer*, void*);
		static String read_handler(Element*, void*);
		void add_handlers();	
	
	private:
		struct RendezvousTableEntry {
			VID gatewayVid;
			Timer *expiry;
		};

		//no real reason for this to be a hash
		typedef HashTable<VID, RendezvousTableEntry> RendezvousTable;
		
		RendezvousTable rdvpts;

		struct TimerData {
			RendezvousTable *rdvpts;
			VID *vid;
		};					
};

CLICK_ENDDECLS
#endif
