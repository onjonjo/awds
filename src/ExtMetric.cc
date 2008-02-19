#include <awds/ExtMetric.h>
#include <iostream>

awds::ExtMetric::ExtMetric(awds::Routing *r):Metric(r) {
}

void awds::ExtMetric::recv_packet(BasePacket *p,void *data) {
  ExtMetric *instance(static_cast<ExtMetric*>(data));
  instance->on_recv(p);
}


void awds::ExtMetric::wait(gea::Handle *h,gea::AbsTime t,void *data) {
  ExtMetric *instance(static_cast<ExtMetric*>(data));
  instance->on_wait(h,t);
}
