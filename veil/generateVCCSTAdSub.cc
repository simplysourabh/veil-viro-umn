#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/vector.hh>
#include "click_veil.hh"
#include "veil_packet.hh"
#include "generatehelloNew.hh"
#include "generateVCCSTAdSub.hh"


/*
 * EXPLAIN THE PURPOSE OF THIS ELEMENT.
 * It is used to generate the Advertisement/Subscription
 * Packets to construct the spanning tree to reach the
 * VCC (veil central controller)
 */

CLICK_DECLS

VEILGenerateVCCSTAdSub::VEILGenerateVCCSTAdSub () : myTimer(this) {}

VEILGenerateVCCSTAdSub::~VEILGenerateVCCSTAdSub () {}

int
VEILGenerateVCCSTAdSub::configure (
	Vector<String> &conf,
	ErrorHandler *errh)
{
	printDebugMessages = true;
	click_chatter("[VCC_AD_SUB_LTOPO_GENERATOR] [FixME] Its mandatory to have 'PRINTDEBUG value' (here value = true/false) at the end of the configuration string!\n");
	return cp_va_kparse(conf, this, errh,
		"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
		"NEIGHBORTABLE", cpkM+cpkP, cpElementCast, "VEILNeighborTable", &neighbors,
		"SPANNINGTREESTATE", cpkM+cpkP, cpElementCast, "VEILSpanningTreeState", &ststate,
		"PRINTDEBUG", 0, cpBool, &printDebugMessages,
		cpEnd);
}

int
VEILGenerateVCCSTAdSub::initialize (
	ErrorHandler *errh)
{
	myTimer.initialize(this);
	myTimer.schedule_now();
	veil_chatter(printDebugMessages, "-o [VCC_AD_SUB_LTOPO_GENERATOR] in initialize: done!\n");
	return(0);
}

void
VEILGenerateVCCSTAdSub::run_timer (
	Timer *timer)
{
	assert(timer == &myTimer);
	veil_chatter(printDebugMessages, "-o [VCC_AD_SUB_LTOPO_GENERATOR] in run_timer: processing the timer!\n");
	/* There are a number of things we do inside this.
	 * 1. see if a path to reach VCC. If we do then publish it to
	 * all my physical neighbors.
	 * 2.
	 */
	// see if we have an entry for a path to reach VCC?
	// if we don't then exit.
	if (ststate->forwardingTableToVCC.empty()) {
		veil_chatter(printDebugMessages, "-o [VCC_AD_SUB_LTOPO_GENERATOR] in run_timer: no path to vcc, rescheduling the timer after %d seconds!\n", VEIL_SPANNING_TREE_COST_BROADCAST);
		myTimer.schedule_after_msec(VEIL_SPANNING_TREE_COST_BROADCAST);
		return;
	}

	// construct and send the vcc advertisement and subscription packets.
	veil_chatter(printDebugMessages, "-o [VCC_AD_SUB_LTOPO_GENERATOR] in run_timer: constructing and sending the vcc advertisement and subscription packets!\n");
	sendVCCAdandSubPackets();

	// now send the neighbor information to VCC.
	veil_chatter(printDebugMessages, "-o [VCC_AD_SUB_LTOPO_GENERATOR] in run_timer: sending the neighbor information to VCC!\n");
	sendNeighborInfotoVCC();

	// The hello interval is 20 seconds.
	myTimer.schedule_after_msec(VEIL_SPANNING_TREE_COST_BROADCAST);
}

