#ifndef _TRACEUCPACKET_H__
#define _TRACEUCPACKET_H__

#include <awds/UnicastPacket.h>

namespace awds {

    class TraceUcPacket : public UnicastPacket {
	
    public: 
	static const size_t OffsetTraceEnd = UnicastPacket::UnicastPacketEnd;
	static const size_t TraceUcPacketEnd = OffsetTraceEnd + 2;
	
	TraceUcPacket(BasePacket& p) : UnicastPacket(p) {
	    
	}

	void setTracePointer(size_t offset) {
	    toArray<u_int16_t>(offset, packet.buffer + OffsetTraceEnd);
	}
	
	size_t getTracePointer() const {
	    return fromArray<u_int16_t>(packet.buffer + OffsetTraceEnd);
	}
	
	bool appendNode(const NodeId& id) {
	    size_t offset = getTracePointer();

	    if (offset + NodeId::size > packet.size) // space left for this node?
		return false;
	    id.toArray(packet.buffer + offset);
	    setTracePointer(offset + NodeId::size);
	    return true;
	}
	
    };
}

#endif //TRACEUCPACKET_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
