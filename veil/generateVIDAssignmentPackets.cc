#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/vector.hh>
#include "click_veil.hh"
#include "veil_packet.hh"
#include "generatehelloNew.hh"
#include "generateVCCSTAdSub.hh"
#include "generateVIDAssignmentPackets.hh"

/*
 * EXPLAIN THE PURPOSE OF THIS ELEMENT.
 * It is used to generate the Advertisement/Subscription
 * Packets to construct the spanning tree to reach the
 * VCC (veil central controller)
 */

CLICK_DECLS

VEILGenerateVIDAssignmentPackets::VEILGenerateVIDAssignmentPackets () : myTimer(this) {}

VEILGenerateVIDAssignmentPackets::~VEILGenerateVIDAssignmentPackets () {}

int
VEILGenerateVIDAssignmentPackets::configure (
		Vector<String> &conf,
		ErrorHandler *errh)
{
	printDebugMessages = true;
	return cp_va_kparse(conf, this, errh,
			"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
			"NEIGHBORTABLE", cpkM+cpkP, cpElementCast, "VEILNeighborTable", &neighbors,
			"SPANNINGTREESTATE", cpkM+cpkP, cpElementCast, "VEILSpanningTreeState", &ststate,
			"NETWORKTOPO", cpkM+cpkP, cpElementCast, "VEILNetworkTopoVIDAssignment", &topo,
			"PRINTDEBUG", 0, cpBool, &printDebugMessages,
			cpEnd);
}

int
VEILGenerateVIDAssignmentPackets::initialize (
		ErrorHandler *errh)
{
	myTimer.initialize(this);
	myTimer.schedule_now();
	veil_chatter(printDebugMessages, "-o [VID_Dispatcher] in initialize: done!\n");
	return(0);
}

void
VEILGenerateVIDAssignmentPackets::run_timer (
		Timer *timer)
{
	assert(timer == &myTimer);
	veil_chatter(printDebugMessages, "-o [VID_Dispatcher] in run_timer: Preparing to dispatch VID assignments!\n");
	/* There are a number of things we do inside this.
	 * 1. Compute the physically topology to virtual binary tree embedding.
	 * for now, the logic is very simple and dumb. Every time, run-timer is called,
	 * it calls a function inside the topo element to generate the vids.
	 * 2. next we check if ether2vid mapping file has any mappings or not. if it does
	 * then we create 1 or more than 1 outgoing packets to dispatch all the mappings.
	 *
	 * also we assume that length of each vid packet is limited to certain bytes.
	 */
	bool vid_changed = topo->performVIDAssignment();
	dispatch_vid(); // dispatch the vid assignments to nodes.
	myTimer.schedule_after_msec(VEIL_VID_BROADCAST_TIME);
}

void
VEILGenerateVIDAssignmentPackets::dispatch_vid(){
	VEILNetworkTopoVIDAssignment::EtherToVID::const_iterator iter;
	for (iter = topo->ether2vid.begin(); iter; iter++){
		EtherAddress dstmac = iter.key();
		VID dstvid = iter.value();
		dispatch_one_vid_mac_pair(dstmac, dstvid);
	}
}

void
VEILGenerateVIDAssignmentPackets::dispatch_one_vid_mac_pair(EtherAddress dstmac, VID dstvid){
	// now form a packet to dispatch the mapping.
	int packet_length = sizeof(click_ether) + sizeof(veil_sub_header) + sizeof(veil_payload_svid_mappings);
	WritablePacket *packet = Packet::make(packet_length);
	if (packet == 0) {
		veil_chatter(true, "-o [VID_Dispatcher] in %s: cannot make packet for 'VID Assignment' for % with vid %s!\n", dstmac.s().c_str(), dstvid.vid_string().c_str());
		return;
	}
	memset(packet->data(), 0, packet->length());
	click_ether *e = (click_ether *) packet-> data();
	packet->set_ether_header(e);
	e->ether_type = htons(ETHERTYPE_VEIL);

	setVEILType(packet, VEIL_SWITCH_VID_FROM_VCC);

	veil_sub_header *veil_sub = (veil_sub_header *) (e+1);
	memcpy(&veil_sub->svid, &topo->vccmac, 6);
	memcpy(&veil_sub->dvid, &dstmac, 6);
	veil_payload_svid_mappings *veil_payload = (veil_payload_svid_mappings *) (veil_sub+1);
	memcpy(&veil_payload->vid, &dstvid, 6);

	// do I have dstmac in the children ?
	if (ststate->forwardingTableFromVCC.get_pointer(dstmac) != NULL){
		memcpy(e->ether_dhost, &dstmac,6);
		// check on which interface this child is directly connected?

		if (neighbors->neighbors.get_pointer(dstmac) != NULL){
			EtherAddress mymac = neighbors->neighbors.get(dstmac).myMac;
			memcpy(e->ether_shost, &mymac,6);
			int myinterfaceid;

			if (interfaces->etheraddToInterfaceIndex.get_pointer(mymac) != NULL){
				myinterfaceid = interfaces->etheraddToInterfaceIndex.get(mymac);
				output(myinterfaceid).push(packet);
			}else{
				//error
				veil_chatter(true, "-o [VID_Dispatcher] Error: Couldn't retrieve the interface id for %s! \n", mymac.s().c_str());
			}
		}else{
			// error!
			veil_chatter(true, "-o [VID_Dispatcher] Error: Couldn't retrieve the my local interface connected to %s! \n", dstmac.s().c_str());
		}

	}
	// no it is not one of my direct neighbors. therefore, will have to forward it
	// to all the children nodes.
	else{
		VEILSpanningTreeState::ForwardingTableFromVCC::const_iterator it;
		for (it = ststate->forwardingTableFromVCC.begin(); it; it++){
			EtherAddress childmac = it.key();
			if (it.value().VccMac == topo->vccmac){
				// create a new copy of the packet.
				WritablePacket *pkt = Packet::make(packet_length);
				memcpy(pkt->data(), packet->data(), packet_length);
				click_ether *e1 = (click_ether *) pkt-> data();

				memcpy(e1->ether_dhost, &childmac,6);
				// check on which interface this child is directly connected?

				if (neighbors->neighbors.get_pointer(childmac) != NULL){
					EtherAddress mymac = neighbors->neighbors.get(childmac).myMac;
					memcpy(e1->ether_shost, &mymac,6);
					int myinterfaceid;
					if (interfaces->etheraddToInterfaceIndex.get_pointer(mymac) != NULL){
						myinterfaceid = interfaces->etheraddToInterfaceIndex.get(mymac);
						output(myinterfaceid).push(pkt);
					}else{
						//error
						veil_chatter(true, "-o [VID_Dispatcher] Error: Couldn't retrieve the interface id for %s! \n", mymac.s().c_str());
					}
				}else{
					// error!
					veil_chatter(true, "-o [VID_Dispatcher] Error: Couldn't retrieve the my local interface connected to %s! \n", childmac.s().c_str());
				}
			}
		}
		packet->kill();
		packet = NULL;
	}
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILGenerateVIDAssignmentPackets)
