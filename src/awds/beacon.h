#ifndef _BEACON_H__
#define _BEACON_H__


#include <cassert>

#include <gea/Time.h>

#include <awds/SrcPacket.h>
#include <awds/NodeId.h>
#include <awds/BasePacket.h>


/**
 * class for accessing fields of a beacon packet.
 */

class Beacon : public SrcPacket {
    
public:

    
    static const size_t OffsetPeriod   = SrcPacketEnd;
    static const size_t OffsetNumNoMpr = OffsetPeriod   + 2;
    static const size_t OffsetNumMpr   = OffsetNumNoMpr + 1;
    static const size_t OffsetLNeigh   = OffsetNumMpr   + 1;
    
    Beacon(BasePacket& p) : SrcPacket(p) {
	packet.setType(PacketTypeBeacon);
    }

    gea::Duration getPeriod() const;
    void setPeriod(const gea::Duration& d);
    

    int getNumNoMpr() {
	return (int)packet.buffer[OffsetNumNoMpr];
    }
    
    void setNumNoMpr(int n) {
	packet.buffer[OffsetNumNoMpr] = (char)n;
	
    }
    
    int getNumMpr() {
	return (int)packet.buffer[OffsetNumMpr];
    }
    
    void setNumMpr(int n) {
	packet.buffer[OffsetNumMpr] = (char)n;
    }

    void setNeigh(class AwdsRouting *awdsRouting, gea::AbsTime t);
    
    void add2Hop(class AwdsRouting *awdsRouting);
    void remove2Hop(class AwdsRouting *awdsRouting);
    
    bool hasNoMpr(const NodeId& id);
    
    bool hasMpr(const NodeId& id);
    bool hasNeigh(const NodeId& id);

    bool tryRemoveFromMpr(AwdsRouting *awdsRouting);

private:
    
    /** helper function for hasMpr() and hasNoMpr() 
     */ 
    bool hasNeighBinsearch(int a, int b, const NodeId& id);

};


#endif //BEACON_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
