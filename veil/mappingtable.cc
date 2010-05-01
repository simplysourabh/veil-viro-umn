#include <click/config.h>
#include <click/confparse.hh>
#include "click_veil.hh"
#include "mappingtable.hh"

CLICK_DECLS

VEILMappingTable::VEILMappingTable () {}

VEILMappingTable::~VEILMappingTable () {}


//String is of the format: some-host-(X)-VID X's-IP my-interfacevid-thru-which-mapping-was-learned
int
VEILMappingTable::cp_mapping(String s, ErrorHandler *errh)
{
	VID vid, myvid;
	IPAddress ip;
	EtherAddress hmac;
	String vid_str, ip_str,mac_str, myvid_str;

	vid_str = cp_shift_spacevec(s);
	if(!cp_vid(vid_str, &vid))
		return errh->error("VID is not in expected format");

	ip_str = cp_shift_spacevec(s);
	if(!cp_ip_address(ip_str, &ip))
		return errh->error("IP is not in expected format");	

	mac_str = cp_shift_spacevec(s);
	if(!cp_ethernet_address(mac_str, &hmac))
		return errh->error("MAC is not in expected format");

	myvid_str = cp_shift_spacevec(s);
	if(!cp_vid(myvid_str, &myvid))
		return errh->error("interface VID is not in expected format");
	updateEntry(&ip, &vid, &myvid,&hmac);
	return 0;
}

int
VEILMappingTable::configure(Vector<String> &conf, ErrorHandler *errh)
{
	click_chatter("[-:MAPPINGTABLE:-][FixME] Its mandagory to have 'PRINTDEBUG value' (here value = true/false) at the end of the configuration string!\n");	
	int res = 0;
	int i = 0;
	for (i = 0; i < conf.size()-1; i++) {
		res = cp_mapping(conf[i], errh);
	}
	cp_shift_spacevec(conf[i]);
	String printflag = cp_shift_spacevec(conf[i]);
	if(!cp_bool(printflag, &printDebugMessages))
		return errh->error("[-:MAPPINGTABLE:-][Error] PRINTDEBUG FLAG should be either true or false");	
	return res;
}

void
VEILMappingTable::updateEntry (IPAddress *ip, VID *ipvid, VID *myvid, EtherAddress *ipmac)
{
	MappingTableEntry entry;
	// first see if mapping is already there or not?
	if(ipmap.find(*ip)== ipmap.end()){
		entry = ipmap.get(*ip);
		if((*ipvid) != entry.ipVid || (*myvid) != entry.myVid || (*ipmac) != entry.ipmac){
			veil_chatter(printDebugMessages,"[-:MAPPINGTABLE:-] New mapping for IP: %s to VID: %s, old vid: %s \n", ip->s().c_str(), ipvid->vid_string().c_str(), entry.ipVid.vid_string().c_str());
			// TODO: Send a VEIL_MAP_UPDATE to original switch, so that it can update its host table.
		}else{
			// refresh the timer in this case.
			MappingTableEntry *ent_pointer = ipmap.get_pointer(*ip);
			ent_pointer->expiry->unschedule();
			ent_pointer->expiry->schedule_after_msec(VEIL_TBL_ENTRY_EXPIRY);
		}
		return;
	}

	TimerData *tdata = new TimerData();
	tdata->ipmap = &ipmap;
	memcpy(&tdata->ip, &ip, 4);
	


	memcpy(&entry.ipVid, ipvid->data(), 6);
	memcpy(&entry.myVid, myvid->data(), 6);
	memcpy(&entry.ipmac, ipmac->data(), 6);
	Timer *expiry = new Timer(&VEILMappingTable::expire, tdata);
	expiry->initialize(this);
	expiry->schedule_after_msec(VEIL_TBL_ENTRY_EXPIRY);
	entry.expiry  = expiry;
    ipmap.set(*ip, entry);
}

bool
VEILMappingTable::lookupIP(IPAddress *ip, VID *ipvid, VID* myvid, EtherAddress *ipmac)
{
	bool found = false;
	if (ipmap.find(*ip) == ipmap.end()) {
		found = false;
	} else {
		MappingTableEntry mte = ipmap.get(*ip);		
		VID ipVid = mte.ipVid;
		VID myVid = mte.myVid;
		memcpy(ipvid, &ipVid, 6);
		memcpy(myvid, &myVid, 6);
		if(ipmac!= NULL){memcpy(ipmac, &mte.ipmac, 6);}
		found = true;
	}
	return found;
}

String
VEILMappingTable::read_handler(Element *e, void *thunk)
{
	StringAccum sa;
	VEILMappingTable *mt = (VEILMappingTable *) e;
	IPMapTable::iterator iter;
	IPMapTable ipmap = mt->ipmap;

	sa << "\n----------------- Mapping START-----------------"<< '\n';
	sa << "HOST IP " << " \t" << "HOST VID" <<"\t\t"<<"Host MAC"<< "\t" << "Access Switch(Interface) VID\n";
	
	for(iter = ipmap.begin(); iter; ++iter){
		IPAddress ipa = iter.key();
		MappingTableEntry mte = iter.value();
		String ipvid = static_cast<VID>(mte.ipVid).vid_string();		
		String myvid = static_cast<VID>(mte.myVid).switchVIDString();
		//String hmac = mte.ipmac.s();
		sa << ipa << '\t' << ipvid << '\t' <<mte.ipmac << '\t'<< myvid << '\n';
	}
	sa<< "----------------- Mapping Table END -----------------\n\n";	
	return sa.take_string();	  
}

void
VEILMappingTable::add_handlers()
{
	add_read_handler("table", read_handler, (void *)0);
}

void
VEILMappingTable::expire(Timer *t, void *data) 	
{
	TimerData *td = (TimerData *) data;
	td->ipmap->erase(td->ip);
	t->clear();
	delete(t);
	//click_chatter("%d entries in neighbor table", td->neighbors->size());
	delete(td); 
}


CLICK_ENDDECLS

EXPORT_ELEMENT(VEILMappingTable)
