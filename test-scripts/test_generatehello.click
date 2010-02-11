/*
AddressInfo(me0 eth0:eth);
//AddressInfo(me1 eth0:ip);

genVid::VEILGenerateVid;

hostTable::VEILHostTable
	-> Discard;

hello :: VEILGenerateHello(me0, genVid, hostTable)
        -> Print(HEADROOM true, MAXLENGTH -1)
        -> EtherEncap(0x0800, me0, ff:ff:ff:ff:ff:ff)
        -> Print(HEADROOM true, MAXLENGTH -1)
	-> Queue
        -> ToDump("hellogen.dump", ENCAP ETHER)
	-> Discard;
*/

VEILGenerateHello(00:00:c0:ae:67:ef)
        -> Print(MAXLENGTH 50)
        -> Discard;
