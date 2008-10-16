#include <sys/types.h>
#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <arpa/inet.h>

#include <stdlib.h>
#include <string>
#include <iostream>
#include <cstring>

#include <gea/gea_main.h>
#include <gea/ObjRepository.h>
#include <gea/posix/UnixFdHandle.h>
#include <gea/API.h>

#include <awds/basic.h>
#include <awds/SendQueue.h>

#include <awds/ext/Shell.h>

#include <awds/linuxbasicCallback.h>

#define ETHERTYPE_AWDS 0x8334

#define PF_AWDS 32

/* we use 'k' as our ioctl magic number */
#define AWDS_IOC_MAGIC 'k'

/* set routing table */
#define AIOCSRTTBL _IOW(AWDS_IOC_MAGIC, 0, char *)
/* set configuration number */
#define AIOCSCFGNR _IOW(AWDS_IOC_MAGIC, 1, int *)
/* get configuration number */
#define AIOCGCFGNR _IOR(AWDS_IOC_MAGIC, 2, int *)

struct sockaddr_awds {
    sa_family_t	sa_family;	          /* address family, PF_AWDS      */
    int         sa_ifindex;               /* the network interface index  */
    char        sa_addr[ETH_ALEN];        /* the network interface index  */
    char	sa_data[10 - ETH_ALEN];	  /* additional information   	  */
};

/** \defgroup linuxbasic_mod
 *  \brief Basic I/O via raw Ethernet frames.
 *
 *  Use this basic module to use the AWDS Socket type for transport of AWDS packets.
 */

using namespace awds;
using namespace std;

class LinuxBasic;

class LinuxHandle : public gea::UnixFdHandle {

public:
    LinuxBasic&  basic;

    LinuxHandle( LinuxBasic& basic, bool readMode);

    virtual int write(const char *buf, int size);
    virtual int read (char *buf, int size);

};

/** \brief Implementation of basic communication mechanisms on top of Ethernet raw socket.
 *  \ingroup linuxbasic_mod
 */
class LinuxBasic : public basic {

public:

    int raw_socket;
    int ifindex;

    sockaddr_awds addr;
    unsigned int addr_len;

    char devicename[IFNAMSIZ+1];

    struct ether_header recv_header;
    struct ether_header send_header;

    NodeId dest;
    NodeId src;

    SendQueue* sendq;

    static void UpdateTopology(void* Basic, const char* TopoDump) {

        if(!Basic)
            return;

        LinuxBasic* LBasic = static_cast<LinuxBasic*>(Basic);
        int fd = LBasic->raw_socket;

	ioctl(fd, AIOCSRTTBL, TopoDump);
    }

    LinuxBasic(const char *dev)
    {
	    strcpy(devicename, dev);

	    if ( !createSocket() )
	        raw_socket = -1;

	    static const unsigned char broadcastAddr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	    BroadcastId.fromArray((char*)broadcastAddr);

	    sendHandle = new LinuxHandle(*this, false);
	    recvHandle = new LinuxHandle(*this, true);
	    sendq = new SendQueue(this, sendHandle);

        linuxbasic_cb* cb = new linuxbasic_cb;

        cb->Basic = this;
        cb->Func = &UpdateTopology;

        ObjRepository::instance().insertObj("linuxbasic_cb", "linuxbasic_cb *", cb);
    }

    virtual bool send(BasePacket *p, bool high_prio) {
	    // add packet to SendQueue, instead of sending it directly
	    return sendq->enqueuePacket(p, high_prio);
    }

public:

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

	    raw_socket = socket(PF_AWDS, SOCK_STREAM, 0);

	    if ( raw_socket < 0)
	        return false;

	    strncpy(req.ifr_name, devicename, IFNAMSIZ);
	    ret = ioctl(raw_socket, SIOCGIFINDEX, &req);
	    if (ret != 0)
	        return false;

	    //	GEA.dbg() << "device " << devicename << " has index " << req.ifr_ifindex << std::endl;
	    getHwAddress();

        addr.sa_family = PF_AWDS;
        addr.sa_ifindex = req.ifr_ifindex;
        MyId.toArray(addr.sa_addr);


	    if (::bind(raw_socket,
		       reinterpret_cast<sockaddr*>(&addr),
		       sizeof(addr)) == -1) {
	        close(raw_socket);
	        return false;
	    }

