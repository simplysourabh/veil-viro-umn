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
	forwarding_type = IP_FORWARDING_TYPE; //click_veil.hh for the documentation
	int retval =  cp_va_kparse(conf, this, errh,
			"HOSTTABLE", cpkM+cpkP, cpElementCast, "VEILHostTable", &hosts,
			"MAPPINGTABLE", cpkM+cpkP, cpElementCast, "VEILMappingTable", &map,
			"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
			"FORWARDING_TYPE",cpkP, cpByte, &forwarding_type,
			"PRINTDEBUG", 0, cpBool, &printDebugMessages,
			cpEnd);
	switch(forwarding_type){
		case IP_FORWARDING_TYPE_ADD_OVERWRITE:
			veil_chatter_new(true,class_name(),"[ProcessIP]Forwarding Type: Default Address overwriting on Ethernet headers");
			break;
		case IP_FORWARDING_TYPE_ENCAP:
			veil_chatter_new(true,class_name(),"[ProcessIP]Forwarding Type: Insert the VEIL SHIM header");
			break;
		case IP_FORWARDING_TYPE_MULTIPATH:
			veil_chatter_new(true,class_name(),"[ProcessIP]Forwarding Type: Multipath Routing + VEIL SHIM header");
			break;
		default:
			veil_chatter_new(true,class_name(),"[ProcessIP]Forwarding Type: Unknown! Use one of the following values [0: IP_FORWARDING_TYPE_ADD_OVERWRITE,1: IP_FORWARDING_TYPE_ENCAP or 2: IP_FORWARDING_TYPE_MULTIPATH]");
			forwarding_type = IP_FORWARDING_TYPE_ENCAP;
			veil_chatter_new(true,class_name(),"[ProcessIP]Using Forwarding Type: Insert the VEIL SHIM header");
			break;
	}

	return retval;

}

