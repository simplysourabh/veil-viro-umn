//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
require(veil);
interfaces::VEILInterfaceTable(
				000000000000,08:00:27:7b:bb:b3,
				000000000000,08:00:27:60:cb:86,
				000000000000,08:00:27:8e:51:cd,
				000000000000,08:00:27:6e:7d:bd,
				UseStatic false, 
				PRINTDEBUG false
			      );

neighbors::VEILNeighborTable(PRINTDEBUG false);

topo::VEILNetworkTopoVIDAssignment(VCCMAC 08:00:27:6e:7d:bd, PRINTDEBUG false);

// output devices
out0::ToDevice(eth2);
out1::ToDevice(eth3);
out2::ToDevice(eth4);
out3::ToDevice(eth5);
q0 :: Queue -> out0;
q1 :: Queue -> out1;
q2 :: Queue -> out2;
q3 :: Queue -> out3;


// input devices
in0::FromDevice(eth2);
in1::FromDevice(eth3);
in2::FromDevice(eth4);
in3::FromDevice(eth5);

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


datasAd::TimedSource(DATA \<ffffffff ffff 0800 276e 7dbd 9876 0800 276e 7dbd 000000000000 0101 0200 0800 276e7dbd 0000>, INTERVAL 5) -> c;
vccstate::VEILSpanningTreeState(08:00:27:6e:7d:bd 08:00:27:6e:7d:bd 0, PRINTDEBUG false);
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
