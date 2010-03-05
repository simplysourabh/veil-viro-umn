#include <click/config.h>
#include <click/confparse.hh>
#include <click/straccum.hh>
#include "neighbortable.hh"

//TODO: make reads and writes atomic

CLICK_DECLS

VEILNeighborTable::VEILNeighborTable () {}

VEILNeighborTable::~VEILNeighborTable () {}

void
VEILNeighborTable::updateEntry (
	VID *nvid, VID *myvid)
{
	TimerData *tdata = new TimerData();
	tdata->vid = nvid;
	tdata->neighbors = &neighbors;
	
	NeighborTableEntry entry;

	memcpy(&entry.myVid, myvid->data(), 6);
	Timer *expiry = new Timer(&VEILNeighborTable::expire, tdata);
	expiry->initialize(this);
	expiry->schedule_after_msec(VEIL_TBL_ENTRY_EXPIRY);
	entry.expiry  = expiry;
	neighbors.set(*nvid, entry);
}

bool
VEILNeighborTable::lookupEntry(VID* nvid, VID* myvid)
{
	bool found = false;
	if (neighbors.find(*nvid) == neighbors.end()) {
		found = false;
	} else {
		NeighborTableEntry nte = neighbors.get(*nvid);
		VID myVid = nte.myVid;
		memcpy(myvid, &myVid, 6);
		found = true;
	}
	return found;
}

void
VEILNeighborTable::expire(Timer *t, void *data) 	
{
	TimerData *td = (TimerData *) data;
	VID* nvid = (VID *) td->vid;
	td->neighbors->erase(*nvid);
	//click_chatter("%d entries in neighbor table", td->neighbors->size());
	delete(td); 
}

String
VEILNeighborTable::read_handler(Element *e, void *thunk)
{
	StringAccum sa;
	VEILNeighborTable *nt = (VEILNeighborTable *) e;
	NeighborTable::iterator iter;
	NeighborTable neighbors = nt->neighbors;

	sa << "Neighbor Table" << '\n';
	sa << "neighborvid" << "  " << "myinterfacevid" << ' ' << "expiry" << '\n';
	
	for(iter = neighbors.begin(); iter; ++iter){
		String vid = static_cast<VID>(iter.key()).vid_string();		
		NeighborTableEntry nte = iter.value();
		String myvid = static_cast<VID>(nte.myVid).vid_string();		
		Timer *t = nte.expiry;

		sa << vid << ' ' << myvid << "   " << t->expiry() << '\n';
	}
	
	return sa.take_string();	  
}

void
VEILNeighborTable::add_handlers()
{
	add_read_handler("table", read_handler, (void *)0);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILNeighborTable)
