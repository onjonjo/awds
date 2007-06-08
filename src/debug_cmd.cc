#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>


#include <errno.h>

#include <fstream>
#include <set>

#include <gea/gea_main.h>
#include <gea/ObjRepository.h>
#include <gea/API.h>
#include <awds/ext/Shell.h>

#include "unixfdostream.h"

//using namespace std;

using namespace awds;

class OutputRedirector : public UnixFdStreamBuf {
    
public:    
    typedef std::set<std::ostream *> OutputSet;
    OutputSet outputSet;
    std::ofstream  * logfile;
    
    
    char  *history;
    int   histSize;
    int   bytesInHistory;
    int   histEnd;

    
    OutputRedirector() : UnixFdStreamBuf(-1) {
	histSize = 64*1024;
	history = new char[histSize];
	bytesInHistory = histEnd = 0;
	logfile = 0;
    }
    
    void dumpHistory(std::ostream *os, int s = -1) {
	
	if (s == -1) s = histSize;
	
	if (s > bytesInHistory) s = bytesInHistory; // we cannot dump more bytes than are in the buffer. 
	
	int dumpStart = histEnd - s;

	if (dumpStart < 0) {
	    int w1 = -dumpStart;
	    dumpStart += histSize;

	    os->write(history + dumpStart, w1);
	    os->write(history, histEnd);
	}
	else {
	    os->write(history + dumpStart, s);
	}
	os->flush();
    }
    
    void storeInHistory(const char* data, int num) {
	
	// don't store more bytes than fit in the history:
	if (num > histSize) {
	    data += num - histSize;
	    num = histSize;
	}
	
	int w2 = (histEnd + num) - histSize; 
	if (w2 <= 0) {
	    memcpy(history + histEnd, data, num);
	} else {
	    memcpy(history + histEnd, data, num - w2);
	    memcpy(history, data + (num-w2), w2);
	}
	
	bytesInHistory += num;
	if (bytesInHistory > histSize) 
	    bytesInHistory = histSize;

	histEnd += num;
	histEnd %= histSize;
    }
    
    virtual ~OutputRedirector() {
	delete[] history;
    }

    void writeToOstreams(const char *data, int count) {
	
	char *buf = new char[count+1];
	memcpy(buf, data, count);
	buf[count] = '\0';
	
	for( OutputSet::iterator itr = outputSet.begin();
	     itr != outputSet.end();
	     ++itr) 
	    {
		**itr << buf;
		(**itr).flush();
	    }
	
	delete[] buf;
    }
    
    virtual bool writeSomeBytes(const char *data, int count) {
	writeToOstreams(data, count);
	storeInHistory(data, count);
	return true;
    }
    
};

typedef std::pair< ShellClient *,OutputRedirector *> xpair_t;



static void std_input(gea::Handle *h, gea::AbsTime t, void *data) {
    
    xpair_t *x = static_cast<xpair_t *>(data);
    ShellClient *scp = x->first;
    OutputRedirector *oprd = x->second;
    
    if (h->status == gea::Handle::Ready) {
	scp->unblock();
	oprd->outputSet.erase(scp->sockout);
    }
    
    delete x;
}


static int debug_command_fn(ShellClient &sc, void *data, int argc, char **argv) {
    OutputRedirector *self = static_cast<OutputRedirector *>(data);
    
    if (argc == 2 && !strcmp(argv[1], "hist")) {
	self->dumpHistory(sc.sockout);
	return 0;
    } else if ( argc == 2 && !strcmp(argv[1], "watch") ) {
	self->outputSet.insert(sc.sockout); 
	GEA.waitFor(sc.sockin, gea::AbsTime::now() + 900000. , std_input, new xpair_t(&sc, self) );
	sc.block();
	return 0;
    } else if ( argc == 3 && !strcmp(argv[1], "logfile") ) {
	
	if (self->logfile) {
	    self->logfile->close();
	    delete self->logfile;
	    self->outputSet.erase(self->logfile);
	}
	const char *filename = argv[2];
	if ( 0 != strcmp(filename, "off") ) {
	    self->logfile = new std::ofstream(filename);
	    self->outputSet.insert(self->logfile);
	}
    } else if ( argc == 1 ) {
	self->dumpHistory(sc.sockout);
	self->outputSet.insert(sc.sockout); 
	GEA.waitFor(sc.sockin, gea::AbsTime::now() + 900000. , std_input, new xpair_t(&sc, self) );
	sc.block();
	return 0;
    }
    return -1;
}

