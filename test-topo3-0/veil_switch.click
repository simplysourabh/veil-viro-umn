require(veil);
/*
set vid1 0x1;
set vid2 0xB;
set vid3 0x5
set eth5 eth2 eth3;
*/

interfaces::VEILInterfaceTable(000000010000,0000000B0000,000000050000, PRINTDEBUG false);

hosts::VEILHostTable(PRINTDEBUG false);
neighbors::VEILNeighborTable(PRINTDEBUG false);
mapping::VEILMappingTable(PRINTDEBUG false);
rendezvouspoints::VEILRendezvousTable(PRINTDEBUG false);
routes::VEILRouteTable(PRINTDEBUG false);
out1::ToDevice(eth5);
out2::ToDevice(eth2);
out3::ToDevice(eth3);

in1::FromDevice(eth5);
in2::FromDevice(eth2);
in3::FromDevice(eth3);

q1::Queue;
q2::Queue;
q3::Queue;
q1 -> out1;
q2 -> out2;
q3 -> out3;

VEILGenerateHello(000000010000, PRINTDEBUG false) -> q1;
VEILGenerateHello(0000000B0000, PRINTDEBUG false) -> q2;
VEILGenerateHello(000000050000, PRINTDEBUG false) -> q3;

c::Classifier(12/9876 14/0001, //0. VEIL_HELLO
	      12/9876 14/0002, //1. VEIL_RDV_PUBLISH
	      12/9876 14/0003, //2. VEIL_RDV_QUERY
              12/9876 14/0004, //3. VEIL_RDV_REPLY
              12/9876 14/0005, //4. VEIL_ARP_REQ
              12/9876 14/0006, //5. VEIL_ARP_RPLY
              12/9876 14/0007, //6. VEIL_PUBLISH
              12/9876 14/0008, //7. VEIL_IP
	      12/0806,       //8. ETHERTYPE_ARP
	      12/0800,       //9. ETHERTYPE_IP
	      -);    	     //A. Everything Else (DISCARD)

router::VEILRoutePacket(hosts, routes, interfaces, neighbors, PRINTDEBUG false);


//need Queue to convert from push to pull
router[0] -> q1;
router[1] -> q2;
router[2] -> q3;
router[3] -> c;
router[4] -> Print(<--RoutePacketERROR-->) -> Discard;

in1 -> Print(IN1) ->  VEILSetPortAnnotation(0) -> c;
in2 -> VEILSetPortAnnotation(1) -> c;
in3 -> VEILSetPortAnnotation(2) -> c;


//c[0] -> Print(VEIL_HELLO) -> VEILProcessHello(neighbors, interfaces);
c[0] -> VEILProcessHello(neighbors, interfaces,PRINTDEBUG false);

prdv::VEILProcessRDV(routes, rendezvouspoints, interfaces, PRINTDEBUG false) -> router;

c[1] -> prdv;
c[2] -> prdv;
c[3] -> prdv;

parp::VEILProcessARP(hosts, mapping, interfaces,PRINTDEBUG true) -> router;

c[4] -> parp;
c[5] -> parp;
c[8] -> Print("PARP: ") -> parp;

c[6] -> paci::VEILProcessAccessInfo(mapping, interfaces,PRINTDEBUG true) -> router;

pip::VEILProcessIP(hosts, mapping, interfaces, PRINTDEBUG true) -> router;

c[7] -> pip;
c[9] -> pip;

VEILBuildRouteTable(neighbors, routes, interfaces,PRINTDEBUG false) -> router;

VEILPublishAccessInfo(hosts, PRINTDEBUG true) -> router;

c[10] -> Discard;

Script(wait 0s, print interfaces.table, wait 30s, loop);
Script(wait 1s, print routes.table, wait 29s, loop);
Script(wait 2s, print neighbors.table, wait 28s, loop);
Script(wait 3s, print rendezvouspoints.table, wait 27s, loop);
Script(wait 4s, print hosts.table, wait 6s, loop);
