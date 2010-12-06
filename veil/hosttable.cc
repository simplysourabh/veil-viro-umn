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
	int interfaceid;
	veil_chatter_new(true, class_name(), "cp_host | Syntax is: <host vid> <host mac> <host ip> <myinterfaceid>");
	/* syntax: vid mac ip interfaceid */
	String hvid_str = cp_shift_spacevec(s);
	if(!cp_vid(hvid_str, &hvid))
		return errh->error("[^^ HOST TABLE ^^][Error] host VID is not in expected format");
	String hmac_str = cp_shift_spacevec(s);
	if(!cp_ethernet_address(hmac_str, &hmac))	
		return errh->error("[^^ HOST TABLE ^^][Error] host MAC is not in expected format");	
	String hip_str = cp_shift_spacevec(s);
	if(!cp_ip_address(hip_str, &hip))
		return errh->error("[^^ HOST TABLE ^^][Error] host IP is not in expected format");	
	String hip_int = cp_shift_spacevec(s);
	if(!cp_integer(hip_int, &interfaceid))
		return errh->error("[^^ HOST TABLE ^^][Error] host InterfaceID is not in expected format");	
	updateEntry(hvid, hmac, hip, interfaceid);
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
	/*
		Everytime the timer fires for a given entry, then remove the entries if 
		the hostvid is same as the current vid for the host.	
	*/
	TimerData *td = (TimerData *) data;
	//remove all the mappings one by one.
	EtherAddress mac;
	HostEntry hentry;
	IPAddress ip;
	VID vid,vid2;
	if(td->host2interfacetable->get_pointer(td->ip)){
		hentry = td->host2interfacetable->get(td->ip);
	}else{
		//error
		veil_chatter_new(true, "VEILHostTable","expire | Error: ip %s is not present in the host2interfacetable.", td->ip.s().c_str());
	}

	if(td->ip2vidtable->get_pointer(td->ip)){
		vid = td->ip2vidtable->get(td->ip);
		// If its the same.
		if(vid == td->hostvid){
			td->ip2vidtable->erase(td->ip);
			td->host2interfacetable->erase(td->ip);
		}
		// else its been assigned a new vid, so don't delete and exit.
		else{
			veil_chatter_new(true,"VEILHostTable", "expire | ip %s has a new vid %s the old vid was %s.", td->ip.s().c_str(), vid.vid_string().c_str(), td->hostvid.vid_string().c_str());
			delete(t); delete(td); return;
		}
	}else{
		//error
		veil_chatter_new(true, "VEILHostTable","expire | Error: ip %s is not present in the ip2vidtable.", td->ip.s().c_str());
		// return here, since we wont be able to proceed without vid.
		delete(t); delete(td); return;
	}

	if(td->vid2ethtable->get_pointer(vid)){		
		mac = td->vid2ethtable->get(vid);
		td->vid2ethtable->erase(vid);
	}else{
		//error
		veil_chatter_new(true, "VEILHostTable","expire | Error: VID %s is not present in the vid2ethtable.", vid.vid_string().c_str());
		// return here. we won't be able to proceed without mac.
		delete(t); delete(td); return;
	}

	if(td->eth2vidtable->get_pointer(mac)){
		vid2 = td->eth2vidtable->get(mac);
		td->eth2vidtable->erase(mac);
	}else{
		//error
		veil_chatter_new(true, "VEILHostTable","expire | Error: MAC %s is not present in the vid2ethtable.", vid.vid_string().c_str());
		delete(t); delete(td); return;
	}

	if (vid != vid2){
		//error
		veil_chatter_new(true, "VEILHostTable","expire | Error: vid %s != vid2 %s is not present in the vid2ethtable.", vid.vid_string().c_str(), vid2.vid_string().c_str());
	}
	veil_chatter_new(true,"VEILHostTable", "expire | Mapping %s -> %s -> %s expired.", td->ip.s().c_str(),vid.vid_string().c_str(),mac.s().c_str());
	delete(t); delete(td);
}

