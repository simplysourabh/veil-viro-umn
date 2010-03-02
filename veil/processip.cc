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
		//TODO: will we ever see pkts from one host to another
		//which are connected to us on the same interface?

		const click_ip *i = p->ip_header();
		IPAddress srcip = IPAddress(i->ip_src);
		IPAddress dstip = IPAddress(i->ip_dst);

		//check if dst is a host connected to us
		//if yes, replace VID with MAC
		//dvid == vid
		VID dvid = VID(eth->ether_dhost);
		VID vid, mvid;
		EtherAddress dmac;
		
		if(hosts->lookupIP(&dstip, &vid)){
			hosts->lookupVID(&dvid, &dmac);
			WritablePacket *q = p->uniqueify();
			click_ether *e = (click_ether*) q->data();
			memcpy(e->ether_dhost, &dmac, 6);
			
			p->kill();
			return q;
		}

		//check if packet is destined to someone in
		//our mapping table
		//if yes, construct VEIL_IP pkt and send to routepacket
		if(map->lookupIP(&dstip, &vid, &mvid)){		
			WritablePacket *q = Packet::make(sizeof(click_ether) + sizeof(veil_header) + sizeof(click_ip));
    			
			if (q == 0) {
        			click_chatter("in processip: cannot make packet!");		
				return NULL;
 			}
			
			click_ether *e = (click_ether *) q->data();
			q->set_ether_header(e);
			//TODO: check if this works
			memcpy(e, eth, sizeof(click_ether));
			e->ether_type = htons(ETHERTYPE_VEIL);
		
			veil_header *vheader = (veil_header *) (e+1);
			vheader->packetType = htons(VEIL_IP);

			click_ip *ip = (click_ip *) (vheader+1);
			memcpy(ip, i, sizeof(click_ip));
				
			p->kill();
			return q;
		}
		
		//TODO: any other conditions?
		//TODO: handle error pkt
		p->kill();
		return NULL;
	}

	//IP pkt from another switch
	if(ntohs(eth->ether_type) == ETHERTYPE_VEIL){
		veil_header *vhdr = (veil_header *) (eth+1);
		if(vhdr->packetType == VEIL_IP){
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
        				click_chatter("in processip: cannot make packet!");		
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
			
			//pkt is not of type VEIL_IP
			//TODO:handle error
			p->kill();
			return NULL;
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
	output(0).push(q);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessIP)

