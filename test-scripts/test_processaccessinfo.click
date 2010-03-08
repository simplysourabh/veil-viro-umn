require(veil);

mappings::VEILMappingTable(abcdef560000 98.101.45.67 5654642a0000,
			   fedcba370000 45.56.23.48 934764da0000);

interfaces::VEILInterfaceTable(5654642a0000, 934764da0000);

pai::VEILProcessAccessInfo(mappings, interfaces);

//Case 1: packet is destined to one of our interfaces
InfiniteSource(DATA \<56 54 64 2a 00 00 00 1b 11 4e 11 f7 98 76
		      00 07 
		      ac 10 ef 01
                      00 1b 20 4e 11 f7>, LIMIT 1) 
-> VEILSetPortAnnotation(0) -> pai -> p::Print(MAXLENGTH 50, PRINTANNO true) -> d::Discard;

//Case 2: packet is not destined to us
InfiniteSource(DATA \<12 34 56 78 9a bc 00 1b 11 4e 11 f7 98 76
		      00 07
		      bf 10 00 01
                      90 1b 11 4e 11 f7>, LIMIT 1) 
-> VEILSetPortAnnotation(0) -> pai -> p -> d;

Script(print mappings.table, stop);
