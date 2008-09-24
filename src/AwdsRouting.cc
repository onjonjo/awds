#include <iostream>
#include <algorithm>

#include <gea/API.h>
#include <gea/ObjRepository.h>
#include <gea/gea_main.h>

#include <awds/AwdsRouting.h>

#include <awds/toArray.h>
#include <awds/SrcPacket.h>
#include <awds/beacon.h>
#include <awds/TopoPacket.h>
#include <awds/UnicastPacket.h>
#include <awds/FlowPacket.h>
#include <awds/TraceUcPacket.h>
#include <awds/Topology.h>
#include <awds/FloodHistory.h>
#include <awds/sqrt_int.h>

#include <awds/ext/Shell.h>

#include <crypto/CryptoUnit.h>

#define DBGTIME(t)  std::fixed << (t) - gea::AbsTime::t0() << ": "

using namespace std;
using namespace awds;
using namespace gea;


awds::AwdsRouting::AwdsRouting(basic *base) :
    FlowRouting(base),
    verbose(false),
    firewall(0), // no firewall by default.
    beaconSeq(0),
    beaconPeriod(this->period, 1000),
    nextBeacon( GEA.lastEventTime + gea::Duration(1,1000)),
    floodSeq(0),
    unicastSeq(0)
{

    this->numNeigh = 0;
    this->neighbors = new NodeDescr[MaxNeighbors];
    this->base = base;
    this->topoPeriod_ms = TOPO_INTERVAL;
    this->topoPeriodType = Adaptive;

    GEA.dbg() << "let's go!" << endl;

    this->topology = new RTopology(base->MyId,this);
    this->floodHistory = new FloodHistory();

    assert(topology->myNodeId == myNodeId);


//     GEA.waitFor(this->udpRecv,
//		this->nextBeacon,
//		recv_packet, this);

    base->recv_callback      = AwdsRouting::recv_packet;
    base->recv_callback_data = static_cast<void *>(this);

    GEA.waitFor(&this->beaconBlocker,
		this->nextBeacon,
		send_beacon,
		this);

    GEA.waitFor( &this->blocker ,
		 GEA.lastEventTime + gea::Duration( this->topoPeriod_ms, 1000),
		 send_periodic_topo, this);


}

awds::AwdsRouting::~AwdsRouting() {
    for (int i = 0; i < numNeigh; ++i)
	neighbors[i].lastBeacon->unref();


    delete topology;
    delete floodHistory;
}

void awds::AwdsRouting::recv_beacon(BasePacket *p) {


    Beacon beacon(*p);
    NodeId neighbor;
    beacon.getSrc(neighbor);

    if (verbose) {
	GEA.dbg() << DBGTIME(GEA.lastEventTime) << "got beacon from " << neighbor
		<< " with seq.nr. " << beacon.getSeq() << std::endl;
    }

    // check signature, if available
    if (this->cryptoUnit) {
	if ( !cryptoUnit->verifySignature(neighbor, p->buffer, p->size - 32, 0) ) {
	    return;
	    GEA.dbg() << " auth fail for beacon from " << neighbor << std::endl;
	}
	//	p->size -= 32;
    }


    if ( this->refreshNeigh(p) ) {
	if (verbose) {
	    GEA.dbg() << "got packet from new neighbor " << neighbor << std::endl;
	}
    }

}


void awds::AwdsRouting::send_topo() {

    BasePacket *p = newFloodPacket(FloodTypeTopo);
    TopoPacket topo(*p);
    assert(topo.getFloodType() == FloodTypeTopo); // done by constructor of TopoPacket

    topo.setNeigh(this);

    if (topoPeriodType == Adaptive) {
	long n = topology->getNumNodes();
	//    long newPeriod = ((n*n)/ isqrt(n) ) * 200;
	long newPeriod = (200 * n * n * 4)/ isqrt(n * 16);
	const long maxNewPerion = newPeriod + (newPeriod / 5); // force slow increase of period
	newPeriod = min(newPeriod, maxNewPerion);
	topoPeriod_ms =  newPeriod;
    }

    topo.setValidity( 3 * topoPeriod_ms);


    // sign the packet
    if (cryptoUnit) {
	const CryptoUnit::MemoryBlock noSign[] = { {p->buffer + Flood::OffsetLastHop, NodeId::size + 1},
						   {0,0}};
	cryptoUnit->sign(p->buffer, p->size, noSign);
	p->size += 32;
    }

    sendBroadcast(p);
    p->unref();
}


