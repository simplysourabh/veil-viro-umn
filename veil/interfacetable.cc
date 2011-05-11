//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/straccum.hh>
#include "interfacetable.hh"

CLICK_DECLS

VEILInterfaceTable::VEILInterfaceTable () {}

VEILInterfaceTable::~VEILInterfaceTable () {}

int 
VEILInterfaceTable::configure(Vector<String> &conf, ErrorHandler *errh)
{
	int r = 0;
	VID interface;
	EtherAddress macadd;
	int i = 0;
	for(i = 0; i < conf.size()-2; i = i+2){
		int index = i/2;
		if(cp_vid(conf[i], &interface)){
			interfaces.set(interface, index);
			//switch_interfaces.set(interface, i);
			rinterfaces.set(index, interface);
			isvidset.set(index,false);
		} else {
			errh->error("[InterfaceTable][Error!]error in processing VID argument in VEILInterfaceTable, the format is <VID 0>, <MAC 0>, <VID 1>, <MAC 1> and so on!");
			r = -EINVAL;
		}

		if(cp_ethernet_address(conf[i+1], &macadd)){
			etheraddToInterfaceIndex.set(macadd, index);
			interfaceIndexToEtherAddr.set(index, macadd);
		} else {
			errh->error("[InterfaceTable][Error!]error in processing MAC ADDR argument in VEILInterfaceTable, the format is <VID 0>, <MAC 0>, <VID 1>, <MAC 1> and so on!");
			r = -EINVAL;	
		}
	}

	cp_shift_spacevec(conf[i]);
	String flag = cp_shift_spacevec(conf[i]);
	if(!cp_bool(flag, &isVIDAssignmentDone))
		return errh->error("[## InterfaceTable **] [Error] UseStatic FLAG should be either true or false");

	for (int i = 0; i < isvidset.size(); i++){
		isvidset.set(i,isVIDAssignmentDone);
	}

	if (isVIDAssignmentDone == false){interfaces.clear();}

	i++;
	cp_shift_spacevec(conf[i]);
	String printflag = cp_shift_spacevec(conf[i]);
	if(!cp_bool(printflag, &printDebugMessages))
		return errh->error("[## InterfaceTable **] [Error] PRINTDEBUG FLAG should be either true or false");	
	return r;
}

/*
void
VEILInterfaceTable::deleteHostInterface(VID *i){	
	switch_interfaces.erase(*i);
}*/

bool
VEILInterfaceTable::lookupVidEntry(VID* ivid, int* i)
{
	bool found = false;
	// make sure that reverse mapping is also correct.
	if (interfaces.get_pointer(*ivid) != NULL){
		*i = interfaces.get(*ivid);
		if(rinterfaces.get(*i) == *ivid){
			return true;
		}else{
			veil_chatter_new(printDebugMessages, class_name(), "Warning: VID %s for interface %d is inconsistent, needs to be removed from interfaces.", ivid->switchVIDString().c_str(), *i);
			return false;
		}
	}

	return found;
}

bool
VEILInterfaceTable::lookupIntEntry(int i, VID* ivid)
{
	bool found = false;
	if (!(rinterfaces.find(i) == rinterfaces.end())) {
		VID vid = rinterfaces.get(i);
		//printf("VEILInterfaceTable::lookupIntEntry vid: %s, int: %d VIDLEN %d\n", vid.vid_string().c_str(), i, VID_LEN);
		memcpy(ivid, &vid, VID_LEN);
		found = true;
	}
	return found;
}

bool
VEILInterfaceTable::getMacAddr(int i, EtherAddress* imacadd){
	bool found = false;
	if (!(interfaceIndexToEtherAddr.find(i) == interfaceIndexToEtherAddr.end())) {
		EtherAddress macadd = interfaceIndexToEtherAddr.get(i);
		memcpy(imacadd, &macadd, 6);
		found = true;
	}
	return found;
}

bool
VEILInterfaceTable::getIntUsingMac(EtherAddress* macadd, int* i){
	bool found = false;
	if (!(etheraddToInterfaceIndex.find(*macadd) == etheraddToInterfaceIndex.end())) {
		*i = etheraddToInterfaceIndex.get(*macadd);
		found = true;
	}
	return found;
}


String
VEILInterfaceTable::read_handler(Element *e, void *thunk)
{
	StringAccum sa;
	VEILInterfaceTable *it = (VEILInterfaceTable *) e;
	sa << "\n-----------------Interface Table START-----------------\n"<<"[InterfaceTable]" << '\n';
	if(it->isVIDAssignmentDone == true){
		sa<<"VIDs are assigned!\n";
	}else{
		sa<<"VIDs are NOT assigned yet!\n";
	}
	sa << "ID\tInterface VID" << "\tMAC Address" << "\n";
	for(int i = 0; i < it->numInterfaces(); i++){
		String vid = static_cast<VID>(it->rinterfaces.get(i)).switchVIDString();
		String macadd = (static_cast<EtherAddress>(it->interfaceIndexToEtherAddr.get(i))).s();
		sa <<i<<'\t'<< vid <<'\t' << macadd <<'\n';
	}

	// print all the vids in the table.
	sa<<"\t\t READING interface(vid,i)\n";
	InterfaceTable::iterator iter;
	for (iter = it->interfaces.begin(); iter; iter++){
		String vid = static_cast<VID>(iter.key()).switchVIDString();
		sa<<iter.value()<<"\t"<<vid<<'\n';
	}
	sa<< "----------------- Interface Table END -----------------\n\n";
	return sa.take_string();
}

void
VEILInterfaceTable::add_handlers()
{
	add_read_handler("table", read_handler, (void *)0);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILInterfaceTable)
