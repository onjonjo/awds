#ifndef _FLOWPACKET_H__
#define _FLOWPACKET_H__


namespace awds {
    
    class FlowPacket : public SrcPacket {
    public:
	
	static const size_t OffsetFlowDest     = SrcPacketEnd;
	static const size_t OffsetFlowId       = OffsetFlowDest + NodeId::size;
	static const size_t OffsetFlowType     = OffsetFlowId   + 4;
	static const size_t FlowPacketEnd      = OffsetFlowType + 4;
    
	FlowPacket(BasePacket &p) : SrcPacket(p) {}
	
	uint32_t getFlowId() const {
	    return fromArray<uint32_t>(packet.buffer + OffsetFlowId);
	}
	
	void setFlowId(uint32_t flowid) {
	    toArray<uint32_t>(flowid, packet.buffer + OffsetFlowId);
	}
	
	NodeId getFlowDest() const {
	    NodeId ret;
	    ret.fromArray(packet.buffer + OffsetFlowDest);
	}
	
	void setFlowDest(const NodeId& id) const {
	    id.toArray(packet.buffer + OffsetFlowDest);
	}
	
	uint32_t getFlowType() const {
	    return fromArray<uint32_t>(packet.buffer + OffsetFlowType);
	}
	
	uint32_t setFlowType(uint32_t type) {
	    toArray<uint32_t>(type, packet.buffer + OffsetFlowType);
	}

    };

}

#endif //FLOWPACKET_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
