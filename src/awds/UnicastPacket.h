#ifndef _UNICASTPACKET_H__
#define _UNICASTPACKET_H__


#include <awds/SrcPacket.h>

namespace awds {
class UnicastPacket : public SrcPacket {

public:
    
    static const size_t OffsetUcDest     = SrcPacketEnd;
    static const size_t OffsetNextHop    = OffsetUcDest  + NodeId::size;
    static const size_t OffsetTTL        = OffsetNextHop + NodeId::size;
    static const size_t OffsetUcType     = OffsetTTL     + 1;
    static const size_t UnicastPacketEnd = OffsetUcType  + 1;
    

    UnicastPacket(BasePacket& p) : SrcPacket(p) {
	packet.setType(PacketTypeUnicast);
    }
    
    int getTTL() { 
	return (int)(unsigned)(unsigned char)packet.buffer[OffsetTTL]; 
    }
    
    void setTTL(int ttl) {
	packet.buffer[OffsetTTL] = (char)(unsigned char)(ttl % 0x0100);
    }
    
    void decrTTL() {
	packet.buffer[OffsetTTL]--;
    }
    
    void incTTL() {
	packet.buffer[OffsetTTL]++;
    }

    NodeId getUcDest() const {
	NodeId ret;
	ret.fromArray(&packet.buffer[OffsetUcDest]);
	return ret;
    }
    
    void setUcDest(const NodeId& id) {
	id.toArray(&packet.buffer[OffsetUcDest]);
    }
    
    NodeId getNextHop() const {
	NodeId ret;
	ret.fromArray(&packet.buffer[OffsetNextHop]);
	return ret;
    }
    
    void setNextHop(const NodeId& id) {
	id.toArray(&packet.buffer[OffsetNextHop]);
    }
        
    
    int getUcPacketType() const {
	return (int)(unsigned)(unsigned char)packet.buffer[OffsetUcType]; 
    }
    
    void setUcPacketType(int type) {
	packet.buffer[OffsetUcType] = static_cast<char>(type % 0x0100);
    }
    
    

};
}

#endif //UNICASTPACKET_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
