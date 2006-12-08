#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <gea/ObjRepository.h>
#include <gea/API.h>
#include <gea/posix/UnixFdHandle.h>
#include <gea/Time.h>

#include <map>

using namespace gea;
using namespace std;

/** 
 * this module provides an interactive command shell
 * to be used over a TCP connection.
 */
class Shell;

class ShellClient {
    enum ClientState {
	CS_Idle,
	CS_Working,
	CS_Finish
    };

    Shell *parent;
    enum ClientState state;
    int fd;
    UnixFdHandle *fdHandle;
    struct sockaddr_in peer_addr;
#define CMD_BUF_MAX 1024
    int cmd_buf_len;
    char cmd_buf[CMD_BUF_MAX];
    
public:
    ShellClient(Shell *pa, int fd, UnixFdHandle *fdHandle, struct sockaddr_in peer) : 
	parent(pa),
	state(CS_Idle),
	fd(fd), 
	fdHandle(fdHandle),
	peer_addr(peer),
	cmd_buf_len(0)
    {}
    
    ShellClient() :
    	state(CS_Idle),
	cmd_buf_len(0)
    {}

    static void read_client_data(gea::Handle *h, gea::AbsTime t, void *data);
    bool parse_command(char *buf, int len);
};

class Shell {

public:    
    static const unsigned short PORT = 8444;
    int l_socket;
    gea::UnixFdHandle *lHandle;
    map<int, ShellClient> clients;

    Shell()
    {
	createSocket();
	lHandle = new UnixFdHandle(l_socket, ShadowHandle::Read);
	
	GEA.waitFor(lHandle, AbsTime::now() + Duration(12.),
		    accept_connection, (void *)this);
	
    }

    bool  createSocket();
    
    void add_command(char *name, void *command, void *data);

    static void accept_connection(gea::Handle *h, gea::AbsTime t, void *data);
    
};


bool  Shell::createSocket() {
	
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
    

void Shell::accept_connection(gea::Handle *h, gea::AbsTime t, void *data) {
    
    Shell * self = static_cast<Shell *>(data);
    
    if (h->status == Handle::Ready) {

	struct sockaddr_in peer_addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	
	int client_fd = accept(self->l_socket, (struct sockaddr *)&peer_addr, &addr_len);
	
	GEA.dbg() << "new shell:" << inet_ntoa(peer_addr.sin_addr) 
		  << ":" << ntohs(peer_addr.sin_port)
		  << std::endl;
	
	//	write(client_fd, "ja\n", 4); 
	
	// close(client_fd);
	UnixFdHandle *fdHandle = new UnixFdHandle(client_fd, ShadowHandle::Read);
	self->clients[client_fd] = ShellClient(self, client_fd, fdHandle, peer_addr);
	GEA.waitFor(fdHandle, t + Duration(120.), ShellClient::read_client_data, &self->clients[client_fd]);
    }
    
    GEA.waitFor(h, t + Duration(12.3), accept_connection, data);
    
}

bool ShellClient::parse_command(char *buf, int len) {
    if (len == 0 ||
	len > CMD_BUF_MAX ||
	NULL != memchr(buf, 0, len)) {
	    GEA.dbg() << "Illegal input from " << inet_ntoa(peer_addr.sin_addr) << ":" << ntohs(peer_addr.sin_port) << endl;
	    return false;
    }

    return true;
}


void ShellClient::read_client_data(gea::Handle *h, gea::AbsTime t, void *data) {
    
    if (h->status == Handle::Ready) {
	ShellClient *self = static_cast<ShellClient *>(data);

	int ret;
	ret = h->read(&self->cmd_buf[self->cmd_buf_len], CMD_BUF_MAX-self->cmd_buf_len);
	if (ret <= 0) {
	    
	    GEA.dbg() << "closing shell connection "  << inet_ntoa(self->peer_addr.sin_addr) << ":" << ntohs(self->peer_addr.sin_port) << endl;
	    delete self->fdHandle;
	    close (self->fd);
	    
	    if (self->state == CS_Working) {
		// unregister callback
	    	assert(!"Shell connection died in progress");
	    }
	    
	    //XXX:
	    self->parent->clients.erase(self->fd);
	    return;
	}
	self->cmd_buf_len += ret;
	if (self->cmd_buf_len >= CMD_BUF_MAX)
		self->cmd_buf_len = CMD_BUF_MAX - 1;

	int nlpos;
	while (memchr(self->cmd_buf, '\n', self->cmd_buf_len) != NULL) {
	    // we found at least one newline, lets parse the code
	    nlpos = (char*)memchr(self->cmd_buf, '\n', self->cmd_buf_len) - self->cmd_buf;
	    self->cmd_buf[nlpos] = '\0';
	    if (nlpos > 0) {
		//XXX parse_command(cmd_buf, nlpos);
		h->write(self->cmd_buf, nlpos);
	    }
	    // now lets shift the remaining input to the buffer start
	    memmove(self->cmd_buf, &self->cmd_buf[nlpos+1], self->cmd_buf_len - nlpos - 1);
	    self->cmd_buf_len -= 1 + nlpos;
	    assert(self->cmd_buf_len >= 0);
	}
    } else {
	// XXX: timeout handling
    }
    
    GEA.waitFor(h, t + Duration(120.), read_client_data, data); 
}


extern "C" 
int gea_main(int argc, const char  * const *argv) {
    

    ObjRepository& rep = ObjRepository::instance();
    //RTopology *topology = (RTopology *)rep.getObj("topology");
    //if (!topology) {
    //    GEA.dbg() << "cannot find object 'topology' in repository" << endl; 
    //    return -1;
    //}
    

    Shell *sh = new Shell();

    rep.insertObj("shell", "Shell", (void*)sh);
  
    return 0;
}


/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
