#ifndef __ABSTRACTID_H_
#define __ABSTRACTID_H_

#include <cstring>

#include <inttypes.h>


/** Unique identifier for a system service (aka port number) 
 *
 *  
 */
template<unsigned Size>
class AbstractID {

public:
    static const size_t size = Size;
    
    /** we should better use an valarray<unsigned char> */
    unsigned char id[size];
    
    AbstractID(){
	for (unsigned short i=0; i < Size; i++)
	    this->id[i] = 0;
    }
    
    AbstractID(const AbstractID<Size>& a) {
	for (unsigned short i=0; i < Size; i++)
	    this->id[i] = a.id[i];
    }
    
    AbstractID(unsigned num) {
	for (short i = Size - 1 ; i >= 0; i--) {
	    id[i] = (char)(num & 0xff);
	    num >>= 8;
	}
    }

    const AbstractID<Size>& operator =(const AbstractID<Size>& a) {
	for (unsigned short i=0; i < Size; i++)
	    this->id[i] = a.id[i];
	return *this;
    }
    
    AbstractID<Size>& fromArray(const char *data) {
	memcpy(this->id, data, size );
	return *this;
    }
    
    AbstractID<Size>& toArray( char *data) const {
	memcpy(data, this->id, size );
	return *this;
    }

 
    /** use the lexicographic operators for comparision: */
#define LEXI_BOOL_OP(op) bool operator op (const AbstractID<Size>& a) const { \
	for(unsigned short i = 0; i < Size; i++) {					\
	    if (this->id[i] == a.id[i]) continue;			\
	    if ( !( this->id[i] op a.id[i] ) ) return false;		\
	    if (this->id[i] op a.id[i]) return true;			\
	}								\
	return (0 op 0);						\
    }
    
    LEXI_BOOL_OP(==);
    LEXI_BOOL_OP(!=);
    LEXI_BOOL_OP(<);
    LEXI_BOOL_OP(>);
    LEXI_BOOL_OP(<=);
    LEXI_BOOL_OP(>=);
  
#undef LEXI_BOOL_OP  
};



template <>
class AbstractID<6> {
    
public:
    unsigned long long d;
    
    static const size_t size = 6;
    
        
    AbstractID() : d(0) {}
    
    AbstractID(const AbstractID<6>& a) : d(a.d) {   }
    
    explicit AbstractID(unsigned num) {
	d = num;
    }
    
    const AbstractID<6>& operator =(const AbstractID<6>& a) {
	d = a.d;
	return *this;
    }
    
    void fromArray(const char *data) {
	d = 0;
	for (size_t i = 0; i < 6; ++i) {
	    d *= 0x0100ULL;
	    d |= (unsigned char)data[i];
	}
    }
    
    void toArray( char *data) const {
	unsigned long long dd = d;
	for (size_t i=0; i < 6; ++i) {
	    data[5-i] = (char)(unsigned char)dd; 
	    dd /= 0x0100ULL;
	}
    }

    operator unsigned long() const {
	return (unsigned long)d;
    }
 
    /** use the lexicographic operators for comparision: */
#define LEXI_BOOL_OP(op) bool operator op (const AbstractID<6>& a) const { return d op a.d;  }
    
    LEXI_BOOL_OP(==);
    LEXI_BOOL_OP(!=);
    LEXI_BOOL_OP(<);
    LEXI_BOOL_OP(>);
    LEXI_BOOL_OP(<=);
    LEXI_BOOL_OP(>=);
    
#undef LEXI_BOOL_OP  
};





#define ID_IO 1
#if ID_IO
#include <iostream>

template <unsigned S> 
std::ostream& operator <<(std::ostream& s, const AbstractID<S>& aid) {
    for (unsigned short i = 0; i < S;i++) {
	static const char *hexnum = "0123456789ABCDEF";
	s << hexnum[aid.id[i] / 0x10] 
	  << hexnum[aid.id[i] % 0x10]; 
    }
    return s;
}
   



inline std::ostream& operator <<(std::ostream& s, const AbstractID<6u> aid) {
    static const char *hexnum = "0123456789ABCDEF";
    char buf[13];  
    unsigned long long dd = aid.d;
    for (size_t i=0; i < 6; ++i) {
	unsigned char x = (unsigned char)dd;
	buf[11 - (2*i)] = hexnum[x % 0x10];
	buf[10 - (2*i)] = hexnum[x / 0x10];
	dd /= 0x0100ULL;
    }
    buf[12] = '\0';
    return s << buf;
}

#endif // ID_IO

#endif // __ABSTRACTID_H_

/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
