//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
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

		//--------------------------
		WritablePacket* p = publish_access_info_packet(ip, hmac, hvd, printDebugMessages, class_name());
		if (p == NULL) {
			veil_chatter_new(true, class_name(), "[Error!] cannot make packet in publishaccessinfo");
			continue;
		}
		//--------------------------
		output(0).push(p);
	}
	myTimer.schedule_after_msec(HOST_ENTRY_EXPIRY/2.5);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILPublishAccessInfo)
