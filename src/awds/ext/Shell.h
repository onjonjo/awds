#ifndef _SHELL_H__
#define _SHELL_H__

#include <ostream>
#include <string>
#include <gea/posix/UnixFdHandle.h>

/** \defgroup shell_mod
 *   \brief interactive shell for the routing daemon
 *
 *  This module provides an interactive command shell
 *  for parameter manipulation and status query.
 */


namespace awds {

    class Shell;
    

    /** \brief interaction object of a shell client 
     */
    class ShellClient {
    public:
	
	/** used to represent the current state of a client.
	 */
	enum ClientState { 
	    CS_Idle,   ///< the client is idle.
	    CS_Blocked ///< the client is blocked.
	};

	gea::UnixFdHandle *sockin; ///< for reading from. and we can block on it.
	std::ostream *sockout;     ///< use this for writing messages.
	enum ClientState state;    ///< the current state of the client.

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
	
	/** execute a command */
	virtual int exec(int argc, char **argv) = 0;

	virtual ~ShellClient() {}
    };
    
    /** \brief type of a shell callback function 
     *  
     *  \param sc     the associated shell client data structure
     *  \param data   the custom data for the callback function
     *  \param argc   the number of arguments
     *  \param argv   the array of arguments.
     *  \returns 0 on success, other values otherwise.
     */
    typedef int (shell_command_fn)(ShellClient &sc, void *data, int argc, char **argv);
    
    /** \brief for storing a registered command. 
     */
    struct ShellCommand {
	shell_command_fn *command; ///< the callback function.
	void *data;                ///< the custom data for the function.
	const char *desc;          ///< the short description.
	const char *help;          ///< the long description.
    };


    /** \brief class for shell object 
     *  \ingroup shell_mod
    */
    class Shell {
    public:    
	
	/** \brief constuctor. */
	Shell() {};
	
	/** \brief add a new command to the shell
	 *
	 *  \param name    the name of the command.
	 *  \param command the callback function of the command.
	 *  \param data    custom data for the callback function.
	 *  \param descr   the short description of the command.
	 *  \param help    the long help for the command.
	 */
	virtual void add_command(const std::string name, shell_command_fn *command,
				 void *data, const char *descr, const char *help) = 0;
	
	/** \brief get the command struct for the correspronding name
	 */
	virtual ShellCommand *get_command(std::string name) = 0;
	
	
	/** \brief destructor.
	 *
	 *  This will normally never be called. So who cares. 
	 */
	virtual ~Shell() {}
    
    }; // end of class Shell
    
} // end of namespace awds

#endif /* _SHELL_H__ */
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