static int quit_command_fn(ShellClient &sc, void *data, int argc, char **argv) {
    if (argc == 2  && !strcmp(argv[1], "now!") )
	_exit(0);
    return 0;
	
}

static void daemonize() {
    
    if (fork()) _exit(0);
    setsid();
    close(1);
    close(2);
}

void parse_options(int argc, const char *const *argv) {
    
    const char *pidfilename = 0;
    bool noDaemon = false;
    bool testIfRunning = false;
    bool terminate = false;
    
    for (int i = 1; i < argc; ++i) {
	const char *cur = argv[i];
	if ( (!strcmp(cur, "-p") || !strcmp(cur, "--pidfile")) && i+1 < argc ) {
	    ++i;
	    pidfilename = argv[i];
	    continue;
	}
	
	if ( !strcmp(cur, "-N") || !strcmp(cur, "--nodaemon") ) {
	    noDaemon = true;
	    continue;
	}
	
	if ( !strcmp(cur, "-k") || !strcmp(cur, "--kill") ) {
	    terminate = true;
	    continue;
	}

	if ( !strcmp(cur, "-r") || !strcmp(cur, "--is-running") ) {
	    testIfRunning = true;
	    continue;
	}
	
    }
    
    bool isRunning;
    int lockfd = 0;
    if (pidfilename) {
	int ret;
	
	lockfd = open(pidfilename, O_CREAT|O_RDWR, 0660);
	do {
	    ret = ::flock(lockfd, LOCK_EX | LOCK_NB);
	} while (ret == -1 && errno == EINTR);
	
	isRunning =  (ret == -1 && errno == EWOULDBLOCK); 
	
	if (terminate) {
	    int nb;
	    char pidtext[64];
	    int pid;
	    
	    if (!isRunning)  // stop here, so we do not kill innocent processes. 
		_exit(1);
	    
	    nb = read(lockfd, pidtext, 63);

	    if (nb >0) {
		pidtext[nb] = '\0'; // add null termination
		sscanf(pidtext, "%d", &pid);
		kill(pid, SIGTERM);
	    }
	    _exit(0);
	}
	
	if (isRunning) 
	    _exit(2);

	if (testIfRunning) // do not start, only perform the previous test
	    _exit(0);
	
    }
    
    
    if (!noDaemon)
	daemonize();
    
    if (pidfilename) {
	//	xxxxx std::ofstream pidfile(pidfilename);
	//xxx	pidfile << getpid();
	char pidstring[32];
	sprintf(pidstring, "%d", getpid());
	ftruncate(lockfd, 0);
	write(lockfd, pidstring, strlen(pidstring));
	fsync(lockfd); 
	// we keep the fd open to hold the lock.
    }
    
    
}

GEA_MAIN(argc, argv)    
{    
    static const char *short_help = "show debug outputs";
    static const char *long_help = 
	"debug [watch|hist|logfile]\n\n"
	"  hist               - print previous debug outputs\n"
	"  watch              - monitor the debug outputs\n"
	"  logfile <filename> - write debug output to file \n"
	"                       writing is stopped, if <filename> is off\n"
	"  without parameter  - first the debug history is printed and than monitored\n";
    
    OutputRedirector *output_redirector = new OutputRedirector();

    REP_MAP_OBJ(Shell *, shell);

    if (shell) {

	shell->add_command("debug", debug_command_fn, output_redirector, short_help, long_help);
	shell->add_command("quit", quit_command_fn, 0, "quit AWDS" , "usage: quit now!");
    }
    
    REP_MAP_OBJ(std::ostream **, GEA_defaultOstream);
    if (GEA_defaultOstream) {
	*GEA_defaultOstream = new std::ostream(output_redirector);
    }
    
    parse_options(argc, argv);
    return 0;
}


/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
