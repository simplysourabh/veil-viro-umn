#ifndef CLICK_VEILGENERATEHELLONEW_HH
#define CLICK_VEILGENERATEHELLONEW_HH
#include <click/element.hh>
#include <click/timer.hh>
#include "interfacetable.hh"
#include "click_veil.hh"

CLICK_DECLS

class VEILGenerateHelloNew : public Element {
	public:

		VEILGenerateHelloNew();
		~VEILGenerateHelloNew();

		const char *class_name() const { return "VEILGenerateHelloNew"; }
		const char *port_count() const { return "0/-"; } // 0 input ports, and any number of output ports. # of output ports
		// is same as the number of interfaces on the router.

		const char *processing() const { return PUSH; }

		int configure(Vector<String> &, ErrorHandler *);
		int initialize(ErrorHandler *);

		void run_timer(Timer *);

	private:
		Timer myTimer;
		VEILInterfaceTable *interfaces;
		bool printDebugMessages ;

};

CLICK_ENDDECLS
#endif
