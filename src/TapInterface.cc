#include <crypto/CryptoUnit.h>
#include <awds/tapiface.h>

#include <gea/gea_main.h>

using namespace awds;
using namespace std;

TapInterface::TapInterface(Routing *routing) :
    routing(routing)
{}


bool TapInterface::init(const char *dev)
{
    if (createDevice(dev) != true) {
	return false;
    }

    setIfaceMTU(routing->getMTU());

    tapHandle = new gea::UnixFdHandle(this->fd, gea::PosixModeRead);

    GEA.waitFor(tapHandle, GEA.lastEventTime + gea::Duration(10,1),
		tap_recv, (void *)this);

    routing->registerUnicastProtocol(PROTO_NR, recv_unicast, (void *)this);
    routing->registerBroadcastProtocol(PROTO_NR, recv_broadcast, (void *)this);
    
    return true;
}

bool TapInterface::setIfaceMTU(int mtu) {

    struct ifreq ifr;
    int fd_, err;

    strncpy(ifr.ifr_name, devname, IFNAMSIZ);

    fd_ = socket(PF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
	return false;

    ifr.ifr_mtu = mtu;
    err = ioctl(fd_, SIOCSIFMTU, (void *)&ifr);

    close(fd_);
    return err == 0;
}


bool TapInterface::createDevice(const char *dev) {

    struct ifreq ifr;
    int  err;

    if( (fd = open("/dev/net/tun", O_RDWR)) < 0 ) {
	if( (fd = open("/dev/tun", O_RDWR)) < 0 ) {
	    perror("open(\"/dev/tun\")");
	    return false;
	}
    }

    memset(&ifr, 0, sizeof(ifr));

    /* Flags: IFF_TUN   - TUN device (no Ethernet headers)
     *        IFF_TAP   - TAP device
     *
     *        IFF_NO_PI - Do not provide packet information
     */
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if(*dev)
	strncpy(ifr.ifr_name, dev, IFNAMSIZ);

    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
	perror("ioctl(TUNSETIFF)");
	close(fd);
	return false;
    }

    strncpy(devname, ifr.ifr_name, IFNAMSIZ);

    return true;
}

void TapInterface::tap_recv(gea::Handle *h, gea::AbsTime t, void *data) {

    TapInterface *self = static_cast<TapInterface *>(data);

    if (h->status == gea::Handle::Ready ) {

	NodeId destNode;

	char buf[3000];
	int ret = h->read(buf,3000);
	assert(ret >= 0);

	bool broadcast;

	broadcast = !self->getNodeForMacAddress(buf, destNode, t);

//	GEA.dbg() << "data on tap size="
//		  << ret
//		  << " broadcast=" << broadcast
//		  << " destNode=" << destNode
//		  << std::endl;

	BasePacket *p;

	if (broadcast) { // shall we broadcast this packet?

	    p = self->routing->newFloodPacket(PROTO_NR);
	    memcpy(&p->buffer[Flood::FloodHeaderEnd], buf, ret);
	    p->size = Flood::FloodHeaderEnd + ret;

	    // evtl. encryption
	    CryptoUnit *cu = self->routing->cryptoUnit;
	    if (cu) {
		CryptoUnit::MemoryBlock extraSign[3] = { { p->buffer, 3 + NodeId::size},
						      { p->buffer + Flood::OffsetFloodType, 1},
						      { 0 , 0}};
		cu->encrypt( p->buffer + Flood::FloodHeaderEnd,
			     ret,
			     extraSign );
		p->size += 32;
	    }

	    p->setSendCallback(tap_sendcb, data);

	    self->routing->sendBroadcast(p);

//	    GEA.dbg() << " sending tap data with packet size " << p->size
//		      << std::endl;

	} else {

	    p = self->routing->newUnicastPacket(PROTO_NR);
	    UnicastPacket(*p).setUcDest(destNode);
	    memcpy(&p->buffer[UnicastPacket::UnicastPacketEnd], buf, ret);
	    p->size = UnicastPacket::UnicastPacketEnd + ret;

	    // evtl. encryption
	    CryptoUnit *cu = self->routing->cryptoUnit;
	    if (cu) {
		CryptoUnit::MemoryBlock extraSign[3] = { { p->buffer, 3 + 2 * NodeId::size},
						      { p->buffer + UnicastPacket::OffsetUcType, 1},
						      { 0 , 0}};
		cu->encrypt( p->buffer + UnicastPacket::UnicastPacketEnd,
			     ret,
			     extraSign);

		p->size += 32;
	    }

	    p->setSendCallback(tap_sendcb, data);

	    self->routing->sendUnicast(p);

	}

	p->unref();


    } else {
	GEA.waitFor(h, t + gea::Duration(10.), tap_recv, data);
    }

}

void TapInterface::tap_sendcb(BasePacket &p, void *data, ssize_t len) {
    TapInterface *self = static_cast<TapInterface *>(data);

#if 0
    if (len <= 0) {
    	GEA.dbg() << "send of tapi packet failed!" << std::endl;
    }
#endif
    /* allow receiving more packets from tapi handle */
    GEA.waitFor(self->tapHandle, GEA.lastEventTime + gea::Duration(10,1), tap_recv, data);
}


