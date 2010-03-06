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
		sa << "\nHostTable - hostvid vs mac\n";
		sa << "hostvid" << "      " << "mac\n";
		for(hiter = hosts.begin(); hiter; ++hiter){
			String vid = static_cast<VID>(hiter.key()).vid_string();		
			EtherAddress ea = hiter.value();	
			sa << vid << ' ' << ea << ' ' << '\n';
		}
		return sa.take_string();
	case i_table:	
		sa << "\nHostTable - hostvid vs ip\n";
		sa << "hostvid" << "      " << "ip\n";
		for(iiter = iphosts.begin(); iiter; ++iiter){
			IPAddress ipa = iiter.key();
			String vid = static_cast<VID>(iiter.value()).vid_string();				
			sa << vid << ' ' << ipa << ' ' << '\n';
		}
		return sa.take_string();
	default: return " ";
	}		  
}

//parameter is of the form: VID MAC 
//this is not the clean way to do things
//we should be adding a parse method in confparse and using that instead
//TODO: move this functionality to confparse
int
VEILHostTable::write_handler(const String &conf_in, Element *e, void *thunk, ErrorHandler *errh)
{
	String s = conf_in;
	VEILHostTable *ht = (VEILHostTable *) e;
	VID hvid;
	EtherAddress hmac;
	IPAddress hip;
	String hvid_str;

	switch (reinterpret_cast<uintptr_t>(thunk)) {
    	case h_table: 
		{
			hvid_str = cp_shift_spacevec(s);
			if(!cp_vid(hvid_str, &hvid))
				return errh->error("host VID is not in expected format");
			String hmac_str = cp_shift_spacevec(s);
			if(!cp_ethernet_address(hmac_str, &hmac))
				return errh->error("host MAC is not in expected format");	
			ht->updateEntry(&hvid, &hmac);
			return 0;
		}
	case i_table:	
		{
			hvid_str = cp_shift_spacevec(s);
			if(!cp_vid(hvid_str, &hvid))
				return errh->error("host VID is not in expected format");
			String hip_str = cp_shift_spacevec(s);
			if(!cp_ip_address(hip_str, &hip))
				return errh->error("host MAC is not in expected format");	
			ht->updateIPEntry(&hip, &hvid);
			return 0;
		}
	default: return -1;
	}		  
	
}

void
VEILHostTable::add_handlers()
{
	add_read_handler("host_table", read_handler, h_table);
	add_read_handler("ip_table", read_handler, i_table);
	add_write_handler("add_host_mac", write_handler, (void*)h_table);
	add_write_handler("add_host_ip", write_handler, (void*)i_table);
}



CLICK_ENDDECLS

EXPORT_ELEMENT(VEILHostTable)
