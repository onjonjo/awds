#include <iostream>

#include <gea/ObjRepository.h>
#include <gea/Blocker.h>
#include <gea/API.h>

#include <awds/PktPair.h>
#include <awds/RTTMetric.h>
#include <awds/TTMetric.h>
#include <awds/gea2mad.h>
#include <awds/Traffic.h>

#include <sstream>


extern "C"
#ifdef PIC
int gea_main(int argc, const char  * const * argv) 
#else
int awdsRouting_gea_main(int argc, const char  * const *argv) 
#endif
{
  ObjRepository& rep = ObjRepository::instance();
  bool stop(false);
  std::string result;
  for (int i(1);i<argc;++i) {
    std::string w(argv[i]);
    std::string p;
    if (i < argc-1) {
      p = argv[i+1];
    }
    std::string p1,p2;
    if (i < argc-2) {
      p1 = argv[i+2];
    }
    if (i < argc-3) {
      p2 = argv[i+3];
    }
    if (w == "tt") {
      gea2mad *g2m(static_cast<gea2mad*>(rep.getObj("gea2mad")));
      if (!g2m) {
	GEA.dbg() << "cannot find objet 'gea2mad' in repository" << std::endl;
      } else {
	std::stringstream str(p);
	int id;
	str >> id;
	GEA.dbg() << "Transmittime: " << g2m->getTT(id) << std::endl;
      }
    }
    if (w == "--rtt-history") {
      RTTMetric *rtt(static_cast<RTTMetric*>(rep.getObj("rttmetric")));
      if (rtt) {
	result = rtt->get_history();
      } else {
	GEA.dbg() << "cannot find object 'rttmetric' in repository" << std::endl;
      }
    }
    if (w == "--rtt-values") {
      RTTMetric *rtt(static_cast<RTTMetric*>(rep.getObj("rttmetric")));
      if (rtt) {
	result = rtt->get_values();
      } else {
	GEA.dbg() << "cannot find object 'rttmetric' in repository" << std::endl;
      }
    }
    if (w == "--tt-values") {
      TTMetric *tt(static_cast<TTMetric*>(rep.getObj("ttmetric")));
      if (tt) {
	result = tt->get_values();
      } else {
	GEA.dbg() << "cannot find object 'ttmetric' in repository" << std::endl;
      }
    }
    if (w == "--pktpair-values") {
      PktPair *pp(static_cast<PktPair*>(rep.getObj("pktpair")));
      if (pp) {
	result = pp->get_values();
      } else {
	GEA.dbg() << "cannot find object 'pktpair' in repository" << std::endl;
      }
    }
    if (w == "--stop") {
      stop = true;
    }
    if (w == "--traffic") {
      int packetCount,packetSize,dest;
      std::stringstream ss(p);
      ss >> packetCount;
      std::stringstream ss1(p1);
      ss1 >> packetSize;
      std::stringstream ss2(p2);
      ss2 >> dest;
      awds::Traffic *traffic(static_cast<Traffic*>(rep.getObj("traffic")));
      if (traffic) {
	traffic->send(packetCount,packetSize,NodeId(dest));
      } else {
	GEA.dbg() << "cannot find object 'traffic' in repository" << std::endl;
      }
    }
  }
  if (result.length()) {
    std::cout << result << std::endl;
  }
  if (stop) {
    GEA.dbg() << "press return" << std::endl;
    std::cin.get();
  }
  return 0;
}
