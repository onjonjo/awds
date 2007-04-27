#ifndef _INTERF_H__
#define _INTERF_H__

#include <map>
#include <cstring>

#include <awds/routing.h>
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
#include <awds/RateMonitor.h>




namespace awds {

    class AwdsRouting : public Routing { 
    
    public:

	basic * base;
    
	static const int UdpPort = 4921;
	static const int period = BEACON_INTERVAL; /* beacon period in milliseconds */
	int topoPeriod; /* topo period in milliseconds */
    
	gea::Handle *udpSend;
	gea::Handle *udpRecv;
	gea::Blocker    blocker;
	class RTopology * topology;
	class FloodHistory *floodHistory;

	class RateMonitor *madwifiRateMonitor;


	AwdsRouting(basic *base);
	virtual ~AwdsRouting();
 
	virtual size_t getMTU();
    
   
	static void send_beacon(gea::Handle *h, gea::AbsTime t, void *data); 
	static void recv_packet(gea::Handle *h, gea::AbsTime t, void *data); 
    
	static void trigger_topo(gea::Handle *h, gea::AbsTime t, void *data); 
  

	static void repeat_flood(gea::Handle *h, gea::AbsTime t, void *data); 
	static void send_unicast(gea::Handle *h, gea::AbsTime t, void *data);

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
	

	virtual int foreachNode(NodeFunctor, void *data) const;
	virtual int foreachEdge(EdgeFunctor, void *data) const;
    

	virtual void addNodeObserver(struct awds::Routing::NodesObserver *observer);
	virtual void addLinkObserver(struct LinksObserver *observer);
  
	
    
	void recv_beacon(BasePacket *p, gea::AbsTime t); 
	void recv_flood(BasePacket *p, gea::AbsTime t); 
	void recv_unicast(BasePacket *p, gea::AbsTime t);

	virtual bool isReachable(const NodeId& id) const;
    
	virtual BasePacket *newFloodPacket(int floodType); 
	virtual BasePacket *newUnicastPacket(int type);
    
	virtual void sendBroadcast(BasePacket *p, gea::AbsTime t);
	virtual void sendUnicast(BasePacket *p, gea::AbsTime t);
	virtual void sendUnicastVia(BasePacket *p,gea::AbsTime t,NodeId nextHop);
	
        
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
	
	
	void removeOldNeigh(gea::AbsTime t);
	
	bool refreshNeigh(BasePacket *p, gea::AbsTime t);
	
	void stat2dyn();
	
	void assert_stat();
	
	void calcMpr();
	
    };
    
}

using namespace awds;

#endif //INTERF_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
