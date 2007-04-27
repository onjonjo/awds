#ifndef _ROUTING_H__
#define _ROUTING_H__

#include <gea/Time.h>
#include <awds/NodeId.h>
#include <string>

namespace awds {
    class BasePacket;
    class CryptoUnit;


class Routing {

public:

    CryptoUnit *cryptoUnit;

    typedef void (*recv_callback)( BasePacket *p, gea::AbsTime t, void *data);
    
    const NodeId       myNodeId;

    enum Metrics {
	TransmitDurationMetrics = 0,
	EtxMetrics              = 1,
	PacketLossMetrics       = 2,
	HopCountMetrics         = 3
    };
    
    enum Metrics metrics;

    
    Routing(const NodeId& id) : 
	cryptoUnit(0),
	myNodeId(id),
	metrics(PacketLossMetrics)
    {}
        
    virtual ~Routing() {};
    
    /* name resolution functions */

    /** convert a node id to a unique name
     *  \param   id which node is looked up
     *  \returns a unique string for identifying the node.
     */
    virtual std::string getNameOfNode(const NodeId& id) const = 0;
    
    /** convert a node name to an internal node id
     *  \param   id a reference to a node id, where the return value is stored.
     *  \param   name the name to look for.
     *  \returns 0 on success, -1 otherwise.
     */
    virtual bool getNodeByName(NodeId& id, const char *name) const = 0;


    /* ---------- for interaction with topology ---------- */

    typedef int (*NodeFunctor)( void *data, const NodeId& id);
    /** function to iterate over the list of nodes
     */
    virtual int foreachNode(NodeFunctor, void *data) const = 0;

    typedef int (*EdgeFunctor)( void *data, const NodeId& from, const NodeId& to);
    /** function to iterate over the list of nodes
     */
    virtual int foreachEdge(EdgeFunctor, void *data) const = 0;
    
    
    struct NodesObserver {
	struct NodesObserver *next;
	virtual void nodesChanged() = 0;
	virtual void nodeAdded(const awds::NodeId& id) = 0 ;
	virtual void nodeRemoved(const awds::NodeId& id) = 0;
    };
    
    virtual void addNodeObserver(struct NodesObserver *observer) = 0;
    
    struct LinksObserver {
	struct LinksObserver *next;
	virtual void linksChanged() = 0;
	virtual void linkAdded(const awds::NodeId& from, const awds::NodeId& to) = 0;
	virtual void linkRemoved(const awds::NodeId& from, const awds::NodeId& to) = 0;
    };

    virtual void addLinkObserver(struct LinksObserver *observer) = 0;
    
    virtual BasePacket *newFloodPacket(int floodType) = 0; 
    virtual BasePacket *newUnicastPacket(int type) = 0;
    
    virtual bool isReachable(const NodeId& id) const = 0;
    
    virtual void sendBroadcast(BasePacket *p, gea::AbsTime t) = 0;
    virtual void sendUnicast(BasePacket *p, gea::AbsTime t) = 0;
    virtual void sendUnicastVia(BasePacket *p, gea::AbsTime t,NodeId nextHop) = 0;

    virtual void registerUnicastProtocol(int num, recv_callback cb, void* data) = 0; 
    virtual void registerBroadcastProtocol(int num, recv_callback cb, void* data) = 0;
    
    virtual size_t getMTU() = 0;
    
};
}

#endif //ROUTING_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
