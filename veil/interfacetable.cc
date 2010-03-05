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
	VID *interface = new VID();
	for(int i = 0; i < conf.size(); i++){
		if(cp_vid(conf[i], interface)){
			interfaces.set(*interface, i);
			rinterfaces.set(i, *interface);
		} else {
			errh->error("error in processing argument in VEILInterfaceTable");
			r = -EINVAL;	
		}
	}

	delete interface;
#include <click/error.hh>
	return r;
}

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
	
	sa << "Interface Table" << '\n';
	sa << "interfacevid" << ' ' << "interface#\n";
	for(iter = interfaces.begin(); iter; ++iter){
		String vid = static_cast<VID>(iter.key()).vid_string();		
		int i = iter.value();
		sa << vid << ' ' << i << '\n';
	}
	
	return sa.take_string();	  
}

void
VEILInterfaceTable::add_handlers()
{
	add_read_handler("table", read_handler, (void *)0);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILInterfaceTable)
