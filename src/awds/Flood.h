#ifndef _FLOOD_H__
#define _FLOOD_H__

#include <awds/SrcPacket.h>
#include <awds/FloodTypes.h>




class Flood : public SrcPacket {
    

public:    
    static const size_t OffsetLastHop   = SrcPacketEnd;
    static const size_t OffsetTTL       = OffsetLastHop + NodeId::size;
    static const size_t OffsetFloodType = OffsetTTL + 1;
    static const size_t FloodHeaderEnd  = OffsetFloodType + 1;

    Flood(BasePacket& p) : SrcPacket(p) {
	packet.setType(PacketTypeFlood);
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
    
    NodeId getLastHop() const {
	NodeId ret;
	ret.fromArray(&packet.buffer[OffsetLastHop]);
	return ret;
    }
    
    void setLastHop(const NodeId& id) {
	id.toArray(&packet.buffer[OffsetLastHop]);
    }
    
    int getFloodType() {
	return (int)packet.buffer[OffsetFloodType];
	
    }
    
    void setFloodType(int ft) {
	packet.buffer[OffsetFloodType] = (char)(ft % 0x0100); 
    }
    
    
    
    

};






#endif //FLOOD_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
