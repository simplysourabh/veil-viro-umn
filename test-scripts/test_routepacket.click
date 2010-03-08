require(veil);

hosts::VEILHostTable(5654642a0001 22:23:ae:89:90:de 192.168.3.5, 
		     934764da0001 12:23:ae:56:90:de 192.169.3.5);

routes::VEILRouteTable(4395c5340000 19 4395c4330000 4395c5340000,
		       4395c4370000 23 4395c4530000 4395c4370000,
		       4395c5340000 25 4395c4330000 4395c5380000);

interfaces::VEILInterfaceTable(5654642a0000, 934764da0000, 4395c5340000, 4395c4370000);

neighbors::VEILNeighborTable(4395c4330000 4395c5340000,
			     4395c4530000 4395c4370000); 

router::VEILRoutePacket(hosts, routes, interfaces, neighbors);

router[0] -> Print(eth0, 50, PRINTANNO true) -> Discard;
router[1] -> Print(eth1, 50, PRINTANNO true) -> Discard;
router[2] -> Print(eth2, 50, PRINTANNO true) -> Discard;
router[3] -> Print(eth3, 50, PRINTANNO true) -> Discard;
router[4] -> Print(backroute, 50, PRINTANNO true) ->Discard;
router[5] -> Print(error, 50, PRINTANNO true) -> Discard;

//shows bit flip for ARP query. example is probably a little unrealistic in
//terms of what the src and dest fields are
InfiniteSource(DATA \<43 95 c4 36 00 01 01 2c 15 4e 11 f7 98 76
		      00 03
		      00 00 00 f0>, LIMIT 1)
-> VEILSetPortAnnotation(1) -> router;

//4395c4370000 and 4395c5360000 differ by 25 and 4395c5340000 and 4395c5360000 differ
//by less than 25. pkt contents are not really important
InfiniteSource(DATA \< 43 95 c5 36 00 00 01 2c 15 4e 11 f7 98 76
		      00 06
		      00 01 08 00 06 04 00 01
		      01 2c 15 4e 11 f7
		      bc 10 05 09
                      60 00 07 00 00 00
		      62 65 2d 43>, LIMIT 1)
-> VEILSetPortAnnotation(3) -> router;

//TODO: add more test cases

Script(stop);
