#include <gea/gea_main.h>
#include <awds/PktPair.h>
#include <iostream>

using namespace awds;
using namespace std;
using namespace gea;

PktPair::PktPair(Routing *r):
  UCastMetric(r),
  interval(0.5),
  debug(false),
  alpha(1),
  packetSize(800),
  bufferSize(0)
{}

PktPair::~PktPair() {
}

RTopology::link_quality_t PktPair::my_get_quality(NodeDescr &ndescr)
{
  RTopology::link_quality_t ret = RTopology::max_quality();
  Nodes::iterator it(nodes.find(ndescr.id));
  if (it != nodes.end()) {
    if (it->second.getTime()) {
      ret = (RTopology::link_quality_t)std::max( 1.,
						 (double)RTopology::max_quality() * it->second.time * 10.);
    }
  }
  return ret;
}

uint32_t PktPair::my_calculate(RTopology::link_quality_t forward,
			       RTopology::link_quality_t backward) {
  return forward+backward;
}

std::string awds::PktPair::get_values() {
  std::string result;
  std::ostringstream r;
  r << "PktPair for: " << (unsigned long) routing->myNodeId << std::endl;
  r << "Alpha: " << alpha << std::endl;
  r << "Interval: " << interval << std::endl;
  r << std::endl;
  Nodes::iterator it(nodes.begin());
  while (it != nodes.end()) {
    r << "DestNode: " << (unsigned long) it->first << std::endl;
    r << "SPP: " << it->second.getTime() << std::endl;
    r << std::endl;
    ++it;
  }
  r << std::endl;
  result = r.str();
  return result;
}



void
awds::PktPair::start() {
  GEA.waitFor(&blocker,
	      gea::AbsTime::now()+interval,
	      &ExtMetric::wait,
	      (void*)this);
}


void PktPair::on_recv(BasePacket *p) {
  const AbsTime t = GEA.lastEventTime;

  UCMetricPacket mp(*p);
  NodeId sender(mp.getSrc());
  if (debug) {
    GEA.dbg() << "PktPair: receive metricpacket from: " << (unsigned int) sender << std::endl;
  }
  gea::Duration help_t(t-gea::AbsTime(0));
  if (mp.getType() == awds::UCMetricPacket::first) {
    firstPackets[sender] = t;
    if (debug) {
      GEA.dbg() << "Was the first one at: " << help_t << std::endl;
    }
  } else {
    if (mp.getType() == awds::UCMetricPacket::second) {
      if (debug) {
	GEA.dbg() << "Was the second one at: " << help_t << std::endl;
      }
      FirstPackets::iterator it(firstPackets.find(sender));
      if (it != firstPackets.end()) {
	gea::Duration duration(t-it->second);
	BasePacket *p_response(routing->newUnicastPacket(PACKET_TYPE_UC_METRIC));
	UCMetricPacket response(*p_response,awds::UCMetricPacket::resp,2,1,duration);
	if (debug) {
	  GEA.dbg() << "PktPair: send response to: " << (unsigned long) sender << " calculated duration: " << duration << std::endl;
	}
	sendvia(p_response,gea::AbsTime::now(),sender);
      }
    } else {
      if (mp.getType() == awds::UCMetricPacket::resp) {
	Nodes::iterator it(nodes.find(sender));
	if (it != nodes.end()) {
	  gea::Duration d(mp.getDuration());
	  it->second.setTime(d,alpha);
	}
      }
    }
  }
}

void PktPair::on_wait(gea::Handle *h,gea::AbsTime t) {
  Nodes::iterator oldest(nodes.begin());
  Nodes::iterator it(nodes.begin());
  while (it != nodes.end()) {
    if (oldest->second.lastsend > it->second.lastsend) {
      oldest = it;
    }
    ++it;
  }
  it = oldest;
  if (it != nodes.end()) {
    gea::AbsTime t(gea::AbsTime::now());
    BasePacket *p(routing->newUnicastPacket(PACKET_TYPE_UC_METRIC));
    UCMetricPacket mp(*p,awds::UCMetricPacket::first,2,1,t);
    sendvia(p,t,it->first,packetSize);
    BasePacket *p2(routing->newUnicastPacket(PACKET_TYPE_UC_METRIC));
    UCMetricPacket mp2(*p2,awds::UCMetricPacket::second,2,1,t);
    sendvia(p2,t,it->first,packetSize);
    it->second.lastsend = t;
  }
  GEA.waitFor(&blocker,
	      gea::AbsTime::now()+interval,
	      &ExtMetric::wait,
	      (void*)this);
}

void PktPair::addNode(NodeId &nodeId) {
  Nodes::iterator it(nodes.find(nodeId));
  if (it == nodes.end()) {
    s_node_data d(bufferSize);
    nodes[nodeId] = d;
  } else {
    it->second.active = true;
  }
}

void PktPair::begin_update() {
  Nodes::iterator it(nodes.begin());
  while (it != nodes.end()) {
    it->second.active = false;
    ++it;
  }
}

void PktPair::end_update() {
  Nodes::iterator it(nodes.begin());
  while (it != nodes.end()) {
    if (!it->second.active) {
      Nodes::iterator h(it);
      ++h;
      nodes.erase(it);
      it = h;
    } else {
      ++it;
    }
  }
}

GEA_MAIN_2(pktpair, argc, argv)
{
  ObjRepository& rep = ObjRepository::instance();
  AwdsRouting *routing = (AwdsRouting *)rep.getObj("awdsRouting");
  if (!routing) {
    GEA.dbg() << "cannot find object 'routing' in repository" << std::endl;
    return -1;
  }
  delete routing->topology->metric;
  PktPair *pp(new PktPair(routing));
  routing->topology->metric = pp;

  for (int i(1);i<argc;++i) {
    std::string w(argv[i]);
    std::string p;
    if (i < argc-1) {
      p = argv[i+1];
    }
    if (w == "--debug") {
      pp->debug = true;
    }
    if (w == "--minimum") {
      std::stringstream ss(p);
      ss >> pp->bufferSize;
      std::cout << "PktPair: using minimum-value, bufferSize: " << pp->bufferSize << std::endl;
    }
    /*    if (w == "--history") {
      pp->go_history();
      }*/
    if (w == "--alpha") {
      std::stringstream ss(p);
      ss >> pp->alpha;
    }
    if (w == "--interval") {
      std::stringstream ss(p);
      double h;
      ss >> h;
      pp->interval = h;
    }
    if (w == "--packetsize") {
      std::stringstream ss(p);
      ss >> pp->packetSize;
    }
  }
  pp->start();

  REP_INSERT_OBJ(awds::Metric*,metric,routing->topology->metric);
  REP_INSERT_OBJ(awds::Metric*,pktpair,routing->topology->metric);
  GEA.dbg() << "PacketPair installed" << std::endl;
  return 0;
}
