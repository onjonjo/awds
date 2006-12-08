#ifndef _AESCCMCRYPTOUNIT_H__
#define _AESCCMCRYPTOUNIT_H__

#include "CryptoUnit.h"
#include <map>

#include <awds/NodeId.h>
#include <sys/types.h>

/** 
 * this is a 128 bit aes ccm implementation of the crypto unit
 */
class AesCcmCryptoUnit : public CryptoUnit {
    
    static const size_t IV_len = 1;
    
    unsigned char key[16];
    
    unsigned char nonce[16];
    
    u_int64_t lastIv;
    
    typedef std::map<NodeId, u_int64_t> IvMap;
    IvMap ivMap;
    
public:
    AesCcmCryptoUnit();
    
    virtual void encrypt(void *data, size_t data_len, const MemoryBlock sg[]);
    bool decrypt(void *data, size_t data_len, const MemoryBlock sg[]);
    
    virtual bool decryptDupDetect(const NodeId& src, void *data, size_t data_len, const MemoryBlock sg[]);
    
    void setKey(void *key);
    
    
    /** 
     *  sign the packet and add a iv.
     *  The memory area defined by sg is EXLUDED from the signing.
     */
    virtual void sign(void *data, size_t data_len, const MemoryBlock sg[]);
        
    virtual bool verifySignature(const NodeId& src, void *data, size_t data_len, const MemoryBlock sg[]);
    
    void test();
    
protected:
    
    void storeNonce(void *data);
    bool verifyNonce(const NodeId& src, const void *noncep_);
    
    bool crypt(bool isDecrypt, void *data, size_t data_len, const MemoryBlock sg[]);    
    
    char * saveNonsignArea(const MemoryBlock sg[]);
    void restoreNonsignArea(char * store, const MemoryBlock sg[]);
    
};




#endif //AESCCMCRYPTOUNIT_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
