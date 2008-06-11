#include <awds/basic.h>
#include <gea/API.h>


awds::basic::~basic() {

}


void awds::basic::recv_data(gea::Handle *h, gea::AbsTime t, void *data) {
    awds::basic *self = static_cast<awds::basic *>(data);
    
    if (h->status ==  gea::Handle::Ready) {
	BasePacket *p = new BasePacket();
	int ret = p->receive(self->recvHandle); 
	
	if (self->recv_callback && (ret > 0) )
	    self->recv_callback(p, self->recv_callback_data);
	p->unref();
    } else {
	// here, we could do something....
    }
  
    GEA.waitFor(h, GEA.lastEventTime + self->recvTimeout, awds::basic::recv_data, data);
}

void awds::basic::start() {
    GEA.waitFor(recvHandle,
		GEA.lastEventTime + recvTimeout,
		recv_data, (void *)this);
}
	
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
