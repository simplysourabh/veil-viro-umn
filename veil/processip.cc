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
	click_ip *i = (click_ip*) (eth+1);
	IPAddress srcip = IPAddress(i->ip_src);
	IPAddress dstip = IPAddress(i->ip_dst);

	VID dvid = VID(eth->ether_dhost);
	VID vid, mvid, svid;
	EtherAddress dmac = EtherAddress(eth->ether_dhost);
	EtherAddress smac = EtherAddress(eth->ether_shost);
	memcpy(&dvid, &dmac, VID_LEN);
	memcpy(&svid, &smac, VID_LEN);
	// TODO: Decrement the TTL on the IP header!
	// SJ: Lets do it inside the routing module
	
	veil_chatter(printDebugMessages,"[ProcessIP][PACKETIN] From (IP: %s, Mac: %s) to (IP: %s, VID: %s),\n", srcip.s().c_str(), smac.s().c_str(), dstip.s().c_str(),dvid.vid_string().c_str());
	
	//if IP packet came from one of our hosts
	if(ntohs(eth->ether_type) == ETHERTYPE_IP){
		//Register "source ip, source mac" to a vid with myvid.	
		VID::generate_host_vid(&myVid, &smac, &svid);	
		hosts->updateEntry(&svid, &smac, &srcip);
		
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
		else{
			// do two things:
			// 1. Overwrite eth->ether_type by the ETHERTYPE_VEIL_IP
			// 2. overwrite "source mac" eth-> ether_shost by source vid.
			short etype = ETHERTYPE_VEIL_IP;
			eth->ether_type = htons(etype);
			memcpy(eth->ether_shost, &svid, VID_LEN); 
			veil_chatter(printDebugMessages,"[... ProcessIP ][EtherTYPE_IP][Encapsulation to VEIL_IP type] From (IP: %s, Mac: %s) to (IP: %s, VID: %s), After add rewriting: From (IP: %s, VID: %s)\n", srcip.s().c_str(), smac.s().c_str(), dstip.s().c_str(),dvid.vid_string().c_str() , srcip.s().c_str(), svid.vid_string().c_str());
			return p;
		}
		
		p->kill();
		return NULL;
	}

	//IP pkt from another switch
	if(ntohs(eth->ether_type) == ETHERTYPE_VEIL_IP){
		// CHECK for following things here:
		// 1. Is destination one of my host-devices?
			// if yes: a. overwrite the eth->ether_type to ETHERTYPE_IP
			// b. overwrite the eth->ether_dhost by the real mac address
			// return the modified packet p
			
		if (hosts->lookupVID(&dvid, &dmac)){
				short etype = ETHERTYPE_IP;
				eth->ether_type = htons(etype);
				memcpy(eth->ether_dhost, &dmac, VID_LEN);
				veil_chatter(printDebugMessages,"[... ProcessIP ][VEIL to EtherTYPE_IP]From (IP: %s, VID: %s) to (IP: %s, VID: %s), After add rewriting: To (IP: %s, MAC: %s)\n", srcip.s().c_str(), svid.vid_string().c_str(), dstip.s().c_str(),dvid.vid_string().c_str() , dstip.s().c_str(), dmac.s().c_str());							
		}else{
			veil_chatter(printDebugMessages,"[... ProcessIP ][VEILSwitch to VEILSwitch]From (IP: %s, VID: %s) to (IP: %s, VID: %s)\n", srcip.s().c_str(), svid.vid_string().c_str(), dstip.s().c_str(),dvid.vid_string().c_str());
		}
		// Simply forward the packet to "routing module"
		return p;
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

