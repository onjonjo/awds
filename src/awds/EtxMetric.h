#ifndef D__EtxMetric
#define D__EtxMetric

#include <awds/Metric.h>

namespace awds {

  class EtxMetric : public Metric {
  protected:
    virtual RTopology::link_quality_t my_get_quality(NodeDescr &ndescr);
    
    virtual unsigned long my_calculate(RTopology::link_quality_t forward,RTopology::link_quality_t backward);

    double scale;

  public:
    EtxMetric(Routing *r);
    virtual ~EtxMetric();
    
  };

}
#endif // D__EtxMetrix
