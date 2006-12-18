#ifndef _TOPOLOGY_H__
#define _TOPOLOGY_H__

#include <map>
#include <vector>
#include <string>

#include <gea/Time.h>
#include <gea/Blocker.h>

#include <awds/NodeId.h>
#include <awds/settings.h>
#include <awds/Callback.h>


class TopoPacket;

class RTopology { 

    gea::Blocker cleanup_blocker;
    
public:
    
    static void cleanup_nodes(gea::Handle *h, gea::AbsTime t, void *data); 
    
    typedef std::vector<NodeId> NList; 
    typedef std::vector<unsigned char> QList; 
    
    struct NDescr {
	NList nList;
	QList qList;
	NodeId nextHop; 
	NodeId prevHop; 
	gea::AbsTime validity;
	
	unsigned long distance;
	
	NDescr() : 
	    nList(),
	    qList()
	{}
    };
    
    typedef std::map<NodeId, NDescr> AdjList;
    
    AdjList adjList;

    bool   dirty; 
    bool   locked;

    const NodeId myNodeId;


    Callback<std::string&> newDotTopology;
    Callback<std::string&> newXmlTopologyDelta;
    
    int addTopoCmd();
        
    RTopology(NodeId id);

    virtual ~RTopology();

    void setLocked(bool t) { locked = t; };
    bool getLocked() const { return locked; }
    virtual void reset();
    
    virtual bool hasLink(const NodeId& from, const NodeId&to) const;
    void feed(const TopoPacket& p, gea::AbsTime t);    
    
    int getNumNodes() const {
	return adjList.size();
    }
    
    gea::AbsTime removeOldNodes(gea::AbsTime t);
    
    void createRemoveMessages(const NodeId& node, const NDescr& nDescr );

    AdjList::iterator getNodeEntry(const NodeId& id, gea::AbsTime t);
    

    
    // -------------------------------------------------------
    //     dykstra stuff...

    
    /** get next hop for a given destination
     *  \param dest    the destination of the route.
     *  \param nextHop the next Hop is stored in the reference.
     *  \param found   true, iff the destination is reachable.
     */
    void getNextHop(const NodeId& dest, NodeId& nextHop, bool& found);
    
    void calcNextHop(const NodeId& id);
    void calcRoutes();

    typedef std::vector<AdjList::iterator> ToDo;
    ToDo todo;

    //debug output
    void print();
    
    virtual std::string getDotString() const; 
    virtual std::string getAdjString() const;
    virtual std::string getXmlString() const;
};



#endif //TOPOLOGY_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
