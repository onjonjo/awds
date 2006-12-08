#ifndef _TOPOPACKET_H__
#define _TOPOPACKET_H__

#include <sys/types.h>

#include <gea/Time.h>

#include <awds/AwdsRouting.h>
#include <awds/Flood.h> 



/** class for accessing fields of a Topology packet.
 */

class TopoPacket : public Flood {
    
public:
    // definitions  of packet layout.
    static const size_t OffsetValidity          = FloodHeaderEnd; 
    static const size_t OffsetNumLinks		= OffsetValidity + sizeof(u_int32_t);
    static const size_t OffsetLinks		= OffsetNumLinks + 1;

    
    
    TopoPacket(BasePacket&p) : Flood(p) {
	setFloodType(FloodTypeTopo);
    }

    /** set the list of neighbors in a packet.
     *  \param interf pointer to the interf object that contains the neihbors.
     *  \param t the current time, used for calculating timeouts. 
     */ 
    void setNeigh(AwdsRouting *interf, gea::AbsTime t);
    
    /** get the number of links in a TopoPacket.
     *  \return number of links.
     */
    int getNumLinks() const {
	return (int)(unsigned)(unsigned char)packet.buffer[OffsetNumLinks];
    }
    
    /** return validity as Duration. 
     */
    gea::Duration getValidity() const {
	u_int32_t v = fromArray<u_int32_t>(&packet.buffer[OffsetValidity]);
	return gea::Duration(double(v) * 0.001);
    }
    

    /** set validity in milliseconds 
     */
    void setValidity(long d) {
	u_int32_t v = d;
	toArray<u_int32_t>(v,&packet.buffer[OffsetValidity]);
    }
    
    void print();
 
};




#endif //TOPOPACKET_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
