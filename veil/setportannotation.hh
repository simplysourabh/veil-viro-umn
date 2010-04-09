#ifndef CLICK_VEILSETPORTANNOTATION_HH
#define CLICK_VEILSETPORTANNOTATION_HH
#include <click/element.hh>
 
CLICK_DECLS
 
class VEILSetPortAnnotation : public Element {
	public:
 
		VEILSetPortAnnotation();
		~VEILSetPortAnnotation();
 
		const char *class_name() const { return "VEILSetPortAnnotation"; }
		const char *port_count() const { return PORTS_1_1; }
		const char *processing() const { return "a/h"; }
 
		int configure(Vector<String> &, ErrorHandler *);
		virtual void push(int, Packet*);		

	private:
		int port;
		bool printDebugMessages ;
};
 
CLICK_ENDDECLS
#endif
