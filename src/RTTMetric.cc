#include <awds/RTTMetric.h>

#include <iostream>

#include <string>
#include <sstream>

using namespace std;
using namespace awds;
using namespace gea;

awds::RTTMetric::RTTMetric(awds::Routing *r):UCastMetric(r),history(0),interval(0.5),debug(false),alpha(1),packetSize(800) {  
  srand(time(0));
}

awds::RTTMetric::~RTTMetric() {
}

void
awds::RTTMetric::start() {
  GEA.waitFor(&blocker,
	      gea::AbsTime::now()+interval,
	      &ExtMetric::wait,
	      (void*)this);
}

std::string 
awds::RTTMetric::get_history() {
  std::string result;
  if (history) {
    std::ostringstream r;
    r << "RTTMetric" << std::endl;
    r << "Alpha: " << alpha << std::endl;
    r << "Interval: " << interval << std::endl;
    r << std::endl;
    History::iterator it(history->begin());
    while (it != history->end()) {
      r << "DestNode: " << (unsigned long) it->first << std::endl;
      r << "Received: " << std::endl;
      double sum(0);
      gea::Duration mi(1),ma(0);
      for (unsigned int i(0);i<it->second.size();i+=2) {
	r << it->second[i] << " ";
	sum += it->second[i];
	mi = std::min(mi,it->second[i]);
	ma = std::max(ma,it->second[i]);
      }
      r << std::endl;
      r << "mean: " << sum/(it->second.size()/2) << std::endl;
      r << "min: " << mi << std::endl;
      r << "max: " << ma << std::endl;
      r << std::endl;
      r << "Smoothed: " << std::endl;
      sum = 0;
      mi = 1;
      ma = 0;
      for (unsigned int i(1);i<it->second.size();i+=2) {
	r << it->second[i] << " ";
	sum += it->second[i];
	mi = std::min(mi,it->second[i]);
	ma = std::max(ma,it->second[i]);
      }
      r << std::endl;
      r << "mean: " << sum/(it->second.size()/2) << std::endl;
      r << "min: " << mi << std::endl;
      r << "max: " << ma << std::endl;
      r << std::endl;
      r << std::endl;
      ++it;
    }
    r << std::endl;
    result = r.str();
  }
  return result;
}

std::string awds::RTTMetric::get_values() {
  std::string result;
  if (history) {
    std::ostringstream r;
    r << "RTTMetric for: " << (unsigned long) routing->myNodeId << std::endl;
    r << "Alpha: " << alpha << std::endl;
    r << "Interval: " << interval << std::endl;
    r << std::endl;
    RTTData::iterator it(rttData.begin());
    while (it != rttData.end()) {
      r << "DestNode: " << (unsigned long) it->first << std::endl;
      r << "SRTT: " << it->second.time << std::endl;
      r << std::endl;
      ++it;
    }
    r << std::endl;
    result = r.str();
  }
  return result;
}


awds::RTopology::link_quality_t 
awds::RTTMetric::my_get_quality(NodeDescr &ndescr) {
  RTopology::link_quality_t ret(max_quality);
  RTTData::iterator it(rttData.find(ndescr.id));
  if (it == rttData.end()) {
    // first time, register to measure
    rttData[ndescr.id] = s_rtt_data();
  } else {
    if (it->second.time) {
      ret = (RTopology::link_quality_t)std::max((double) 1,(double)max_quality*it->second.time*10);
    }
  }
  return ret;
}

void
awds::RTTMetric::addNode(NodeId &nodeId) {
  RTTData::iterator it(rttData.find(nodeId));
  if (it == rttData.end()) {
    // first time, register to measure
    rttData[nodeId] = s_rtt_data();
  } else {
    it->second.in_use = true;
  }
}

void 
awds::RTTMetric::begin_update() {
  RTTData::iterator it(rttData.begin());
  while (it != rttData.end()) {
    it->second.in_use = false;
    ++it;
  }
}

void 
awds::RTTMetric::end_update() {
  RTTData::iterator it(rttData.begin());
  while (it != rttData.end()) {
    RTTData::iterator backup(it);
    if (!it->second.in_use) {
      ++backup;
      //std::cout << "removing: " << (unsigned int) it->first << std::endl;
      rttData.erase(it);
      it = backup;
    } else {
      ++it;
    }
  }
}

