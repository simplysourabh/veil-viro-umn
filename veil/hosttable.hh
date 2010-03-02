#ifndef CLICK_VEILHOSTTABLE_HH
#define CLICK_VEILHOSTTABLE_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/hashtable.hh>
#include "click_veil.hh"

CLICK_DECLS

/*
struct hostTableEntry {
	EtherAddress mac;
	Timer *publish_interval;	
	//bool valid;
};
*/

//we don't need to keep track of which interface the host is connected
//to here because interface VID can be derived from host VID
class VEILHostTable : public Element {
	public:

		typedef HashTable<VID, EtherAddress> HostTable;
		typedef HashTable<EtherAddress, VID> ReverseHostTable;
		typedef HashTable<IPAddress, VID> HostIPTable;

		VEILHostTable();
		~VEILHostTable();

		const char* class_name() const { return "VEILHostTable"; }
		//const char* port_count() const { return PORTS_0_1; }
		const char* port_count() const { return PORTS_0_0; }
		//const char* processing() const { return PUSH; }

		//int configure(Vector<String>&, ErrorHandler*);
		void updateEntry(VID*, EtherAddress*);//, uint32_t);
		void updateIPEntry(IPAddress*, VID*);
		bool lookupVID(VID*, EtherAddress*);
		bool lookupMAC(EtherAddress*, VID*);
		bool lookupIP(IPAddress*, VID*);

		inline const HostIPTable* get_host_iptable_handle(){
			return &iphosts;
		}

		static String read_handler(Element*, void*);
		void add_handlers();

	private:
		HostTable hosts;
		ReverseHostTable rhosts;
		HostIPTable iphosts;

		//TODO:if we decide to expire extries change all hashtable value types
		/*
		struct TimerData {
			VEILHostTable *hosts;
			VID *vid;
		};
		static void handleExpiry(Timer*, void*);
	 	void expire(VID&, TimerData*);
		*/
	
		enum { h_table, i_table };
};

CLICK_ENDDECLS
#endif
