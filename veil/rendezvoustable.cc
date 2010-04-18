#include <click/config.h>
#include <click/confparse.hh>
#include <click/straccum.hh>
#include "rendezvoustable.hh"
#include <time.h>

CLICK_DECLS

VEILRendezvousTable::VEILRendezvousTable () {}

VEILRendezvousTable::~VEILRendezvousTable () {}

//String is of the form: srcVID gatewayVID
int
VEILRendezvousTable::cp_rdv_entry(String s, ErrorHandler* errh){
	VID svid, gvid;

	String svid_str = cp_shift_spacevec(s);
	if(!cp_vid(svid_str, &svid))
		return errh->error("[RendezvousTable -->][Error!] source VID is not in expected format");
	String gvid_str = cp_shift_spacevec(s);
	if(!cp_vid(gvid_str, &gvid))
		return errh->error("[RendezvousTable -->][Error!] gateway VID is not in expected format");
	updateEntry(&svid,&gvid);
	
	veil_chatter(printDebugMessages,"[RendezvousTable -->] [Warning!]: Assumes that length of VID = 6 bytes, switch vid length = 4 bytes, size of int on the machine = 4 bytes\n");
	return 0;
}

int
VEILRendezvousTable::configure(Vector<String> &conf, ErrorHandler *errh)
{	
click_chatter("[RendezvousTable -->][FixME] Its mandagory to have 'PRINTDEBUG value' (here value = true/false) at the end of the configuration string!\n");
	int res = 0;
	int i = 0;
	for (i = 0; i < conf.size()-1; i++) {
		res = cp_rdv_entry(conf[i], errh);
	}
	
	cp_shift_spacevec(conf[i]);
	String printflag = cp_shift_spacevec(conf[i]);
	if(!cp_bool(printflag, &printDebugMessages))
		return errh->error("[RendezvousTable -->][Error] PRINTDEBUG FLAG should be either true or false");	
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
		veil_chatter(printDebugMessages,"[RendezvousTable -->][Edge Refresh] End1 VID: |%s| --> End2 VID: |%s| \n",src->swtichVIDString().c_str(),dest->swtichVIDString().c_str());
	}else{
		veil_chatter(printDebugMessages,"[RendezvousTable -->][New Edge] End1 VID: |%s| --> End2 VID: |%s|\n",src->swtichVIDString().c_str(),dest->swtichVIDString().c_str());
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
			uint32_t newdist = VID::XOR(edge.src, *svid);
			if (newdist <= bestXORDist){
				memcpy(gateway, &edge.src, VID_LEN);
				veil_chatter(printDebugMessages,"[RendezvousTable -->][Gateway Selection] Found a better GW |%s| than |%s| for |%s| at level %d\n", edge.src.swtichVIDString().c_str(),gateway->swtichVIDString().c_str(),svid->swtichVIDString().c_str(),k);
				bestXORDist = newdist; 
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
	veil_chatter(true,"[RendezvousTable -->][Timer Expired] End1 VID: |%s| --> End2 VID: |%s| \n",edge->src.swtichVIDString().c_str(),edge->dest.swtichVIDString().c_str());
	td->rdvedges->erase(*edge);
	t->clear(); 
	delete(t);
	delete(td); 
}

String
VEILRendezvousTable::read_handler(Element *e, void *thunk)
{
	StringAccum sa;
	VEILRendezvousTable *rdvt = (VEILRendezvousTable *) e;
	RendezvousTable::iterator iter;
	RendezvousTable rdvedges = rdvt->rdvedges;
	sa << "\n-----------------RDV Table START-----------------\n"<<"[RendezvousTable -->]" << '\n';
	sa << "End1 VID   -->   End2 VID \tTTL\n";
	for(iter = rdvedges.begin(); iter; ++iter){
		String svid = static_cast<VID>(iter.key().src).swtichVIDString();		
		RendezvousTableEntry rdvte = iter.value();
		String gvid = static_cast<VID>(iter.key().dest).swtichVIDString();		
		Timer *t = rdvte.expiry;
		sa << svid << " --> " << gvid <<"\t"<<t->expiry().sec() - time(NULL) << " sec\n";
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
