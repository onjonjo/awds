#ifndef D__PktPair
#define D__PktPair

#include <awds/UCastMetric.h>

#include <string>
#include <sstream>


namespace awds {
  class PktPair : public UCastMetric {
  protected:

    virtual RTopology::link_quality_t my_get_quality(NodeDescr &ndescr);
    virtual unsigned long my_calculate(RTopology::link_quality_t forward,RTopology::link_quality_t backward);

    struct s_node_data {
      bool active;
      gea::AbsTime lastsend;
      gea::Duration time;
      int capacity;
      std::vector<gea::Duration> times;
      int position;
      s_node_data(int c=0):active(true),lastsend(0),time(0),capacity(c),position(0) {
	if (capacity) {
	  //	  std::cout << "constructor: " << position << "  " << capacity << std::endl;
	  times.resize(capacity);
	  for (int i(0);i<capacity;++i) {
	    times[i] = 1;
	  }
	}
      }
      void setTime(gea::Duration t,double alpha) {
	if (times.size()) {
	  times[position++] = t;
	  position %= capacity;
	} else {
	  time = alpha*t+(1-alpha)*(time);;
	}
      }
      gea::Duration getTime() {
	if (times.size()) {
	  time = times[0];
	  for (int i(1);i<capacity;++i) {
	    time = std::min(time,times[i]);
	  }
	}
	return time;
      }
    };
    typedef std::map<NodeId,s_node_data> Nodes;
    Nodes nodes;
    typedef std::map<NodeId,gea::AbsTime> FirstPackets;
    FirstPackets firstPackets;

    gea::Blocker blocker;
  public:
    gea::Duration interval;
    bool debug;
    double alpha;
    int packetSize;
    int bufferSize;
    PktPair(Routing *r);
    virtual ~PktPair();
    virtual void on_recv(BasePacket *p,gea::AbsTime t);
    virtual void on_wait(gea::Handle *h,gea::AbsTime t);
    virtual void addNode(NodeId &nodeId);
    virtual void begin_update();
    virtual void end_update();
    void start();
    virtual std::string get_values();
  };
}




#endif // D__PktPair
