
#include <sstream>
#include <cassert>
#include <limits>
#include <iostream>
#include <algorithm>
#include <ext/algorithm>

#include <gea/API.h>

#include <gea/ObjRepository.h>
#include <awds/Topology.h>
#include <awds/TopoPacket.h>

#include <awds/ext/Shell.h>

#include <awds/Metric.h>

using namespace std;
using namespace gea;

bool awds::operator==(RTopology::LinkQuality const &lq,NodeId const &n) {
    return lq.neighbor == n;
}
bool awds::operator==(RTopology::LinkQuality const &lq,RTopology::LinkQuality const &lq2) {
    return lq.neighbor == lq2.neighbor;
}
bool awds::operator<(RTopology::LinkQuality const &lq,RTopology::LinkQuality const &lq2) {
    return lq.neighbor < lq2.neighbor;
}

bool awds::operator==(std::pair<NodeId,RTopology::NDescr> const &a,NodeId const &b) {
    return a.first == b;
}

bool awds::control_topology(RTopology::AdjList &adjList) {
    return true;
    RTopology::AdjList::iterator it(adjList.begin());
    while (it != adjList.end()) {
	RTopology::LinkList::iterator n(it->second.linklist.begin());
	while (n != it->second.linklist.end()) {
	    RTopology::AdjList::iterator it2(adjList.find(n->neighbor));
	    if (adjList.end() != it2) {
		RTopology::LinkList::iterator n2(find(it2->second.linklist.begin(),it2->second.linklist.end(),it->first));
		//	    assert(n2 != it2->second.linklist.end());
		if (n2 != it2->second.linklist.end()) {
		    assert(n2->counterpart == &*n);
		    assert(n->counterpart == &*n2);
		}
	    }
	    ++n;
	}
	++it;
    }
    return true;
}

bool 
RTopology::LinkQuality::get_qualities(RTopology::link_quality_t &f,
				      RTopology::link_quality_t &b) const {
    if (counterpart) {
	f = quality;
	b = counterpart->quality;
	return true;
    }
    return false;
}


bool RTopology::AdjList::find(NodeId const &from,NodeId const &to) const {
    const_iterator ait(const_find(from));
    if (ait != end()) {
	LinkList const &list(ait->second.linklist);
	LinkList::const_iterator it = ::find(list.begin(),list.end(),to);
	if (it != list.end()) {
	    return true;
	}
    }
    return false;
}

bool RTopology::AdjList::find(NodeId const &from,NodeId const &to,RTopology::LinkList::iterator &it) {
    iterator ait(find(from));
    if (ait != end()) {
	LinkList &list(ait->second.linklist);
	it = ::find(list.begin(),list.end(),to);
	if (it != list.end()) {
	    return true;
	}
    }
    return false;
}


RTopology::LinkList::iterator 
RTopology::LinkList::insert(RTopology::LinkQuality const &lq,
			    RTopology::AdjList &adjList,NodeId const &me) {
    //    assert(control_topology(adjList));
    
    LinkList::iterator it(find(begin(),end(),lq));  // lookup
    if (it != end()) {
	it->quality = lq.quality; // found then update
    } else {
	it = begin(); // not found insert	
	while ((it != end()) && (*it < lq)) { // at the right position
	    ++it;
	}
	it = std::vector<LinkQuality>::insert(it,lq);

	assert(is_sorted(begin(),end()));

	AdjList::iterator ait(adjList.find(lq.neighbor)); // find the first part of the counterpart
	if (ait != adjList.end()) {
	    LinkList &list2nd(ait->second.linklist);
	    LinkList::iterator it2(find(list2nd.begin(),list2nd.end(),me)); // find the second part
	    if (it2 != list2nd.end()) {
		// found, then set the two pointers vice versa
		it->set_counterpart(&*it2);
	    }
	}
    }

    assert(control_topology(adjList));

    return it;
}

RTopology::RTopology(NodeId id,Routing *routing) : 
    verbose(false),
    metric(new Metric(routing)),
    adjList(),
    dirty(true),
    locked(false),
    myNodeId(id)
{
    nodeObservers = 0;
    linkObserver = 0;
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
    //    std::cout << __PRETTY_FUNCTION__ << std::endl;
    delete metric;
} 

