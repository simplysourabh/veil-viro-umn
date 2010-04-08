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
	return cp_va_kparse(conf, this, errh,
		"HOSTTABLE", cpkM+cpkP, cpElementCast, "VEILHostTable", &hosts,
		"MAPPINGTABLE", cpkM+cpkP, cpElementCast, "VEILMappingTable", &map,
		"INTERFACETABLE", cpkM+cpkP, cpElementCast, "VEILInterfaceTable", &interfaces,
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
		
	//if IP packet came from one of our hosts
	if(ntohs(eth->ether_type) == ETHERTYPE_IP){
		const click_ip *i = (click_ip*) (eth+1);
		IPAddress srcip = IPAddress(i->ip_src);
		IPAddress dstip = IPAddress(i->ip_dst);

		VID dvid = VID(eth->ether_dhost);
		VID vid, mvid, svid;
		EtherAddress dmac;
		EtherAddress smac = EtherAddress(eth->ether_shost);

		//check if dst is a host connected to us
		if(hosts->lookupIP(&dstip, &vid)){
			//if src and dst are connected to us
			//thru the same interface, ignore packet
			VID dvid;
			vid.extract_switch_vid(&dvid);
			if(dvid == myVid){
				p->kill();
				return NULL;
			}

			hosts->lookupMAC(&smac, &svid);
			hosts->lookupVID(&dvid, &dmac);
			memcpy(eth->ether_shost, &svid, VID_LEN);
			memcpy(eth->ether_dhost, &dmac, VID_LEN);
			return p;
		}

		//check if packet is destined to someone in
		//our mapping table
		//if yes, construct VEIL_IP pkt and send to routepacket
		//replace src with vid of src host
		if(map->lookupIP(&dstip, &vid, &mvid)){		
			WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(veil_header) + sizeof(click_ip));
    			
			if (q == 0) {
        			veil_chatter("in processip: cannot make packet!");		
				return NULL;
 			}
			
			click_ether *e = (click_ether *) q->data();
			q->set_ether_header(e);
			hosts->lookupMAC(&smac, &svid);
			memcpy(e, eth, sizeof(click_ether));
			memcpy(e->ether_shost, &svid, VID_LEN);
			e->ether_type = htons(ETHERTYPE_VEIL);
		
			veil_header *vheader = (veil_header *) (e+1);
			vheader->packetType = htons(VEIL_IP);

			click_ip *ip = (click_ip *) (vheader+1);
			memcpy(ip, i, sizeof(click_ip));
				
			p->kill();
			return q;
		}
		
		//TODO: any other conditions?
		//TODO: handle error
		p->kill();
		return NULL;
	}

	//IP pkt from another switch
	if(ntohs(eth->ether_type) == ETHERTYPE_VEIL){
		veil_header *vhdr = (veil_header *) (eth+1);
		if(ntohs(vhdr->packetType) == VEIL_IP){
			click_ip *i = (click_ip *) (vhdr+1);

			//check if destination is one of our hosts
			//if yes, create IP pkt, replace with MAC
			//else send to routepacket for forwarding
			IPAddress dst = IPAddress(i->ip_dst);
			VID hvid, myvid;
			if(hosts->lookupIP(&dst, &hvid)){
				EtherAddress dmac;
				hosts->lookupVID(&hvid, &dmac);
				
				WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(click_ip));
    			
				if (q == 0) {
        				veil_chatter("in processip: cannot make packet!");		
					return NULL;
 				}
			
				click_ether *e = (click_ether *) q->data();
				q->set_ether_header(e);
				//TODO: check if this works
				memcpy(e, eth, sizeof(click_ether));
				e->ether_type = htons(ETHERTYPE_IP);
				memcpy(e->ether_dhost, &dmac, 6);
		 					
				click_ip *ip = (click_ip *) (e+1);
				memcpy(ip, i, sizeof(click_ip));

				p->kill();
				return q;
			} else {
				return p;
			}			
		}
		//pkt is not of type VEIL_IP
		//TODO:handle error
		p->kill();
		return NULL;
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

