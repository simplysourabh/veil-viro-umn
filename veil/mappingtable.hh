//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
#ifndef CLICK_VEILIPMAPPINGTABLE_HH
#define CLICK_VEILIPMAPPINGTABLE_HH

#include <click/element.hh>
#include <click/timer.hh>
#include <click/error.hh>
#include <click/hashtable.hh>
#include "click_veil.hh"

CLICK_DECLS

//typedef HashTable<VID, IPAddress> ReverseIPMapTable; //do i need this?

class VEILMappingTable : public Element {
	public:
		VEILMappingTable();
		~VEILMappingTable();

		const char* class_name() const { return "VEILMappingTable"; }
		const char* port_count() const { return PORTS_0_0; }
		const char* processing() const { return PUSH; }

		int cp_mapping(String, ErrorHandler*);

		int configure(Vector<String>&, ErrorHandler*);
		void updateEntry(IPAddress ip, VID ipvid, VID myvid, EtherAddress mac);
		bool lookupIP(IPAddress *ip, VID *ipvid, VID* myvid, EtherAddress *mac=NULL);
		static void expire(Timer*, void*);
		static String read_handler(Element*, void*);
		void add_handlers();

	private:
		struct MappingTableEntry {
			VID ipVid;
			VID myVid;
			EtherAddress ipmac;
			Timer *expiry;
		};
		
		typedef HashTable<IPAddress, MappingTableEntry> IPMapTable;
		IPMapTable ipmap;

		struct TimerData {
			IPMapTable *ipmap;
			IPAddress ip;
		};
		bool printDebugMessages ;

};

CLICK_ENDDECLS
#endif