void awds::AwdsRouting::send_periodic_topo(gea::Handle *h, gea::AbsTime t, void *data) {

    AwdsRouting *self = static_cast<AwdsRouting*>(data);

    self->send_topo();

    GEA.waitFor(h, t + gea::Duration(self->topoPeriod_ms,1000),
		send_periodic_topo, data);
}


void awds::AwdsRouting::recv_packet(BasePacket *p, void *data) {
    AwdsRouting *self = static_cast<AwdsRouting*>(data);

    if ( (!self->firewall || self->firewall->check_packet(p) ) &&
	 SrcPacket(*p).getSrc() != self->myNodeId )

	switch (p->getType()) {
	case PacketTypeBeacon:  self->recv_beacon(p);    break;
	case PacketTypeFlood:   self->recv_flood(p);     break;
	case PacketTypeUnicast:
	    if (self->verbose) {
		GEA.dbg() << "received UC packet from "
			  << UnicastPacket(*p).getSrc() << endl;
	    }
	    self->recv_unicast(p);
	    break;
	case PacketTypeForward:
	    self->sendFlowPacket(p); break;
	}
}

void awds::AwdsRouting::send_beacon(gea::Handle *h, gea::AbsTime t, void *data) {

    AwdsRouting *self = static_cast<AwdsRouting*>(data);
    self->calcMpr();
    BasePacket *p = new BasePacket();
    Beacon beacon(*p);
    beacon.setSrc(self->myNodeId);
    beacon.setNeigh(self, t);
    beacon.setSeq(self->beaconSeq++);
    beacon.setPeriod(self->beaconPeriod);

    p->setDest( self->base->BroadcastId );

    if (self->cryptoUnit) {
	self->cryptoUnit->sign(p->buffer, p->size, 0);
	p->size += 32;
    }

    if (!self->base->send(p, true))
	GEA.dbg() << "cannot send beacon packets"<< std::endl;
    p->unref(); // we don't need the packet anymore. 

    // calculate next period in future.
    // this is a safty measure to prevent scheduling events in the past
    while (self->nextBeacon <= GEA.lastEventTime) {
	self->nextBeacon += self->beaconPeriod;
    }
    GEA.waitFor(h, self->nextBeacon, send_beacon, data);
}


bool awds::AwdsRouting::refreshNeigh(BasePacket *p) {

    gea::AbsTime t = GEA.lastEventTime;
    assert(p->getType() == PacketTypeBeacon);

    Beacon beacon(*p);

    NodeId src = beacon.getSrc();

    int idx  = findNeigh( src );

    if ( (idx < 0) && (numNeigh == MaxNeighbors)) // cannot add more neighbors
	return false;

    assert(numNeigh < MaxNeighbors);
    //	GEA.dbg() << "index is " << idx << std::endl;
    bool is_new = idx < 0;
    if (  is_new ) {
	idx = -idx - 1;   // position to insert, calculated by findNeigh, negative because src was not found
	//  new neigbour to be inserted
	// -> shift all successors.
	memmove(&neighbors[ idx + 1 ], &neighbors[idx],
		(numNeigh - idx) * sizeof(NodeDescr));

	neighbors[idx].init( beacon.getSrc(), p, t);
	++numNeigh;
	assert(numNeigh <= MaxNeighbors);
	p->ref();
	(void)neighbors[idx].isGood();
	// update 2hop neighbors.

	beacon.add2Hop(this);

    } else {
	Beacon bLast(* (neighbors[idx].lastBeacon));
	bLast.remove2Hop(this);
	beacon.add2Hop(this);
    }

    NodeDescr& nIdx = neighbors[idx];

    // update beacon history...
    unsigned char lostBeacon = beacon.getSeq() - Beacon(*nIdx.lastBeacon).getSeq();



    nIdx.beaconHist >>= lostBeacon;
    nIdx.beaconHist |= 0x80000000UL;

    nIdx.lastBeacon->unref();
    nIdx.lastBeacon     = p;
    nIdx.lastBeaconTime = t;
    nIdx.beaconInterval = beacon.getPeriod();
    p->ref();

    if (nIdx.updateActive()) {
	/* At this point a reactive topo packet could be send to propagate
	 * the change of the topology. */

	if ( this->linkFailBlocker.status != gea::Handle::Blocked) {
	    /* The checkLinkFail-job is currently not running but has to
	     * be activated */
	    if (this->verbose)
		GEA.dbg() << "checkLinkFailure job activated, next call: "
			  << DBGTIME(nIdx.linkValidity) << std::endl;
	    GEA.waitFor(&this->linkFailBlocker, nIdx.linkValidity,
			checkLinkFailure, this);
	}

    }

    return is_new;
}

