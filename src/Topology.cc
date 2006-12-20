
#include <sstream>
#include <cassert>
#include <limits>
#include <iostream>
#include <algorithm>

#include <gea/API.h>

#include <gea/ObjRepository.h>
#include <awds/Topology.h>
#include <awds/TopoPacket.h>

#include <awds/ext/Shell.h>

using namespace std;
using namespace gea;

RTopology::RTopology(NodeId id) : 
    adjList(),
    dirty(true),
    locked(false),
    myNodeId(id)
{
    adjList.clear();
    adjList[myNodeId] = NDescr();

    // zero string as default name.
    nodeName[0] = '\0';
    
    // start regular cleanup of removed stations.
    gea::AbsTime t = gea::AbsTime::now();
    adjList[myNodeId].validity = t 
	+ gea::Duration(double(5 * TOPO_INTERVAL) * 0.001);
    cleanup_nodes(&cleanup_blocker, t, this);
    
    addTopoCmd();
}



RTopology::~RTopology() {
    
} 

RTopology::AdjList::iterator 
RTopology::getNodeEntry(const NodeId& id, gea::AbsTime t ) {

    std::pair<AdjList::iterator, bool> ret = 
	adjList.insert(AdjList::value_type( id, NDescr() ));
    //    assert(ret.second);
    
    AdjList::iterator itr = ret.first;
    
    assert(itr != adjList.end());
    if (ret.second) { // new entry
	if (!newXmlTopologyDelta.empty()) {
	    ostringstream ns;
	    ns << "<topodiff>\n";
	    ns << "  <add_node id=\"" << id << "\" name=\"" << getNameOfNode(id) <<"\" />\n";  
	    ns << "</topodiff>";
	    string s = ns.str();
	    newXmlTopologyDelta(s);
	} 
    }
    return itr;    
}

std::string RTopology::getNameOfNode(const NodeId& id) const {
    
    string ret;
    bool ambiguous = false;

    AdjList::const_iterator itr = adjList.find(id);
    const char *n = itr->second.nodeName;
    
    if ( itr == adjList.end() || strlen(n) == 0 ) { // not found 
	ret = "";
	ambiguous = true;
    } else {
	ret = n;
    }
    

    for (itr = adjList.begin(); itr != adjList.end(); ++itr) {
	if ( 0 == strcmp(itr->second.nodeName, n ) && itr->first != id ) {
	    ambiguous = true;
	    break;
	}
    }
    

    if (ambiguous) {
	ostringstream os;
	os << ret << "[" << id << "]";
	ret = os.str();
    }
    
    return ret;
    
}

