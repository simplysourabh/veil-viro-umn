//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
require(veil);
/*
set vid1 0x1;
set vid2 0xB;
set eth5 eth2;
*/

interfaces::VEILInterfaceTable(000000010000,0000000B0000, PRINTDEBUG false);

hosts::VEILHostTable(PRINTDEBUG false);
neighbors::VEILNeighborTable(PRINTDEBUG false);
mapping::VEILMappingTable(PRINTDEBUG false);
rendezvouspoints::VEILRendezvousTable(PRINTDEBUG false);
routes::VEILRouteTable(PRINTDEBUG false);

out1::ToDevice(eth5);
out2::ToDevice(eth2);

in1::FromDevice(eth5);
in2::FromDevice(eth2);

q1::Queue;
q2::Queue;
q1 -> out1;
q2 -> out2;

VEILGenerateHello(000000010000, PRINTDEBUG false) -> q1;
VEILGenerateHello(0000000B0000, PRINTDEBUG false) -> q2;

c::Classifier(12/9876 26/0000, //0. VEIL_HELLO
	      12/9876 26/0403, //1. VEIL_RDV_PUBLISH
	      12/9876 26/0404, //2. VEIL_RDV_QUERY
              12/9876 26/0402, //3. VEIL_RDV_REPLY
              12/9876 26/0602, //4. VEIL_ENCAP_ARP
              12/9876 26/0201, //5. VEIL_MAP_PUBLISH
              12/9876 26/0202, //6. VEIL_MAP_UPDATE
              12/9876 26/0601, //7. VEIL_ENCAP_IP
	      12/0806,         //8. ETHERTYPE_ARP
	      12/0800,         //9. ETHERTYPE_IP
	      12/9878,	       //A. OLD ETHERTYPE_VEIL_IP
	      -);    	       //B. Everything Else (DISCARD)

router::VEILRoutePacket(hosts, routes, interfaces, neighbors, PRINTDEBUG false);


//need Queue to convert from push to pull
router[0] -> q1;
router[1] -> q2;
router[2] -> c;
router[3] -> Print(<--RoutePacketERROR-->) -> Discard;

in1 -> VEILSetPortAnnotation(0) -> c;
in2 -> VEILSetPortAnnotation(1) -> c;

c[0] -> VEILProcessHello(neighbors, interfaces,PRINTDEBUG false);

prdv::VEILProcessRDV(routes, rendezvouspoints, interfaces, PRINTDEBUG false) -> router;

c[1] -> prdv;
c[2] -> prdv;
c[3] -> prdv;

parp::VEILProcessARP(hosts, mapping, interfaces,PRINTDEBUG true) ->  router;

c[4]  -> Print (ETHERARP) ->  parp;
c[8]  -> Print (VEIL_ARP) ->  parp;

paci::VEILProcessAccessInfo(mapping, interfaces,PRINTDEBUG false) -> router;
c[5] -> paci;
c[6] -> paci; 

pip::VEILProcessIP(hosts, mapping, interfaces, PRINTDEBUG true)-> router;

c[7]  -> pip;
c[9]  -> pip;
c[10] -> pip;



VEILBuildRouteTable(neighbors, routes, interfaces,PRINTDEBUG false) -> router;

VEILPublishAccessInfo(hosts, PRINTDEBUG false) -> router;

c[11] -> Discard;

Script(wait 0s, print interfaces.table, wait 600s, loop);
Script(wait 1s, print routes.table, wait 10s, loop);
Script(wait 2s, print neighbors.table, wait 615s, loop);
Script(wait 3s, print rendezvouspoints.table, wait 615s, loop);
Script(wait 4s, print hosts.table, wait 200s, loop);
Script(wait 5s, print mapping.table, wait 200s, loop);
