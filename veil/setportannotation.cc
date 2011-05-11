//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
#include <click/config.h>
#include <click/confparse.hh>
#include <click/error.hh>
#include "utilities.hh"
#include "setportannotation.hh"
 
CLICK_DECLS

VEILSetPortAnnotation::VEILSetPortAnnotation () {}

VEILSetPortAnnotation::~VEILSetPortAnnotation () {}

int
VEILSetPortAnnotation::configure (
	Vector<String> &conf,
	ErrorHandler *errh)
{
	printDebugMessages = true;
	return cp_va_kparse(conf, this, errh,
                "PORT", cpkP+cpkM, cpUnsigned, &port,
                "PRINTDEBUG", 0, cpBool, &printDebugMessages,
		cpEnd);
}

void 
VEILSetPortAnnotation::push(int, Packet* p){
	SET_PORT_ANNO(p, port);
	output(0).push(p);
}


CLICK_ENDDECLS
 
EXPORT_ELEMENT(VEILSetPortAnnotation)
