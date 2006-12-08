#include <gea/API.h>

#include <awds/tapiface2.h>

bool   TapInterface2::getNodeForMacAddress(const char* mac, NodeId& id, gea::AbsTime t) {
    NodeId m;
    m.fromArray(mac);
    
    MacTable::const_iterator itr = macTable.find(m);
    if ( (itr == macTable.end())  // not found
	 || (itr->second.validity < t)  // entry too old
	 )
	{
// 	    GEA.dbg() << "cannot find " << m 
// 		      << " sending as broadcast" <<std::endl;
	    return false;
	}
    id = itr->second.id;
    return true;
}


void TapInterface2::storeSrcAndMac(const NodeId &id, const char *buf, gea::AbsTime t) {
    
    NodeId m;
    m.fromArray(buf+6); // read src mac.
    
    MacTable::iterator itr = macTable.find(m);
    
    if ( itr == macTable.end() ) {
	GEA.dbg() << "adding " << m << "@" << id << " to the mac table" << std::endl;
    }
    
    MacEntry& macEntry = macTable[m];
    macEntry.id = id;
    macEntry.validity = t + gea::Duration(30.);
}

bool TapInterface2::setIfaceHwAddress(const NodeId& id) {
    return true;
}

    


/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */



