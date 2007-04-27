
#include <cstdlib>
#include <cstdio>

#include <gea/ObjRepository.h>
#include <gea/Blocker.h>
#include <gea/API.h>

#include <awds/packettypes.h>
#include <awds/TraceUcPacket.h>
#include <awds/routing.h>
#include <awds/ext/Shell.h>
#include <awds/Topology.h>
#include <awds/toArray.h>

using namespace awds;
using namespace gea;
using namespace std;

static const char *short_help = "ping another AWDS node";

static const char *long_help = 
    "ping  <DESTINATION>\n";
    


struct Pinger {
    
    bool busy;
    NodeId dest;
    unsigned ttl;
    int numRepeat;
    bool doTracePath;
    
    // stats.
    double minRTT, maxRTT, sumRTT;
    int numReceived;
    int numTransmitted;
    


    awds::Routing *awdsRouting;
    gea::Blocker blocker;
    ostream *out;
    ShellClient *shellClient;
    
    gea::Duration period;
    size_t pingSize;

    gea::AbsTime myT0;
    
    Pinger( awds::Routing *awdsRouting):
	awdsRouting(awdsRouting),
	period(2.)
    {
	busy = false;
	ttl = 44;
	// register ping protocol
	awdsRouting->registerUnicastProtocol(PACKET_TYPE_UC_PING, ping_recv, (void*)this );
	this->out = 0;
	myT0 = gea::AbsTime::now();
    }
    
    static void next_ping(gea::Handle *h, gea::AbsTime t, void *data);
    static void ping_recv(BasePacket *p, gea::AbsTime t, void *data);
    
    //    int x_getNodeByName(awds::NodeId& ret, const awds::RTopology::AdjList& l, const char* name);
    
    
    ostream& dbg() {
	if (out)
	    return *out;
	else
	    return GEA.dbg();
    }
    
    void resetStats();
    void print_stats(std::ostream& out);
    int startPing( Handle *h = 0);
    int parse_opts(int argc, const char* const *argv);
};


void Pinger::resetStats() {
    minRTT=5000000.;
    maxRTT=-12;
    sumRTT=0.;
    numReceived = 0;
    numTransmitted = 0;
}



int Pinger::parse_opts(int argc, const char* const *argv) {
	
    const char *destName = 0;
    char *end_ptr;
    double interval;
	
    // default settings:
    this->period = 1.0;
    this->pingSize = 200;
    this->ttl=64;
    this->numRepeat = -1;
    this->doTracePath = false;
    
    for (int i=1; i<argc; ++i) {
	
	if (!strcmp(argv[i], "-p")) {
	    this->doTracePath = true;
	    continue;
	} 
	
	if (!strcmp(argv[i],"-s")) {
	    ++i;
	    if (i == argc) // no following argument!
		return -1;
	    pingSize = strtol(argv[i], &end_ptr, 0);
	    if (*end_ptr || argv[i] == end_ptr) // invalid conversation
		return -1;
	    continue;
	} 
	
	if (!strcmp(argv[i],"-i")) {
	    ++i;
	    if (i == argc) // no following argument!
		return -1;
	    interval = strtod(argv[i], &end_ptr);
	    if (*end_ptr || argv[i] == end_ptr) // invalid conversation
		return -1;
	    this->period = interval;
	    continue;
	} 
	
	if (!strcmp(argv[i],"-t")) {
	    ++i;
	    if (i == argc) // no following argument!
		return -1;
	    ttl = strtol(argv[i], &end_ptr,0 );
	    if (*end_ptr || argv[i] == end_ptr) // invalid conversation
		return -1;
	    continue;
	} 
	
	// must be the destination name...
	{
	    if (destName) // destination was already given.
		return -1;
	    destName = argv[i];
	}
	    
    } // end for
	
    if (!destName) 
	return -1;
	
    //    REP_MAP_OBJ(RTopology *, topology);
    dbg() << " dest is " << destName << endl;
    return awdsRouting->getNodeByName(this->dest, destName);
    
} // end parse_opts



static void toArray(const gea::Duration& dur, char *buf) {
    
    unsigned long long d = (unsigned long long)( (double)dur * (double)0x1000000000ULL );
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
    return gea::Duration( (double)d  / (double)0x1000000000ULL );
}

static const size_t PingTraceNum  = UnicastPacket::UnicastPacketEnd;
static const size_t PingDirection = PingTraceNum + 2;
static const size_t PingTimeStamp = PingDirection + 1;
static const size_t PingHeaderEnd = PingTimeStamp + 8;

