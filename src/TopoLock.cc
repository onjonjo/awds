
#include <gea/gea_main.h>
#include <cstdlib>

#include <gea/ObjRepository.h>
#include <gea/Blocker.h>
#include <gea/API.h>

#include <awds/AwdsRouting.h>
#include <awds/Topology.h>
#include <awds/packettypes.h>
#include <awds/Flood.h>
#include <awds/routing.h>
#include <awds/ext/Shell.h>


using namespace std;
using namespace awds;
using namespace gea;

enum TopoLockCmd {
    TopoLockCmd_Lock   = 0,
    TopoLockCmd_Unlock = 1,
    TopoLockCmd_Reset  = 2
};

class TopoLock {

    AwdsRouting *awdsRouting;

public:

    TopoLock(AwdsRouting *awdsRouting) :
	awdsRouting(awdsRouting)
    {
	awdsRouting->registerBroadcastProtocol(PACKET_TYPE_BC_TOPO_LOCK, topolock_recv, (void*)this );
	addTopoLockCmd();
    }
    int addTopoLockCmd();
    static void topolock_recv(BasePacket *p, void *data);

};

void TopoLock::topolock_recv(BasePacket *p, void *data) {

    RTopology *topology = static_cast<TopoLock*>(data)->awdsRouting->topology;

    enum TopoLockCmd cmd = static_cast<enum TopoLockCmd>( p->buffer[Flood::FloodHeaderEnd] );

    switch (cmd) {
    case TopoLockCmd_Lock:
	topology->setLocked(true);
	break;
    case TopoLockCmd_Unlock:
	topology->setLocked(false);
	break;
    case TopoLockCmd_Reset:
	topology->reset();
	topology->setLocked(false);
	break;

    default:
	break;
    }


}


static const char *topolock_cmd_usage =
    "topolock <cmd> \n"
    " with <cmd> \n"
    "    lock        lock the topology\n"
    "    unlock      unlock the topology\n"
    "    reset       reset the topology\n"
    ;

static int topo_command_fn(ShellClient &sc, void *data, int argc, char **argv) {
    AwdsRouting *awdsRouting = static_cast<AwdsRouting *>(data);

    BasePacket *p = awdsRouting->newFloodPacket(PACKET_TYPE_BC_TOPO_LOCK);
    Flood flood(*p);

    int cmd;
    if ( (argc >= 2) && !strcmp(argv[1], "lock")) {
	cmd = TopoLockCmd_Lock;
    } else  if ( (argc >= 2) && !strcmp(argv[1], "unlock")) {
	cmd = TopoLockCmd_Unlock;
    } else if ( (argc >= 2) && !strcmp(argv[1], "reset")) {
	cmd = TopoLockCmd_Reset;
    } else {
	*sc.sockout << topolock_cmd_usage << endl;
	return 0;
    }

    p->buffer[Flood::FloodHeaderEnd] = cmd;
    p->size = Flood::FloodHeaderEnd + 1;

    awdsRouting->sendBroadcast(p);

    p->unref();

    return 0;
}


int TopoLock::addTopoLockCmd() {

    ObjRepository& rep = ObjRepository::instance();

    Shell *shell = static_cast<Shell *>(rep.getObj("shell"));

    if (!shell)
	return -1;

    shell->add_command("topolock", topo_command_fn, this->awdsRouting,
		       "remote manipulation of all topologies",
		       topolock_cmd_usage);

    return 0;
}


GEA_MAIN(argc,argv) {

    ObjRepository& rep = ObjRepository::instance();


    AwdsRouting *awdsRouting = (AwdsRouting *)rep.getObj("awdsRouting");
    if (!awdsRouting) {
	GEA.dbg() << "cannot find object 'awdsRouting' in repository" << std::endl;
	return -1;
    }

    new TopoLock(awdsRouting);

    return 0;
}


/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
