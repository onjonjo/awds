#ifndef _TOPOPACKET_H__
#define _TOPOPACKET_H__

#include <sys/types.h>

#include <gea/Time.h>

#include <awds/AwdsRouting.h>
#include <awds/Flood.h> 



namespace awds {
    
    /** \brief helper class for iterating over the entries in a topo packet.
     *  \see TopoPacket
     */
    struct TopoPacketNeighItr {
	char *ptr;
    public:

	bool operator ==(const TopoPacketNeighItr& other) const { return ptr == other.ptr; }
	bool operator !=(const TopoPacketNeighItr& other) const { return !(*this == other); }
	TopoPacketNeighItr& operator++() { ptr += awds::NodeId::size + 2; return *this; }
	NodeId operator *() const { NodeId ret; ret.fromArray(ptr); return ret; }
    };
    
    /** \brief class for accessing fields of a Topology packet.
     *
     *  \code
     *
     *  awds::BasePacket *p = getPacketFromSomewhere();
     *  awds::TopoPacket topoP(*p);
     *  int n = topoP.getNumLinks();
     *
     * \endcode
     */
    class TopoPacket : 
	public Flood 
    {
    
    public:
	// definitions  of packet layout.
	static const size_t OffsetValidity      = FloodHeaderEnd; 
	static const size_t OffsetNumLinks	= OffsetValidity + sizeof(u_int32_t);
	static const size_t OffsetLinks		= OffsetNumLinks + 1;

    
    
	TopoPacket(BasePacket&p) : Flood(p) {
	    setFloodType(FloodTypeTopo);
	}

	/** \brief set the list of neighbors in a packet.
	 *  \param awdsRouting pointer to the interf object that contains the neihbors.
	 *  \param t the current time, used for calculating timeouts. 
	 */ 
	void setNeigh(AwdsRouting *awdsRouting);
    
	/** \brief get the number of links in a TopoPacket.
	 *  \return number of links.
	 */
	int getNumLinks() const {
	    return (int)(unsigned)(unsigned char)packet.buffer[OffsetNumLinks];
	}
    
	/** \brief return validity as Duration. 
	 */
	gea::Duration getValidity() const {
	    u_int32_t v = fromArray<u_int32_t>(&packet.buffer[OffsetValidity]);
	    return gea::Duration(double(v) * 0.001);
	}
    

	/** \brief set validity in milliseconds.
	 */
	void setValidity(long d) {
	    u_int32_t v = d;
	    toArray<u_int32_t>(v,&packet.buffer[OffsetValidity]);
	}
	/** \brief dump the topo packet.
	 */
	void print();
 
    }; // end of class TopoPacket

} // end of namespace awds

#endif //TOPOPACKET_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
