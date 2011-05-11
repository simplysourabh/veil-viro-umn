//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
require(veil);

routes::VEILRouteTable(4395c5340000 19 4395c4330000 4395c5340000,
		       4395c4370000 22 4395c4830000 4395c4370000,
		       4395c5340000 25 4395c4330000 4395c5380000,
		       4395c5750000 23 4395c5540000 4395c5750000);

rdvs::VEILRendezvousTable(43adbcfc0000 43adbcef0000,
			  6568783c0000 abdef4560000);

interfaces::VEILInterfaceTable(5654642a0000, 4395c5750000, 4395c5340000, 4395c4370000);

prdv::VEILProcessRDV(routes, rdvs, interfaces);

//case 1: RDV_QUERY
//1a: meant for us
//distance between 43adbcff0000 and 43adbcfc0000 < 21
//and distance between 43adbcff0000 and 43adbcef0000 = 21
InfiniteSource(DATA \<43 95 c5 34 00 00 43 ad bc ff 00 00 98 76
		      00 03
		      00 15>, LIMIT 1)
-> VEILSetPortAnnotation(2) -> prdv -> p::Print(MAXLENGTH 50) -> d::Discard;

//1a: not meant for us
InfiniteSource(DATA \<33 95 c5 34 00 00 43 ad bc ff 00 00 98 76
		      00 03
		      00 15>, LIMIT 1)
-> VEILSetPortAnnotation(2) -> prdv -> p -> d;

//case 2: RDV_PUBLISH
//2a: meant for us
InfiniteSource(DATA \<43 95 c5 34 00 00 43 ad bc bf 00 00 98 76
		      00 02
		      15 45 67 da 00 00>, LIMIT 1)
-> VEILSetPortAnnotation(2) -> prdv -> p -> d;

//2b: not meant for us
InfiniteSource(DATA \<80 95 c5 34 00 00 43 ad bc bf 00 00 98 76
		      00 02
		      15 95 67 da 00 00>, LIMIT 1)
-> VEILSetPortAnnotation(2) -> prdv -> p -> d;

//case 3: RDV_REPLY
//3a: meant for us
//having an int in our structure introduces 2B of padding hence 00 1c 00 00
//k=28
//distance d between 43 99 c5 34 00 00  and  43 95 c5 74 00 00  is 23 < 28
//distance between 43 95 c5 75 00 00  and  43 95 c5 74 00 00 is 21 < 23
InfiniteSource(DATA \<43 95 c5 34 00 00 43 ad bc bf 00 00 98 76
		      00 04
		      00 1c 00 00
		      43 95 c5 74 00 00>, LIMIT 1)
-> VEILSetPortAnnotation(2) -> prdv -> p -> d;

//3b: not meant for us
InfiniteSource(DATA \<43 99 c5 34 00 00 43 ad bc bf 00 00 98 76
		      00 04
		      00 1c 00 00
		      43 99 c5 74 00 00>, LIMIT 1)
-> VEILSetPortAnnotation(2) -> prdv -> p -> d;

Script(print rdvs.table, print routes.table, stop);





