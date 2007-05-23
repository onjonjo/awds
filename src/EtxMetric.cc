#include <awds/EtxMetric.h>

#include <gea/ObjRepository.h>
#include <gea/Blocker.h>
#include <gea/API.h>

#include <awds/AwdsRouting.h>
#include <awds/Topology.h>

RTopology::link_quality_t awds::EtxMetric::my_get_quality(NodeDescr &ndescr) {
  RTopology::link_quality_t q(32-ndescr.quality());
  // highest value is worst value!
  return q*(max_quality/32);
}
    
unsigned long awds::EtxMetric::my_calculate(RTopology::link_quality_t forward,RTopology::link_quality_t backward) {
  forward /= (max_quality/32);
  backward /= (max_quality/32);
  double f(forward),b(backward);
  double lra = f/32.0;  // loss rate
  double lrb = b/32.0;
  double etx = (1.0/((1.-lra)*(1.-lrb)))*scale;
  return etx;
}

awds::EtxMetric::EtxMetric(Routing *r):Metric(r) {
  scale = std::numeric_limits<unsigned long>::max()/1024;
}
 
awds::EtxMetric::~EtxMetric() {
}

extern "C"
#ifdef PIC
int gea_main(int argc, const char  * const * argv) 
#else
int awdsRouting_gea_main(int argc, const char  * const *argv) 
#endif

{
  ObjRepository& rep = ObjRepository::instance();
  AwdsRouting *routing = (AwdsRouting *)rep.getObj("awdsRouting");
  if (!routing) {
    GEA.dbg() << "cannot find object 'routing' in repository" << std::endl; 
    return -1;
  }
  delete routing->topology->metric;
  routing->topology->metric = new EtxMetric(routing);

  REP_INSERT_OBJ(awds::Metric*,metric,routing->topology->metric);
  REP_INSERT_OBJ(awds::Metric*,etxmetric,routing->topology->metric);
  GEA.dbg() << "ETX-Metric installed" << std::endl;
  return 0;
}