void Pinger::next_ping(gea::Handle *h, gea::AbsTime t, void *data) {
    
    Pinger *self = static_cast<Pinger *>(data);
    
    if (h->status == gea::Handle::Ready) {
	// key pressed in interactive mode!
	self->shellClient->unblock();
	self->busy = false;
	self->print_stats( self->dbg() );
	return;
    }
    
    
    BasePacket *p = self->awdsRouting->newUnicastPacket(PACKET_TYPE_UC_PING);
    p->setDest(self->dest);
    
    TraceUcPacket uniP(*p);
   
    uniP.setUcDest(self->dest);
    uniP.setTTL(self->ttl);
    self->pingSize = max(self->pingSize,  PingHeaderEnd);
	
    p->size = self->pingSize;
    p->buffer[PingDirection] = 'i';
    toArray( t - self->myT0, &p->buffer[PingTimeStamp] );
    
    if (self->doTracePath) {
	uniP.setTracePointer(PingHeaderEnd);
	uniP.setTraceFlag(1);
    }    

    self->dbg() << "sending ping to " << self->dest 
	       << std::endl;
    
    self->awdsRouting->sendUnicast(p, t);
    p->unref();
    
    self->numTransmitted++;
    
    GEA.waitFor( h, 
		 t + self->period, 
		 next_ping,
		 data);
    
}

void Pinger::print_stats(std::ostream& out) {
    out << "--- " << dest << " ping statistics ---" << endl
	<< numTransmitted << "  packets transmitted, " << numReceived << " received, " 
	<< ( 100. * double(numTransmitted - numReceived) / numTransmitted) << "% packet loss" << endl
	<< "rtt min/avg/max = " << minRTT << "/" << (sumRTT / numReceived) << "/" << maxRTT
	<< endl;
}


void Pinger::ping_recv(BasePacket *p, gea::AbsTime t, void *data) {
    
    Pinger *self = static_cast<Pinger *>(data);
    
    TraceUcPacket uniP(*p);
    
    if ( p->buffer[PingDirection] == 'i' ) {
	
	GEA.dbg() << "received ping from " << uniP.getSrc() 
		  << " ttl=" << uniP.getTTL()
		  << std::endl;
	
	uniP.setUcDest(uniP.getSrc());
	uniP.setSrc(self->awdsRouting->myNodeId);
	p->buffer[PingDirection] = 'o';
	p->ref();
	self->awdsRouting->sendUnicast(p, t);
	p->unref();
    } else {
	Duration timestamp = durationFromArray( &p->buffer[PingTimeStamp]);
	double deltaT= (double)(t - ( self->myT0 + timestamp ) );
	ostream& out = self->dbg();
	
	out << "received pong from " << uniP.getSrc() 
	    << " dur=" <<  deltaT<< " sec."
	    << " ttl=" << uniP.getTTL();
	    

	if (uniP.getTraceFlag()) {

	    const size_t tPtr = uniP.getTracePointer();

	    out << "\tpath[" << ( (tPtr - PingHeaderEnd) / NodeId::size ) <<"]";
	    for ( size_t nPtr = PingHeaderEnd;
		  nPtr < tPtr;
		  nPtr += NodeId::size) {
		
		NodeId node;
		node.fromArray(p->buffer + nPtr);
		out << " " << node;
	    }
		
	    
	}
	out << endl;
	
	self->minRTT = min(self->minRTT, deltaT);
	self->maxRTT = max(self->maxRTT, deltaT);
	self->sumRTT += deltaT;
	self->numReceived++;
	
	//	self->print_stats(self->dbg());
	
    }

}


static int ping_command_fn(ShellClient &sc, void *data, int argc, char **argv) {
    Pinger *self = static_cast<Pinger*>(data);
    
    if (argc <= 1) {
	*sc.sockout << long_help << endl;
	return -1;
    }
    
    if (self->busy) {
	*sc.sockout << "ping already running" << endl;
	return 0;
    }
    
    
    
  

    int ret = self->parse_opts(argc,argv);
    if (ret != 0) {
	*sc.sockout << long_help << endl;
	return -1;
    }
    
    if ( self->busy ) {
	*sc.sockout << "pinger is already in use " << endl;
	return -1;
    }
    
    self->shellClient = &sc;
    self->out = sc.sockout;
    sc.block();
    
    ret = self->startPing(sc.sockin);

    return 0;
}

void add_shell(Pinger *pinger) {
    REP_MAP_OBJ(Shell *, shell);
    
    if (!shell) return;
    
    shell->add_command("ping", ping_command_fn, pinger, short_help, long_help);

}

int Pinger::startPing( Handle *h) {
    if (busy) 
	return -1;
    busy = true;
    if (h == 0) 
	h = &this->blocker;
    
    resetStats();
    dbg() << "pinging " << dest << endl;
    
    GEA.waitFor(h, gea::AbsTime::now() + 0.00001, next_ping, static_cast<void *>(this));
    //next_ping(h, gea::AbsTime::now(), static_cast<void *>(this));
    
    return 0;
}

extern "C"
#ifdef PIC
int gea_main(int argc, const char  * const * argv) 
#else
int pinger_gea_main(int argc, const char  * const *argv) 
#endif
    
{    
    Pinger *pinger;
    REP_MAP_OBJ(awds::Routing *, routing);
    
    if (!routing) {
	GEA.dbg() << "cannot find object 'routing' in repository" << std::endl; 
	return -1;
    }
    
    pinger = new Pinger(routing);

    add_shell(pinger);
    
    if (argc > 1) {
	pinger->parse_opts(argc, argv);
	pinger->startPing();
    }
    
    return 0;
}
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
