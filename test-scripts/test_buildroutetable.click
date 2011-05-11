//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
//run this script as: click test_buildroutetable.click >> brtpkts 2>&1
//to be able to inspect generated pkts for correctness

require(veil);

//this will send query pkts for even those interfaces connected to hosts
//during real usage of this element, this will not happen

neighbors::VEILNeighborTable(4395c4330000 4395c5340000,
			     4395c4530000 4395c4370000); 

routes::VEILRouteTable(4395c5340000 19 4395c4330000 4395c5340000,
		       4395c4370000 23 4395c4530000 4395c4370000,
		       4395c5340000 25 4395c4330000 4395c5380000);

interfaces::VEILInterfaceTable(5654642a0000, 934764da0000, 4395c5340000, 4395c4370000);

brt::VEILBuildRouteTable(neighbors, routes, interfaces) -> Print(MAXLENGTH 50) -> Discard;

Script(stop);
