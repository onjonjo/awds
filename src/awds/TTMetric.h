#ifndef D__TTMetric
#define D__TTMetric

#include <awds/UCastMetric.h>
#include <map>

namespace awds {
  class gea2mad;
  class TTMetric : public UCastMetric {
  public:
    struct NodeData {
      int tt;
      bool active;
      gea::AbsTime lastsend;
      
      NodeData():tt( RTopology::max_quality()),active(true),lastsend(0) {
      }
    };
    typedef std::map<NodeId,NodeData> TTData;
    TTData ttData;
    gea::Blocker blocker;
    gea::Duration interval;

    virtual void on_recv(BasePacket *p) {}
    virtual void on_wait(gea::Handle *h,gea::AbsTime t);

    bool debug;
    unsigned int packetSize;

    TTMetric(Routing *r);
    virtual ~TTMetric();
    void start();
    gea2mad *g2m;
    virtual int update();
    virtual RTopology::link_quality_t my_get_quality(NodeDescr &ndescr);
    virtual unsigned long my_calculate(RTopology::link_quality_t forward,RTopology::link_quality_t backward);
    virtual std::string get_values();

    virtual void addNode(NodeId &nodeId);
    virtual void begin_update();
    virtual void end_update();
  };
}

#endif // D__TTMetric
