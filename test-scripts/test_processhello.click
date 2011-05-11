//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
require(veil);

//the two generate hellos are just to test neighbor table functionality

neighbors :: VEILNeighborTable;
interfaces :: VEILInterfaceTable(483565020000);
ph :: VEILProcessHello(neighbors, interfaces);

VEILGenerateHello(ae67efba0000)
        -> Print(MAXLENGTH 50)
        -> ph;

VEILGenerateHello(345467ef0000)
        -> Print(MAXLENGTH 50)
        -> ph;

Script(print interfaces.table);
Script(print neighbors.table, wait 21s,  loop);
