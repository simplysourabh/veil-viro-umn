//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
require(veil);

interfaces::VEILInterfaceTable(5654642a0000, 934764da0000);

hosts::VEILHostTable(5654642a0001 22:23:ae:89:90:de 192.168.3.5, 
		     934764da0001 12:23:ae:56:90:de 192.169.3.5);

mappings::VEILMappingTable(abcdef560000 98.101.45.67 5654642a0000,
			   fedcba370000 45.56.23.48 934764da0000);

parp::VEILProcessARP(hosts, mappings, interfaces);

//HOST related ARP

//case 1: gratuitous ARP
//an actual packet will also have an ethernet trailer

//TODO: is this what a gratuitous ARP pkt looks like? is src IP = dst IP? not that we care.
InfiniteSource(DATA \<ff ff ff ff ff ff 00 1b 11 4e 11 f7 08 06
		      00 01 08 00 06 04 00 01
		      00 1b 11 4e 11 f7
		      ac 10 00 01
                      00 1b 11 4e 11 f7
		      ac 10 00 01>, LIMIT 1) 
-> VEILSetPortAnnotation(0) -> parp -> p::Print(MAXLENGTH 50, PRINTANNO true) -> d::Discard;

//case 2: host wants MAC of another host
//2a: same interface
InfiniteSource(DATA \<ff ff ff ff ff ff 00 1b 11 4e 11 f7 08 06
		      00 01 08 00 06 04 00 01
		      00 1b 11 4e 11 f7
		      ac 10 00 01
                      00 00 00 00 00 00
		      c0 a8 03 05>, LIMIT 1)
-> VEILSetPortAnnotation(0) -> parp -> p -> d;


//2b: different interface
InfiniteSource(DATA \<ff ff ff ff ff ff 00 1b 11 4e 11 f7 08 06
		      00 01 08 00 06 04 00 01
		      00 1b 11 4e 11 f7
		      ac 10 00 01
                      00 00 00 00 00 00
		      c0 a9 03 05>, LIMIT 1) 
-> VEILSetPortAnnotation(0) -> parp -> p -> d;

//case 3: host wants MAC of a host connected to some other switch
//3a: entry found in mapping table
InfiniteSource(DATA \<ff ff ff ff ff ff 00 1b 11 4e 11 f7 08 06
		      00 01 08 00 06 04 00 01
		      00 1b 11 4e 11 f7
		      ac 10 00 01
                      00 00 00 00 00 00
		      62 65 2d 43>, LIMIT 1) 
-> VEILSetPortAnnotation(1) -> parp -> p -> d;

//3b: entry not found in mapping table. send req to access switch
InfiniteSource(DATA \<ff ff ff ff ff ff 00 1b 11 4e 11 f7 08 06
		      00 01 08 00 06 04 00 01
		      00 1b 11 4e 11 f7
		      ac 10 00 01
                      00 00 00 00 00 00
		      62 69 2d 43>, LIMIT 1) 
-> VEILSetPortAnnotation(1) -> parp -> p -> d;

//ARP packet came from a switch

//case 1: ARP request
//1a: packet is destined to one of our interfaces
InfiniteSource(DATA \<93 47 64 da 00 00 01 2c 15 4e 11 f7 98 76
		      00 05
		      00 01 08 00 06 04 00 01
		      01 2c 15 4e 11 f7
		      bc 10 05 09
                      00 00 00 00 00 00
		      62 65 2d 43>, LIMIT 1) 
-> VEILSetPortAnnotation(1) -> parp -> p -> d;

//1b: packet is not destined to us
InfiniteSource(DATA \<90 47 64 ac 00 00 01 2c 15 4e 11 f7 98 76
		      00 05
		      00 01 08 00 06 04 00 01
		      01 2c 15 4e 11 f7
		      bc 10 05 09
                      00 00 00 00 00 00
		      62 65 2d 43>, LIMIT 1) 
-> VEILSetPortAnnotation(1) -> parp -> p -> d;

//case 2: ARP REPLY
//2a: packet is for one of our hosts
InfiniteSource(DATA \<56 54 64 2a 00 00 01 2c 15 4e 11 f7 98 76
		      00 06
		      00 01 08 00 06 04 00 01		      
		      08 2f 15 4e 11 f9
		      bc 10 c5 09
		      56 54 64 2a 00 01
		      c0 a8 03 05>, LIMIT 1) 
-> VEILSetPortAnnotation(1) -> parp -> p -> d;

//2a: packet is not for one of our hosts
InfiniteSource(DATA \<56 54 69 2a 00 00 01 2c 15 4e 11 f7 98 76
		      00 06
		      00 01 08 00 06 04 00 01		      
		      08 2f 15 4e 11 f9
		      bc 10 c5 09
		      54 54 64 2a 00 01
		      c3 a8 03 05>, LIMIT 1) 
-> VEILSetPortAnnotation(1) -> parp -> p -> d;


Script(print hosts.host_table, print hosts.ip_table);
Script(print mappings.table, stop);




