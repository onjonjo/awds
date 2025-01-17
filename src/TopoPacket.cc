

#include <algorithm>

#include <awds/Topology.h>
#include <awds/TopoPacket.h>
#include <awds/routing.h>
#include <awds/RateMonitor.h>

#include <awds/Metric.h>

#include <iostream>

using namespace std;
using namespace awds;
using namespace gea;

void TopoPacket::setNeigh(AwdsRouting *awdsRouting) {

    unsigned char  n = 0;

    char *addr =  &(packet.buffer[OffsetLinks]);

    /*
	  RateMonitor *rm = awdsRouting->madwifiRateMonitor;

	  if ( (awdsRouting->metrics == Routing::TransmitDurationMetrics) && (rm != 0) )
	  rm->update();*/

    awdsRouting->topology->metric->update();
    for (int i = 0; i < awdsRouting->numNeigh; ++i) {

	if ( awdsRouting->neighbors[i].isBidiGood(awdsRouting->myNodeId) )
	    {
		NodeId nId = awdsRouting->neighbors[i].id;
		nId.toArray(addr);
		++n;
		addr += NodeId::size;
		// the installed metric decides which quality values will be send
		RTopology::link_quality_t quality = awdsRouting->topology->metric->get_quality(awdsRouting->neighbors[i]);

		toArray<uint16_t>( quality, addr);
		addr += 2;

	    }
    }
    const char* nodeName = awdsRouting->topology->nodeName;
    int len = strlen(nodeName);
    if (len > 32) len = 32;
    *(addr++) = (char)len;
    memcpy(addr, nodeName, len);

    packet.buffer[OffsetNumLinks] = (char)n;
    packet.size =
	OffsetLinks + (NodeId::size + sizeof(RTopology::link_quality_t)) * (size_t) n
	+ len+1 /* the station name */;

    //  assert(getNumLinks() == n);
}

#include <iostream>

using namespace std;

void TopoPacket::print() {

    ostream& os = GEA.dbg();

    os << "packet[" << packet.size << "] " << getSrc() << " : ";

    int n = getNumLinks();

    char *addr =  &(packet.buffer[OffsetLinks]);

    NodeId id;
    for (int i = 1 ; i < n; ++i) {

	id.fromArray(addr);
	os << id << " ";
	addr += NodeId::size + 1;
    }

    os << endl;
}


/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
