#include <awds/TTMetric.h>
#include <awds/gea2mad.h>
#include <iostream>

#include <string>
#include <sstream>

using namespace awds;
using namespace std;
using namespace gea;

awds::TTMetric::TTMetric(awds::Routing *r):UCastMetric(r),interval(1),debug(false),packetSize(800) {
}

awds::TTMetric::~TTMetric() {
}

void
awds::TTMetric::start() {
  GEA.waitFor(&blocker,
	      gea::AbsTime::now()+interval,
	      &ExtMetric::wait,
	      (void*)this);
}


awds::RTopology::link_quality_t
awds::TTMetric::my_get_quality(NodeDescr &ndescr) {
  //  std::cout << "TTraw: " << g2m->getTT(ndescr.id) << std::endl;
  RTopology::link_quality_t ret =
    std::min( (RTopology::link_quality_t)(g2m->getTT(ndescr.id)  * RTopology::max_quality()),
	      RTopology::max_quality());
  ttData[ndescr.id].tt = ret;
  return ret;
}

uint32_t
awds::TTMetric::my_calculate(awds::RTopology::link_quality_t forward,
			      awds::RTopology::link_quality_t backward) {
  return forward;
}

int
awds::TTMetric::update() {
  return 0;
}


void
awds::TTMetric::addNode(NodeId &nodeId) {
  ttData[nodeId].active = true;
}

void
awds::TTMetric::begin_update() {
  TTData::iterator it(ttData.begin());
  while (it != ttData.end()) {
    it->second.active = false;
    ++it;
  }
}

void
awds::TTMetric::end_update() {
  TTData::iterator it(ttData.begin());
  while (it != ttData.end()) {
    TTData::iterator h(it);
    ++it;
    if (!h->second.active) {
      ttData.erase(h);
    }
  }
}


std::string
awds::TTMetric::get_values() {
  std::ostringstream r;
  r << "TTMetric for: " << (unsigned long) routing->myNodeId << std::endl;
  r << std::endl;
  TTData::iterator it(ttData.begin());
  while (it != ttData.end()) {
    r << "DestNode: " << (unsigned long) it->first << std::endl;
    r << "TT: " << it->second.tt << std::endl;
    r << std::endl;
    ++it;
  }
  r << std::endl;
  return r.str();
}

void
awds::TTMetric::on_wait(gea::Handle *h,gea::AbsTime t) {
  TTData::iterator oldest (ttData.begin());
  TTData::iterator it(ttData.begin());
  while (it != ttData.end()) {
    if (oldest->second.lastsend > it->second.lastsend) {
      oldest = it;
    }
    ++it;
  }
  it = oldest;
  if (it != ttData.end()) {
    gea::AbsTime t(gea::AbsTime::now());
    BasePacket *p = routing->newUnicastPacket(PACKET_TYPE_UC_METRIC);
    UCMetricPacket mp(*p,awds::UCMetricPacket::req,2,1,t);
    if (debug) {
      GEA.dbg() << "tt: send request to " << (unsigned long) it->first << std::endl;
    }
    sendvia(p,t,it->first,packetSize);
    it->second.lastsend = t;
  }
  GEA.waitFor(&blocker,
	      gea::AbsTime::now()+interval,
	      &ExtMetric::wait,
	      (void*)this);
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
  gea2mad *g2m = (gea2mad *)rep.getObj("gea2mad");
  if (!g2m) {
    GEA.dbg() << "cannot find object 'gea2mad' in repository" << std::endl;
    return -1;
  }
  delete routing->topology->metric;
  TTMetric *tt(new TTMetric(routing));
  routing->topology->metric = tt;

  tt->g2m = g2m;

  for (int i(1);i<argc;++i) {
    std::string w(argv[i]);
    std::string p;
    if (i < argc-1) {
      p = argv[i+1];
    }
    if (w == "--debug") {
      tt->debug = true;
    }
    if (w == "--interval") {
      std::stringstream ss(p);
      double h;
      ss >> h;
      tt->interval = h;
    }
    if (w == "--packetsize") {
      std::stringstream ss(p);
      ss >> tt->packetSize;
    }
  }
  tt->start();

  REP_INSERT_OBJ(awds::Metric*,metric,routing->topology->metric);
  REP_INSERT_OBJ(awds::Metric*,ttmetric,routing->topology->metric);
  GEA.dbg() << "TT-Metric installed" << std::endl;
  return 0;
}