void 
VEILHostTable::updateEntry (VID vid, EtherAddress mac, IPAddress ip, int interfaceid){
	/*
		LOGIC
		First check if ip to interface entry is there:
			if its there then check if its same or not.
				if its different then remove the old entry/entries and create a new one.
				else (same) then update the expiry by T seconds, and make sure the other entries are also correct.
			else (entry is not present) then enter it and create the expiry data structure.
	*/
	
	HostEntry he;
	if (inthosts.get_pointer(ip) != NULL){
		he = inthosts[ip];
		if(he.interfaceid != interfaceid){
			// new interface.
			veil_chatter_new(printDebugMessages, class_name(), "For ip %s interface has changed from %d to %d.", ip.s().c_str(), he.interfaceid, interfaceid);
		}
		he.expiry->unschedule();
		he.expiry->schedule_after_msec(HOST_ENTRY_EXPIRY);
	}else{
		TimerData *tdata = new TimerData();
		tdata->vid2ethtable = &hosts;
		tdata->eth2vidtable = &rhosts;
		tdata->ip2vidtable = &iphosts;
		tdata->host2interfacetable = &inthosts;
		tdata->hostvid = vid;
		tdata->ip = ip;
		
		Timer *expiry = new Timer(&VEILHostTable::expire, tdata);
		expiry->initialize(this);
		expiry->schedule_after_msec(HOST_ENTRY_EXPIRY);
		he.expiry = expiry;
	}
	// update the entry
	// TODO: make sure that there are no stale entries here.
	he.interfaceid = interfaceid;
	inthosts[ip] = he;
	hosts[vid] = mac;
	rhosts[mac] = vid;
	iphosts[ip] = vid;
	
	veil_chatter_new(printDebugMessages, class_name(),"Updated mapping: ip %s vid %s mac %s interface %d.", ip.s().c_str(), vid.vid_string().c_str(), mac.s().c_str(), interfaceid);
}


bool 
VEILHostTable::generate_host_vid(IPAddress hip, EtherAddress hmac, int myinterfaceid, VID switchvid, VID *hvid){
	/*
		this function generates a host-vid for a given host-ip, host-mac, and the my interface id
		connected to the host and the corresponding switch vid.
	 
		The key idea here is to generate a random number and see if that number is already there in the
		the current list, and perform a linear search from that point to find an empty slot.
		if no empty slot is found then we have no more available vids to be assigned. However, it is less
		likely to happen real implementation since there are 2^16 - ß1 such vids at a given switch.
	*/
	bool isNew = false;
	veil_chatter_new(printDebugMessages,class_name(),"generate_host_vid | Host ip %s Mac %s Myinterface %d SwitchVID %s", hip.s().c_str(), hmac.s().c_str(), myinterfaceid, switchvid.switchVIDString().c_str());
	// first see if the ip has currently assigned vid.
	if (inthosts.get_pointer(hip) != NULL) {
		HostEntry he = inthosts[hip];
		veil_chatter_new(printDebugMessages, class_name(), "generate_host_vid | host ip %s is already present at interface %d", hip.s().c_str(), he.interfaceid);
		// check if the switch vid is still the same or not.
		if (he.interfaceid == myinterfaceid) {
			// update the timers.
			VID vid1 = iphosts[hip];
			memcpy(hvid, &vid1,6);
			he.expiry->unschedule();
			he.expiry->schedule_after_msec(HOST_ENTRY_EXPIRY);
			inthosts[hip] = he;
			hosts[vid1] = hmac;
			rhosts[hmac] = vid1;
			iphosts[hip] = vid1;
			veil_chatter_new(printDebugMessages, class_name(), "generate_host_vid | refreshed the entry for ip %s by %dms", hip.s().c_str(), HOST_ENTRY_EXPIRY);
			return false;
		}else {
			// remove the old mapping.
			veil_chatter_new(printDebugMessages, class_name(), "generate_host_vid | host ip %s has different INTERFACE %d old interface was %d", hip.s().c_str(), myinterfaceid, he.interfaceid);
			he.expiry->unschedule();
			delete(he.expiry);
			hosts.erase(iphosts[hip]);
			rhosts.erase(hmac);
			iphosts.erase(hip);
			inthosts.erase(hip);
		}
	}
	
	// since we reached here, it means that 
	// we will be creating a new entry for hip.
	isNew = true;
	
	// pick a random number to start the search.
	uint16_t startrand;
	startrand = (uint16_t) rand();
	uint16_t currrand = startrand;
	VID v1;
	unsigned char *vptr = v1.data();
	memcpy(&v1, &switchvid, 4);
	memcpy(vptr+4,&startrand,2);
	
	// see if this random id exists or not.
	veil_chatter_new(printDebugMessages, class_name(), "generate_host_vid | Checking for availability of %s for %s", v1.vid_string().c_str(), hip.s().c_str());
	while(hosts.get_pointer(v1) != NULL){
		// this vid already exists. try a new vid.
		currrand++;
		if (currrand == startrand) {
			// we ran out of the new vids.
			veil_chatter_new(true, class_name(), "generate_host_vid | ERROR: No more unused VIDs for the host ip %s", hip.s().c_str());
			return false;
		}
		memcpy(vptr+4, &currrand, 2);
	}
	
	// assign this vid to hip.
	veil_chatter_new(printDebugMessages, class_name(), "generate_host_vid | Finally assigning VID %s to host ip %s", v1.vid_string().c_str(), hip.s().c_str());
	updateEntry(v1, hmac, hip, myinterfaceid);
	return isNew;
}

