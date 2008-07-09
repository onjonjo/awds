#ifndef _CALLBACK_H__
#define _CALLBACK_H__

#include <list>
#include <algorithm>

template<typename P>
class Callback {

    typedef bool (*callback_f)(void *, const P);
    typedef std::pair<callback_f, void*> Entry;

    typedef std::list<Entry> EntrySet;
    EntrySet entrySet;


public:

    Callback()     {
	entrySet.clear();
    }

    ~Callback() {

    }

    bool operator()(const P param) {

	bool ret = true;
	typename EntrySet::iterator itr;

	for (  itr =  entrySet.begin();
	       itr != entrySet.end();
	       ++itr) {

	    ret = itr->first(itr->second, param);
	    if (!ret)
		return ret;
	}
	return ret;
    }


    bool add(callback_f f, void *data) {

	// check if not already in set
	if (find(entrySet.begin(), entrySet.end(), Entry(f,data)) != entrySet.end() )
	    return false;

	entrySet.push_back(Entry(f,data));
	return true;
    }

    bool remove(callback_f f, void *data) {

	typename EntrySet::iterator itr =
	    find(entrySet.begin(), entrySet.end(), Entry(f,data));

	if (itr == entrySet.end())
	    return false;

	entrySet.erase(itr);
	return true;
    }

    bool empty() const {
	return entrySet.empty();
    }

};




#endif //CALLBACK_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
