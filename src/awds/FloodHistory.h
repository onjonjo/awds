#ifndef _FLOODHISTOY_H__
#define _FLOODHISTOY_H__

#include <cstddef>
#include <sys/types.h>

#include <awds/NodeId.h>

namespace awds{

    /** \brief data structure to remember and lookup the recent flood packets
     * 
     *  It implements a ring buffer. 
     *  
     */
    class FloodHistory {

    public:

	/** \brief an entry in the flood history */
	struct Entry {
	    NodeId id;
	    u_int16_t seq;
	};

#define FLOOD_HISTORY_NUM_ENTRIES ((size_t)0x80) ///< how many packets to remember

	size_t end;    ///< end pointer in the ring buffer
	size_t count;  ///< number of entries in the ring buffer

	Entry hist[FLOOD_HISTORY_NUM_ENTRIES];

	FloodHistory() 
	{
	    count = 0;
	    end = 0;
	}

	void insert(const NodeId& id, u_int16_t seq) {
	    end = ( end + 1 ) % FLOOD_HISTORY_NUM_ENTRIES;
	    hist[end].id = id;
	    hist[end].seq = seq;
	    count = std::min(count+1, FLOOD_HISTORY_NUM_ENTRIES);
	}

	void printHistoryOfNode(const NodeId& id);

	bool contains(const NodeId& id, u_int16_t seq) const;

    };
}

#endif //FLOODHISTOY_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
