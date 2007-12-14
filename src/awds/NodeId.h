#ifndef _NODEID_H__
#define _NODEID_H__

#include <awds/AbstractId.h>

namespace awds {

    /** \brief type for representing an address of a station
     * 
     *   An AWDS node identifier is used to store a unique identifier of a station.
     *   When using the RawBasic interface, the MAC address of the wireless interface
     *   is used. Therefore, it must be at least 6 bytes wide. 
     *
     *   When used with the UdpBasic interface, the topmost bytes will be zero.
     *   \see basic::MyId
     *   \see Routing::myNodeId
     */
    typedef AbstractID<6> NodeId; 
    
}

#endif //NODEID_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
