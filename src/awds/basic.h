#ifndef _BASIC_H__
#define _BASIC_H__

#include <awds/NodeId.h>
#include <awds/BasePacket.h>
#include <gea/Handle.h>
#include <gea/Time.h>

namespace awds {

    /** \brief Interface class that provides basic communication mechanisms.
     */
    struct basic {
	
	typedef void (*recv_callback_t)(BasePacket *p, void *self_p);
	recv_callback_t recv_callback;
	void *recv_callback_data;
	gea::Duration recvTimeout;
	
	NodeId BroadcastId;
	NodeId MyId;
	
    protected:
	gea::Handle *sendHandle;
	gea::Handle *recvHandle;
    public:
	
	basic() : recv_callback(0), recvTimeout(30,2) {}
	virtual void setSendDest(const NodeId& id) = 0;
	virtual void getRecvSrc(NodeId& id) = 0;
	virtual ~basic() = 0;

        virtual bool doSend(BasePacket *p) {
            setSendDest(p->dest);
            ssize_t ret = sendHandle->write(p->buffer, p->size);
            if (p->cb)
                p->cb(*p, p->cb_data, ret);
            p->cb = NULL;
            return ret >= 0;

        }

        virtual bool send(BasePacket *p, bool high_prio) {
            return doSend(p);
        }
	
	// virtual int receive(BasePacket *p) { return (p->size = recvHandle->read(p->buffer, p->MaxSize)) ; }
	
	virtual void start();
	
    protected:
	static void recv_data(gea::Handle *h, gea::AbsTime t, void *data);
	
    };
    
}

#endif //BASIC_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
