#include <sys/types.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <arpa/inet.h>

#include <string>
#include <iostream>
#include <cstring>

#include <gea/gea_main.h>
#include <gea/ObjRepository.h>
#include <gea/posix/UnixFdHandle.h>
#include <gea/API.h>

#include <awds/basic.h>


#define ETHERTYPE_AWDS 0x8334


/** \defgroup rawbasic_mod
 *  \brief Basic I/O via raw Ethernet frames. 
 *  
 *  Use this basic module to use raw Ethernet frames for transport of AWDS packets. 
 */

using namespace awds;

class RawBasic;

class RawHandle : public gea::UnixFdHandle {
    
public:
    RawBasic&  basic;
    
    RawHandle( RawBasic& basic, bool readMode);
    
    virtual int write(const char *buf, int size);
    virtual int read (char *buf, int size);

};

/** \brief Implementation of basic communication mechanisms on top of Ethernet raw socket.
 *  \ingroup rawbasic_mod
 */
class RawBasic : public basic {

public:
    
    int raw_socket;
    int ifindex;
    
    sockaddr_ll addr;
    unsigned int addr_len;
    
    char devicename[IFNAMSIZ+1];
    
    struct ether_header recv_header;
    struct ether_header send_header;
    
    NodeId dest;
    NodeId src;
    
    RawBasic(const char *dev) { 
	strcpy(devicename, dev);
	
	if ( !createSocket() )
	    raw_socket = -1;
	getHwAddress();
	
	static const unsigned char broadcastAddr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	BroadcastId.fromArray((char*)broadcastAddr);
	
	sendHandle = new RawHandle(*this, false);
	recvHandle = new RawHandle(*this, true);
    }
    
    
    bool getHwAddress() {
	struct ifreq req;

	strncpy(req.ifr_name, devicename, IFNAMSIZ);
	ioctl(raw_socket, SIOCGIFHWADDR, &req);
	MyId.fromArray(req.ifr_hwaddr.sa_data);
	
	return true;
    }
    
    
    
    bool createSocket() {
	
	int ret;
	struct ifreq req;
	
	raw_socket = socket(PF_PACKET,SOCK_RAW,htons(ETHERTYPE_AWDS));
	
	if ( raw_socket < 0) 
	    return false;
	
	strncpy(req.ifr_name, devicename, IFNAMSIZ);
	ret = ioctl(raw_socket, SIOCGIFINDEX, &req);
	if (ret != 0) 
	    return false;
	
	//	GEA.dbg() << "device " << devicename << " has index " << req.ifr_ifindex << std::endl;
	
	addr.sll_family = AF_PACKET;
	addr.sll_protocol = htons(ETHERTYPE_AWDS);
	addr.sll_ifindex = ifindex = req.ifr_ifindex;
	
	
	if (::bind(raw_socket,
		   reinterpret_cast<sockaddr*>(&addr),
		   sizeof(addr)) == -1) {
	    close(raw_socket);
	    return false;
	}
	
	return true;
    }
    

    virtual ~RawBasic();
    virtual void setSendDest(const NodeId&);
    virtual void getRecvSrc(NodeId&);
    
};


    
RawHandle::RawHandle( RawBasic& basic, bool readMode) :
    UnixFdHandle( basic.raw_socket, 
		  readMode ? gea::PosixModeRead : gea::PosixModeWrite ),
    basic(basic)
{
    
}
		 
int RawHandle::write(const char *buf, int size) {
    struct iovec vector[2];
    vector[0].iov_base = &basic.send_header;
    vector[0].iov_len  = sizeof(struct ether_header);
    vector[1].iov_base = (void*)buf;
    vector[1].iov_len  = size;
    
    basic.MyId.toArray((char*)&basic.send_header.ether_shost);
    basic.dest.toArray((char*)&basic.send_header.ether_dhost);
    basic.send_header.ether_type = htons(ETHERTYPE_AWDS);
    
    return writev(basic.raw_socket, vector, 2);
}

int RawHandle::read(char *buf, int size) {
    struct iovec vector[2];
    vector[0].iov_base = &basic.recv_header;
    vector[0].iov_len  = sizeof(struct ether_header);
    vector[1].iov_base = buf;
    vector[1].iov_len  = size;
    
    int ret = readv(basic.raw_socket, vector, 2);
    basic.src.fromArray((char*)&basic.recv_header.ether_shost);
    
    if (basic.recv_header.ether_type != htons(ETHERTYPE_AWDS) ) 
	return -2;
    else 
	return ret - sizeof(struct ether_header);
    
}




RawBasic::~RawBasic() { 
    
}

void RawBasic::setSendDest(const NodeId& d) {
    dest = d;
}

void RawBasic::getRecvSrc(NodeId& s) {
    s = src;
}

GEA_MAIN_2(rawbasic, argc, argv) {
    
    RawBasic *basic;
    const  char *netif = "ath0";
    
    if (argc > 1) {
	netif = argv[1];
    } 
    basic = new RawBasic(netif);
    
    if (basic->raw_socket == -1) {
	GEA.dbg() << argv[0] << 
	    ": cannot open raw socket interface on device " << netif << std::endl;
	return -1;
    }
    
    //   basic->init(MyId);
    
    ObjRepository& rep = ObjRepository::instance(); 
    
    rep.insertObj("basic", "basic", (void*)basic);

    GEA.dbg() << "running RAW basic on " << basic->MyId << std::endl;
    
//     basic->setSendDest(basic->BroadcastId);
//     basic->sendHandle->write("Hallllllllllllllllllllo:  : ", 100);
    
    return 0;
}



/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
