#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <gea/gea_main.h>
#include <gea/ObjRepository.h>
#include <gea/API.h>
#include <gea/Time.h>

#include <errno.h>
#include <string.h>

#include <map>
#include <string>
#include <iostream>
#include <ext/stdio_filebuf.h>

#include <awds/ext/Shell.h>
#include "unixfdostream.h"

using namespace gea;
using namespace std;
using namespace awds;

class TcpShell;

class TcpShellClient : public ShellClient {
    int fd;
    struct sockaddr_in peer_addr;
#define CMD_BUF_MAX 1024
#define CMD_PARAM_MAX 128
#define CMD_IFS " \t\r\n"
    int cmd_buf_len;
    char cmd_buf[CMD_BUF_MAX];
    
    TcpShell *shell;
    
protected:
    friend class TcpShell;
    void prompt(bool wait);

public:

    TcpShellClient(TcpShell *pa, int fd, gea::UnixFdHandle *_sockin, ostream *_sockout,
   		struct sockaddr_in peer) : 
    	ShellClient(_sockin, _sockout),
	fd(fd), 
	peer_addr(peer),
	cmd_buf_len(0),
        shell(pa)
    {
    }
    
    TcpShellClient() :
	cmd_buf_len(0)
    {}

    virtual int exec(int argc, char **argv);
    static void read_client_data(gea::Handle *h, gea::AbsTime t, void *data);
    bool parse_command(char *buf, int len);

    void block();
    void unblock();
};

class TcpShell : public Shell {
    static const unsigned short PORT = 8444;
    int l_socket;
    gea::UnixFdHandle *lHandle;

    map<string, ShellCommand> commands;

    bool  createSocket();
    static void accept_connection(gea::Handle *h, gea::AbsTime t, void *data);

    static int help(ShellClient &sc, void *data, int argc, char **argv);
    static int watch(ShellClient &sc, void *data, int argc, char **argv);

public:    
    map<int, TcpShellClient> clients;

    TcpShell();

    
    void add_command(const string name, shell_command_fn *command,
   		void *data, const char *descr, const char *help);
    ShellCommand *get_command(string name);
    
};


#define HELP \
"Lorem  ipsum  dolor  sit  amet,  consectetur  adipisicing  elit,  sed do\n" \
"eiusmod  tempor incididunt ut labore et dolore magna aliqua.  Ut enim ad\n" \
"minim veniam,  quis nostrud exercitation ullamco laboris nisi ut aliquip\n" \
"ex  ea  commodo  consequat.  Duis  aute  irure dolor in reprehenderit in\n" \
"voluptate  velit esse cillum dolore eu fugiat nulla pariatur.  Excepteur\n" \
"sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt\n" \
"mollit anim id est laborum.\n"

TcpShell::TcpShell()
{
    bool can_start = createSocket();
    if (!can_start) {
	GEA.dbg() << "cannot create TCP socket on port " << PORT << endl;
	return;
    }
    
    lHandle = new UnixFdHandle(l_socket, ShadowHandle::Read);
    
    add_command("help", help, this, "print the help for a command", HELP);
    //    add_command("watch", watch, this, "repeat the execution of a program", NULL);
    GEA.dbg() << "TcpShell listening..." << endl;
    GEA.waitFor(lHandle, AbsTime::now() + Duration(12.),
		accept_connection, (void *)this);
    
}

bool  TcpShell::createSocket() {
	
    struct sockaddr_in addr;
    socklen_t socklen;
    int ret;

	
    l_socket = socket(PF_INET, SOCK_STREAM, 0);
	
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
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

void TcpShell::add_command(const string name, shell_command_fn *command,
		void *data, const char *desc, const char *help) {
    commands[name].command = command;
    commands[name].data = data;
    commands[name].desc = desc;
    commands[name].help = help;
}

ShellCommand *TcpShell::get_command(string name) {

    map<string, ShellCommand>::iterator cmd = commands.find(name);
    if (cmd != commands.end())
   	 return &cmd->second;
    else return NULL;
}

int TcpShell::help(ShellClient &cl, void *data, int argc, char **argv) {
    TcpShell *self = static_cast<TcpShell*>(data);
    if (argc > 1) {
	ShellCommand *scmd = self->get_command(argv[1]);
	if (scmd != NULL) {
	    *cl.sockout << argv[1] << "\t- " << scmd->desc << endl;
	    if (scmd->help) {
		*cl.sockout << endl << scmd->help << endl;
	    }
	    return 0;
	} else {
	    *cl.sockout << "Unknown command '" << argv[1] << "'!" << endl;
	    return 1;
	}
    } else {
	map<string, ShellCommand>::iterator cmd;
	for (cmd = self->commands.begin(); cmd != self->commands.end(); cmd++) {
	    *cl.sockout << cmd->first << "\t- " << cmd->second.desc << endl;
	}
	return 0;
    }
}

int TcpShell::watch(ShellClient &cl, void *data, int argc, char **argv) {
    //    TcpShell *self = static_cast<TcpShell*>(data);
    return 0;
    if (argc > 1) {
        while (cl.state == ShellClient::CS_Idle) {
	    cl.exec(argc-1, &argv[1]);
	}
    } else {
    	*cl.sockout << "watch [command [args]]" << endl;
	return 1;
    }
    return 0;
}

void TcpShell::accept_connection(gea::Handle *h, gea::AbsTime t, void *data) {
    
    TcpShell * self = static_cast<TcpShell *>(data);
    
    if (h->status == Handle::Ready) {

	struct sockaddr_in peer_addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);
	
	int client_fd = accept(self->l_socket, (struct sockaddr *)&peer_addr, &addr_len);
	
	ostream *shellout = new ostream(new UnixFdStreamBuf(client_fd));
	*shellout << "Welcome to localhost!" << endl;
	shellout->flush();
	
	GEA.dbg() << "new shell:" << inet_ntoa(peer_addr.sin_addr) 
		  << ":" << ntohs(peer_addr.sin_port)
		  << std::endl;
	
	
	UnixFdHandle *fdHandle = new UnixFdHandle(client_fd, ShadowHandle::Read);
	self->clients[client_fd] = TcpShellClient(self, client_fd, fdHandle, shellout, peer_addr);
	//implicit:
	self->clients[client_fd].prompt(true);
    }
    
    GEA.waitFor(h, t + Duration(12.3), accept_connection, data);
    
}

