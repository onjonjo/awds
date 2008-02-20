#ifndef _SENDQUEUE_H__
#define _SENDQUEUE_H__

#include <gea/Time.h>
#include <awds/BasePacket.h>
#include <awds/basic.h>
#include <list>

namespace awds {
	class SendQueue {
		gea::Handle *h; /**< handle to block on */
		basic *base; /**< needed for setSendDest */
		std::list<BasePacket*> queue;
	public:

		SendQueue(basic *_b) : h(_b->sendHandle), base(_b) {
		}

		bool enqueuePacket(BasePacket *p, bool high_prio);

	private:
		static void xmit_cb(gea::Handle *h, gea::AbsTime t, void *data);
		void registerCallback();
	};


}
#endif // _SENDQUEUE_H__

