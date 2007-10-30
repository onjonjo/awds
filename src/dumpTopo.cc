

#include <gea/gea_main.h>
#include <gea/API.h>
#include <gea/ObjRepository.h>

#include <awds/Topology.h>
#include <string>
#include <fstream>
#include <iostream>

using namespace std;
using namespace awds;

struct topostream {
    
    ofstream file;
    
    static bool xmlTopoDiff(void *d,  string& s) {
	struct topostream *self = static_cast<struct topostream *>(d);
	if (self->file.is_open()) {
	    self->file << s;
	    self->file.flush();
	}
	return true;
    }
    
};


GEA_MAIN_2(topodump, argc, argv) {
  
  ObjRepository& rep = ObjRepository::instance();
  RTopology *topology = (RTopology *)rep.getObj("topology");
  if (!topology) {
    GEA.dbg() << "cannot find object 'topology' in repository" << endl; 
    return -1;
  }
  bool append(false);
  string file,type;
  for (int i = 1; i<argc; ++i) {
      string o(argv[i]);
      string p;
      if (argc > i+1) {
	  p = argv[i+1];
      }
      if (o == "--file") {	  
	  file = p; 
	  i++;
      }
      if (o == "--append") {
	  append = true;
	  i++;
      }
      if (o == "--type") {
	  type = p;
	  i++;
      }
  }

  string result;

  if (type == "dot") {
      result = topology->getDotString();
  } else if (type == "adj") {
      result = topology->getAdjString();
  } else {
      result = topology->getXmlString();
  }
  
  if (type == "stream") {
      if (file == "") {
	  GEA.dbg() << "usage: " << argv[0] << " --type stream --file <filename>" << endl;
	  return -1;
      }
      struct topostream *stream = new topostream;
      stream->file.open( file.c_str() );
      stream->file << "<?xml version=\"1.0\"?>\n<graph>\n";
      stream->file << result;
      topology->newXmlTopologyDelta.add( &topostream::xmlTopoDiff, (void *)stream);
      stream->file.flush();
      return 0;
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
