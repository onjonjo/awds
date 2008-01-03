#ifndef _SRCFILTER_H__
#define _SRCFILTER_H__

#include <awds/Firewall.h>
#include <awds/Topology.h>

#include <awds/ext/Shell.h>

#include <map>


namespace awds {

    /** \brief Firewall filter, based on the source of the packet.
     */
    class SrcFilter : public Firewall {

	awds::Topology * const topology; ///< for resolving names.

	/** \brief data structure for storing the individual rules */
	typedef std::map<awds::NodeId, bool> Rules;

	Rules rules;  ///< Here are the rules stored.

	bool default_policy; ///< default policy.

    public:

	/** \brief add rules
	 *
	 *  This is a parser for adding new rules.
	 *  \param argc the number of arguments.
	 *  \param argv the array of arguments.
	 *  \param os the stream to write the output to.
	 *  \returns 0 on success, other values otherwise.
	 */
	int addRules(int argc, const char *const *argv, std::ostream& os);

	/** \brief dump the currently acrtive rules
	 *
	 *  Use this function to dump the currently avtive rules to the screen.
	 *  \param os the stream to write the output to.
	 */
	void dumpRules(std::ostream& os) const;

	/** \brief callback for the shell
	 */
	static int cmd_filter(awds::ShellClient &sc, void *data, int argc, char **argv);

	/** \brief constructor */
	SrcFilter(awds::Topology *topology);

	/** \brief decide, if a packet is accepted
	 *
	 *  \param p the packet to check.
	 *  \return true, if accepted. false otherwise.
	 */
	virtual bool check_packet(const awds::BasePacket *p);

	/** destructor */
	virtual ~SrcFilter();
    };

}


#endif //SRCFILTER_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
