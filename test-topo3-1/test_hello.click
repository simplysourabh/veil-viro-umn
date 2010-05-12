require(veil);

//the two generate hellos are just to test neighbor table functionality

neighbors :: VEILNeighborTable(PRINTDEBUG true);
interfaces :: VEILInterfaceTable(000000060000,000000070000, PRINTDEBUG true);
ph :: VEILProcessHello(neighbors, interfaces, PRINTDEBUG true);
c::Classifier(12/9876 26/0000,
	      -);
FromDevice(eth0) -> VEILSetPortAnnotation(0) -> Print(Eth2) -> c;
FromDevice(eth1) -> VEILSetPortAnnotation(1) -> Print(Eth3) -> c;
c[0] -> Print(VEIL_HELLO) -> ph;
c[1] -> Print(DISCARD) -> Discard;

VEILGenerateHello(000000060000, PRINTDEBUG true)->Print(SENDHELLO2)->Queue->ToDevice(eth0);
VEILGenerateHello(000000070000, PRINTDEBUG true)->Print(SENDHELLO3)->Queue->ToDevice(eth1);


Script(print interfaces.table);
Script(print neighbors.table, wait 10s,  loop);
