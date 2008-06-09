#ifndef D__metric
#define D__metric


#include <awds/NodeId.h>
#include <awds/NodeDescr.h>
#include <awds/Topology.h>

#include <limits>

#include <iostream>

namespace awds {

    class Routing;

    /**
     *  The Metric class is the base class for all metrics.
     *  It implements the famouse Hop Count Metric that returns the same weight for
     *  every link. Feel free to implement a better one by creating a subclass.
     */
    class Metric {
    protected:
	Routing *routing;

	virtual RTopology::link_quality_t my_get_quality(NodeDescr &ndescr) {
	    return 10;
	}

	virtual uint32_t my_calculate( RTopology::link_quality_t forward,
				       RTopology::link_quality_t backward) {
	    // no testing for "forward" and "backward" greater than 0 neccesary it
	    // is all done in calculate and get_qualities
	    return 1;
	}
    public:
	Metric(Routing *r):routing(r) {
	}

	virtual ~Metric() {}


	/** called to add a new neighbour to the metric table
	 */
	virtual void addNode(const NodeId& nodeId) {}

	/** called to remove a  neighbour from the metric table.
	 */
	virtual void delNode(const NodeId& nodeId) {}

	/** begin_update() is called before the incoming topopaket is parsed, so
	 *  that the metric can  reset its node data, see end_update() too
	 */
	virtual void begin_update() {}

	/** end_update() is called right after the topopaket has been parsed, so
	 *  still reseted nodes can be deleted, so begin_update() too
	 */
	virtual void end_update() {}

	/** update() is called right before a topopacket ist filled with data
	 *  (\see get_quality(NodeDescr&)) so the mertic can prepare for (for TTMetric in rl it is to parse a file)
	 */
	virtual int update() {
	    return 8;
	}

	/** this method is called by the Topopacket to get the sendable
	 *  qualityvalues of the Link to ndescr
	 */
	RTopology::link_quality_t get_quality(NodeDescr &ndescr) {

	    return std::max( std::min(my_get_quality(ndescr), RTopology::max_quality()),
			     (RTopology::link_quality_t)1);
	}

	/** on receive of a Topopacket calculate will be called on each LinkQuality to calculate the
	 *  "metric_weight" depended on the quality values by the two corresponding LinkQualities
	 */
	uint32_t calculate(RTopology::LinkList::iterator &it) {
	    RTopology::link_quality_t f,b;
	    uint32_t mw = std::numeric_limits<uint32_t>::max();

	    if (it->get_qualities(f,b))
		mw = my_calculate(f,b);

	    mw = std::min(mw, std::numeric_limits<uint32_t>::max() / 1024U);

	    it->set_metric_weight(mw);
	    return mw;
	}

    };
}

#endif // D__metric

/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
