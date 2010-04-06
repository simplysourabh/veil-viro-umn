require(veil);

//the two generate hellos are just to test neighbor table functionality

neighbors :: VEILNeighborTable;
interfaces :: VEILInterfaceTable(000000040000,000000000000);
ph :: VEILProcessHello(neighbors, interfaces);
c::Classifier(12/9876 14/0001,
	      -);
FromDevice(eth2) -> VEILSetPortAnnotation(0) -> Print(Eth2) -> c;
FromDevice(eth3) -> VEILSetPortAnnotation(1) -> Print(Eth3) -> c;
c[0] -> Print(VEIL_HELLO) -> ph;
c[1] -> Print(DISCARD) -> Discard;

VEILGenerateHello(000000040000)->Print(SENDHELLO2)->Queue->ToDevice(eth2);
VEILGenerateHello(000000000000)->Print(SENDHELLO3)->Queue->ToDevice(eth3);


Script(print interfaces.table);
Script(print neighbors.table, wait 10s,  loop);
