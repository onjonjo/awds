
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <cstring>

#include <gea/gea_main.h>
#include <gea/ObjRepository.h>
#include <gea/UdpHandle.h>
#include <gea/API.h>

#include <awds/basic.h>
#include <awds/SendQueue.h>

using namespace awds;

#define PORT 3334

/** \brief Implementation of basic communication mechanisms ontop of UDP datagrams.
 */
struct UdpBasic : public basic {

    SendQueue* sendq;

    void init(const NodeId& myId) {

        MyId = myId;
	BroadcastId = NodeId(0xFFFFFFFFUL);
	sendHandle =  new gea::UdpHandle( gea::UdpHandle::Write,
					  gea::UdpAddress(gea::UdpAddress::IPADDR_BROADCAST,
							  PORT /*port*/) );

	recvHandle = new gea::UdpHandle(gea::UdpHandle::Read,
					gea::UdpAddress(gea::UdpAddress::IPADDR_ANY,
							PORT /*port*/));

        sendq = new SendQueue(this, sendHandle);
    }

    virtual void setSendDest(const NodeId& id);
    virtual void getRecvSrc(NodeId& id);


    virtual ~UdpBasic() {
	delete sendHandle;
	delete recvHandle;
        delete sendq;
    }

    virtual bool send(BasePacket *p, bool high_prio) {
        // add packet to SendQueue, instead of sending it directly
        return sendq->enqueuePacket(p, high_prio);
    }
};


void UdpBasic::setSendDest(const NodeId& id) {

    if (id == BroadcastId) {

	((gea::UdpHandle *)sendHandle)->setDest(gea::UdpAddress(gea::UdpAddress::IPADDR_BROADCAST,
								PORT ) );

    } else {

	gea::UdpAddress dest( (u_int32_t)(unsigned long)id, (u_int16_t)PORT);

	((gea::UdpHandle *)sendHandle)->setDest(dest);
    }
}


void UdpBasic::getRecvSrc(NodeId& id) {

}

GEA_MAIN(argc,argv)
{


    NodeId MyId;
    if (argc <= 1) {
	MyId = NodeId(gea::UdpHandle::getIP());
    } else {

	MyId = NodeId( inet_addr(argv[1]) );
    }

    UdpBasic *basic = new UdpBasic();

    basic->init(MyId);
    basic->start();
    
    REP_INSERT_OBJ(awds::basic *, basic, basic);

    GEA.dbg() << "running UDP basic on " << MyId << std::endl;
    
    return 0;
}



/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
