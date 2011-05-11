//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
require(veil);

VEILGenerateHello(c0ae67ef0000)
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




