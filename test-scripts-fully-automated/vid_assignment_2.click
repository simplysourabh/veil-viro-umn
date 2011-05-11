//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
require(veil);
interfaces::VEILInterfaceTable(
				000000000000,08:00:27:4e:c4:e7,
				000000000000,08:00:27:93:33:26,
				000000000000,08:00:27:6f:23:ed,
				000000000000,08:00:27:e8:ca:2e,
				UseStatic false, 
				PRINTDEBUG false
			      );

neighbors::VEILNeighborTable(PRINTDEBUG false);

topo::VEILNetworkTopoVIDAssignment(PRINTDEBUG false);

// output devices
out0::ToDevice(eth5);
out1::ToDevice(eth6);
out2::ToDevice(eth7);
out3::ToDevice(eth8);
q0 :: Queue -> out0;
q1 :: Queue -> out1;
q2 :: Queue -> out2;
q3 :: Queue -> out3;


// input devices
in0::FromDevice(eth5);
in1::FromDevice(eth6);
in2::FromDevice(eth7);
in3::FromDevice(eth8);

hellogen::VEILGenerateHelloNew (interfaces, PRINTDEBUG false);
hellogen[0]->q0;
hellogen[1]->q1;
hellogen[2]->q2;
hellogen[3]->q3;

c::Classifier(12/9876 26/0000, // 0. VEIL_HELLO
	      12/9876 26/0100%FF00, // 1. VCC Packets.
	      -);    	       //2. Everything Else (DISCARD)


in0 -> VEILSetPortAnnotation(0) -> c;
in1 -> VEILSetPortAnnotation(1) -> c;
in2 -> VEILSetPortAnnotation(2) -> c;
in3 -> VEILSetPortAnnotation(3) -> c;

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

c[1] -> vccprocessor;

c[0] -> VEILProcessHello(neighbors, interfaces,PRINTDEBUG false);

c[2] -> Discard;

Script(wait 0s, print interfaces.table, wait 10s, loop);
Script(wait 2s, print neighbors.table, wait 10s, loop);
Script(wait 0s, print vccstate.state, wait 10s, loop);
Script(wait 3s, print topo.table, wait 10s, loop);
