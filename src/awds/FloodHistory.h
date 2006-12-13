#ifndef _FLOODHISTOY_H__
#define _FLOODHISTOY_H__

#include <cstddef>
#include <sys/types.h>

#include <awds/NodeId.h>

class FloodHistory {
    
public: 
    
    struct Entry {
	NodeId id; 
	u_int16_t seq;
    };
    

    size_t size;
    size_t start, end;
    
    Entry *hist;
    
    FloodHistory() : size(0x80) {
	start = 0;
	end = 0;
	
	hist = new Entry[size];
    }
    
    
    ~FloodHistory() {
	delete hist;
    }

    void insert(const NodeId& id, u_int16_t seq) {
	end = ( end + 1 ) % size;
	hist[end].id = id;
	hist[end].seq = seq;
	if ( end == start) 
	    start = (start + 1) % size;
    }
    
    void printHistoryOfNode(const NodeId& id);  

    bool contains(const NodeId& id, u_int16_t seq) const;


};


#endif //FLOODHISTOY_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */