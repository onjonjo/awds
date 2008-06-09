#ifndef _NODEDESCR_H__
#define _NODEDESCR_H__

#include <cassert>


#include <stdint.h>

#include <gea/API.h>
#include <gea/Time.h>

#include <awds/BasePacket.h>
#include <awds/NodeId.h>
#include <awds/beacon.h>
#include <awds/settings.h>

#include <iostream>


namespace awds {
struct NodeDescr {


    static const int LostTrigger = 8;
    static bool verbose;

    NodeId id;                    /// ID of the node
    BasePacket *lastBeacon;       /// pointer to the last received beacon
    gea::AbsTime lastBeaconTime;  /// when was the last beacon received
    gea::Duration beaconInterval; /// beacon interval of the node.
    //    unsigned long beaconHist;
    u_int32_t beaconHist;         /// bitfield of the last received/lost beacons
    bool active;                  /// is this an active (good recieved) node
    bool mpr;			  /// is this a MPR node

    void updateActive( /* gea::AbsTime t */) {

	const bool last12received = (beaconHist > 0xFFF00000UL);
	const bool last4lost      = (beaconHist < 0x08000000UL );
	const bool lastBeacon2old = (lastBeaconTime <
				     GEA.lastEventTime - gea::Duration( (double)beaconInterval
									* (double)LostTrigger) );
	if ( !active && !lastBeacon2old && last12received) {
	    active = true;
	    if (verbose) {
		GEA.dbg() << "neighbor "
			  << Beacon(*lastBeacon).getSrc() << " became active" << std::endl;
	    }
	} else if (  active && ( last4lost || lastBeacon2old ) )  {
	    active = false;
	    if (verbose) {
		GEA.dbg() << "neighbor "
			  << Beacon(*lastBeacon).getSrc() << " became inactive "
			  << ( last4lost ? "(last 4 beacon lost)" : "(last beacon too old)")
			  << std::endl;
	    }
	}
    }

    bool isGood() {
	updateActive();
	return active;
    }


    /** test for good bidirection connectivity to a neighbor.
     *  \return true, if neighbor link has good bidirectional connectivity.
     */
    bool isBidiGood(const NodeId& myId) {
	if (!lastBeacon) {
	    return false;
	}
	Beacon beacon(*lastBeacon);
	if (!beacon.hasNeigh(myId)) {
	    return false;
	}
	return isGood();
    }

    bool isTooOld() {
	return lastBeaconTime + (beaconInterval * 32)  < GEA.lastEventTime;

    }

    NodeDescr() : beaconInterval(0.) {}

    void init(const NodeId& _id, BasePacket *p, gea::AbsTime t) {
	active = false;
	mpr = true;
	beaconHist = 0;
	beaconInterval = (double)BEACON_INTERVAL / 1000.;
	this->id = _id;
	lastBeacon = p;
	lastBeaconTime = t;
    }

    /* \brief return the number of bits set in the beacon history
     * \returns the number of bits between 0 and 32
     */
    unsigned char quality() const {
	uint32_t x = beaconHist;

	unsigned char  n = 0;
	/*
	** The loop will execute once for each bit of x set, this is in average
	** twice as fast as the shift/test method.
	*/
	if (x) {
	    do {
		++n;
	    } while (0 != (x = x&(x-1)));
	}

	assert(n > 0);
	return n;
    }
};
}


#endif //NODEDESCR_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
