#ifndef D__RTTMetric
#define D__RTTMetric

#include <awds/UCastMetric.h>

#include <string>

namespace awds {
    
    /** \brief A class that implements the RTT metric.
     */
    class RTTMetric : public UCastMetric {
    protected:

	virtual RTopology::link_quality_t my_get_quality(NodeDescr &ndescr);
	virtual unsigned long my_calculate(RTopology::link_quality_t forward,RTopology::link_quality_t backward);

	struct s_rtt_data {
	    gea::Duration time;
	    gea::AbsTime lastsend,lastrecv;
	    bool in_use;
	    s_rtt_data():time(1),lastsend(0),lastrecv(0),in_use(true) {}
	};
	typedef std::map<NodeId,s_rtt_data> RTTData;
	RTTData rttData;

	typedef std::map<NodeId,std::vector<gea::Duration> > History;
	History *history;

	gea::Blocker blocker;
    public:
	gea::Duration interval;

	bool debug;
	double alpha;
	unsigned int packetSize;
	RTTMetric(Routing *r);
	virtual ~RTTMetric();
	virtual void on_recv(BasePacket *p);
	virtual void on_wait(gea::Handle *h,gea::AbsTime t);
    
	void go_history() {
	    history = new History;
	}
    
	void start();

	virtual void addNode(NodeId &nodeId);
	virtual void begin_update();
	virtual void end_update();

	virtual std::string get_history();
	virtual std::string get_values();

	void go_measure();
    };
}

#endif // D__RTTMetric
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
