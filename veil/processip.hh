#ifndef CLICK_VEILPROCESSIP_HH
#define CLICK_VEILPROCESSIP_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashmap.hh>
#include <click/vid.hh>
#include "utilities.hh"
#include "click_veil.hh"
#include "hosttable.hh"
#include "interfacetable.hh"
#include "mappingtable.hh"

CLICK_DECLS

class VEILProcessIP : public Element {
	public:
		VEILProcessIP();
		~VEILProcessIP();

		const char* class_name() const { return "VEILProcessIP"; }
		const char* port_count() const { return PORTS_1_1; }
		const char* processing() const { return "a/h"; }

		int configure(Vector<String>&, ErrorHandler*);
		Packet* smaction(Packet*);
	 	virtual void push (int, Packet*);	
		void send_arp_query_packet(IPAddress srcip, IPAddress dstip, VID srcvid, VID myvid);

	private:
		VEILInterfaceTable *interfaces;
		VEILMappingTable *map;
		VEILHostTable *hosts;
		bool printDebugMessages ;
		uint8_t forwarding_type;
};

CLICK_ENDDECLS
#endif












