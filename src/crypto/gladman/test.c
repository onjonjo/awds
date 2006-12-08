
//==========================================================================
// Proposed AES CTR/CBC-MAC mode test vector generation
//
// 11-02-001r2-I-AES-Encryption & Authentication-Using-CTR-Mode-with-CBC-MAC
//
// Author:  Doug Whiting, Hifn  <dwhiting@hifn.com>
//          modified by Brian Gladman <brg@gladman.me.uk>
//
// This code is a modified version of the original file ccm.c authored by
// Doug Whiting of Hifn <dwhiting@hifn.com>. 
//
// It has been modified by Brian Gladman <brg@gladman.me.uk> to act as a 
// test file for his own implementation of CCM.

// This code is released to the public domain, on an as-is basis.
// 
//==========================================================================
// Release date 14th July 2002

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "ccm.h"

/*#ifdef	_SHOW_
#define	_VERBOSE_	1
#elif !defined(_VERBOSE_)
#define	_VERBOSE_	0
#endif
*/ 
/* show what compiler we used */

#if   defined(__BORLANDC__)	
#define	COMPILER_ID "Borland"
#define	LITTLE_ENDIAN	1
#elif defined(_MSC_VER)
#define	COMPILER_ID "Microsoft"
#define	LITTLE_ENDIAN	1
#elif defined(__GNUC__)
#define	COMPILER_ID "GNU"
#ifndef BIG_ENDIAN		/* assume gcc = little-endian, unless told otherwise */
#define	LITTLE_ENDIAN	1
#endif
#else					/* assume big endian, if compiler is unknown */
#define	COMPILER_ID "Unknown"
#endif

#include "aes.h"			// AES calling interface

#define Lo8(x) ((aes_08t) ((x) & 0xFF))

typedef int BOOL;           // boolean

enum
{   MAX_PACKET  =   3*512,  // largest packet size
    N_RESERVED  =   0,      // reserved nonce octet value
    A_DATA      =   0x40,   // the Adata bit in the flags
    M_SHIFT     =   3,      // how much to shift the 3-bit M field
    L_SHIFT     =   0,      // how much to shift the 3-bit L field
    L_SIZE      =   2       // size of the l(m) length field (in octets)
};

typedef union				// AES cipher block
{	aes_32t x[AES_BLOCK_SIZE/4];	// access as 8-bit octets or 32-bit words
    aes_08t  b[AES_BLOCK_SIZE];
} block;

typedef struct
{	BOOL		encrypted;					// TRUE if encrypted
    aes_08t		TA[6];						// xmit address
    int			micLength;					// # octets of MIC appended to plaintext (M)
    int			clrCount;					// # cleartext octets covered by MIC
    aes_32t	pktNum[2];					// unique packet sequence number (like WEP IV)
    block		key;						// the encryption key (K)
    int			length;						// # octets in data[]
    aes_08t    data[MAX_PACKET+2*AES_BLOCK_SIZE];	// packet contents
} packet;

struct
{	int     ctr_cnt;	// how many words left in ctr_enc
    block   ctr_blk;    // the counter input
    block   ctr_enc;	// the ciphertext (prng output)
} prng;

void init_rand(aes_32t seed)
{
    memset(prng.ctr_blk.b, 0, AES_BLOCK_SIZE);
	seed *= 17;
#ifdef	BIG_ENDIAN
	prng.ctr_blk.b[AES_BLOCK_SIZE - 1] = (aes_08t)(seed);
	prng.ctr_blk.b[AES_BLOCK_SIZE - 2] = (aes_08t)(seed >> 8);
	prng.ctr_blk.b[AES_BLOCK_SIZE - 3] = (aes_08t)(seed >> 16);
	prng.ctr_blk.b[AES_BLOCK_SIZE - 4] = (aes_08t)(seed >> 24);
#else
	prng.ctr_blk.b[AES_BLOCK_SIZE - 4] = (aes_08t)(seed);
	prng.ctr_blk.b[AES_BLOCK_SIZE - 3] = (aes_08t)(seed >> 8);
	prng.ctr_blk.b[AES_BLOCK_SIZE - 2] = (aes_08t)(seed >> 16);
	prng.ctr_blk.b[AES_BLOCK_SIZE - 1] = (aes_08t)(seed >> 24);
#endif
    prng.ctr_cnt = 0;
    }

