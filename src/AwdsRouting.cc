
#include <iostream>
#include <algorithm>

#include <gea/API.h>
#include <gea/ObjRepository.h>

#include <awds/AwdsRouting.h> 

#include <awds/toArray.h>
#include <awds/SrcPacket.h>
#include <awds/beacon.h>
#include <awds/TopoPacket.h>
#include <awds/UnicastPacket.h>
#include <awds/Topology.h>
#include <awds/FloodHistory.h>
#include <awds/sqrt_int.h>

#include <crypto/CryptoUnit.h>

using namespace std;

awds::AwdsRouting::AwdsRouting(basic *base) :
    Routing(base->MyId),
    beaconSeq(0),
    beaconPeriod((double)(this->period) / 1000.),
    nextBeacon( gea::AbsTime::now() + beaconPeriod),
    floodSeq(0),
    unicastSeq(0),
    madwifiRateMonitor(0)
{
    
    this->numNeigh = 0;
    this->neighbors = new NodeDescr[MaxNeighbors];
    this->base = base;
    this->topoPeriod = TOPO_INTERVAL;
    
    GEA.dbg() << "let's go!" << endl;
    //     this->udpSend = new gea::UdpHandle( gea::UdpHandle::Write,
    // 					gea::UdpAddress(UdpPort /*port*/,
    // 							gea::UdpAddress::IP_BROADCAST
    // 							/*ip*/ ));
    
    //     this->udpRecv = new gea::UdpHandle(gea::UdpHandle::Read,
    // 				       gea::UdpAddress(UdpPort /*port*/,
    // 						       gea::UdpAddress::IP_ANY /*ip*/ ));
    this->udpSend = base->sendHandle;
    this->udpRecv = base->recvHandle;
    
    this->topology = new RTopology(base->MyId);
    this->floodHistory = new FloodHistory();
    
    assert(topology->myNodeId == myNodeId);
  
    
    GEA.waitFor(this->udpRecv,
		this->nextBeacon,
		recv_packet, this);


    GEA.waitFor( &this->blocker , 
		 gea::AbsTime::now() + gea::Duration( (double)topoPeriod / 1000.),
		 trigger_topo, this);

    
}

awds::AwdsRouting::~AwdsRouting() {
    for (int i = 0; i < numNeigh; ++i)
	neighbors[i].lastBeacon->unref();
    
    
    delete topology;
    delete floodHistory;
}

void awds::AwdsRouting::recv_beacon(BasePacket *p, gea::AbsTime t) {
    
    
    Beacon beacon(*p);
    NodeId neighbor;
    beacon.getSrc(neighbor);

    // check signature, if available
    if (this->cryptoUnit) {
	if ( !cryptoUnit->verifySignature(neighbor, p->buffer, p->size - 32, 0) ) {
	    return;
	    GEA.dbg() << " auth fail for beacon from " << neighbor << std::endl;
	}
	//	p->size -= 32;
    }
    

    if ( this->refreshNeigh(p, t) ) {
	GEA.dbg() << "got packet from new neighbor " << neighbor << std::endl;	  
    }
	
}



void awds::AwdsRouting::trigger_topo(gea::Handle *h, gea::AbsTime t, void *data) {
    
    AwdsRouting *self = static_cast<AwdsRouting*>(data);

    //    GEA.waitFor( self->udpSend, t + gea::Duration(12.2), send_topo, data);
    
      
    BasePacket *p = self->newFloodPacket(FloodTypeTopo);
    TopoPacket topo(*p);
    assert(topo.getFloodType() == FloodTypeTopo); // done by constructor of TopoPacket
    
    topo.setNeigh(self, t);
    long n = self->topology->getNumNodes();
    
    //    long newPeriod = ((n*n)/ isqrt(n) ) * 200; 
    long newPeriod = (200 * n * n * 4)/ isqrt(n * 16); 

    if ( newPeriod > 2 * self->topoPeriod) { // force slow increase of period
	
	newPeriod = 2 * self->topoPeriod;
    }
    
    self->topoPeriod =  newPeriod;
    topo.setValidity( 3 * self->topoPeriod);
    

    // sign the packet
    if (self->cryptoUnit) {
	const CryptoUnit::MemoryBlock noSign[] = { {p->buffer + Flood::OffsetLastHop, NodeId::size + 1},
					     {0,0}};
	self->cryptoUnit->sign(p->buffer, p->size, noSign);
	p->size += 32;
	//	GEA.dbg() << "topo packet siye = " << p->size << endl;
	//assert ( self->cryptoUnit->verifySignature(self->myNodeId, p->buffer, p->size, noSign) );
    }

    self->sendBroadcast(p,t);
    p->unref();
    
    GEA.waitFor(h, t + ( (double)self->topoPeriod * 0.001),
		trigger_topo, data);

    
}


