#include <click/config.h>
#include <click/confparse.hh>
#include <click/straccum.hh>
#include "routetable.hh"

//TODO: make reads and writes atomic

CLICK_DECLS

VEILRouteTable::VEILRouteTable () {}

VEILRouteTable::~VEILRouteTable () {}

void
VEILRouteTable::updateEntry (
	VID *i, int b, VID *nh, VID *g)
{
	bool present = false;
	TimerData *tdata = new TimerData();
	tdata->bucket = b;
	tdata->interface = new VID();
	memcpy(tdata->interface, i, VID_LEN);
	tdata->routes = &routes;

	//we first want to check if an InnerRouteTable is already present

	InnerRouteTable *rt;
	if(routes.find(*i) != routes.end()){
		rt = routes.get_pointer(*i);
		present = true;
	} else {
		rt = new HashTable<int, InnerRouteTableEntry>::HashTable();
	}

	InnerRouteTableEntry entry;

	memcpy(&entry.nextHop, nh, 6);
	memcpy(&entry.gateway, g, 6);
	Timer *expiry = new Timer(&VEILRouteTable::expire, tdata);
	expiry->initialize(this);
	expiry->schedule_after_msec(VEIL_TBL_ENTRY_EXPIRY);
	entry.expiry  = expiry;
	rt->set(b, entry);

	if(!present)
		routes.set(*i,*rt);
}

bool
VEILRouteTable::lookupEntry(int b, VID* i, VID *nh, VID *g)
{
	bool found = false;
	
	//go thru RTs for all interfaces and return the 
	//first entry found 
	//TODO: we can change this policy later so that if multiple
	//routes are present, we choose the one with the lowest XOR dist
	OuterRouteTable::iterator iter;
	for (iter = routes.begin(); iter; ++iter){
		InnerRouteTable rt = iter.value();
		if (rt.find(b) != rt.end()) {	
			InnerRouteTableEntry irte = rt.get(b);
			memcpy(i, &iter.key(), 6);
			memcpy(nh, &irte.nextHop, 6);
			memcpy(g, &irte.gateway, 6);
			found = true;
		}
	}
	return found;
}

void
VEILRouteTable::expire(Timer *t, void *data) 	
{
	TimerData *td = (TimerData *) data;
	int bucket = td->bucket;
	VID interface;
	memcpy(&interface, td->interface, VID_LEN);

	delete td->interface;
	InnerRouteTable irt = td->routes->get(interface);		
	irt.erase(bucket);
	if(0 == irt.size()){
		td->routes->erase(interface);
	}
	//click_chatter("%d entries in neighbor table", td->neighbors->size());
	delete(td); 
}

String
VEILRouteTable::read_handler(Element *e, void *thunk)
{
	StringAccum sa;
	VEILRouteTable *rt = (VEILRouteTable *) e;
	OuterRouteTable::iterator iter1;
	InnerRouteTable::iterator iter2;
	OuterRouteTable routes = rt->routes;
	
	for(iter1 = routes.begin(); iter1; ++iter1){
		String myInterface = static_cast<VID>(iter1.key()).vid_string();
		InnerRouteTable rt = iter1.value();
		for(iter2 = rt.begin(); iter2; ++iter2){
			int bucket = iter2.key();		
			InnerRouteTableEntry irte = iter2.value();
			String nextHop = static_cast<VID>(irte.nextHop).vid_string();		
			String gateway = static_cast<VID>(irte.gateway).vid_string();		
			Timer *t = irte.expiry;
			sa << myInterface << ' ' << bucket << ' ' << nextHop << ' ' << gateway << ' ' << t->expiry() << '\n';
		}
	}
	
	return sa.take_string();	  
}

void
VEILRouteTable::add_handlers()
{
	add_read_handler("table", read_handler, (void *)0);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILRouteTable)
