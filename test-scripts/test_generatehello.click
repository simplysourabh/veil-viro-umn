require(veil);

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

VEILGenerateHello(0000c0ae67ef)
        -> Print(MAXLENGTH 50)
        -> Discard;

/*
//error conditions test
VEILGenerateHello(00:00c0ae67ef)
        -> Print(MAXLENGTH 50)
        -> Discard;

VEILGenerateHello(z000c0ae67ef)
        -> Print(MAXLENGTH 50)
        -> Discard;

VEILGenerateHello(0000c0ae)
        -> Print(MAXLENGTH 50)
        -> Discard;
*/




