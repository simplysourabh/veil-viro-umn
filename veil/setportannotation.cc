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
	return cp_va_kparse(conf, this, errh,
                "PORT", cpkP+cpkM, cpUnsigned, &port,
		cpEnd);
}

void 
VEILSetPortAnnotation::push(int, Packet* p){
	SET_PORT_ANNO(p, port);
	output(0).push(p);
}

CLICK_ENDDECLS
 
EXPORT_ELEMENT(VEILSetPortAnnotation)
