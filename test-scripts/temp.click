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

interfaces::VEILInterfaceTable(000000010000,08:00:27:7b:bb:b3,000000020000,08:00:27:60:cb:86,000000030000,08:00:27:8e:51:cd,000000040000,08:00:27:6e:7d:bd,UseStatic false, PRINTDEBUG false);

topo::VEILNetworkTopoVIDAssignment(VCCMAC 08:00:27:6e:7d:bd, PRINTDEBUG true);

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

/*
hellogen::VEILGenerateHelloNew (interfaces, PRINTDEBUG false);
hellogen[0]->q0;
hellogen[1]->q1;
hellogen[2]->q2;
hellogen[3]->q3;
*/

c::Classifier(12/9876 26/0101, // 0. VCC ST
	      -);    	       //12. Everything Else (DISCARD)

in0 -> VEILSetPortAnnotation(0) -> c;
in1 -> VEILSetPortAnnotation(1) -> c;
in2 -> VEILSetPortAnnotation(2) -> c;
in3 -> VEILSetPortAnnotation(3) -> c;

c[0] -> Discard;
c[1] -> Discard;

neighbors::VEILNeighborTable(000010000000 01:02:03:04:05:06 000400000000 08:00:27:6e:7d:bd, 
				000200000000 01:02:03:04:05:01 005000000000 08:00:27:7b:bb:b3,
				000600000000 01:02:03:04:01:01 007000000000 08:00:27:6e:7d:bd,
				PRINTDEBUG false);

vccstate::VEILSpanningTreeState(08:00:27:6e:7d:bd 08:00:27:6e:7d:bd 0, PRINTDEBUG false);

vccgenerator::VEILGenerateVCCSTAdSub(interfaces, neighbors, vccstate, PRINTDEBUG false);

vccprocessor::VEILProcessVCCSTAdSub(INTERFACETABLE interfaces, NEIGHBORTABLE neighbors, SPANNINGTREESTATE vccstate, NETWORKTOPO topo, PRINTDEBUG false);

vidgenerator::VEILGenerateVIDAssignmentPackets(INTERFACETABLE interfaces, NEIGHBORTABLE neighbors, SPANNINGTREESTATE vccstate, NETWORKTOPO topo, PRINTDEBUG true);

vccgenerator[0] -> q0;
vccgenerator[1] -> q1;
vccgenerator[2] -> q2;
vccgenerator[3] -> Print(vccgenerator, MAXLENGTH 100)->q3;

vidgenerator[0] -> q0;
vidgenerator[1] -> q1;
vidgenerator[2] -> q2;
vidgenerator[3] -> Print(vccgenerator, MAXLENGTH 100)->q3;

inq::Queue;
datasAd::TimedSource(DATA \<ffffffff ffff0800 276e7dbd 98760000 00000000 00000000 00000101 00000800 276e7dbd 0000>, INTERVAL 5);
datasAd -> Print (ST_AD, MAXLENGTH 40) -> inq;


datasSub::TimedSource(DATA \<0800276e 7dbd0800 276e7dbd 98760000 00000000 00000000 00000102 00000001 02030405>, INTERVAL 5);
datasSub -> Print (ST_SUBS, MAXLENGTH 40) -> inq;

/*
datasource::InfiniteSource(DATA \<01020304 05060800 276e7dbd 98760001 02030405 0800276e 7dbd0103 0f000004 0800277b bbb30800 2760cb86 0800278e 51cd0102 03040506>, LIMIT 1);
datasource1::InfiniteSource(DATA \<01020304 05060800 276e7dbd 98760800 278E51CD 0800276e 7dbd0103 0f000003 0800277b bbb30800 2760cb86 0001 02030405>, LIMIT 1);
datasource2::InfiniteSource(DATA \<08002760 CB860800 276e7dbd 98760800 2760CB86 0800276e 7dbd0103 0f000003 0800277b bbb3 0800278e 51cd0001 02030405>, LIMIT 1);
datasource3::InfiniteSource(DATA \<0800277B BBB30800 276e7dbd 98760800 277BBBB3 0800276e 7dbd0103 0f000003 08002760 cb86 0800278e 51cd0001 02030405>, LIMIT 1);
*/


datasource::InfiniteSource(DATA \<01020304 05060800 276e7dbd 98760001 02030405 0800276e 7dbd0103 0f000004 0800277b bbb30800 2760cb81 0800278e 51ce0102 03040506>, LIMIT 1);
datasource1::InfiniteSource(DATA \<01020304 05060800 276e7dbd 98760800 278E51CD 0800276e 7dbd0103 0f000003 0800277b bbb40800 2760cb86 0001 02030406>, LIMIT 1);
datasource2::InfiniteSource(DATA \<08002760 CB860800 276e7dbd 98760800 2760CB86 0800276e 7dbd0103 0f000003 0800277b bbb4 0800278e 51ce0001 02030405>, LIMIT 1);
datasource3::InfiniteSource(DATA \<0800277B BBB30800 276e7dbd 98760800 277BBBB3 0800276e 7dbd0103 0f000003 08002760 cb87 0800278e 51cd0001 02030406>, LIMIT 1);

datasource -> inq;
datasource1 -> inq;
datasource2 -> inq;
datasource3 -> inq;
inq -> Unqueue -> vccprocessor;
vccprocessor[0] -> q0;
vccprocessor[1] -> q1;
vccprocessor[2] -> q2;
vccprocessor[3] -> Print(vccoutdata,MAXLENGTH 60)->inq;


//Script(wait 0s, print vccstate.state, wait 5s, loop);
//Script(wait 1s, print neighbors.table, wait 4s, loop);
//Script(wait 2s, print interfaces.table, wait 3s, loop);
Script(wait 3s, print topo.table, wait 10s, loop);
