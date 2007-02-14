
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <cstring>

#include <gea/ObjRepository.h>
#include <gea/UdpHandle.h>
#include <gea/API.h>

#include <awds/basic.h>

using namespace awds;

#define PORT 3334

struct UdpBasic : public basic {

    void init(const NodeId& myId) {
	
        MyId = myId;
	BroadcastId = NodeId(0xFFFFFFFFUL);
	sendHandle =  new gea::UdpHandle( gea::UdpHandle::Write,
					  gea::UdpAddress(PORT /*port*/,
							  gea::UdpAddress::IP_BROADCAST
							  /*ip*/ ));
	
	recvHandle = new gea::UdpHandle(gea::UdpHandle::Read,
					gea::UdpAddress(PORT /*port*/,
							gea::UdpAddress::IP_ANY /*ip*/ ));

    }

    virtual void setSendDest(const NodeId& id);
    virtual void getRecvSrc(NodeId& id);
    
  
    virtual ~UdpBasic() {
	delete sendHandle;
	delete recvHandle;
    }

    
};

// static unsigned long id2IPv4(const NodeId& id) {
//     char buf[16];
//     id.toArray(buf);
    
//     unsigned long ret = 0;
//     for (unsigned i= (int)NodeId::size - 4; i < NodeId::size  ; ++i) {
// 	ret <<= 8;
// 	ret |= (unsigned long)(unsigned char)(buf[i]);
//     }
//     return ret;
// }

// static void id2str(char *buf, const NodeId& id) {
//     unsigned long num = ntohl(id2IPv4(id));
//     sprintf(buf, "%lu", num);
// }

void UdpBasic::setSendDest(const NodeId& id) {
    
    if (id == BroadcastId) {
	
	((gea::UdpHandle *)sendHandle)->setDest(gea::UdpAddress(PORT, 
								gea::UdpAddress::IP_BROADCAST));
	
    } else {

	gea::UdpAddress dest( (u_int32_t)(unsigned long)id, (u_int16_t)PORT);

	((gea::UdpHandle *)sendHandle)->setDest(dest);
    }
}


void UdpBasic::getRecvSrc(NodeId& id) {
    
    
    
}


extern "C" 
int gea_main(int argc, const char  * const *argv) {

    
    NodeId MyId;
    if (argc <= 1) {
	MyId = NodeId(gea::UdpHandle::getIP());
    } else {
	
	MyId = NodeId( inet_addr(argv[1]) );
    }
  
    UdpBasic *basic = new UdpBasic();
    
    basic->init(MyId);
    
    //    ObjRepository& rep = ObjRepository::instance(); 
    
    //    rep.insertObj("awds::basic", "basic", basic);
    REP_INSERT_OBJ(awds::basic *, basic, basic);

    //     char str[123];
    //     id2str(str, MyId);
    //     GEA.dbg() << "test " << MyId << ": [" <<str << "] " 
    // 	      << gea::UdpHandle::getIP() << " vs " << id2IPv4(MyId) << std::endl;
    
    
    GEA.dbg() << "running UDP basic on " << MyId << std::endl;
    
    //basic->setSendDest(MyId);
    //    basic->sendHandle->write(str,2);
    return 0;
}



/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
