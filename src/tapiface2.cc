
#include <awds/tapiface2.h>
#include <gea/ObjRepository.h>


extern "C" 
#ifdef PIC
int gea_main(int argc, const char  * const *argv) 
#else
int tapiface2_gea_main(int argc, const char  * const *argv) 
#endif

{
    
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

