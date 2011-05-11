//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
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
		return errh->error(" [Error] interface VID is not in expected format");
	String bucket_str = cp_shift_spacevec(s);
	if(!cp_integer(bucket_str, &bucket))
		return errh->error(" [Error] invalid integer");
	String nhvid_str = cp_shift_spacevec(s);
	if(!cp_vid(nhvid_str, &nhvid))
		return errh->error(" [Error] nexthop VID is not in expected format");
	String gvid_str = cp_shift_spacevec(s);
	if(!cp_vid(gvid_str, &gvid))
		return errh->error(" [Error] gateway VID is not in expected format");
	updateEntry(&ivid, bucket, &nhvid, &gvid);
	return 0;
}

int
VEILRouteTable::configure(Vector<String> &conf, ErrorHandler *errh)
{	
	veil_chatter_new(true,class_name(),"[FixME] Its mandatory to have 'PRINTDEBUG value' (here value = true/false) at the end of the configuration string!");
	veil_chatter_new(true,class_name(),"[FixME] Updating default routing entry for a bucket purges all the other routing entries, also default entry is always added in the beginning of the list of routing entries.");
	int res = 0;
	int i = 0;
	for (i = 0; i < conf.size()-2; i++) {
		res = cp_viro_route(conf[i], errh);
	}
	Vector<String> conf1;
	conf1.push_back(conf[i]);
	i++;
	conf1.push_back(conf[i]);
	res = cp_va_kparse(conf1, this, errh,
			"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
			"PRINTDEBUG", 0, cpBool, &printDebugMessages,
			cpEnd);

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
	if (interfaces->interfaces.get_pointer(*i) == NULL){
		veil_chatter_new(printDebugMessages, class_name(), "in updateEntry | Interface with VID %s is not available anymore!", i->vid_string().c_str());
		return;
	}
	int int_id = interfaces->interfaces.get(*i);

	veil_chatter_new(printDebugMessages, class_name(),"updateEntry | For my vid %s at interface %d bucket %d nh %s gw %s", i->switchVIDString().c_str(), int_id, b, nh->switchVIDString().c_str(), g->switchVIDString().c_str());
	InnerRouteTable *rt; uint8_t j = 0;
	if(routes.find(int_id) != routes.end()){
		// yes we have a innerroutetable for interface vid = i
		// get the pointer to the table.
		rt = routes.get_pointer(int_id);
	} else {
		// No we dont have the innerroutetable for interface vid = i
		// create new innerroutetable for the interface vid = i, and 
		// add it to the main routing table.
		rt = new HashTable<uint8_t, Bucket>::HashTable();
		routes.set(int_id,*rt);
		rt = routes.get_pointer(int_id);
	}

	// Now see if we have any mapping for the bucket:b in interface:i
	Bucket *buck;
	buck = rt->get_pointer(b);

	// there is no mapping already, so create one and add it.
	if (buck == NULL){
		Bucket tempbucket;
		rt->set(b,tempbucket);
		buck = rt->get_pointer(b);
		//set up the timer
		TimerData *tdata = new TimerData();
		tdata->bucket = b;
		tdata->interface_id = int_id;
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
		veil_chatter_new(printDebugMessages, class_name()," Updated Default and \"purged\" all the other Bucket %d for Interface |%s| ",b, i->switchVIDString().c_str());
		for (j = 1; j < MAX_GW_PER_BUCKET; j++){
			buck->buckets[j].isValid = false;
		}
		//printf("in VEILRouteTable::updateEntry done1\n");
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
			veil_chatter_new(printDebugMessages, class_name()," Bucket %d for Interface |%s| ",b, i->switchVIDString().c_str());
			return;
		}
	}
}


