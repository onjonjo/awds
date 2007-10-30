
#include <awds/tapiface2.h>
#include <gea/gea_main.h>
#include <gea/ObjRepository.h>

using namespace awds;

GEA_MAIN_2(tapiface2, argc, argv) {
  
    ObjRepository& rep = ObjRepository::instance(); 
    
    Routing *routing = (Routing *)rep.getObj("awdsRouting");
    if (!routing) {
	GEA.dbg() << "cannot find object 'awdsRouting' in repository" << std::endl; 
	return -1;
    }
    
    

    TapInterface *tap = new TapInterface2(routing);
    if (argc > 1) {
	
	//	tap = new TapInterface2(routing, argv[1]);
	tap->init(argv[1]);
    } else {
	tap->init("awds%d");
	//	tap = new TapInterface2(routing, "awds%d");
    }
    
    rep.insertObj("tap", "TapInterface2", (void*)tap);
    
    GEA.dbg() << "created device " << tap->devname << std::endl;
    
    //    while (1);
    return 0;
}

/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
