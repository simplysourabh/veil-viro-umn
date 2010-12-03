#ifndef CLICK_VEILHOSTTABLE_HH
#define CLICK_VEILHOSTTABLE_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/error.hh>
#include <click/hashtable.hh>
#include "click_veil.hh"

CLICK_DECLS

//we don't need to keep track of which interface the host is connected
//to here because interface VID can be derived from host VID
class VEILHostTable : public Element {
	public:
		
		struct HostEntry{
			int interfaceid;
			Timer *expiry;
		};

		typedef HashTable<VID, EtherAddress> HostTable;
		typedef HashTable<EtherAddress, VID> ReverseHostTable;
		typedef HashTable<IPAddress, VID> HostIPTable;
		typedef HashTable<IPAddress, HostEntry> HostInterfaceTable;
		VEILHostTable();
		~VEILHostTable();

		const char* class_name() const { return "VEILHostTable"; }
		const char* port_count() const { return PORTS_0_0; }
		
		int cp_host(String, ErrorHandler*);

		int configure(Vector<String>&, ErrorHandler*);
		void updateEntry(VID*, EtherAddress*);
		void updateIPEntry(IPAddress*, VID*);
		bool lookupVID(VID*, EtherAddress*);
		bool lookupMAC(EtherAddress*, VID*);
		bool lookupIP(IPAddress*, VID*);
		void updateEntry (VID *vid, EtherAddress *mac, IPAddress *ip);
		
		static void expire(Timer*, void*);
		struct TimerData {
			HostTable *vid2ethtable;
			ReverseHostTable *eth2vidtable;
			HostIPTable *ip2vidtable;
			HostInterfaceTable *host2interfacetable;
			VID hostvid;
			IPAddress ip;
		};

		inline const HostIPTable* get_host_iptable_handle(){
			return &iphosts;
		}

		static String read_handler(Element*, void*);
		void add_handlers();

	private:
		HostTable hosts;
		ReverseHostTable rhosts;
		HostIPTable iphosts;
		bool printDebugMessages ;
		enum { h_table, i_table, table };
};

CLICK_ENDDECLS
#endif
