#ifndef D__UCastMetric
#define D__UCastMetric

#include <awds/ExtMetric.h>
#include <awds/UCMetricPacket.h>

namespace awds {

    /** \brief A base class for all metrics that use probe packets.
     */
    class UCastMetric :
	public awds::ExtMetric
    {
    public:
	UCastMetric(awds::Routing *r);
	void send(BasePacket *p,gea::AbsTime t,NodeId dest);
	void sendvia(BasePacket *p,gea::AbsTime t,NodeId dest,size_t size = 0);
    };

}

#endif // D__UCastMetric
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
