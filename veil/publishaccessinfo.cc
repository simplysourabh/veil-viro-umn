#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include "publishaccessinfo.hh"

//there is one of this element per switch

CLICK_DECLS

VEILPublishAccessInfo::VEILPublishAccessInfo () : myTimer(this) {}

VEILPublishAccessInfo::~VEILPublishAccessInfo () {}

int
VEILPublishAccessInfo::configure (
	Vector<String> &conf,
	ErrorHandler *errh)
{
	return cp_va_kparse(conf, this, errh,
                "HOSTTABLE", cpkM+cpkP, cpElementCast, "VEILHostTable", &hosts,	cpEnd);
}

int
VEILPublishAccessInfo::initialize (
	ErrorHandler *errh)
{
	myTimer.initialize(this);
	myTimer.schedule_now();
	return(0);
}

void
VEILPublishAccessInfo::run_timer (
	Timer *timer) 
{
	assert(timer == &myTimer);

	const HostIPTable *hipt = hosts->get_host_iptable_handle();
	HostIPTable::const_iterator iter;	

	//hash of IP yields a unique value so we need to generate one pkt/host
	for(iter = hipt->begin(); iter; ++iter){
		int packet_length = sizeof(click_ether) + sizeof(veil_header) + sizeof(access_info);
		WritablePacket *p = Packet::make(packet_length);

        	if (p == 0) {
                	click_chatter( "cannot make packet in publishaccessinfo");
                	return;
        	}

		memset(p->data(), 0, p->length());

		click_ether *e = (click_ether *) p->data();
		p->set_ether_header(e);
		
		IPAddress ip = iter.key();
		VID accessvid = calculate_access_switch(&ip);
		memcpy(e->ether_dhost, &accessvid, 6); 
		e->ether_type = htons(ETHERTYPE_VEIL);

		veil_header *vheader = (veil_header*) (e + 1);
		vheader->packetType = htons(VEIL_PUBLISH);

		access_info *af = (access_info*) (vheader + 1);
		//TODO: does this work?
		memcpy(af->ip.data(), &ip, 6); 
		VID hvid = iter.value();
		memcpy(af->vid.data(), &hvid, 6);	

		//source vid is the interface to which the host is connected
		memset(e->ether_shost, 0, 6);
		memcpy(e->ether_shost, &hvid, 4);

		SET_REROUTE_ANNO(p, 'r');
	
		output(0).push(p);
	}
	myTimer.schedule_after_msec(VEIL_PUBLISH_INTERVAL);
}

CLICK_ENDDECLS
 
EXPORT_ELEMENT(VEILPublishAccessInfo)
