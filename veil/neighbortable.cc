#include <click/config.h>
#include <click/confparse.hh>
#include<click/straccum.hh>
#include "neighbortable.hh"

//TODO: make reads and writes atomic

CLICK_DECLS

VEILNeighborTable::VEILNeighborTable () {}

VEILNeighborTable::~VEILNeighborTable () {}

void
VEILNeighborTable::updateEntry (
	VID *nvid, String interface)
{
	NeighborTable::iterator iter;
	
	TimerData *tdata = new TimerData();
	tdata->vid = nvid;
	tdata->neighbors = &neighbors;
	
	NeighborTableEntry entry;

	if((iter = neighbors.find(*nvid)) != neighbors.end()){
		//TODO: did this give us the correct entry?
		entry = (NeighborTableEntry) neighbors.get(*nvid);
		entry.expiry->schedule_after_msec(VEIL_TBL_ENTRY_EXPIRY);
	} else {
		entry.interface = interface;
	
		Timer *expiry = new Timer(&VEILNeighborTable::expire, tdata);
		expiry->initialize(this);
		expiry->schedule_after_msec(VEIL_TBL_ENTRY_EXPIRY);
		entry.expiry  = expiry;
		neighbors.set(*nvid, entry);
	}
}

void
VEILNeighborTable::expire(Timer *t, void *data) 	
{
	//TODO: does erase free memory allocated for key and value?	
	TimerData *td = (TimerData *) data;
	VID* nvid = (VID *) td->vid;
	td->neighbors->erase(*nvid);
	click_chatter("%d entries in neighbor table", td->neighbors->size());
	delete(td); 
}

String
VEILNeighborTable::read_handler(Element *e, void *thunk)
{
	StringAccum sa;
	VEILNeighborTable *nt = (VEILNeighborTable *) e;
	NeighborTable::iterator iter;
	NeighborTable neighbors = nt->neighbors;
	
	for(iter = neighbors.begin(); iter; ++iter){
		String vid = static_cast<VID>(iter.key()).vid_string();		
		NeighborTableEntry nte = iter.value();
		Timer *t = nte.expiry;

		sa << vid << ' ' << nte.interface << ' ' << t->expiry() << '\n';
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