aes_32t rand_32(aes_encrypt_ctx aes[1])
{	int			i;
	aes_32t	x;

    if(prng.ctr_cnt == 0)
    {
		// use whatever key is currently defined
        prng.ctr_cnt = AES_BLOCK_SIZE / 4;
		
		for(i = 0; i < 16; ++i)
			if(++(prng.ctr_blk.b[i]))
				break;

		aes_encrypt(prng.ctr_blk.b, prng.ctr_enc.b, aes);
    }

    --(prng.ctr_cnt);

	for(i = x = 0; i < 4; ++i)	// return in big-endian format
		x = (x << 8) + prng.ctr_enc.b[4 * prng.ctr_cnt + i];

	return x;
}

void show_block(const block *blk, const char *prefix, const char *suffix, int a)
{	int		i, blkSize = AES_BLOCK_SIZE;
    
	printf(prefix, a);
    
	if(suffix == NULL) 
	{ 
		suffix = "\n"; blkSize = a; 
	}
    
	for(i = 0; i < blkSize; ++i)
        printf("%02X%s", blk->b[i], (i & 3) == 3 ? "  " : " ");
    
	printf(suffix);
}

void show_addr(const packet *p)
{	int	i;

    printf("      TA = ");

	for(i = 0; i < 6; ++i) 
		printf("%02X%s", p->TA[i], i == 3 ? "  " : " ");

    printf("  48-bit pktNum = %04X.%08X\n", p->pktNum[1], p->pktNum[0]);
}

void show_packet(const packet *p, const char *pComment, int a)
{	int		i;

    printf("Total packet length = %4d. ",p->length);
    printf(pComment,a);
    if(p->encrypted) 
		printf("[Encrypted]");
    
	for(i = 0; i < p->length; ++i)
	{
        if(!(i & 15)) 
			printf("\n%11s", "");

        printf("%02X%s", p->data[i], (i & 3) == 3 ? "  " : " ");
    }
    
	printf("\n");
}