void
VEILGenerateVCCSTAdSub::sendVCCAdandSubPackets(){
	VEILSpanningTreeState::ForwardingTableToVCC::iterator iter;
	VID zerovid;
	memset(&zerovid, 0, 6);
	EtherAddress vccmac;
	int ninterfaces = interfaces->numInterfaces();

	for (iter = ststate->forwardingTableToVCC.begin(); iter; ++iter){
		vccmac = iter.key();
		VEILSpanningTreeState::ParentEntry pe = iter.value();
		veil_chatter(printDebugMessages, "-o [VCC_AD_SUB_LTOPO_GENERATOR] in sendVCCAdandSubPackets: For vcc %s\n", vccmac.s().c_str());
		// else for each VCC announce its cost to all the neighbors.
		for (int i = 0; i < ninterfaces; i++){
			EtherAddress ethsrc;
			ethsrc = interfaces->interfaceIndexToEtherAddr.get(i);
			// create the advertisement packet.
			int packet_length = sizeof(click_ether) + sizeof(veil_sub_header) + sizeof(veil_payload_vcc_sp_tree_ad);
			WritablePacket *packet = Packet::make(packet_length);
			if (packet == 0) {
					veil_chatter(true, "-o [VCC_AD_SUB_LTOPO_GENERATOR] in %s: cannot make packet for 'advertisement'!\n", name().c_str());
					return;
			}
			memset(packet->data(), 0, packet->length());
			click_ether *e = (click_ether *) packet-> data();
			packet->set_ether_header(e);
			memset(e->ether_dhost, 0xff,6);
			memcpy(e->ether_shost, &ethsrc,6);

			e->ether_type = htons(ETHERTYPE_VEIL);

			setSrcVID(packet,zerovid);
			setDstVID(packet,zerovid);
			setVEILType(packet, VEIL_VCC_SPANNING_TREE_AD);

			veil_sub_header *veil_sub = (veil_sub_header *) (e+1);
			veil_payload_vcc_sp_tree_ad *veil_payload = (veil_payload_vcc_sp_tree_ad *) (veil_sub+1);

			memcpy(&veil_payload->vccmac, &vccmac, 6);
			veil_payload->cost = htons(pe.hopsToVcc);
			veil_chatter(printDebugMessages, "-o [VCC_AD_SUB_LTOPO_GENERATOR] in sendVCCAdandSubPackets: Sending Ad packet from %s to ff:ff:...:ff at interface %d!\n",ethsrc.s().c_str(),i);
			output(i).push(packet);
		}

		// send the subscription packets to the parent neighbor.
		int packet_len = sizeof(click_ether) + sizeof(veil_sub_header) + sizeof(veil_payload_vcc_sp_tree_sub);
		WritablePacket *packet = Packet::make(packet_len);
		if (packet == 0) {
				veil_chatter(true, "-o [VCC_AD_SUB_LTOPO_GENERATOR] in %s: cannot make packet for 'subscription' !\n", name().c_str());
				return;
		}
		memset(packet->data(), 0, packet->length());
		click_ether *e = (click_ether *) packet -> data();
		packet->set_ether_header(e);

		// find out which physical interface is directly connected to pe.ParentMac.
		VEILNeighborTable::NeighborTableEntry* nte;
		if (neighbors->lookupEntry(pe.ParentMac, &nte)){

			memcpy(e->ether_shost, &nte->myMac, 6);
			memcpy(e->ether_dhost, &pe.ParentMac, 6);

			e->ether_type = htons(ETHERTYPE_VEIL);

			setSrcVID(packet,zerovid);
			setDstVID(packet,zerovid);
			setVEILType(packet, VEIL_VCC_SPANNING_TREE_SUBS);

			veil_sub_header *veil_sub = (veil_sub_header *) (e+1);
			veil_payload_vcc_sp_tree_sub *veil_payload = (veil_payload_vcc_sp_tree_sub *) (veil_sub+1);

			memcpy(&veil_payload->vccmac, &vccmac, 6);

			// find out the physical interface corresponding to the nte.myMac
			int interface2parent = interfaces->etheraddToInterfaceIndex.get(nte->myMac);
			if (interfaces->etheraddToInterfaceIndex.get_pointer(vccmac) != NULL){
				interface2parent = interfaces->numInterfaces();
			}
			if (interface2parent >= 0 && interface2parent <= interfaces->numInterfaces()){
				veil_chatter(printDebugMessages, "-o [VCC_AD_SUB_LTOPO_GENERATOR] in sendVCCAdandSubPackets: Sending Sub packet from %s to %s vcc %s at interface %d!\n",nte->myMac.s().c_str(),pe.ParentMac.s().c_str(),vccmac.s().c_str(),interface2parent);
				output(interface2parent).push(packet);
			}else{
				// we don't which interface it is.
				// this is definitely an error.
				veil_chatter(true, "-o [VCC_AD_SUB_LTOPO_GENERATOR]  can't find the index for interface: %s, failed index: %d !\n", nte->myMac.s().c_str(), interface2parent);
				packet->kill();
			}
		}else{
			// we don't how I am connected to the parent mac address yet.
			packet->kill();
		}
	}
}

