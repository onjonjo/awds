#ifndef _FLOWROUTING_H__
#define _FLOWROUTING_H__

#include <awds/basic.h>
#include <awds/routing.h>
#include <awds/NodeId.h>
#include <awds/BasePacket.h>

namespace awds {
    
    /** \brief Inteface class for routing with flow tables.
     */
    class FlowRouting : public Routing {
    public:	
	typedef uint32_t FlowId;
	
	typedef void (*FlowReceiver)(BasePacket *p, void *data);
    

	FlowRouting(basic *basic) : 
	    Routing(basic->MyId) 
	{}

	virtual int  addForwardingRule(FlowId flowid, NodeId nextHop) = 0;
	virtual int  delForwardingRule(FlowId flowid) = 0;
	//	virtual bool getNextHop(FlowId flowid, NodeId& retval) = 0;

	virtual int addFlowReceiver(FlowId flowid, FlowReceiver, void *data) = 0;
	virtual int delFlowReceiver(FlowId) = 0;

	virtual BasePacket *newFlowPacket(FlowId flowid) = 0; 
	virtual int sendFlowPacket(BasePacket *p) = 0;
	
	virtual ~FlowRouting() {}
    };
}


#endif //FLOWROUTING_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