unsigned long 
awds::RTTMetric::my_calculate(awds::RTopology::link_quality_t forward,
			      awds::RTopology::link_quality_t backward) {
  return forward+backward;
}

void awds::RTTMetric::go_measure() {
  RTTData::iterator oldest (rttData.begin());
  RTTData::iterator it(rttData.begin());
  while (it != rttData.end()) {
    //    std::cout << routing->myNodeId << "  "  << oldest->first << std::endl;
    if (oldest->second.lastsend > it->second.lastsend) {
      oldest = it;
    }
    ++it;
  }
  it = oldest;
  if (it != rttData.end()) {
    gea::AbsTime t(gea::AbsTime::now());
    BasePacket *p = routing->newUnicastPacket(PACKET_TYPE_UC_METRIC);
    UCMetricPacket mp(*p,awds::UCMetricPacket::req,2,1,t);
    if (debug) {
      GEA.dbg() << "rtt: send request to " << (unsigned long) it->first << std::endl;
    }
    sendvia(p,t,it->first,packetSize);
    it->second.lastsend = t;
  }
  GEA.waitFor(&blocker,
	      gea::AbsTime::now()+interval+(rand()%10/10.0),
	      &ExtMetric::wait,
	      (void*)this);
}

void 
awds::RTTMetric::on_recv(BasePacket *p,
			 gea::AbsTime t) {
  UCMetricPacket mp(*p);
  if (debug) {
    GEA.dbg() << "rtt: receive metricpacket from: " << (unsigned int) mp.getSrc() << std::endl;
  }
  if (mp.getType() == awds::UCMetricPacket::req) {    
    if (debug) {
      GEA.dbg() << "rtt: request" << std::endl;
    }
    BasePacket *p_response(routing->newUnicastPacket(PACKET_TYPE_UC_METRIC));
    UCMetricPacket response(*p_response,awds::UCMetricPacket::resp,2,1,mp.getTime1());
    if (debug) {
      GEA.dbg() << "rtt: send response to " << (unsigned long) mp.getSrc() << std::endl;
    }
    sendvia(p_response,gea::AbsTime::now(),mp.getSrc());
  } else {
    gea::AbsTime t1(mp.getTime1());
    gea::Duration d(t-t1);
    RTTData::iterator it(rttData.find(mp.getSrc()));
    if (it != rttData.end()) {
      //      std::cout << "alpha: " << alpha << "  " << d << "  " << it->second.time << std::endl;
      it->second.time = alpha*d+(1-alpha)*(it->second.time);
      //std::cout << it->second.time << std::endl;
      it->second.lastrecv = t;
      if (debug) {
	GEA.dbg() << "rtt: response measured rtt: " << d << std::endl;
      }
      if (history) {
	(*history)[mp.getSrc()].push_back(d);
	(*history)[mp.getSrc()].push_back(it->second.time);
      }
    }
  }
}

void 
awds::RTTMetric::on_wait(gea::Handle *h,
			 gea::AbsTime t) {
  go_measure();
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
  RTTMetric *rtt(new RTTMetric(routing));
  routing->topology->metric = rtt;

  for (int i(1);i<argc;++i) {
    std::string w(argv[i]);
    std::string p;
    if (i < argc-1) {
      p = argv[i+1];
    }
    if (w == "--debug") {
      rtt->debug = true;
    }
    if (w == "--history") {
      rtt->go_history();
    }
    if (w == "--alpha") {
      std::stringstream ss(p);
      ss >> rtt->alpha;
    }
    if (w == "--interval") {
      std::stringstream ss(p);
      double h;
      ss >> h;
      rtt->interval = h;
    }
    if (w == "--packetsize") {
      std::stringstream ss(p);
      ss >> rtt->packetSize;
    }
  }
  rtt->start();

  REP_INSERT_OBJ(awds::Metric*,metric,routing->topology->metric);
  REP_INSERT_OBJ(awds::Metric*,rttmetric,routing->topology->metric);
  GEA.dbg() << "RTT-Metric installed" << std::endl;
  return 0;
}
