#ifndef _SRCPACKET_H__
#define _SRCPACKET_H__


#include <sys/types.h>

#include <cstddef>

#include <awds/BasePacket.h>
#include <awds/NodeId.h>
#include <awds/toArray.h>

namespace awds {
    
    /** \brief The access class for all packets that contain a source address.
     *
     *  \code
     *
     *  awds::BasePacket *p = getPacketFromSomewhere();
     *  awds::SrcPacket srcP(*p);
     *  awds::NodeId src = srcP.getSrc();
     *
     * \endcode
     */
    class SrcPacket {

    public:
	static const size_t OffsetSrc      = 1;
	static const size_t OffsetSeq      = OffsetSrc + NodeId::size;
	static const size_t SrcPacketEnd   = OffsetSeq + 2;
    
	BasePacket& packet; 
    
	/** \brief The constructor.
	 *
	 *  Creates a SrcPacket that accesses the values in \a packet
	 */
	SrcPacket(BasePacket& packet) :packet(packet) {
	}

	/** \brief Set the source address of the packet.
	 */
	void setSrc(const NodeId& id) {
	    id.toArray(&packet.buffer[OffsetSrc]);	
	}
    
	/** \brief Get the source address of the packet.
	 * 
	 *  This function is used to get the source address of the packet.
	 *  It is an optimized version of the getSrc() member funtion, because it
	 *  avoids the creation of a temporary NodeId object.
	 */
	void getSrc(NodeId& id) const {
	    id.fromArray(&packet.buffer[OffsetSrc]);
	}
	
	/** \brief Get the source address of the packet.
	 */
	NodeId getSrc() const {
	    NodeId ret;
	    getSrc(ret);
	    return ret;
	}
	
	/** \brief Set sequence number of the packet.
	 */
	void setSeq(u_int16_t num) {
	    toArray<u_int16_t>(num,&packet.buffer[OffsetSeq]);
	}
	
	/** \brief Get the sequence number of the packet.
	 */
	u_int16_t getSeq() const {
	    return fromArray<u_int16_t>(&packet.buffer[OffsetSeq]);
	}
	
	
	/** \brief Manipulate a control bit.
	 */
	void setControlBit(int bit, bool v = true) {
	    assert(bit >= 2 && bit <= 7);
	    packet.buffer[0] = (packet.buffer[0] & ~('\1' << bit)) | (!!v << bit);
	}
	
	/** \brief Get the value of a control bit. 
	 */
	bool getControlBit(int bit) const {
	    assert(bit >= 2 && bit <= 7);
	    return !!(packet.buffer[0] & ('\1' << bit));
	}
	
	/** \brief Set the trace control bit.
	 */
	void setTraceFlag(bool v = true) {
	    setControlBit(7,v);
	}
	
	/** \brief Get the trace control bit. 
	 */
	bool getTraceFlag() const {
	    return getControlBit(7);
	}

	
    }; // end of SrcPacket

} // end of namespace awds

#endif //SRCPACKET_H__

/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
