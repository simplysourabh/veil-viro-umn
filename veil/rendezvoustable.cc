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
	updateEntry(&svid,&gvid);
	
	click_chatter("[Warning!]: Assumes that length of VID = 6 bytes, switch vid length = 4 bytes, size of int on the machine = 4 bytes\n");
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
	VID *src, VID *dest)
{
	RendezvousEdge *edge = new RendezvousEdge(src, dest);
	RendezvousEdge *ed1 = new RendezvousEdge(src, dest);
	TimerData *tdata = new TimerData();
	tdata->edge = ed1;
	tdata->rdvedges = &rdvedges;
	
	RendezvousTableEntry entry;
	Timer *expiry = new Timer(&VEILRendezvousTable::expire, tdata);
	expiry->initialize(this);
	expiry->schedule_after_msec(VEIL_TBL_ENTRY_EXPIRY);
	entry.expiry  = expiry;
	// First see if the key is already present?
	RendezvousTableEntry * oldEntry;
	oldEntry = rdvedges.get_pointer(*edge);
	if (oldEntry != NULL){
		oldEntry->expiry->unschedule();
		delete(oldEntry->expiry);
		click_chatter("[RendezvousTable][Edge Refresh] End1 VID: |%s| ==> End2 VID: |%s| \n",src->vid_string().c_str(),dest->vid_string().c_str());
	}else{
		click_chatter("[RendezvousTable][New Edge] End1 VID: |%s| ==> End2 VID: |%s|\n",src->vid_string().c_str(),dest->vid_string().c_str());
	}
	
	rdvedges.set(*edge, entry);
}

//TODO: this returns the first entry with dist k. Modify it to return one with lowest logical dist or one at closest XOR distance.
bool
VEILRendezvousTable::getRdvPoint(int k, VID* svid, VID* gateway)
{
	bool found = false;
	RendezvousTable::iterator iter;
	unsigned int bestXORDist = 0xFFFFFFFF; // only considers the best 32 bits of the VIDs
	
	for(iter = rdvedges.begin(); iter; ++iter){
		RendezvousEdge edge = iter.key();
		if(k == svid->logical_distance(&edge.dest) && 
		   svid->logical_distance(&edge.src) < k){
			unsigned int temp1,temp2;
			memcpy(&temp1, &edge.src, 4);
			memcpy(&temp2, gateway,4);
			if ((temp1 ^ temp2) <= bestXORDist){
				memcpy(gateway, &edge.src, VID_LEN);
				click_chatter("[RendezvousTable][Gateway Selection] Found a better GW |%s| than |%s| for |%s| at level %d\n", edge.src.vid_string().c_str(),gateway->vid_string().c_str(),svid->vid_string().c_str(),k);
				bestXORDist = temp1 ^ temp2; 
			}
			found = true;		
		}
	}
	return found;
}

void
VEILRendezvousTable::expire(Timer *t, void *data) 	
{
	TimerData *td = (TimerData *) data;
	RendezvousEdge* edge = (RendezvousEdge *) td->edge;
	click_chatter("[RendezvousTable][Timer Expired] End1 VID: |%s| ==> End2 VID: |%s| \n",edge->src.vid_string().c_str(),edge->dest.vid_string().c_str());
	td->rdvedges->erase(*edge);
	delete(td); 
}

String
VEILRendezvousTable::read_handler(Element *e, void *thunk)
{
	StringAccum sa;
	VEILRendezvousTable *rdvt = (VEILRendezvousTable *) e;
	RendezvousTable::iterator iter;
	RendezvousTable rdvedges = rdvt->rdvedges;
	sa << "\n-----------------RDV Table START-----------------\n"<<"[RendezvousTable]" << '\n';
	sa << "End1 VID   ==>   End2 VID \t\t Expiry\n";
	for(iter = rdvedges.begin(); iter; ++iter){
		String svid = static_cast<VID>(iter.key().src).vid_string();		
		RendezvousTableEntry rdvte = iter.value();
		String gvid = static_cast<VID>(iter.key().dest).vid_string();		
		Timer *t = rdvte.expiry;

		sa << svid << " ==> " << gvid << "\t Status:" << t->scheduled() << " ("<<t->expiry().sec() << " second to expire) \n";
	}
	sa << "----------------- RDV Table END -----------------\n\n";
	return sa.take_string();	  
}

void
VEILRendezvousTable::add_handlers()
{
	add_read_handler("table", read_handler, (void *)0);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILRendezvousTable)
