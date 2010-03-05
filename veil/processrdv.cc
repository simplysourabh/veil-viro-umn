#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include <click/vid.hh>
#include "utilities.hh"
#include "click_veil.hh"
#include "processrdv.hh"

CLICK_DECLS

//all types of RDV packets come here

VEILProcessRDV::VEILProcessRDV () {}

VEILProcessRDV::~VEILProcessRDV () {}

int
VEILProcessRDV::configure (
	Vector<String> &conf,
	ErrorHandler *errh)
{
	return cp_va_kparse(conf, this, errh,
		"ROUTETABLE", cpkM+cpkP, cpElementCast, "VEILHostTable", &routes,
		"RENDEZVOUSTABLE", cpkM+cpkP, cpElementCast, "VEILMappingTable", &rdvs,
		"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
		cpEnd);
}

Packet* 
VEILProcessRDV::smaction(Packet* p){
	int myport = PORT_ANNO(p);
	VID myVid;
	interfaces->lookupIntEntry(myport, &myVid);

	const click_ether *eth = (const click_ether *) p->mac_header();
	VID svid = VID(eth->ether_shost);
	VID dvid = VID(eth->ether_dhost);
	veil_header *vhdr = (veil_header*) (eth+1);
	
	if(vhdr->packetType == VEIL_RDV_QUERY){
		//if pkt was destined for one of our interfaces, send reply
		//reply is a unicast pkt hence routed like other pkts
		int interfacenum;
		if(interfaces->lookupVidEntry(&dvid, &interfacenum)){
			int *kptr = (int*) (vhdr + 1);
			int k = ntohs(*kptr);
			VID gateway;
			if(rdvs->getRdvPoint(k, &svid, &gateway)){
				//construct and send rdv reply
				int packet_length = sizeof(click_ether) + sizeof(veil_header) + sizeof(rdv_reply);		
				WritablePacket *q = Packet::make(packet_length);

	        		if (q == 0) {
	                		click_chatter( "cannot make packet in processrdv");
	                		return NULL;
	        		}

				memset(q->data(), 0, q->length());

				click_ether *e = (click_ether *) q->data();
				q->set_ether_header(e);
	
				memcpy(e->ether_dhost, &svid, 6); 
				//TODO: should this be svid instead?
				//is svid == myVid always for this case?
				memcpy(e->ether_shost, &myVid, 6);

				e->ether_type = htons(ETHERTYPE_VEIL);

				veil_header *vheader = (veil_header*) (e + 1);
				vheader->packetType = htons(VEIL_RDV_REPLY);

				rdv_reply *r = (rdv_reply*) (vheader+1);
				r->k = htons(k);
				memcpy(&r->gatewayvid, &gateway, VID_LEN);

				return q;	
			} else {
				//TODO: didn't get a rdv point. handle this
				p->kill();
				return NULL;
			}
		} else {
			//needs to be rerouted
			return p;
		}
	}
	
	if(vhdr->packetType == VEIL_RDV_PUBLISH){
		int interfacenum;
		//process pkt only if it was meant for us
		if(interfaces->lookupVidEntry(&dvid, &interfacenum)){
			VID *gvid = (VID*) (vhdr + 1);
			rdvs->updateEntry(&svid, gvid);
			p->kill();
			return NULL;
		} else {
			//needs to be rerouted
			return p;
		}
	}

	//TODO: check if this is correct
	if(vhdr->packetType == VEIL_RDV_REPLY){
		int interfacenum;

		//process pkt only if it was meant for us
		if(interfaces->lookupVidEntry(&dvid, &interfacenum)){
			rdv_reply* r = (rdv_reply*) (vhdr + 1);
			int k = ntohs(r->k);
			VID gateway;
			memcpy(&gateway, &r->gatewayvid, VID_LEN);
			VID i, nh, g;

			int dist_to_gateway = myVid.logical_distance(&gateway);
			//find nexthop to reach gateway
			if(routes->getRoute(&gateway, dist_to_gateway, &i, &nh, &g))	
			{
				routes->updateEntry(&myVid, k, &nh, &gateway);
			} else {
				//we didn't find a nexthop for our gateway
				//TODO: what to do?
			}

			p->kill();
			return NULL;
		} else {
			//needs to be forwarded
			return p;
		}		
	}

	//not a rdv pkt
	//TODO: handle error
	p->kill();
	return NULL;
}
 
void
VEILProcessRDV::push (
	int port,
	Packet* pkt)
{
	Packet *p = smaction(pkt);
	output(0).push(p);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessRDV)
