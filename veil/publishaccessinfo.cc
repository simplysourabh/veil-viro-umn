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
	printDebugMessages = true;
	return cp_va_kparse(conf, this, errh,
			"HOSTTABLE", cpkM+cpkP, cpElementCast, "VEILHostTable", &hosts,
			"PRINTDEBUG", 0, cpBool, &printDebugMessages,
			cpEnd);
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

	const VEILHostTable::HostIPTable *hipt = hosts->get_host_iptable_handle();
	VEILHostTable::HostIPTable::const_iterator iter;	

	//hash of IP yields a unique value so we need to generate one pkt/host
	for(iter = hipt->begin(); iter; ++iter){
		IPAddress ip = iter.key();
		VID hvd = iter.value();
		EtherAddress hmac;
		if(!hosts->lookupVID(&hvd, &hmac)){
			veil_chatter_new(true, class_name(), "[Error!] Missing MAC for VID: %s and IP: %s", hvd.vid_string().c_str(), ip.s().c_str());
			continue;
		}

		int packet_length = sizeof(click_ether) + sizeof(veil_sub_header) + sizeof(veil_payload_map_publish);
		WritablePacket *p = Packet::make(packet_length);

		if (p == 0) {
			veil_chatter_new(true, class_name(), "[Error!] cannot make packet in publishaccessinfo");
			return;
		}

		memset(p->data(), 0, p->length());

		click_ether *e = (click_ether *) p->data();
		p->set_ether_header(e);


		VID accessvid = calculate_access_switch(&ip);
		memcpy(e->ether_dhost, &accessvid, 6); 
		bzero(e->ether_shost,6);
		memcpy(e->ether_shost, &hvd, 4);
		e->ether_type = htons(ETHERTYPE_VEIL);

		veil_sub_header *vheader = (veil_sub_header*) (e + 1);
		vheader->veil_type = htons(VEIL_MAP_PUBLISH);
		vheader->ttl = MAX_TTL;
		memcpy(&vheader->dvid, &accessvid, 6);
		bzero(&vheader->svid, 6);
		memcpy(&vheader->svid, &hvd, 4);

		veil_payload_map_publish * payload_publish = (veil_payload_map_publish * )(vheader+1);
		memcpy(&payload_publish->ip, &ip, 4);
		memcpy(&payload_publish->mac, &hmac, 6);
		memcpy(&payload_publish->vid, &hvd, 6);

		SET_REROUTE_ANNO(p, 'r');

		output(0).push(p);
		veil_chatter_new(printDebugMessages, class_name(),"[Access Info Publish] HOST IP: %s  VID: %s  MAC: %s AccessSwitchVID: %s", ip.s().c_str(),  hvd.vid_string().c_str(),hmac.s().c_str(),accessvid.switchVIDString().c_str() );
	}
	myTimer.schedule_after_msec(VEIL_PUBLISH_INTERVAL);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILPublishAccessInfo)
