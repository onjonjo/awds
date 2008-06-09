#include <awds/EtxMetric.h>

#include <gea/gea_main.h>
#include <gea/ObjRepository.h>
#include <gea/Blocker.h>
#include <gea/API.h>

#include <awds/AwdsRouting.h>
#include <awds/Topology.h>

using namespace std;
using namespace awds;
using namespace gea;


RTopology::link_quality_t awds::EtxMetric::my_get_quality(NodeDescr& ndescr) {
  RTopology::link_quality_t q = 32-ndescr.quality();
  /* q=0 means: last 32 beacons received = very good link
   * q>0 means: some beacons lost = not so good link.
   * highest value is worst value!
   */

  // scale value to use the whole range of the quality type.
  return q * (RTopology::max_quality() / 32U);
}

uint32_t awds::EtxMetric::my_calculate(RTopology::link_quality_t forward,
					    RTopology::link_quality_t backward)
{

  forward /= (RTopology::max_quality() / 32U);
  backward /= (RTopology::max_quality() / 32U);

  uint32_t etx = (1024U * scale)/( (32U-forward) * (32U-backward));
  return etx;
}

awds::EtxMetric::EtxMetric(Routing *r):
  Metric(r),
  scale(2048)
{}

awds::EtxMetric::~EtxMetric() {
}

GEA_MAIN_2(etxmetric, argc, argv)
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
