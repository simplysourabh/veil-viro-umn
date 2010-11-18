#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/straccum.hh>
#include "networkTopoVIDAssignment.hh"
#include <time.h>

//TODO: make reads and writes atomic#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include <clicknet/ip.h>
#include "click_veil.hh"

CLICK_DECLS

VEILNetworkTopoVIDAssignment::VEILNetworkTopoVIDAssignment () {}

VEILNetworkTopoVIDAssignment::~VEILNetworkTopoVIDAssignment () {}

int
VEILNetworkTopoVIDAssignment::cp_an_edge(String s, ErrorHandler* errh){
	String type = cp_shift_spacevec(s);
	if (type.lower() == "edge" ){
		EtherAddress from, to;
		String from_str = cp_shift_spacevec(s);
		if(!cp_ethernet_address(from_str, &from))
			return errh->error("[VEIL-Network-topo-VID-Assignment] From EtherAddress is not in expected format!");

		String to_str = cp_shift_spacevec(s);
		if(!cp_ethernet_address(to_str, &to))
			return errh->error("[VEIL-Network-topo-VID-Assignment] From EtherAddress is not in expected format!");

		addAnEdge(from, to);
	}else if(type.lower() == "ether2vid"){
		EtherAddress mac;
		VID vid;
		String mac_str = cp_shift_spacevec(s);
		if(!cp_ethernet_address(mac_str, &mac))
			return errh->error("[VEIL-Network-topo-VID-Assignment] in Ether2VID map EtherAddress is not in expected format!");

		String vid_str = cp_shift_spacevec(s);
		if(!cp_vid(vid_str, &vid))
			return errh->error("[VEIL-Network-topo-VID-Assignment] in Ether2VID map VID is not in expected format!");

		addAMap(vid, mac);
	}
	else if(type.lower() == "vccmac"){
			String mac_str = cp_shift_spacevec(s);
			if(!cp_ethernet_address(mac_str, &vccmac))
				return errh->error("[VEIL-Network-topo-VID-Assignment] in VCCMAC EtherAddress is not in expected format!");

		}
	return 0;
}

int
VEILNetworkTopoVIDAssignment::configure(Vector<String> &conf, ErrorHandler *errh)
{
	click_chatter("[VEIL-Network-topo-VID-Assignment] [FixME] Its mandatory to have 'PRINTDEBUG value' (here value = true/false) at the end of the configuration string!\n");
	int res = 0;
	int i = 0;
	for (i = 0; i < conf.size()-1; i++) {
		res = cp_an_edge(conf[i], errh);
	}
	veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] Configured the initial Network topology and vid assignments.\n");

	cp_shift_spacevec(conf[i]);
	String printflag = cp_shift_spacevec(conf[i]);
	if(!cp_bool(printflag, &printDebugMessages))
		return errh->error("[VEIL-Network-topo-VID-Assignment] [Error] PRINTDEBUG FLAG should be either true or false");
	return res;
}

int
VEILNetworkTopoVIDAssignment::removeNode(EtherAddress mac){
	return networktopo.erase(mac);
}

bool
VEILNetworkTopoVIDAssignment::addAnEdge(EtherAddress fromMac, EtherAddress toMac){ // adds a directed edge frommac -> tomac
	bool added = false;
	veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] Adding the edge %s -> %s \n", fromMac.s().c_str(), toMac.s().c_str());
	if (networktopo.get_pointer(fromMac) == NULL){
		veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] Creating neighbor vector for %s \n", fromMac.s().c_str());
		Vector<EtherAddress> v;
		networktopo[fromMac] = v;
	}
	int i = 0;
	for (i = 0; i < networktopo[fromMac].size(); i++){
		if (networktopo[fromMac][i] == toMac){
			veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] Duplicate edge %s -> %s \n", fromMac.s().c_str(), toMac.s().c_str());
			return false;
		}
	}
	networktopo[fromMac].push_back(toMac);
	added = true;
	return added;
}

bool
VEILNetworkTopoVIDAssignment::addAMap(VID vid, EtherAddress mac){ // adds a mapping from vid to mac and reverse mapping
	bool added = false;
	veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] Adding the mapping MAC %s <-> VID %s \n", mac.s().c_str(), vid.vid_string().c_str());

	if (ether2vid[mac] != ether2vid.default_value()){
		if (ether2vid[mac] != vid){
			veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] Warning already have a mapping MAC %s -> VID %s \n", mac.s().c_str(), ether2vid[mac].vid_string().c_str());
		}
	}
	ether2vid[mac] = vid;

	if (vid2ether[vid] != vid2ether.default_value()){
		if (vid2ether[vid] != mac){
			veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] Warning already have a mapping VID %s -> MAC %s \n", vid.vid_string().c_str(), vid2ether[vid].s().c_str());
		}
	}
	vid2ether[vid] = mac;
	return true;
}

String
VEILNetworkTopoVIDAssignment::read_handler(Element *e, void *thunk){
	StringAccum sa;
	VEILNetworkTopoVIDAssignment *nt = (VEILNetworkTopoVIDAssignment *) e;
	NetworkTopo::iterator iter;
	NetworkTopo topo = nt->networktopo;

	sa << "\n-----------------Network Topology START-----------------"<< '\n';
	sa << "Node (MAC)              " << " " << "<Neighbors(MAC)>\n";

	for(iter = topo.begin(); iter; ++iter){
		EtherAddress node = static_cast<EtherAddress>(iter.key());
		sa << node.s() <<" <";
		for (int i = 0; i < topo[node].size(); i++){
			if (i == 0) sa << topo[node][i].s();
			else sa << " " << topo[node][i].s();
		}
		sa << ">\n";
	}
	sa<< "----------------- Network Topology END -----------------\n\n";


	EtherToVID::iterator iter1;
	EtherToVID map = nt->ether2vid;

	sa << "\n----------------- VID assignment to VEIL-Switches START-----------------"<< '\n';
	sa << "Node (MAC)              " << " " << "Node (VID)\n";

	for(iter1 = map.begin(); iter1; ++iter1){
		EtherAddress node = static_cast<EtherAddress>(iter1.key());
		sa << node.s() <<" "<<map[node].vid_string()<<"\n";
	}
	sa<< "----------------- VID assignment to VEIL-Switches END -----------------\n\n";

	return sa.take_string();
}


void
VEILNetworkTopoVIDAssignment::add_handlers()
{
	add_read_handler("table", read_handler, (void *)0);
}


CLICK_ENDDECLS

EXPORT_ELEMENT(VEILNetworkTopoVIDAssignment)