void awds::AwdsRouting::recv_packet(gea::Handle *h, gea::AbsTime t, void *data) {

    AwdsRouting *self = static_cast<AwdsRouting*>(data);
    
    if (h->status == gea::Handle::Ready) {
	
	// okay, we received some packet
	
	BasePacket *p = new BasePacket(); 
	p->receive(h); 
	
	SrcPacket srcP(*p);
	
	if (srcP.getSrc() != self->myNodeId) {
	    
	 
	    if (p->getType() == PacketTypeBeacon) {
		
		self->recv_beacon(p,t);
	
	    } else if (p->getType() == PacketTypeFlood) {
	    
		self->recv_flood(p, t);
	    
	    } else if (p->getType() == PacketTypeUnicast ) {
		self->recv_unicast(p,t);
	    }

	}
	p->unref();
    } else {
	
	self->nextBeacon += gea::Duration((double)(self->period) / 1000. );
		
	GEA.waitFor(self->udpSend,
		    self->nextBeacon,
		    send_beacon, data);
	
    }
    ///GEA.dbg() << "AAAA" <<std::endl;
    GEA.waitFor(h, self->nextBeacon, AwdsRouting::recv_packet, data);
    ///GEA.dbg() << "BBBB" <<std::endl;
}

void awds::AwdsRouting::send_beacon(gea::Handle *h, gea::AbsTime t, void *data) {
    
    AwdsRouting *self = static_cast<AwdsRouting*>(data);
    self->calcMpr();
    BasePacket p;
    Beacon beacon(p);
    beacon.setSrc(self->myNodeId);
    beacon.setNeigh(self, t);
    beacon.setSeq(self->beaconSeq++);
    beacon.setPeriod(self->beaconPeriod);
    
    self->base->setSendDest( self->base->BroadcastId );
    
    if (self->cryptoUnit) {
	self->cryptoUnit->sign(p.buffer, p.size, 0);
	p.size += 32;
    }
    
    if (p.send(h) < 0)
	GEA.dbg() << " cannot send"<< std::endl;
    
    
}



