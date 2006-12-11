
#include <cstdlib>

#include <gea/ObjRepository.h>
#include <gea/Blocker.h>
#include <gea/API.h>


#include <awds/UnicastPacket.h>
#include <awds/routing.h>

struct Pinger {
    
    NodeId dest;
    Routing *awdsRouting;
    gea::Blocker blocker;
    
    gea::Duration period;
    
    unsigned char seq;
    
    Pinger( Routing *awdsRouting):
	awdsRouting(awdsRouting),
	period(2.),
	seq(0)
    {
	
	// register ping protocol
	awdsRouting->registerUnicastProtocol(42, ping_recv, (void*)this );
    }
    
    
    
    Pinger(const NodeId& id, Routing *awdsRouting):
	dest(id),
	awdsRouting(awdsRouting),
	period(2.),
	seq(0)
    {
	
	GEA.waitFor( &blocker, 
		     gea::AbsTime::now() + period, 
		     next_ping,
		     this);
	
	
	// register ping protocol
	awdsRouting->registerUnicastProtocol(42, ping_recv, (void *)this);
	
    }

static void next_ping(gea::Handle *h, gea::AbsTime t, void *data);
static void ping_recv(BasePacket *p, gea::AbsTime t, void *data);
    
};

static void toArray(const gea::Duration& dur, char *buf) {
    
    unsigned long long d = (unsigned long long)(double)dur;
    for (int i = 0; i < 8; ++i) {
	buf[i] = (char)(unsigned char)d;
	d /= 0x0100ULL;
    }
}

static gea::Duration durationFromArray(const char *buf) {
    
    unsigned long long d = 0;
    for (int i = 7; i >= 0; --i) {
	d *= 0x0100ULL;
	d |= (unsigned char)buf[i];
    }
    return gea::Duration( (double)d );
}


void Pinger::next_ping(gea::Handle *h, gea::AbsTime t, void *data) {
    
    Pinger *self = static_cast<Pinger *>(data);

    BasePacket *p = self->awdsRouting->newUnicastPacket(42);
    p->setDest(self->dest);
    
    UnicastPacket uniP(*p);
   
    uniP.setSeq(self->seq++);
    uniP.setUcDest(self->dest);
 
    p->size = UnicastPacket::UnicastPacketEnd + 13;
    p->buffer[UnicastPacket::UnicastPacketEnd] = 'i';
    toArray(t-gea::AbsTime::t0(), &p->buffer[UnicastPacket::UnicastPacketEnd + 1] );
    
    GEA.dbg() << "sending ping to " << self->dest << std::endl;
    
    self->awdsRouting->sendUnicast(p, t);
    p->unref();

    GEA.waitFor( &self->blocker, 
		 t + self->period, 
		 next_ping,
		 data);
    
}



void Pinger::ping_recv(BasePacket *p, gea::AbsTime t, void *data) {
    



    Pinger *self = static_cast<Pinger *>(data);
    


    UnicastPacket uniP(*p);
    
    if ( p->buffer[UnicastPacket::UnicastPacketEnd] == 'i' ) {
	
	GEA.dbg() << "received ping from " << uniP.getSrc() << std::endl;
	
	uniP.setUcDest(uniP.getSrc());
	uniP.setSrc(self->awdsRouting->myNodeId);
	uniP.setTTL(44);
	p->buffer[UnicastPacket::UnicastPacketEnd] = 'o';
	p->ref();
	self->awdsRouting->sendUnicast(p, t);
	p->unref();
    } else {
	
	GEA.dbg() << "received pong from " << uniP.getSrc() 
		  << " dur: " << (double)(t - ( gea::AbsTime::t0() 
					+ durationFromArray( &p->buffer[UnicastPacket::UnicastPacketEnd + 1]) ) )
		  << std::endl;
	
    }

}



extern "C" 
int gea_main(int argc, const char  * const * argv) {
    
    
    ObjRepository& rep = ObjRepository::instance();
    Routing *awdsRouting = (Routing *)rep.getObj("awdsRouting");
    if (!awdsRouting) {
	GEA.dbg() << "cannot find object 'awdsRouting' in repository" << std::endl; 
	return -1;
    }
    
    
    
    
    if (argc > 1) {
	NodeId dest(atoi(argv[1]));
	(void)new Pinger(dest,awdsRouting);
    } else 
	(void)new Pinger(awdsRouting);
    return 0;
}
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