void RTopology::feed(const TopoPacket& p, gea::AbsTime t) {
    
    if (locked) return; // when locked status is set, ignore new packets

    NodeId src = p.getSrc();

    int n = p.getNumLinks();

    string deltaQ;
    string deltaN;
    

    AdjList::iterator itr ;
    
    itr = getNodeEntry(src, t); // find entry, create a new one if not found and return it.
    gea::AbsTime newValidity = t + p.getValidity(); 
    itr->second.validity = newValidity;   // set validity
    
    NList  old_nList;   
    NList& nList = itr->second.nList;
    nList.swap(old_nList);
    nList.reserve(n);
    assert(nList.size() == 0);
    
    QList  old_qList;
    QList& qList = itr->second.qList;
    qList.swap(old_qList);
    qList.reserve(n);
    assert(qList.size() == 0);
    
    NodeId node;

    // parse Topopacket and build up neighbour list and link quality list.
    char *addr =  &(p.packet.buffer[TopoPacket::OffsetLinks]);
    for (int i = 0; i < n ; ++i) {

	node.fromArray(addr);	

	addr += NodeId::size; // skip the node entry
	unsigned char q = (unsigned char)(*addr);
	
	assert(q);
	
	addr += 1;            // skip the quality entry
	nList.push_back(node);
	qList.push_back(q);
	
	// set validity of referenced nodes to the validity of this topo packet.
	// This way a node does not become invalid as long as it is referenced by others.
	AdjList::iterator nItr = getNodeEntry(node, t);
	gea::AbsTime& itsValidity = nItr->second.validity;
	if (itsValidity < newValidity)
	    itsValidity = newValidity;
	
    }
 
    bool nameHasChanged = false;
    
    
    if (p.packet.buffer + p.packet.size > addr) {
	char buf[33];
	
	unsigned  namelen = (unsigned)(unsigned char)(*addr++);
	if (namelen > 32) namelen = 32;
	
	memcpy(buf, addr, namelen);
	buf[namelen]='\0'; // add termination, evtl. its the second one. 
	nameHasChanged = strcmp( itr->second.nodeName, buf) != 0;
	strcpy(itr->second.nodeName, buf); 
    }
    
    if (!newXmlTopologyDelta.empty() && nameHasChanged) {
	ostringstream ns;
	ns << "  <modify_node id=\"" 
	   << src << "\" name=\"" 
	   << getNameOfNode(src) << "\" />\n";
	deltaN += ns.str();
    }
    
    if (!newXmlTopologyDelta.empty()) {
	
	for (unsigned i = 0; i < nList.size(); ++i) {
	    
	    
	    bool found = false;
	    for (unsigned j = 0; j < old_nList.size(); ++j) {
		if (nList[i] == old_nList[j]) {
		    found = true;
		    if (qList[i] != old_qList[j]) {
			ostringstream qs;
			qs << "  <modify_edge from=\"" << src << "\" to=\"" 
			   << nList[i] << "\"> <quality value=\""
			   << (qList[i] * 100 / 0xff ) << "%\" /></modify_edge>\n";
			deltaQ += qs.str();
		    }
		    break;
		}
	    }
	    
	    if (!found) {
		// this is a new edge...
		ostringstream es;
		es << "  <add_edge from=\"" << src << "\" to=\"" 
		   << nList[i] << "\"> <quality value=\""
		   << (qList[i] * 100 / 0xff) << "%\" /></add_edge>\n";
		deltaN += es.str();
	    }
	}

	for (unsigned i = 0; i < old_nList.size(); ++i) {
	    bool found = false;
	    for (unsigned j = 0; j < nList.size(); ++j) 
		if (nList[j] == old_nList[i]) {
		    found = true;	
		    break;
		}
	    if (!found) {
		ostringstream ns;
		ns << "  <remove_edge from=\"" << src 
		   << "\" to=\"" << old_nList[i] << "\"/>\n";
		deltaN += ns.str();
	    }
	}
	
	if (deltaN.size() + deltaQ.size() != 0) {
	    ostringstream deltaS;
	    deltaS << "<topodiff>\n" << deltaN << deltaQ << "</topodiff>\n";
	    string xmlDelta = deltaS.str();
	    newXmlTopologyDelta(xmlDelta);
	}
    }
    
    if ( (old_qList != qList) || (old_nList != nList))
	dirty = true;
    
    assert(nList.size() == qList.size());
   
    if ( dirty && !newDotTopology.empty() ) {
	std::string s = getDotString();
	newDotTopology( s );
    }
    
}


#include <iostream>

using namespace std;


struct printAdjX {
    
    ostream& os;
    
    printAdjX(ostream& os) : os(os) {}    
    void operator()(const RTopology::AdjList::value_type &v) const {
	for (size_t i = 0; i < v.second.nList.size(); ++i ) {
	    
	    unsigned  q = (unsigned)(v.second.qList[i]) * 255U / 0xff; 
	    char colorstring[13];
	    sprintf(colorstring, "%2x00%2x", q, 255 - q);
	    os << " n" << v.first <<" -> n" << (v.second.nList[i]) << "[color=\"#" 
	       << colorstring << "\"]; ";
	}
    }
};


void RTopology::print() {
    
    ostream&  os = GEA.dbg();
    os << "digraph topo { ";
    for_each(adjList.begin(), adjList.end(), printAdjX(os));
    
    os <<"}" << endl;
}

std::string RTopology::getDotString() const {
    
    std::stringstream os;
    os << "digraph topo { ";
    os << " n" << myNodeId << " [shape=box]; ";
    
    for_each(adjList.begin(), adjList.end(), printAdjX(os));
    os <<"}" << endl;
    
    return os.str();
    
}

std::string RTopology::getAdjString() const {
    
    std::stringstream os;
    
    for (AdjList::const_iterator i = adjList.begin();
	 i != adjList.end(); ++i) {
	
	os << i->first << ": ";
	for (size_t j = 0; j < i->second.nList.size(); ++j ) {
	    os << (i->second.nList[j]) << " ";
	}
	os << std::endl;
    }
    //    for_each(adjList.begin(), adjList.end(), printAdjX(os));
    //    os <<"}" << endl;
    
    return os.str();
}

std::string RTopology::getXmlString() const {
    
    std::ostringstream os;
    
    os << "<topology>\n";
    for (AdjList::const_iterator i = adjList.begin();
	 i != adjList.end(); ++i) {
	
	os << "<node id=\"" << i->first << "\" name=\"" 
	   << getNameOfNode(i->first)
	   << "\" />\n";
    }
    
    for (AdjList::const_iterator i = adjList.begin();
	 i != adjList.end(); ++i) {
	
	
	for (size_t j = 0; j < i->second.nList.size(); ++j ) {
	    os << "<edge from=\"" << i->first << "\" to=\""
	       << (i->second.nList[j]) << "\" ><quality value=\""
	       << (i->second.qList[j] * 100 / 0xff) << "%\" /></edge>\n";
	}
    }
    //    for_each(adjList.begin(), adjList.end(), printAdjX(os));
    //    os <<"}" << endl;
    
    os << "</topology>\n";
    return os.str();
}





