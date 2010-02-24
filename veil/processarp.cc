#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include <click/vid.hh>
#include "utilities.hh"
#include "click_veil.hh"
#include "processarp.hh"

CLICK_DECLS

//packets come here if ETHERTYPE_ARP || (ETHERTYPE_VEIL && (VEIL_ARP_REQ | VEIL_ARP_RPLY))

VEILProcessARP::VEILProcessARP () {}

VEILProcessARP::~VEILProcessARP () {}

int
VEILProcessARP::configure (
	Vector<String> &conf,
	ErrorHandler *errh)
{
	return cp_va_kparse(conf, this, errh,
		"MYVID", cpkP+cpkM, cpVid, &myVid,
		//"VEILGENERATEVID", cpkP+cpkM, cpElementCast, "VEILGenerateVid", &genVid,
		"HOSTTABLE", cpkM+cpkP, cpElementCast, "VEILHostTable", &host_table,
		"MAPPINGTABLE", cpkM+cpkP, cpElementCast, "VEILMappingTable", &map,
		cpEnd);
}

Packet* 
VEILProcessARP::smaction(Packet* p){
	const click_ether *eth = (const click_ether *) p->mac_header();

	//ARP pkt came from a host
	if(ntohs(eth->ether_type) == ETHERTYPE_ARP){
		VID *hvid= new VID();

		//TODO: are both ARP-dest-mac and eth-dest-mac ffffff for 
		//gratuitous ARP?
		EtherAddress edest = EtherAddress(eth->ether_dhost);
		//TODO: are we supposed to be looking at src in ARP?
		//are eth-src and ARP-src always the same?
		EtherAddress esrc = EtherAddress(eth->ether_shost);

		const click_ether_arp *ap = (const click_ether_arp *) p->network_header();
		IPAddress src = IPAddress(ap->arp_spa);
		IPAddress dst = IPAddress(ap->arp_tpa);

		//Gratuitous ARP request or reply
		//update host table
		if(edest.is_broadcast() && src == dst)   
                {				
			VID::generate_host_vid(&myVid, &esrc, hvid);
			host_table->updateEntry(hvid, &esrc);
			host_table->updateIPEntry(&src, hvid);
			delete hvid;
			p->kill();
			return NULL;
		}
		
		/* host sent an ARP requesting MAC of another host 
		 * connected to the same switch. 
		 * checking if 'dst' is in host_table works for 
		 * both requests and replies. ignore them all.
		 */
		if(host_table->lookupIP(&dst, hvid))
		{
			delete hvid;			
			p->kill();
			return NULL;
		}

		//host sent an ARP request 
		if(ntohs(ap->ea_hdr.ar_op) == ARPOP_REQUEST){
			VID *ipvid = new VID();
			VID *myvid = new VID();
			//found MAC in mapping table
		        //construct ARP reply
			if(map->lookupIP(&dst, ipvid, myvid)){
				WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(click_ether_arp));
    				if (q == 0) {
        				click_chatter("in processarp: cannot make packet!");
					delete hvid;
					delete ipvid;
					delete myvid;

        				return 0;
    				}
				click_ether *e = (click_ether *) q->data();
				q->set_ether_header(e);
				memcpy(e->ether_dhost, &esrc, 6);
				memcpy(e->ether_shost, ipvid, 6);
				click_ether_arp *ea = (click_ether_arp *) (e + 1);
    				ea->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
				ea->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
				ea->ea_hdr.ar_hln = 6;
				ea->ea_hdr.ar_pln = 4;
				ea->ea_hdr.ar_op = htons(ARPOP_REPLY);
				memcpy(ea->arp_sha, ipvid, 6);
				memcpy(ea->arp_spa, &dst, 4);
    				memcpy(ea->arp_tha, &esrc, 6);
				memcpy(ea->arp_tpa, &src, 4);
				
				delete hvid;
				delete ipvid;
				delete myvid;

				p->kill();
				return q;
			}

			/* required MAC was not in the mapping table
			 * construct an ARP request dst'd to access switch. 
                         * When we rcv a reply to this req in the future, 
			 * we need to know which host wanted it. Hence 				 * src_eth = host_vid and not switch vid
			 */
			else{		
				//find host VID in host table
				//TODO: what happens if host VID is not found? should we add it to host_table? 
				//TODO: is tpa 0 because we don't know it? 
                                if(host_table->lookupIP(&src, hvid))
                                {
					WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(veil_header) + sizeof(click_ether_arp));
    					if (q == 0) {
        					click_chatter("in processarp: cannot make packet!");
						delete hvid;
						delete ipvid;
						delete myvid;
        	
						return 0;
    					} 						click_ether *e = (click_ether *) q->data();
					q->set_ether_header(e);
					//calculate access switch VID
					VID accvid = calculate_access_switch(&dst);					
					memcpy(e->ether_dhost, &accvid, 6);
					memcpy(e->ether_shost, hvid, 6); 						e->ether_type = htons(ETHERTYPE_VEIL);
					veil_header *vhdr = (veil_header*) (e+1);
					vhdr->packetType = htons(VEIL_ARP_REQ);
					click_ether_arp *ea = (click_ether_arp *) (vhdr + 1);
    					ea->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
					ea->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
					ea->ea_hdr.ar_hln = 6;
					ea->ea_hdr.ar_pln = 4;
					ea->ea_hdr.ar_op = htons(ARPOP_REQUEST);
					memcpy(ea->arp_sha, hvid, 6);
					memcpy(ea->arp_spa, &src, 4);
    					memset(ea->arp_tha, 0, 6);
					memcpy(ea->arp_tpa, &dst, 4);

					delete hvid;
					delete ipvid;
					delete myvid;

					p->kill();
					return q;
				}		
			}
		}		

		//all other packets should not have come here
		//TODO: handle error. for now just return p.
		//TODO: any other use cases?
		delete hvid;
		return p;
	}

	//packet came from another switch
	if(ntohs(eth->ether_type) == ETHERTYPE_VEIL){
		VID svid = VID(eth->ether_shost);
		VID dvid = VID(eth->ether_dhost);
		veil_header *vhdr = (veil_header*) (eth+1);
		const click_ether_arp *ea = (const click_ether_arp *) (vhdr+1);
		IPAddress src = IPAddress(ea->arp_spa);
		IPAddress dst = IPAddress(ea->arp_tpa);

		//ARP request: lookup host (and mapping table?). if found send reply. else route pkt to dst.		
		//TODO: is eth-src the vid found in host_table for both eth- header and arp-header? 		 
		if(vhdr->packetType == VEIL_ARP_REQ){
			VID *ipvid = new VID();
			if(host_table->lookupIP(&dst, ipvid))
			{
				WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(veil_header) + sizeof(click_ether_arp));
    					if (q == 0) {
        					click_chatter("in processarp: cannot make packet!");
						delete ipvid;
        					return 0;
    					} 						click_ether *e = (click_ether *) q->data();
					q->set_ether_header(e);
					
					memcpy(e->ether_dhost, &svid, 6);
					memcpy(e->ether_shost, ipvid, 6); 						e->ether_type = htons(ETHERTYPE_VEIL);
					veil_header *vhdr = (veil_header*) (e+1);
					vhdr->packetType = htons(VEIL_ARP_RPLY);
					click_ether_arp *ea = (click_ether_arp *) (vhdr + 1);
    					ea->ea_hdr.ar_hrd = htons(ARPHRD_ETHER);
					ea->ea_hdr.ar_pro = htons(ETHERTYPE_IP);
					ea->ea_hdr.ar_hln = 6;
					ea->ea_hdr.ar_pln = 4;
					ea->ea_hdr.ar_op = htons(ARPOP_REPLY);
					memcpy(ea->arp_sha, ipvid, 6);
					memcpy(ea->arp_spa, &dst, 4);
    					memcpy(ea->arp_tha, &svid, 6);
					memcpy(ea->arp_tpa, &src, 4);

					delete ipvid;
					p->kill();
					return q;	
			}
			else {
				/* we're not the access switch
				 * set this annotation to tell the 
				 * routing element it needs to 
				 * find the best access switch
				 */
				SET_REROUTE_ANNO(p, 'r');
				delete ipvid;
				return p;			
			}
		}

		//ARP reply: destined to some host. update
		//mapping table. send ARP reply to host	
		if(vhdr->packetType == VEIL_ARP_RPLY){
			map->updateEntry(&src, &svid, &myVid);
			EtherAddress *dest = new EtherAddress();
			host_table->lookupVID(&dvid, dest);
			
			WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(click_ether_arp));
    			if (q == 0) {
			        click_chatter("in arp responder: cannot make packet!");
				delete dest;
			        return 0;
    			}

    			click_ether *e = (click_ether *) q->data();
			q->set_ether_header(e);
			memcpy(e->ether_dhost, dest->data(), 6);
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
			memcpy(ea->arp_tha, dest->data(), 6);
			memcpy(ea->arp_tpa, &dst, 4);
			
			delete dest;
			p->kill();
			return q;			
		}	
		
		//all other packets should not have come here
		//TODO: handle error. for now just return p.
		//TODO: any other use cases?
		return p;			
	}

	//all other packets should not have come here
	//TODO: handle error. for now just return p.
	return p;
}

void
VEILProcessARP::push (
	int port,
	Packet* pkt)
{
	Packet *p = smaction(pkt);
	output(0).push(p);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessARP)
