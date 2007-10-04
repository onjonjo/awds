#include <awds/UCMetricPacket.h>

awds::UCMetricPacket::UCMetricPacket(BasePacket &p):UnicastPacket(p) {
  packet.size = MetricHeaderEnd;
}

awds::UCMetricPacket::UCMetricPacket(BasePacket &p,Type t,unsigned int ttl,unsigned int seq,gea::AbsTime time):UnicastPacket(p) {
  setType(t);
  setTTL(ttl);
  setSeq(seq);
  setTime1(time);
  packet.size = MetricHeaderEnd;
}

awds::UCMetricPacket::UCMetricPacket(BasePacket &p,Type t,unsigned int ttl,unsigned int seq,gea::Duration duration):UnicastPacket(p) {
  setType(t);
  setTTL(ttl);
  setSeq(seq);
  setDuration(duration);
  packet.size = MetricHeaderEnd;
}
