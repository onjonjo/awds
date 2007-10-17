#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <gea/gea_main.h>
#include <gea/ObjRepository.h>
#include <gea/API.h>
#include <gea/posix/UnixFdHandle.h>
#include <gea/Time.h>

#include <map>

#include <awds/Topology.h>

using namespace gea;
using namespace std;
using namespace awds;

/** 
 * this module provides a way of observing changes in the local topology.
 * Therfore, it creates a TCP server that outputs a description 
 * of the modifications. \
 */
class TopoWatch {

public:    
    static const unsigned short PORT = 8333;
    
    int l_socket;

    gea::UnixFdHandle *lHandle;

    enum ClientState {
	CS_Dot,
	CS_Adj,
	CS_Xml,
	CS_Undefined
    };

    struct ClientDescr {
	enum ClientState state;
	int fd;
	UnixFdHandle *fdHandle;
	
	ClientDescr(int fd, UnixFdHandle *fdHandle) : 
	    state(CS_Undefined),
	    fd(fd), 
	    fdHandle(fdHandle) 
	{}
	
	ClientDescr() {}
    };
    
    map<int, ClientDescr> clients;

    RTopology * topology;

    TopoWatch(RTopology *topology)
	: topology(topology)
    {
	
	createSocket();
	lHandle = new UnixFdHandle(l_socket, ShadowHandle::Read);
	
	GEA.waitFor(lHandle, AbsTime::now() + Duration(12.),
		    accept_connection, (void *)this);
	
    }

    bool  createSocket();
    

    static void accept_connection(gea::Handle *h, gea::AbsTime t, void *data);
    static void read_client_data(gea::Handle *h, gea::AbsTime t, void *data);
    static bool write_topo(void *,  std::string& );
    
};


bool TopoWatch::write_topo(void *data, std::string& s) {
    ClientDescr *d = (ClientDescr *)data;
    const char *str = s.c_str();
    int   len = s.size();
    int   ret;
    while (len) {
	ret = write(d->fd, str, len);
	assert( ret <= len );
	str += ret;
	len -= ret;
	
    }
    return true;
}

bool  TopoWatch::createSocket() {
	
    struct sockaddr_in addr;
    socklen_t socklen;
    int ret;

	
    l_socket = socket(PF_INET, SOCK_STREAM, 0);
	
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    socklen = sizeof( struct sockaddr_in);
	
    int one = 1;
    ret = setsockopt(l_socket, SOL_SOCKET,  SO_REUSEADDR, &one, sizeof(int) );
    if (ret != 0) {
	close(l_socket);
	return false;
    }

    ret = bind(l_socket, (struct sockaddr *)&addr, sizeof( struct sockaddr_in));
    if (ret != 0) {
	close(l_socket);
	return false;
    }
	
    ret = listen(l_socket, 4);
    if (ret != 0) {
	close(l_socket);
	return false;
    }
    
        
    return true;
	
}
    

void TopoWatch::accept_connection(gea::Handle *h, gea::AbsTime t, void *data) {
    
    TopoWatch * self = static_cast<TopoWatch *>(data);
    
    if (h->status == Handle::Ready) {

	struct sockaddr_in peer_addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	
	int client_fd = accept(self->l_socket, (struct sockaddr *)&peer_addr, &addr_len);
	
	GEA.dbg() << "new client:" << inet_ntoa(peer_addr.sin_addr) 
		  << ":" << ntohs(peer_addr.sin_port)
		  << std::endl;
	
	//	write(client_fd, "ja\n", 4); 
	
	// close(client_fd);
	UnixFdHandle *fdHandle = new UnixFdHandle(client_fd, ShadowHandle::Read);
	self->clients[client_fd] = ClientDescr(client_fd, fdHandle);
	GEA.waitFor(fdHandle, t + Duration(10.), read_client_data, data );
    }
    
    GEA.waitFor(h, t + Duration(12.3), accept_connection, data);
    
}



void TopoWatch::read_client_data(gea::Handle *h, gea::AbsTime t, void *data) {
    
    if (h->status == Handle::Ready) {
	
	TopoWatch *self = static_cast<TopoWatch *>(data);
	ClientDescr *descr = 0; 
	
	map<int, ClientDescr>::iterator itr;
	for (itr = self->clients.begin(); itr != self->clients.end(); ++itr) {
	    if (itr->second.fdHandle == h) // pointer comparison
		descr=&itr->second;
	}
	assert(descr);

	char buffer[1024];
	int ret;
	ret = h->read(buffer, 1024);
	if (ret <= 0) {
	    
	    GEA.dbg() << "closing connection to client" << endl;
	    delete descr->fdHandle;
	    close (descr->fd);
	    
	    if (descr->state == CS_Xml) {
		// unregister callback
		self->topology->newXmlTopologyDelta.remove(TopoWatch::write_topo, descr);
	    }
	    
	    self->clients.erase(descr->fd);
	    return;
	}
	
	switch (buffer[ret-1]) {
	case 'd': 
	    {
		std::string s = self->topology->getDotString();
		self->write_topo(descr, s);
	    }
	    descr->state = CS_Dot;
	    break;
	case 'a':
	    {
		std::string s = self->topology->getAdjString();
		self->write_topo(descr, s);
		
	    }
	    descr->state = CS_Adj;
	    break;
	case 'x':
	    {
		std::string  s = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n<graph>\n"; 
		s += self->topology->getXmlString();
		self->write_topo(descr, s);
		self->topology->newXmlTopologyDelta.add(TopoWatch::write_topo, descr );
	    }
	    descr->state = CS_Xml;
	    break;
	default:
	    descr->state = CS_Undefined;
	}
    }
    
    
    
    GEA.waitFor(h, t + Duration(10.), read_client_data, data); 
}


GEA_MAIN(argc, argv)
{
    

    ObjRepository& rep = ObjRepository::instance();
    RTopology *topology = (RTopology *)rep.getObj("topology");
    if (!topology) {
	GEA.dbg() << "cannot find object 'topology' in repository" << endl; 
	return -1;
    }
    

    new TopoWatch(topology);
  
    return 0;
}


/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
