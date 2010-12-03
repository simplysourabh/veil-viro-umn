#include <click/config.h>
#include <click/confparse.hh>
#include "hosttable.hh"

CLICK_DECLS

VEILHostTable::VEILHostTable () {}

VEILHostTable::~VEILHostTable () {}

//str is of the form: VID MAC IP
//TODO: SJ:
/*
We need to add the timer with each entry here.
When a new packet comes in, we will first check if it has a corresponding VID or not:
If yes: Then reschedule he timer after XX number of seconds.
If no: Add a new entry, and initialize the timer with XX number of seconds.
*/
int 
VEILHostTable::cp_host(String s, ErrorHandler* errh)
{
	VID hvid;
	EtherAddress hmac;
	IPAddress hip;

	String hvid_str = cp_shift_spacevec(s);
	if(!cp_vid(hvid_str, &hvid))
		return errh->error("[^^ HOST TABLE ^^][Error] host VID is not in expected format");
	String hmac_str = cp_shift_spacevec(s);
	if(!cp_ethernet_address(hmac_str, &hmac))	
		return errh->error("[^^ HOST TABLE ^^][Error] host MAC is not in expected format");	
	updateEntry(&hvid, &hmac);
	String hip_str = cp_shift_spacevec(s);
	if(!cp_ip_address(hip_str, &hip))
		return errh->error("[^^ HOST TABLE ^^][Error] host IP is not in expected format");	
	updateIPEntry(&hip, &hvid);
	return 0;
}

int
VEILHostTable::configure(Vector<String> &conf, ErrorHandler *errh)
{
	veil_chatter_new(true,class_name(),"[FixME] Its mandagory to have 'PRINTDEBUG value' (here value = true/false) at the end of the configuration string!");
	int res = 0;
	int i = 0;
	for ( i = 0; i < conf.size()-1; i++) {
		res = cp_host(conf[i], errh);
	}
	
	cp_shift_spacevec(conf[i]);
	String printflag = cp_shift_spacevec(conf[i]);
	if(!cp_bool(printflag, &printDebugMessages))
		return errh->error("[^^ HOST TABLE ^^][Error] PRINTDEBUG FLAG should be either true or false");	
	return res;
}

void 
VEILHostTable::expire(Timer *t, void *data){
	TimerData *td = (TimerData *) data;
	//remove all the mappings one by one.
	EtherAddress mac;
	HostEntry hentry;
	IPAddress ip;
	VID vid,vid2;
	if(td->host2interfacetable.get_pointer(td->ip)){
		hentry = td->host2interfacetable[td->ip]
	}else{
		//error
	}
	td->host2interfacetable.erase(td->ip);

	if(td->ip2vidtable.get_pointer(td->ip)){
		vid = td->ip2vidtable[td->ip];
	}else{
		//error
	}
	td->ip2vidtable.erase(td->ip);

	if(td->vid2ethtable.get_pointer(vid)){
		mac = td->vid2ethtable[vid];
	}else{
		//error
	}
	td->vid2ethtable.erase(vid);

	if(td->eth2vidtable.get_pointer(mac)){
		vid2 = td->eth2vidtable[mac];
	}else{
		//error
	}
	td->eth2vidtable.erase(mac);

	if (vid != vid2){
		//error
	}

	

	delete(t);
	delete(td);
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
VEILHostTable::updateEntry (
	VID *vid,
	EtherAddress *mac,
	IPAddress *ip)
{
	updateEntry(vid, mac);
	updateIPEntry(ip, vid);
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
	case table:
		sa << "\n----------------- Host Table START-----------------\n" << '\n';
		sa << "Host IP   " << "\t" << "Host MAC        " << '\t' << "Host VID" << '\n';
		for(iiter = iphosts.begin(); iiter; ++iiter){
			IPAddress ipa = iiter.key();
			VID v = static_cast<VID>(iiter.value());
			EtherAddress ea;
			if(!ht->lookupVID(&v, &ea)){
				veil_chatter_new(true, ht->class_name(), "[Error] No IP to VID mapping for : %s", ipa.s().c_str());
			}				
			sa << ipa << '\t' << ea << '\t' << v.vid_string()<<'\n';
		}
		sa<< "----------------- Host Table END -----------------\n\n";
		return sa.take_string();
	
	default: return " ";
	}		  
}

void
VEILHostTable::add_handlers()
{
	add_read_handler("host_table", read_handler, h_table);
	add_read_handler("ip_table", read_handler, i_table);
	add_read_handler("table", read_handler, table);
}



CLICK_ENDDECLS

EXPORT_ELEMENT(VEILHostTable)
