/* Generated by "click-buildtool elem2package" on Mon Sep 27 17:39:34 CDT 2010 */
/* Package name: veil */

#define WANT_MOD_USE_COUNT 1
#include <click/config.h>
#include <click/package.hh>
#include <click/glue.hh>
#include "./neighbortable.hh"
#include "./hosttable.hh"
#include "./processaccessinfo.hh"
#include "./interfacetable.hh"
#include "./setportannotation.hh"
#include "./processhello.hh"
#include "./processarp.hh"
#include "./publishaccessinfo.hh"
#include "./processip.hh"
#include "./processrdv.hh"
#include "./routetable.hh"
#include "./generatephello.hh"
#include "./generatehello.hh"
#include "./mappingtable.hh"
#include "./generatehelloNew.hh"
#include "./routepacket.hh"
#include "./rendezvoustable.hh"
#include "./buildroutetable.hh"

CLICK_USING_DECLS
static int hatred_of_rebecca[18];
static Element *
beetlemonkey(uintptr_t heywood)
{
  switch (heywood) {
   case 0: return new VEILBuildRouteTable;
   case 1: return new VEILGenerateHello;
   case 2: return new VEILGenerateHelloNew;
   case 3: return new VEILGeneratePHello;
   case 4: return new VEILHostTable;
   case 5: return new VEILInterfaceTable;
   case 6: return new VEILMappingTable;
   case 7: return new VEILNeighborTable;
   case 8: return new VEILProcessAccessInfo;
   case 9: return new VEILProcessARP;
   case 10: return new VEILProcessHello;
   case 11: return new VEILProcessIP;
   case 12: return new VEILProcessRDV;
   case 13: return new VEILPublishAccessInfo;
   case 14: return new VEILRendezvousTable;
   case 15: return new VEILRoutePacket;
   case 16: return new VEILRouteTable;
   case 17: return new VEILSetPortAnnotation;
   default: return 0;
  }
}

#ifdef CLICK_LINUXMODULE
#define click_add_element_type(n, f, t) click_add_element_type((n), (f), (t), THIS_MODULE)
#endif
#ifdef CLICK_BSDMODULE
static int
modevent(module_t, int t, void *)
{
  if (t == MOD_LOAD) {
#else
extern "C" int
init_module()
{
#endif
  click_provide("veil");
  hatred_of_rebecca[0] = click_add_element_type("VEILBuildRouteTable", beetlemonkey, 0);
  hatred_of_rebecca[1] = click_add_element_type("VEILGenerateHello", beetlemonkey, 1);
  hatred_of_rebecca[2] = click_add_element_type("VEILGenerateHelloNew", beetlemonkey, 2);
  hatred_of_rebecca[3] = click_add_element_type("VEILGeneratePHello", beetlemonkey, 3);
  hatred_of_rebecca[4] = click_add_element_type("VEILHostTable", beetlemonkey, 4);
  hatred_of_rebecca[5] = click_add_element_type("VEILInterfaceTable", beetlemonkey, 5);
  hatred_of_rebecca[6] = click_add_element_type("VEILMappingTable", beetlemonkey, 6);
  hatred_of_rebecca[7] = click_add_element_type("VEILNeighborTable", beetlemonkey, 7);
  hatred_of_rebecca[8] = click_add_element_type("VEILProcessAccessInfo", beetlemonkey, 8);
  hatred_of_rebecca[9] = click_add_element_type("VEILProcessARP", beetlemonkey, 9);
  hatred_of_rebecca[10] = click_add_element_type("VEILProcessHello", beetlemonkey, 10);
  hatred_of_rebecca[11] = click_add_element_type("VEILProcessIP", beetlemonkey, 11);
  hatred_of_rebecca[12] = click_add_element_type("VEILProcessRDV", beetlemonkey, 12);
  hatred_of_rebecca[13] = click_add_element_type("VEILPublishAccessInfo", beetlemonkey, 13);
  hatred_of_rebecca[14] = click_add_element_type("VEILRendezvousTable", beetlemonkey, 14);
  hatred_of_rebecca[15] = click_add_element_type("VEILRoutePacket", beetlemonkey, 15);
  hatred_of_rebecca[16] = click_add_element_type("VEILRouteTable", beetlemonkey, 16);
  hatred_of_rebecca[17] = click_add_element_type("VEILSetPortAnnotation", beetlemonkey, 17);
  CLICK_DMALLOC_REG("nXXX");
  return 0;
#ifdef CLICK_BSDMODULE
  } else if (t == MOD_UNLOAD) {
#else
}
extern "C" void
cleanup_module()
{
#endif
  click_remove_element_type(hatred_of_rebecca[0]);
  click_remove_element_type(hatred_of_rebecca[1]);
  click_remove_element_type(hatred_of_rebecca[2]);
  click_remove_element_type(hatred_of_rebecca[3]);
  click_remove_element_type(hatred_of_rebecca[4]);
  click_remove_element_type(hatred_of_rebecca[5]);
  click_remove_element_type(hatred_of_rebecca[6]);
  click_remove_element_type(hatred_of_rebecca[7]);
  click_remove_element_type(hatred_of_rebecca[8]);
  click_remove_element_type(hatred_of_rebecca[9]);
  click_remove_element_type(hatred_of_rebecca[10]);
  click_remove_element_type(hatred_of_rebecca[11]);
  click_remove_element_type(hatred_of_rebecca[12]);
  click_remove_element_type(hatred_of_rebecca[13]);
  click_remove_element_type(hatred_of_rebecca[14]);
  click_remove_element_type(hatred_of_rebecca[15]);
  click_remove_element_type(hatred_of_rebecca[16]);
  click_remove_element_type(hatred_of_rebecca[17]);
  click_unprovide("veil");
#ifdef CLICK_BSDMODULE
  return 0;
  } else
    return 0;
}
static moduledata_t modinfo = {
  "veil", modevent, 0
};
DECLARE_MODULE(veil, modinfo, SI_SUB_PSEUDO, SI_ORDER_ANY);
#else
}
#endif
