#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include "click_veil.hh"
#include "veil_packet.hh"
#include "generatehelloNew.hh"

CLICK_DECLS

VEILGenerateHelloNew::VEILGenerateHelloNew () : myTimer(this) {}

VEILGenerateHelloNew::~VEILGenerateHelloNew () {}

int
VEILGenerateHelloNew::configure (
	Vector<String> &conf,
	ErrorHandler *errh)
{
	printDebugMessages = true;

	return cp_va_kparse(conf, this, errh,
		"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
		"PRINTDEBUG", 0, cpBool, &printDebugMessages,
		cpEnd);
}

int
VEILGenerateHelloNew::initialize (
	ErrorHandler *errh)
{
	myTimer.initialize(this);
	myTimer.schedule_now();

	return(0);
}

void
VEILGenerateHelloNew::run_timer (
	Timer *timer)
{
	assert(timer == &myTimer);

	for (int i = 0; i < interfaces->numInterfaces(); i++){
		int packet_length = sizeof(click_ether) + sizeof(veil_sub_header);
		WritablePacket *packet = Packet::make(packet_length);

			if (packet == 0) {
					veil_chatter(true, "-o-o- [ GenerateHello ]in %s: cannot make packet!\n", name().c_str());
					return;
			}

		memset(packet->data(), 0, packet->length());

		click_ether *e = (click_ether *) packet->data();
		packet->set_ether_header(e);
		VID vid;
		VID zerovid;
		memset(&zerovid, 0, 6);
		EtherAddress macadd;
		if (interfaces->getMacAddr(i, &macadd) && interfaces->lookupIntEntry(i, &vid)){
			memset(e->ether_dhost, 0xff, 6); // broadcast
			memcpy(e->ether_shost, &macadd, 6);
			//TODO: we don't have a shim header to identify VEIL pkts hence this temporary solution. find a better alternative. Instead of type, could we do with just first testing type and if invalid, looking for the right VEIL type in the 2B just after eth header?
			e->ether_type = htons(ETHERTYPE_VEIL);

			if (interfaces->isvidset[i]){
				setSrcVID(packet, vid);
			}else{
				setSrcVID(packet, zerovid); // puts zeros if the vid assignment is not done yet.
			}
			//Do not need to set the destination vid in this case.
			//setDstVID(packet,)
			setVEILType(packet, VEIL_HELLO);
			//No need to set the TTL on this packet
			//setVEILTTL(packet,...);

			//Send HELLO packets to neighbors
			output(i).push(packet);
		}else{
			veil_chatter(true, "-o-o- [ GenerateHello ]in %s: cannot retrieve MAC/VID information for the interface '%d'.\n", name().c_str(),i);
			// print the error message.
		}
	}

	// The hello interval is 20 seconds.
	myTimer.schedule_after_msec(VEIL_HELLO_INTERVAL);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILGenerateHelloNew)