// return the gateway:g and nexthop:nh for bucket:b
// returns the default entry if isDefault = true else returns anyone of the entries at random
bool
VEILRouteTable::getRoute(VID* dst, VID myinterface, VID *nh, VID *g,bool isDefault)
{
	if (interfaces->interfaces.get_pointer(myinterface) == NULL){
		veil_chatter_new(true, class_name(), "in getRoute | Interface with VID %s is not available anymore!", myinterface.vid_string().c_str());
		return false;
	}
	int int_id = interfaces->interfaces.get(myinterface);

	//printf("VEILRouteTable::getRoute 1\n");
	bool found = false;
	uint8_t j = 0;
	uint8_t b = myinterface.logical_distance(dst);
	InnerRouteTable *rt = routes.get_pointer(int_id);
	//printf("VEILRouteTable::getRoute myinterface %s dst %s\n", myinterface.vid_string().c_str(), dst->vid_string().c_str());
	if (rt == NULL){
		veil_chatter_new(true, class_name(),"VEILRouteTable::getRoute: rt is NULL!");
		return false;
	}

	if(b <= 16){
		memcpy(nh, &myinterface,6);
		veil_chatter_new(printDebugMessages, class_name(), "getRoute | dst %s is me %s, and logical_distance is %d. Nexthop %s",dst->vid_string().c_str(), myinterface.switchVIDString().c_str(), b, nh->switchVIDString());
		return true;
	}
	//printf("VEILRouteTable::getRoute 2\n");
	Bucket *buck = rt->get_pointer(b);
	//printf("VEILRouteTable::getRoute 3\n");
	if (buck != NULL){
		//printf("VEILRouteTable::getRoute 4\n");
		if (isDefault){
			//printf("VEILRouteTable::getRoute 5\n");
			for (j = 0; j < MAX_GW_PER_BUCKET; j++){
				//printf("VEILRouteTable::getRoute 6\n");
				if (buck->buckets[j].isValid && buck->buckets[j].isDefault){
					//printf("VEILRouteTable::getRoute 7\n");
					memcpy(nh, &(buck->buckets[j].nexthop), 6);
					memcpy(g, &(buck->buckets[j].gateway), 6);
					return true;
				}
			}
			return false;
		}else{
			//srand(time(NULL));
			//printf("VEILRouteTable::getRoute 8\n");
			if(entrytouse.get_pointer(b) == NULL){
				entrytouse[b] = 0;
			}
			uint8_t totalValidEntries = 0;
			uint8_t k = 0;

			// first count how many valid entries are there.
			for (k = 0; k < MAX_GW_PER_BUCKET; k++){
				//printf("VEILRouteTable::getRoute 9\n");
				if (buck->buckets[k].isValid){
					//printf("VEILRouteTable::getRoute 10\n");
					totalValidEntries++;
				}
			}

			//if no valid entries then exit.
			if (totalValidEntries == 0){return false;}

			// now pick one randomly
			uint8_t pickj = (entrytouse[b]+1) % totalValidEntries;
			entrytouse[b] = pickj;
			uint8_t r = 0;
			for (k = 0; k < MAX_GW_PER_BUCKET; k++){
				//printf("VEILRouteTable::getRoute 11\n");
				if (buck->buckets[k].isValid){
					//printf("VEILRouteTable::getRoute 12\n");
					if (r == pickj){
						//printf("VEILRouteTable::getRoute 13\n");
						memcpy(nh, &(buck->buckets[pickj].nexthop), 6);
						memcpy(g, &(buck->buckets[pickj].gateway), 6);
						veil_chatter_new(printDebugMessages,class_name(), "%d options to reach dest %s. Chose %dth via nh %s, gw %s", totalValidEntries, dst->vid_string().c_str(), pickj, buck->buckets[pickj].nexthop.vid_string().c_str(),buck->buckets[pickj].gateway.vid_string().c_str() );
						return true;
					}
					r++;
				}
			}
		}
	}
	//printf("VEILRouteTable::getRoute 14\n");
	return found;
}

// return the default nexthop:nh for bucket:b for interface:i
bool
VEILRouteTable::getBucket(uint8_t b, VID* i, VID *nh)
{
	if (interfaces->interfaces.get_pointer(*i) == NULL){
		veil_chatter_new(printDebugMessages, class_name(), "Interface with VID %s is not available anymore!", i->switchVIDString().c_str());
		return false;
	}
	int int_id = interfaces->interfaces.get(*i);
	bool found = false;
	uint8_t j = 0;
	if (routes.find(int_id) != routes.end()) {
		InnerRouteTable rt = routes.get(int_id);
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
	int interfaceid = td->interface_id;
	veil_chatter_new(true,"VEILRouteTable", " [Timer Expired] on Interface %d: for bucket %d ",interfaceid, bucket);
	InnerRouteTable* irt = td->routes->get_pointer(interfaceid);
	Bucket* buck = irt->get_pointer(bucket);
	// Erase returns the number of elements deleted, so if it is 0, then it means that corresponding entry was not deleted.
	if (irt->find(bucket) == irt->end()){
		veil_chatter_new(true, "VEILRouteTable" ,"[Delete ERROR!!][Timer Expired] on Interface %d for bucket %d ",interfaceid, bucket);
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
	sa << "My VID" << "\t\t" << "Bucket" << '\t' <<"NextHop VID" << "\t"<< "Gateway VID" <<"\t"<<"IsDefault" <<" "<< "TTL" << '\n';
	for(iter1 = routes1.begin(); iter1; ++iter1){
		String myInterface = static_cast<VID>(rt1->interfaces->rinterfaces[iter1.key()]).switchVIDString();
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
