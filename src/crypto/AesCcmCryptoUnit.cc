
#include <gea/ObjRepository.h>
#include <gea/API.h>

#include <awds/toArray.h>
#include <awds/routing.h>
#include <crypto/crypto_helpers.h>

#include "gladman/ccm.h"
#include "AesCcmCryptoUnit.h"



using namespace std;
using namespace gea;
using namespace awds;

AesCcmCryptoUnit::AesCcmCryptoUnit() {
    lastIv = 0LL;

    getRandomByte(nonce,16);


}



void AesCcmCryptoUnit::encrypt(void *data, size_t data_len, const MemoryBlock sg[]) {

    storeNonce((char*)data + data_len + 16);
    crypt(false, data, data_len, sg);
}


void AesCcmCryptoUnit::storeNonce(void *data) {

    char *noncep = static_cast<char*>(data);

    memcpy(noncep + 8, this->nonce, 8);

    this->lastIv +=  1;
    toArray<u_int64_t>(this->lastIv, noncep);

    assert(fromArray<u_int64_t>(noncep) == this->lastIv);
}


char *  AesCcmCryptoUnit::saveNonsignArea(const MemoryBlock sg[]) {

    const MemoryBlock * nonsign;
    size_t storemem = 0;

    // calculate required memory:
    for (nonsign = sg; nonsign->p; nonsign++) {
	storemem += nonsign->size;
    }

    char *store = new char[storemem];
    char *storep = store;

    for (nonsign = sg; nonsign->p; nonsign++) {
	size_t s = nonsign->size;
	memcpy(storep, nonsign->p, s);
	memset(nonsign->p, 0, s);
	storep += s;

    }

    return store;
}

void AesCcmCryptoUnit::restoreNonsignArea(char * store, const MemoryBlock sg[]) {

    const MemoryBlock * nonsign;
    //size_t storemem = 0;

    char *storep = store;

    for (nonsign = sg; nonsign->p; nonsign++) {
	size_t s = nonsign->size;
	memcpy( nonsign->p, storep, s);
	storep += s;
    }

    delete[] store;
}



void AesCcmCryptoUnit::sign(void *data, size_t data_len, const MemoryBlock sg[]) {

    unsigned char* noncep  = (unsigned char*)data + data_len;

    storeNonce(noncep);

    // save non-signed areas and fill with zeros...

    char *store;
    if (sg) {
	store = saveNonsignArea(sg);
    }

    CCM_mode(this->key, 16,
	     noncep,
	     (unsigned char *)data, data_len + 16,
	     noncep + 16, 0,
	     16, // 16 byte MAC
	     0);


    //crypt(false, data, data_len, sg);

    if (sg) {
	restoreNonsignArea(store, sg);
    }

}


bool
AesCcmCryptoUnit::verifySignature(const NodeId& src,
				  void  *data,
				  size_t data_len,
				  const MemoryBlock sg[])
{
    ret_type ret;
    unsigned char* noncep  = (unsigned char*)data + data_len;



    // save non-signed areas and fill with zeros...

    char *store;
    if (sg) {
	store = saveNonsignArea(sg);
    }

    ret = CCM_mode(this->key, 16,
		   noncep,
		   (unsigned char *)data, data_len + 16,
		   noncep + 16, 0,
		   16, // 16 byte MAC
		   1);

    //crypt(false, data, data_len, sg);

    if (sg) {
	restoreNonsignArea(store, sg);
    }

    if (!verifyNonce(src, noncep))
	return false;

    return (ret >= 0);
}


bool AesCcmCryptoUnit::decrypt(void *data, size_t data_len, const MemoryBlock sg[]) {


    return crypt(true, data, data_len, sg);
}


bool AesCcmCryptoUnit::verifyNonce(const NodeId& src, const void *noncep_) {

    const char *noncep = static_cast<const char *>(noncep_);
    u_int64_t iv;

    iv = fromArray<u_int64_t>((char*)noncep);

    IvMap::iterator itr = ivMap.find(src);
    if (itr == ivMap.end()) {
	itr = ivMap.insert( IvMap::value_type(src, iv-1LL) ).first;
    }

    if (itr->second >= iv) {
	// cout << "new iv " << iv << " is smaller than last iv " << itr->second << endl;
	return false;
    } else {
	// cout << "GOOD! new iv " << iv << " is larger than last iv " << itr->second << endl;

    }

    itr->second = iv;
    if (iv > this->lastIv)
	lastIv = iv;
    return true;
}