RTopology::AdjList::iterator 
RTopology::getNodeEntry(const NodeId& id, gea::AbsTime t ) {

    std::pair<AdjList::iterator, bool> ret = 
	adjList.insert(AdjList::value_type( id, NDescr() ));
    //    assert(ret.second);
    
    AdjList::iterator itr = ret.first;
    
    assert(itr != adjList.end());
    if (ret.second) { // new entry
	sendNodesChanged();
	sendNodeAdded(id);
	if (!newXmlTopologyDelta.empty()) {
	    ostringstream ns;
	    ns << "<topodiff timestamp=\"" << (GEA.lastEventTime - gea::AbsTime::t0())<<"\" />\n" 
	       << "  <add_node id=\"" << id << "\" name=\"" << getNameOfNode(id) <<"\" />\n"
	       << "</topodiff>\n";
	    string s = ns.str();
	    newXmlTopologyDelta(s);
	} 
    }
    return itr;    
}

std::string RTopology::getNameOfNode(const NodeId& id) const {
    
    string ret;
    bool ambiguous = false;

    AdjList::const_iterator itr = adjList.const_find(id);
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

    LinkList &linklist = itr->second.linklist;
    LinkList old_linklist;
    if (!newXmlTopologyDelta.empty() || linkObserver) {
	old_linklist = linklist;
    }

    //    linklist.swap(old_linklist);
    //    assert(linklist.size() == 0);
    
    /*    NList  old_nList;   
	  NList& nList = itr->second.nList;
	  nList.swap(old_nList);
	  nList.reserve(n);
	  assert(nList.size() == 0);
	  
	  QList  old_qList;
	  QList& qList = itr->second.qList;
	  qList.swap(old_qList);
	  qList.reserve(n);
	  assert(qList.size() == 0);*/
    
    NodeId node;

    // parse Topopacket and build up neighbour list and link quality list.

    if (src == myNodeId) {
	metric->begin_update();
    }

    char *addr =  &(p.packet.buffer[TopoPacket::OffsetLinks]);
    for (int i = 0; i < n ; ++i) {
	
	node.fromArray(addr);	

	addr += NodeId::size; // skip the node entry
	link_quality_t q = *(reinterpret_cast<link_quality_t*> (addr));
	    //	unsigned char q = (unsigned char)(*addr);
	assert(q);
	
	addr += sizeof(link_quality_t);

	AdjList::iterator nItr = getNodeEntry(node, t);
	gea::AbsTime& itsValidity = nItr->second.validity;
	if (itsValidity < newValidity)
	    itsValidity = newValidity;


	if (src == myNodeId) {
	    metric->addNode(node);
	}
	LinkList::iterator it(linklist.insert(LinkQuality(node,q),adjList,src));
	metric->calculate(it);

	//linklist.push_back(LinkQuality(node,q,0,0));
	//	nList.push_back(node);
	//	qList.push_back(q);
	
	// set validity of referenced nodes to the validity of this topo packet.
	// This way a node does not become invalid as long as it is referenced by others.
	
    }

    if (src == myNodeId) {
	metric->end_update();
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
    
    if (!newXmlTopologyDelta.empty() || linkObserver) {
	
	//	for (unsigned i = 0; i < nList.size(); ++i) {
	for (unsigned i = 0; i < linklist.size(); ++i) {
	    	    
	    bool found = false;
	    for (unsigned j = 0; j < old_linklist.size(); ++j) {
		if (linklist[i] == old_linklist[j]) {
		    found = true;
		    if (linklist[i].metric_weight != old_linklist[j].metric_weight) {
			ostringstream qs;
			qs << "  <modify_edge from=\"" << src << "\" to=\"" 
			   << linklist[i].neighbor << "\"> <quality value=\""			    
			   << linklist[i].get_percentage() << "%\" /></modify_edge>\n";
			deltaQ += qs.str();
		    }
		    break;
		}
	    }
	    
	    if (!found) {
		// this is a new edge...
		ostringstream es;
		es << "  <add_edge from=\"" << src << "\" to=\"" 
		   << linklist[i].neighbor << "\"> <quality value=\""
		   << linklist[i].get_percentage() << "%\" /></add_edge>\n";
		deltaN += es.str();
		this->sendLinkAdded(src, linklist[i].neighbor);
	    }
	}

	for (unsigned i = 0; i < old_linklist.size(); ++i) {
	    bool found = false;
	    for (unsigned j = 0; j < linklist.size(); ++j) 
		if (linklist[j] == old_linklist[i]) {
		    found = true;	
		    break;
		}
	    if (!found) {
		ostringstream ns;
		ns << "  <remove_edge from=\"" << src 
		   << "\" to=\"" << old_linklist[i].neighbor << "\"/>\n";
		deltaN += ns.str();
		this->sendLinkRemoved(src, old_linklist[i].neighbor);
	    }
	}
	
	if (deltaN.size() + deltaQ.size() != 0) {
	    ostringstream deltaS;
	    deltaS << "<topodiff timestamp=\"" << (GEA.lastEventTime - gea::AbsTime::t0()) << "\" >\n" 
		   << deltaN << deltaQ << "</topodiff>\n";
	    string xmlDelta = deltaS.str();
	    newXmlTopologyDelta(xmlDelta);
	    this->sendLinksChanged();
	}
    }
    
    if (old_linklist != linklist)
	dirty = true;
    
    //    assert(nList.size() == qList.size());
   
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
	for (size_t i = 0; i < v.second.linklist.size(); ++i ) {
	    
	    unsigned  q = (unsigned)(v.second.linklist[i].metric_weight) * 255U / 0xff; 
	    char colorstring[13];
	    sprintf(colorstring, "%02x00%02x", q, max_quality - q);
	    os << " n" << v.first <<" -> n" << (v.second.linklist[i].neighbor) << "[color=\"#" 
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
	for (size_t j = 0; j < i->second.linklist.size(); ++j ) {
	    os << (i->second.linklist[j].neighbor) << " ";
	}
	os << std::endl;
    }
    //    for_each(adjList.begin(), adjList.end(), printAdjX(os));
    //    os <<"}" << endl;
    
    return os.str();
}

std::string RTopology::getXmlString() const {
    
    std::ostringstream os;
    
    os << "<topology timestamp=\"" << (GEA.lastEventTime - gea::AbsTime::t0()) << "\" >\n";
    for (AdjList::const_iterator i = adjList.begin();
	 i != adjList.end(); ++i) {
	
	os << "<node id=\"" << i->first << "\" name=\"" 
	   << getNameOfNode(i->first)
	   << "\" />\n";
    }
    
    for (AdjList::const_iterator i = adjList.begin();
	 i != adjList.end(); ++i) {
	
	
	for (size_t j = 0; j < i->second.linklist.size(); ++j ) {
	    os << "<edge from=\"" << i->first << "\" to=\""
	       << (i->second.linklist[j].neighbor) << "\" ><quality value=\""
	       << i->second.linklist[j].get_percentage() << "%\" /></edge>\n";
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
	size_t numN = ndescr.linklist.size();
	//	assert( numN == ndescr.qList.size() );
	
	for (size_t i = 0; i < numN; ++i) {
	    if (ndescr.distance == numeric_limits<unsigned>::max() ) {
		// all other nodes are unreachable, so we can stop here.
		break;
	    }
	    AdjList::iterator itr2 = adjList.find(ndescr.linklist[i].neighbor);
	    if (itr2 == adjList.end() ) continue;
	    
	    NDescr& neigh = itr2->second;
	    //	    assert(ndescr.qList[i] <= 64U); // no negative edges allowed
	    unsigned newDist = ndescr.distance + (unsigned)(ndescr.linklist[i].metric_weight);
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
    return adjList.find(from,to);

    /*    AdjList::const_iterator itr = adjList.const_find(from);
    if (itr == adjList.end()) return false;
    
    const LinkList& linklist = itr->second.linklist;
    LinkList::const_iterator itr2 
	= find(linklist.begin(),linklist.end(), to);
	return itr2 != linklist.end();*/
}


void RTopology::cleanup_nodes(gea::Handle *h, gea::AbsTime t, void *data) {
    
    RTopology *self = static_cast<RTopology *>(data);
    GEA.waitFor(h, self->removeOldNodes(t), cleanup_nodes, data);
}

void RTopology::createRemoveMessages(const NodeId& node, const NDescr& nDescr ) {
    
    if (!newXmlTopologyDelta.empty()) {
	ostringstream ns;
	
	ns << "<topodiff timestamp=\"" << (GEA.lastEventTime - gea::AbsTime::t0()) << "\" >\n";

	const LinkList &linklist = nDescr.linklist;
	LinkList::const_iterator i;
	for (i = linklist.begin(); i != linklist.end(); ++i) {
	    ns << "  <remove_edge from=\"" << node << "\" to=\"" 
	       << (i->neighbor) << "\" ></remove_edge>\n";
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
    gea::AbsTime t = gea::AbsTime::now();
    std::for_each(adjList.begin(), adjList.end(), doInvalidate(t, myNodeId) );
    removeOldNodes(t);
} 

gea::AbsTime RTopology::removeOldNodes(gea::AbsTime t) {

    ostringstream edge_s;
    ostringstream node_s;
    bool oneRemoved = false;

    edge_s << "<topodiff timestamp=\"" << (GEA.lastEventTime - gea::AbsTime::t0()) << "\" >\n";
    
    AdjList::iterator itr = adjList.begin();

    assert(itr != adjList.end());
    gea::AbsTime nextTimeout = t + gea::Duration(314.1592);
    
    if (locked) return nextTimeout;
    
    while( itr != adjList.end() ) {
	NodeId currentNodeId = itr->first;
	gea::AbsTime validity = itr->second.validity;
	
	
	if (validity <= t) {
	    
	    if (currentNodeId == myNodeId) {
		GEA.dbg() << "failure! removing my one node from topology" 
			  << " age is " << ( t - validity )
			  << endl;
	    }
	  
	    if (verbose) {
		GEA.dbg() << "removing node "
			  << currentNodeId << endl;
	    }
	    
	   

	    AdjList::iterator itr2 = itr;
	    itr2++;
	    
	    if (!newXmlTopologyDelta.empty() || linkObserver ) {
		const LinkList& linklist = itr->second.linklist;
		for (LinkList::const_iterator iN = linklist.begin();
		     iN != linklist.end();
		     ++iN) {
		    edge_s << "  <remove_edge from=\"" << currentNodeId
			   << "\" to=\"" << iN->neighbor << "\" />\n";
		    sendLinkRemoved(currentNodeId, iN->neighbor);
		}
		node_s << "  <remove_node id=\"" << currentNodeId << "\" />\n";
	    }
	    // createRemoveMessages(itr->first, itr->second);
	    oneRemoved = true;

	    LinkList::iterator ll(itr->second.linklist.begin());
	    while (ll != itr->second.linklist.end()) {
		ll->remove_reference();
		++ll;
	    }

	    adjList.erase(itr);
	    itr = itr2;
	    
	    sendNodeRemoved(currentNodeId);
	
	} else {
	    itr++;
	    
	    if ( validity < nextTimeout)
		nextTimeout = validity;
	    
	}
    }
    if (!newXmlTopologyDelta.empty() && oneRemoved) {

	string s = "";
	node_s <<  "</topodiff>\n";
	
	s += edge_s.str() + node_s.str();
	newXmlTopologyDelta(s);
	sendNodesChanged();
	sendLinksChanged();
    }
    //   GEA.dbg() << " next cleanup in "
    // 	      << (nextTimeout - t) << endl;
  
    return nextTimeout;
}


bool awds::RTopology::getNodeByName(awds::NodeId& id, const char *name) const {
    //GEA.dbg() << "looking in " << l.size() << endl;
    // try to find topology entry.
    for (AdjList::const_iterator itr = adjList.begin();
	 itr != adjList.end();
	 ++itr) {
	
	if (!strncmp( name, itr->second.nodeName, 32)) {
	    // found it;
	    id = itr->first;
	    return true;
	}
    }
    //    GEA.dbg() << "not found " << endl;
    
    // try to convert 12 hex-digit syntax
    if (strlen(name) == 12) {
	char mac[6];
	const char *p= name;
	bool parse_success = true;
	
	for (int i = 0; i < 6; ++i) {
	    unsigned v;
	    int ret = sscanf(p, "%2X", &v);
	    if (ret != 1) {
		parse_success = false;
		break;
	    }
	    mac[i] = (char)(unsigned char)v;
	}
	if (parse_success) {
	    id.fromArray(mac);
	    return true;
	}
    }
	
    // try nummerical conversation.
    char * endptr;
    unsigned long int v = strtoul(name, &endptr, 0);
    if (*endptr || endptr == name)  // parse error!
	return false;
    
    id = awds::NodeId(v);
    return true;
    
}


std::string RTopology::getNameList() const {
    std::ostringstream os;
    for (AdjList::const_iterator itr = adjList.begin();
	 itr != adjList.end();
	 ++itr) 
	{
	    os << itr->first << "\t'" << itr->second.nodeName << "'\n";
	}
    return os.str();
}




void RTopology::sendNodesChanged() const {
    struct awds::Routing::NodesObserver *nodeObserver = this->nodeObservers;
    //    GEA.dbg() << "signaling topochange to " << (void*)nodeObserver << endl;
    while(nodeObserver) {
	//GEA.dbg() << "signaling topochange to " << (void*)nodeObserver << endl;
	nodeObserver->nodesChanged();
	nodeObserver = nodeObserver->next;
    }
}

void RTopology::sendNodeAdded(const NodeId& id) const {
    struct awds::Routing::NodesObserver *nodeObserver = this->nodeObservers;
    while(nodeObserver) {
	nodeObserver->nodeAdded(id);
	nodeObserver = nodeObserver->next;
    }
}

void RTopology::sendNodeRemoved(const NodeId& id) const {
    struct awds::Routing::NodesObserver *nodeObserver = this->nodeObservers;
    while(nodeObserver) {
	nodeObserver->nodeRemoved(id);
	nodeObserver = nodeObserver->next;
    }
}

void RTopology::sendLinksChanged() const {
    struct awds::Routing::LinksObserver *lobserver = this->linkObserver;
    while(lobserver) {
	lobserver->linksChanged();
	lobserver = lobserver->next;
    }
}

void RTopology::sendLinkAdded(const NodeId& from, const NodeId& to) const {
    struct awds::Routing::LinksObserver *lobserver = this->linkObserver;
    while(lobserver) {
	lobserver->linkAdded(from, to);
	lobserver = lobserver->next;
    }
}

void RTopology::sendLinkRemoved(const NodeId& from, const NodeId& to) const {
    struct awds::Routing::LinksObserver *lobserver = this->linkObserver;
    while(lobserver) {
	lobserver->linkRemoved(from, to);
	lobserver = lobserver->next;
    }
}


static const char *topo_cmd_usage = "topo <cmd> \n"
 " with <cmd> \n"
 "    dump        print the current topology\n"
 "    get_locked  print the lock status of the topology\n"
 "    lock        lock the topology\n"
 "    unlock      unlock the topology\n"
 "    names       list the known names of all stations\n"
 ;

static int topo_command_fn(ShellClient &sc, void *data, int argc, char **argv) {
    RTopology *topology = static_cast<RTopology *>(data);
    
    if ( (argc == 2) && !strcmp(argv[1], "dump") ) {
	*sc.sockout << topology->getAdjString() << endl;
	
	return 0;
    } else   if ( (argc == 2) && !strcmp(argv[1], "get_locked") ) {
	*sc.sockout << (topology->getLocked() ? "true" : "false") << endl;
	return 0;
    } else if ( (argc == 2) && !strcmp(argv[1], "lock") ) {
	topology->setLocked(true);
	*sc.sockout << "topology is now locked" << endl;
	return 0;
    } else if ( (argc == 2) && !strcmp(argv[1], "unlock") ) {
	topology->setLocked(false);
	*sc.sockout << "topology is now unlocked" << endl;
	return 0;
    } else if ( (argc == 2) && !strcmp(argv[1], "names") ) {
	*sc.sockout << topology->getNameList();
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
