#ifndef _BASEPACKET_H__
#define _BASEPACKET_H__

#include <gea/Handle.h>
#include <awds/NodeId.h>
#include <gea/UdpHandle.h>

enum PacketType {
    PacketTypeBeacon = 12,
    PacketTypeFlood,
    PacketTypeUnicast
};

class BasePacket {
    
public:
    
    static const int MaxSize = 0x1000;
    char buffer[MaxSize];
    size_t size;
    int refcount; 
    
    
    NodeId dest;

    BasePacket() : size(0), refcount(1) {

    }

    int send(gea::Handle* h) { 
	
	return h->write(buffer, size);
	
    }
    int receive(gea::Handle* h) { return (size = h->read(buffer, MaxSize)) ; }
    
    int ref()   { return ++refcount; }
    int unref() { 
	int ret; 
	--refcount; 
	ret = refcount;
	if (refcount == 0) 
	    delete this;
	return ret;
    }
    
    PacketType getType() {
	return static_cast<PacketType>(buffer[0]);
    }
    
    void setType(PacketType pt) {
	buffer[0] = static_cast<char>(pt);
    }

    void setDest(const NodeId& dest) {
	this->dest = dest;
    }
};





#endif //_BASEPACKET_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
