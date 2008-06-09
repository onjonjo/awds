#ifndef D__EtxMetric
#define D__EtxMetric

#include <awds/Metric.h>

namespace awds {

    /** \brief class that implements the ETX metric/
     */
    class EtxMetric : public Metric {
    protected:
	virtual RTopology::link_quality_t my_get_quality(NodeDescr &ndescr);

	virtual uint32_t my_calculate(RTopology::link_quality_t forward,RTopology::link_quality_t backward);

	const uint32_t scale;

    public:
	EtxMetric(Routing *r);
	virtual ~EtxMetric();

    };

}
#endif // D__EtxMetrix

/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
