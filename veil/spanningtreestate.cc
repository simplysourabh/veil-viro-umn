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
	EtherAddress nmac, mymac;

	String nvid_str = cp_shift_spacevec(s);
	if(!cp_vid(nvid_str, &nvid))
		return errh->error("[::NeighborTable::] neighbor VID is not in expected format");

	String nmac_str = cp_shift_spacevec(s);
	if(!cp_ethernet_address(nmac_str, &nmac))
		return errh->error("[::NeighborTable::] neighbor MAC is not in expected format");

	String myvid_str = cp_shift_spacevec(s);
	if(!cp_vid(myvid_str, &myvid))
		return errh->error("[::NeighborTable::] interface VID is not in expected format");

	String mymac_str = cp_shift_spacevec(s);
	if(!cp_ethernet_address(mymac_str, &mymac))
		return errh->error("[::NeighborTable::] interface MAC is not in expected format");

	updateEntry(&nmac, &nvid, &mymac, &myvid);
	return 0;
}

int
VEILNeighborTable::configure(Vector<String> &conf, ErrorHandler *errh)
{
	click_chatter("[::NeighborTable::][FixME] Its mandatory to have 'PRINTDEBUG value' (here value = true/false) at the end of the configuration string!\n");
	int res = 0;
	int i = 0;
	/*for (i = 0; i < conf.size()-1; i++) {
		res = cp_neighbor(conf[i], errh);
	}*/
	veil_chatter(printDebugMessages,"[::NeighborTable::] Configured the neighbor table!\n");

	cp_shift_spacevec(conf[i]);
	String printflag = cp_shift_spacevec(conf[i]);
	if(!cp_bool(printflag, &printDebugMessages))
		return errh->error("[::NeighborTable::][Error] PRINTDEBUG FLAG should be either true or false");	
	return res;
}


bool
VEILNeighborTable::updateEntry(EtherAddress *neigh_mac, VID* neighborvid, EtherAddress* mymac, VID* myvid)
{
	bool retval = false; // returns false if a new entry is created
	// else returns true if the entry already existed.

	TimerData *tdata = new TimerData();
	EtherAddress * neighbormac = new EtherAddress(*neigh_mac);
	tdata->neighbormac = neighbormac;
	tdata->neighbors = &neighbors;
	tdata->ntable = this;

	NeighborTableEntry entry;

	memcpy(&entry.myVid, myvid->data(), 6);
	memcpy(&entry.myMac, mymac, 6);
	memcpy(&entry.neighborVID, neighborvid, 6);

	Timer *expiry = new Timer(&VEILNeighborTable::expire, tdata);
	expiry->initialize(this);
	expiry->schedule_after_msec(VEIL_TBL_ENTRY_EXPIRY);
	entry.expiry  = expiry;
	// First see if the key is already present?
	NeighborTableEntry * oldEntry;
	oldEntry = neighbors.get_pointer(*neighbormac);
	if (oldEntry != NULL){
		oldEntry->expiry->unschedule();
		delete(oldEntry->expiry);
		veil_chatter(printDebugMessages,"[::NeighborTable::] Neighbor VID: |%s| \n",neighborvid->switchVIDString().c_str());
		retval = true;
		/* SJ: No topology writing as of now.
		 * if(writeTopoFlag){
			writeTopo();
		}*/
	}else{
		veil_chatter(printDebugMessages,"[::NeighborTable::][New neighbor] Neighbor VID: |%s| MyVID: |%s|\n",neighborvid->switchVIDString().c_str(),myvid->switchVIDString().c_str());
		retval = false;
	}
	neighbors.set(*neighbormac, entry);

	return retval;
}


bool
VEILNeighborTable::lookupEntry(EtherAddress neighbormac, NeighborTableEntry* entry){
	bool found = false;
	if (neighbors.find(neighbormac) == neighbors.end()) {
		found = false;
	} else {
		entry = neighbors.get_pointer(neighbormac);
		found = true;
	}
	return found;
}

bool
VEILNeighborTable::lookupEntry(VID *nvid, VID* myvid){
	bool found = false;
	NeighborTable::iterator iter;
	for(iter = neighbors.begin(); iter; ++iter){
		NeighborTableEntry nte = iter.value();
		if (nte.neighborVID == *nvid){
			memcpy(myvid, &nte.myVid, 6);
			return true;
		}
	}
	return found;
}


void
VEILNeighborTable::expire(Timer *t, void *data) 	
{

	TimerData *td = (TimerData *) data;
	EtherAddress* nmac = (EtherAddress *) td->neighbormac;
	// Temporary NOT deleting  entries 
	//  Just for debugging
	veil_chatter(true,"[::NeighborTable::] [Timer Expired] MAC: |%s|\n",nmac->s().c_str());
	//veil_chatter(printDebugMessages,"[BEFORE EXPIRE] %d entries in neighbor table", td->neighbors->size());
	if (td->neighbors->get_pointer(*nmac) == NULL){
		veil_chatter(true,"[::NeighborTable::] [ERROR: Entry NOT FOUND] Key: |%s| \n",nmac->s().c_str());
	}
	td->neighbors->erase(*nmac);
	//veil_chatter(printDebugMessages,"[AFTER EXPIRE] %d entries in neighbor table", td->neighbors->size());
	//click_chatter (read_handler(this, NULL).c_str());
	td->ntable->writeTopo();
	delete(td); 
	delete(t);
}

void
VEILNeighborTable::enableUpdatedTopologyWriting(String Filename, bool writeToFile){
	writeTopoFlag = writeToFile;
	topoFile = Filename;
}
bool
VEILNeighborTable::writeTopo(){
	if(writeTopoFlag){
		FILE *topo = fopen(topoFile.c_str(), "w");
		NeighborTable::iterator iter;
		fprintf(topo, "MyInterfaceID MyMac NeighborInterfaceID MyNeighborMac\n");
		for(iter = neighbors.begin(); iter; ++iter){
			String nmac = static_cast<EtherAddress>(iter.key()).s();
			NeighborTableEntry nte = iter.value();

			String nvid = static_cast<VID>(nte.neighborVID).switchVIDString();
			String mymac = static_cast<EtherAddress>(nte.myMac).s();
			String myvid = static_cast<VID>(nte.myVid).switchVIDString();
			fprintf(topo, "%s", (myvid + " " + mymac + " "  + nvid+ " " + nmac+ "\n").c_str());
		}
		fclose(topo);
		return topo == NULL ? false:true;
	}
	return false;
}

String
VEILNeighborTable::read_handler(Element *e, void *thunk)
{
	StringAccum sa;
	VEILNeighborTable *nt = (VEILNeighborTable *) e;
	NeighborTable::iterator iter;
	NeighborTable neighbors = nt->neighbors;

	sa << "\n-----------------Neighbor Table START-----------------\n"<<"[::NeighborTable::]" << '\n';
	sa << "Neighbor VID,MAC" << "\t" << "Myinterface VID,MAC" << '\t' << "TTL" << '\n';

	for(iter = neighbors.begin(); iter; ++iter){
		String nmac = static_cast<EtherAddress>(iter.key()).s();
		NeighborTableEntry nte = iter.value();

		String nvid = static_cast<VID>(nte.neighborVID).switchVIDString();
		String mymac = static_cast<EtherAddress>(nte.myMac).s();
		String myvid = static_cast<VID>(nte.myVid).switchVIDString();

		Timer *t = nte.expiry;
		sa << nvid <<","<<nmac<< '\t' << myvid <<","<<mymac<< "\t"<<t->expiry().sec() - time(NULL) << " sec\n";
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