void 
VEILProcessIP::send_arp_query_packet(IPAddress srcip, IPAddress dstip, VID srcvid, VID myvid){
	// we send an artificial veil_arp query packet to the access switch.
	//calculate access switch VID
	VID accvid = calculate_access_switch(&dstip);                                     

	veil_chatter_new(printDebugMessages, class_name()," send_arp_query_packet |  Asking for mapping for IP: %s SrcVID: %s to access switch: %s ", dstip.s().c_str(), srcvid.vid_string().c_str(), accvid.switchVIDString().c_str());
	WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(veil_sub_header) + sizeof(click_ether_arp));
	if (q == 0) {
		veil_chatter_new(true, class_name(),"[Error!] in processarp: cannot make packet!");
		return;
	}
	click_ether *e = (click_ether *) q->data();
	q->set_ether_header(e);

	memcpy(e->ether_dhost, &accvid, 6);
	memcpy(e->ether_shost, &myvid, 6);
	e->ether_type = htons(ETHERTYPE_VEIL);

	veil_sub_header *vheader = (veil_sub_header*) (e+1);
	memcpy(&vheader->dvid, &accvid, 6);
	memcpy(&vheader->svid, &srcvid, 6);
	vheader->veil_type = htons(VEIL_ENCAP_ARP);
	vheader->ttl = MAX_TTL;

	click_ether_arp *arp_payload = (click_ether_arp *) (vheader + 1);
	arp_payload->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
        arp_payload->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
	arp_payload->ea_hdr.ar_hln = 6;
	arp_payload->ea_hdr.ar_pln = 4;
	arp_payload->ea_hdr.ar_op = htons(ARPOP_REQUEST);
	memcpy(arp_payload->arp_sha, &srcvid, 6);
	memcpy(arp_payload->arp_spa, &srcip, 4);
	memset(arp_payload->arp_tha, 0, 6);
	memcpy(arp_payload->arp_tpa, &dstip, 4);
	SET_REROUTE_ANNO(q, 'r');
	
	output(0).push(q);
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

	veil_chatter_new(printDebugMessages, class_name(),"[ProcessIP][PACKETIN] From (Mac: %s) to (VID: %s),",  smac.s().c_str(), dvid.vid_string().c_str());

	//if IP packet came from one of our hosts
	if(ntohs(eth->ether_type) == ETHERTYPE_IP){
		click_ip *ip_header = (click_ip*) (eth+1);
		IPAddress srcip = IPAddress(ip_header->ip_src);
		IPAddress dstip = IPAddress(ip_header->ip_dst);

		//Register "source ip, source mac" to a vid with myvid.	
		bool isNew = hosts->generate_host_vid(srcip, smac, myport, myVid, &svid);
		if (isNew){
			veil_chatter_new(printDebugMessages, class_name(), "smaction | publishing the info for the new host. ip %s vid %s mac %s at interface %d", srcip.s().c_str(), svid.vid_string().c_str(), smac.s().c_str(), myport);
			// this is a new mapping. publish it.
			//--------------------------
			WritablePacket* p = publish_access_info_packet(srcip, smac, svid, printDebugMessages, class_name());
			if (p == NULL) {
				veil_chatter_new(true, class_name(), "[Error!] cannot make packet in publishaccessinfo");
			}else{
				output(0).push(p);
			}
			//--------------------------
			// create the publish access info packet and push it to the output.
			
		}
		//veil_chatter_new(true, class_name(), "PacketLength : %d ", ntohs(ip_header->ip_len));
		//check if dst is a host connected to us
		VID sipvid, dipvid; 
		if(hosts->lookupIP(&dstip, &dvid)){
			//if src and dst are connected to us
			//thru the same interface, ignore packet
			dvid.extract_switch_vid(&vid);
			if(vid == myVid){
				veil_chatter_new(printDebugMessages, class_name(),"[ProcessIP][EtherTYPE_IP][Both Host are on same interface] From (IP: %s, Mac: %s) to (IP: %s, VID: %s),", srcip.s().c_str(), smac.s().c_str(), dstip.s().c_str(),dvid.vid_string().c_str());

				// see if the source and destnation address on the ether header are not using 
				// the vids for the hosts.

				// first fetch the svid, dvid for hosts, and compare it with the mac addresses 
				// used on the packet. 
				if(hosts->lookupIP(&dstip, &dipvid)){
					EtherAddress dipmac;
					if(hosts->lookupVID(&dipvid, &dipmac)){
						if(dmac != dipmac){
							// update the destination mac address on the packet.
							memcpy(eth->ether_dhost, dipmac.data(), 6);
							veil_chatter_new(printDebugMessages, class_name(), "Both source and destination are on the same lan, but vid %s was used to address the destination instead of the MAC %s.", dvid.vid_string().c_str(), dipmac.s().c_str());
							// push the packet to be routed.
							return p;
						}
					}
				}
				// if the 
				p->kill();
				return NULL;
			}

			hosts->lookupMAC(&smac, &svid);
			hosts->lookupVID(&dvid, &dmac);
			memcpy(eth->ether_shost, &svid, VID_LEN);
			memcpy(eth->ether_dhost, &dmac, VID_LEN);
			veil_chatter_new(printDebugMessages, class_name(),"[EtherTYPE_IP][Host are on different interfaces] From (IP: %s, Mac: %s) to (IP: %s, VID: %s), After add rewriting: From (IP: %s, VID: %s) to (IP: %s, Mac: %s) ", srcip.s().c_str(), smac.s().c_str(), dstip.s().c_str(),dvid.vid_string().c_str() , srcip.s().c_str(), svid.vid_string().c_str(), dstip.s().c_str(),dmac.s().c_str());
			return p;
		}


		// For every other packet we can simply overwrite the "source mac address" by the corresponding VID.
		else if (forwarding_type == IP_FORWARDING_TYPE_ADD_OVERWRITE){
			// do two things:
			// 1. Overwrite eth->ether_type by the ETHERTYPE_VEIL_IP
			// 2. overwrite "source mac" eth-> ether_shost by source vid.
			short etype = ETHERTYPE_VEIL_IP;
			eth->ether_type = htons(etype);
			memcpy(eth->ether_shost, &svid, VID_LEN); 
			veil_chatter_new(printDebugMessages, class_name(),"[EtherTYPE_IP][Encapsulation to VEIL_IP type] From (IP: %s, Mac: %s) to (IP: %s, VID: %s), After add rewriting: From (IP: %s, VID: %s)", srcip.s().c_str(), smac.s().c_str(), dstip.s().c_str(),dvid.vid_string().c_str() , srcip.s().c_str(), svid.vid_string().c_str());
			return p;
		} 
		else if (forwarding_type == IP_FORWARDING_TYPE_ENCAP){
			WritablePacket *q = p->push(sizeof(veil_sub_header));

			// set the ether header
			click_ether* q_eth = (click_ether*)(q->data());
			memcpy(q_eth->ether_dhost,&dvid, 6);
			memcpy(q_eth->ether_shost,&svid, 6);
			q_eth->ether_type = htons(ETHERTYPE_VEIL);

			//insert the veil_sub_header
			veil_sub_header *q_vheader = (veil_sub_header*) (q_eth + 1);
			memcpy(&q_vheader->dvid, &dvid, 6);
			memcpy(&q_vheader->svid, &svid, 6);
			q_vheader->veil_type = htons(VEIL_ENCAP_IP);
			q_vheader->ttl = MAX_TTL;

			SET_PORT_ANNO(q, myport);
			return q;
		}else if(forwarding_type == IP_FORWARDING_TYPE_MULTIPATH){

			/*
			 * Create a new packet and copy the contents on to it.
			 * It was less efficient therefore trying to reuse the same old packet P
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
			 */
			//veil_chatter_new(true, class_name(),"Packet Length Before PUSH: %d Headroom: %d ", p->length(), p->headroom());
			WritablePacket *q = p->push(sizeof(veil_sub_header) + sizeof(veil_payload_multipath));
			//veil_chatter_new(true, class_name(),"Packet Length after PUSH: %d Headroom: %d ", p->length(), p->headroom());
			// set the ether header
			click_ether* q_eth = (click_ether*)(q->data());
			memcpy(q_eth->ether_dhost,&dvid, 6);
			memcpy(q_eth->ether_shost,&svid, 6);
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

			SET_PORT_ANNO(q, myport);
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
			veil_chatter_new(printDebugMessages, class_name(),"[VEIL to EtherTYPE_IP] After add rewriting: From (IP: %s, VID: %s) to (IP: %s, VID: %s, MAC: %s)", srcip.s().c_str(), svid.vid_string().c_str(), dstip.s().c_str(), dvid.vid_string().c_str(), dmac.s().c_str());
		}else{
			// see if packet belongs to one of my interfaces or not:
			VID intvid;
			memset(&intvid,0,6);
			memcpy(&intvid,&dvid, 4);
			int interface;
			if(interfaces->lookupVidEntry(&intvid, &interface)){
				veil_chatter_new(printDebugMessages, class_name(),"[VEILSwitch to VEILSwitch][NO MAC for my host!]From (IP: %s, VID: %s) to (IP: %s, VID: %s)", srcip.s().c_str(), svid.vid_string().c_str(), dstip.s().c_str(),dvid.vid_string().c_str());
				
				send_arp_query_packet(srcip, dstip, svid, myVid);
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
		veil_sub_header * vheader = (veil_sub_header *) (eth +1);
		memcpy(&svid, &vheader->svid,6);
		if(interfaces->lookupVidEntry(&dvid, &interface)){
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
					//veil_chatter_new(true, class_name(),"Packet Length Before PULL: %d ", p->length());
					// remove the shim header
					p->pull( (sizeof(veil_sub_header)) );
					//veil_chatter_new(true, class_name(),"Packet Length after PULL: %d ", p->length());
					// copy the ether_header
					// now set up the Ether Header
					click_ether *q_eth = (click_ether *)p->data();
					q_eth->ether_type = htons(ETHERTYPE_IP);
					memcpy(q_eth->ether_dhost, &dmac, VID_LEN);
					memcpy(q_eth->ether_shost, &svid,6);

					veil_chatter_new(printDebugMessages, class_name(),"[VEIL to EtherTYPE_IP][After decapsulation 1]From (IP: %s) to (IP: %s)", srcip.s().c_str(), dstip.s().c_str());
					return p;

				}else{
					// see if packet belongs to one of my interfaces or not:
					VID intvid;
					memset(&intvid,0,6);
					memcpy(&intvid,&dvid, 4);
					int interface;
					if(interfaces->lookupVidEntry(&intvid, &interface)){
						veil_chatter_new(printDebugMessages, class_name(),"[VEILSwitch to VEILSwitch][NO MAC for my host!]From (IP: %s, VID: %s) to (IP: %s, VID: %s)", srcip.s().c_str(), svid.vid_string().c_str(), dstip.s().c_str(),dvid.vid_string().c_str());
						send_arp_query_packet(srcip, dstip, svid, myVid);
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

					/*
					 * Create a new packet and copy the contents on to it.
					 * It was less efficient therefore trying to reuse the same old packet P
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
						veil_chatter_new(printDebugMessages, class_name(),"[VEIL to EtherTYPE_IP][After decapsulation]From (IP: %s) to (IP: %s)", srcip.s().c_str(), dstip.s().c_str());
						p->kill();return q;
					 */

					//veil_chatter_new(true, class_name(),"Packet Length Before PULL: %d ", p->length());
					// remove the shim header
					p->pull( (sizeof(veil_sub_header)+sizeof(veil_payload_multipath)) );
					//veil_chatter_new(true, class_name(),"Packet Length after PULL: %d ", p->length());
					// copy the ether_header
					// now set up the Ether Header
					click_ether *q_eth = (click_ether *)p->data();
					q_eth->ether_type = htons(ETHERTYPE_IP);
					memcpy(q_eth->ether_dhost, &dmac, VID_LEN);
					memcpy(q_eth->ether_shost, &svid,6);

					veil_chatter_new(printDebugMessages, class_name(),"[VEIL to EtherTYPE_IP][After decapsulation 2]From (IP: %s) to (IP: %s)", srcip.s().c_str(), dstip.s().c_str());
					return p;

				}else{
					// see if packet belongs to one of my interfaces or not:
					VID intvid;
					memset(&intvid,0,6);
					memcpy(&intvid,&dvid, 4);
					int interface;
					if(interfaces->lookupVidEntry(&intvid, &interface)){
						veil_chatter_new(printDebugMessages, class_name(),"[VEILSwitch to VEILSwitch][NO MAC for my host!]From (IP: %s, VID: %s) to (IP: %s, VID: %s)", srcip.s().c_str(), svid.vid_string().c_str(), dstip.s().c_str(),dvid.vid_string().c_str());
						send_arp_query_packet(srcip, dstip, svid, myVid);
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