void awds::AwdsRouting::checkLinkFailure(gea::Handle *h, gea::AbsTime t, void *data) {

    AwdsRouting *self = static_cast<AwdsRouting*>(data);
    bool topoChanged = false;
    gea::AbsTime *minValidity = NULL;

    if (self->verbose)
	GEA.dbg() << DBGTIME(GEA.lastEventTime) << "checkLinkFailure()"
		<< std::endl;

    for (size_t i = 0; i < (size_t)self->numNeigh; ++i ) {
	NodeDescr &node = self->neighbors[i];
	topoChanged = topoChanged || node.updateInactive();

	if (node.active) {
	    assert( node.linkValidity > GEA.lastEventTime);
	    if ((minValidity == NULL) || (node.linkValidity < *minValidity))
		minValidity = &node.linkValidity;
	}
    }

    if (minValidity) {
	if (self->verbose)
	    GEA.dbg() << "\tnext call: " << std::fixed
		      << DBGTIME(*minValidity) << " now=" << GEA.lastEventTime << std::endl;
	GEA.waitFor(h, *minValidity, checkLinkFailure, data);
    }

    self->removeOldNeigh();

    if (topoChanged) {
	if (self->verbose)
	    GEA.dbg() << "Topology has changed, send aperiodic topo packet." << std::endl;
	self->send_topo();
    }


}

static void cpStat2Dyn(awds::AwdsRouting::Hop2List::value_type& r) {
    r.second.dyn = r.second.stat;
}

void awds::AwdsRouting::stat2dyn() {
    for_each(hop2list.begin(), hop2list.end(), cpStat2Dyn);
}

void awds::AwdsRouting::assert_stat() {


}

std::string awds::AwdsRouting::getNameOfNode(const awds::NodeId& id) const {
    return topology->getNameOfNode(id);
}

bool awds::AwdsRouting::getNodeByName(awds::NodeId& id, const char *name) const {
    return topology->getNodeByName(id, name);
}


void awds::AwdsRouting::removeOldNeigh() {

    size_t newnum = 0;
    for (size_t i = 0; i < (size_t)numNeigh; ++i) {

	if (!neighbors[i].isExpired()) {

	    if (newnum != i) neighbors[newnum] = neighbors[i];
	    ++newnum;

	} else {
	    if (verbose) {
		//GEA.dbg() << "removing old node " << neighbors[i].id << " from list" << std::endl;
	    }

	}
    }
    numNeigh = newnum;
    assert(numNeigh <= MaxNeighbors);
}




void awds::AwdsRouting::calcMpr() {

    stat2dyn();

    for (size_t i = 0; i < (size_t)numNeigh; ++i ) {

	NodeDescr& neigh = neighbors[i];
	neigh.mpr = Beacon(*neigh.lastBeacon).tryRemoveFromMpr(this);

    }

}



void awds::AwdsRouting::sendBroadcast(BasePacket *p) {

    // increase TTL by one, because it will be decreased by recv_flood in the next
    // step
    Flood(*p).incTTL();
    recv_flood(p);

}