static bool dist_comp(RTopology::AdjList::iterator a, RTopology::AdjList::iterator b) {
    return a->second.distance > b->second.distance;
}


void RTopology::calcNextHop(const NodeId& id) {
    
    NDescr& d = adjList[id];
    
    if (d.distance == numeric_limits<unsigned>::max()) {
	d.nextHop = myNodeId;
	return;
    }
    
    if (d.nextHop != myNodeId)  // already calculated;
	return;
    
    if (d.prevHop == myNodeId) { // end of recursion 
	d.nextHop = id;

    } else {
	if (d.distance < adjList[d.prevHop].distance) {

	    GEA.dbg() << id << "(" 
		      << d.distance
		      << ") prev=" << d.prevHop << "(" 
		      << adjList[d.prevHop].distance << ")"
		      << std::endl;
	    assert(d.distance >= adjList[d.prevHop].distance);
	}
	calcNextHop(d.prevHop); // recursion !!!
	d.nextHop = adjList[d.prevHop].nextHop;
    }
}

void RTopology::calcRoutes() {
    
    todo.reserve(adjList.size());
    todo.clear();
    


    assert(adjList.find(myNodeId) != adjList.end());
      
    for(AdjList::iterator itr = adjList.begin();
	itr != adjList.end(); ++itr) {
	
	itr->second.nextHop = myNodeId; // mark as "no next hop defined, yet"
	
	todo.push_back(itr);
	if (itr->first == myNodeId) { 
	    itr->second.distance = 0;
	    itr->second.prevHop = myNodeId;
	} else 
	    itr->second.distance = numeric_limits<unsigned>::max();
    }
 
  
    
    while(!todo.empty()) {
	
	sort(todo.begin(), todo.end(), dist_comp);
	AdjList::iterator nearest = todo.back();
	assert(nearest->second.distance <= todo.front()->second.distance);
	todo.pop_back();
	
	const NDescr& ndescr = nearest->second;
	size_t numN = ndescr.nList.size();
	assert( numN == ndescr.qList.size() );
	
	for (size_t i = 0; i < numN; ++i) {
	    if (ndescr.distance == numeric_limits<unsigned>::max() ) {
		// all other nodes are unreachable, so we can stop here.
		break;
	    }
	    AdjList::iterator itr2 = adjList.find(ndescr.nList[i]);
	    if (itr2 == adjList.end() ) continue;
	    
	    NDescr& neigh = itr2->second;
	    //	    assert(ndescr.qList[i] <= 64U); // no negative edges allowed
	    unsigned newDist = ndescr.distance + (unsigned)(ndescr.qList[i]);
	    if (newDist < neigh.distance) {

		// debugging stuff 
	// 	if ( find(todo.begin(), todo.end(), itr2) == todo.end() ) {
// 		    GEA.dbg() << " new path to " <<  itr2->first 
// 			      << " pref=(" << itr2->second.prevHop << ") "
// 			      << " shorter than previous found" << std::endl;
// 		    assert (!"ouch!" );
// 		}
		// end of debugging stuff 
		neigh.distance = newDist;
		neigh.prevHop = nearest->first;
	    }
	}


    }

    for(AdjList::iterator itr = adjList.begin(); itr != adjList.end(); ++itr) {
	calcNextHop(itr->first);
    }
    
    dirty = false;
}



void RTopology::getNextHop(const NodeId& dest, NodeId& nextHop, bool& found) {
    
    
    if (dirty) { 
	
	removeOldNodes(gea::AbsTime::now());
	
	calcRoutes();
    }
    
    
    AdjList::const_iterator itr = adjList.find(dest);
    found = (itr != adjList.end());
    if (found)
	nextHop = itr->second.nextHop;
    
    if (nextHop == myNodeId)
	found = false;

 
}

bool RTopology::hasLink(const NodeId& from, const NodeId& to) const { 
    
    AdjList::const_iterator itr = adjList.find(from);
    if (itr == adjList.end()) return false;
    
    const NList& nList = itr->second.nList;
    NList::const_iterator itr2 
	= find(nList.begin(), nList.end(), to);
    return itr2 != nList.end();
}


void RTopology::cleanup_nodes(gea::Handle *h, gea::AbsTime t, void *data) {
    
    RTopology *self = static_cast<RTopology *>(data);
    GEA.waitFor(h, self->removeOldNodes(t), cleanup_nodes, data);
}

