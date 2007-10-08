#ifndef D__Traffic
#define D__Traffic

#include <awds/UnicastPacket.h>
#include <gea/Time.h>
#include <gea/Blocker.h>
#include <awds/NodeId.h>
#include <awds/NodeDescr.h>
#include <awds/Topology.h>
#include <gea/ObjRepository.h>
#include <gea/Blocker.h>
#include <gea/API.h>

#include <awds/packettypes.h>
#include <awds/TraceUcPacket.h>
#include <awds/AwdsRouting.h>
#include <awds/ext/Shell.h>
#include <awds/toArray.h>

#include <awds/Flood.h>

namespace awds {
  class Traffic {
  private:
    Traffic() {}
  public:
    enum Type {src,sink};
    Type type;
    Routing *routing;
    bool debug;

    int packetCount,packetSize;
    int count,lastcount;
    NodeId dest;
    gea::AbsTime start,end;
    gea::Blocker blocker;

    Traffic(Type t,Routing *r);
    virtual ~Traffic() {}

    virtual void send(int pCount,int pSize,NodeId d);
    void send_reply(NodeId dest);

    static void recv_packet(BasePacket *p,void *data);
    void on_recv(BasePacket *p);

    static void wait(gea::Handle *h,gea::AbsTime t,void *data);
    void on_wait(gea::Handle *h,gea::AbsTime t);
  };
  class TrafficPacket : public UnicastPacket {
  public:
    enum Type {fromsrc,fromsink};
    static const size_t OffsetSeq = UnicastPacket::UnicastPacketEnd;
    static const size_t OffsetType = OffsetSeq+sizeof(unsigned int);
    static const size_t TrafficHeaderEnd = OffsetType+sizeof(size_t);
    TrafficPacket(BasePacket &p):UnicastPacket(p) {
      packet.size = TrafficHeaderEnd;
    }
    void setSeq(unsigned int s) {
      *((unsigned int*)&packet.buffer[OffsetSeq]) = s;
    }
    unsigned int getSeq() {
      return *((unsigned int*)&packet.buffer[OffsetSeq]);
    }
    void setType(Type t) {
      packet.buffer[OffsetType] = t;
    }
    Type getType() {
      return static_cast<Type>(packet.buffer[OffsetType]);
    }
  };
}

#endif // D__Traffic
