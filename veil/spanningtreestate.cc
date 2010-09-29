#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/straccum.hh>
#include "spanningtreestate.hh"
#include <time.h>

//TODO: make reads and writes atomic

CLICK_DECLS

VEILSpanningTreeState::VEILSpanningTreeState () {}

VEILSpanningTreeState::~VEILSpanningTreeState () {}

int
VEILSpanningTreeState::configure(Vector<String> &conf, ErrorHandler *errh)
{
	click_chatter("[::NeighborTable::][FixME] Its mandatory to have 'PRINTDEBUG value' (here value = true/false) at the end of the configuration string!\n");
	int res = 0;
	int i = 0;
	/*for (i = 0; i < conf.size()-1; i++) {
		res = xxxx(conf[i], errh);
	}*/
	veil_chatter(printDebugMessages,"[::NeighborTable::] Configured the neighbor table!\n");

	cp_shift_spacevec(conf[i]);
	String printflag = cp_shift_spacevec(conf[i]);
	if(!cp_bool(printflag, &printDebugMessages))
		return errh->error("[::NeighborTable::][Error] PRINTDEBUG FLAG should be either true or false");	
	return res;
}


bool
VEILSpanningTreeState::updateCostToVCC(EtherAddress neighbormac, EtherAddress vccmac, uint16_t cost)
{
	// caller is assumed to make sure if the entry here is a valid one.
	// the cost is the cost advertised by the neighbor.

	bool retval = false; // returns false if a new entry is created
	// else returns true if the entry already existed.

	ParentEntry* pentry = forwardingTableToVCC.get_pointer(vccmac);
	if (pentry != NULL){
		if(pentry->hopsToVcc >= cost + 1){
			pentry->hopsToVcc = cost + 1;
			if (pentry->ParentMac != neighbormac){
				// just update the timer value.
				memcpy(&pentry->ParentMac, &neighbormac, 6);
			}

			pentry->expiry->unschedule();
			pentry->expiry->schedule_after_msec(VEIL_SPANNING_TREE_ENTRY_EXPIRY);
			veil_chatter(printDebugMessages,"[::-SpanningTreeState-::][Refreshed Parent MAC: %s] to VCC: %s with cost %d \n",pentry->ParentMac.s().c_str(),vccmac.s().c_str(), cost+1);
			return true;
		}
	}

	TimerData *tdata = new TimerData();
	tdata->ftable = &forwardingTableToVCC;
	tdata->ststate = this;
	memcpy(&tdata->mac, &vccmac, 6);
	tdata->isParentEntry = true;

	ParentEntry newentry;
	memcpy(&newentry.ParentMac, &neighbormac, 6);
	newentry.hopsToVcc = cost + 1;

	Timer *expiry = new Timer(&VEILSpanningTreeState::expire, tdata);
	expiry->initialize(this);
	expiry->schedule_after_msec(VEIL_SPANNING_TREE_ENTRY_EXPIRY);
	newentry.expiry  = expiry;

	veil_chatter(printDebugMessages,"[::-SpanningTreeState-::][New Parent MAC: %s] to VCC: %s with cost %d \n",newentry.ParentMac.s().c_str(),vccmac.s().c_str(), cost+1);
	forwardingTableToVCC.set(vccmac, newentry);

	return retval;
}



bool
VEILSpanningTreeState::updateChild(EtherAddress childmac, EtherAddress vccmac){
	// caller is assumed to make sure if the entry here is a valid one.
	// the cost is the cost advertised by the neighbor.

	bool retval = false; // returns false if a new entry is created
	// else returns true if the entry already existed.

	ChildEntry* centry = forwardingTableFromVCC.get_pointer(childmac);
	if (centry != NULL){
		if (centry->VccMac != vccmac){
			// just update the timer value.
			memcpy(&centry->VccMac, &vccmac, 6);
		}

		centry->expiry->unschedule();
		centry->expiry->schedule_after_msec(VEIL_SPANNING_TREE_ENTRY_EXPIRY);
		veil_chatter(printDebugMessages,"[::-SpanningTreeState-::][Refreshed Child MAC: %s] to VCC: %s\n",childmac.s().c_str(),centry->VccMac.s().c_str());
		return true;
	}

	TimerData *tdata = new TimerData();
	tdata->ftable = &forwardingTableFromVCC;
	tdata->ststate = this;
	memcpy(&tdata->mac, &childmac, 6);
	tdata->isParentEntry = false;

	ChildEntry newentry;
	memcpy(&newentry.VccMac, &vccmac, 6);

	Timer *expiry = new Timer(&VEILSpanningTreeState::expire, tdata);
	expiry->initialize(this);
	expiry->schedule_after_msec(VEIL_SPANNING_TREE_ENTRY_EXPIRY);
	newentry.expiry  = expiry;

	veil_chatter(printDebugMessages,"[::-SpanningTreeState-::][New Child MAC: %s] to VCC: %s\n",childmac.s().c_str(),newentry.VccMac.s().c_str());
	forwardingTableFromVCC.set(childmac, newentry);

	return retval;
}

void
VEILSpanningTreeState::expire(Timer *t, void *data)
{
	TimerData *td = (TimerData *) data;

	if (td->isParentEntry){
		ForwardingTableToVCC * ftable = (ForwardingTableToVCC *) td->ftable;
		ftable->erase(td->mac);
		veil_chatter(true,"[::-SpanningTreeState-::] [Timer Expired] ParentEntry for VCC MAC: |%s|\n",td->mac.s().c_str());
	}else{
		ForwardingTableFromVCC * ftable = (ForwardingTableFromVCC *) td->ftable;
		ftable->erase(td->mac);
		veil_chatter(true,"[::-SpanningTreeState-::] [Timer Expired] Child MAC: |%s|\n",td->mac.s().c_str());
	}
	delete(td);
	delete(t);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILSpanningTreeState)
