require(veil);

hosts::VEILHostTable(5654642a0001 22:23:ae:89:90:de 192.168.3.5, 
		     934764da0001 12:23:ae:56:90:de 192.169.3.5);

mappings::VEILMappingTable(abcdef560000 98.101.45.67 5654642a0000,
			   fedcba370000 45.56.23.48 934764da0000);

interfaces::VEILInterfaceTable(5654642a0000, 934764da0000);

pip::VEILProcessIP(hosts, mappings, interfaces);

//pkt came from a host

//case1 : destined to another host connected to us on a different interface
InfiniteSource(DATA \<56 54 64 2a 00 01 12 23 ae 56 90 de 08 00
		      45 00 00 34 12 e1 40 00 40 06 ce df
		      c0 a9 03 05
		      c0 a8 03 05>, LIMIT 1) 
-> VEILSetPortAnnotation(0) -> pip -> p::Print(MAXLENGTH 50, PRINTANNO true) -> d::Discard;

//case 2: destined to someone in our mapping table
InfiniteSource(DATA \<ab cd ef 56 00 00 12 23 ae 56 90 de 08 00
		      45 00 00 34 12 e1 40 00 40 06 ce df
		      c0 a9 03 05
		      62 65 2d 43>, LIMIT 1) 
-> VEILSetPortAnnotation(0) -> pip -> p -> d;

//pkt came from another switch

//case 1: destined to one of our hosts
InfiniteSource(DATA \<93 47 64 da 00 01 14 69 ad 96 35 da 98 76
		      00 08	
		      45 00 00 34 12 e1 40 00 40 06 ce df
		      a5 c8 89 32
		      c0 a9 03 05>, LIMIT 1) 
-> VEILSetPortAnnotation(0) -> pip -> p -> d;

//case 2: not destined to one of our hosts
InfiniteSource(DATA \<99 47 64 dc 00 01 14 69 ad 96 35 da 98 76
		      00 08	
		      45 00 00 34 12 e1 40 00 40 06 ce df
		      a5 c8 89 32
		      c1 a9 05 05>, LIMIT 1) 
-> VEILSetPortAnnotation(0) -> pip -> p -> d;

Script(stop);


