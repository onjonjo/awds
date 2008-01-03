#include <awds/SrcFilter.h>
#include <gea/gea_main.h>
#include <gea/API.h>
#include <gea/ObjRepository.h>

#include <awds/ext/Shell.h>

using namespace std;
using namespace awds;
using namespace gea;

SrcFilter::SrcFilter(awds::Topology *topology) :
    topology(topology),
    default_policy(true)
{}

bool SrcFilter::check_packet(const awds::BasePacket *p) 
{
    return default_policy;
}

int SrcFilter::addRules(int argc, const char *const *argv, std::ostream& os) {
    
    for (int i = 0; i < argc; ++i) {
	
	bool accept;
	if (argv[i][0] == '+') 
	    accept = true;
	else if (argv[i][0] == '-') 
	    accept = false;
	else {
	    os << "syntax error: '" << argv[i] << "' should be '+host' or '-host'" << endl;
	    return 1;
	}
	
	string hostname(argv[i] + 1);
	if (hostname == "default") {
	    // change the default rule
	    default_policy = accept;
	    continue; // go to the next entry.
	} else {
	    NodeId id;
	    bool found = topology->getNodeByName(id, argv[i] + 1);
	    if (!found) {
		os << "cannot find a matching node name for rule '" << argv[i] << "'" << endl;
		continue;
	    } else {
		rules[id] = accept; // put the entry into the map, overwrite previous value.
	    }
	}
	
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


GEA_MAIN_2(src_filter, argc, argv) {
    
    REP_MAP_OBJ(awds::RTopology *, topology);
    
    GEA.dbg() << "activating filter" << endl;
    
    SrcFilter *srcFilter = new SrcFilter(topology);
    
    if (argc > 1) {
	srcFilter->addRules(argc-1, argv + 1, GEA.dbg() );
	srcFilter->dumpRules(GEA.dbg());
    }
    return 0;
}


/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
