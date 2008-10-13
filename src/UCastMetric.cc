#include <awds/UCastMetric.h>
#include <algorithm>

awds::UCastMetric::UCastMetric(awds::Routing *r):ExtMetric(r) {
  routing->registerUnicastProtocol(PACKET_TYPE_UC_METRIC,recv_packet,(void*)this);
}

void awds::UCastMetric::send(BasePacket *p,gea::AbsTime t,NodeId dest) {
  UnicastPacket uniP(*p);
  uniP.setUcDest(dest);
  p->setDest(dest);
  routing->sendUnicast(p);
}

void awds::UCastMetric::sendvia(BasePacket *p,gea::AbsTime t,NodeId dest,unsigned int size) {
  UnicastPacket uniP(*p);
  uniP.setUcDest(dest);
  p->setDest(dest);
  uniP.packet.size = std::max(uniP.packet.size,size);
  routing->sendUnicastVia(p,dest);
}
