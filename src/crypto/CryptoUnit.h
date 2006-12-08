#ifndef _CRYPTOUNIT_H__
#define _CRYPTOUNIT_H__

/* how does it work?
 *
 *  ------------- -------------- ---------------- -------------
 * |   	   	 |       	| 		 |             |
 * | Plain Block | Cypher Block | Checksum Block | Nonce Block |
 * |       	 |              |		 |             |
 *  ------------- -------------- ---------------- -------------
 * |<-plain_len->| 
 *
 * |<------- total_len -------->|
 *
 *                              |<-------- crypto_len -------->|
 * 
 *
 * The crypto unit can authenticate and encrypt a continous block of bytes.
 * The additional data for this is appended to the block.
 * The plain block is authenticated but not encrypted.
 * The cyper block is encrypted.
 * The checksum and nonce block are arbitrarily depending on the implemetation.
 * 
 *
 *
 */

#include <cstddef>
#include <awds/NodeId.h>


class CryptoUnit {

public:
    struct MemoryBlock {
	void   *p;
	size_t size;
    };
    
    virtual void encrypt(void *data, size_t data_len, const MemoryBlock sg[]) = 0;
    virtual bool decryptDupDetect(const NodeId& src, void *data, size_t data_len, const MemoryBlock sg[]) = 0;

    /** 
     *  sign the packet and add a iv.
     *  The memory area defined by sg is EXLUDED from the signing.
     */
    virtual void sign(void *data, size_t data_len, const MemoryBlock sg[]) = 0;
    
    
    virtual bool verifySignature(const NodeId& src, void *data, size_t data_len, const MemoryBlock sg[]) = 0;

    virtual ~CryptoUnit() {}
    
};


#endif //CRYPTOUNIT_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
