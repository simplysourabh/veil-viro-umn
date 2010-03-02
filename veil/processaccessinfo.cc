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
	return cp_va_kparse(conf, this, errh,
		"MAPPINGTABLE", cpkM+cpkP, cpElementCast, "VEILMappingTable", &map,
		"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
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

	//we should process this packet only if it is destined to one of 
	//our interfaces
	VID dvid = VID(e->ether_dhost);
	int interface;
	if(interfaces->lookupVidEntry(&dvid, &interface))
	{
		veil_header *vhdr = (veil_header*) (p->data() + sizeof(click_ether));
		access_info *ai = (access_info*)  (p->data() + sizeof(click_ether) + sizeof(veil_header));
	
		//TODO: does this work?
		IPAddress ip = ai->ip;	
		VID vid = ai->vid;
		map->updateEntry(&ip, &vid, &myVid);

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
	output(0).push(q);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessAccessInfo)