bool AesCcmCryptoUnit::decryptDupDetect(const NodeId& src, void *data, size_t data_len, const MemoryBlock sg[]) {

    //    u_int64_t iv;
    bool ret;

    unsigned char* noncep  = (unsigned char*)data + data_len + 16;



    ret = decrypt(data, data_len, sg);
    if (!ret)
	return false;

    if (!verifyNonce(src, noncep)) {

	return false;
    }


    return true;

}




bool AesCcmCryptoUnit::crypt(bool isDecrypt, void *data, size_t data_len, const MemoryBlock sg[]) {

    ret_type ret;

    unsigned char* noncep  = (unsigned char*)data + data_len + 16;

    unsigned char * auth_end = noncep + 16;

    if (sg)
	while(sg[0].p) {
	    size_t s = sg[0].size;
	    memcpy(auth_end, sg[0].p, s);
	    auth_end +=s;
	    sg++;
	}


    ret = CCM_mode(key, 16,
		   noncep,
		   noncep,auth_end - noncep,
		   (unsigned char *)data, data_len,
		   16, // 16 byte MAC
		   isDecrypt);

    return ret >= 0;
}


void AesCcmCryptoUnit::setKey(void *key) {
    memcpy(this->key, key, 16);
}


/* *************************** testing stuff ***************************** */

#include <iostream>
#include <cstring>
#include <sys/types.h>

using namespace std;

void dumpPacket(void* p) {

    const char *pp = (const char *)p;
    cout << "[len=" << *(u_int16_t*)pp << "] '"
	 << (pp + 2 ) << "'" << endl;


}


void AesCcmCryptoUnit::test() {

    NodeId myNodeId(4711);

    char key[16];
    char packet[2000];
    int  data_len;
    int  packet_len;

    const char *text1 = "Das Pferd frisst keinen Gurkensalat.";
    data_len = strlen(text1) + 1;
    memcpy(packet + 2, text1, data_len );
    *(u_int16_t*)packet = data_len;

    packet_len = data_len + 2;

    memcpy(key, "Dies ist der Schluessel", 16);
    setKey(key);

    cout << "testing ... " << endl;

    dumpPacket(packet);

    MemoryBlock mask[2] = {
	{ packet, 1 },
	{ 0, 0 }
    };

    encrypt(packet + 2, packet_len - 2, mask);
    packet[packet_len + 32] = 44;

    packet[1]=1;

    dumpPacket(packet);

    char dupPacket[2000];
    memcpy(dupPacket, packet, 2000);



    if (decryptDupDetect(myNodeId, packet + 2, packet_len-2, mask) ) {
	dumpPacket(packet);
    }

    //key[2]=1;
    //    setKey(key);

    if (!decryptDupDetect(myNodeId, dupPacket + 2, packet_len-2, mask) )
	cout << "ouch" <<endl;

    encrypt(packet + 2, packet_len - 2, mask);
    decryptDupDetect(myNodeId, packet + 2, packet_len-2, mask);

    dumpPacket(packet);



    MemoryBlock mask2[2] = {{packet, packet_len}, {0,0} };

    encrypt(packet + packet_len, 0, mask2);
    packet[packet_len + 32] = 55;

    dumpPacket(packet);

    if (decryptDupDetect(myNodeId, packet + packet_len, 0, mask2) )
	dumpPacket(packet);



    MemoryBlock signMask[3] = {{packet + 4, 2},
			       {packet + 7, 3},
			       {0,0}};

    sign(packet, packet_len, signMask);
    dumpPacket(packet);
    packet[5]=1;
    packet[9]=2;
    if (!verifySignature(myNodeId, packet, packet_len, signMask)) {
	cout << "Signatur is böse" << endl;
    }

}

static void print_usage(const char* name) {

    GEA.dbg() << "usage: " << name
	      << " -k <keyfile>" <<std::endl;

}


extern "C"
#ifdef PIC
int gea_main(int argc, const char  * const *argv)
#else
int aesccm_gea_main(int argc, const char  * const *argv)
#endif
{
    char key[16];

    if (argc >= 3)
	readKeyFromFile(argv[2], key);
    else {
	print_usage(argv[0]);
	return -1;
    }


    ObjRepository& rep = ObjRepository::instance();
    Routing *routing = (Routing *)rep.getObj("awdsRouting");
    if (!routing) {
	GEA.dbg() << "cannot find object 'awdsRouting' in repository" << std::endl;
	return -1;
    }

    AesCcmCryptoUnit *cu = new AesCcmCryptoUnit();
    routing->cryptoUnit = cu;

    cu->setKey(key);

    return 0;
}



/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
