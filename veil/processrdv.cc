#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include <click/vid.hh>
#include "utilities.hh"
#include "click_veil.hh"
#include "processrdv.hh"
#include <strings.h>

CLICK_DECLS

//all types of RDV packets come here

VEILProcessRDV::VEILProcessRDV () {}

VEILProcessRDV::~VEILProcessRDV () {}

int
VEILProcessRDV::configure (
		Vector<String> &conf,
		ErrorHandler *errh)
{
	printDebugMessages = true;
	return cp_va_kparse(conf, this, errh,
			"ROUTETABLE", cpkM+cpkP, cpElementCast, "VEILRouteTable", &routes,
			"RENDEZVOUSTABLE", cpkM+cpkP, cpElementCast, "VEILRendezvousTable", &rdvs,
			"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
			"PRINTDEBUG", 0, cpBool, &printDebugMessages,
			cpEnd);
}

Packet* 
VEILProcessRDV::smaction(Packet* p){
	int myport = PORT_ANNO(p);
	VID myVid;
	interfaces->lookupIntEntry(myport, &myVid);

	const click_ether *eth = (const click_ether *) p->data();

	// FIRST CHECK IF I SHOULD BE HANDLING THIS PACKET OR NOT
	// TO CHECK THIS: SEE IF THE DESTINATION IN THE ETH HEADER IS ME OR NOT
	int intnum;
	VID dvid;
	memcpy(&dvid, eth->ether_dhost, 6);
	if(!(interfaces->lookupVidEntry(&dvid, &intnum))){
		// NO REROUTING IN THIS CASE
		p->kill();
		return NULL;
	}

	const veil_sub_header *vheader = (const veil_sub_header *) (eth+1);
	VID svid;
	memcpy(&svid, &vheader->svid,6);
	memcpy(&dvid, &vheader->dvid,6);

	int interfacenum;
	// if it is not destined to me then route the packet:
	if(!(interfaces->lookupVidEntry(&dvid, &interfacenum))){
		return p;
	}

	// RDV QUERY PACKET
	if(ntohs(vheader->veil_type) == VEIL_RDV_QUERY){
		const veil_payload_rdv_query* query_payload = (const veil_payload_rdv_query*)(vheader+1);
		uint8_t k = query_payload->bucket;
		VID gateway;
		veil_chatter(printDebugMessages, "[-->ProcessRDV<--][RDV Query] Querying node: |%s| for bucket %d\n", svid.switchVIDString().c_str(), k);
		if(rdvs->getRdvPoint(k, &svid, &gateway)){
			//construct and send rdv reply
			//sizeof(rdv_reply) reports a larger size
			//to account for alignment and padding
			//hence the split up
			veil_chatter(printDebugMessages, "[-->ProcessRDV<--][RDV Query Answered] Querying node: |%s| GW node: |%s| for bucket %d\n", svid.switchVIDString().c_str(),gateway.switchVIDString().c_str(), k);
			int packet_length = sizeof(click_ether) + sizeof(veil_sub_header) + sizeof(veil_payload_rdv_reply);

			WritablePacket *q = Packet::make(packet_length);

			if (q == 0) {
				veil_chatter(true, "[-->ProcessRDV<--][Error!] cannot make packet in processrdv");
				return NULL;
			}

			memset(q->data(), 0, q->length());

			click_ether *e = (click_ether *) q->data();
			q->set_ether_header(e);

			//TODO: Make sure that ethernet header is properly modified inside the routepacket element
			// setup the ehternet header
			memcpy(e->ether_dhost, &svid, 6);
			memcpy(e->ether_shost, &myVid, 6);

			e->ether_type = htons(ETHERTYPE_VEIL);

			// setup the veil sub header
			veil_sub_header *reply_vheader = (veil_sub_header*) (e + 1);
			memcpy(&reply_vheader->dvid, &svid,6);
			memcpy(&reply_vheader->svid, &myVid,6);
			reply_vheader->veil_type = htons(VEIL_RDV_REPLY);
			reply_vheader->ttl = MAX_TTL;

			// setup the payload

			veil_payload_rdv_reply * reply_payload = (veil_payload_rdv_reply*)(reply_vheader + 1);
			reply_payload->bucket = k;
			reply_payload->gw_count = 1;
			memcpy(reply_payload->gw_vid[0], &gateway,4);
			p->kill();
			return q;
		} else {
			//TODO: didn't get a rdv point. handle this
			p->kill();
			return NULL;
		}
	}

	// RDV PUBLISH PACKET
	if(ntohs(vheader->veil_type) == VEIL_RDV_PUBLISH){
		const veil_payload_rdv_publish* publish_payload = (const veil_payload_rdv_publish*)(vheader+1);
		VID end2vid;
		bzero(&end2vid,6);
		memcpy(&end2vid, publish_payload->neighbor_vid, 4);
		rdvs->updateEntry(&svid, &end2vid);
		veil_chatter(printDebugMessages, "[-->ProcessRDV<--][RDV Publish] Learned RDV edge: |%s| --> |%s| \n", svid.switchVIDString().c_str(),end2vid.switchVIDString().c_str());
		p->kill();
		return NULL;
	}

	// RDV REPLY PACKET
	if(ntohs(vheader->veil_type) == VEIL_RDV_REPLY){
		const veil_payload_rdv_reply* reply_payload = (const veil_payload_rdv_reply*)(vheader+1);
		uint8_t k = reply_payload->bucket;
		uint8_t gw_count = reply_payload->gw_count;
		uint8_t j = 0;
		for (j = 0; j < gw_count; j++){
			VID gateway,nh,g;
			bzero(&gateway,6);
			memcpy(&gateway, reply_payload->gw_vid[j], 4);
			uint8_t dist_to_gateway = dvid.logical_distance(&gateway);

			if (dist_to_gateway > 16){
				veil_chatter(printDebugMessages, "[-->ProcessRDV<--][RDV Reply][Gateway] MyVID: |%s| GWVID: |%s| BucketLevel: %d \n", dvid.switchVIDString().c_str(),gateway.switchVIDString().c_str(), k);
				//find nexthop to reach gateway
				if(routes->getRoute(&gateway, dist_to_gateway, dvid, &nh, &g, true))
				{
					// is it the default route entry? (i.e. j == o
					if (j == 0)
						{routes->updateEntry(&dvid, k, &nh, &gateway, true);}
					// Add a non-default route entry
					else{routes->updateEntry(&dvid, k, &nh, &gateway, false);}

				} else {
					veil_chatter(true, "[-->ProcessRDV<--][RDV Reply][Error!][No Nexthop to GW] MyVID: |%s| GWVID: |%s| BucketLevel: %d \n", dvid.switchVIDString().c_str(),gateway.switchVIDString().c_str(), dist_to_gateway);
				}
			}
		}

		p->kill();
		return NULL;

	}

	//not a rdv pkt
	//TODO: handle error
	veil_chatter(true, "[-->ProcessRDV<--][RDV Unknown Type][Error!] veil_type: %d \n",vheader->veil_type);
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