void
VEILGenerateVCCSTAdSub::sendNeighborInfotoVCC(){

	VEILSpanningTreeState::ForwardingTableToVCC::iterator iter;
	VID zerovid;
	memset(&zerovid, 0, 6);
	EtherAddress vccmac;
	int ninterfaces = interfaces->numInterfaces();

	// check if we have any vcc entry in our list. else exit.
	if (ststate->forwardingTableToVCC.size() == 0){
		veil_chatter(printDebugMessages,"-o [VCC_AD_SUB_LTOPO_GENERATOR] in sendNeighborInfotoVCC: No VCC is learned yet!\n");
		return;
	}

	veil_chatter(printDebugMessages,"-o [VCC_AD_SUB_LTOPO_GENERATOR] in sendNeighborInfotoVCC: Reading the first key in the iterator!\n");
	iter = ststate->forwardingTableToVCC.begin();
	vccmac = iter.key();
	veil_chatter(printDebugMessages,"-o [VCC_AD_SUB_LTOPO_GENERATOR] in sendNeighborInfotoVCC: to vcc %s!\n", vccmac.s().c_str());
	// since I have the vcc mac, choose the one that is closest to me based on the cost,
	// if the cost is same then choose anyone of them.
	// however, for now am assuming that there is only once VCC
	if(ststate->forwardingTableToVCC.size() > 1){
		//choose one vccmac out of all the options.
		// leaving it for now.
		veil_chatter(printDebugMessages,"-o [VCC_AD_SUB_LTOPO_GENERATOR] Warning: in sendNeighborInfotoVCC: Multiple VCC, currently sends the LTOPO to the first VCC (%s) only.!\n", vccmac.s().c_str());
	}

	// forwarder node to reach the VCC
	veil_chatter(printDebugMessages,"-o [VCC_AD_SUB_LTOPO_GENERATOR] in sendNeighborInfotoVCC: Reading the first value in the iterator!\n");
	VEILSpanningTreeState::ParentEntry pe = iter.value();

	EtherAddress parentToVcc = pe.ParentMac;
	EtherAddress myLocalMac;
	memset(&myLocalMac, 0, 6);
	int interface2parent = -1;

	// now construct the Local topology packets, to be sent to VCC.
	// extract the neighbors for each of my interfaces.
	HashTable<EtherAddress, Vector<EtherAddress> > myinterface_neighbors;
	for (int i = 0; i < ninterfaces; i++){
		Vector<EtherAddress> vec;
		EtherAddress mymac = interfaces->interfaceIndexToEtherAddr[i];
		myinterface_neighbors.set(mymac,vec);
	}
	VEILNeighborTable::NeighborTable::iterator it1;
	for (it1 = neighbors->neighbors.begin(); it1; ++it1){
		EtherAddress neigh = it1.key();
		VEILNeighborTable::NeighborTableEntry nte = it1.value();
		Vector <EtherAddress> *vector = myinterface_neighbors.get_pointer(nte.myMac);
		if(vector == NULL){
			veil_chatter(true,"-o [VCC_AD_SUB_LTOPO_GENERATOR] in sendNeighborInfotoVCC: vector is NULL for my interface %s!\n", nte.myMac.s().c_str());
			return;
		}

		if(neigh == parentToVcc){
			memcpy(&myLocalMac, &nte.myMac, 6);
			interface2parent = interfaces->etheraddToInterfaceIndex.get(nte.myMac);
		}
		vector->push_back(neigh);
	}

	if (interfaces->etheraddToInterfaceIndex.get_pointer(vccmac) != NULL){
		interface2parent = ninterfaces;
		veil_chatter(printDebugMessages,"-o [VCC_AD_SUB_LTOPO_GENERATOR] in sendNeighborInfotoVCC: the vccmac %s is me at interface index %d!\n", vccmac.s().c_str(), ninterfaces);
	}if (interface2parent < 0 || interface2parent > ninterfaces){
		veil_chatter(true, "-o [VCC_AD_SUB_LTOPO_GENERATOR] Dont know which of my interface connects to parentToVCC %s, interfaceindex is %d !\n", parentToVcc.s().c_str(), interface2parent);
	}

	for (int i = 0; i < ninterfaces; i++){
		//count the number of neighbors for this interface;
		EtherAddress ethsrc = interfaces->interfaceIndexToEtherAddr.get(i);
		Vector <EtherAddress> *vector = myinterface_neighbors.get_pointer(ethsrc);
		if(vector == NULL){continue;}
		/*if (vector->size() == 0){
			myinterface_neighbors.erase(ethsrc);
			//todo need to erase the vector cleanly before exitting.
			//delete(vector);
			continue;
		}*/
		uint16_t n_neigh = ninterfaces - 1 + vector->size();
		// now create the outputpacket with appropriate length
		veil_chatter(printDebugMessages,"-o [VCC_AD_SUB_LTOPO_GENERATOR] in sendNeighborInfotoVCC: %s has %d neighbors. \n", ethsrc.s().c_str(), n_neigh);
		int packet_len = sizeof(click_ether) + sizeof(veil_sub_header) + sizeof(veil_payload_ltopo_to_vcc) + n_neigh*6;
		WritablePacket *packet = Packet::make(packet_len);
		if (packet == 0) {
				veil_chatter(true, "-o [VCC_AD_SUB_LTOPO_GENERATOR] in %s: cannot make packet for 'subscription' !\n", name().c_str());
				return;
		}
		memset(packet->data(), 0, packet->length());
		click_ether *e = (click_ether *) packet -> data();
		packet->set_ether_header(e);

		memcpy(e->ether_shost, &ethsrc, 6);
		memcpy(e->ether_dhost, &parentToVcc, 6);

		e->ether_type = htons(ETHERTYPE_VEIL);

		veil_sub_header *veil_sub = (veil_sub_header *) (e+1);
		memcpy(&(veil_sub->dvid), &vccmac, 6);
		memcpy(&(veil_sub->svid), &ethsrc, 6);
		setVEILType(packet, VEIL_LOCAL_TOPO_TO_VCC);
		veil_sub->ttl = MAX_TTL;
		veil_payload_ltopo_to_vcc *veil_payload = (veil_payload_ltopo_to_vcc *) (veil_sub+1);
		veil_payload->neighbor_count = htons(n_neigh);

		char * veil_payload_addr = (char *)(veil_payload+1);

		// now first write my local interfaces and then my neighbor interfaces.
		for (int j = 0; j < ninterfaces; j++){
			if (i == j) continue;
			EtherAddress ethadd = interfaces->interfaceIndexToEtherAddr.get(j);
			// copy it on the packet.
			memcpy(veil_payload_addr, &ethadd, 6);
			veil_payload_addr = veil_payload_addr + 6;
		}
		// copy the physical neighbors for the interface on the packet.
		for (int j = 0; j < vector->size(); j++){
			EtherAddress ethadd = (*vector)[j];
			// copy it on the packet.
			memcpy(veil_payload_addr, &ethadd, 6);
			veil_payload_addr = veil_payload_addr + 6;
		}
		// delete the vector now:
		if (vector == NULL){
		}else{
			//todo need to erase the vector cleanly before exitting.
			//delete(vector);
		}
		myinterface_neighbors.erase(ethsrc);

		// now send the packet out from the interface.
		veil_chatter(printDebugMessages,"-o [VCC_AD_SUB_LTOPO_GENERATOR] in sendNeighborInfotoVCC: Pushing the LTOPO packet for interface %s!\n",ethsrc.s().c_str());
		// if vccmac corresponds to one of my interfaces then forward the packet
		// on interface = ninterfaces, it goes back to the classifier.
		if (interface2parent < 0 || interface2parent > ninterfaces){
			veil_chatter(true, "-o [VCC_AD_SUB_LTOPO_GENERATOR] Dont know which of my interface %s connects to parentToVCC %s, interfaceindex is %d !\n", myLocalMac.s().c_str(), parentToVcc.s().c_str(), interface2parent);
		}else{
			output(interface2parent).push(packet);
		}
	}
}
CLICK_ENDDECLS

EXPORT_ELEMENT(VEILGenerateVCCSTAdSub)
