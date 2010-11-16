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
		veil_chatter(true, "-o [VEIL_Process_VCC_ST_Ad_Sub] ERROR: ETHER_TYPE must be ETHERTYPE_VEIL(%d), but found %d!\n",ETHERTYPE_VEIL, ethtype);
		p->kill();
		p = NULL;
		return -1;
	}
	veil_sub_header *veil_sub = (veil_sub_header *) (e+1);
	uint16_t  veiltype = ntohs(veil_sub->veil_type);
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
		case VEIL_SWITCH_VID_LIST_FROM_VCC:
			outport = processSwitchVIDList(p);
			break;
		default:
			veil_chatter(true, "-o [VEIL_Process_VCC_ST_Ad_Sub] ERROR: VEIL_TYPE must be VEIL_VCC_SPANNING_TREE_AD/SUBS/LOCAL_TOPO/VID_LIST(%d,%d,%d,%d), but found %d!\n",VEIL_VCC_SPANNING_TREE_AD,VEIL_VCC_SPANNING_TREE_SUBS,VEIL_LOCAL_TOPO_TO_VCC,VEIL_SWITCH_VID_LIST_FROM_VCC, veiltype);
			p->kill();
			p = NULL;
			return -1;
	}

	if (outport == -1 && p != NULL){
		veil_chatter(true, "-o [VEIL_Process_VCC_ST_Ad_Sub] ERROR: While processing the packet!\n");
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
	veil_chatter(printDebugMessages, "-o [VEIL_Process_VCC_ST_Ad_Sub] received st ad packet from parent %s to vcc %s with cost %d\n", parent.s().c_str(), vcc.s().c_str(),cost);
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

	veil_chatter(printDebugMessages, "-o [VEIL_Process_VCC_ST_Ad_Sub] Added children %s in the list to reach vcc %s\n", child.s().c_str(), vcc.s().c_str());
}

int
VEILProcessVCCSTAdSub::processLocalTopo(Packet*p){
	int outport = -1;
	/*
	 * TODO: if I am vcc then take care of it accordingly.
	 */

	// else part when this node is not the vcc.
	EtherAddress dstVCC, myinterfaceMAC, myneighborInterfaceMac;
	click_ether *e = (click_ether *) p-> data();
	veil_sub_header *veil_sub = (veil_sub_header *) (e+1);
	memcpy(&dstVCC, &veil_sub->dvid,6);

	// lookup the interface to reach memcpy.
	if(ststate->getParentNodeToVCC(dstVCC, &myneighborInterfaceMac)){
		// now find the index of my interface where my parent node to reach vcc is connected.
		// find out which physical interface is directly connected to pe.ParentMac.
		VEILNeighborTable::NeighborTableEntry* nte;
		if (neighbors->lookupEntry(myneighborInterfaceMac, &nte)){
			memcpy(e->ether_shost, &nte->myMac, 6);
			memcpy(e->ether_dhost, &myneighborInterfaceMac, 6);

			veil_sub_header *veil_sub = (veil_sub_header *) (e+1);
			// ttl check
			if (veil_sub->ttl <= 1){
				veil_chatter(true, "-o [VEIL_Process_VCC_ST_Ad_Sub]  ERROR: in processLocalTopo: TTL expired.");
				return -1;
			}
			veil_sub->ttl = veil_sub->ttl -1;

			// find out the physical interface corresponding to the nte.myMac
			int interface2parent = interfaces->etheraddToInterfaceIndex.get(nte->myMac);
			if (interface2parent >= 0 && interface2parent < interfaces->numInterfaces()){
				outport = interface2parent;
			}else{
				veil_chatter(true, "-o [VEIL_Process_VCC_ST_Ad_Sub]  ERROR: in processLocalTopo: can't find the index for interface: %s, failed index: %d !\n", nte->myMac.s().c_str(), interface2parent);
				outport = -1;
			}
		}
	}
	return outport;
}

int
VEILProcessVCCSTAdSub::processSwitchVIDList(Packet*p){
	int outport = -1;
	// extract VID for my interfaces.

	// see if I have any children for this vcc, if I do then send the packets downwards.
	return outport;
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessVCCSTAdSub)
