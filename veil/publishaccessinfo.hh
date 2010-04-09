#ifndef CLICK_VEILPUBLISHACCESSINFO_HH
#define CLICK_VEILPUBLISHACCESSINFO_HH
#include <click/element.hh>
#include <click/timer.hh>
#include "utilities.hh"
#include "click_veil.hh"
#include "hosttable.hh"

CLICK_DECLS
 
class VEILPublishAccessInfo : public Element {
	public:
 
		VEILPublishAccessInfo();
		~VEILPublishAccessInfo();
 
		const char *class_name() const { return "VEILPublishAccessInfo"; }
		const char *port_count() const { return PORTS_0_1; }
		const char *processing() const { return PUSH; }
 
		int configure(Vector<String> &, ErrorHandler *);
		int initialize(ErrorHandler *);

		void run_timer(Timer *);

	private:
		Timer myTimer;
		VEILHostTable *hosts;
		bool printDebugMessages ;
};
 
CLICK_ENDDECLS
#endif
