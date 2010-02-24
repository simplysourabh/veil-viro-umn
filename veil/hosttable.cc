#include <click/config.h>
#include <click/confparse.hh>
#include "hosttable.hh"

CLICK_DECLS

VEILHostTable::VEILHostTable () {}

VEILHostTable::~VEILHostTable () {}

void
VEILHostTable::updateEntry (
	VID *vid,
	EtherAddress *mac)
//,
//	uint32_t interval)
{
	/*
        struct hostTableEntry entry;
        //todo delete this memory later
        TimerData *tdata = new TimerData();
	tdata->vid = vid;
        tdata->hosts = this;

        entry.mac = *mac;
        entry.publish_interval = new Timer(&VEILHostTable::handleExpiry, tdata);
        entry.publish_interval->initialize(this);
        entry.publish_interval->schedule_after_msec(interval);
        hosts.set(*vid, entry);
	*/
	hosts.set(*vid, *mac);
	rhosts.set(*mac, *vid);
}

void
VEILHostTable::updateIPEntry (
	IPAddress *ip,
	VID *vid)
{
	iphosts.set(*ip, *vid);
}

bool
VEILHostTable::lookupVID (
	VID *vid,
	EtherAddress *mac)
{
	bool found = false;	
	if (hosts.find(*vid) == hosts.end()) {
		found = false;
		mac = NULL;
	} else {
		/*
		struct hostTableEntry hte;
		hte = hosts.get(*vid);	
		//todo error check mac ptr
		memcpy(mac, hte.mac.data(), 6); 
		*/
		EtherAddress e = hosts.get(*vid);
		memcpy(mac, e.data(), 6); 
		found = true;
	}
	return(found);
}

bool
VEILHostTable::lookupMAC (
	EtherAddress *mac,
	VID *vid)
{
	bool found = false;
	if (rhosts.find(*mac) == rhosts.end()) {
		found = false;
		vid = NULL;
	} else {
		VID myvid = rhosts.get(*mac);
		memcpy(vid, myvid.data(), 6);
		found = true;
	}

	return(found);
}

bool
VEILHostTable::lookupIP (
	IPAddress *ip,
	VID *vid)
{
	bool found = false;
	if (iphosts.find(*ip) == iphosts.end()) {
		found = false;
		vid = NULL;
	} else {
		VID myvid = iphosts.get(*ip);
		memcpy(vid, myvid.data(), 6);
		found = true;
	}

	return(found);
}

/*
void
VEILHostTable::handleExpiry (
	Timer*,
	void *data)
{
	TimerData *tdata = (TimerData *) data;
	tdata->hosts->expire(*(tdata->vid), tdata);
}

void
VEILHostTable::expire (
	VID &vid,
	TimerData *tdata)
{
	struct hostTableEntry hte;
	hte = hosts.get(vid);
	// Create PUBLISH packet
	// Send it out to the AccessNode
}
*/

String
VEILHostTable::read_handler(Element *e, void *thunk)
{
	StringAccum sa;
	VEILHostTable *ht = (VEILHostTable *) e;

	HostTable::iterator hiter;
	HostTable hosts = ht->hosts;

	HostIPTable::iterator iiter;
	HostIPTable iphosts = ht->iphosts;

	switch (reinterpret_cast<uintptr_t>(thunk)) {
    	case h_table:		
		for(hiter = hosts.begin(); hiter; ++hiter){
			String vid = static_cast<VID>(hiter.key()).vid_string();		
			EtherAddress ea = hiter.value();	
			sa << vid << ' ' << ea << ' ' << '\n';
		}
		return sa.take_string();
	case i_table:	
		for(iiter = iphosts.begin(); iiter; ++iiter){
			IPAddress ipa = iiter.key();
			String vid = static_cast<VID>(iiter.value()).vid_string();				
			sa << vid << ' ' << ipa << ' ' << '\n';
		}
		return sa.take_string();
	default: return " ";
	}		  
}

void
VEILHostTable::add_handlers()
{
	add_read_handler("host_table", read_handler, h_table);
	add_read_handler("ip_table", read_handler, i_table);
}



CLICK_ENDDECLS

EXPORT_ELEMENT(VEILHostTable)
