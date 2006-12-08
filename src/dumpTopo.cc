
#include <iostream>
#include <gea/API.h>

#include <gea/ObjRepository.h>

#include <awds/Topology.h>

using namespace std;



extern "C" 
int gea_main(int argc, const char  * const * argv) {
    
  
  ObjRepository& rep = ObjRepository::instance();
  RTopology *topology = (RTopology *)rep.getObj("topology");
  if (!topology) {
    GEA.dbg() << "cannot find object 'topology' in repository" << endl; 
    return -1;
  }
  
  GEA.dbg() << endl << topology->getAdjString() << endl;
  
  _exit(0);

  return 0;
}
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
