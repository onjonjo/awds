#ifndef _RATEMONITOR_H__
#define _RATEMONITOR_H__

#include <awds/NodeId.h>

class RateMonitor {
public:
    
    virtual void update()=0;
    virtual int  getTT(const NodeId& id) = 0;
    virtual ~RateMonitor() {}
};


#endif //RATEMONITOR_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
