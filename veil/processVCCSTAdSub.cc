#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/vector.hh>
#include "click_veil.hh"
#include "veil_packet.hh"
#include "generatehelloNew.hh"
#include "processVCCSTAdSub.hh"


/*
 * EXPLAIN THE PURPOSE OF THIS ELEMENT.
 * It is used to process the Advertisement/Subscription
 * Packets to construct the spanning tree to reach the
 * VCC (veil central controller)
 */

CLICK_DECLS

VEILProcessVCCSTAdSub::VEILProcessVCCSTAdSub () {}

VEILProcessVCCSTAdSub::~VEILProcessVCCSTAdSub () {}

int
VEILProcessVCCSTAdSub::configure (
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

// process the new incoming packet.
// if its an advertizement packet then check its cost, and if its
// better than the existing ads (or same), then replace the current best cost.
// however, if its worse then discard the advertisement for now.

// if the incoming packet is a subscription packet then
// insert the neighbor info in the table.

int
VEILProcessVCCSTAdSub::smaction(Packet* p){
	assert(p);

	int myport = PORT_ANNO(p);
	int outport = -1;
	/*
	 * Process the packets of types, VCC ST AD, SUB, LOCAL TOPO, VID LIST packets here.
	 */

	// extract the packet type.
	click_ether *e = (click_ether *) p->data();
	uint16_t ethtype = ntohs(e->ether_type);
	if (ethtype != ETHERTYPE_VEIL){
		veil_chatter_new(true, class_name(), " ERROR: ETHER_TYPE must be ETHERTYPE_VEIL(%d), but found %d!",ETHERTYPE_VEIL, ethtype);
		p->kill();
		p = NULL;
		return -1;
	}
	veil_sub_header *veil_sub = (veil_sub_header *) (e+1);
	uint16_t  veiltype = ntohs(veil_sub->veil_type);
	veil_chatter_new(printDebugMessages, class_name(),"Processing the packet type: %s", get_packet_type_str(veiltype));

	// except if the packet if of type VEIL_VCC_SPANNING_TREE_AD
	// make sure that packet is destined to me.
	if (veiltype != VEIL_VCC_SPANNING_TREE_AD){
		EtherAddress dmac;
		memcpy(dmac.data(), e->ether_dhost, 6);
		// see if dmac is one of my interfaces address or not.
		if (interfaces->etheraddToInterfaceIndex.get_pointer(dmac) == NULL){
			// it is not destined to me, therefore, I should not handle this packet.
			// lets kill this packet.
			veil_chatter_new(true, class_name(), "Packet is destined to %s, which is not my MAC address.", dmac.s().c_str());
			p->kill();
			p=NULL;
			return -1;
		}
		
	}

	switch(veiltype){
		case VEIL_VCC_SPANNING_TREE_AD:
			processAD(p);
			p->kill();
			p = NULL;
			break;
		case VEIL_VCC_SPANNING_TREE_SUBS:
			processSUBS(p);
			p->kill();
			p = NULL;
			break;
		case VEIL_LOCAL_TOPO_TO_VCC:
			outport = processLocalTopo(p);
			break;
		case VEIL_SWITCH_VID_FROM_VCC:
			outport = processSwitchVID(p);
			break;
		default:
			veil_chatter_new(true, class_name(), "ERROR: VEIL_TYPE must be VEIL_VCC_SPANNING_TREE_AD/SUBS/LOCAL_TOPO/VID(%d,%d,%d,%d), but found %d!",VEIL_VCC_SPANNING_TREE_AD,VEIL_VCC_SPANNING_TREE_SUBS,VEIL_LOCAL_TOPO_TO_VCC,VEIL_SWITCH_VID_FROM_VCC, veiltype);
			p->kill();
			p = NULL;
			return -1;
	}

	if (outport == -1 && p != NULL){
		veil_chatter_new(printDebugMessages, class_name(), "Outport is %d or packet is NULL.", outport);
		p->kill();
		p = NULL;
		return -1;
	}

	return outport;
}


// pushes the packets to
void
VEILProcessVCCSTAdSub::push (int inp, Packet* pkt){
	int port = smaction(pkt);
	if (pkt != NULL && port != -1){
		output(port).push(pkt);
	}
}

void
VEILProcessVCCSTAdSub::processAD(Packet*p){
	EtherAddress vcc, parent;
	uint16_t cost;

	click_ether *e = (click_ether *) p-> data();
	memcpy(&parent, e->ether_shost,6);

	veil_sub_header *veil_sub = (veil_sub_header *) (e+1);
	veil_payload_vcc_sp_tree_ad *veil_payload = (veil_payload_vcc_sp_tree_ad *) (veil_sub+1);

	memcpy(&vcc, &(veil_payload->vccmac), 6);
	cost = ntohs(veil_payload->cost);

	ststate->updateCostToVCC(parent, vcc, cost);
	veil_chatter_new(printDebugMessages, class_name(), "received st ad packet from parent %s to vcc %s with cost %d", parent.s().c_str(), vcc.s().c_str(),cost);
}

void
VEILProcessVCCSTAdSub::processSUBS(Packet*p){
	EtherAddress vcc, child;

	click_ether *e = (click_ether *) p-> data();
	memcpy(&child, e->ether_shost,6);

	veil_sub_header *veil_sub = (veil_sub_header *) (e+1);
	veil_payload_vcc_sp_tree_ad *veil_payload = (veil_payload_vcc_sp_tree_ad *) (veil_sub+1);

	memcpy(&vcc, &(veil_payload->vccmac), 6);

	ststate->updateChild(child, vcc);

	veil_chatter_new(printDebugMessages, class_name(), "Added children %s in the list to reach vcc %s", child.s().c_str(), vcc.s().c_str());
}

int
VEILProcessVCCSTAdSub::processLocalTopo(Packet*p){
	int outport = -1;

	EtherAddress dstVCC, myinterfaceMAC, myneighborInterfaceMac, srcmac;
	click_ether *e = (click_ether *) p-> data();
	veil_sub_header *veil_sub = (veil_sub_header *) (e+1);
	memcpy(&dstVCC, &veil_sub->dvid,6);
	memcpy(&srcmac, &veil_sub->svid,6);
	veil_chatter_new(printDebugMessages, class_name(), "Received LTOPO packet from %s to VCC %s", srcmac.s().c_str(), dstVCC.s().c_str());
	/*
	 * TODO: if I am vcc then take care of it accordingly.
	 */
	if (interfaces->etheraddToInterfaceIndex.get_pointer(dstVCC) != NULL){
		veil_chatter_new(printDebugMessages, class_name(), "Received processLocalTopo for my interface %s as VCC from %s as sourcemac.", dstVCC.s().c_str(), srcmac.s().c_str());
		// extract the neighbor list for this interface and update the topology information.
		uint16_t n_neigh;
		veil_payload_ltopo_to_vcc *veil_payload = (veil_payload_ltopo_to_vcc *) (veil_sub+1);
		n_neigh = ntohs(veil_payload->neighbor_count);

		veil_chatter_new(printDebugMessages, class_name(), " # of neighbors for %s = %d", srcmac.s().c_str(), n_neigh);
		char * neighbor_addr = (char *)(veil_payload+1);

		// now extract all the neighbor addresses one by one.
		// to overwrite the neighbor vector first remove the corresponding
		// neighbor from the topology graph, and then re-enter it.
		topo->removeNode(srcmac);
		for (int i = 0; i < n_neigh; i++){
			EtherAddress neighmac;
			memcpy(&neighmac, neighbor_addr, 6);
			neighbor_addr += 6;
			topo->addAnEdge(srcmac, neighmac);
			topo->addAnEdge(neighmac,srcmac); // assumes that its a symmetric
		}
		topo->lastUpdateTimeForNode(srcmac);
		return -1;
	}
	// else part when this node is not the vcc.
	else{
		// lookup tVCChe interface to reach memcpy.
		if(ststate->getParentNodeToVCC(dstVCC, &myneighborInterfaceMac)){
			// now find the index of my interface where my parent node to reach vcc is connected.
			// find out which physical interface is directly connected to pe.ParentMac.
			VEILNeighborTable::NeighborTableEntry* nte;
			if (neighbors->lookupEntry(myneighborInterfaceMac, &nte)){
				memcpy(e->ether_shost, &nte->myMac, 6);
				memcpy(e->ether_dhost, &myneighborInterfaceMac, 6);

				veil_sub_header *veil_sub = (veil_sub_header *) (e+1);
				// ttl check
				if (veil_sub->ttl < 1){
					veil_chatter_new(true, class_name(), "ERROR: in processLocalTopo: TTL expired.");
					return -1;
				}
				veil_sub->ttl = veil_sub->ttl -1;
				// find out the physical interface corresponding to the nte.myMac
				int interface2parent = interfaces->etheraddToInterfaceIndex.get(nte->myMac);
				if (interface2parent >= 0 && interface2parent < interfaces->numInterfaces()){
					outport = interface2parent;
				}else{
					veil_chatter_new(true, class_name(), "ERROR: in processLocalTopo: can't find the index for interface: %s, failed index: %d !", nte->myMac.s().c_str(), interface2parent);
					outport = -1;
				}
			}
		}
	}
	return outport;
}

int
VEILProcessVCCSTAdSub::processSwitchVID(Packet*p){
	int outport = -1;
	// check if the packet is for me using the dvid field in the veil_sub_header.
	EtherAddress vccmac, dstmac;
	click_ether *e = (click_ether *) p-> data();
	veil_sub_header *veil_sub = (veil_sub_header *) (e+1);
	memcpy(&dstmac, &veil_sub->dvid,6);
	memcpy(&vccmac, &veil_sub->svid,6);
	if (veil_sub->ttl < 1){
		veil_chatter_new(true, class_name(), "ERROR: in processLocalTopo: TTL expired.");
		return -1;
	}
	//printf("TTL is : %d ", veil_sub->ttl);
	veil_sub->ttl = veil_sub->ttl -1;
	//printf("Updated TTL is : %d\n", veil_sub->ttl);

	veil_chatter_new(printDebugMessages, class_name(), "Packet of type: switch vid assignment, coming from vccmac %s dstmac %s", vccmac.s().c_str(), dstmac.s().c_str());

	// it is destined to one of my interfaces.
	if(interfaces->etheraddToInterfaceIndex.get_pointer(dstmac) != NULL){
		bool vidset = true;
		veil_chatter_new(printDebugMessages, class_name(), "Packet of type: switch vid assignment, destination %s is me!", dstmac.s().c_str());
		veil_payload_svid_mappings *veil_payload = (veil_payload_svid_mappings *) (veil_sub+1);
		VID myvid;
		memcpy(&myvid, &veil_payload->vid, 6);
		// update my interface vid.
		int myintindex = interfaces->etheraddToInterfaceIndex.get(dstmac);
		if (interfaces->rinterfaces.get_pointer(myintindex) != NULL){
			//duplicate
			VID cvid = interfaces->rinterfaces.get(myintindex);
			if (cvid != myvid) {
				veil_chatter_new(true, class_name(), "Warning: Changed vid for interface %d, from %s to %s! ",myintindex, cvid.vid_string().c_str(), myvid.vid_string().c_str());
			}
		}
		interfaces->rinterfaces[myintindex] = myvid;
		interfaces->interfaces[myvid] = myintindex;
		veil_chatter_new(printDebugMessages, class_name(), "VID for interface %d is %s! ",myintindex, myvid.vid_string().c_str());
		// now check if the myintindex is associated with different vid in the
		// interfaces->interfaces table.
		VEILInterfaceTable::InterfaceTable::iterator iter;
		iter = interfaces->interfaces.begin();
		while (iter){
			int int_id = iter.value();
			VID cvid = iter.key();
			if (cvid != myvid && int_id == myintindex){
				veil_chatter_new(printDebugMessages, class_name(), "Warning: removing old assignment vid(%s) -> interface id (%d)", cvid.switchVIDString().c_str(), int_id);
				iter = interfaces->interfaces.erase(iter);
			}else{
				iter++;
			}
				
		}

		interfaces->isvidset.set(myintindex, vidset);

		// now make sure if vid is set for all the interfaces or not.
		bool allset = true;
		for (int i = 0; i < interfaces->etheraddToInterfaceIndex.size(); i++){
			if(!interfaces->isvidset[i]) {
				veil_chatter_new(printDebugMessages, class_name(),"%d th interface %s does not have the vid assignment yet.", i, interfaces->interfaceIndexToEtherAddr[i].s().c_str());
				allset = false;break;
			}
		}
		interfaces->isVIDAssignmentDone = allset;

	}
	else{
		VEILSpanningTreeState::ForwardingTableFromVCC::const_iterator it;
		for (it = ststate->forwardingTableFromVCC.begin(); it; it++){
			EtherAddress childmac = it.key();
			if (it.value().VccMac == vccmac){
				// create a new copy of the packet.
				WritablePacket *pkt = Packet::make(p->length());
				memcpy(pkt->data(), p->data(), p->length());
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
						veil_chatter_new(printDebugMessages, class_name(), "Packet of type: switch vid assignment, sending out from %s to %s at interfaceid %d", mymac.s().c_str(), vccmac.s().c_str(), myinterfaceid);
					}else{
						//error
						veil_chatter_new(true, class_name(), "Error: Couldn't retrieve the interface id for %s! ", mymac.s().c_str());
						veil_chatter_new(printDebugMessages, class_name(), "Packet of type: switch vid assignment, sending out from %s to %s at interfaceid %d", mymac.s().c_str(), vccmac.s().c_str(), myinterfaceid);
					}
				}else{
					// error!
					veil_chatter_new(true, class_name(), "Error: Couldn't retrieve the my local interface connected to %s! ", childmac.s().c_str());
				}
			}
		}
		return -1;
	}
	return -1;
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessVCCSTAdSub)
