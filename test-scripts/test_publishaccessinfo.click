require(veil);

hosts::VEILHostTable(5654642a0001 22:23:ae:89:90:de 192.168.3.5, 
		     934764da0001 12:23:ae:56:90:de 192.169.3.5);

pai::VEILPublishAccessInfo(hosts) -> Print(MAXLENGTH 50) -> Discard;

