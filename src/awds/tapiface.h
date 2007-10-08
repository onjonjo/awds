#ifndef _TAPIFACE_H__
#define _TAPIFACE_H__


#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <unistd.h>
#include <net/if_arp.h>

#include <gea/posix/UnixFdHandle.h>
#include <gea/ObjRepository.h>
#include <gea/API.h>

#include <awds/NodeId.h>
#include <awds/routing.h>
#include <awds/UnicastPacket.h>
#include <awds/Flood.h>


#define ADDR_TRNSLTR 0x04
#define PROTO_NR     0x62

namespace awds {
class TapInterface {

public:
    int fd;
    char devname[IFNAMSIZ+1];
    //    NodeId devMac;
    
    gea::UnixFdHandle *tapHandle;
    Routing *routing;
    
    TapInterface(Routing *routing);
    
    virtual ~TapInterface() {}
    
    virtual void init(const char*dev);
    
    virtual bool setIfaceHwAddress(const NodeId& id);
    
    bool setIfaceMTU(int mtu);
    bool createDevice(const char *dev);
    
    static void tap_recv(gea::Handle *h, gea::AbsTime t, void *data);
    static void recv_unicast  ( BasePacket *p, void *data);
    static void recv_broadcast( BasePacket *p, void *data);
    
    /** detemine the routing node ID for the given MAC address.
     *  \param mac pointer to 6 chars with the MAC address
     *  \praram id reference to a NodeID variable. If a valid destination id is found, 
     *             it is stored here.
     *  \returns true, if id was found, false otherwise (means broadcast).
     */
    virtual bool   getNodeForMacAddress(const char* mac, NodeId& id, gea::AbsTime t);

    /** store node ID and src MAC in the internal table.
     *  this is not used in the basic tap awdsRoutingace
     */
    virtual void   storeSrcAndMac(const NodeId &id, const char *bufO, gea::AbsTime t);
    
    
};
}


#endif //TAPIFACE_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
