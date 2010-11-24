require(veil);
interfaces::VEILInterfaceTable(
				000000000000,08:00:27:7b:bb:b3,
				000000000000,08:00:27:60:cb:86,
				000000000000,08:00:27:8e:51:cd,
				000000000000,08:00:27:6e:7d:bd,
				UseStatic false, 
				PRINTDEBUG false
			      );

hosts::VEILHostTable(PRINTDEBUG true);
neighbors::VEILNeighborTable(PRINTDEBUG true);

mapping::VEILMappingTable(PRINTDEBUG true);
rendezvouspoints::VEILRendezvousTable(PRINTDEBUG true);
routes::VEILRouteTable(INTERFACETABLE interfaces, PRINTDEBUG true);

topo::VEILNetworkTopoVIDAssignment(VCCMAC 08:00:27:6e:7d:bd, PRINTDEBUG false);

// output devices
out0::ToDevice(eth2);
out1::ToDevice(eth3);
out2::ToDevice(eth4);
out3::ToDevice(eth5);
q0 :: Queue -> out0;
q1 :: Queue -> out1;
q2 :: Queue -> out2;
q3 :: Queue -> out3;


// input devices
in0::FromDevice(eth2);
in1::FromDevice(eth3);
in2::FromDevice(eth4);
in3::FromDevice(eth5);

hellogen::VEILGenerateHelloNew (interfaces, PRINTDEBUG false);
hellogen[0]->q0;
hellogen[1]->q1;
hellogen[2]->q2;
hellogen[3]->q3;

c::Classifier(12/9876 26/0000, // 0. VEIL_HELLO
	      12/9876 26/0403, // 1. VEIL_RDV_PUBLISH
	      12/9876 26/0404, // 2. VEIL_RDV_QUERY
              12/9876 26/0402, // 3. VEIL_RDV_REPLY
              12/9876 26/0602, // 4. VEIL_ENCAP_ARP
              12/9876 26/0201, // 5. VEIL_MAP_PUBLISH
              12/9876 26/0202, // 6. VEIL_MAP_UPDATE
              12/9876 26/0601, // 7. VEIL_ENCAP_IP
              12/9876 26/0801, // 8. VEIL_ENCAP_MULTIPATH_IP
              12/9878,	       // 9. ETHERTYPE_VEIL_IP
	      12/0806,         //10. ETHERTYPE_ARP
	      12/0800,         //11. ETHERTYPE_IP
	      12/9876 26/0100%FF00, // 12. VCC Packets.
	      -);    	       //13. Everything Else (DISCARD)


in0 -> VEILSetPortAnnotation(0) -> c;
in1 -> VEILSetPortAnnotation(1) -> c;
in2 -> VEILSetPortAnnotation(2) -> c;
in3 -> VEILSetPortAnnotation(3) -> c;


router::VEILRoutePacket(hosts, routes, interfaces, neighbors, PRINTDEBUG true);
//need Queue to convert from push to pull
router[0] -> q0;
router[1] -> q1;
router[2] -> q2;
router[3] -> q3;
router[4] -> c;
router[5] -> Discard;

datasAd::TimedSource(DATA \<ffffffff ffff 0800 276e 7dbd 9876 0800 276e 7dbd 000000000000 0101 0200 0800 276e7dbd 0000>, INTERVAL 5) -> c;
vccstate::VEILSpanningTreeState(08:00:27:6e:7d:bd 08:00:27:6e:7d:bd 0, PRINTDEBUG false);
vccgenerator::VEILGenerateVCCSTAdSub(interfaces, neighbors, vccstate, PRINTDEBUG false);
vccprocessor::VEILProcessVCCSTAdSub(INTERFACETABLE interfaces, NEIGHBORTABLE neighbors, SPANNINGTREESTATE vccstate, NETWORKTOPO topo, PRINTDEBUG false);
vidgenerator::VEILGenerateVIDAssignmentPackets(INTERFACETABLE interfaces, NEIGHBORTABLE neighbors, SPANNINGTREESTATE vccstate, NETWORKTOPO topo, PRINTDEBUG false);
vccgenerator[0] -> q0;
vccgenerator[1] -> q1;
vccgenerator[2] -> q2;
vccgenerator[3] -> q3;
vccgenerator[4] -> c;

vidgenerator[0] -> q0;
vidgenerator[1] -> q1;
vidgenerator[2] -> q2;
vidgenerator[3] -> q3;
vidgenerator[4] -> c;

vccprocessor[0] -> q0;
vccprocessor[1] -> q1;
vccprocessor[2] -> q2;
vccprocessor[3] -> q3;

c[12] -> vccprocessor;

c[0] -> VEILProcessHello(neighbors, interfaces,PRINTDEBUG false);

prdv::VEILProcessRDV(routes, rendezvouspoints, interfaces, PRINTDEBUG false) -> router;

c[1] -> prdv;
c[2] -> prdv;
c[3] -> prdv;

parp::VEILProcessARP(hosts, mapping, interfaces,PRINTDEBUG true) ->  router;

c[4]  -> Print (ETHERARP) ->  parp;
c[10] -> Print (VEIL_ARP) ->  parp;

paci::VEILProcessAccessInfo(mapping, interfaces,PRINTDEBUG true) -> router;
c[5] -> paci;
c[6] -> paci; 

pip::VEILProcessIP(hosts, mapping, interfaces,FORWARDING_TYPE 2, PRINTDEBUG true)-> router;

c[7]  -> pip;
c[8]  -> pip;
c[9]  -> pip;
c[11] -> pip;


VEILBuildRouteTable(neighbors, routes, interfaces,PRINTDEBUG true) -> router;

VEILPublishAccessInfo(hosts, PRINTDEBUG true) -> router;

c[13] -> Discard;


Script(wait 0s, print interfaces.table, wait 20s, loop);
Script(wait 1s, print routes.table, wait 19s, loop);
Script(wait 2s, print neighbors.table, wait 18s, loop);
Script(wait 3s, print rendezvouspoints.table, wait 17s, loop);
Script(wait 4s, print hosts.table, wait 16s, loop);
Script(wait 5s, print mapping.table, wait 15s, loop);
Script(wait 6s, print vccstate.state, wait 14s, loop);
Script(wait 7s, print topo.table, wait 13s, loop);
