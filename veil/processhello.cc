#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include "click_veil.hh"
#include "processhello.hh"

//we will only get hello packets if we were someone's neighbor
//don't foresee any reason why packets should not be destroyed here

CLICK_DECLS

VEILProcessHello::VEILProcessHello () {}

VEILProcessHello::~VEILProcessHello () {}

int
VEILProcessHello::configure (
	Vector<String> &conf,
	ErrorHandler *errh)
{
	return cp_va_kparse(conf, this, errh,
		"MYVID", cpkM+cpkP, cpVid, &myVid,
		"NEIGHBORTABLE", cpkM+cpkP, cpElementCast, "VEILNeighborTable", &neighbor_table,
		cpEnd);
}

Packet* 
VEILProcessHello::simple_action(Packet* p)
{
	assert(p);

	click_ether *e = (click_ether*) p->data();

	VID nVid = VID((const unsigned char*)e->ether_shost);
	neighbor_table->updateEntry(&nVid, &myVid);

	p->kill();
	return NULL;	
}


CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessHello)

