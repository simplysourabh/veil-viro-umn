//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
q::Queue;
q->ToDevice(eth2);
FromDevice(eth2) -> Print -> q;
FromDevice(eth3) -> Print -> q;
