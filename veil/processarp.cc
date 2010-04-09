#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include <click/vid.hh>
#include <click/straccum.hh>
#include "utilities.hh"
#include "click_veil.hh"
#include "processarp.hh"

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

	const click_ether *eth = (const click_ether *) p->data();
		
	//ARP pkt came from a host
	if(ntohs(eth->ether_type) == ETHERTYPE_ARP){
		VID hvid;

		EtherAddress edest = EtherAddress(eth->ether_dhost);
		EtherAddress esrc = EtherAddress(eth->ether_shost);

		const click_ether_arp *ap = (const click_ether_arp *) (eth+1);
		IPAddress src = IPAddress(ap->arp_spa);
		IPAddress dst = IPAddress(ap->arp_tpa);

		//Gratuitous ARP request or reply
		//update host table
		if(edest.is_broadcast() && src == dst)   
                {	
			VID::generate_host_vid(&myVid, &esrc, &hvid);
			host_table->updateEntry(&hvid, &esrc);
			host_table->updateIPEntry(&src, &hvid);
			interfaces->deleteHostInterface(&myVid);
			p->kill();
			return NULL;
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
			if(ntohs(ap->ea_hdr.ar_op) == ARPOP_REQUEST){
				VID dest_int;
				hvid.extract_switch_vid(&dest_int);

				//if both hosts are not connect thru
				//the same switch interface, send reply
				if(dest_int != myVid) {
					//send ARP reply
					WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(click_ether_arp));
    					if (q == 0) {
        					veil_chatter(true,"[  ProcessARP  ][Error!] in processarp: cannot make packet!");		
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
			if(map->lookupIP(&dst, &ipvid, &interfacevid)){
				WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(click_ether_arp));
    				if (q == 0) {
        				veil_chatter(true,"[  ProcessARP  ][Error!] in processarp: cannot make packet!");
        				return 0;
    				}
				click_ether *e = (click_ether *) q->data();
				q->set_ether_header(e);
				memcpy(e->ether_dhost, &esrc, 6);
				memcpy(e->ether_shost, &ipvid, 6);
				e->ether_type = htons(ETHERTYPE_ARP);

				click_ether_arp *ea = (click_ether_arp *) (e + 1);
    				ea->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
				ea->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
				ea->ea_hdr.ar_hln = 6;
				ea->ea_hdr.ar_pln = 4;
				ea->ea_hdr.ar_op = htons(ARPOP_REPLY);
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
					WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(veil_header) + sizeof(click_ether_arp));
    					if (q == 0) {
        					veil_chatter(true,"[  ProcessARP  ][Error!] in processarp: cannot make packet!");
						return 0;
    					}
					click_ether *e = (click_ether *) q->data();
					q->set_ether_header(e);
					//calculate access switch VID
					VID accvid = calculate_access_switch(&dst);					
					memcpy(e->ether_dhost, &accvid, 6);
					memcpy(e->ether_shost, &myVid, 6);
					e->ether_type = htons(ETHERTYPE_VEIL);
					veil_header *vhdr = (veil_header*) (e+1);
					vhdr->packetType = htons(VEIL_ARP_REQ);
					click_ether_arp *ea = (click_ether_arp *) (vhdr + 1);
    					ea->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
					ea->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
					ea->ea_hdr.ar_hln = 6;
					ea->ea_hdr.ar_pln = 4;
					ea->ea_hdr.ar_op = htons(ARPOP_REQUEST);
					memcpy(ea->arp_sha, &hvid, 6);
					memcpy(ea->arp_spa, &src, 4);
    					memset(ea->arp_tha, 0, 6);
					memcpy(ea->arp_tpa, &dst, 4);
			
					SET_REROUTE_ANNO(q, 'r');

					p->kill();
					return q;
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
		VID svid = VID(eth->ether_shost);
		VID dvid = VID(eth->ether_dhost);
		veil_header *vhdr = (veil_header*) (eth+1);
		const click_ether_arp *ea = (const click_ether_arp *) (vhdr+1);
		IPAddress src = IPAddress(ea->arp_spa);
		IPAddress dst = IPAddress(ea->arp_tpa);

		//ARP request: lookup mapping and host table. if found send reply. else route pkt to dst. 
		if(ntohs(vhdr->packetType) == VEIL_ARP_REQ){
			VID ipvid, mvid;
			
			//check if the packet was destined to 
			//one of our interfaces	
			int interface;
			if(interfaces->lookupVidEntry(&dvid, &interface))
			{
				if(map->lookupIP(&dst, &ipvid, &mvid) || host_table->lookupIP(&dst, &ipvid)){
					WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(veil_header) + sizeof(click_ether_arp));
    					if (q == 0) {
        					veil_chatter(true,"[  ProcessARP  ][Error!] in processarp: cannot make packet!");
        					return 0;
    					}
					click_ether *e = (click_ether *) q->data();
					q->set_ether_header(e);
					
					memcpy(e->ether_dhost, &svid, 6);
					memcpy(e->ether_shost, &dvid, 6);
					e->ether_type = htons(ETHERTYPE_VEIL);
					veil_header *vhdr = (veil_header*) (e+1);
					vhdr->packetType = htons(VEIL_ARP_RPLY);
					click_ether_arp *ea = (click_ether_arp *) (vhdr + 1);
    					ea->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
					ea->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
					ea->ea_hdr.ar_hln = 6;
					ea->ea_hdr.ar_pln = 4;
					ea->ea_hdr.ar_op = htons(ARPOP_REPLY);
					memcpy(ea->arp_sha, &ipvid, 6);
					memcpy(ea->arp_spa, &dst, 4);
    					memcpy(ea->arp_tha, &svid, 6);
					memcpy(ea->arp_tpa, &src, 4);

					p->kill();
					return q;	
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
		if(ntohs(vhdr->packetType) == VEIL_ARP_RPLY){
			VID myvid;
			EtherAddress dest;
			VID hostvid = VID(ea->arp_tha);

			if(host_table->lookupIP(&dst, &myvid)){
				map->updateEntry(&src, &svid, &myVid);
				if(host_table->lookupVID(&hostvid, &dest)){
					WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(click_ether_arp));
		    			if (q == 0) {
					        veil_chatter(true,"[  ProcessARP  ][Error!] in arp responder: cannot make packet!");
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
	if(p != NULL)
		output(0).push(p);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessARP)
