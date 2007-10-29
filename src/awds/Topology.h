#ifndef _TOPOLOGY_H__
#define _TOPOLOGY_H__

#include <map>
#include <vector>
#include <string>

#include <gea/Time.h>
#include <gea/Blocker.h>

#include <awds/routing.h>
#include <awds/NodeId.h>
#include <awds/settings.h>
#include <awds/Callback.h>

#include <awds/routing.h>

#include <iostream>

namespace awds {


    class Metric;

    class TopoPacket;
    class RTopology;
    
    
    /** class that contains all topology information of the routing 
     *
     */
    class RTopology { 


	gea::Blocker cleanup_blocker;

    public:
	bool verbose;
	Metric *metric;
    
	char nodeName[32];
    
	static void cleanup_nodes(gea::Handle *h, gea::AbsTime t, void *data); 
    
	typedef uint16_t link_quality_t;
	static link_quality_t max_quality() { return 0xFFFFu; }
	
	// #define max_quality 0xffff

	class LinkList;
	class AdjList;

	class LinkQuality {
	protected:
	public:	    
	    LinkQuality *counterpart; // pointer to the reverse LinkQuality in another NDescr

	    NodeId neighbor;
	    
	    //const NodeId& getNeighbor() const { return neighbor; }
	    link_quality_t quality; // unidirectional, received by topopackets
	    unsigned long metric_weight; // calculated by metric bidirectional
	    //	    friend const NodeId& getNodeId(const RTopology::LinkQuality& lq);
	    friend bool operator==(LinkQuality const &lq, NodeId const &n);
	    friend bool operator==(LinkQuality const &lq, LinkQuality const &lq2);
	    friend bool operator<(LinkQuality const &lq, LinkQuality const &lq2);
	    //	    friend bool check_topology(RTopology::AdjList &adjList);

	    LinkQuality &operator=(const LinkQuality& lq) {
		counterpart   = lq.counterpart;
		neighbor      = lq.neighbor;
		quality       = lq.quality;
		metric_weight = lq.metric_weight;

		if (counterpart) {
		    counterpart->counterpart = this;
		}
		return *this;
	    }
	
	    LinkQuality(LinkQuality const &lq) {
		*this = lq;
	    }
	    
	    LinkQuality():counterpart(0),quality(0),metric_weight(0) {}
	    
	    LinkQuality(NodeId n, link_quality_t w): 
		counterpart(0),
		neighbor(n),
		quality(w),
		metric_weight(0) {}

	    void remove_reference() {
		if (counterpart) {
		    counterpart->counterpart = 0;
		    counterpart = 0;
		}
	    }

	    void set_counterpart(LinkQuality *lq) {
		if (lq)
		    lq->counterpart = this;
		counterpart = lq;
	    }

	    bool get_qualities(link_quality_t &f, link_quality_t &b) const;

	    double get_percentage() const {
		double v(quality);
		v = 100.0*((double)max_quality() - quality)/(double)max_quality();
		return v;
	    }

	    void set_metric_weight(unsigned long mw) {
		metric_weight = mw;
		if (counterpart) {  // is this really neccessary?
		    counterpart->metric_weight = mw;
		}
	    }
	
	};

	class LinkList : public std::vector<LinkQuality> {
	
	    // protected:
	    // 	    void copy(LinkList const &ll) {  
	    // 		// the copy of a linklist is not allowed to use copycontructor of LinkQuality due 
	    // 		// to the counterpart links between two of them, see LinkQuality CopyConstructor 
	    // 		LinkList::const_iterator it(ll.begin());
	    // 		while (it != ll.end()) {
		    
	    // 		    LinkQuality lq;
	    // 		    lq.neighbor = it->neighbor;
	    // 		    lq.quality = it->quality;
	    // 		    lq.metric_weight = it->metric_weight;
	    // 		    push_back(lq);
	    // 		    ++it;
	    // 		}
	    // 	    }
	public:
	
	    LinkList():std::vector<LinkQuality>() {}
	
	    //LinkList &operator=(LinkList const &ll) {
	    // 		copy(ll);
	    // 		return *this;
	    //}
	
	    // 	    LinkList(LinkList const &ll) : 
	    // 		std::vector<LinkQuality>() 
	    // 	    {
	    // 		*this = ll;
	    // 	    }
	
	    /*  inserts or updates the LinkQuality */
	    //	    LinkList::iterator insert(LinkQuality const &lq, 
	    //		      AdjList &adjList,const NodeId &me); 
	};

	
    
	struct NDescr {
	    LinkList linklist;
	    NodeId nextHop; 
	    NodeId prevHop; 
	    gea::AbsTime validity;
	
	    unsigned long distance;
	
	    char nodeName[33];
	
	    NDescr() : linklist()
	    {
		nodeName[0]='\0';
	    }

	    void update_validity(const gea::AbsTime& new_v) {
		if (this->validity < new_v)
		    this->validity = new_v;
	    }

	    
	    LinkQuality *findLinkQuality(NodeId id);
	};

	class AdjList : public std::map<NodeId, NDescr> {
	public:
	    friend bool operator==(std::pair<NodeId,NDescr> const &a,NodeId const &b);
	    iterator find(NodeId const &nodeId) {
		return std::map<NodeId,NDescr>::find(nodeId);
	    }
	    const_iterator find(NodeId const &nodeId) const {
		return std::map<NodeId,NDescr>::find(nodeId);
	    }
	    bool find(NodeId const &from,NodeId const &to,LinkList::iterator &it);
	    bool find(NodeId const &from,NodeId const &to) const;
	};
	
	//    typedef std::map<NodeId, NDescr> AdjList;
	
	AdjList adjList;

	bool   dirty; 
	bool   locked;
	
	const NodeId myNodeId;


	Callback<std::string&> newDotTopology;
	Callback<std::string&> newXmlTopologyDelta;
    
	int addTopoCmd();
        
	RTopology(NodeId id,Routing *routing);

	virtual ~RTopology();
	
	void setLocked(bool t) { locked = t; };
	bool getLocked() const { return locked; }
	virtual void reset();
	
	bool hasNode(const NodeId &node) const;
	virtual bool hasLink(const NodeId& from, const NodeId&to) const;
	
	/** 
	 * feed a topology update in the internal topology representation 
	 */
	void feed(const TopoPacket& p);    

	virtual std::string getNameOfNode(const NodeId& id) const;
	virtual bool getNodeByName(NodeId& id, const char *name) const;
	
	std::string getNameList() const;
    
	/** 
	 * get the number of nodes in the topology
	 */
	int getNumNodes() const {
	    return adjList.size();
	}

	/** 
	 * try to find node entries in the topology and remove them.
	 */
	gea::AbsTime removeOldNodes();
    
	void createRemoveMessages(const NodeId& node, const NDescr& nDescr );

	AdjList::iterator getNodeEntry(const NodeId& id, gea::AbsTime t);
        
	struct awds::Routing::NodesObserver *nodeObservers;
	struct awds::Routing::LinksObserver *linkObserver;
    protected:
	void sendNodesChanged() const;
	void sendNodeAdded(const NodeId& id) const;
	void sendNodeRemoved(const NodeId& id) const;
	void sendLinksChanged() const ;
	void sendLinkAdded(const NodeId& from, const NodeId& to) const;
	void sendLinkRemoved(const NodeId& from, const NodeId& to) const;
	
    public:
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

    // some access functions. 
    
    static const NodeId& getNodeId(const RTopology::LinkQuality& lq) {
	return lq.neighbor;
    }
    

}



#endif //TOPOLOGY_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
