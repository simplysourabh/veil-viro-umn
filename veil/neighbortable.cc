#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/straccum.hh>
#include "neighbortable.hh"
#include <time.h>

//TODO: make reads and writes atomic

CLICK_DECLS

VEILNeighborTable::VEILNeighborTable () {}

VEILNeighborTable::~VEILNeighborTable () {}

//String is of the form: neighborVID interfaceVID 
int
VEILNeighborTable::cp_neighbor(String s, ErrorHandler* errh){
	VID nvid, myvid;

	String nvid_str = cp_shift_spacevec(s);
	if(!cp_vid(nvid_str, &nvid))
		return errh->error("[::NeighborTable::] neighbor VID is not in expected format");
	String myvid_str = cp_shift_spacevec(s);
	if(!cp_vid(myvid_str, &myvid))
		return errh->error("[::NeighborTable::] interface VID is not in expected format");
	updateEntry(&nvid, &myvid);
	return 0;
}

int
VEILNeighborTable::configure(Vector<String> &conf, ErrorHandler *errh)
{
click_chatter("[::NeighborTable::][FixME] Its mandagory to have 'PRINTDEBUG value' (here value = true/false) at the end of the configuration string!\n");
	int res = 0;
	int i = 0;
	for (i = 0; i < conf.size()-1; i++) {
		res = cp_neighbor(conf[i], errh);
	}
	veil_chatter(printDebugMessages,"[::NeighborTable::] Configured the neighbor table!\n");
	
	cp_shift_spacevec(conf[i]);
	String printflag = cp_shift_spacevec(conf[i]);
	if(!cp_bool(printflag, &printDebugMessages))
		return errh->error("[::NeighborTable::][Error] PRINTDEBUG FLAG should be either true or false");	
	return res;
}

void
VEILNeighborTable::updateEntry (
	VID *nvid, VID *myvid)
{
	TimerData *tdata = new TimerData();
	VID * nvid1 = new VID(nvid->data());
	tdata->vid = nvid1;
	tdata->neighbors = &neighbors;
	
	NeighborTableEntry entry;

	memcpy(&entry.myVid, myvid->data(), 6);
	Timer *expiry = new Timer(&VEILNeighborTable::expire, tdata);
	expiry->initialize(this);
	expiry->schedule_after_msec(VEIL_TBL_ENTRY_EXPIRY);
	entry.expiry  = expiry;
	// First see if the key is already present?
	NeighborTableEntry * oldEntry;
	oldEntry = neighbors.get_pointer(*nvid);
	if (oldEntry != NULL){
		oldEntry->expiry->unschedule();
		delete(oldEntry->expiry);
		veil_chatter(printDebugMessages,"[::NeighborTable::] Neighbor VID: |%s| \n",nvid->switchVIDString().c_str());
	}else{
		veil_chatter(printDebugMessages,"[::NeighborTable::][New neighbor] Neighbor VID: |%s| MyVID: |%s|\n",nvid->switchVIDString().c_str(),myvid->switchVIDString().c_str());
	}
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
	// Temporary NOT deleting  entries 
	//  Just for debugging
	veil_chatter(true,"[::NeighborTable::] [Timer Expired] VID: |%s|\n",nvid->switchVIDString().c_str());
	//veil_chatter(printDebugMessages,"[BEFORE EXPIRE] %d entries in neighbor table", td->neighbors->size());
	if (td->neighbors->get_pointer(*nvid) == NULL){
		veil_chatter(true,"[::NeighborTable::] [ERROR: Entry NOT FOUND] Key: |%s| \n",nvid->switchVIDString().c_str());
	}
	td->neighbors->erase(*nvid);
	//veil_chatter(printDebugMessages,"[AFTER EXPIRE] %d entries in neighbor table", td->neighbors->size());
	//click_chatter (read_handler(this, NULL).c_str());
	delete(td); 
	delete(t);
}

String
VEILNeighborTable::read_handler(Element *e, void *thunk)
{
	StringAccum sa;
	VEILNeighborTable *nt = (VEILNeighborTable *) e;
	NeighborTable::iterator iter;
	NeighborTable neighbors = nt->neighbors;

	sa << "\n-----------------Neighbor Table START-----------------\n"<<"[::NeighborTable::]" << '\n';
	sa << "Neighbor VID" << "\t" << "Myinterface VID" << '\t' << "TTL" << '\n';
	
	for(iter = neighbors.begin(); iter; ++iter){
		String vid = static_cast<VID>(iter.key()).switchVIDString();		
		NeighborTableEntry nte = iter.value();
		String myvid = static_cast<VID>(nte.myVid).switchVIDString();		
		Timer *t = nte.expiry;
		sa << vid << '\t' << myvid << "\t"<<t->expiry().sec() - time(NULL) << " sec\n";
	}
	sa<< "----------------- Neighbor Table END -----------------\n\n";
	return sa.take_string();	  
}

void
VEILNeighborTable::add_handlers()
{
	add_read_handler("table", read_handler, (void *)0);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILNeighborTable)
