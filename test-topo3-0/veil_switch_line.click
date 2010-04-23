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

c::Classifier(12/9876 14/0001, //0. VEIL_HELLO
	      12/9876 14/0002, //1. VEIL_RDV_PUBLISH
	      12/9876 14/0003, //2. VEIL_RDV_QUERY
              12/9876 14/0004, //3. VEIL_RDV_REPLY
              12/9876 14/0005, //4. VEIL_ARP_REQ
              12/9876 14/0006, //5. VEIL_ARP_RPLY
              12/9876 14/0007, //6. VEIL_PUBLISH
              12/9878, 	       //7. ETHERTYPE_VEIL_IP
	      12/0806,         //8. ETHERTYPE_ARP
	      12/0800,         //9. ETHERTYPE_IP
	      -);    	       //A. Everything Else (DISCARD)

router::VEILRoutePacket(hosts, routes, interfaces, neighbors, PRINTDEBUG false);


//need Queue to convert from push to pull
router[0] -> q1;
router[1] -> q2;
router[2] -> c;
router[3] -> Print(<--RoutePacketERROR-->) -> Discard;

/*
in1 -> VEILSetPortAnnotation(0) -> Print(IN1) -> c;
in2 -> VEILSetPortAnnotation(1) -> Print(IN2) -> c;
*/

in1 -> VEILSetPortAnnotation(0) -> c;
in2 -> VEILSetPortAnnotation(1) -> c;


//c[0] -> Print(VEIL_HELLO) -> VEILProcessHello(neighbors, interfaces);
c[0] -> VEILProcessHello(neighbors, interfaces,PRINTDEBUG false);

prdv::VEILProcessRDV(routes, rendezvouspoints, interfaces, PRINTDEBUG false) -> router;

c[1] -> prdv;
c[2] -> prdv;
c[3] -> prdv;

parp::VEILProcessARP(hosts, mapping, interfaces,PRINTDEBUG false) ->  router;

c[4]  -> parp;
c[5]  -> parp;
c[8]  ->  parp;

c[6] -> paci::VEILProcessAccessInfo(mapping, interfaces,PRINTDEBUG false) -> router;

pip::VEILProcessIP(hosts, mapping, interfaces, PRINTDEBUG false) -> router;

c[7]  ->  pip;
c[9]  -> pip;


VEILBuildRouteTable(neighbors, routes, interfaces,PRINTDEBUG false) -> router;

VEILPublishAccessInfo(hosts, PRINTDEBUG false) -> router;

c[10] -> Discard;

Script(wait 0s, print interfaces.table, wait 600s, loop);
Script(wait 1s, print routes.table, wait 590s, loop);
Script(wait 2s, print neighbors.table, wait 580s, loop);
Script(wait 3s, print rendezvouspoints.table, wait 570s, loop);
Script(wait 4s, print hosts.table, wait 106s, loop);
Script(wait 5s, print mapping.table, wait 105s, loop);