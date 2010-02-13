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
//		"VID", cpkP+cpkM, cpVid, &myVid,
//		"HOSTTABLE", cpkM+cpkP, cpElementCast, "VEILHostTable", &hosts,
		"INTERFACE", cpkM+cpkP, cpString, &interface,
		"NEIGHBORTABLE", cpkM+cpkP, cpElementCast, "VEILNeighborTable", &neighbor_table,
		cpEnd);
}

Packet*
VEILProcessHello::simple_action (Packet* p)
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

/*
	VID *new_vid = (VID*) (pkt->data() + sizeof(click_ether) + sizeof(veil_header));
	click_chatter("new vid is %x %x %x %x %x %x", new_vid->sdata()[0], new_vid->sdata()[1], new_vid->sdata()[2],
		new_vid->sdata()[3], new_vid->sdata()[4], new_vid->sdata()[5]);

       	//has_mac_header check 
	const click_ether *macHeader = (const click_ether *) pkt->data();
	click_chatter("neighbor VID is %x %x %x %x %x %x", p->ether_shost[0], macHeader->ether_shost[1], macHeader->ether_shost[2],
		macHeader->ether_shost[3], macHeader->ether_shost[4], macHeader->ether_shost[5]);
*/

	//unsigned char vid[6];
	//memcpy(vid, (void*) e->ether_shost, 6);
	//TODO: is nVid local or are we actually allocating memory?
	//VID nVid = VID((const unsigned char*) vid);
	VID nVid = VID((const unsigned char*)e->ether_shost);
	click_chatter("%s: address of allocated VID is %p", name().c_str(), &nVid);

/*
        EtherAddress tempVid = EtherAddress(p->ether_shost);
	click_chatter("%s: neighbor VID is %s", name().c_str(), tempEther.unparse().c_str());
	neighbor_table->updateEntry(new_vid, &tempEther, VEIL_HELLO_INTERVAL);
*/

	//we will have a unique VEILProcessHello for each interface
	//so usage of myVid is correct
	neighbor_table->updateEntry(&nVid, interface);
	neighbor_table->printNeighborTable();

	//discard in the script
	output(0).push(p);
}


CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessHello)

