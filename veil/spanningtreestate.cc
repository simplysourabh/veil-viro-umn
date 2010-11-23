#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/straccum.hh>
#include "spanningtreestate.hh"
#include <time.h>

//TODO: make reads and writes atomic#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include "click_veil.hh"
#include "processip.hh"

CLICK_DECLS

VEILSpanningTreeState::VEILSpanningTreeState () {}

VEILSpanningTreeState::~VEILSpanningTreeState () {}

//String is of the form: VCCMAC parentVCCMac nhops
int
VEILSpanningTreeState::cp_spanning_tree_state(String s, ErrorHandler* errh){
	EtherAddress vcc,parent;
	uint16_t nhops;
	unsigned int n;
	String vccmac_str = cp_shift_spacevec(s);
	if(!cp_ethernet_address(vccmac_str, &vcc)){
		veil_chatter_new(printDebugMessages, class_name()," [Error] VCC MAC ADD is not in expected format. Entered string: %s",vccmac_str);
		return errh->error(" [Error] VCC MAC ADD is not in expected format.");
	}

	String parentmac_str = cp_shift_spacevec(s);
	if(!cp_ethernet_address(parentmac_str, &parent)){
		veil_chatter_new(printDebugMessages, class_name()," [Error] PARENT MAC ADD is not in expected format. Entered string: %s",parentmac_str);
		return errh->error(" [Error] PARENT MAC ADD is not in expected format.");
	}
	String nhops_str = cp_shift_spacevec(s);
	if(!cp_unsigned(nhops_str, &n)){
		veil_chatter_new(printDebugMessages, class_name()," [Error] Cost is not in correct format. Entered string: %s",nhops_str);
		return errh->error(" [Error] Cost is not in correct format.");
	}

	updateCostToVCC(parent, vcc, (uint16_t)n );
	return 0;
}


int
VEILSpanningTreeState::configure(Vector<String> &conf, ErrorHandler *errh)
{
	veil_chatter_new(true,class_name()," [FixME] Its mandatory to have 'PRINTDEBUG value' (here value = true/false) at the end of the configuration string!");
	int res = 0;
	int i = 0;
	for (i = 0; i < conf.size()-1; i++) {
		res = cp_spanning_tree_state(conf[i], errh);
		if (res != 0){
			veil_chatter_new(printDebugMessages, class_name()," Error occurred while parsing the string:'%s'!",conf[i]);
		}
	}
	veil_chatter_new(printDebugMessages, class_name()," Configured the initial SpanningTreeState table!");

	cp_shift_spacevec(conf[i]);
	String printflag = cp_shift_spacevec(conf[i]);
	if(!cp_bool(printflag, &printDebugMessages))
		return errh->error(" [Error] PRINTDEBUG FLAG should be either true or false");
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
	// if the entry already exists in the table.
	if (pentry != NULL){
		if(pentry->hopsToVcc >= cost + 1){
			pentry->hopsToVcc = cost + 1;
			if (pentry->ParentMac != neighbormac){
				// just update the timer value.
				memcpy(&pentry->ParentMac, &neighbormac, 6);
			}

			pentry->expiry->unschedule();
			pentry->expiry->schedule_after_msec(VEIL_SPANNING_TREE_ENTRY_EXPIRY);
			veil_chatter_new(printDebugMessages, class_name(),"[Refreshed Parent MAC: %s] to VCC: %s with cost %d ",pentry->ParentMac.s().c_str(),vccmac.s().c_str(), cost+1);
			return true;
		}else{
			return false;
		}
	}

	// else if entry for vccmac is not already there.
	// in this case create a new entry.

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

	veil_chatter_new(printDebugMessages, class_name(),"[New Parent MAC: %s] to VCC: %s with cost %d ",newentry.ParentMac.s().c_str(),vccmac.s().c_str(), cost+1);
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
		veil_chatter_new(printDebugMessages, class_name(),"[Refreshed Child MAC: %s] to VCC: %s",childmac.s().c_str(),centry->VccMac.s().c_str());
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

	veil_chatter_new(printDebugMessages, class_name(),"[New Child MAC: %s] to VCC: %s",childmac.s().c_str(),newentry.VccMac.s().c_str());
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
		veil_chatter_new(true, "VEILSpanningTreeState"," [Timer Expired] ParentEntry for VCC MAC: |%s|",td->mac.s().c_str());
	}else{
		ForwardingTableFromVCC * ftable = (ForwardingTableFromVCC *) td->ftable;
		ftable->erase(td->mac);
		veil_chatter_new(true, "VEILSpanningTreeState"," [Timer Expired] Child MAC: |%s|",td->mac.s().c_str());
	}
	delete(td);
	delete(t);
}


String
VEILSpanningTreeState::read_handler(Element *e, void *thunk)
{
	StringAccum sa;
	VEILSpanningTreeState *st = (VEILSpanningTreeState *) e;
	ForwardingTableToVCC::iterator iterTo;
	ForwardingTableFromVCC::iterator iterFrom;

	sa << "\n-----------------SpanningTreeState START-----------------\n"<<"" << '\n';
	sa << "VCC MAC" << "\t\t\t" << "Parent(forwarder)MAC" << "\t\tCost(#hops)"<<'\t' << "TTL" << '\n';
	for (iterTo = st->forwardingTableToVCC.begin(); iterTo; ++iterTo){
		String vccmac = static_cast <EtherAddress> (iterTo.key()).s();
		ParentEntry pe = iterTo.value();
		String parentmac = pe.ParentMac.s();
		Timer *t = pe.expiry;
		sa << vccmac <<"\t"<<parentmac<<"\t"<<pe.hopsToVcc<<"\t"<<t->expiry().sec() - time(NULL) << " Sec\n";
	}
	sa <<"\n";
	sa << "Child MAC\t\t\tVCC MAC\t\tTTL\n";
	for (iterFrom = st->forwardingTableFromVCC.begin(); iterFrom; ++iterFrom){
		String childmac = static_cast <EtherAddress> (iterFrom.key()).s();
		ChildEntry ce = iterFrom.value();
		String vccmac = ce.VccMac;
		Timer *t = ce.expiry;
		sa << vccmac <<"\t"<<childmac<<"\t"<<t->expiry().sec() - time(NULL) << " Sec\n";
	}

	sa<< "----------------- SpanningTreeState END -----------------\n\n";
	return sa.take_string();
}

void
VEILSpanningTreeState::add_handlers()
{
	add_read_handler("state", read_handler, (void *)0);
}

bool
VEILSpanningTreeState::getParentNodeToVCC(EtherAddress vccmac, EtherAddress *parent){
	bool neigh_found = false;
	ParentEntry* pe = forwardingTableToVCC.get_pointer(vccmac);
	if (pe != NULL){
		memcpy(parent, &pe->ParentMac, 6);
		neigh_found = true;
	}
	return neigh_found;
}


CLICK_ENDDECLS

EXPORT_ELEMENT(VEILSpanningTreeState)
