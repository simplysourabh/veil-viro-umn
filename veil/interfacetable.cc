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
	int i = 0;
	for(i = 0; i < conf.size()-1; i++){
		if(cp_vid(conf[i], &interface)){
			interfaces.set(interface, i);
			switch_interfaces.set(interface, i);
			rinterfaces.set(i, interface);
		} else {
			errh->error("[InterfaceTable][Error!]error in processing argument in VEILInterfaceTable");
			r = -EINVAL;	
		}
	}

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
	if (!(interfaces.find(*ivid) == interfaces.end())) {
		*i = interfaces.get(*ivid);
		found = true;
	}
	return found;
}

bool
VEILInterfaceTable::lookupIntEntry(int i, VID* ivid)
{
	bool found = false;
	if (!(rinterfaces.find(i) == rinterfaces.end())) {
		VID vid = rinterfaces.get(i);
		memcpy(ivid, &vid, VID_LEN);
		found = true;
	}
	return found;
}


String
VEILInterfaceTable::read_handler(Element *e, void *thunk)
{
	StringAccum sa;
	VEILInterfaceTable *it = (VEILInterfaceTable *) e;
	InterfaceTable::iterator iter;
	InterfaceTable interfaces = it->interfaces;
	sa << "\n-----------------Interface Table START-----------------\n"<<"[InterfaceTable]" << '\n';
	sa << "Interface VID" << '\t' << "interface#\n";
	for(iter = interfaces.begin(); iter; ++iter){
		String vid = static_cast<VID>(iter.key()).switchVIDString();		
		int i = iter.value();
		sa << vid << '\t' << i << '\n';
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
