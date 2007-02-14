#ifndef _ROUTING_H__
#define _ROUTING_H__

#include <gea/Time.h>
#include <awds/NodeId.h>


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
	HopCountMetrics          = 3
    };
    
    enum Metrics metrics;

    
    Routing(const NodeId& id) : 
	cryptoUnit(0),
	myNodeId(id),
	metrics(PacketLossMetrics)
    {}
        
    virtual ~Routing() {};
    
    virtual BasePacket *newFloodPacket(int floodType) = 0; 
    virtual BasePacket *newUnicastPacket(int type) = 0;
    
    virtual bool isReachable(const NodeId& id) const = 0;
    
    virtual void sendBroadcast(BasePacket *p, gea::AbsTime t) = 0;
    virtual void sendUnicast(BasePacket *p, gea::AbsTime t) = 0;

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
