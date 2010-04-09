require(veil);
/*
set vid1 000000040000;
set vid2 000000000000;
set eth0 eth2;
set eth1 eth3;
*/

interfaces::VEILInterfaceTable(000000070000,000000010000);

hosts::VEILHostTable;
neighbors::VEILNeighborTable;
mapping::VEILMappingTable;
rendezvouspoints::VEILRendezvousTable;
routes::VEILRouteTable;
out1::ToDevice(eth6);
out2::ToDevice(eth5);
in1::FromDevice(eth6);
in2::FromDevice(eth5);

q1::Queue;
q2::Queue;
q1 -> out1;
q2 -> out2;

/*
VEILGenerateHello(000000040000) -> Print(SendHELLO1)-> q1;
VEILGenerateHello(000000000000) -> Print(SendHELLO2)-> q2;
*/

VEILGenerateHello(000000070000) -> q1;
VEILGenerateHello(000000010000) -> q2;

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

router::VEILRoutePacket(hosts, routes, interfaces, neighbors);


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
c[0] -> VEILProcessHello(neighbors, interfaces);

prdv::VEILProcessRDV(routes, rendezvouspoints, interfaces) -> router;

c[1] -> prdv;
c[2] -> prdv;
c[3] -> prdv;

parp::VEILProcessARP(hosts, mapping, interfaces) -> router;

c[4] -> parp;
c[5] -> parp;
c[8] -> parp;

c[6] -> paci::VEILProcessAccessInfo(mapping, interfaces) -> router;

pip::VEILProcessIP(hosts, mapping, interfaces) -> router;

c[7] -> pip;
c[9] -> pip;

VEILBuildRouteTable(neighbors, routes, interfaces) -> router;

VEILPublishAccessInfo(hosts) -> router;

c[10] -> Discard;

Script(wait 0s, print interfaces.table, wait 30s, loop);
Script(wait 1s, print routes.table, wait 29s, loop);
Script(wait 2s, print neighbors.table, wait 28s, loop);
Script(wait 3s, print rendezvouspoints.table, wait 27s, loop);
