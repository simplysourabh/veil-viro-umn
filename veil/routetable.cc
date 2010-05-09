#include <click/config.h>
#include <click/confparse.hh>
#include <click/straccum.hh>
#include <click/error.hh>
#include "routetable.hh"
#include <time.h>
#include "click_veil.hh"
#include <stdlib.h>

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
		return errh->error("[++RouteTable] [Error] interface VID is not in expected format");
	String bucket_str = cp_shift_spacevec(s);
	if(!cp_integer(bucket_str, &bucket))
		return errh->error("[++RouteTable] [Error] invalid integer");
	String nhvid_str = cp_shift_spacevec(s);
	if(!cp_vid(nhvid_str, &nhvid))
		return errh->error("[++RouteTable] [Error] nexthop VID is not in expected format");
	String gvid_str = cp_shift_spacevec(s);
	if(!cp_vid(gvid_str, &gvid))
		return errh->error("[++RouteTable] [Error] gateway VID is not in expected format");
	updateEntry(&ivid, bucket, &nhvid, &gvid);
	return 0;
}

int
VEILRouteTable::configure(Vector<String> &conf, ErrorHandler *errh)
{	
	click_chatter("[++RouteTable][FixME] Its mandatory to have 'PRINTDEBUG value' (here value = true/false) at the end of the configuration string!\n");
	click_chatter("[++RouteTable][FixME] Updating default routing entry for a bucket purges all the other routing entries, also default entry is always added in the beginning of the list of routing entries.\n");
	int res = 0;
	int i = 0;
	for (i = 0; i < conf.size()-1; i++) {
		res = cp_viro_route(conf[i], errh);
	}
	cp_shift_spacevec(conf[i]);
	String printflag = cp_shift_spacevec(conf[i]);
	if(!cp_bool(printflag, &printDebugMessages))
		return errh->error("[++RouteTable] [Error] PRINTDEBUG FLAG should be either true or false");	
	return res;
}


// updates the routing entry for the bucket:b of interface:i with nexthop:nh gateway:g
// Also whenever default entry is updated, all the other entries are removed
// TODO: get rid of the above restriction.
void
VEILRouteTable::updateEntry (
	VID *i, uint8_t b, VID *nh, VID *g, bool isDefault)
{
	// SJ : TODO: Allowing multiple routing entries per bucket?
	
	//we first want to check if an InnerRouteTable is already present
	//Check if innerroutetable is present for interface vid = i?
	InnerRouteTable *rt; uint8_t j = 0;
	if(routes.find(*i) != routes.end()){
		// yes we have a innerroutetable for interface vid = i
		// get the pointer to the table.
		rt = routes.get_pointer(*i);
	} else {
		// No we dont have the innerroutetable for interface vid = i
		// create new innerroutetable for the interface vid = i, and 
		// add it to the main routing table.
		rt = new HashTable<uint8_t, Bucket>::HashTable();
		routes.set(*i,*rt);
		rt = routes.get_pointer(*i);
	}

	// Now see if we have any mapping for the bucket:b in interface:i
	Bucket *buck;
	buck = rt->get_pointer(b);

	// there is no mapping already, so create one and add it.
	if (buck == NULL){
		Bucket *newbuck = new Bucket();
		rt->set(b,*newbuck);
		buck = rt->get_pointer(b);
		//set up the timer
		TimerData *tdata = new TimerData();
		tdata->bucket = b;
		tdata->interface = new VID(i->data());
		tdata->routes = &routes;
		Timer *expiry = new Timer(&VEILRouteTable::expire, tdata);
		expiry->initialize(this);
		expiry->schedule_after_msec(VEIL_TBL_ENTRY_EXPIRY);
		buck->expiry  = expiry;
		//make all the buckets invalid initially
		for (j = 0; j < MAX_GW_PER_BUCKET; j++){
			buck->buckets[j].isValid = false;
			buck->buckets[j].isDefault = false;
		}
	}

	// if its the default entry then write it in the beginning, and purge all the old entries:
	if (isDefault){
		memcpy(&(buck->buckets[0].nexthop), nh, 6);
		memcpy(&(buck->buckets[0].gateway), g, 6);
		buck->buckets[0].isValid = true;
		buck->buckets[0].isDefault = true;
		buck->expiry->unschedule();
		buck->expiry->schedule_after_msec(VEIL_TBL_ENTRY_EXPIRY);
		veil_chatter(printDebugMessages,"[++RouteTable] Updated Default and \" purged\" all the other Bucket %d for Interface |%s| \n",b, i->switchVIDString().c_str());
		for (j = 1; j < MAX_GW_PER_BUCKET; j++){
			buck->buckets[j].isValid = false;
		}
		return;
	}
	for (j = 0; j < MAX_GW_PER_BUCKET; j++){
		if (buck->buckets[j].isValid == false){
			memcpy(&(buck->buckets[j].nexthop), nh, 6);
			memcpy(&(buck->buckets[j].gateway), g, 6);
			buck->buckets[j].isValid = true;
			buck->buckets[j].isDefault = false;
			buck->expiry->unschedule();
			buck->expiry->schedule_after_msec(VEIL_TBL_ENTRY_EXPIRY);
			veil_chatter(printDebugMessages,"[++RouteTable] Bucket %d for Interface |%s| \n",b, i->switchVIDString().c_str());
			return;
		}
	}
}