void awds::AwdsRouting::sendUnicast(BasePacket *p) {

    UnicastPacket uniP(*p);
    uniP.incTTL();
    uniP.setNextHop(myNodeId);
    recv_unicast(p);
}

void awds::AwdsRouting::sendUnicastVia(BasePacket *p,/*gea::AbsTime t,*/ NodeId nextHop) {
    UnicastPacket ucPacket(*p);

    if (ucPacket.getTTL() == 0)
	return ;

    if (ucPacket.getTraceFlag() ) {
	// append own ID in the packet.
	TraceUcPacket traceP(*p);
	traceP.appendNode(myNodeId);
    }

    ucPacket.setNextHop(nextHop);
    p->setDest(nextHop);

    if (!base->send(p, false))
	GEA.dbg() << " cannot send unicast packet"<< std::endl;
}


void awds::AwdsRouting::registerUnicastProtocol(int num, recv_callback cb, void* data) {
	unicastRegister[num] = RegisterEntry(cb, data);
    }

void awds::AwdsRouting::registerBroadcastProtocol(int num, recv_callback cb, void* data) {
    broadcastRegister[num] = RegisterEntry(cb, data);
}




void awds::AwdsRouting::recv_flood(BasePacket *p ) {
    Flood flood(*p);

    NodeId    srcId  = flood.getSrc();
    u_int16_t srcSeq = flood.getSeq();


    if (floodHistory->contains(srcId, srcSeq)) {
	//	GEA.dbg() << "received duplicate" << std::endl;
	return;
    }

    /*     GEA.dbg() << "received flood from " << flood.getSrc()
	   << " seq=" << (unsigned)flood.getSeq()
	   << " ttl=" << flood.getTTL()
	      << std::endl;
    */
    floodHistory->insert(srcId, srcSeq);

    if (flood.getTTL() == 0) {
	GEA.dbg() << "received illegal packet with TTL=0" << std::endl;
	return;
    }
    if (flood.getFloodType() == FloodTypeTopo) {
	//	GEA.dbg() << "received topo packet" << std::endl;

	TopoPacket topoPacket(*p);

	bool validPacket = true;

	if (this->cryptoUnit) {

	    CryptoUnit::MemoryBlock noSign[]
		= { {p->buffer + Flood::OffsetLastHop, NodeId::size + 1},
		    {0,0} };
	    if ( ! this->cryptoUnit->verifySignature(srcId, p->buffer, p->size - 32, noSign) ) {
		validPacket = false;
		GEA.dbg() << "auth fail for topo packet from " <<  srcId << std::endl;
		return;
	    }
	    //	    p->size -= 32;
	}

	//	topoPacket.print();
	if (validPacket)
	    topology->feed(topoPacket);

	//	if (myNodeId == NodeId(2)) {
	//	    this->topology->print();
	//	}

    } else {

	ProtocolRegister::iterator itr = broadcastRegister.find(flood.getFloodType());
	if (itr != broadcastRegister.end()) {
	    itr->second.first(p,itr->second.second);

	} else {
	    if (verbose)
		GEA.dbg() << "unknown Flood Type " << flood.getFloodType() << std::endl;
	}
    }


    // else: we should repeat it!

    int nIdx = findNeigh(flood.getLastHop());

    if ( ( nIdx >= 0 ) ) {
	Beacon lastBeacon(*(neighbors[nIdx].lastBeacon));
	if (lastBeacon.hasNoMpr(myNodeId)) {
	    //GEA.dbg() << "MPR things happened" << endl;
	    return;
	}
    }

    // LastHop is used for the MPR mechanism.
    flood.setLastHop(myNodeId);
    flood.decrTTL();
    if (flood.getTTL() == 0)
	return;

    //    p->ref();
    p->setDest( base->BroadcastId );
    if (!base->send(p, false)) {
	//	GEA.dbg() << " cannot send flood packet"<< std::endl;
    }
}


BasePacket *awds::AwdsRouting::newFloodPacket(int floodType) {

    BasePacket * p = new BasePacket();
    if (p) {
	Flood flood(*p);

	flood.setSrc(myNodeId);
	flood.setLastHop(myNodeId);
	flood.setFloodType(floodType);
	flood.setTTL(32);
	flood.setSeq(floodSeq++);

	p->setDest( base->BroadcastId );
    }

    return p;
}



