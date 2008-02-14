#include <awds/Traffic.h>
#include <iostream>


using namespace awds;
using namespace std;
using namespace gea;


awds::Traffic::Traffic(Type t,Routing *r):type(t),routing(r),debug(false),packetCount(0),count(0),start(gea::AbsTime::now()) {
  routing->registerUnicastProtocol(PACKET_TYPE_TRAFFIC,recv_packet,(void*)this);
}

void awds::Traffic::send(int pCount,int pSize,NodeId d) {
  if (type == sink) {
    GEA.dbg() << "Traffic: I am a Sink not a Source!" << std::endl;
    return;
  }
  if (!packetCount) {
    packetCount = pCount;
    packetSize = pSize;
    dest = d;
    lastcount = count = 0;
    start = gea::AbsTime::now();
  }
  if (count > packetCount) {
    return;
  }
  if (!routing->isReachable(dest)) {
    if (debug) {
      GEA.dbg() << gea::AbsTime::now()-start << " Traffic: destination not reachable, going to wait for 10 seconds" << std::endl;
      std::cout << static_cast<AwdsRouting*>(routing)->topology->getAdjString() << std::endl;
    }
    GEA.waitFor(&blocker,
		gea::AbsTime::now()+10,
		&Traffic::wait,
		(void*)this);
    return;
  }
  if (debug) {
    GEA.dbg() << "Destination reachable" << std::endl;
  }
  if (count == 0) {
    start = gea::AbsTime::now();
  }
  BasePacket *p = routing->newUnicastPacket(PACKET_TYPE_TRAFFIC);
  TrafficPacket tp(*p);
  tp.setType(TrafficPacket::fromsrc);
  tp.setSeq(count);
  count++;
  lastcount = count;

  UnicastPacket uniP(*p);
  uniP.setUcDest(dest);
  uniP.packet.size = packetSize;
  p->setDest(dest);
  routing->sendUnicast(p);
  GEA.waitFor(&blocker,
	      gea::AbsTime::now()+10,
	      &Traffic::wait,
	      (void*)this);
}

void awds::Traffic::wait(gea::Handle *h,gea::AbsTime t,void *data) {
  Traffic *instance(static_cast<Traffic*>(data));
  instance->on_wait(h,t);
}

void awds::Traffic::on_wait(gea::Handle *h,gea::AbsTime t) {
  if (type == src) {
    if (lastcount == count) {
      send(0,0,NodeId(0));
    }
  } else {
    if (routing->isReachable(dest)) {
      send_reply(dest);
    } else {
      std::cout << "waiting" << std::endl;
      GEA.waitFor(&blocker,
		  gea::AbsTime::now()+1,
		  &Traffic::wait,
		  (void*)this);
    }
  }
}


void awds::Traffic::recv_packet(BasePacket *p,void *data) {
  Traffic *instance(static_cast<Traffic*>(data));
  instance->on_recv(p);
}

void awds::Traffic::send_reply(NodeId dest) {
  BasePacket *rp = routing->newUnicastPacket(PACKET_TYPE_TRAFFIC);
  TrafficPacket tp(*rp);
  tp.setType(TrafficPacket::fromsink);
  tp.setSeq(count);
  UnicastPacket uniP(*rp);
  uniP.setUcDest(dest);
  rp->setDest(dest);
  routing->sendUnicast(rp);
}

void awds::Traffic::on_recv(BasePacket *p) {
  if (debug) {
    GEA.dbg() << GEA.lastEventTime - start <<" got packet" << std::endl;
    //    GEA.dbg() << count << " " << packetCount << " " << std::endl;
  }
  if (type == src) {
    TrafficPacket tp(*p);
    if (tp.getType() == TrafficPacket::fromsink) {
      if (packetCount > count) {
	// send next packet
	if (debug) {
	  GEA.dbg() << "sending packet no:" << count << std::endl;
	}
	send(0,0,NodeId(0));
      } else {
	// finished, print result:
	end = GEA.lastEventTime;
	count = packetCount+1;
	GEA.dbg() << "Traffic finished start: " << "  end: "  << " difference: " << end-start << std::endl;
	exit(1);
      }
    } else {
      GEA.dbg() << "Received unknown packet" << std::endl;
    }
  } else {
    if (TrafficPacket(*p).getType() == TrafficPacket::fromsrc) {
      //respond
      NodeId src(TrafficPacket(*p).getSrc());
      //      GEA.dbg() << "respond to:" << (int) src<< std::endl;

      if (routing->isReachable(src)) {
	send_reply(src);
      } else {
	std::cout << "waiting" << std::endl;
	dest = src;
	GEA.waitFor(&blocker,
		    GEA.lastEventTime + Duration(1,1),
		    &Traffic::wait,
		    (void*)this);
      }
     }
  }
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

  awds::Traffic::Type type(awds::Traffic::src);
  bool debug(false);
  for (int i(1);i<argc;++i) {
    std::string w(argv[i]);
    if (w == "--sink") {
      type = awds::Traffic::sink;
    }
    if (w == "--debug") {
      debug = true;
    }
  }

  awds::Traffic *traffic(new awds::Traffic(type,routing));
  traffic->debug = debug;

  REP_INSERT_OBJ(awds::Traffic*,traffic,traffic);
  if (type == awds::Traffic::src) {
    GEA.dbg() << "Traffic-Generator installed: Source" << std::endl;
  } else {
    GEA.dbg() << "Traffic-Generator installed: Sink" << std::endl;
  }
  return 0;
}
