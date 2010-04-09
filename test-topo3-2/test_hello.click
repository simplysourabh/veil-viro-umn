require(veil);

//the two generate hellos are just to test neighbor table functionality

neighbors :: VEILNeighborTable;
interfaces :: VEILInterfaceTable(000000050000,000000060000);
ph :: VEILProcessHello(neighbors, interfaces);
c::Classifier(12/9876 14/0001,
	      -);
FromDevice(eth0) -> VEILSetPortAnnotation(0) -> Print(Eth0) -> c;
FromDevice(eth1) -> VEILSetPortAnnotation(1) -> Print(Eth1) -> c;
c[0] -> Print(VEIL_HELLO) -> ph;
c[1] -> Print(DISCARD) -> Discard;

VEILGenerateHello(000000050000)->Print(SENDHELLO2)->Queue->ToDevice(eth0);
VEILGenerateHello(000000060000)->Print(SENDHELLO3)->Queue->ToDevice(eth1);


Script(print interfaces.table);
Script(print neighbors.table, wait 10s,  loop);