// assumes AES_SetKey is called elsewhere
void Generate_CTR_CBC_Vector(packet *p,int verbose, aes_encrypt_ctx aes[1])
    {
    int     i,j,len,needPad,blkNum;
    block   m,x,T;

    assert(p->length >= p->clrCount && p->length <= MAX_PACKET);
    assert(p->micLength > 0 && p->micLength <= AES_BLOCK_SIZE);
    len = p->length - p->clrCount;		// l(m)
    
    //---- generate the first AES block for CBC-MAC
    m.b[ 0] = (aes_08t) ((L_SIZE-1) << L_SHIFT) +      // flags octet
                     ((p->clrCount) ? A_DATA : 0) + (((p->micLength - 2) / 2 << M_SHIFT));
    
	m.b[ 1] = N_RESERVED;				// reserved nonce octet 
    m.b[ 2] = Lo8(p->pktNum[1] >> 8);   // 48 bits of packet number ("IV")
    m.b[ 3] = Lo8(p->pktNum[1]);
    m.b[ 4] = Lo8(p->pktNum[0] >>24);
    m.b[ 5] = Lo8(p->pktNum[0] >>16);
    m.b[ 6] = Lo8(p->pktNum[0] >> 8);
    m.b[ 7] = Lo8(p->pktNum[0]);
    
	m.b[ 8] = p->TA[0];                 // 48 bits of transmitter address
    m.b[ 9] = p->TA[1];
    m.b[10] = p->TA[2];
    m.b[11] = p->TA[3];
    m.b[12] = p->TA[4];
    m.b[13] = p->TA[5];
    
	m.b[14] = Lo8(len >> 8);            // l(m) field
    m.b[15] = Lo8(len);

    //---- compute the CBC-MAC tag (MIC)
    aes_encrypt(m.b, x.b, aes);			// produce the CBC IV
    show_block(&m, "CBC IV in: ", "\n", 0);
    if(verbose) 
		show_block(&x, "CBC IV out:", "\n", 0);
    j = 0;								// j = octet counter inside the AES block
	if(p->clrCount)						// is there a header?
	{									// if so, "insert" length field: l(a)
        assert(p->clrCount < 0xFFF0);   // [don't handle larger cases (yet)]
        x.b[j++] ^= (p->clrCount >> 8) & 0xFF;
        x.b[j++] ^= p->clrCount & 0xFF;
    }
    
	for(i = blkNum = 0; i < p->length; ++i)    
	{									// do the CBC-MAC processing
        x.b[j++] ^= p->data[i];         // perform the CBC xor
        needPad = (i == p->clrCount - 1) || (i == p->length - 1);

        if(j == AES_BLOCK_SIZE || needPad)	// full block, or hit pad boundary
		{
			if(verbose) 
				show_block(&x, "After xor: ", (i >= p->clrCount) ? " [msg]\n" : " [hdr]\n",blkNum);

            aes_encrypt(x.b, x.b, aes);	// encrypt the CBC-MAC block, in place
            if(verbose) 
				show_block(&x, "After AES: ", "\n", blkNum);
            blkNum++;                   // count the blocks
            j = 0;                      // the block is now empty
		}
	}      
    
	memcpy(T.b, x.b, p->micLength);		// save the MIC tag 
    show_block(&T, "MIC tag  : ", NULL, p->micLength);

    //---- encrypt the data packet using CTR mode
    m.b[0] &= ~(A_DATA | (7 << M_SHIFT));	// clear flag fields for counter mode
    
	for(i = blkNum = 0; i + p->clrCount < p->length; ++i)
	{
        if(!(i % AES_BLOCK_SIZE))
		{								// generate new keystream block
            blkNum++;                   // start data with block #1
            m.b[14] = blkNum / 256;
            m.b[15] = blkNum % 256;
            aes_encrypt(m.b, x.b, aes);	// then encrypt the counter
            
			if(verbose && i == 0) 
				show_block(&m, "CTR Start: ", "\n", 0);
            if(verbose) 
				show_block(&x, "CTR[%04X]: " , "\n", blkNum);
		}
		
		p->data[i+p->clrCount] ^= x.b[i % AES_BLOCK_SIZE];    // merge in the keystream
	}

    //---- truncate, encrypt, and append MIC to packet
    m.b[14] = m.b[15] = 0;              // use block counter value zero for tag
    aes_encrypt(m.b, x.b, aes);         // encrypt the counter
    
	if(verbose) 
		show_block(&x, "CTR[MIC ]: ", NULL, p->micLength);
    
	for(i = 0; i < p->micLength; ++i)  
        p->data[p->length + i] = T.b[i] ^ x.b[i];

    p->length += p->micLength;			// adjust packet length accordingly
    p->encrypted = 1;
}

