#ifndef _INTERF_H__
#define _INTERF_H__

#include <map>
#include <cstring>

#include <awds/routing.h>
#include <awds/FlowRouting.h>
#include <awds/basic.h>

#include <gea/API.h>
#include <gea/UdpHandle.h>
#include <gea/Time.h>
#include <gea/Blocker.h>

#include <awds/AbstractId.h>
#include <awds/NodeDescr.h>

#include <awds/NodeId.h>
#include <awds/beacon.h>
#include <awds/settings.h>
// #include <awds/RateMonitor.h>

#include <awds/SendQueue.h>

#include <awds/Firewall.h>

namespace awds {

    /** \brief This class implement the routing functionality
     *  \ingroup awdsrouting_mod
     *
     *  \see awds::RTopology
     */
    class AwdsRouting : public FlowRouting {

    public:
	bool verbose; /**< enable verbose debug output */

	basic * base;

	//static const int UdpPort = 4921;
	static const int period = BEACON_INTERVAL; /**< beacon period in milliseconds */
	int topoPeriod; /**< topology propagtion period in milliseconds */

	enum periodType { /**< type for constant or adaptive periods */
		Constant,
		Adaptive
	};
	periodType topoPeriodType; /**< period type for topo packets */

	gea::Handle *udpSend; /**< for sending packets */
	gea::Handle *udpRecv; /**< for receiving packets */
	gea::Blocker    blocker;
	class RTopology * topology;  ///< for storing the topology
	class FloodHistory *floodHistory; /**< history of recent flood packets */

	class awds::Firewall *firewall; /**< used for packet filtering */

	SendQueue sendq;

	AwdsRouting(basic *base);

	/** \brief destructor */
	virtual ~AwdsRouting();

	/** get the maximum transfer unit */
	virtual size_t getMTU();

	static void send_beacon(gea::Handle *h, gea::AbsTime t, void *data);
	static void recv_packet(gea::Handle *h, gea::AbsTime t, void *data);

	static void send_periodic_topo(gea::Handle *h, gea::AbsTime t, void *data);

	/** Send a topology packet. */
	virtual void send_topo();

	/** convert a node id to a unique name
	 *  This method implements the abstact version in awds::Routing
	 *  \param   id which node is looked up
	 *  \returns a unique string for identifying the node.
	 */
	virtual std::string getNameOfNode(const awds::NodeId& id) const;

	/** convert a node name to an internal node id
	 *  This method implements the abstact version in awds::Routing
	 *  \param   id a reference to a node id, where the return value is stored.
	 *  \param   name the name to look for.
	 *  \returns 0 on success, -1 otherwise.
	 */
	virtual bool getNodeByName(awds::NodeId& id, const char *name) const;

	virtual int foreachNode(awds::Routing::NodeFunctor, void *data) const;
	virtual int foreachEdge(awds::Routing::EdgeFunctor, void *data) const;

	virtual void addNodeObserver(struct awds::Routing::NodesObserver *observer);
	virtual void addLinkObserver(struct LinksObserver *observer);

	void recv_beacon(BasePacket *p);
	void recv_flood(BasePacket *p);
	void recv_unicast(BasePacket *p);

	virtual bool isReachable(const NodeId& id) const;

	virtual BasePacket *newFloodPacket(int floodType);
	virtual BasePacket *newUnicastPacket(int type);

	virtual void sendBroadcast(BasePacket *p);
	virtual void sendUnicast(BasePacket *p);
	virtual void sendUnicastVia(BasePacket *p,NodeId nextHop);

	struct Hop2RefCount {
	    short stat;
	    short dyn;
	    Hop2RefCount() {}
	    Hop2RefCount(short stat) : stat(stat) {}
	};

	typedef std::map<NodeId, Hop2RefCount> Hop2List;
	Hop2List hop2list;

	//    typedef void (*recv_callback)( BasePacket *p, gea::AbsTime t, void *data);
	typedef std::pair<recv_callback, void*> RegisterEntry;
	typedef std::map<int, RegisterEntry >  ProtocolRegister;

	ProtocolRegister unicastRegister;
	ProtocolRegister broadcastRegister;

	virtual void registerUnicastProtocol(int num, recv_callback cb, void* data);
	virtual void registerBroadcastProtocol(int num, recv_callback cb, void* data);


	static const int MaxNeighbors = 40;// (1000 - 120) / NodeId::size;
	NodeDescr    *neighbors;// [MaxNeighbors];
	int          numNeigh;

	u_int16_t beaconSeq;
	gea::Duration beaconPeriod;
	gea::AbsTime nextBeacon;

	u_int16_t floodSeq;
	u_int16_t unicastSeq;


	int findNeigh(const NodeId& id) const {

	    /* do a binary search in the sorted array */
	    int a = 0;
	    int b = numNeigh;

	    while (a != b) {
		int m = (a+b)/2;
		if (neighbors[m].id == id) return m;
		if (neighbors[m].id < id)
		    a = m+1;
		else if (neighbors[m].id > id)
		    b = m;

	    }
	    return -a - 1;  // not found, return position to keep sorted array, negative to indicate not found

	}

	bool hasNeigh(const NodeId& id) const {
	    return findNeigh(id) >= 0;
	}

	void removeOldNeigh();

	bool refreshNeigh(BasePacket *p);

	void stat2dyn();

	void assert_stat();

	void calcMpr();

	/* -------------------- flow routing stuff -------------------- */

	struct FlowReceiverData {
	    FlowRouting::FlowReceiver receiver;
	    void *data;
	};

	typedef std::map<FlowRouting::FlowId, struct FlowReceiverData> FlowReceiverMap;
	FlowReceiverMap flowReceiverMap;

	typedef std::map<FlowRouting::FlowId, NodeId> ForwardingTable;
	ForwardingTable forwardingTable;

	virtual int  addForwardingRule(FlowRouting::FlowId flowid, NodeId nextHop);
	virtual int  delForwardingRule(FlowRouting::FlowId flowid);
	//	virtual bool getNextHop(FlowId flowid, NodeId& retval);

	virtual int addFlowReceiver(FlowRouting::FlowId flowid, FlowRouting::FlowReceiver, void *data);
	virtual int delFlowReceiver(FlowRouting::FlowId);

	virtual BasePacket *newFlowPacket(FlowRouting::FlowId flowid);
	virtual int sendFlowPacket(BasePacket *p);


    };

}

// using namespace awds;

#endif //INTERF_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
