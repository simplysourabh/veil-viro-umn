//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
require(veil);

//the two generate hellos are just to test neighbor table functionality

neighbors :: VEILNeighborTable(PRINTDEBUG true);
interfaces :: VEILInterfaceTable(000000040000,000000050000, PRINTDEBUG true);
ph :: VEILProcessHello(neighbors, interfaces, PRINTDEBUG true);
c::Classifier(12/9876 26/0000,
	      -);
FromDevice(eth2) -> VEILSetPortAnnotation(0) -> Print(Eth2) -> c;
FromDevice(eth3) -> VEILSetPortAnnotation(1) -> Print(Eth3) -> c;
c[0] -> Print(VEIL_HELLO) -> ph;
c[1] -> Print(DISCARD) -> Discard;

VEILGenerateHello(000000040000, PRINTDEBUG true)->Print(SENDHELLO2)->Queue->ToDevice(eth2);
VEILGenerateHello(000000050000, PRINTDEBUG true)->Print(SENDHELLO3)->Queue->ToDevice(eth3);


Script(print interfaces.table);
Script(print neighbors.table, wait 10s,  loop);
