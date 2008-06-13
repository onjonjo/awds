
#include <sys/types.h>

#include <cassert>

#include <awds/beacon.h>
#include <awds/AwdsRouting.h>
#include <awds/toArray.h>

#include <iostream>
using namespace std;
using namespace awds;
using namespace gea;

gea::Duration Beacon::getPeriod() const {
    u_int16_t period = fromArray<u_int16_t>(&packet.buffer[OffsetPeriod]);
    return gea::Duration(period, 1000);
}

void Beacon::setPeriod(const gea::Duration& d) {
    u_int16_t period = d.getNanoSecsLL() / 1000000LL; // duration in ms.
    toArray<u_int16_t>(period, &packet.buffer[OffsetPeriod]);
}




void Beacon::setNeigh(AwdsRouting *awdsRouting, gea::AbsTime  t) {


    int i;

    char *addr =  &(packet.buffer[OffsetLNeigh]);

    int numMpr = 0;
    int numNoMpr = 0;


    // insert all MPR nodes:
    for (i = 0; i != awdsRouting->numNeigh; ++i) {

	if ( awdsRouting->neighbors[i].isGood() &&
	     awdsRouting->neighbors[i].mpr )
	    {

		awdsRouting->neighbors[i].id.toArray(addr);
		++numMpr;
		addr += NodeId::size;
	    }
    }

    for (i = 0; i != awdsRouting->numNeigh; ++i) {

	if ( awdsRouting->neighbors[i].isGood() &&
	     ! awdsRouting->neighbors[i].mpr )
	    {

		awdsRouting->neighbors[i].id.toArray(addr);
		++numNoMpr;
		addr += NodeId::size;
	    }
    }


    this->setNumMpr(numMpr);
    this->setNumNoMpr(numNoMpr);


    this->packet.size = OffsetLNeigh + ( (numMpr + numNoMpr) * NodeId::size );
}


bool Beacon::hasNeighBinsearch(int a, int b, const NodeId& id) {

    NodeId mId;

    while (a != b) {
	int m = (a+b)/2;
	mId.fromArray(&(packet.buffer[OffsetLNeigh + (m * NodeId::size)]));
	if (mId == id) return true;
	if (mId < id)
	    a = m + 1;
	else if (mId > id)
	    b = m;

    }
    return false;
}


bool Beacon::hasMpr(const NodeId& id) {
    int a = 0;
    int b = this->getNumMpr();
    return hasNeighBinsearch(a,b,id);
}

bool Beacon::hasNoMpr(const NodeId& id) {

    int a = this->getNumMpr();
    int b = a + this->getNumNoMpr();

    return hasNeighBinsearch(a,b,id);


//     while (a != b) {
//	int m = (a+b)/2;
//	NodeId mId;
//	mId.fromArray(&(packet.buffer[OffsetLNeigh + (m * NodeId::size)]));
//	if (mId == id) return true;
//	if (mId < id)
//	    a = m + 1;
//	else if (mId > id)
//	    b = m;

//     }
//     return false;
}

bool Beacon::hasNeigh(const NodeId& id) {
    return hasMpr(id) || hasNoMpr(id);
}

void Beacon::add2Hop(AwdsRouting *awdsRouting) {
    char *addr =  &(packet.buffer[OffsetLNeigh]);
    int n = getNumNoMpr() + getNumMpr();

    for (int i=0; i < n; ++i) {
	NodeId n2hop;
	n2hop.fromArray(addr);
	addr += NodeId::size;

	AwdsRouting::Hop2List::iterator itr = awdsRouting->hop2list.find(n2hop);
	if (itr == awdsRouting->hop2list.end()) {

	    awdsRouting->hop2list[n2hop] = AwdsRouting::Hop2RefCount(1);
	} else {
	    awdsRouting->hop2list[n2hop].stat++;
	}
    }
}

void Beacon::remove2Hop(AwdsRouting *awdsRouting) {
    char *addr =  &(packet.buffer[OffsetLNeigh]);
    int n = getNumNoMpr() + getNumMpr();

    //    cout << "Source: " << getSrc() << endl;
    for (int i=0; i < n; ++i) {
	NodeId n2hop;
	n2hop.fromArray(addr);
	addr += NodeId::size;


	// cout << "Node: " << n2hop << endl;
	AwdsRouting::Hop2List::iterator itr = awdsRouting->hop2list.find(n2hop);
	assert (itr != awdsRouting->hop2list.end());

	if ( (--itr->second.stat) == 0) {
	    awdsRouting->hop2list.erase(itr);
	}

    }
}

bool Beacon::tryRemoveFromMpr(AwdsRouting *awdsRouting) {

    // disable MPR calculation ....
    return true;

    NodeId src = getSrc();

    char *addr =  &(packet.buffer[OffsetLNeigh]);
    int n = getNumNoMpr() + getNumMpr();



    for (int i=0; i < n; ++i) {

	NodeId n2hop;
	n2hop.fromArray(addr);
	addr += NodeId::size;

	AwdsRouting::Hop2List::iterator itr = awdsRouting->hop2list.find(n2hop);
	assert (itr != awdsRouting->hop2list.end());


	if (itr->second.dyn == 1) { // reference counter would fall to zero
	    return true;
	}
    }


    addr =  &(packet.buffer[OffsetLNeigh]); // reset address

    for (int i=0; i < n; ++i) {
	NodeId n2hop;
	n2hop.fromArray(addr);
	addr += NodeId::size;

	AwdsRouting::Hop2List::iterator itr = awdsRouting->hop2list.find(n2hop);
	assert (itr != awdsRouting->hop2list.end());
	itr->second.dyn--;
	assert(itr->second.dyn > 0);
    }

    return false;
}



/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
