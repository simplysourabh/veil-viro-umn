#include <click/config.h>
#include <click/confparse.hh>
#include <click/straccum.hh>
#include <click/error.hh>
#include "routetable.hh"
#include<time.h>

//TODO: make reads and writes atomic

CLICK_DECLS

VEILRouteTable::VEILRouteTable () {}

VEILRouteTable::~VEILRouteTable () {}

//String is of the form: interfaceVID bucket# nhVID gVID
int
VEILRouteTable::cp_viro_route(String s, ErrorHandler* errh){
	VID ivid, nhvid, gvid;
	int bucket;

	String ivid_str = cp_shift_spacevec(s);
	if(!cp_vid(ivid_str, &ivid))
		return errh->error("[RouteTable] [Error] interface VID is not in expected format");
	String bucket_str = cp_shift_spacevec(s);
	if(!cp_integer(bucket_str, &bucket))
		return errh->error("[RouteTable] [Error] invalid integer");
	String nhvid_str = cp_shift_spacevec(s);
	if(!cp_vid(nhvid_str, &nhvid))
		return errh->error("[RouteTable] [Error] nexthop VID is not in expected format");
	String gvid_str = cp_shift_spacevec(s);
	if(!cp_vid(gvid_str, &gvid))
		return errh->error("[RouteTable] [Error] gateway VID is not in expected format");
	updateEntry(&ivid, bucket, &nhvid, &gvid);
	return 0;
}

int
VEILRouteTable::configure(Vector<String> &conf, ErrorHandler *errh)
{	
	int res = 0;
	for (int i = 0; i < conf.size(); i++) {
		res = cp_viro_route(conf[i], errh);
	}
	return res;
}

void
VEILRouteTable::updateEntry (
	VID *i, int b, VID *nh, VID *g)
{
	// SJ : TODO: Allowing multiple routing entries per bucket?
	
	TimerData *tdata = new TimerData();
	tdata->bucket = b;
	tdata->interface = new VID(i->data());
	tdata->routes = &routes;

	//we first want to check if an InnerRouteTable is already present
	//Check if innerroutetable is present for interface vid = i?
	InnerRouteTable *rt;
	if(routes.find(*i) != routes.end()){
		// yes we have a innerroutetable for interface vid = i
		// get the pointer to the table.
		rt = routes.get_pointer(*i);
	} else {
		// No we dont have the innerroutetable for interface vid = i
		// create new innerroutetable for the interface vid = i, and 
		// add it to the main routing table.
		rt = new HashTable<int, InnerRouteTableEntry>::HashTable();
		routes.set(*i,*rt);
		rt = routes.get_pointer(*i);
	}

	InnerRouteTableEntry* entry = new InnerRouteTableEntry(); // New routing entry

	memcpy(&(entry->nextHop), nh, 6);
	memcpy(&(entry->gateway), g, 6);
	Timer *expiry = new Timer(&VEILRouteTable::expire, tdata);
	expiry->initialize(this);
	expiry->schedule_after_msec(VEIL_TBL_ENTRY_EXPIRY);
	entry->expiry  = expiry;
	
	// First see if the key is already present?
	InnerRouteTableEntry * oldEntry;
	oldEntry = rt->get_pointer(b);
	if (oldEntry != NULL){
		oldEntry->expiry->unschedule();
		delete(oldEntry->expiry);
		click_chatter("[RouteTable] Existing Bucket %d for Interface |%s| \n",b, i->vid_string().c_str());
	}else{
		click_chatter("[RouteTable] New Bucket %d for Interface |%s| \n",b, i->vid_string().c_str());
	}	
	rt->set(b, *entry);
}

bool
VEILRouteTable::getRoute(VID* dst, int b, VID* i, VID *nh, VID *g)
{
	bool found = false;
	
	//go thru RTs for all interfaces and return the 
	//first entry found 
	//TODO: we can change this policy later so that if multiple
	//routes are present, we choose the one with the lowest XOR dist
	OuterRouteTable::iterator iter;
	for (iter = routes.begin(); iter; ++iter){
		InnerRouteTable rt = iter.value();
		VID interface = iter.key();

		if (rt.find(b) != rt.end() && dst->logical_distance(&interface) < b) {
			InnerRouteTableEntry irte = rt.get(b);
			memcpy(i, &iter.key(), 6);
			memcpy(nh, &irte.nextHop, 6);
			memcpy(g, &irte.gateway, 6);
			found = true;
		}
	}
	return found;
}

bool
VEILRouteTable::getBucket(int b, VID* i, VID *nh)
{
	bool found = false;
	if (routes.find(*i) != routes.end()) {
		InnerRouteTable rt = routes.get(*i);
		if (rt.find(b) != rt.end()) {
			InnerRouteTableEntry irte = rt.get(b);
			memcpy(nh, &irte.nextHop, VID_LEN);
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

	click_chatter("[RouteTable] [Timer Expired] on Interface VID: |%s| for bucket %d \n",interface.vid_string().c_str(), bucket);
	delete td->interface;
	InnerRouteTable irt = td->routes->get(interface);		
	irt.erase(bucket);
	// SJ: Why do we need to delete the entry for the 
	// Interface when it has no buckets in there?
	// I guess, it doesn't really matter. So for now I have 
	// Commented it.
	/*if(0 == irt.size()){
		td->routes->erase(interface);
	}*/
	//click_chatter("%d entries in neighbor table", td->neighbors->size());
	delete(td); 
}

String
VEILRouteTable::read_handler(Element *e, void *thunk)
{
	StringAccum sa;
	VEILRouteTable *rt1 = (VEILRouteTable *) e;
	OuterRouteTable::iterator iter1;
	InnerRouteTable::iterator iter2;
	OuterRouteTable routes1 = rt1->routes;
	sa << "\n-----------------Routing Table START-----------------\n"<<"[Routing Table]" << '\n';
	sa << "My VID" << "\t\t" << "Bucket" << '\t' <<"NextHop VID" << "\t"<< "Gateway VID" << "\t"<< "TTL" << '\n';
	for(iter1 = routes1.begin(); iter1; ++iter1){
		String myInterface = static_cast<VID>(iter1.key()).vid_string();
		InnerRouteTable rt = iter1.value();
		for(iter2 = rt.begin(); iter2; ++iter2){
			int bucket = iter2.key();		
			InnerRouteTableEntry irte = iter2.value();
			String nextHop = static_cast<VID>(irte.nextHop).vid_string();		
			String gateway = static_cast<VID>(irte.gateway).vid_string();		
			Timer *t = irte.expiry;
			sa << myInterface << '\t' << bucket << '\t' << nextHop << '\t' << gateway << "\t"<<t->expiry().sec() - time(NULL) << " sec\n";
		}
	}
	sa<< "----------------- Routing Table END -----------------\n\n";
	return sa.take_string();	  
}

void
VEILRouteTable::add_handlers()
{
	add_read_handler("table", read_handler, (void *)0);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILRouteTable)
