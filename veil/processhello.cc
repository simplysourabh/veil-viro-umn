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
	printDebugMessages = true;
	return cp_va_kparse(conf, this, errh,
		"NEIGHBORTABLE", cpkM+cpkP, cpElementCast, "VEILNeighborTable", &neighbor_table,
		"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
		"PRINTDEBUG", 0, cpBool, &printDebugMessages,
		cpEnd);
}

Packet* 
VEILProcessHello::simple_action(Packet* p)
{
	assert(p);

	int myport = PORT_ANNO(p);
	VID myvid;
	interfaces->lookupIntEntry(myport, &myvid);
	EtherAddress mymac;
	interfaces->getMacAddr(myport, &mymac);

	click_ether *e = (click_ether*) p->data();
	EtherAddress nmac;
	memcpy(&nmac, e->ether_shost,6);

	VID nvid = getSrcVID(p);
	neighbor_table->updateEntry(&nmac, &nvid, &mymac, &myvid);

	p->kill();
	return NULL;	
}


CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessHello)

