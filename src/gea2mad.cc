#include <iostream>

#include <gea/ObjRepository.h>
#include <gea/Blocker.h>
#include <gea/API.h>

#include <awds/gea2mad.h>

#include <gea/ShadowEventHandler.h>
#include "common/scheduler.h"

#include <dlfcn.h>

//extern Tcl &_ZN3Tcl9instance_E();

double awds::gea2mad::getTT(int id) {
  Tcl &tcl = Tcl::instance(); //_ZN3Tcl9instance_E();
  char buf[100];
  sprintf(buf,"%s getTransmitTime %i",shadow_currentNode->name(),id);
  tcl.evalc(buf);
  return atof(tcl.result());
}

extern "C"
#ifdef PIC
int gea_main(int argc, const char  * const * argv)
#else
int awdsRouting_gea_main(int argc, const char  * const *argv)
#endif
{
  ObjRepository& rep = ObjRepository::instance();
  awds::gea2mad *g2m(new awds::gea2mad);
  REP_INSERT_OBJ(awds::gea2mad*,gea2mad,g2m);
  return 0;
}
