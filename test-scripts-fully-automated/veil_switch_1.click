require(veil);
interfaces::VEILInterfaceTable(
				000000000000,08:00:27:04:cc:0d,
				000000000000,08:00:27:d7:2d:6d,
				000000000000,08:00:27:e0:59:f4,
				000000000000,08:00:27:f5:cb:a6,
				UseStatic false, 
				PRINTDEBUG false
			      );

hosts::VEILHostTable(PRINTDEBUG false);
neighbors::VEILNeighborTable(PRINTDEBUG false);

mapping::VEILMappingTable(PRINTDEBUG false);
rendezvouspoints::VEILRendezvousTable(PRINTDEBUG false);
routes::VEILRouteTable(INTERFACETABLE interfaces, PRINTDEBUG true);

topo::VEILNetworkTopoVIDAssignment(PRINTDEBUG true);

// output devices
out0::ToDevice(eth0);
out1::ToDevice(eth1);
out2::ToDevice(eth2);
out3::ToDevice(eth3);
q0 :: Queue -> out0;
q1 :: Queue -> out1;
q2 :: Queue -> out2;
q3 :: Queue -> out3;


// input devices
in0::FromDevice(eth0);
in1::FromDevice(eth1);
in2::FromDevice(eth2);
in3::FromDevice(eth3);

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
router[5] -> Print(<--RoutePacketERROR-->) -> Discard;

vccstate::VEILSpanningTreeState(PRINTDEBUG false);
vccgenerator::VEILGenerateVCCSTAdSub(interfaces, neighbors, vccstate, PRINTDEBUG false);
vccprocessor::VEILProcessVCCSTAdSub(INTERFACETABLE interfaces, NEIGHBORTABLE neighbors, SPANNINGTREESTATE vccstate, NETWORKTOPO topo, PRINTDEBUG false);
vidgenerator::VEILGenerateVIDAssignmentPackets(INTERFACETABLE interfaces, NEIGHBORTABLE neighbors, SPANNINGTREESTATE vccstate, NETWORKTOPO topo, PRINTDEBUG false);
vccgenerator[0] -> q0;
vccgenerator[1] -> q1;
vccgenerator[2] -> q2;
vccgenerator[3] -> q3;
vccgenerator[4] -> Print(PACKETISFORME) -> c;

vidgenerator[0] -> q0;
vidgenerator[1] -> q1;
vidgenerator[2] -> q2;
vidgenerator[3] -> q3;
vidgenerator[4] -> Print(PACKETISFORMEFromVIDGenerator, MAXLENGTH 50) -> c;

vccprocessor[0] -> q0;
vccprocessor[1] -> q1;
vccprocessor[2] -> q2;
vccprocessor[3] -> q3;

c[12] -> vccprocessor;

c[0] -> VEILProcessHello(neighbors, interfaces,PRINTDEBUG false);

prdv::VEILProcessRDV(routes, rendezvouspoints, interfaces, PRINTDEBUG true) -> router;

c[1] -> prdv;
c[2] -> prdv;
c[3] -> prdv;

parp::VEILProcessARP(hosts, mapping, interfaces,PRINTDEBUG false) ->  router;

c[4]  -> Print (ETHERARP) ->  parp;
c[10] -> Print (VEIL_ARP) ->  parp;

paci::VEILProcessAccessInfo(mapping, interfaces,PRINTDEBUG false) -> router;
c[5] -> paci;
c[6] -> paci; 

pip::VEILProcessIP(hosts, mapping, interfaces,FORWARDING_TYPE 2, PRINTDEBUG false)-> router;

c[7]  -> pip;
c[8]  -> pip;
c[9]  -> pip;
c[11] -> pip;


VEILBuildRouteTable(neighbors, routes, interfaces,PRINTDEBUG true) -> router;

VEILPublishAccessInfo(hosts, PRINTDEBUG false) -> router;

c[13] -> Discard;

Script(wait 0s, print interfaces.table, wait 10s, loop);
Script(wait 1s, print routes.table, wait 10s, loop);
Script(wait 2s, print neighbors.table, wait 10s, loop);
Script(wait 3s, print rendezvouspoints.table, wait 10s, loop);
Script(wait 4s, print hosts.table, wait 200s, loop);
Script(wait 5s, print mapping.table, wait 200s, loop);
Script(wait 0s, print vccstate.state, wait 10s, loop);
Script(wait 3s, print topo.table, wait 10s, loop);