	    return true;
    }

    short int getModuleConfig()
    {
        short int level = 0;

        if(raw_socket<0) return -1;
        if(ioctl(raw_socket, AIOCGCFGNR, (short int*)&level)) {
          level = -1;
        }
        return level;
    }

    void setModuleConfig(short int level)
    {
        if(raw_socket<0) return;
        ioctl(raw_socket, AIOCSCFGNR, (short int*)&level);
    }

    virtual ~LinuxBasic();
    virtual void setSendDest(const NodeId&);
    virtual void getRecvSrc(NodeId&);

};



LinuxHandle::LinuxHandle( LinuxBasic& basic, bool readMode) :
    UnixFdHandle( basic.raw_socket,
		  readMode ? gea::PosixModeRead : gea::PosixModeWrite ),
    basic(basic)
{

}

int LinuxHandle::write(const char *buf, int size) {

    basic.dest.toArray((char*)basic.addr.sa_addr);

    int ret = sendto(basic.raw_socket, buf, size, 0, (struct sockaddr*)&basic.addr, sizeof(basic.addr));

    return ret;
}

int LinuxHandle::read(char *buf, int size) {

	basic.MyId.toArray((char*)basic.addr.sa_addr);

    socklen_t addr_size = sizeof(basic.addr);

    int ret = recvfrom(basic.raw_socket, buf, size, 0, (struct sockaddr*)&basic.addr, &addr_size);

    return ret;
}

LinuxBasic::~LinuxBasic() {
    delete sendHandle;
    delete recvHandle;
    delete sendq;
}

void LinuxBasic::setSendDest(const NodeId& d) {
    dest = d;
}

void LinuxBasic::getRecvSrc(NodeId& s) {
    s = src;
}

static const char *moduleconfig_cmd_usage =
    "moduleconfig [3 | 2 | 1 | 0]\n"
    "  3: enable both configuration options\n"
    "  2: enable forwarding only\n"
    "  1: enable optimized local delivery only\n"
    "  0: enable nothing\n";

static int moduleconfig_command_fn(ShellClient &sc, void *data, int argc, char **argv) {
    LinuxBasic *self = static_cast<LinuxBasic*>(data);

    if (argc == 1)
	*sc.sockout << "moduleconfig: " << self->getModuleConfig() << std::endl;
    else if (argc == 2) {
        short int level = atoi(argv[1]);

        if((level<0) || (level>3)) {
		    *sc.sockout << moduleconfig_cmd_usage;
		    return -1;
        }
        else {
            self->setModuleConfig(level);
        }
    } else {
        *sc.sockout << moduleconfig_cmd_usage;
	return -1;
    }
    return 0;
}

#define MODULE_NAME linuxbasic

GEA_MAIN_2(linuxbasic, argc, argv) {

    LinuxBasic *basic;
    const  char *netif = 0;
    
    /* parse the options */ 
    
    int idx = 1;
    
    while (idx < argc) {
	
	if (!strcmp(argv[idx],"--raw-device") && (idx+1 <= argc)) {
	    ++idx;
	    netif = argv[idx];
	}
    else if(!strcmp(argv[idx],"--help")) {
    	GEA.dbg() << "linuxbasic\t: please specify the network device to use for communication" << endl
        		  << "linuxbasic\t: "<< argv[0] << " --raw-device <dev>" << endl;        
        return -1;
	}
	
	++idx;
    }
    
    if (!netif) {
    	GEA.dbg() << "linuxbasic\t: please specify the network device to use for communication" << endl
        		  << "linuxbasic\t: "<< argv[0] << " --raw-device <dev>" << endl;        
        return -1;
    }

    /* end of parsing options */

    basic = new LinuxBasic(netif);

    if (basic->raw_socket == -1) {
	GEA.dbg() << argv[0] <<
	    ": cannot open AWDS socket interface on device " << netif << std::endl;
	return -1;
    }

    basic->start();
    //   basic->init(MyId);

    ObjRepository& rep = ObjRepository::instance();

    rep.insertObj("basic", "basic", (void*)basic);

    REP_MAP_OBJ(Shell *, shell);
    if (shell) {
        shell->add_command("moduleconfig", moduleconfig_command_fn, basic,
                "set/get awds kernel module configuration level", moduleconfig_cmd_usage);
    }

    GEA.dbg() << "running LINUX basic on " << netif << " address=" << basic->MyId << std::endl;

    return 0;
}

/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
