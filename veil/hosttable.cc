#include <click/config.h>
#include <click/confparse.hh>
#include "hosttable.hh"

CLICK_DECLS

VEILHostTable::VEILHostTable () {}

VEILHostTable::~VEILHostTable () {}

//str is of the form: VID MAC IP
int 
VEILHostTable::cp_host(String s, ErrorHandler* errh)
{
	VID hvid;
	EtherAddress hmac;
	IPAddress hip;

	String hvid_str = cp_shift_spacevec(s);
	if(!cp_vid(hvid_str, &hvid))
		return errh->error("host VID is not in expected format");
	String hmac_str = cp_shift_spacevec(s);
	if(!cp_ethernet_address(hmac_str, &hmac))	
		return errh->error("host MAC is not in expected format");	
	updateEntry(&hvid, &hmac);
	String hip_str = cp_shift_spacevec(s);
	if(!cp_ip_address(hip_str, &hip))
		return errh->error("host IP is not in expected format");	
	updateIPEntry(&hip, &hvid);
	return 0;
}

int
VEILHostTable::configure(Vector<String> &conf, ErrorHandler *errh)
{
	int res = -1;
	for (int i = 0; i < conf.size(); i++) {
		res = cp_host(conf[i], errh);
	}
	
	return res;
}

void
VEILHostTable::updateEntry (
	VID *vid,
	EtherAddress *mac)
{
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

void
VEILHostTable::add_handlers()
{
	add_read_handler("host_table", read_handler, h_table);
	add_read_handler("ip_table", read_handler, i_table);
}



CLICK_ENDDECLS

EXPORT_ELEMENT(VEILHostTable)
