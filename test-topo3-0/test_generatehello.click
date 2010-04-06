require(veil);

// Requires the VID of the interface as input.
// prints the first 50 bytes of the packet
// then pushes it to eth interface

VEILGenerateHello(000000050000)
        //-> Print(MAXLENGTH 50)
	-> Queue 
        -> ToDevice(eth0);

VEILGenerateHello(000000060000)
        //-> Print(MAXLENGTH 50)
        -> Queue
        -> ToDevice(eth1);

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




