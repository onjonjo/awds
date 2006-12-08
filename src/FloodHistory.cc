#include <gea/API.h>
#include <awds/FloodHistory.h>

using namespace std;

bool FloodHistory::contains(const NodeId& id, u_int16_t seq) const {
	
    size_t p = end;
	
    while (1) {
	if ( (hist[p].seq == seq ) && (hist[p].id == id )) 
	    return true;
	if (p == start) 
	    return false;
	    
	if (p == 0) 
	    p = size-1;
	else 
	    --p;
    }
    
    assert(!"never reached");
}


void FloodHistory::printHistoryOfNode(const NodeId& id)  {
    std::ostream& os = GEA.dbg(); 
    os << "hist of " << id << ":";
	
    for (unsigned i = 0; i < size; ++i) {
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

