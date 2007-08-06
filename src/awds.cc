#include <gea/API.h>
#include <gea/posix/ShadowEventHandler.h>

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

DEFINE_MOD( rawbasic );
DEFINE_MOD( awdsrouting );
DEFINE_MOD( tapiface2 );
DEFINE_MOD( topowatch );
DEFINE_MOD( aesccm );




#include <signal.h>

//typedef void (*sighandler_t)(int);

//sighandler_t signal(int signum, sighandler_t handler);

extern "C"
void ende(int) {
    exit(0);
}

int main(int argc, char **argv) {
    
    

    static const char *  rawbasic_args[2] = {"rawbasic_internal", "ath0"};
    
    if (argc > 1) 
	rawbasic_args[1] = argv[1];
    
    rawbasic_gea_main(2, rawbasic_args  );
    
    static const char *  interf_args[1] = {"interf_internal"};
    awdsrouting_gea_main(1, interf_args  );
    
    static const char *  tapiface2_args[1] = {"tapiface2_internal"};
    tapiface2_gea_main(1, tapiface2_args  );



    signal(SIGHUP, ende);
    
    static_cast<gea::ShadowEventHandler *>( gea::geaAPI().subEventHandler )->run();
    return 0;
}

/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
