#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include <click/vid.hh>
#include <click/straccum.hh>
#include "utilities.hh"
#include "click_veil.hh"
#include "processarp.hh"
#include <strings.h>

CLICK_DECLS

//packets come here if ETHERTYPE_ARP || (ETHERTYPE_VEIL && (VEIL_ARP_REQ | VEIL_ARP_RPLY))

//for most of the communication, eth-src and eth-dest are switch VIDs and hosts are identified by looking at arp-src and arp-dest

VEILProcessARP::VEILProcessARP () {}

VEILProcessARP::~VEILProcessARP () {}

int
VEILProcessARP::configure (
		Vector<String> &conf,
		ErrorHandler *errh)
{
	printDebugMessages = true;
	veil_chatter_new(printDebugMessages, class_name(), "configure | WARNING: currently disards the ARP packets which has the source address as 0.0.0.0\n This is done to avoid the \"IP CONFLICT\" detection forced by certain operating systems in the network");
	return cp_va_kparse(conf, this, errh,
			"HOSTTABLE", cpkM+cpkP, cpElementCast, "VEILHostTable", &host_table,
			"MAPPINGTABLE", cpkM+cpkP, cpElementCast, "VEILMappingTable", &map,
			"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
			"PRINTDEBUG", 0, cpBool, &printDebugMessages,
			cpEnd);
}