void RTopology::createRemoveMessages(const NodeId& node, const NDescr& nDescr ) {
    
    if (!newXmlTopologyDelta.empty()) {
	ostringstream ns;
	
	ns << "<topodiff>\n";

	const NList &nlist = nDescr.nList;
	NList::const_iterator i;
	for (i = nlist.begin(); i != nlist.end(); ++i) {
	    ns << "  <remove_edge from=\"" << node << "\" to=\"" 
	       << (*i) << "\" ></remove_edge>\n";
	}
	ns << "  <remove_node id=\"" << node << "\" ></remove_node>\n";  
	ns << "</topodiff>\n";
	string s = ns.str();
	
	newXmlTopologyDelta(s);
    } 
    
}


struct doInvalidate {
    gea::AbsTime t;
    const NodeId& myNodeId;
    
    doInvalidate(gea::AbsTime t, const NodeId myId) : t(t), myNodeId(myId) {}
    void operator ()( RTopology::AdjList::value_type& v)  const  {
	if (v.first != myNodeId) {
	    v.second.validity = t;
	}
    }
};


void RTopology::reset() {
    this->dirty = true;
    std::for_each(adjList.begin(), adjList.end(), doInvalidate(gea::AbsTime::now(), myNodeId) );
    removeOldNodes(gea::AbsTime::now());
} 

gea::AbsTime RTopology::removeOldNodes(gea::AbsTime t) {

    ostringstream edge_s;
    ostringstream node_s;
    bool oneRemoved = false;

    
    AdjList::iterator itr = adjList.begin();

    assert(itr != adjList.end());
    gea::AbsTime nextTimeout = t + gea::Duration(314.1592);
    
    if (locked) return nextTimeout;
    
    while( itr != adjList.end() ) {

	gea::AbsTime validity = itr->second.validity;
	
	
	if (validity <= t) {

	    if (itr->first == myNodeId) {
		GEA.dbg() << "failure! removing my one node from topology" 
			  << " age is " << ( t - validity )
			  << endl;
	    }
	  
	    GEA.dbg() << "removing node "
		      << itr->first << endl;
	    
	   

	    AdjList::iterator itr2 = itr;
	    itr2++;
	    
	    if (!newXmlTopologyDelta.empty()) {
		const NList& nList = itr->second.nList;
		for (NList::const_iterator iN = nList.begin();
		     iN != nList.end();
		     ++iN) {
		    edge_s << "  <remove_edge from=\"" << itr->first
			   << "\" to=\"" << *iN << "\" />\n";
		}
		node_s << "  <remove_node id=\"" << itr->first << "\" />\n";
	    }
	    // createRemoveMessages(itr->first, itr->second);
	    oneRemoved = true;
	    adjList.erase(itr);
	    itr = itr2;
	    
	} else {
	    itr++;
	    
	    if ( validity < nextTimeout)
		nextTimeout = validity;
	    
	}
    }
    if (!newXmlTopologyDelta.empty() && oneRemoved) {

	string s = "<topodiff>\n";
	node_s <<  "</topodiff>\n";
	
	s += edge_s.str() + node_s.str();
	newXmlTopologyDelta(s);
    }
    //   GEA.dbg() << " next cleanup in "
    // 	      << (nextTimeout - t) << endl;
  
    return nextTimeout;
}



static const char *topo_cmd_usage = 
    "topo <cmd> \n"
    " with <cmd> \n"
    "    dump        print the current topology\n"
    "    get_locked  print the lock status of the topology\n"
    "    lock        lock the topology\n"
    "    unlock      unlock the topology\n"
    ;

static int topo_command_fn(ShellClient &sc, void *data, int argc, char **argv) {
    RTopology *topology = static_cast<RTopology *>(data);
    
    if ( (argc >= 2) && !strcmp(argv[1], "dump") ) {
	*sc.sockout << topology->getAdjString() << endl;
	
	return 0;
    } else   if ( (argc >= 2) && !strcmp(argv[1], "get_locked") ) {
	*sc.sockout << (topology->getLocked() ? "true" : "false") << endl;
	return 0;
    } else if ( (argc >= 2) && !strcmp(argv[1], "lock") ) {
	topology->setLocked(true);
	*sc.sockout << "topology is now locked" << endl;
	return 0;
    } else if ( (argc >= 2) && !strcmp(argv[1], "unlock") ) {
	topology->setLocked(false);
	*sc.sockout << "topology is now unlocked" << endl;
	return 0;
    }
    
    *sc.sockout << topo_cmd_usage <<endl;
    
    return 0;
}


int RTopology::addTopoCmd() {
    
    ObjRepository& rep = ObjRepository::instance();
    
    Shell *shell = static_cast<Shell *>(rep.getObj("shell"));
    
    if (!shell) 
	return -1;
    
    shell->add_command("topo", topo_command_fn, this, "functions for the routing topology", topo_cmd_usage);
    
    return 0;
}


/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
