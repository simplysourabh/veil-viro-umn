#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include "processaccessinfo.hh"

CLICK_DECLS

VEILProcessAccessInfo::VEILProcessAccessInfo () {}

VEILProcessAccessInfo::~VEILProcessAccessInfo () {}

int
VEILProcessAccessInfo::configure (
	Vector<String> &conf,
	ErrorHandler *errh)
{
	printDebugMessages = true;
	return cp_va_kparse(conf, this, errh,
		"MAPPINGTABLE", cpkM+cpkP, cpElementCast, "VEILMappingTable", &map,
		"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
		"PRINTDEBUG", 0, cpBool, &printDebugMessages,
		cpEnd);
}

Packet* 
VEILProcessAccessInfo::smaction(Packet* p)
{
	assert(p);

	int myport = PORT_ANNO(p);
	VID myVid;
	interfaces->lookupIntEntry(myport, &myVid);

	click_ether *e = (click_ether*) p->data();

	VID dvid = VID(e->ether_dhost);

	int interface;
	if (!interfaces->lookupVidEntry(&dvid, &interface)){
		// Should I have received this packet? see if ether_dhost is me or not?
		p->kill();
		return NULL;
	}

	veil_sub_header* vheader = (veil_sub_header *) (e+1);
	memcpy(&dvid, &vheader->dvid, 6);

	// Am I the final destination?
	if(interfaces->lookupVidEntry(&dvid, &interface))
	{
		veil_payload_map_publish * payload_publish = (veil_payload_map_publish*)(vheader+1);
		IPAddress ip = payload_publish->ip;
		VID vid = payload_publish->vid;
		EtherAddress mac;
		memcpy(&mac,&payload_publish->mac,6);
		map->updateEntry(&ip, &vid, &myVid, &mac);
		veil_chatter_new(printDebugMessages, class_name(),"[STORE MAPPING] HOST IP: %s  VID: %s  AccessSwitchVID: %s", ip.s().c_str(),  vid.vid_string().c_str(),myVid.switchVIDString().c_str()) ;
		
		p->kill();	
		return NULL;
	}
	
	//if the packet was not destined to us, we need to reroute it
	SET_REROUTE_ANNO(p, 'r');
	return p;
}

void 
VEILProcessAccessInfo::push(int, Packet* p)
{
	Packet *q = smaction(p);
	if(q != NULL)
		output(0).push(q);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessAccessInfo)

