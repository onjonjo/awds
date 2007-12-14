#ifndef _FLOOD_H__
#define _FLOOD_H__

#include <awds/SrcPacket.h>
#include <awds/FloodTypes.h>



namespace awds {
    
    /** \brief access class for flood packets 
     * 
     *  Use this class to access the content of a flood packet.
     *  It should be used the following way
     *  \code
     *  
     *  BasePacket *p = getPacketFromSomewhere();
     *  Flood flood(*p);
     *  flood.setTTL(1);
     *
     *  \endcode
     *  
     */
    class Flood : public SrcPacket {
    

    public:    
	static const size_t OffsetLastHop   = SrcPacketEnd;
	static const size_t OffsetTTL       = OffsetLastHop + NodeId::size;
	static const size_t OffsetFloodType = OffsetTTL + 1;
	static const size_t FloodHeaderEnd  = OffsetFloodType + 1;
	
	Flood(BasePacket& p) : SrcPacket(p) {
	    packet.setType(PacketTypeFlood);
	}
	
	/** \brief Get the Time to Live (TTL) of a flood packet.
	 *
	 *  The TTL is in fact the maximum hop count of a packet.
	 */
	int getTTL() { 
	    return (int)(unsigned)(unsigned char)packet.buffer[OffsetTTL]; 
	}
	/** \brief Set the Time to Live (TTL) of a flood packet.
	 * 
	 *  The TTL is in fact the maximum hop count of a packet.
	 */
	void setTTL(int ttl) {
	    packet.buffer[OffsetTTL] = (char)(unsigned char)(ttl % 0x0100);
	}
	
	/** \brief Decrease the TTL value of the packet by one.
	 */
	void decrTTL() {
	    packet.buffer[OffsetTTL]--;
	}
	
	/** \brief Increase the TTL value of the packet by one.
	 */    
	void incTTL() {
	    packet.buffer[OffsetTTL]++;
	}
	
	/** \brief Get the node the station came from.
	 *  
	 *  \see setLastHop(const NodeId&)
	 */
 	NodeId getLastHop() const {
	    NodeId ret;
	    ret.fromArray(&packet.buffer[OffsetLastHop]);
	    return ret;
	}
	
	/** \brief Set the node id of the LastHop field in the packet.
	 *
	 *  \see getLastHop(const NodeId&)
	 */
	void setLastHop(const NodeId& id) {
	    id.toArray(&packet.buffer[OffsetLastHop]);
	}
    
	
	int getFloodType() {
	    return (int)packet.buffer[OffsetFloodType];
	    
	}
    
	void setFloodType(int ft) {
	    packet.buffer[OffsetFloodType] = (char)(ft % 0x0100); 
	}
    
    }; // end of class Flood

} // end of namespace awds





#endif //FLOOD_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
