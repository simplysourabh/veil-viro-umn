#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include "click_veil.hh"
#include "processhelloNew.hh"

//we will only get hello packets if we were someone's neighbor
//don't foresee any reason why packets should not be destroyed here

CLICK_DECLS

VEILProcessHelloNew::VEILProcessHelloNew () {}

VEILProcessHelloNew::~VEILProcessHelloNew () {}

int
VEILProcessHelloNew::configure (
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
VEILProcessHelloNew::smaction(Packet* p)
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

	// see if the packet type is "HELLO_REPLY" then kill the packet.
	// else send the HELLO_REPLY packet back.
	veil_sub_header *vheader = (veil_sub_header *) (e+1);
	uint16_t vtype = ntohs(vheader->veil_type);
	veil_chatter_new(printDebugMessages, class_name(), "Received a packet of type %d from %s,%s to %s,%s", vtype, nmac.s().c_str(), nvid.switchVIDString().c_str(), mymac.s().c_str(), myvid.switchVIDString().c_str());
	if (vtype == VEIL_HELLO){
		// change the packet type to VEIL_HELLO_REPLY
		vheader->veil_type = htons(VEIL_HELLO_REPLY);
		//change the src mac to dst mac
		memcpy(e->ether_dhost, &nmac, 6);
		// change the src vid to dst vid
		memcpy(e->ether_shost, &mymac, 6);
		// put my mac as src mac
		memcpy(&vheader->dvid, &nvid, 6);
		// put my vid as src vid
		memcpy(&vheader->svid, &myvid, 6);

		output(myport).push(p);
	}
	else{
		p->kill();
		return NULL;	
	}
	return NULL;
}

void
VEILProcessHelloNew::push(int port, Packet* p){
	p = smaction(p);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessHelloNew)