// return the gateway:g and nexthop:nh for bucket:b
// returns the default entry if isDefault = true else returns anyone of the entries at random
bool
VEILRouteTable::getRoute(VID* dst, uint8_t b, VID i, VID *nh, VID *g,bool isDefault)
{
	bool found = false;
	uint8_t j = 0;
	InnerRouteTable *rt = routes.get_pointer(i);
	Bucket *buck = rt->get_pointer(b);
	if (buck != NULL){
		if (isDefault){
			for (j = 0; j < MAX_GW_PER_BUCKET; j++){
				if (buck->buckets[j].isValid && buck->buckets[j].isDefault){
					memcpy(nh, &(buck->buckets[j].nexthop), 6);
					memcpy(g, &(buck->buckets[j].gateway), 6);
					return true;
				}
			}
			return false;
		}else{
			srand(time(NULL));
			uint8_t totalValidEntries = 0;
			uint8_t k = 0;

			// first count how many valid entries are there.
			for (k = 0; k < MAX_GW_PER_BUCKET; k++){
				if (buck->buckets[k].isValid){
					totalValidEntries++;
				}
			}

			//if no valid entries then exit.
			if (totalValidEntries == 0){return false;}

			// now pick one randomly
			uint8_t pickj = rand() % totalValidEntries;
			uint8_t r = 0;
			for (k = 0; k < MAX_GW_PER_BUCKET; k++){
				if (buck->buckets[k].isValid){
					if (r == pickj){
						memcpy(nh, &(buck->buckets[pickj].nexthop), 6);
						memcpy(g, &(buck->buckets[pickj].gateway), 6);
						return true;
					}
					r++;
				}
			}
		}
	}
	return found;
}

// return the default nexthop:nh for bucket:b for interface:i
bool
VEILRouteTable::getBucket(uint8_t b, VID* i, VID *nh)
{
	bool found = false;
	uint8_t j = 0;
	if (routes.find(*i) != routes.end()) {
		InnerRouteTable rt = routes.get(*i);
		if (rt.find(b) != rt.end()) {
			Bucket buck = rt.get(b);
			for (j = 0; j < MAX_GW_PER_BUCKET; j++){
				if (buck.buckets[j].isDefault && buck.buckets[j].isValid){
					break;
				}
			}
			if (j < MAX_GW_PER_BUCKET){
				memcpy(nh, &(buck.buckets[j].nexthop), VID_LEN);
				found = true;
			}
		}
	}
	return found;
}

void
VEILRouteTable::expire(Timer *t, void *data) 	
{
	TimerData *td = (TimerData *) data;
	uint8_t bucket = td->bucket;
	VID interface;
	memcpy(&interface, td->interface, VID_LEN);
	veil_chatter(true,"[++RouteTable] [Timer Expired] on Interface VID: |%s| for bucket %d \n",interface.switchVIDString().c_str(), bucket);
	delete(td->interface);
	InnerRouteTable* irt = td->routes->get_pointer(interface);
	Bucket* buck = irt->get_pointer(bucket);
	// Erase returns the number of elements deleted, so if it is 0, then it means that corresponding entry was not deleted.
	if (irt->find(bucket) == irt->end()){
		veil_chatter(true,"[++RouteTable][Delete ERROR!!][Timer Expired] on Interface VID: |%s| for bucket %d \n",interface.switchVIDString().c_str(), bucket);
	}else{
		for (uint8_t i = 0; i < MAX_GW_PER_BUCKET; i++){
			buck->buckets[i].isValid = false;
			buck->buckets[i].isDefault = false;
		}
	}
	t->clear();
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
	sa << "My VID" << "\t\t" << "Bucket" << '\t' <<"NextHop VID" << "\t"<< "Gateway VID" <<"\t"<<"IsDefault" <<"\t"<< "TTL" << '\n';
	for(iter1 = routes1.begin(); iter1; ++iter1){
		String myInterface = static_cast<VID>(iter1.key()).switchVIDString();
		InnerRouteTable rt = iter1.value();
		for(iter2 = rt.begin(); iter2; ++iter2){
			int bucket = iter2.key();		
			Bucket buck = iter2.value();
			for (uint8_t i =0; i< MAX_GW_PER_BUCKET;i++){
				if (buck.buckets[i].isValid){
					String nexthop = static_cast<VID>(buck.buckets[i].nexthop).switchVIDString();
					String gateway = static_cast<VID>(buck.buckets[i].gateway).switchVIDString();
					Timer *t = buck.expiry;
					String def = buck.buckets[i].isDefault?"Yes":"No";
					sa << myInterface << '\t' << bucket << '\t' << nexthop << '\t' << gateway << "\t"<<def<<"\t"<<t->expiry().sec() - time(NULL) << " sec\n";
				}
			}
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
