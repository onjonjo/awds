// Copyright 2005, Chris Frey.  To God be the glory.
// You are free to use, modify, redistribute, and sublicense this code,
// as long as this copyright message is not removed from the source,
// and as long as the existence of any changes are noted in the source
// as well.  (i.e. You don't need a complete history, just don't claim
// that the modified code was written entirely by me -- include your own
// copyright notice as well.)
//
// If you find this code useful, please email me and let me know.
//
// Conclusion:
//	C++'s getline() processes the stream one character at a time, in order
//	to search for the delimiter.  Therefore, with networkbuf, which only
//	fills the input buffer as much as it can with the available network
//	data and then returns, it doesn't block until it really needs to,
//	and gets all available lines from the kernel, even with a nice large
//	buffer.
//
//	With stdio_filebuf, this is built on top of C's streams, which has its
//	own buffer.  So the likelyhood of blocking before getting all the
//	data is higher, since C's buffers may empty and get forced to fill
//	with an internal fread() while C++'s getline is only asking for a
//	single char.
//
//	C++ streambufs and derived classes are actually pretty cool.  I just
//	wish they had used more user-friendly names.  But as is, you can
//	still use functions like xsgetn and xsputn as read and write, and
//	the streambuf supplies all the needed buffering.  Plus you can do it
//	on a character basis, while still maintaining efficiency with
//	buffered kernel calls.  Plus there are iterators to work with these
//	things, which I haven't fully investigated.
//
//	Time to rethink my design of reuse lib's buffer classes, and turn
//	them into streambufs perhaps, or at least derive streambuf interface
//	classes to make use of them.  Also, the transfer classes should be
//	able to use streambufs as well.
//
//	This stuff is complicated, and not commonly well documented, but
//	it sure is useful.
//
// Chris Frey
// <cdfrey@netdirect.ca>
// 2005/02/13
//

#include <cassert>
#include <iostream>

class UnixFdStreamBuf : public  std::basic_streambuf<char> {
  
    static const size_t bufSize = 256;
    char buffer[bufSize + 1]; // one additional character for  overflow ...
  
    const int fd;
  
public:
    UnixFdStreamBuf(int fd) : std::basic_streambuf<char>(),
			      fd(fd)
    {
	setp(buffer, buffer + bufSize);
    };
  
    // write out any data in the out buffer, and write c as well if c!=eof()
    virtual int_type overflow (int_type c) {
	//		cout << "overflow called: " << c << endl;
	assert( pbase() );
	bool have_extra = c != traits_type::eof();

	// pbase() is a pointer at the start of our buffer,
	// and pptr() is a pointer to the next free spot,
	// so we can check for data in buffer by comparing them
	if( pptr() > pbase() || have_extra ) {
	    // we have something to write!
	    ssize_t count = pptr() - pbase();
	    if( have_extra ) {
		// tack extra value onto the end of the buf
		// (note constructor added a byte for us here,
		// just in case)
		*(pptr()) = traits_type::to_char_type(c);
		count++;
	    }

	    char_type *wp = pbase();
	    
	    if ( ! writeSomeBytes(wp, count) )
		return traits_type::eof();
	    
	    // reset output buffer to empty state
	    setp(buffer, buffer+bufSize);
	}
	return traits_type::not_eof(c);
    }

  
    virtual int sync() {
	if( overflow(traits_type::eof()) == traits_type::eof() )
	    return -1;
	return 0;
    }
  
protected:
    
    virtual bool writeSomeBytes(const char *data, int count) {
	int ret;
	
	do {
	    do {
	    ret = ::write( this->fd, data, count);
	    } while (ret == -1 && errno == EINTR);
	    
	    if( ret > 0 ) {
		count -= ret;
		assert(count >= 0);
		data += ret;
	    }
	    else {
		// fixme... do we set badbit here?
		// failbit?
		return false;
	    }
	} while(count);

	return true;
    }
};



/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