BasePacket *awds::AwdsRouting::newUnicastPacket(int type) {

    BasePacket *p = new BasePacket();
    if (p) {
	UnicastPacket uniP(*p);
	uniP.setTraceFlag(false);
	uniP.setSrc(myNodeId);
	uniP.setNextHop(myNodeId);
	uniP.setTTL(32);
	uniP.setSeq(unicastSeq++);
	uniP.setUcPacketType(type);
    }
    return p;
}



void awds::AwdsRouting::recv_unicast(BasePacket *p) {
    UnicastPacket ucPacket(*p);

    ucPacket.decrTTL();
    if (ucPacket.getTTL() == 0)
	return ;

    if (ucPacket.getNextHop() != myNodeId)
	return;

    if (ucPacket.getTraceFlag() ) {
	// append own ID in the packet.
	TraceUcPacket traceP(*p);
	traceP.appendNode(myNodeId);
    }

    NodeId dest = ucPacket.getUcDest();

    if (dest == myNodeId) {
	ProtocolRegister::iterator itr = unicastRegister.find(ucPacket.getUcPacketType());
	if (itr != unicastRegister.end()) {
	    itr->second.first(p,  itr->second.second);
	}

	return;
    }

    NodeId nextHop;
    bool found;
    topology->getNextHop(dest, nextHop, found);
    if (!found) {
	GEA.dbg() << "no route to host " << dest << std::endl;
	//	topology->print();
	return;
    } else {
//	if (verbose) {
//	    GEA.dbg() << "next hop to " << dest
//		      << " is " << nextHop
//		      << std::endl;
//	}

    }
    ucPacket.setNextHop(nextHop);
    p->setDest(nextHop);
  
    if (!base->send(p, false))
	GEA.dbg() << " cannot send unicast packet"<< std::endl;
}


int awds::AwdsRouting::foreachNode(NodeFunctor f, void *data) const {
    int ret = 0;
    int ret2;
    RTopology::AdjList::const_iterator itr;

    for (itr = topology->adjList.begin();
	 itr != topology->adjList.end();
	 ++itr)
	{

	    ret++;
	    ret2 =  f(data, itr->first);
	    if (ret2)
		return ret2;
	}

    return ret;
}

int awds::AwdsRouting::foreachEdge(EdgeFunctor f, void *data) const {
    int ret;
    int ret2;
    RTopology::AdjList::const_iterator  itr1;
    RTopology::LinkList::const_iterator itr2;

    for (itr1 = topology->adjList.begin();
	 itr1 != topology->adjList.end();
	 ++itr1)
	{
	    const RTopology::LinkList& llist = itr1->second.linklist;
	    ret++;

	    for (itr2 = llist.begin();
		 itr2 != llist.end();
		 ++itr2)
		{
		    ret2 =  f(data, itr1->first, getNodeId(*itr2) );

		    if (ret2)
			return ret2;
		}
	}

    return ret;

}


void awds::AwdsRouting::addNodeObserver(struct NodesObserver *observer) {
    observer->next = topology->nodeObservers;
    topology->nodeObservers = observer;
    //    GEA.dbg() << "adding node observer " << (void*)topology->nodeObservers << endl;
}

void awds::AwdsRouting::addLinkObserver(struct LinksObserver *observer) {
    observer->next = topology->linkObserver;
    topology->linkObserver  = observer;
}


bool awds::AwdsRouting::isReachable(const NodeId& id) const {
    bool ret;
    NodeId nH;
    topology->getNextHop(id, nH, ret);
    return ret;
}


size_t awds::AwdsRouting::getMTU() {

    size_t max = UnicastPacket::UnicastPacketEnd;
    if (Flood::FloodHeaderEnd > max)
	max = Flood::FloodHeaderEnd;
    return 2000U - max;
}

int  awds::AwdsRouting::addForwardingRule(FlowRouting::FlowId flowid, NodeId nextHop) {
    if (forwardingTable.find(flowid) != forwardingTable.end() ) {
	// entry already exists;
	return -1;
    }
    forwardingTable[flowid] = nextHop;
    return 0;
}

