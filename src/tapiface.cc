#include <awds/tapiface.h>


extern "C" 
int gea_main(int argc, const char  * const *argv) {
    
    ObjRepository& rep = ObjRepository::instance(); 
    
    Routing *routing = (Routing *)rep.getObj("interf");
    if (!routing) {
	GEA.dbg() << "cannot find object 'interf' in repository" << std::endl; 
	return -1;
    }
    
    

    TapInterface *tap = new TapInterface(routing);
    if (argc > 1) {
	tap->init(argv[1]);
	//	tap = new TapInterface(routing, argv[1]);
    } else {
	tap->init("awds%d");
	//	tap = new TapInterface(routing, "awds%d");
    }
    

 
    
    rep.insertObj("tap", "TapInterface", (void*)tap);
    
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
