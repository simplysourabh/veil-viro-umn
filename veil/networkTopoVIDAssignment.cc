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

bool
VEILNetworkTopoVIDAssignment::performVIDAssignment(){
	bool vid_changed = false;
	// first put the current topo in double hashmap.
	// nodeid to int key map;
	HashTable<EtherAddress, int> nodeids;
	// reverse mapping: int-key to nodeid (mac address)
	HashTable<int, EtherAddress> ids2node;

	// topology graph: node-int-key to node-int-key,int pair
	HashTable<int, HashTable<int, int> > nodes;

	HashTable<int, String> node2vid;
	Vector<int> node2cluster;

	int node_n = 0;
	int n_cluster = 0;

	NetworkTopo::iterator iter;
	veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] in performVIDAssignment: Generating the duplicate topo map.\n");

	// initialize the data structures.
	for (iter = networktopo.begin(); iter; iter++){
		EtherAddress node = static_cast<EtherAddress>(iter.key());
		nodeids.set(node, node_n);
		ids2node.set(node_n, node);
		node2cluster.push_back(node_n); // initial clusterid and nodeids are same.
		node2vid.set(node_n, "1"); // assign empty strings as vids for the nodes initially.
		HashTable<int,int> list; // list of nodes as a hashmap. helps in searching.
		nodes.set(node_n, list);
		node_n++;
	}

	// learn the topology map.
	StringAccum sa;
	for (iter = networktopo.begin(); iter; iter++){
		EtherAddress node = static_cast<EtherAddress>(iter.key());
		int nodeid = nodeids.get(node);
		sa << "\nNode:"<<nodeid<<" ->";
		for (int i = 0; i < networktopo[node].size(); i++){
			EtherAddress neigh = networktopo[node][i];
			// exclude neighbor if it hasn't sent its local topo to vcc.
			if (nodeids.get_pointer(neigh) == NULL) {continue;}
			int neighid = nodeids.get(neigh);
			nodes.get_pointer(nodeid)->set(neighid,1);
			nodes.get_pointer(neighid)->set(nodeid,1);
			sa <<" "<< neighid;
		}
	}
	veil_chatter(printDebugMessages,"Initial Topology: %s\n", sa.c_str());
	sa.clear();

	// create a cluster topology, store which clusterid is connected to which clusterid
	veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] in performVIDAssignment: Creating the initial cluster topology\n");
	HashTable<int, HashTable<int,int> > clusterTopo;
	for (int i = 0; i < node_n; i++){
		HashTable<int, int>::iterator it;
		HashTable<int, int> neigh_list = nodes.get(i);
		HashTable<int,int> list;
		clusterTopo.set(i,list);
		for (it = neigh_list.begin(); it; it++){
			clusterTopo[i].set(it.key(),1);
		}
		n_cluster++;
	}

	// now cluster all the nodes one by one till we have only 1 large node left.
	int MAXITER = 5000;
	int c_iter = 0;

	while (c_iter++ < MAXITER && n_cluster > 1){
		veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] in performVIDAssignment: Iteration %d\n", c_iter);
		if (n_cluster == 1){break;} // done. we have only one cluster now.

		// find the smallest clsuter and see if it can be clustered with other nodes.
		HashTable<int,int> clustersizes;
		for (int i = 0; i< node_n; i++){
			int label = node2cluster[i];
			if (clustersizes.get_pointer(label) == NULL) {clustersizes.set(label,0);}
			clustersizes[label]++;
		}

		int smallestClusterLabel = 0;
		int smallestClusterSize = clustersizes[0];

		// find the smallest cluster and its label
		HashTable<int, int>::iterator iter1;
		for (iter1 = clustersizes.begin(); iter1; iter1++){
			if (iter1.value() < smallestClusterSize){
				smallestClusterSize = iter1.value();
				smallestClusterLabel = iter1.key();
			}
			// todo what if the size is same? which one to keep. leaving it for now.
		}

		veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] in performVIDAssignment: Smallest Cluster label %d, size %d\n", smallestClusterLabel, smallestClusterSize);
		// now find the smallest cluster that we can cluster with our earlier cluster.
		int secondCluster = -1;
		int secondClusterSize = -1;


		for (iter1= clusterTopo[smallestClusterLabel].begin(); iter1; iter1++){
			if(iter1.value() == 0){continue;}
			if (secondClusterSize == -1){
				secondCluster = iter1.key();
				secondClusterSize = clustersizes[secondCluster];
			}else if(secondClusterSize > clustersizes[iter1.key()]){
				secondCluster = iter1.key();
				secondClusterSize = clustersizes[secondCluster];
			}
			// todo what if the size is same? which one to keep. leaving it for now.
		}

		int newclusterLabel = min(smallestClusterLabel, secondCluster);
		int len1 = -1; int len2 = -1;

		for (int i = 0; i < node_n; i++){
			if (node2cluster[i] == secondCluster){
				len2 = node2vid[i].length();
				printf("node %d, vid %s, len %d, clusterid %d \n", i, node2vid[i].c_str(),len2,node2cluster[i]);
			} else if (node2cluster[i] == smallestClusterLabel){
				len1 = node2vid[i].length();
				printf("node %d, vid %s, len %d, clusterid %d \n", i, node2vid[i].c_str(),len1,node2cluster[i]);
			}
		}
		int len3 = max(len1,len2);
		veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] in performVIDAssignment: secondcluster %d, size %d, len1 %d, len2 %d, len3 %d\n", secondCluster, secondClusterSize, len1, len2, len3);
		// make sure that nodes in both clusters have same number of bits in their vids
		if (len1 != len2){
			for (int i = 0; i < node_n; i++){
				if (node2cluster[i] == secondCluster || node2cluster[i] == smallestClusterLabel){
					while (node2vid[i].length() < len3){
						node2vid[i] = "0" + node2vid[i];
					}
				}
			}
		}
		if(len1 == -1 || len2 == -1 || secondCluster == -1 || secondClusterSize < 1 || smallestClusterSize < 1){
			veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] in performVIDAssignment: breaking out of the loop.\n");
			break;
		}
		// update the cluster labels and vids
		for (int i = 0; i < node_n; i++){
			if (node2cluster[i] == secondCluster){
				node2vid[i] = "0" + node2vid[i];
				node2cluster[i] = newclusterLabel;
			} else if (node2cluster[i] == smallestClusterLabel){
				node2vid[i] = "1" + node2vid[i];
				node2cluster[i] = newclusterLabel;
			}
		}

		// now update the topology
		// first change the label for clusters to newclusterlabel.
		int clusterid2remove = max(smallestClusterLabel, secondCluster);
		HashTable<int, HashTable<int,int> >::iterator it2;
		for (it2 = clusterTopo.begin(); it2; it2++){
			if (it2.value().get_pointer(smallestClusterLabel) != NULL){
				it2.value()[smallestClusterLabel] = 0;
				it2.value()[newclusterLabel] = 1;
			}
			if (it2.value().get_pointer(secondCluster) != NULL){
				it2.value()[secondCluster] = 0;
				it2.value()[newclusterLabel] = 1;
			}
			if (it2.value().get_pointer(clusterid2remove) != NULL){
				it2.value().erase(clusterid2remove);
			}
		}

		// put the neighbors of clusterid2remove in the newclusterlabel
		for (iter1 = clusterTopo[clusterid2remove].begin(); iter1; iter1++){
			clusterTopo[newclusterLabel].set(iter1.key(),1);
		}

		// remove the self id from the newly formed cluster.
		clusterTopo[newclusterLabel].erase(newclusterLabel);
		// now remove the clusterid2remove
		clusterTopo.erase(clusterid2remove);

		n_cluster = clusterTopo.size();
		veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] in performVIDAssignment: n_cluster %d at the end of iteration %d\n", n_cluster, c_iter);
	}
	veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] Assigned VIDS:\nEther Address \t VID \t Index\n");
	int vidlen = node2vid[0].length();

	// adding enough room at the prefix and suffix levels
	int prefixlen = (32-vidlen)/2;
	int suffixlen = 32 - vidlen - prefixlen;
	veil_chatter(printDebugMessages,"[VEIL-Network-topo-VID-Assignment] Adding a prefix of len %d, suffix of len %d to current vidlen %d\n", prefixlen,suffixlen, vidlen);
	while (vidlen < 32){
		for (int i = 0; i < node_n; i++){
			if(prefixlen > 0){
				node2vid[i] = "0" + node2vid[i];
			}else{
				node2vid[i] = node2vid[i]+"0";
			}
		}
		prefixlen = prefixlen > 0? prefixlen-1:0;
		vidlen = node2vid[0].length();
	}

	for (int i = 0; i < node_n; i++){
		veil_chatter(printDebugMessages,"%s \t %s \t %d\n", ids2node[i].s().c_str(), node2vid[i].c_str(), i);
	}

	if (vidlen > 32){
		veil_chatter(true, "[VEIL-Network-topo-VID-Assignment] ERROR: vid_len %d > 31, exceeds the limit of 31!! \n", vidlen);
		return true;
	}else if (vidlen == 0){
		veil_chatter(true, "[VEIL-Network-topo-VID-Assignment] ERROR: vid_len %d, is ZERO!! \n", vidlen);
		return true;
	}

	// convert the vids to VID format.
	for (int i = 0; i < node_n; i++){
		char bytes[6];
		VID vid;
		uint32_t vidval = strtoul(node2vid[i].c_str(),NULL,2);
		//printf("node2vid[%d] %s vidval %d\n", i, node2vid[i].c_str(),vidval);
		for (int j = 0; j < 4; j++){
			uint32_t bitshiftval = (3-j)*8;
			uint32_t base = 1;
			base = base << bitshiftval;
			bytes[j] = (char)(vidval >> bitshiftval);
			vidval = vidval % base;
			//printf("%02x or %d, vidval %d | ", bytes[j], bytes[j], vidval);
		}
		//printf("\n");
		/*
		bytes[0] = (char)(vidval >> 24);
		vidval = vidval % 2^24;
		bytes[1] = (char)(vidval >> 16);
		vidval = vidval % 2^16;
		bytes[2] = (char)(vidval >> 8);
		bytes[3] = (char)(vidval % 2^8);
		bytes[4] = bytes[5] = 0;
		*/
		memcpy(&vid, bytes, 6);

		EtherAddress mac = ids2node[i];
		if (ether2vid.get_pointer(mac) != NULL){
			if (ether2vid[mac] != vid){
				veil_chatter(true,"[VEIL-Network-topo-VID-Assignment] VID CHANGED! New VID %s for MAC %s from old VID %s\n",vid.vid_string().c_str(), mac.s().c_str(),ether2vid[mac].vid_string().c_str() );
				vid_changed = true;
				ether2vid[mac] = vid;
				vid2ether[vid] = mac;
			}
		}else{
			ether2vid[mac] = vid;
			vid2ether[vid] = mac;
		}
		veil_chatter(printDebugMessages,"EtherAddress: %s MAC:%s \n", vid.vid_string().c_str(), mac.s().c_str());
	}


	return vid_changed;
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

