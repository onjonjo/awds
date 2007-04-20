#ifndef D__UCastMetric
#define D__UCastMetric

#include <awds/ExtMetric.h>
#include <awds/UCMetricPacket.h>

namespace awds {
  class UCastMetric : public ExtMetric {
  public:
    UCastMetric(awds::Routing *r);
    void send(BasePacket *p,gea::AbsTime t,NodeId dest);
    void sendvia(BasePacket *p,gea::AbsTime t,NodeId dest,unsigned int size = 0);
  };

}

#endif // D__UCastMetric
