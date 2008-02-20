#include <awds/SendQueue.h>
#include <awds/settings.h>
#include <gea/API.h>

bool awds::SendQueue::enqueuePacket(BasePacket *p, bool high_prio) {
	/* attach high priority packets to queue front,
	 * this might reverse the order if we cannot transmit
	 * for longer than a beacon interval */
	if (high_prio) {
		queue.push_front(p);
	} else {
		if (queue.size() >= QUEUE_LENGTH) {
			return false;
		}
		queue.push_back(p);
	}
	/* if we inserted the first element, the send CB must be registered */
	if (queue.size() == 1)
		registerCallback();
	return true;
}

void awds::SendQueue::xmit_cb(gea::Handle *h, gea::AbsTime t, void *data) {
	SendQueue *self = static_cast<SendQueue*>(data);
	assert(self->queue.size() > 0);
	if (h->status == gea::Handle::Ready) {
		BasePacket *p = self->queue.front();
		self->queue.pop_front();
		self->base->setSendDest(p->dest);
		if (p->send(h) < 0) {
			GEA.dbg() << "error sending from SendQueue" << std::endl;
		}
		p->unref();
	}
	if (self->queue.size() > 0)
		self->registerCallback();
}

void awds::SendQueue::registerCallback() {
	GEA.waitFor(h, gea::AbsTime::now() + gea::Duration( 1000.),
		xmit_cb, this);
}
