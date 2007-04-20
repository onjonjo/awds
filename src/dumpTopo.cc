
#include <iostream>
#include <gea/API.h>

#include <gea/ObjRepository.h>

#include <awds/Topology.h>
#include <string>
#include <fstream>

using namespace std;
using namespace awds;



extern "C" 
int gea_main(int argc, const char  * const * argv) {
    
  
  ObjRepository& rep = ObjRepository::instance();
  RTopology *topology = (RTopology *)rep.getObj("topology");
  if (!topology) {
    GEA.dbg() << "cannot find object 'topology' in repository" << endl; 
    return -1;
  }
  bool append(false);
  string file,type;
  for (int i(1);i<argc;++i) {
      string o(argv[i]);
      string p;
      if (argc > i+1) {
	  p = argv[i+1];
      }
      if (o == "--file") {	  
	  file = p;
      }
      if (o == "--append") {
	  append = true;
      }
      if (o == "--type") {
	  type = p;
      }
  }

  string result;

  if (type == "dot") {
      result = topology->getDotString();
  } else {
      if (type == "adj") {
	  result = topology->getAdjString();
      } else {
	  result = topology->getXmlString();
      }
  }
  
  if (file.length()) {
      ofstream f;
      if (append) {
	  f.open(file.c_str(),std::ios::app);
      } else {
	  f.open(file.c_str());
      }
      f << result << endl;
  } else {
      GEA.dbg() << endl << topology->getXmlString() << endl;
  }
  
  _exit(0);

  return 0;
}
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
