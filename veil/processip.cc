#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include "click_veil.hh"
#include "processip.hh"

CLICK_DECLS

VEILProcessIP::VEILProcessIP () {}

VEILProcessIP::~VEILProcessIP () {}

int
VEILProcessIP::configure (
		Vector<String> &conf,
		ErrorHandler *errh)
{
	printDebugMessages = true;
	return cp_va_kparse(conf, this, errh,
			"HOSTTABLE", cpkM+cpkP, cpElementCast, "VEILHostTable", &hosts,
			"MAPPINGTABLE", cpkM+cpkP, cpElementCast, "VEILMappingTable", &map,
			"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
			"PRINTDEBUG", 0, cpBool, &printDebugMessages,
			cpEnd);
}

Packet* 
VEILProcessIP::smaction(Packet* p)
{
	assert(p);

	int myport = PORT_ANNO(p);
	VID myVid;
	interfaces->lookupIntEntry(myport, &myVid);

	click_ether *eth = (click_ether*) p->data();

	VID dvid;
	VID vid, mvid, svid;
	EtherAddress dmac = EtherAddress(eth->ether_dhost);
	EtherAddress smac = EtherAddress(eth->ether_shost);
	memcpy(&dvid, &dmac, VID_LEN);
	memcpy(&svid, &smac, VID_LEN);
	// TODO: Decrement the TTL on the IP header!
	// SJ: Lets do it inside the routing module

	veil_chatter(printDebugMessages,"[ProcessIP][PACKETIN] From (Mac: %s) to (VID: %s),\n",  smac.s().c_str(), dvid.vid_string().c_str());

	//if IP packet came from one of our hosts
	if(ntohs(eth->ether_type) == ETHERTYPE_IP){
		click_ip *ip_header = (click_ip*) (eth+1);
		IPAddress srcip = IPAddress(ip_header->ip_src);
		IPAddress dstip = IPAddress(ip_header->ip_dst);

		//Register "source ip, source mac" to a vid with myvid.	
		VID::generate_host_vid(&myVid, &smac, &svid);	
		hosts->updateEntry(&svid, &smac, &srcip);
		//veil_chatter(true, "PacketLength : %d \n", ntohs(ip_header->ip_len));
		//check if dst is a host connected to us
		if(hosts->lookupIP(&dstip, &dvid)){
			//if src and dst are connected to us
			//thru the same interface, ignore packet
			dvid.extract_switch_vid(&vid);
			if(vid == myVid){
				veil_chatter(printDebugMessages,"[ProcessIP][EtherTYPE_IP][Both Host are on same interface] From (IP: %s, Mac: %s) to (IP: %s, VID: %s),\n", srcip.s().c_str(), smac.s().c_str(), dstip.s().c_str(),dvid.vid_string().c_str());
				p->kill();
				return NULL;
			}

			hosts->lookupMAC(&smac, &svid);
			hosts->lookupVID(&dvid, &dmac);
			memcpy(eth->ether_shost, &svid, VID_LEN);
			memcpy(eth->ether_dhost, &dmac, VID_LEN);
			veil_chatter(printDebugMessages,"[... ProcessIP ][EtherTYPE_IP][Host are on different interfaces] From (IP: %s, Mac: %s) to (IP: %s, VID: %s), After add rewriting: From (IP: %s, VID: %s) to (IP: %s, Mac: %s) \n", srcip.s().c_str(), smac.s().c_str(), dstip.s().c_str(),dvid.vid_string().c_str() , srcip.s().c_str(), svid.vid_string().c_str(), dstip.s().c_str(),dmac.s().c_str());
			return p;
		}


		// For every other packet we can simply overwrite the "source mac address" by the corresponding VID.
		else if (IP_FORWARDING_TYPE == IP_FORWARDING_TYPE_ADD_OVERWRITE){
			// do two things:
			// 1. Overwrite eth->ether_type by the ETHERTYPE_VEIL_IP
			// 2. overwrite "source mac" eth-> ether_shost by source vid.
			short etype = ETHERTYPE_VEIL_IP;
			eth->ether_type = htons(etype);
			memcpy(eth->ether_shost, &svid, VID_LEN); 
			veil_chatter(printDebugMessages,"[... ProcessIP ][EtherTYPE_IP][Encapsulation to VEIL_IP type] From (IP: %s, Mac: %s) to (IP: %s, VID: %s), After add rewriting: From (IP: %s, VID: %s)\n", srcip.s().c_str(), smac.s().c_str(), dstip.s().c_str(),dvid.vid_string().c_str() , srcip.s().c_str(), svid.vid_string().c_str());
			return p;
		} else if (IP_FORWARDING_TYPE == IP_FORWARDING_TYPE_ENCAP){
			int packet_length = p->length() + sizeof(veil_sub_header);
			int ip_payload = p->length() - sizeof(click_ether);
			WritablePacket * q = Packet::make(packet_length);

			// set the ether header
			click_ether* q_eth = (click_ether*)(q->data());
			memcpy(q_eth, eth, sizeof(click_ether));
			// memcpy(q_eth->dhost, .. , 6);
			memcpy(q_eth->ether_shost,&svid , 6);
			q_eth->ether_type = htons(ETHERTYPE_VEIL);

			//insert the veil_sub_header
			veil_sub_header *q_vheader = (veil_sub_header*) (q_eth + 1);
			memcpy(&q_vheader->dvid, &dvid, 6);
			memcpy(&q_vheader->svid, &svid, 6);
			q_vheader->veil_type = htons(VEIL_ENCAP_IP);
			q_vheader->ttl = MAX_TTL;

			memcpy( (q_vheader+1), (eth+1),ip_payload);
			p->kill();
			return q;
		}else if(IP_FORWARDING_TYPE == IP_FORWARDING_TYPE_MULTIPATH){
			int packet_length = p->length() + sizeof(veil_sub_header) + sizeof(veil_payload_multipath);
			int ip_payload = p->length() - sizeof(click_ether);
			WritablePacket * q = Packet::make(packet_length);

			// set the ether header
			click_ether* q_eth = (click_ether*)(q->data());
			memcpy(q_eth, eth, sizeof(click_ether));
			// memcpy(q_eth->dhost, .. , 6);
			memcpy(q_eth->ether_shost,&svid , 6);
			q_eth->ether_type = htons(ETHERTYPE_VEIL);

			//insert the veil_sub_header
			veil_sub_header *q_vheader = (veil_sub_header*) (q_eth + 1);
			memcpy(&q_vheader->dvid, &dvid, 6);
			memcpy(&q_vheader->svid, &svid, 6);
			q_vheader->veil_type = htons(VEIL_ENCAP_MULTIPATH_IP);
			q_vheader->ttl = MAX_TTL;

			//insert the veil_multipath_payload
			veil_payload_multipath *q_veil_payload = (veil_payload_multipath*) (q_vheader + 1);
			memcpy(&q_veil_payload->final_dvid, &dvid, 6);

			memcpy( (q_veil_payload+1), (eth+1),ip_payload);
			SET_PORT_ANNO(q, myport);
			p->kill();
			return q;
		}

		p->kill();
		return NULL;
	}

	//IP pkt from another switch
	if(ntohs(eth->ether_type) == ETHERTYPE_VEIL_IP){
		click_ip *i = (click_ip*) (eth+1);
		IPAddress srcip = IPAddress(i->ip_src);
		IPAddress dstip = IPAddress(i->ip_dst);

		// CHECK for following things here:
		// 1. Is destination one of my host-devices?
		// if yes: a. overwrite the eth->ether_type to ETHERTYPE_IP
		// b. overwrite the eth->ether_dhost by the real mac address
		// return the modified packet p

		if (hosts->lookupVID(&dvid, &dmac)){
			short etype = ETHERTYPE_IP;
			eth->ether_type = htons(etype);
			memcpy(eth->ether_dhost, &dmac, VID_LEN);
			veil_chatter(printDebugMessages,"[... ProcessIP ][VEIL to EtherTYPE_IP] After add rewriting: From (IP: %s, VID: %s) to (IP: %s, VID: %s, MAC: %s)\n", srcip.s().c_str(), svid.vid_string().c_str(), dstip.s().c_str(), dvid.vid_string().c_str(), dmac.s().c_str());
		}else{
			// see if packet belongs to one of my interfaces or not:
			VID intvid;
			memset(&intvid,0,6);
			memcpy(&intvid,&dvid, 4);
			int interface;
			if(interfaces->lookupVidEntry(&intvid, &interface)){
				veil_chatter(printDebugMessages,"[... ProcessIP ][VEILSwitch to VEILSwitch][NO MAC for my host!]From (IP: %s, VID: %s) to (IP: %s, VID: %s)\n", srcip.s().c_str(), svid.vid_string().c_str(), dstip.s().c_str(),dvid.vid_string().c_str());
				p->kill();
				return NULL;
			}
		}
		// Simply forward the packet to "routing module"
		return p;
	}

	//IP pkt from another switch as ENCAPSULATED VEIL PACKET
	if(ntohs(eth->ether_type) == ETHERTYPE_VEIL){
		// First see if the packet is destined to one of my interfaces or not?
		int interface;
		if(interfaces->lookupVidEntry(&dvid, &interface)){
			veil_sub_header * vheader = (veil_sub_header *) (eth +1);

			if (ntohs(vheader->veil_type) == VEIL_ENCAP_IP){
				click_ip *i = (click_ip*) (vheader+1);
				IPAddress srcip = IPAddress(i->ip_src);
				IPAddress dstip = IPAddress(i->ip_dst);

				// CHECK for following things here:
				// 1. Is destination one of my host-devices?
				// if yes: a. overwrite the eth->ether_type to ETHERTYPE_IP
				// b. overwrite the eth->ether_dhost by the real mac address
				// return the modified packet p
				memcpy(&dvid, &vheader->dvid, 6);
				if (hosts->lookupVID(&dvid, &dmac)){
					int ip_payload_size = p->length() - sizeof(veil_sub_header) - sizeof(click_ether);
					int packetlen = p->length() - sizeof(veil_sub_header);
					// create a new packet with the length = click_ether + ip payload
					WritablePacket *q = Packet::make(packetlen);
					veil_chatter (printDebugMessages, "Process IP : Packet Length1: %d \n", q->length());
					memset(q->data(), 0, q->length());
					veil_chatter (printDebugMessages, "Process IP : Packet Length2: %d \n", q->length());

					// copy the ether_header
					memcpy(q->data(), p->data(), sizeof(click_ether));
					// now set up the Ether Header
					click_ether *q_eth = (click_ether *)q->data();
					q_eth->ether_type = htons(ETHERTYPE_IP);
					memcpy(q_eth->ether_dhost, &dmac, VID_LEN);
					memcpy(q_eth->ether_shost, &vheader->svid,6);

					// copy the ip payload
					click_ip *q_ip = (click_ip *)(q_eth+1);
					memcpy(q_ip, i,ntohs(i->ip_len));
					veil_chatter(printDebugMessages,"[... ProcessIP ][VEIL to EtherTYPE_IP][After decapsulation]From (IP: %s) to (IP: %s)\n", srcip.s().c_str(), dstip.s().c_str());
					p->kill();return q;
				}else{
					// see if packet belongs to one of my interfaces or not:
					VID intvid;
					memset(&intvid,0,6);
					memcpy(&intvid,&dvid, 4);
					int interface;
					if(interfaces->lookupVidEntry(&intvid, &interface)){
						veil_chatter(printDebugMessages,"[... ProcessIP ][VEILSwitch to VEILSwitch][NO MAC for my host!]From (IP: %s, VID: %s) to (IP: %s, VID: %s)\n", srcip.s().c_str(), svid.vid_string().c_str(), dstip.s().c_str(),dvid.vid_string().c_str());
						p->kill();
						return NULL;
					}
				}
			}else if (ntohs(vheader->veil_type) == VEIL_ENCAP_MULTIPATH_IP){
				// CHECK for following things here:
				// 1. Is destination one of my host-devices?
				// if yes: a. overwrite the eth->ether_type to ETHERTYPE_IP
				// b. overwrite the eth->ether_dhost by the real mac address
				// return the modified packet p
				veil_payload_multipath *veil_payload = (veil_payload_multipath*) (vheader + 1);
				click_ip *i = (click_ip*) (veil_payload+1);
				IPAddress srcip = IPAddress(i->ip_src);
				IPAddress dstip = IPAddress(i->ip_dst);
				memcpy(&dvid, &veil_payload->final_dvid, 6);
				if (hosts->lookupVID(&dvid, &dmac)){
					int ip_payload_size = p->length() - sizeof(veil_sub_header) - sizeof(click_ether) - sizeof(veil_payload_multipath);
					int packetlen = p->length() - sizeof(veil_sub_header)- sizeof(veil_payload_multipath);

					// create a new packet with the length = click_ether + ip payload
					WritablePacket *q = Packet::make(packetlen);
					memset(q->data(), 0, q->length());

					// copy the ether_header
					memcpy(q->data(), p->data(), sizeof(click_ether));
					// now set up the Ether Header
					click_ether *q_eth = (click_ether *)q->data();
					q_eth->ether_type = htons(ETHERTYPE_IP);
					memcpy(q_eth->ether_dhost, &dmac, VID_LEN);
					memcpy(q_eth->ether_shost, &vheader->svid,6);

					// copy the ip payload
					click_ip *q_ip = (click_ip *)(q_eth+1);
					memcpy(q_ip, i,ntohs(i->ip_len));
					veil_chatter(printDebugMessages,"[... ProcessIP ][VEIL to EtherTYPE_IP][After decapsulation]From (IP: %s) to (IP: %s)\n", srcip.s().c_str(), dstip.s().c_str());
					p->kill();return q;
				}else{
					// see if packet belongs to one of my interfaces or not:
					VID intvid;
					memset(&intvid,0,6);
					memcpy(&intvid,&dvid, 4);
					int interface;
					if(interfaces->lookupVidEntry(&intvid, &interface)){
						veil_chatter(printDebugMessages,"[... ProcessIP ][VEILSwitch to VEILSwitch][NO MAC for my host!]From (IP: %s, VID: %s) to (IP: %s, VID: %s)\n", srcip.s().c_str(), svid.vid_string().c_str(), dstip.s().c_str(),dvid.vid_string().c_str());
						p->kill();
						return NULL;
					}
				}
			}
			return p;
		}
	}

	//pkt is neither of type IP not VEIL
	//TODO:handle error
	p->kill();
	return NULL;
}

void 
VEILProcessIP::push(int, Packet* p)
{
	Packet *q = smaction(p);
	if(q != NULL)
		output(0).push(q);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessIP)

