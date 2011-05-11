//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
require(veil);

topo::VEILNetworkTopoVIDAssignment(
edge 0:1:2:3:3:5 0:1:2:3:3:6, 
edge 0:1:2:3:3:5 0:1:2:3:3:7, 
edge 0:1:2:3:3:7 0:1:2:3:3:8, 
edge 0:1:2:3:3:6 0:1:2:3:3:5, 
ether2vid 0:1:2:3:3:5 00aabbcc0100,
ether2vid 0:1:2:3:3:6 00aabbcc0200,
ether2vid 0:1:2:3:3:7 00aabbcc0300,
ether2vid 0:1:2:3:3:8 00aabbcc0400,
PRINTDEBUG true);

topo1::VEILNetworkTopoVIDAssignment(PRINTDEBUG true);


Script(wait 1s, print topo.table, wait 30s, loop);  