bool awds::AwdsRouting::refreshNeigh(BasePacket *p, gea::AbsTime t) {

    
	
    assert(p->getType() == PacketTypeBeacon);
		
    Beacon beacon(*p);
    
    NodeId src = beacon.getSrc();
    
    int idx  = findNeigh( src );
    
    if ( (idx < 0) && (numNeigh == MaxNeighbors)) // cannot add more neighbors
	return false;

    assert(numNeigh <= MaxNeighbors);
    //	GEA.dbg() << "index is " << idx << std::endl;	
    bool is_new = idx < 0;
    if (  is_new ) {
	idx = -idx - 1; 
	//  new neigbour to be inserted
	// -> shift all successors.
	memmove(&neighbors[ idx + 1 ], &neighbors[idx],
		(numNeigh - idx) * sizeof(NodeDescr));

	neighbors[idx].init( beacon.getSrc(), p, t);
	++numNeigh;
	assert(numNeigh <= MaxNeighbors);
	p->ref();
	(void)neighbors[idx].isGood(t);
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
    //  (void)nIdx.isGood(t);
    //   GEA.dbg() << "lost of " << beacon.getSrc() << " is " 
    // << (unsigned)lostBeacon << " hist=" << nIdx.beaconHist << std::endl;  
    
    nIdx.lastBeacon->unref();
    nIdx.lastBeacon     = p;
    nIdx.lastBeaconTime = t;
    nIdx.beaconInterval = beacon.getPeriod();
    p->ref();	
	
    return is_new;
}


static void cpStat2Dyn(AwdsRouting::Hop2List::value_type& r) {
    r.second.dyn = r.second.stat;
}

void awds::AwdsRouting::stat2dyn() {
    for_each(hop2list.begin(), hop2list.end(), cpStat2Dyn);
}

void awds::AwdsRouting::assert_stat() {
    
    
}




void awds::AwdsRouting::removeOldNeigh(gea::AbsTime t) {

    
    size_t newnum = 0;
    for (size_t i = 0; i < (size_t)numNeigh; ++i) {
	bool tooOld = neighbors[i].isTooOld(t);
	
	if (!tooOld) {
	    
	    if (newnum != i) neighbors[newnum] = neighbors[i];
	    ++newnum;
	    
	} else {
	    
	    GEA.dbg() << "removing old node " << neighbors[i].id << " from list" << std::endl;
	    
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



void awds::AwdsRouting::sendBroadcast(BasePacket *p, gea::AbsTime t) {
    
    // increase TTL by one, because it will be decreased by recv_flood in the next
    // step 
    Flood(*p).incTTL();
    recv_flood(p,t);
    
}

void awds::AwdsRouting::sendUnicast(BasePacket *p, gea::AbsTime t) {
    
    UnicastPacket uniP(*p);
    uniP.setNextHop(myNodeId);
    recv_unicast(p, t);
    
}


void awds::AwdsRouting::registerUnicastProtocol(int num, recv_callback cb, void* data) {
	unicastRegister[num] = RegisterEntry(cb, data);
    }
    
void awds::AwdsRouting::registerBroadcastProtocol(int num, recv_callback cb, void* data) {
    broadcastRegister[num] = RegisterEntry(cb, data);
}
    



void awds::AwdsRouting::recv_flood(BasePacket *p, gea::AbsTime t) {
        
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
	    topology->feed(topoPacket, t);

	//GEA.dbg() << "got topo from " << srcId << std::endl;
	
	// 	if (myNodeId == NodeId(2)) {
	// 	    this->topology->print();
	// 	}
    
    } else { 
	
	ProtocolRegister::iterator itr = broadcastRegister.find(flood.getFloodType());
	if (itr != broadcastRegister.end()) {
	    itr->second.first(p,t,itr->second.second);
	    
	} else {
	    GEA.dbg() << "unknown Flood Type" << flood.getFloodType() << std::endl;
	}
    }
    
    
    // else: we should repeat it!
    
    int nIdx = findNeigh(flood.getLastHop());
    
    if ( ( nIdx >= 0 ) ) {
	Beacon lastBeacon(*(neighbors[nIdx].lastBeacon));
	if (lastBeacon.hasNoMpr(myNodeId)) {
	    return;
	}
    }
    
    flood.decrTTL();
    if (flood.getTTL() == 0) 
	return;
	
    p->ref();
        
    GEA.waitFor(udpSend,
		t + gea::Duration(12.2),
		AwdsRouting::repeat_flood, 
		p);
    
    
}



void awds::AwdsRouting::repeat_flood(gea::Handle *h, gea::AbsTime t, void *data) {
    
    BasePacket *p = (BasePacket *)data;
    
    //self->base->setSendDest( self->base->BroadcastId );
    
    p->send(h);
    
    p->unref();
    
    
}


BasePacket *awds::AwdsRouting::newFloodPacket(int floodType) {
	
    BasePacket * p = new BasePacket();
    Flood flood(*p);
	
    flood.setSrc(myNodeId);
    flood.setLastHop(myNodeId);
    flood.setFloodType(floodType);
    flood.setTTL(32);
    flood.setSeq(floodSeq++);
    
    return p;
}



BasePacket *awds::AwdsRouting::newUnicastPacket(int type) {
    
    BasePacket *p = new BasePacket();
    
    UnicastPacket uniP(*p);
    
    uniP.setSrc(myNodeId);
    uniP.setNextHop(myNodeId);
    uniP.setTTL(32);
    uniP.setSeq(unicastSeq++);
    uniP.setUcPacketType(type);
    return p;
}



void awds::AwdsRouting::recv_unicast(BasePacket *p, gea::AbsTime t) {
    
    UnicastPacket ucPacket(*p);
    
    ucPacket.decrTTL();
    if (ucPacket.getTTL() == 0)
	return ;
    
    if (ucPacket.getNextHop() != myNodeId)
	return;
    
    NodeId dest = ucPacket.getUcDest();
    
    if (dest == myNodeId) {
	//	GEA.dbg() << "received Packet from " << ucPacket.getSrc() << std::endl;
	
	ProtocolRegister::iterator itr = unicastRegister.find(ucPacket.getUcPacketType());
	if (itr != unicastRegister.end()) {
	    itr->second.first(p,t,itr->second.second);
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
// 	GEA.dbg() << "next hop to " << dest 
// 		  << " is " << nextHop 
// 		  << std::endl;
	
    }
    ucPacket.setNextHop(nextHop);
    p->setDest(nextHop);
    p->ref();
    GEA.waitFor(this->udpSend, 
		t + gea::Duration(12.2),
		send_unicast,
		new std::pair<BasePacket *,AwdsRouting *>(p,this) );
    
}



void awds::AwdsRouting::send_unicast(gea::Handle *h, gea::AbsTime t, void *data) {
    
    std::pair<BasePacket *,AwdsRouting *>* xdata = ( std::pair<BasePacket *,AwdsRouting *>* )data;
    BasePacket *p = xdata->first;
    AwdsRouting *self = xdata->second;
    
    UnicastPacket uniP(*p);
    
    self->base->setSendDest( uniP.getNextHop() );
    
    p->send(h);
    
    self->base->setSendDest( self->base->BroadcastId );

    p->unref();
    
    delete xdata;
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

extern "C"
#ifdef PIC
int gea_main(int argc, const char  * const * argv) 
#else
int awdsRouting_gea_main(int argc, const char  * const *argv) 
#endif

{
    
    ObjRepository& rep = ObjRepository::instance();
    basic *base = (basic *)rep.getObj("basic");
    if (!base) {
	GEA.dbg() << "cannot find object 'basic' in repository" << endl; 
	return -1;
    }
    
    AwdsRouting* awdsRouting = new AwdsRouting(base);
    
    REP_INSERT_OBJ(awds::AwdsRouting *, awdsRouting, awdsRouting);
    REP_INSERT_OBJ(awds::Routing *,     routing,     awdsRouting);
    REP_INSERT_OBJ(awds::RTopology *,   topology,    awdsRouting->topology);
    
    //    rep.insertObj("awdsRouting", "AwdsRouting", awdsRouting);
    // rep.insertObj("topology","Topology", awdsRouting->topology);
    
    // RateMonitor *rateMonitor = (RateMonitor *)rep.getObj("rateMonitor");
    REP_MAP_OBJ(awds::RateMonitor *, rateMonitor);
    if (rateMonitor) {
	awdsRouting->madwifiRateMonitor = rateMonitor;
	GEA.dbg() << "adding transmission duration metrics of rate module" << endl;
	awdsRouting->metrics = Routing::TransmitDurationMetrics; 
	//	rateMonitor->update();
    }
    
    if ( (argc >= 3) && (!strcmp(argv[1], "--name") ) ) {
	strncpy(awdsRouting->topology->nodeName, argv[2], 32);
    }
        
    return 0;
}


/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
