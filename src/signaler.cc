#include <iostream>

#include <gea/ObjRepository.h>
#include <gea/Blocker.h>
#include <gea/API.h>

#include <awds/RTTMetric.h>


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
    if (w == "--stop") {
      stop = true;
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
