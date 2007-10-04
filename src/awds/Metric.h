#ifndef D__metric
#define D__metric

/* Hier will ich einfach mal meine Idee einer Metrikklasse aufschreiben.
Jede Metrik (hopcnt,ett,etx usw) soll von dieser Klasse ableiten.
Jede Metrik soll ein geamodul werden, dass sich in das routing einklinkt

Ich versuche mal die gemeinsame Schnittstelle aller Metriken zu brainstormen
*/

#include <awds/NodeId.h>
#include <awds/NodeDescr.h>
#include <awds/Topology.h>

#include <limits>

#include <iostream>

namespace awds {
  
  class Routing;


  class Metric {
  protected:
    Routing *routing;

    virtual RTopology::link_quality_t my_get_quality(NodeDescr &ndescr) {
      return 10;
    }
    
    virtual unsigned long my_calculate(RTopology::link_quality_t forward,RTopology::link_quality_t backward) {
      // no testing for "forward" and "backward" greater than 0 neccesary it is all done in calculate and get_qualities
      return 1;
    }
  public:
    Metric(Routing *r):routing(r) {
    }

      virtual ~Metric() {}

      virtual void addNode(NodeId &nodeId) {}
      virtual void begin_update() {}  
      // begin_update() is called before the incoming topopaket is parsed, so that the metric can  reset its node data, see end_update() too
      virtual void end_update() {}
      // end_update() is called right after the topopaket has been parsed, so still reseted nodes can be deleted, so begin_update() too

      virtual int update() {
	return 8;
      }
      // update() is called right before a topopacket ist filled with data (see get_quality(NodeDescr&)) so the mertic can prepare for (for TTMetric in rl it is to parse a file)
      
      RTopology::link_quality_t get_quality(NodeDescr &ndescr) {
	// this method is called by the Topopacket to get the sendable qualityvalues of the Link to ndescr
	return std::max( std::min(my_get_quality(ndescr), (RTopology::link_quality_t)max_quality), (RTopology::link_quality_t)1);
      }

      unsigned long calculate(RTopology::LinkList::iterator &it) {
	// on receive of a Topopacket calculate will be called on each LinkQuality to calculate the 
	// "metric_weight" depended on the quality values by the two corresponding LinkQualities
	RTopology::link_quality_t f,b;
	if (it->get_qualities(f,b)) {
	  unsigned long mw(my_calculate(f,b));
	  it->set_metric_weight(mw);
	  return mw;
	}
	return std::numeric_limits<unsigned long>::max();
      }

  };
}

#endif // D__metric
