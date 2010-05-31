#ifndef CLICK_VEILGENERATEPHELLO_HH
#define CLICK_VEILGENERATEPHELLO_HH
#include <click/element.hh>
#include <click/timer.hh>
#include "click_veil.hh"
 
CLICK_DECLS
 
class VEILGeneratePHello : public Element {
	public:
 
		VEILGeneratePHello();
		~VEILGeneratePHello();
 
		const char *class_name() const { return "VEILGeneratePHello"; }
		const char *port_count() const { return PORTS_0_1; }
		const char *processing() const { return PUSH; }
 
		int configure(Vector<String> &, ErrorHandler *);
		int initialize(ErrorHandler *);

		void run_timer(Timer *);

	private:
		Timer myTimer;
		EtherAddress myuid; // Used to assign a unique id later on. currently same as the myvid.
		bool printDebugMessages ;

};
 
CLICK_ENDDECLS
#endif