int main(int argc,char *argv[])
{	int				i, j, k, len, pktNum, seed, ll;
    packet			p;
	aes_encrypt_ctx	aes[1];
	unsigned char	nonce[14], dat[128], ref[128];

    seed = (argc > 1) ? atoi(argv[1]) : (int) time(NULL);
    init_rand(seed);
    
	printf("%s C compiler [%s %s].%s\nRandom seed = %d\n", COMPILER_ID,__DATE__,__TIME__,
#ifdef LITTLE_ENDIAN
	   " Little-endian.",
#else
	   " Big-endian.",
#endif
	   seed);

    // generate CTR-CBC vectors for various parameter settings
    
	for(k = pktNum = 0; k < 2; ++k)
	{   // k==1 --> random vectors. k==0 --> "visually simple" vectors
        
		for(i = 0; i < AES_BLOCK_SIZE; ++i)          
			p.key.b[i] = (k) ? (aes_08t) rand_32(aes) & 0xFF : i + 0xC0;
        
		for(i = 0; i < 6; ++i)          
            p.TA[i]    = (k) ? (aes_08t) rand_32(aes) & 0xFF : i + 0xA0;
        
		aes_encrypt_key128(p.key.b, aes);     // run the key schedule

        // now generate the vectors
        for(p.micLength = 8; p.micLength < 12; p.micLength += 2)
         for(p.clrCount = 8; p.clrCount < 16; p.clrCount += 4)
          for(len = 32; len < 64; len *= 2)
           for(i = -1; i < 2; ++i)
			{
				p.pktNum[0] = (k) ? rand_32(aes) : pktNum * 0x01010101 + 0x03020100;
				p.pktNum[1] = (k) ? rand_32(aes) & 0xFFFF : 0;    // 48-bit IV

				p.length = len + i;				// len+i is packet length
				p.encrypted = 0;
				assert(p.length <= MAX_PACKET);
            
				for(j = 0; j < p.length; ++j)	// generate random packet contents
					p.data[j] = (k) ? rand_32(aes) : j;
				pktNum++;
            
				printf("=============== Packet Vector #%d ==================\n",pktNum);
				show_block(&p.key , "AES Key:   ", "\n", 0);
				show_addr(&p);

				nonce[ 0] = 0;
				nonce[ 1] = Lo8(p.pktNum[1] >> 8);   // 48 bits of packet number ("IV")
				nonce[ 2] = Lo8(p.pktNum[1]);
				nonce[ 3] = Lo8(p.pktNum[0] >>24);
				nonce[ 4] = Lo8(p.pktNum[0] >>16);
				nonce[ 5] = Lo8(p.pktNum[0] >> 8);
				nonce[ 6] = Lo8(p.pktNum[0]);

				nonce[ 7] = p.TA[0];                 // 48 bits of transmitter address
				nonce[ 8] = p.TA[1];
				nonce[ 9] = p.TA[2];
				nonce[10] = p.TA[3];
				nonce[11] = p.TA[4];
				nonce[12] = p.TA[5];
				
				memcpy(ref, p.data, p.length);
				memcpy(dat, p.data, p.length); ll = p.length - p.clrCount;

			    show_packet(&p, "[Input (%d cleartext header octets)]", p.clrCount);
				Generate_CTR_CBC_Vector(&p, 1, aes);
			    show_packet(&p, "", 0);				// show the final encrypted packet

#ifdef MULTIPLE_CALL
				{	CCM_ctx	ctx[1];
					CCM_init( p.key.b, AES_BLOCK_SIZE, nonce, dat, p.clrCount, ll, p.micLength, ctx);
					CCM_encrypt(dat + p.clrCount, dat + p.clrCount, ll, ctx);
				}
#else
				CCM_mode( p.key.b, AES_BLOCK_SIZE, nonce, dat, p.clrCount, dat + p.clrCount, ll,
																p.micLength, 0);
#endif
				if(memcmp(p.data, dat, p.length))
				{	
					printf("bad data (encryption)"); exit(1);
				}

#ifdef MULTIPLE_CALL
				{	CCM_ctx	ctx[1];
					CCM_init(p.key.b, AES_BLOCK_SIZE, nonce, dat, p.clrCount, ll, p.micLength, ctx);
					CCM_decrypt(dat + p.clrCount, dat + p.clrCount, ll + p.micLength, ctx);
				}
#else
				CCM_mode( p.key.b, AES_BLOCK_SIZE, nonce, dat, p.clrCount, dat + p.clrCount, ll,
																p.micLength, 1);
#endif
				if(memcmp(ref, dat, ll + p.clrCount))
				{	
					printf("bad data (decryption)"); exit(1);
				}
            }
	}

	return 0;
}
