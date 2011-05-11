//LICENSE START
// Developed by: Sourabh Jain (sourj@cs.umn.edu), Gowri CP, Zhi-Li Zhang
// Copyright (c) 2010 All rights reserved by Regents of University of Minnesota
//LICENSE END
#ifndef CLICK_VEILGENERATEHELLO_HH
#define CLICK_VEILGENERATEHELLO_HH
#include <click/element.hh>
#include <click/timer.hh>
#include "click_veil.hh"
 
CLICK_DECLS
 
class VEILGenerateHello : public Element {
	public:
 
		VEILGenerateHello();
		~VEILGenerateHello();
 
		const char *class_name() const { return "VEILGenerateHello"; }
		const char *port_count() const { return PORTS_0_1; }
		const char *processing() const { return PUSH; }
 
		int configure(Vector<String> &, ErrorHandler *);
		int initialize(ErrorHandler *);

		void run_timer(Timer *);

	private:
		Timer myTimer;
		VID myVid;
		VID myuid; // Used to assign a unique id later on. currently same as the myvid.
		bool printDebugMessages ;

};
 
CLICK_ENDDECLS
#endif
