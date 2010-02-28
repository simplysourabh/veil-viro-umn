#include <click/config.h>
#include <click/confparse.hh>
#include "click_veil.hh"
#include "mappingtable.hh"

CLICK_DECLS

VEILMappingTable::VEILMappingTable () {}

VEILMappingTable::~VEILMappingTable () {}

void
VEILMappingTable::updateEntry (IPAddress *ip, VID *ipvid, VID *myvid)
{
	TimerData *tdata = new TimerData();
	tdata->ip = ip;
	tdata->ipmap = &ipmap;
	
	MappingTableEntry entry;

	memcpy(&entry.ipVid, ipvid->data(), 6);
	memcpy(&entry.myVid, myvid->data(), 6);
	Timer *expiry = new Timer(&VEILMappingTable::expire, tdata);
	expiry->initialize(this);
	expiry->schedule_after_msec(VEIL_TBL_ENTRY_EXPIRY);
	entry.expiry  = expiry;
        ipmap.set(*ip, entry);
}

bool
VEILMappingTable::lookupIP (IPAddress *ip, VID *ipvid, VID* myvid)
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
	
	for(iter = ipmap.begin(); iter; ++iter){
		IPAddress ipa = iter.key();
		MappingTableEntry mte = iter.value();
		String ipvid = static_cast<VID>(mte.ipVid).vid_string();		
		String myvid = static_cast<VID>(mte.myVid).vid_string();				
		sa << ipa << ' ' << ipvid << ' ' << myvid << '\n';
	}	
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
	IPAddress* ip = (IPAddress *) td->ip;
	td->ipmap->erase(*ip);
	//click_chatter("%d entries in neighbor table", td->neighbors->size());
	delete(td); 
}


CLICK_ENDDECLS

EXPORT_ELEMENT(VEILMappingTable)