int TcpShellClient::exec(int argc, char **argv) {
    if ((argc > 0) && argv[0] && *argv[0]) {
    	ShellCommand *sc = shell->get_command(argv[0]);
	if (!sc) {
	    *sockout << "Illegal command '" << argv[0] << "'! Use 'help'." << endl;
	} else if (sc->command) {
	    GEA.dbg() << "executing " << argv[0] << endl;
	    return sc->command(*this, sc->data, argc, argv);
	}
    }
    return 0;
}

bool TcpShellClient::parse_command(char *buf, int len) {
    char *argv[CMD_PARAM_MAX];
    int argc = 0;
    if (len == 0 ||
	len > CMD_BUF_MAX ||
	NULL != memchr(buf, '\0', len)) {
	    GEA.dbg() << "Illegal input from " << inet_ntoa(peer_addr.sin_addr) << ":" << ntohs(peer_addr.sin_port) << endl;
	    return false;
    }
    //    GEA.dbg() << "parse_command '" << buf << "' " << argc << " " << len << endl;

    char *strtrtr;
    argv[argc] = strtok_r(buf, CMD_IFS, &strtrtr);
    while ((argc < CMD_PARAM_MAX-1) && argv[argc] != NULL) {
    	//GEA.dbg() << "arg " << argc << ": " << argv[argc] << endl;
	argc++;
	argv[argc] = strtok_r(NULL, CMD_IFS, &strtrtr);
    }
    exec(argc, argv);
    return true;
}


void TcpShellClient::read_client_data(gea::Handle *h, gea::AbsTime t, void *data) {
    TcpShellClient *self = static_cast<TcpShellClient *>(data);
    
    if (h->status == Handle::Ready) {

	int ret;
	ret = h->read(&self->cmd_buf[self->cmd_buf_len], CMD_BUF_MAX-self->cmd_buf_len);
	if (ret <= 0) {
	    GEA.dbg() << "closing shell connection "  << inet_ntoa(self->peer_addr.sin_addr) << ":" << ntohs(self->peer_addr.sin_port) << endl;
	    delete h;
	    close (self->fd);
	    
	    if (self->state == CS_Blocked) {
		// unregister callback
	    	assert(!"Shell connection died in progress");
	    }
	    
	    //XXX: cross layer
	    self->shell->clients.erase(self->fd);
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
		self->parse_command(self->cmd_buf, nlpos);
		//h->write(self->cmd_buf, nlpos);
	    }
	    // now lets shift the remaining input to the buffer start
	    memmove(self->cmd_buf, &self->cmd_buf[nlpos+1], self->cmd_buf_len - nlpos - 1);
	    self->cmd_buf_len -= 1 + nlpos;
	    assert(self->cmd_buf_len >= 0);
	    if (self->state == CS_Idle) {
		self->prompt(false);
	    }
	}
    } else {
	// XXX: timeout handling
    }
    
    if (self->state == CS_Idle) {
	GEA.waitFor(h, t + Duration(120.), read_client_data, data); 
    }
}


void TcpShellClient::prompt(bool wait) {
	*sockout << "# ";
	sockout->flush();
	if (wait)
	    GEA.waitFor(sockin, AbsTime::now() + Duration(120.), read_client_data, this);
}

void TcpShellClient::block() {
	assert(state == CS_Idle);
	state = CS_Blocked;
}

void TcpShellClient::unblock() {
	assert(state == CS_Blocked);
	state = CS_Idle;
	prompt(true);
}


void test_stop(gea::Handle *h, gea::AbsTime t, void *data) {
    ShellClient *sc = static_cast<ShellClient *>(data);

    *sc->sockout << "Now he is dead." << endl;
    sc->unblock();
}

int test(ShellClient &sc, void *data, int argc, char **argv) {
    if (argc > 1)
	*sc.sockout << "I shot " << argv[1] << "!" << endl;
    else
	*sc.sockout << "I shot the sherriff!" << endl;
    sc.block();
    GEA.waitFor(sc.sockin, AbsTime::now() + Duration(3.), test_stop, &sc);
    return 0;
}

GEA_MAIN_2(shell, argc, argv)
{    

    ObjRepository& rep = ObjRepository::instance();
    //RTopology *topology = (RTopology *)rep.getObj("topology");
    //if (!topology) {
    //    GEA.dbg() << "cannot find object 'topology' in repository" << endl; 
    //    return -1;
    //}
    

    Shell *sh = new TcpShell();
    //    sh->add_command("test", test, NULL, "shell function example", "shell function example long help");

    rep.insertObj("shell", "Shell", (void*)sh);
  
    return 0;
}


/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
