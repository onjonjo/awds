#ifndef _BASEPACKET_H__
#define _BASEPACKET_H__

#include <cassert>

#include <gea/Handle.h>
#include <awds/NodeId.h>
#include <gea/UdpHandle.h>

namespace awds {
    
    /** \brief primary packet type 
     *
     *   There are four primary packet types in AWDS.
     */ 
    enum PacketType {
	PacketTypeBeacon  = 0, /**< The packet is a beacon packet. */ 
	PacketTypeFlood   = 1, /**< The packet is used for flooding data. */
	PacketTypeUnicast = 2, /**< The packet is used for transmitting unicast data. */
	PacketTypeForward = 3  /**< The packet is used for unicast packets with a forwarding table */
    };

    /** 
     * \brief base data structure for representing packets.
     * 
     * The BasePacket class is used for holding the data of a packet.
     */

    class BasePacket {
    
    public:
    
	static const int MaxSize = 0x1000; /**< The maximum number of bytes per packet */
	char buffer[MaxSize]; /**< the buffer with the actual data */
	size_t size;          /**< the number of bytes in the packet, including all headers */
	/** the refcount is used for memory management.
	 *  \see BasePacket::ref()
	 *  \see BasePacket::unref()
	 */
	int refcount;         
    
    
	NodeId dest;

	BasePacket() : size(0), refcount(1) {
	    buffer[0] = 0;
	}

	int send(gea::Handle* h) { 
	
	    return h->write(buffer, size);
	
	}
	int receive(gea::Handle* h) { return (size = h->read(buffer, MaxSize)) ; }
    
	/** \brief increase the reference counter.
	 */
	int ref()   { return ++refcount; }
	
	/** 
	 * \brief decrease the reference counter.
	 *
	 * This method decreases the reference counter. When its value becomes zero,
	 * the packet is automatically deallocated.
	 * \see awds::BasePacket::ref()
	 */
	int unref() { 
	    int ret; 
	    assert(refcount > 0);
	    --refcount; 
	    ret = refcount;
	    if (refcount == 0) 
		delete this;
	    return ret;
	}
    
	PacketType getType() const {
	    return static_cast<PacketType>(buffer[0] & 0x03);
	}
	
	void setType(PacketType pt) {
	    buffer[0] = (buffer[0] & ~0x03) | static_cast<char>(pt);
	}

	void setDest(const NodeId& dest) {
	    this->dest = dest;
	}
	
    };
}




#endif //_BASEPACKET_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