bool 
VEILHostTable::refreshIPintEntry(IPAddress ip, int interfaceid){
	bool refreshed = false;
	if (inthosts.get_pointer(ip)) {
		HostEntry he = inthosts[ip];
		if (he.interfaceid == interfaceid) {
			he.expiry->unschedule();
			he.expiry->schedule_after_msec(HOST_ENTRY_EXPIRY);
			return true;
		}
		veil_chatter_new(printDebugMessages, class_name(), "refreshIPintEntry | For ip %s interface has changed from %d to %d.", ip.s().c_str(), he.interfaceid, interfaceid);
	}
	return false;
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

void
VEILHostTable::deleteIP(IPAddress ip){
	veil_chatter_new(printDebugMessages, class_name(),"Deleting the mappings for the ip %s", ip.s().c_str());
	HostEntry he;
	if(inthosts.get_pointer(ip) != NULL){
		he.expiry->unschedule();
		delete(he.expiry);
		inthosts.erase(ip);
		VID vid; EtherAddress mac; 
		if (iphosts.get_pointer(ip) == NULL){
			//error
			veil_chatter_new(printDebugMessages, class_name(),"deleteIP | Mappings for the ip %s was not found in the iphosts!", ip.s().c_str());
			return;
		}
		vid = iphosts[ip];
		iphosts.erase(ip);

		if (hosts.get_pointer(vid) == NULL){
			//error
			veil_chatter_new(printDebugMessages, class_name(),"deleteIP | Mappings for the ip %s with vid %s was not found in the hosts!", ip.s().c_str(), vid.vid_string().c_str());
			return;
		}
		mac = hosts[vid];
		hosts.erase(vid);

		if(rhosts.get_pointer(mac) == NULL){
			//error
			veil_chatter_new(printDebugMessages, class_name(),"deleteIP | Mappings for the ip %s with mac %s was not found in the rhosts!", ip.s().c_str(), mac.s().c_str());
			return;
		}

		rhosts.erase(mac);

	}else{
		veil_chatter_new(printDebugMessages, class_name(),"deleteIP | Mappings for the ip %s was not found!", ip.s().c_str());
	}
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
		sa << "Host IP   " << "\t" << "Host MAC        " << '\t' << "Host VID\tMy Interface\tExpiry\n";
		for(iiter = iphosts.begin(); iiter; ++iiter){
			IPAddress ipa = iiter.key();
			VID v = static_cast<VID>(iiter.value());
			EtherAddress ea;
			int myint = -1;
			int secremaining = -1;
			if(!ht->lookupVID(&v, &ea)){
				veil_chatter_new(true, ht->class_name(), "[Error] No IP to VID mapping for : %s", ipa.s().c_str());
			}
			if(ht->inthosts.get_pointer(ipa) != NULL){
				HostEntry he = ht->inthosts[ipa];
				myint = he.interfaceid;
				secremaining = he.expiry->expiry().sec() - time(NULL);
			}else{
				veil_chatter_new(true, ht->class_name(), "read_handler | No hostentry found for ip %s!", ipa.s().c_str());
			}
			sa << ipa << '\t' << ea << '\t' << v.vid_string()<<'\t'<<myint<<'\t'<<secremaining<<" Sec\n";
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
