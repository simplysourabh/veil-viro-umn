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
		"ROUTETABLE", cpkM+cpkP, cpElementCast, "VEILRouteTable", &routes,
		"RENDEZVOUSTABLE", cpkM+cpkP, cpElementCast, "VEILRendezvousTable", &rdvs,
		"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
		cpEnd);
}

Packet* 
VEILProcessRDV::smaction(Packet* p){
	int myport = PORT_ANNO(p);
	VID myVid;
	interfaces->lookupIntEntry(myport, &myVid);

	const click_ether *eth = (const click_ether *) p->data();
	VID svid = VID(eth->ether_shost);
	VID dvid = VID(eth->ether_dhost);
	veil_header *vhdr = (veil_header*) (eth+1);
	
	if(ntohs(vhdr->packetType) == VEIL_RDV_QUERY){
		//if pkt was destined for one of our interfaces, send reply
		//reply is a unicast pkt hence routed like other pkts
		int interfacenum;
		if(interfaces->lookupVidEntry(&dvid, &interfacenum)){
			uint16_t *kptr = (uint16_t*) (vhdr + 1);
			uint16_t k = ntohs(*kptr);
			VID gateway;
			click_chatter( "[ProcessRDV][RDV Query] Querying node: |%s| for bucket %d\n", svid.vid_string().c_str(), k);
			if(rdvs->getRdvPoint(k, &svid, &gateway)){
				//construct and send rdv reply
				//sizeof(rdv_reply) reports a larger size
				//to account for alignment and padding
				//hence the split up
				click_chatter( "[ProcessRDV][RDV Query Answered] Querying node: |%s| GW node: |%s| for bucket %d\n", svid.vid_string().c_str(),gateway.vid_string().c_str(), k);
				int packet_length = sizeof(click_ether) + sizeof(veil_header) + sizeof(rdv_reply);	

				WritablePacket *q = Packet::make(packet_length);

	        		if (q == 0) {
	                		click_chatter( "[ProcessRDV][Error!] cannot make packet in processrdv");
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
				
				// SJ: IT Seems that we have forgotten to KILL the old packet p!
				p->kill();
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
	
	if(ntohs(vhdr->packetType) == VEIL_RDV_PUBLISH){
		int interfacenum;
		//process pkt only if it was meant for us
		if(interfaces->lookupVidEntry(&dvid, &interfacenum)){
			VID *end1vid = &svid;
			VID *end2vid = (VID*) (vhdr + 1);
			rdvs->updateEntry(end1vid, end2vid);
			p->kill();
			click_chatter( "[ProcessRDV][RDV Publish] Learned RDV edge: |%s| --> |%s| \n", end1vid->vid_string().c_str(),end2vid->vid_string().c_str());
			return NULL;
		} else {
			//needs to be rerouted
			return p;
		}
	}

	if(ntohs(vhdr->packetType) == VEIL_RDV_REPLY){
		int interfacenum;

		//process pkt only if it was meant for us
		if(interfaces->lookupVidEntry(&dvid, &interfacenum)){
			rdv_reply* r = (rdv_reply*) (vhdr + 1);
			uint16_t k = ntohs(r->k);
			VID gateway;
			memcpy(&gateway, &r->gatewayvid, VID_LEN);
			VID nh, g;
			
			uint16_t dist_to_gateway = dvid.logical_distance(&gateway);
			click_chatter( "[ProcessRDV][RDV Reply][Gateway] MyVID: |%s| GWVID: |%s| BucketLevel: %d \n", dvid.vid_string().c_str(),gateway.vid_string().c_str(), k);
			//find nexthop to reach gateway
			if(routes->getRoute(&gateway, dist_to_gateway, dvid, &nh, &g))	
			{
				routes->updateEntry(&dvid, k, &nh, &gateway);
			} else {
				//we didn't find a nexthop for our gateway
				//TODO: what to do?
				click_chatter( "[ProcessRDV][RDV Reply][Error!][No Nexthop to GW] MyVID: |%s| GWVID: |%s| BucketLevel: %d \n", dvid.vid_string().c_str(),gateway.vid_string().c_str(), dist_to_gateway);
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
	click_chatter( "[ProcessRDV][RDV Unknown Type][Error!] PacketType: %d \n",vhdr->packetType);
	p->kill();
	return NULL;
}
 
void
VEILProcessRDV::push (
	int port,
	Packet* pkt)
{
	Packet *p = smaction(pkt);
	if(p != NULL)
		output(0).push(p);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessRDV)
