#ifndef _SRCPACKET_H__
#define _SRCPACKET_H__


#include <sys/types.h>

#include <cstddef>

#include <awds/BasePacket.h>
#include <awds/NodeId.h>
#include <awds/toArray.h>

namespace awds {

    class SrcPacket {

    public:
	static const size_t OffsetSrc      = 1;
	static const size_t OffsetSeq      = OffsetSrc + NodeId::size;
	static const size_t SrcPacketEnd   = OffsetSeq + 2;
    
	BasePacket& packet; 
    
	SrcPacket(BasePacket& packet) :packet(packet) {

	}

	void setSrc(const NodeId& id) {
	    id.toArray(&packet.buffer[OffsetSrc]);	

	}
    
	void getSrc(NodeId& id) const {
	    id.fromArray(&packet.buffer[OffsetSrc]);
	}
	    
	NodeId getSrc() const {
	    NodeId ret;
	    getSrc(ret);
	    return ret;
	}
    
	void setSeq(u_int16_t num) {
	    toArray<u_int16_t>(num,&packet.buffer[OffsetSeq]);
	}
    
	u_int16_t getSeq() const {
	    return fromArray<u_int16_t>(&packet.buffer[OffsetSeq]);
	}
    
	void setControlBit(int bit, bool v = true) {
	    assert(bit >= 2 && bit <= 7);
	    packet.buffer[0] = (packet.buffer[0] & ~('\1' << bit)) | (!!v << bit);
	}
	
	bool getControlBit(int bit) const {
	    assert(bit >= 2 && bit <= 7);
	    return !!(packet.buffer[0] & ('\1' << bit));
	}
	
	void setTraceFlag(bool v = true) {
	    setControlBit(7,v);
	}
	
	bool getTraceFlag() const {
	    return getControlBit(7);
	}

	
    };
}

#endif //SRCPACKET_H__

/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
