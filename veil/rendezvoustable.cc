#include <click/config.h>
#include <click/confparse.hh>
#include <click/straccum.hh>
#include "rendezvoustable.hh"

CLICK_DECLS

VEILRendezvousTable::VEILRendezvousTable () {}

VEILRendezvousTable::~VEILRendezvousTable () {}

//String is of the form: srcVID gatewayVID
int
VEILRendezvousTable::cp_rdv_entry(String s, ErrorHandler* errh){
	VID svid, gvid;

	String svid_str = cp_shift_spacevec(s);
	if(!cp_vid(svid_str, &svid))
		return errh->error("source VID is not in expected format");
	String gvid_str = cp_shift_spacevec(s);
	if(!cp_vid(gvid_str, &gvid))
		return errh->error("gateway VID is not in expected format");
	updateEntry(&svid, &gvid);
	return 0;
}

int
VEILRendezvousTable::configure(Vector<String> &conf, ErrorHandler *errh)
{	
	int res = 0;
	for (int i = 0; i < conf.size(); i++) {
		res = cp_rdv_entry(conf[i], errh);
	}
	return res;
}

void
VEILRendezvousTable::updateEntry (
	VID *svid, VID *gvid)
{
	TimerData *tdata = new TimerData();
	tdata->vid = svid;
	tdata->rdvpts = &rdvpts;
	
	RendezvousTableEntry entry;

	memcpy(&entry.gatewayVid, gvid, 6);
	Timer *expiry = new Timer(&VEILRendezvousTable::expire, tdata);
	expiry->initialize(this);
	expiry->schedule_after_msec(VEIL_TBL_ENTRY_EXPIRY);
	entry.expiry  = expiry;
	rdvpts.set(*svid, entry);
}

//TODO: this returns the first entry with dist k. Modify it to return one with lowest logical dist or one at closest XOR distance.
bool
VEILRendezvousTable::getRdvPoint(int k, VID* svid, VID* gvid)
{
	bool found = false;
	RendezvousTable::iterator iter;

	for(iter = rdvpts.begin(); iter; ++iter){
		RendezvousTableEntry rte = iter.value();
		VID end1 = iter.key();
		if(k == svid->logical_distance(&rte.gatewayVid) && 
			svid->logical_distance(&end1) < k){
			memcpy(gvid, &iter.key(), VID_LEN);
			found = true;
			break;			
		}
	}
	return found;
}

void
VEILRendezvousTable::expire(Timer *t, void *data) 	
{
	TimerData *td = (TimerData *) data;
	VID* svid = (VID *) td->vid;
	td->rdvpts->erase(*svid);
	delete(td); 
}

String
VEILRendezvousTable::read_handler(Element *e, void *thunk)
{
	StringAccum sa;
	VEILRendezvousTable *rdvt = (VEILRendezvousTable *) e;
	RendezvousTable::iterator iter;
	RendezvousTable rdvpts = rdvt->rdvpts;
	
	for(iter = rdvpts.begin(); iter; ++iter){
		String svid = static_cast<VID>(iter.key()).vid_string();		
		RendezvousTableEntry rdvte = iter.value();
		String gvid = static_cast<VID>(rdvte.gatewayVid).vid_string();		
		Timer *t = rdvte.expiry;

		sa << svid << ' ' << gvid << ' ' << t->expiry() << '\n';
	}
	
	return sa.take_string();	  
}

void
VEILRendezvousTable::add_handlers()
{
	add_read_handler("table", read_handler, (void *)0);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILRendezvousTable)
