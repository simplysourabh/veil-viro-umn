//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
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
		return errh->error("[Error!] source VID is not in expected format");
	String gvid_str = cp_shift_spacevec(s);
	if(!cp_vid(gvid_str, &gvid))
		return errh->error("[Error!] gateway VID is not in expected format");
	updateEntry(&svid,&gvid);

	veil_chatter_new(printDebugMessages, class_name()," [Warning!]: Assumes that length of VID = 6 bytes, switch vid length = 4 bytes, size of int on the machine = 4 bytes");
	return 0;
}

int
VEILRendezvousTable::configure(Vector<String> &conf, ErrorHandler *errh)
{	
	veil_chatter_new(true,class_name(),"[FixME] Its mandagory to have 'PRINTDEBUG value' (here value = true/false) at the end of the configuration string!");
	int res = 0;
	int i = 0;
	for (i = 0; i < conf.size()-1; i++) {
		res = cp_rdv_entry(conf[i], errh);
	}

	cp_shift_spacevec(conf[i]);
	String printflag = cp_shift_spacevec(conf[i]);
	if(!cp_bool(printflag, &printDebugMessages))
		return errh->error("[Error] PRINTDEBUG FLAG should be either true or false");	
	return res;
}

void
VEILRendezvousTable::updateEntry (
		VID *src, VID *dest)
{
	RendezvousEdge *edge = new RendezvousEdge(src, dest);
	TimerData *tdata = new TimerData();
	tdata->edge = edge;
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
		veil_chatter_new(printDebugMessages, class_name(),"[Edge Refresh] End1 VID: |%s| --> End2 VID: |%s| ",src->switchVIDString().c_str(),dest->switchVIDString().c_str());
	}else{
		veil_chatter_new(printDebugMessages, class_name(),"[New Edge] End1 VID: |%s| --> End2 VID: |%s|",src->switchVIDString().c_str(),dest->switchVIDString().c_str());
	}

	rdvedges.set(*edge, entry);
}

//TODO: this returns the first entry with dist k. Modify it to return one with lowest logical dist or one at closest XOR distance.
uint8_t
VEILRendezvousTable::getRdvPoint(int k, VID* svid, VID* gateway)
{

	RendezvousTable::iterator iter;
	unsigned int bestXORDist = 0xFFFFFFFF; // only considers the best 32 bits of the VIDs
	VID defaultgw;
	uint8_t ngws = 0;
	uint8_t retval = 0;
	for(iter = rdvedges.begin(); iter; ++iter){
		RendezvousEdge edge = iter.key();
		if(k == svid->logical_distance(&edge.dest) && 
				svid->logical_distance(&edge.src) < k){
			uint32_t newdist = VID::XOR(edge.src, *svid);
			veil_chatter_new(printDebugMessages, class_name(),"[Gateway Selection] Found a GW |%s|  for |%s| at level %d", edge.src.switchVIDString().c_str(),svid->switchVIDString().c_str(),k);
			if (newdist <= bestXORDist){
				//veil_chatter_new(printDebugMessages, class_name(),"[Gateway Selection] Found a better GW |%s| than |%s| for |%s| at level %d", edge.src.switchVIDString().c_str(),defaultgw.switchVIDString().c_str(),svid->switchVIDString().c_str(),k);
				memcpy(&defaultgw, &edge.src, VID_LEN);
				bestXORDist = newdist; 
			}
			ngws++;
		}
	}
	// Note that ngws may count same gateway vid more than once.

	// number of gateways are bounded by the MAX_GW_PER_BUCKET
	ngws = ngws > MAX_GW_PER_BUCKET? MAX_GW_PER_BUCKET:ngws;
	if (ngws > 0){
		// first copy the default gateway:
		memcpy(gateway, &defaultgw, 6);
		uint8_t r_gw = 1;
		retval++;
		for(iter = rdvedges.begin(); iter; ++iter){
			if (r_gw == ngws){break;}
			RendezvousEdge edge = iter.key();
			if(k == svid->logical_distance(&edge.dest) &&
					svid->logical_distance(&edge.src) < k){
				if (edge.src != defaultgw){
					memcpy(gateway+ r_gw, &edge.src, 6);
					veil_chatter_new(printDebugMessages, class_name(),"[Gateway Selection] Writing GW |%s|  for |%s| at level %d", (gateway+r_gw)->switchVIDString().c_str(),svid->switchVIDString().c_str(),k);
					r_gw++;
					retval++;
					//retval now contains the actual count of number of gateways returned.
				}
			}
		}
	}
	veil_chatter_new(printDebugMessages, class_name(),"[Gateway Selection] Found %d gateways  for |%s| at level %d", retval,svid->switchVIDString().c_str(),k);
	return retval;
}

void
VEILRendezvousTable::expire(Timer *t, void *data) 	
{
	TimerData *td = (TimerData *) data;
	RendezvousEdge* edge = (RendezvousEdge *) td->edge;
	veil_chatter_new(true, "VEILRendezvousTable" ,"[Timer Expired] End1 VID: |%s| --> End2 VID: |%s| ",edge->src.switchVIDString().c_str(),edge->dest.switchVIDString().c_str());
	td->rdvedges->erase(*edge);
	delete(td->edge);
	delete(td);
	t->clear(); 
	delete(t);
}

String
VEILRendezvousTable::read_handler(Element *e, void *thunk)
{
	StringAccum sa;
	VEILRendezvousTable *rdvt = (VEILRendezvousTable *) e;
	RendezvousTable::iterator iter;
	RendezvousTable rdvedges = rdvt->rdvedges;
	sa << "\n-----------------RDV Table START-----------------\n"<<"" << '\n';
	sa << "End1 VID   -->   End2 VID \tTTL\n";
	for(iter = rdvedges.begin(); iter; ++iter){
		String svid = static_cast<VID>(iter.key().src).switchVIDString();		
		RendezvousTableEntry rdvte = iter.value();
		String gvid = static_cast<VID>(iter.key().dest).switchVIDString();		
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
