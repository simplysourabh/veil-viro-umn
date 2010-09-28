#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include "click_veil.hh"
#include "processphello.hh"

//we will only get hello packets if we were someone's neighbor
//don't foresee any reason why packets should not be destroyed here

CLICK_DECLS

VEILProcessPHello::VEILProcessPHello () {}

VEILProcessPHello::~VEILProcessPHello () {}

int
VEILProcessPHello::configure (
	Vector<String> &conf,
	ErrorHandler *errh)
{
	printDebugMessages = true;
	return cp_va_kparse(conf, this, errh,
		"NEIGHBORTABLE", cpkM+cpkP, cpElementCast, "VEILNeighborTable", &neighbor_table,
		"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
		"PRINTDEBUG", 0, cpBool, &printDebugMessages,
		cpEnd);

	neighbor_table->enableUpdatedTopologyWriting("mylocal-topo", true);
	// this allows to write the topology whenever there is a change
}

Packet* 
VEILProcessPHello::simple_action(Packet* p)
{
	assert(p);

	int myport = PORT_ANNO(p);
	VID myVid;
	interfaces->lookupIntEntry(myport, &myVid);		

	click_ether *e = (click_ether*) p->data();

	VID nVid = getSrcVID(p);
	bool isNew = neighbor_table->updateEntry(&nVid, &myVid);

	p->kill();
	return NULL;	
}


CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessPHello)