int  awds::AwdsRouting::delForwardingRule(FlowRouting::FlowId flowid) {
    ForwardingTable::iterator itr  = forwardingTable.find(flowid);
    if (itr == forwardingTable.end())
	return -1; // entry not found;
    forwardingTable.erase(itr);
    return 0;
}

int awds::AwdsRouting::addFlowReceiver(FlowRouting::FlowId flowid, FlowRouting::FlowReceiver r, void *data) {
    FlowReceiverMap::const_iterator itr = flowReceiverMap.find(flowid);
    if (itr != flowReceiverMap.end()) {
	/* entry already exists! */
	return -1;
    }
    FlowReceiverData &entry = flowReceiverMap[flowid];
    entry.receiver = r;
    entry.data     = data;
    return 0;
}

int awds::AwdsRouting::delFlowReceiver(FlowRouting::FlowId flowid) {
    FlowReceiverMap::iterator itr = flowReceiverMap.find(flowid);
    if (itr == flowReceiverMap.end()) {
	/*	cannot find  the entry */
	return -1;
    }

    flowReceiverMap.erase(itr);
    return 0;
}

BasePacket *awds::AwdsRouting::newFlowPacket(FlowRouting::FlowId flowid) {
    BasePacket *p = new BasePacket();
    if (p) {
	p->setType(PacketTypeForward);
	awds::FlowPacket flowP(*p);
	flowP.setTraceFlag(false);
	flowP.setSrc(myNodeId);
	flowP.setFlowId(flowid);
    }
    return p;

}

int awds::AwdsRouting::sendFlowPacket(BasePacket *p) {

    int retval;
    
    assert(p->getType() == PacketTypeForward);

    awds::FlowPacket flowP(*p);

    if (flowP.getFlowDest() == myNodeId) {
	// I'm the destination node
	FlowReceiverMap::iterator itr = flowReceiverMap.find(flowP.getFlowType());
	if (itr == flowReceiverMap.end()) {
	    GEA.dbg() << "received flow type " << flowP.getFlowType() << ", but there's no handler registered" << endl;
	    return 0;
	}
	itr->second.receiver(p, itr->second.data);
	return 0;
    }

    ForwardingTable::const_iterator citr =
	forwardingTable.find( flowP.getFlowId() );

    if (citr == forwardingTable.end())
	return -1;

    p->setDest( citr->second );

    retval =  base->send(p, true)?0:-1;

    return retval;
}


static const char *topoPeriod_cmd_usage =
    "topoperiod ( show | adaptive | constant <millisecs> )\n"
    "   show            - show current settings\n"
    "   adaptive        - use adaptive period\n"
    "   constant <ms>   - use constant period of <ms> milliseconds.\n";


static int topoPeriod_command_fn(ShellClient &sc, void *data, int argc, char **argv) {
    AwdsRouting *self = static_cast<AwdsRouting*>(data);

    if (argc == 2 && (string(argv[1]) == "show")) {
	*sc.sockout << (self->topoPeriodType == AwdsRouting::Adaptive ? "Adaptive: " : "Constant: ");
	*sc.sockout << self->topoPeriod_ms << " ms" << endl;
    } else if (argc == 2 && (string(argv[1]) == "adaptive")) {
	*sc.sockout << "Setting adaptive period." << endl;
	self->topoPeriodType = AwdsRouting::Adaptive;
    } else if (argc == 3 && (string(argv[1]) == "constant")) {
	int p = atoi(argv[2]);
	if (p == 0) {
	    *sc.sockout << "Invalid value: " << p << endl;
	    return -1;
	}
	*sc.sockout << "Setting constant period to " << p << " ms." << endl;
	self->topoPeriodType = AwdsRouting::Constant;
	self->topoPeriod_ms = p;
    } else {
	*sc.sockout << topoPeriod_cmd_usage;
    }
}


static const char *beaconPeriod_cmd_usage =
    "beaconperiod ( show | set <millisecs> )\n"
    "   show            - show current settings\n"
    "   set <ms>        - set period to <ms> milliseconds.\n";


