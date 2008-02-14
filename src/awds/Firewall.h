#ifndef _FIREWALL_H__
#define _FIREWALL_H__

#include <awds/BasePacket.h>

namespace awds {


    /** \brief base class for implementing firewalls rules
     */
    class Firewall {

    public:
	/** \brief decide, if a packet is accepted
	 *
	 *  \param p the packet to check.
	 *  \return true, if accepted. false otherwise.
	 */
	virtual bool check_packet(BasePacket *p) = 0;
	virtual ~Firewall() {}
    };

}



#endif //FIREWALL_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
