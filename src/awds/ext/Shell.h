#ifndef _SHELL_H__
#define _SHELL_H__

#include <ostream>
#include <string>
#include <gea/posix/UnixFdHandle.h>

/** 
 * this module provides an interactive command shell
 * for parameter manipulation and status query.
 */
class Shell;

class ShellClient {
public:
    enum ClientState {
	CS_Idle,
	CS_Blocked
    };

    gea::UnixFdHandle *sockin;
    std::ostream *sockout;
    enum ClientState state;

    ShellClient(gea::UnixFdHandle *_sockin, std::ostream *_sockout) :
	sockin(_sockin),
	sockout(_sockout),
    	state(CS_Idle)
    {}

    ShellClient() :
    	state(CS_Idle)
    {}

    virtual void block() = 0;
    virtual void unblock() = 0;

    virtual int exec(int argc, char **argv) = 0;

    virtual ~ShellClient() {}
};

typedef int (shell_command_fn)(ShellClient &sc, void *data, int argc, char **argv);

struct ShellCommand {
	shell_command_fn *command;
	void *data;
	char *desc;
	char *help;
};

class Shell {
public:    
    Shell() {};

    virtual void add_command(const std::string name, shell_command_fn *command,
   		void *data, char *descr, char *help) = 0;
    virtual ShellCommand *get_command(std::string name) = 0;

    virtual ~Shell() {}
};

#endif /* _SHELL_H__ */
