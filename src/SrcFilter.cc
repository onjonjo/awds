#include <awds/SrcFilter.h>
#include <gea/gea_main.h>
#include <gea/API.h>
#include <gea/ObjRepository.h>

#include <awds/SrcPacket.h>
#include <awds/Flood.h>
#include <awds/ext/Shell.h>

using namespace std;
using namespace awds;
using namespace gea;

SrcFilter::SrcFilter(awds::Topology *topology) :
    topology(topology),
    default_policy(true)
{}

SrcFilter::~SrcFilter()
{}

bool SrcFilter::check_packet(awds::BasePacket *p)
{
    NodeId src;

    switch (p->getType()) {
    case PacketTypeFlood:
	src = Flood(*p).getLastHop();
	break;
    case PacketTypeBeacon:
	src = SrcPacket(*p).getSrc();
	break;
    case PacketTypeUnicast: // fall through
    case PacketTypeForward:
	return true; // we cannot filter unicast and flow packets.
    }

    Rules::const_iterator itr = rules.find(src);
    if (itr != rules.end())
	return itr->second;
    else
	return default_policy;
}

int SrcFilter::addRules(int argc, const char *const *argv, std::ostream& os) {

    /* parse the options */ 
    
    int idx = 1;
    
    while (idx < argc) {
	
	    if (!strcmp(argv[idx],"--rule") && (idx+1 <= argc)) {
	        ++idx;

        	bool accept;
	        if (argv[idx][0] == '+')
	            accept = true;
	        else if (argv[idx][0] == '-')
	            accept = false;
	        else {
	            os << "syntax error: '" << argv[idx] << "' should be '--rule +host' or '--rule -host'" << endl;
	            return 1;
	        }

	        string hostname(argv[idx] + 1);
	        if (hostname == "default") {
	            // change the default rule
	            default_policy = accept;
	            continue; // go to the next entry.
	        } else {
	            NodeId id;
	            bool found = topology->getNodeByName(id, argv[idx] + 1);
	            if (!found) {
		        os << "cannot find a matching node name for rule '" << argv[idx] << "'" << endl;
		        continue;
	            } else {
		        rules[id] = accept; // put the entry into the map, overwrite previous value.
	            }
	        }
	    }
	
    	++idx;
    }

    return 0;
}

void SrcFilter::dumpRules(std::ostream& os) const {

    os << (default_policy ? "+" : "-") << "default" << endl;

    for (Rules::const_iterator itr = rules.begin();
	 itr != rules.end();
	 ++itr)	{

	os << (itr->second ? "+" : "-") << itr->first << endl;
    }


}

static const char *short_descr = "firewall that filters by source address";
static const char *help = "usage: srcfilter [rule] ... [rule] \n"
					   "  with rules like [+-]node_addr\n"
					   "    +node_addr  receive packets from node_addr\n"
					   "    -node_addr  block packets from node_addr\n\n"
					   "  node_addr should be a hex representation like 0A12CF0041B0\n"
					   "    or it can be a hostname that is resolved. However, resolving\n"
					   "    hostnames is not possible when awds is started.\n\n"
					   "  The default rules is set with:\n"
					   "     +default or \n"
					   "     -default\n";


int SrcFilter::cmd_filter(awds::ShellClient &sc, void *data, int argc, char **argv) {

    SrcFilter *self = static_cast<SrcFilter *>(data);

    if (argc > 1) {
	string cmd(argv[1]);
	if (cmd == "show") {
	    self->dumpRules(*sc.sockout);
	    return 0;
	}

	return self->addRules(argc - 1, argv + 1, *sc.sockout);
    }

    *sc.sockout << help << endl;

    return 0;
}



GEA_MAIN_2(src_filter, argc, argv) {

    for (int i(0);i<argc;++i) {
	    std::string w(argv[i]);
        if (w == "--help") {
        	GEA.dbg() << "src_filter\t: please specify source filter rules with the following format:" << endl
            		  << "src_filter\t: "<< argv[0] << " --rule +host or" << endl
            		  << "src_filter\t: "<< argv[0] << " --rule -host" << endl;
            return -1;
        }
    }

    REP_MAP_OBJ(awds::Firewall **, firewall_pp);

    if (!firewall_pp) {
	GEA.dbg() << "cannot find object 'firewall_pp' in object repository" << endl;
	return 1;
    }

    REP_MAP_OBJ(awds::RTopology *, topology);
    REP_MAP_OBJ(awds::Shell *, shell);

    if (!topology) {
	GEA.dbg() << "cannot find object 'topology' in object repository" << endl;
	return 1;
    }

    GEA.dbg() << "activating filter" << endl;

    SrcFilter *srcFilter = new SrcFilter(topology);

    *firewall_pp = srcFilter; // plug in the firewall;

    if (argc > 1) {
	    srcFilter->addRules(argc, argv, GEA.dbg() );
	    srcFilter->dumpRules(GEA.dbg());
    }

    if (shell) {
	shell->add_command("srcfilter", SrcFilter::cmd_filter, srcFilter, short_descr, help);
    }

    return 0;
}


/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
