#ifndef D__UCMetricPacket
#define D__UCMetricPacket

#include <gea/ObjRepository.h>
#include <gea/Blocker.h>
#include <gea/API.h>

#include <awds/packettypes.h>
#include <awds/TraceUcPacket.h>
#include <awds/AwdsRouting.h>
#include <awds/ext/Shell.h>
#include <awds/Topology.h>
#include <awds/toArray.h>

#include <awds/Flood.h>

namespace awds {

  class UCMetricPacket : public UnicastPacket {
  public:
    enum Type {req,resp,first,second};
    static const size_t OffsetSeq = UnicastPacket::UnicastPacketEnd;
    static const size_t OffsetType = OffsetSeq+sizeof(unsigned int);
    static const size_t OffsetOriginator = OffsetType+sizeof(Type);
    static const size_t OffsetTime1 = OffsetOriginator+sizeof(NodeId);
    static const size_t OffsetTime2 = OffsetTime1+sizeof(gea::AbsTime);
    static const size_t OffsetDuration = OffsetTime2+sizeof(gea::AbsTime);
    static const size_t MetricHeaderEnd = OffsetDuration+sizeof(gea::Duration);
    UCMetricPacket(BasePacket &p);
    UCMetricPacket(BasePacket &p,Type t,unsigned int ttl,unsigned int seq,gea::AbsTime time);
    UCMetricPacket(BasePacket &p,Type t,unsigned int ttl,unsigned int seq,gea::Duration duration);

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
    void setOriginator(const NodeId& id) {
      id.toArray(&packet.buffer[OffsetOriginator]);	
    }
    
    void getOriginator(NodeId& id) const {
      id.fromArray(&packet.buffer[OffsetOriginator]);
    }

    void setTime1(gea::AbsTime t) {
      *((gea::AbsTime*)&packet.buffer[OffsetTime1]) = t;
    }

    gea::AbsTime getTime1() {
      return *((gea::AbsTime*)&packet.buffer[OffsetTime1]);
    }

    void setDuration(gea::Duration d) {
      *((gea::Duration*)&packet.buffer[OffsetDuration]) = d;
    }

    gea::Duration getDuration() {
      return *((gea::Duration*)&packet.buffer[OffsetDuration]);
    }
  };

}

#endif // D__UCMetricPacket
