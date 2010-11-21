#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include "click_veil.hh"
#include "veil_packet.hh"
#include "generatehello.hh"
 
CLICK_DECLS

VEILGenerateHello::VEILGenerateHello () : myTimer(this) {}

VEILGenerateHello::~VEILGenerateHello () {}

int
VEILGenerateHello::configure (
	Vector<String> &conf,
	ErrorHandler *errh)
{
	int res = -1;
	printDebugMessages = true;
	res = cp_va_kparse(conf, this, errh,
                "MYVID", cpkP+cpkM, cpVid, &myVid,
                "PRINTDEBUG", 0, cpBool, &printDebugMessages,
		cpEnd);
	// initialize the myuid to myVid
	memcpy(&myuid, &myVid, 6);
	//TODO: pass initialization errors if any to errh
	return(res);
}

int
VEILGenerateHello::initialize (
	ErrorHandler *errh)
{
	myTimer.initialize(this);
	myTimer.schedule_now();

	return(0);
}

void
VEILGenerateHello::run_timer (
	Timer *timer) 
{
	assert(timer == &myTimer);

	int packet_length = sizeof(click_ether) + sizeof(veil_sub_header);
	WritablePacket *packet = Packet::make(packet_length);

        if (packet == 0) {
                veil_chatter_new(true, class_name(),"in %s: cannot make packet!", name().c_str());
                return;
        }

	memset(packet->data(), 0, packet->length());

	click_ether *e = (click_ether *) packet->data();
	packet->set_ether_header(e);
	memset(e->ether_dhost, 0xff, 6); // broadcast
	memcpy(e->ether_shost, myuid.data(), 6);
	//TODO: we don't have a shim header to identify VEIL pkts hence this temporary solution. find a better alternative. Instead of type, could we do with just first testing type and if invalid, looking for the right VEIL type in the 2B just after eth header?
	e->ether_type = htons(ETHERTYPE_VEIL);

	setSrcVID(packet, myVid);
	//Do not need to set the destination vid in this case.
	//setDstVID(packet,)
	setVEILType(packet, VEIL_HELLO);
	//No need to set the TTL on this packet
	//setVEILTTL(packet,...);

	// Send HELLO packets to neighbors
	output(0).push(packet);

	// The hello interval is 20 seconds.
	myTimer.schedule_after_msec(VEIL_HELLO_INTERVAL);
}

CLICK_ENDDECLS
 
EXPORT_ELEMENT(VEILGenerateHello)
