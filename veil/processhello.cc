#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include "click_veil.hh"
#include "processhello.hh"

CLICK_DECLS

VEILProcessHello::VEILProcessHello () {}

VEILProcessHello::~VEILProcessHello () {}

int
VEILProcessHello::configure (
	Vector<String> &conf,
	ErrorHandler *errh)
{
	return cp_va_kparse(conf, this, errh,
		"INTERFACE", cpkM+cpkP, cpString, &interface,
		"NEIGHBORTABLE", cpkM+cpkP, cpElementCast, "VEILNeighborTable", &neighbor_table,
		cpEnd);
}

Packet* 
VEILProcessHello::smaction(Packet* p)
{
	assert(p);

	//we got a valid eth pkt because the top level Classifier in the Click script thought so. this check is really redundant.
	click_ether *e = (click_ether*) p->data();
	if(ntohs(e->ether_type) != ETHERTYPE_VEIL)
	{ 
		click_chatter("%s: not a VEIL packet", name().c_str());
		return p;
	}

	veil_header *vhdr = (veil_header*) (p->data() + sizeof(click_ether));
	uint16_t ptype = ntohs(vhdr->packetType);
	if(ptype != VEIL_HELLO)
	{
		click_chatter("%s: packet type is %x and is not a hello pkt", name().c_str(), ptype);
		return p;
	}
	
	VID nVid = VID((const unsigned char*)e->ether_shost);
	neighbor_table->updateEntry(&nVid, interface);

	return p;	
}

void 
VEILProcessHello::push(int, Packet* p)
{
	smaction(p);
	//discard in the script
	output(0).push(p);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessHello)

