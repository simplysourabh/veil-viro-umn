#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include "processaccessinfo.hh"

CLICK_DECLS

VEILProcessAccessInfo::VEILProcessAccessInfo () {}

VEILProcessAccessInfo::~VEILProcessAccessInfo () {}

int
VEILProcessAccessInfo::configure (
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
VEILProcessAccessInfo::smaction(Packet* p)
{
	assert(p);

	int myport = PORT_ANNO(p);
	VID myVid;
	interfaces->lookupIntEntry(myport, &myVid);

	click_ether *e = (click_ether*) p->data();

	VID dvid = VID(e->ether_dhost);

	int interface;
	if (!interfaces->lookupVidEntry(&dvid, &interface)){
		// Should I have received this packet? see if ether_dhost is me or not?
		p->kill();
		return NULL;
	}

	veil_sub_header* vheader = (veil_sub_header *) (e+1);
	memcpy(&dvid, &vheader->dvid, 6);
	uint16_t veil_type = ntohs(vheader->veil_type);
	if(veil_type == VEIL_MAP_UPDATE){
		veil_chatter_new(printDebugMessages, class_name(), "smaction | Receivd a VEIL_MAP_UDPATE packet");
		// see if I am the destination.
		if(interfaces->lookupVidEntry(&dvid, &interface)){
			// update the host table entry.
			veil_payload_map_update * update_payload = (veil_payload_map_update *)(vheader+1);
			IPAddress hostip = update_payload->ip;
			EtherAddress hostmac = update_payload->mac;
			VID hostvid = update_payload->vid;
			VID myhostvid;
			hosts->lookupIP(&hostip, &myhostvid);
		
			veil_chatter_new(printDebugMessages, class_name(), "smaction | VEIL_MAP_UPDATE for ip %s old vid%s newvid %s", hostip.s().c_str(), myhostvid.vid_string().c_str(), hostvid.vid_string().c_str());
			// if the hostvid is not same as the current mapping then we 
			// need to delete our mapping.
			if (myhostvid != hostvid){
				hosts->deleteIP(hostip);
			}
			// see if the host table has the IP address already.

			p->kill();
			return NULL;
		}else{
			//I am not the destination. route the packet.
			return p;
		}
	}
	// Am I the final destination?
	if(interfaces->lookupVidEntry(&dvid, &interface))
	{
		veil_payload_map_publish * payload_publish = (veil_payload_map_publish*)(vheader+1);
		IPAddress ip = payload_publish->ip;
		VID vid = payload_publish->vid;
		EtherAddress mac;
		memcpy(&mac,&payload_publish->mac,6);
		//TODO: First check if there is any old mapping for this IP with different vid or not.
		VID vidold, myvidold; EtherAddress ipmacold;
		if(map->lookupIP(&ip, &vidold, &myvidold, &ipmacold)){
			// check if this mapping is same as the current one or not.
			if (vidold != vid){
				// since we have new mapping for the ip address
				veil_chatter_new(printDebugMessages, class_name(),"smaction | For ip %s old vid was %s  new vid is %s", ip.s().c_str(), vidold.vid_string().c_str(), vid.vid_string().c_str());

				// create the veil_map_update packet
				VID dst_host_switchvid;
				bzero(&dst_host_switchvid, 6);
				memcpy(&dst_host_switchvid, &vidold, 4);
        			WritablePacket* p = update_access_info_packet(ip, mac, vid, dst_host_switchvid, myVid, printDebugMessages, class_name());
				veil_chatter_new(printDebugMessages, class_name(), "Sending the packet to %s", dst_host_switchvid);

				// push the packet to output(0)
				if(p){ output(0).push(p);}
			}
		}
		map->updateEntry(ip, vid, myVid, mac);
		veil_chatter_new(printDebugMessages, class_name(),"[STORE MAPPING] HOST IP: %s  VID: %s  AccessSwitchVID: %s", ip.s().c_str(),  vid.vid_string().c_str(),myVid.switchVIDString().c_str()) ;
		
		p->kill();	
		return NULL;
	}
	
	//if the packet was not destined to us, we need to reroute it
	SET_REROUTE_ANNO(p, 'r');
	return p;
}

void 
VEILProcessAccessInfo::push(int, Packet* p)
{
	Packet *q = smaction(p);
	if(q != NULL)
		output(0).push(q);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(VEILProcessAccessInfo)