void TapInterface::recv_unicast ( BasePacket *p,  void *data) {

    // UnicastPacket uinP(*p);
    TapInterface *self = static_cast<TapInterface *>(data);
    SrcPacket srcP(*p);
    NodeId srcId = srcP.getSrc();

    // evtl. encryption
    CryptoUnit *cu = self->routing->cryptoUnit;
    if (cu) {
	CryptoUnit::MemoryBlock extraSign[3] = { { p->buffer, 3 + 2 * NodeId::size},
						 { p->buffer + UnicastPacket::OffsetUcType, 1},
						 { 0 , 0}};
	if (! cu->decryptDupDetect( srcId,  p->buffer + UnicastPacket::UnicastPacketEnd,
				    p->size - 32 - UnicastPacket::UnicastPacketEnd,
				    extraSign))
	    {
		GEA.dbg() << "decrypt of UC packet from " << srcId << " failed" << std::endl;
		return;

	    }
	p->size -= 32;
    }


    write(self->fd,
	  &p->buffer[UnicastPacket::UnicastPacketEnd],
	  p->size - UnicastPacket::UnicastPacketEnd );


    self->storeSrcAndMac(srcP.getSrc(),
			 &p->buffer[UnicastPacket::UnicastPacketEnd],
			 GEA.lastEventTime );

}

void TapInterface::recv_broadcast ( BasePacket *p, void *data) {

    Flood flood(*p);
    SrcPacket srcP(*p);
    NodeId srcId = srcP.getSrc();

    TapInterface *self = static_cast<TapInterface *>(data);

    if ( srcId == self->routing->myNodeId )
	return;

    // evtl. encryption
    CryptoUnit *cu = self->routing->cryptoUnit;
    if (cu) {
	CryptoUnit::MemoryBlock extraSign[3] = { { p->buffer, 3 + NodeId::size},
						 { p->buffer + Flood::OffsetFloodType, 1},
						 { 0 , 0}};
	if (! cu->decryptDupDetect( srcId,  p->buffer + Flood::FloodHeaderEnd,
				    p->size - 32 - Flood::FloodHeaderEnd,
				    extraSign ) )
	    {
		GEA.dbg() << "decrypt of BC packet from " << srcId << " failed" << std::endl;
		return;
	    }
	p->size -= 32;
    }

    //     GEA.dbg() << "received broadcast from " << flood.getSrc()
    //	      << " size=" << (p->size /* -  Flood::FloodHeaderEnd */ )
    //	      << std::endl;

    write(self->fd,
	  &p->buffer[Flood::FloodHeaderEnd],
	  p->size - Flood::FloodHeaderEnd);

    self->storeSrcAndMac( srcP.getSrc(),
			  &p->buffer[Flood::FloodHeaderEnd],
			  GEA.lastEventTime);
}

bool  TapInterface::getNodeForMacAddress(const char* mac, NodeId& id, gea::AbsTime t) {
    NodeId m;
    m.fromArray(mac);

    MacTable::const_iterator itr = macTable.find(m);
    if ( (itr == macTable.end())  // not found
	 || (itr->second.validity < t)  // entry too old
	 )
	{
	    GEA.dbg() << "cannot find " << m
		      << " sending as broadcast" <<std::endl;
	    return false;
	}
    id = itr->second.id;
    return true;
}


void TapInterface::storeSrcAndMac(const NodeId &id, const char *buf, gea::AbsTime t) {

    NodeId m;
    m.fromArray(buf+6); // read src mac.

    MacTable::iterator itr = macTable.find(m);

    if ( itr == macTable.end() ) {
	GEA.dbg() << "adding " << m << "@" << id << " to the mac table" << std::endl;
    }

    MacEntry& macEntry = macTable[m];
    macEntry.id = id;
    macEntry.validity = t + gea::Duration(30.);
}

GEA_MAIN_2(tapiface, argc, argv) {

    const char *tapiface_name = "awds%d";

    for (int i(0);i<argc;++i) {
	    std::string w(argv[i]);
        if (w == "--help") {
            GEA.dbg() << "tapiface\t: please specify the tap interface name in the following format (default: "
                      << tapiface_name << ")" << endl
                      << "tapiface\t: "<< argv[0] << " --tapiface-name <name>" << endl;
            return -1;
        }
    }

    ObjRepository& rep = ObjRepository::instance();

    Routing *routing = (Routing *)rep.getObj("awdsRouting");
    if (!routing) {
	GEA.dbg() << "cannot find object 'awdsRouting' in repository" << std::endl;
	return -1;
    }

    /* parse options */

    int idx = 1;
    while (idx < argc) {

	if (!strcmp(argv[idx],"--tapiface-name") && (idx+1 <= argc)) {
	    ++idx;
	    tapiface_name = argv[idx];
	}

	++idx;
    }

    /* end of parsing options */

    TapInterface *tap = new TapInterface(routing);
    if (! tap->init(tapiface_name) ) {
	GEA.dbg() << "error while initializing a tap interface with name '" << tapiface_name << "'" << endl;
	return 1;
    }

    rep.insertObj("tap", "TapInterface", (void*)tap);

    GEA.dbg() << "created device " << tap->devname << std::endl;

    return 0;
}


/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
