
#include <crypto/CryptoUnit.h>
#include <awds/tapiface.h>

using namespace awds;
 
TapInterface::TapInterface(Routing *routing) :
    routing(routing)
{}


void TapInterface::init(const char *dev)
{ 
    //struct ifreq ifr;
    //int fd_, err;
    	
    createDevice(dev);
	
    setIfaceHwAddress(routing->myNodeId);
    setIfaceMTU(routing->getMTU());
	
    tapHandle = new gea::UnixFdHandle(this->fd, gea::ShadowHandle::Read);
	
    GEA.waitFor(tapHandle, gea::AbsTime::now() + gea::Duration(10.),
		tap_recv, (void *)this);

    routing->registerUnicastProtocol(PROTO_NR, recv_unicast, (void *)this);
    routing->registerBroadcastProtocol(PROTO_NR, recv_broadcast, (void *)this);
}



bool TapInterface::setIfaceHwAddress(const NodeId& id) {
    
    struct ifreq ifr;
    int fd_, err;
	
    id.toArray((char*)&ifr.ifr_hwaddr.sa_data);
	
    ((char*)&ifr.ifr_hwaddr.sa_data)[0] ^= ADDR_TRNSLTR;
    ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;	
	
    strncpy(ifr.ifr_name, devname, IFNAMSIZ);

    GEA.dbg() << "setting hardware address" << std::endl;

    fd_ = socket(PF_INET, SOCK_DGRAM, 0);
    err = ioctl(fd_, SIOCSIFHWADDR, (void *)&ifr);
    close(fd_);
    return err == 0;
	
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
	//perror("open(\"/dev/net/tun\")");
	return false;
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
	// -- perror("ioctl(TUNSETIFF)");
	close(fd);
	return false;
    }
    
    strncpy(devname, ifr.ifr_name, IFNAMSIZ);
    
    return true;
}


bool TapInterface::getNodeForMacAddress(const char* mac, NodeId& id, gea::AbsTime t) {
    
    char dest_addr[6];
    memcpy(dest_addr, mac, 6);
    dest_addr[0] ^= ADDR_TRNSLTR;
    
    id.fromArray(dest_addr);

    return true;
}

void TapInterface::storeSrcAndMac(const NodeId &id, const char *bufO, gea::AbsTime t) {

    // dummy, do nothing in basic version.
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
	
// 	GEA.dbg() << "data on tap size=" 
// 		  << ret
// 		  << " broadcast=" << broadcast
// 		  << " destNode=" << destNode
// 		  << std::endl;	
	
	BasePacket *p;

	

	if (broadcast) {
	    
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
	    
	    
	    self->routing->sendBroadcast(p,t);

// 	    GEA.dbg() << " sending tap data with packet size " << p->size 
// 		      << std::endl;
	    
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
	    
	    self->routing->sendUnicast(p,t);
	    
	}
	
	p->unref();
	

    }
    
    GEA.waitFor(h, t + gea::Duration(10.), tap_recv, data);
    
}

void TapInterface::recv_unicast ( BasePacket *p, gea::AbsTime t, void *data) { 
    
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


    self->storeSrcAndMac(srcP.getSrc(), &p->buffer[UnicastPacket::UnicastPacketEnd], t);
    
}


void TapInterface::recv_broadcast ( BasePacket *p, gea::AbsTime t, void *data) {

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
    // 	      << " size=" << (p->size /* -  Flood::FloodHeaderEnd */ )
    // 	      << std::endl;
 
    write(self->fd, 
	  &p->buffer[Flood::FloodHeaderEnd],
	  p->size - Flood::FloodHeaderEnd);

    self->storeSrcAndMac(srcP.getSrc(), &p->buffer[Flood::FloodHeaderEnd], t);
}



/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
