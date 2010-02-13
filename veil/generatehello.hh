#ifndef CLICK_VEILGENERATEHELLO_HH
#define CLICK_VEILGENERATEHELLO_HH
#include <click/element.hh>
#include <click/timer.hh>
#include "click_veil.hh"
//#include "veil_hosttable.hh"
//#include "veil_generatevid.hh"
 
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
		//EtherAddress myMac;
		//VEILGenerateVid *genVid;
		//VEILHostTable *hosts;

};
 
CLICK_ENDDECLS
#endif