Packet* 
VEILProcessARP::smaction(Packet* p){
	int myport = PORT_ANNO(p);

	VID myVid;
	interfaces->lookupIntEntry(myport, &myVid);

	click_ether *eth = (click_ether *) p->data();

	//ARP pkt came from a host
	if(ntohs(eth->ether_type) == ETHERTYPE_ARP){
		VID hvid;

		EtherAddress edest = EtherAddress(eth->ether_dhost);
		EtherAddress esrc = EtherAddress(eth->ether_shost);

		const click_ether_arp *ap = (const click_ether_arp *) (eth+1);
		IPAddress src = IPAddress(ap->arp_spa);
		IPAddress dst = IPAddress(ap->arp_tpa);
		IPAddress zeroip;
		memset(&zeroip, 0, 4);
		if (zeroip == src){
			// Discard the ARP packets with source IP as 0.0.0.0 to by-pass the IP conflict check.
			veil_chatter_new(true, class_name(), "configure | WARNING: Discarding the ARP query packet from 0.0.0.0,%s to %s,%s", esrc.s().c_str(), dst.s().c_str(), edest.s().c_str());
			p->kill();
			return NULL;
		}
		//host_table->generate_host_vid(src, esrc, myport, myVid, &hvid);
		bool isNew = host_table->generate_host_vid(src, esrc, myport, myVid, &hvid);
		if (isNew){
			veil_chatter_new(printDebugMessages, class_name(), "smaction | publishing the info for the new host. ip %s vid %s mac %s at interface %d", src.s().c_str(), hvid.vid_string().c_str(), esrc.s().c_str(), myport);
			// this is a new mapping. publish it.

			//--------------------------
			WritablePacket* p = publish_access_info_packet(src, esrc, hvid, printDebugMessages, class_name());
			if (p == NULL) {
				veil_chatter_new(true, class_name(), "[Error!] cannot make packet in publishaccessinfo");
			}else{
				output(0).push(p);
			}
			//--------------------------
			// create the publish access info packet and push it to the output.
			
		}
		/* TODO 
			Fix this generate_host_vid function. Instead of calling it from the VID class
			call it from the hosts class.
		*/

		veil_chatter_new(printDebugMessages, class_name(),"[ARP] From ip=%s(mac=%s,vid=%s) to ip=%s(mac=%s),", src.s().c_str(), esrc.s().c_str(), hvid.vid_string().c_str(),dst.s().c_str(),edest.s().c_str());

		//Gratuitous ARP request or reply
		//update host table
		// SJ: Let's assume, if its a broadcast packet then it is 
		// one of our directly connected host 
		// Therefore dropping the condition:  && src == dst in the if below.
		if(edest.is_broadcast()){
			//WHy were we doing this??? Deleting the interface entry ?? May be we want to separate the interfaces connected to client? SJ
			//interfaces->deleteHostInterface(&myVid);
			// IF both src IP and dst IP are same then, exit!
			if (src == dst) {
				veil_chatter_new(printDebugMessages, class_name(),"[Gratuitous ARP] IP: %s MAC: %s,", src.s().c_str(), esrc.s().c_str());
				p->kill();
				return NULL;
			}else{
				veil_chatter_new(printDebugMessages, class_name(),"[HOST UPDATE] IP: %s MAC: %s,", src.s().c_str(), esrc.s().c_str());
			}
		}

		/* host sent an ARP requesting MAC of another host 
		 * connected to the same switch. 
		 * checking if 'dst' is in host_table works for 
		 * both requests and replies. ignore replies
		 * and requests coming from hosts connected to 
		 * the switch thru the same interface say thru a hub
		 */
		if(host_table->lookupIP(&dst, &hvid))
		{
			veil_chatter_new(printDebugMessages, class_name(),"[DestIP-IsMyHost] Dest IP: %s SrcVID: %s, packet came on Interface: %s,", dst.s().c_str(), hvid.vid_string().c_str(), myVid.switchVIDString().c_str());
			if(ntohs(ap->ea_hdr.ar_op) == ARPOP_REQUEST){
				VID dest_int;
				hvid.extract_switch_vid(&dest_int);

				//if both hosts are not connect thru
				//the same switch interface, send reply
				if(dest_int != myVid) {
					veil_chatter_new(printDebugMessages, class_name(),"[ARP Reply] Sending Reply for IP: %s SrcVID: %s, packet came on Interface: %s,", dst.s().c_str(), hvid.vid_string().c_str(), myVid.switchVIDString().c_str());
					//send ARP reply
					WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(click_ether_arp));
					if (q == 0) {
						veil_chatter_new(true, class_name(),"[Error!] in processarp: cannot make packet!");
						return 0;
					}
					click_ether *e = (click_ether *) q->data();
					q->set_ether_header(e);
					memcpy(e->ether_dhost, &esrc, 6);
					memcpy(e->ether_shost, &hvid, 6);
					e->ether_type = htons(ETHERTYPE_ARP);
					click_ether_arp *ea = (click_ether_arp *) (e + 1);
					ea->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
					ea->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
					ea->ea_hdr.ar_hln = 6;
					ea->ea_hdr.ar_pln = 4;
					ea->ea_hdr.ar_op = htons(ARPOP_REPLY);
					memcpy(ea->arp_sha, &hvid, 6);
					memcpy(ea->arp_spa, &dst, 4);
					memcpy(ea->arp_tha, &esrc, 6);
					memcpy(ea->arp_tpa, &src, 4);

					p->kill();
					return q;
				}
			}			
			p->kill();
			return NULL;
		}

		//host sent an ARP request 
		if(ntohs(ap->ea_hdr.ar_op) == ARPOP_REQUEST){
			VID ipvid, interfacevid;
			//found MAC in mapping table
			//construct ARP reply
			veil_chatter_new(printDebugMessages, class_name(),"[ARP Req] Looking into my MAPPING TABLE for IP: %s SrcVID: %s, packet came on Interface: %s,", dst.s().c_str(), hvid.vid_string().c_str(), myVid.switchVIDString().c_str());
			if(map->lookupIP(&dst, &ipvid, &interfacevid)){
				veil_chatter_new(printDebugMessages, class_name(),"[ARP Req] Found!! Mapping for IP: %s SrcVID: %s in my MAPPING TABLE mapped to: %s ", dst.s().c_str(), hvid.vid_string().c_str(), ipvid.vid_string().c_str());

				short opcode = htons(ARPOP_REPLY);
				short etypeip = htons(ETHERTYPE_IP);
				short htypeether = htons(ARPHRD_ETHER);
				short ethertype = htons(ETHERTYPE_ARP);

				WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(click_ether_arp));
				if (q == 0) {
					veil_chatter_new(true, class_name(),"[Error!] in processarp: cannot make packet!");
					return 0;
				}
				click_ether *e = (click_ether *) q->data();
				q->set_ether_header(e);
				memcpy(e->ether_dhost, &esrc, 6);
				memcpy(e->ether_shost, &ipvid, 6);
				e->ether_type = ethertype;

				click_ether_arp *ea = (click_ether_arp *) (e + 1);
				ea->ea_hdr.ar_hrd = htypeether;
				ea->ea_hdr.ar_pro = etypeip;
				ea->ea_hdr.ar_hln = 6;
				ea->ea_hdr.ar_pln = 4;
				ea->ea_hdr.ar_op = opcode;
				memcpy(ea->arp_sha, &ipvid, 6);
				memcpy(ea->arp_spa, &dst, 4);
				memcpy(ea->arp_tha, &esrc, 6);
				memcpy(ea->arp_tpa, &src, 4);

				p->kill();
				return q;
			}

			/* required MAC was not in the mapping table
			 * construct an ARP request dst'd to access switch. 
			 * When we rcv a reply to this req in the future,
			 * we need to know which host wanted it. Hence
			 * src_eth = host_vid and not switch vid
			 */
			else{		
				//find host VID in host table
				//TODO: do we always find host VID at this stage?
				//TODO: is tpa 0 because we don't know it? 
				if(host_table->lookupIP(&src, &hvid))
				{
					//calculate access switch VID
					VID accvid = calculate_access_switch(&dst);					

					veil_chatter_new(printDebugMessages, class_name(),"[ARP Req][ToAccessSwitch] Asking for mapping for IP: %s SrcVID: %s to access switch: %s ", dst.s().c_str(), hvid.vid_string().c_str(), accvid.switchVIDString().c_str());
					WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(veil_sub_header) + sizeof(click_ether_arp));
					if (q == 0) {
						veil_chatter_new(true, class_name(),"[Error!] in processarp: cannot make packet!");
						return 0;
					}
					click_ether *e = (click_ether *) q->data();
					q->set_ether_header(e);

					//TODO This has to be written inside the "RoutePacket.cc"
					memcpy(e->ether_dhost, &accvid, 6);
					memcpy(e->ether_shost, &hvid, 6);
					e->ether_type = htons(ETHERTYPE_VEIL);

					veil_sub_header *vheader = (veil_sub_header*) (e+1);
					memcpy(&vheader->dvid, &accvid, 6);
					memcpy(&vheader->svid, &hvid, 6);
					vheader->veil_type = htons(VEIL_ENCAP_ARP);
					vheader->ttl = MAX_TTL;

					click_ether_arp *arp_payload = (click_ether_arp *) (vheader + 1);
					arp_payload->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
					arp_payload->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
					arp_payload->ea_hdr.ar_hln = 6;
					arp_payload->ea_hdr.ar_pln = 4;
					arp_payload->ea_hdr.ar_op = htons(ARPOP_REQUEST);
					memcpy(arp_payload->arp_sha, &hvid, 6);
					memcpy(arp_payload->arp_spa, &src, 4);
					memset(arp_payload->arp_tha, 0, 6);
					memcpy(arp_payload->arp_tpa, &dst, 4);

					SET_REROUTE_ANNO(q, 'r');

					p->kill();
					return q;
				}else{
					veil_chatter_new(printDebugMessages, class_name(),"[ARP Req][Error!] HOSTTABLE Dont have the mapping for IP: %s", src.s().c_str());
					p->kill();
					return NULL;
				}		
			}
		}		

		//all other packets should not have come here
		//TODO: handle error. for now kill p.
		//TODO: any other use cases?
		p->kill();
		return NULL;
	}

	//packet came from another switch
	if(ntohs(eth->ether_type) == ETHERTYPE_VEIL){
		// FIRST CHECK IF PACKET IS DESTINED TO US OR NOT:
		VID dvid = VID(eth->ether_dhost);
		int intnum;
		if (!interfaces->lookupVidEntry(&dvid, &intnum)){
			p->kill();
			return NULL;
		}

		veil_sub_header *vheader = (veil_sub_header*) (eth+1);
		VID svid;
		memcpy (&svid, &vheader->svid,6);
		memcpy (&dvid, &vheader->dvid,6);

		click_ether_arp *arp_payload = (click_ether_arp *) (vheader+1);
		IPAddress src = IPAddress(arp_payload->arp_spa);
		IPAddress dst = IPAddress(arp_payload->arp_tpa);
		EtherAddress hmac = EtherAddress(arp_payload->arp_sha);

		veil_chatter_new(printDebugMessages, class_name(),"[VEIL_ENCAP_ARP] For IP: %s by SrcVID: %s to DstVID: %s.", dst.s().c_str(),  svid.vid_string().c_str(), dvid.vid_string().c_str());

		//ARP request: lookup mapping and host table. if found send reply. else route pkt to dst. 
		if((ntohs(vheader->veil_type) == VEIL_ENCAP_ARP) && (ntohs(arp_payload->ea_hdr.ar_op) == ARPOP_REQUEST)){
			VID ipvid, mvid;
			//check if the packet was destined to 
			//one of our interfaces	
			int interface;
			if(interfaces->lookupVidEntry(&dvid, &interface))
			{
				if(map->lookupIP(&dst, &ipvid, &mvid) || host_table->lookupIP(&dst, &ipvid)){
					veil_chatter_new(printDebugMessages, class_name(),"[VEIL_ENCAP_ARP][Mapping Found] For IP: %s to VID: %s ", dst.s().c_str(),  ipvid.vid_string().c_str());

					//TODO Make sure that routepacket takes care of it.
					memcpy(eth->ether_dhost, &svid, 6);
					// put the eth->ether_shost as one of my own
					// interface addresses. 
					// for now i am putting the 0th interface's vid.
					memcpy(eth->ether_shost, &interfaces->rinterfaces[0],6);
					// NO NEED TO CHANGE eth->ethertype = htons(ETHERTYPE_VEIL);

					// update the veil_sub_header now
					memcpy(&vheader->dvid, &svid,6);
					memcpy(&vheader->svid, &ipvid,6);
					vheader->ttl = MAX_TTL;
					vheader->veil_type = htons(VEIL_ENCAP_ARP);

					// update the arp payload
					arp_payload->ea_hdr.ar_op = htons(ARPOP_REPLY);
					memcpy(arp_payload->arp_sha, &ipvid, 6);
					memcpy(arp_payload->arp_spa, &dst, 4);
					memcpy(arp_payload->arp_tha, &svid, 6);
					memcpy(arp_payload->arp_tpa, &src, 4);

					return p;
				}else{
					veil_chatter_new(printDebugMessages, class_name(),"[VEIL_ENCAP_ARP][NO Mapping Found] For IP: %s", dst.s().c_str());
					p->kill();return NULL;
				}							
			}
			/* we're not the access switch or destination
			 * set this annotation to tell the 
			 * routing element it needs to 
			 * find the best access switch
			 */
			SET_REROUTE_ANNO(p, 'r');
			return p;
		}

		//ARP reply: if destined to our host. update
		//mapping table and send ARP reply to host	
		if((ntohs(vheader->veil_type) == VEIL_ENCAP_ARP) && (ntohs(arp_payload->ea_hdr.ar_op) == ARPOP_REPLY)){
			VID myvid;
			EtherAddress dest;
			VID hostvid = VID(arp_payload->arp_tha);

			if(host_table->lookupIP(&dst, &myvid)){
				//veil_chatter_new(printDebugMessages, class_name(),"[VEIL_ENCAP_ARP][ARP Reply] To my HOST IP: %s at VID: %s ", dst.s().c_str(),  myvid.vid_string().c_str());
				
				// SJ: Do not cache the mapping locally. it may create problems 
				// when hosts move around more frequently.
				//map->updateEntry(src, svid, myVid, hmac );
				if(host_table->lookupVID(&hostvid, &dest)){
					veil_chatter_new(printDebugMessages, class_name(),"[VEIL_ENCAP_ARP][ARP Reply] To my HOST IP: %s  HostVID: %s ", dst.s().c_str(),  hostvid.vid_string().c_str());
					WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(click_ether_arp));
					if (q == 0) {
						veil_chatter_new(true, class_name(),"[Error!] in arp responder: cannot make packet!");
						return 0;
					}

					click_ether *e = (click_ether *) q->data();
					q->set_ether_header(e);
					memcpy(e->ether_dhost, dest.data(), 6);
					memcpy(e->ether_shost, &svid, 6);
					e->ether_type = htons(ETHERTYPE_ARP);

					click_ether_arp *ea = (click_ether_arp *) (e + 1);
					ea->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);

					ea->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
					ea->ea_hdr.ar_hln = 6;
					ea->ea_hdr.ar_pln = 4;
					ea->ea_hdr.ar_op = htons(ARPOP_REPLY);
					memcpy(ea->arp_sha, &svid, 6);
					memcpy(ea->arp_spa, &src, 4);
					memcpy(ea->arp_tha, dest.data(), 6);
					memcpy(ea->arp_tpa, &dst, 4);

					p->kill();
					return q;
				}	
			} else {
				//packet was not destined to us, fwd it
				return p;
			}
		}	

		//pkt is ETHERTYPE VEIL but not VEIL REQ or VEIL RPLY
		//TODO: handle error. for now kill p
		//TODO: any other use cases?
		p->kill();
		return NULL;			
	}

	//packet is neither ETHERTYPE ARP nor ETHERTYPE VEIL
	//TODO: handle error. for now kill p.
	p->kill();
	return NULL;
}

void
VEILProcessARP::push (
		int port,
		Packet* pkt)
{
	Packet *p = smaction(pkt);
	if(p != NULL){
		//veil_chatter_new(printDebugMessages, class_name(),"[VEIL_ENCAP_ARP][ARP Reply] BEFORE Pushing the packet to be routed!!");
		output(0).push(p);
		//veil_chatter_new(printDebugMessages, class_name(),"[VEIL_ENCAP_ARP][ARP Reply] AFTER Pushing the packet to be routed!!");	
	}
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessARP)
