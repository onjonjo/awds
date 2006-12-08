#ifndef _TAPIFACE2_H__
#define _TAPIFACE2_H__

#include <awds/tapiface.h>

class TapInterface2 : public TapInterface {

public:
    TapInterface2(Routing *routing) :
	TapInterface(routing) 
    {}
    
    virtual ~TapInterface2() {}

    virtual bool setIfaceHwAddress(const NodeId& id);
    
/** detemine the routing node ID for the given MAC address.
     *  \param mac pointer to 6 chars with the MAC address
     *  \praram id reference to a NodeID variable. If a valid destination id is found, 
     *             it is stored here.
     *  \returns true, if id was found, false otherwise (means broadcast).
     */
    virtual bool   getNodeForMacAddress(const char* mac, NodeId& id, gea::AbsTime t);

    /** store node ID and src MAC in the internal table.
     *  this is not used in the basic tap interface
     */
    virtual void   storeSrcAndMac(const NodeId &id, const char *bufO, gea::AbsTime t);
    
protected:
    
    struct MacEntry {
	NodeId id;
	gea::AbsTime validity;
    };
    
    typedef std::map<NodeId, struct MacEntry> MacTable;
    MacTable macTable;

};

#endif //TAPIFACE2_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
