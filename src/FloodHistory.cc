#include <gea/API.h>
#include <awds/FloodHistory.h>

using namespace std;

bool awds::FloodHistory::contains(const NodeId& id, u_int16_t seq) const {

    size_t p = end;

    for (size_t i = 0; i<count; ++i) {
	if ( (hist[p].seq == seq ) && (hist[p].id == id ))
	    return true;
	if (p == 0)
	    p = FLOOD_HISTORY_NUM_ENTRIES - 1;
	else
	    --p;
    }
    return false;
}


void awds::FloodHistory::printHistoryOfNode(const NodeId& id)  {
    std::ostream& os = GEA.dbg();
    os << "hist of " << id << ":";

    for (size_t i = 0; i < FLOOD_HISTORY_NUM_ENTRIES; ++i) {
	if (hist[i].id == id) {
	    os << " " << hist[i].seq;
	}
    }
    os << endl;
}


/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
