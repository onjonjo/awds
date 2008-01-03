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


    /** \brief class that contains all topology information of the routing
     *  \ingroup awdsrouting_mod
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

	class LinkList;
	class AdjList;

	/** \brief class that describes a link in the topology
	 */
	class LinkQuality {
	protected:
	public:
	    LinkQuality *counterpart; // pointer to the reverse LinkQuality in another NDescr

	    NodeId neighbor; /**< the endpoint of the link */

	    link_quality_t quality; /**< unidirectional quality, received by topo packets */
	    unsigned long metric_weight; /**< calculated by metric bidirectional */

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

	/** \brief list of neigbours */
	class LinkList : public std::vector<LinkQuality> {

	  public:

	    LinkList():std::vector<LinkQuality>() {}

	};

	/** \brief Struct that contains all information about a node in the topology.
	 */
	struct NDescr {
	    LinkList linklist; /**< list of neighbours */
	    NodeId nextHop;
	    NodeId prevHop;
	    gea::AbsTime validity; /**< time of expiry */

	    unsigned long distance; /**< distance value used for dijkstra */

	    /** The index is used for  certain graph algorithms. It is intended to
	     *	be used by any external algorithm, so don't rely on its value.
	     */
	    int index;

	    char nodeName[33]; /**< name of this node as propagated via topo packets */

	    NDescr() : linklist()
	    {
		nodeName[0]='\0';
	    }

	    void update_validity(const gea::AbsTime& new_v) {
		if (this->validity < new_v)
		    this->validity = new_v;
	    }


	    virtual LinkQuality *findLinkQuality(NodeId id);
	    virtual const LinkQuality *findLinkQuality(NodeId id) const;
	    virtual ~NDescr();
	};

	/** \brief class that represents the adjacency list of the topology graph
	 */
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

	    /** \brief iterate over all nodes and assign an index */
	    inline void enumerateNodes() {
		int idx = 0;
		for (AdjList::iterator i = this->begin(); i != this->end(); ++i)
		    i->second.index = idx++;
	    }
	};

	void enumerateNodes() {
	    adjList.enumerateNodes();
	}

	AdjList adjList; /**< the adjacency list of the graph */

	bool   dirty;    /**< is the routing information outdated? */
	bool   locked;   /**< When true, now topology updates are accepted */

	const NodeId myNodeId; /**< the node identifier of this instance */


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

    static inline const NodeId& getNodeId(const RTopology::LinkQuality& lq) {
	return lq.neighbor;
    }

    
    typedef RTopology Topology; // for migrating the class name.
    
} // end of namespace awds



#endif //TOPOLOGY_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