static int beaconPeriod_command_fn(ShellClient &sc, void *data, int argc, char **argv) {
    AwdsRouting *self = static_cast<AwdsRouting*>(data);

    if (argc == 2 && (string(argv[1]) == "show")) {
	*sc.sockout << (self->beaconPeriod * 1000) << " ms" << endl;
    } else if (argc == 3 && (string(argv[1]) == "set")) {
	int p = atoi(argv[2]);
	if (p == 0) {
	    *sc.sockout << "Invalid value: " << p << endl;
	    return -1;
	}
	*sc.sockout << "Setting period to " << p << " ms." << endl;
	self->beaconPeriod = gea::Duration(p, 1000);
    } else {
	*sc.sockout << beaconPeriod_cmd_usage;
    }
}


static const char *verbose_cmd_usage =
    "verbose [ on | off ]\n"
    "  set or unset verbose debug ouput\n";


static int verbose_command_fn(ShellClient &sc, void *data, int argc, char **argv) {
    AwdsRouting *self = static_cast<AwdsRouting*>(data);

    if (argc == 1)
	*sc.sockout << "verbose: " << (self->verbose ? "on" : "off" ) << std::endl;
    else if (argc == 2) {
	    if (string(argv[1]) == "on")
		    self->verbose = true;
	    else if (string(argv[1]) == "off")
		    self->verbose = false;
	    else
		    *sc.sockout << verbose_cmd_usage;
    } else {
        *sc.sockout << verbose_cmd_usage;
    }
}


#define MODULE_NAME awdsrouting

GEA_MAIN_2(awdsrouting, argc, argv)
{
    for (int i(0);i<argc;++i) {
	    std::string w(argv[i]);
        if (w == "--help") {
            GEA.dbg() << "awdsrouting\t: please specify the node name in the following format:" << endl
                      << "awdsrouting\t: "<< argv[0] << " --name <nodename>" << endl;
            return -1;
        }
    }

    ObjRepository& rep = ObjRepository::instance();
    basic *base = (basic *)rep.getObj("basic");
    if (!base) {
	GEA.dbg() << "cannot find object 'basic' in repository" << endl;
    return -1;
    }

    AwdsRouting* awdsRouting = new AwdsRouting(base);

    REP_INSERT_OBJ(awds::AwdsRouting *, awdsRouting, awdsRouting);
    REP_INSERT_OBJ(awds::FlowRouting *, flowRouting, awdsRouting);
    REP_INSERT_OBJ(awds::Routing *,     routing,     awdsRouting);
    REP_INSERT_OBJ(awds::RTopology *,   topology,    awdsRouting->topology);
    REP_INSERT_OBJ(awds::Firewall **,   firewall_pp, &(awdsRouting->firewall) );

    bool name_found = false;

    for (int i(0);i<argc;++i) {
	    std::string w(argv[i]);
	    if (w == "--verbose") {
	        awdsRouting->topology->verbose = true;
	        awdsRouting->verbose = true;
	        awds::NodeDescr::verbose = true;
	    }
	    else if ((w == "--name") && (i+1 <= argc)) {
	        strncpy(awdsRouting->topology->nodeName, argv[i+1], 32);
            name_found = true;
	    }
    }

    if(!name_found)
    {
        GEA.dbg() << "awdsrouting\t: please specify the node name in the following format:" << endl
                  << "awdsrouting\t: "<< argv[0] << " --name <nodename>" << endl;
        return -1;
    }

    REP_MAP_OBJ(Shell *, shell);
    if (shell) {
	shell->add_command("topoperiod", topoPeriod_command_fn, awdsRouting,
		"set period for topology packets", topoPeriod_cmd_usage);
	shell->add_command("beaconperiod", beaconPeriod_command_fn, awdsRouting,
		"set period for beacon packets", beaconPeriod_cmd_usage);
	shell->add_command("verbose", verbose_command_fn, awdsRouting,
		"set verbose debug output", verbose_cmd_usage);
    }

    return 0;
}

bool awds::NodeDescr::verbose = false;

/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
