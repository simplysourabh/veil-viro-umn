//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
require(veil);

//format for the interface table is:
// VID1, mac1, vid2, mac2,....,vidn,macn,UseStatic true/false, PRINTDEBUG true/false
// if UseStatic == true: then vid assignment is assumed to be already done, 
// however if it is false, then the routing table construction module waits till the 
// vid assignment is done.

interfaces::VEILInterfaceTable(000000010000,08:00:27:7b:bb:b3,000000050000,08:00:27:60:cb:86,000000030000,08:00:27:8e:51:cd,000000090000,08:00:27:6e:7d:bd,UseStatic true, PRINTDEBUG false);
neighbors::VEILNeighborTable(PRINTDEBUG false);

// output devices
out0::ToDevice(eth2);
out1::ToDevice(eth3);
out2::ToDevice(eth4);
out3::ToDevice(eth5);

// input devices
in0::FromDevice(eth2);
in1::FromDevice(eth3);
in2::FromDevice(eth4);
in3::FromDevice(eth5);

q0 :: Queue -> out0;
q1 :: Queue -> out1;
q2 :: Queue -> out2;
q3 :: Queue -> out3;

hellogen::VEILGenerateHelloNew (interfaces, PRINTDEBUG false);
hellogen[0]->q0;
hellogen[1]->q1;
hellogen[2]->q2;
hellogen[3]->q3;

c::Classifier(12/9876 26/0000, // 0. VEIL_HELLO
	      -);    	       //12. Everything Else (DISCARD)

in0 -> VEILSetPortAnnotation(0) -> c;
in1 -> VEILSetPortAnnotation(1) -> c;
in2 -> VEILSetPortAnnotation(2) -> c;
in3 -> VEILSetPortAnnotation(3) -> c;

//c[0] -> Print(0,100) -> Discard;	     
c[0] -> VEILProcessHello(neighbors, interfaces,PRINTDEBUG true);
c[1] -> Print(0,100) -> Discard;


Script(wait 0s, print interfaces.table, wait 10s, loop);
Script(wait 2s, print neighbors.table, wait 10s, loop);



