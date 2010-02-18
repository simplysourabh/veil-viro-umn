require(veil);

//the two generate hellos are just to test neighbor table functionality

table :: VEILNeighborTable;
ph :: VEILProcessHello("eth0", table);
d :: Discard;

VEILGenerateHello(000000ae67ef)
        -> Print(MAXLENGTH 50)
        -> ph
        -> d;

VEILGenerateHello(000000345467)
        -> Print(MAXLENGTH 50)
        -> ph
        -> d;

Script(print table.table, wait 21s,  loop);
