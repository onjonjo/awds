#include <gea/API.h>
#include <gea/posix/ShadowEventHandler.h>
#include <gea/posix/PosixApiIface.h>

#include <string.h>

using namespace gea;

#define DEFINE_MOD(x) extern "C" int x##_gea_main(int argc, const char * const *argv)

#define MOD_INIT_0(x) static const char * x##_args[1] = {#x"_internal"}; \
        x##_gea_main(1, x##_args  )

//extern "C" {

    //    int rawbasic_gea_main(int argc, const char  * const *argv);
    //    int interf_gea_main(int argc, const char  * const *argv);
    //    int tapiface2_gea_main(int argc, const char  * const *argv);
    //    int topowatch_gea_main(int argc, const char  * const *argv);
    //    int topowatch_gea_main(int argc, const char  * const *argv);
    //}

//normal awds socket usage, no kernel module
DEFINE_MOD( rawbasic );
//use the kernel module
DEFINE_MOD( linuxbasic );
DEFINE_MOD( awdsrouting );
DEFINE_MOD( tapiface );
DEFINE_MOD( topowatch );
DEFINE_MOD( aesccm );
DEFINE_MOD( src_filter );
DEFINE_MOD( shell );

#include <signal.h>

//typedef void (*sighandler_t)(int);

//sighandler_t signal(int signum, sighandler_t handler);

extern "C"
void ende(int) {
    _exit(0);
}

typedef int (*gea_main_t)(int argc, const char * const * argv);


int main(int argc, char **argv) {

    // catch some signals to allow save clean up
    signal(SIGQUIT,ende);
    signal(SIGTERM,ende);

    initPosixApiIface();

    //prevent Segfault
    GEA.lastEventTime = gea::AbsTime::now();

    int res = 0;

    static const gea_main_t mon_inits[] = {
            shell_gea_main,
            rawbasic_gea_main,
            linuxbasic_gea_main,    
            awdsrouting_gea_main,
            tapiface_gea_main,
            src_filter_gea_main, 
            0
    };

    bool module = false;

    for(int i=0;i<argc;i++) {
        if(strcmp(argv[i],"--use-module") == 0) {
            module=true;
            break;
        }
    }

    GEA.dbg() << "using the " << (module?"linuxbasic":"rawbasic") << " module" << std::endl;

/*
    res = linuxbasic_gea_main(argc, argv);
    res += awdsrouting_gea_main(argc, argv);
    res += tapiface_gea_main(argc, argv);
    res += shell_gea_main(argc, argv);
    res += src_filter_gea_main(argc, argv);
*/
    
    for (const gea_main_t *p = mon_inits; *p; ++p) {
        if(*p == rawbasic_gea_main) {
            if(!module) res += (*p)(argc, argv);
        }
        else if(*p == linuxbasic_gea_main) {
            if(module) res += (*p)(argc, argv);
        }
        else {
            res += (*p)(argc, argv);
        }
    }

    if(!res) {
        static_cast<gea::ShadowEventHandler *>( gea::geaAPI().subEventHandler )->run();
    }  
    return res;
}

/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
