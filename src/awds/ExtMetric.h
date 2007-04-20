#ifndef D__ExtMetric
#define D__ExtMetric

#include <awds/Metric.h>

namespace awds {
  class ExtMetric : public Metric {
  public:
    ExtMetric(Routing *r);
    virtual ~ExtMetric(){}

    static void recv_packet(BasePacket *p,gea::AbsTime t,void *data);
    virtual void on_recv(BasePacket *p,gea::AbsTime t) = 0;

    static void wait(gea::Handle *h,gea::AbsTime t,void *data);
    virtual void on_wait(gea::Handle *h,gea::AbsTime t) = 0;
  };
}

#endif // D__ExtMetric
