/*
 ---------------------------------------------------------------------------
 Copyright (c) 2003, Dr Brian Gladman <brg@gladman.me.uk>, Worcester, UK.
 All rights reserved.

 LICENSE TERMS

 The free distribution and use of this software in both source and binary
 form is allowed (with or without changes) provided that:

   1. distributions of this source code include the above copyright
      notice, this list of conditions and the following disclaimer;

   2. distributions in binary form include the above copyright
      notice, this list of conditions and the following disclaimer
      in the documentation and/or other associated materials;

   3. the copyright holder's name is not used to endorse products
      built using this software without specific written permission.

 ALTERNATIVELY, provided that this notice is retained in full, this product
 may be distributed under the terms of the GNU General Public License (GPL),
 in which case the provisions of the GPL apply INSTEAD OF those given above.

 DISCLAIMER

 This software is provided 'as is' with no explicit or implied warranties
 in respect of its properties, including, but not limited to, correctness
 and/or fitness for purpose.
 ---------------------------------------------------------------------------
 Issue Date: 1/05/2003

 This code implements the CCM combined encryption and authentication 
 mode specified by Doug Whiting, Russ Housley and Niels Ferguson. 

 The additonal authenicated data in this version is memory resident in 
 a single block and is limited to less than 65536 - 256 (= 65280) bytes. 
 The message data is limited to less 2^32 bytes but can be processed in
 either a single memory resident block or in multiple smaller blocks.

 My thanks to Erik Andersen <andersen@codepoet.org> for finding a bug in
 an earlier relaease of this code.
*/

#include "ccm.h"

#ifndef LONG_MESSAGES   /* messages less than 2^32 bytes    */

/* set the 4, 3 or 2 byte CTR value (switch fall through is correct)    */

#define set_ctr(x,v)                            \
    switch(x->blk[0]) {                         \
    case 3: x->blk[12] = (aes_08t)((v) >> 24);  \
    case 2: x->blk[13] = (aes_08t)((v) >> 16);  \
    case 1: x->blk[14] = (aes_08t)((v) >>  8);  \
            x->blk[15] = (aes_08t) (v); }

/* increment the 4, 3 or 2 byte CTR value   */

#define inc_ctr(x)      \
    switch(x->blk[0]) { \
    case 3: (void)(!++(x->blk[15]) && !++(x->blk[14]) && !++(x->blk[13]) &&  ++(x->blk[12]) ); break; \
    case 2: (void)(!++(x->blk[15]) && !++(x->blk[14]) &&  ++(x->blk[13]) ); break; \
    case 1: (void)(!++(x->blk[15]) &&  ++(x->blk[14]) ); break; }

#else

/* set the 8, 7, 6, 5, 4, 3 or 2 byte CTR value (switch fall through is correct)    */

#define set_ctr(x,v)                            \
    switch(x->blk[0]) {                         \
    case 7: x->blk[12] = (aes_08t)((v) >> 56);  \
    case 6: x->blk[12] = (aes_08t)((v) >> 48);  \
    case 5: x->blk[12] = (aes_08t)((v) >> 40);  \
    case 4: x->blk[12] = (aes_08t)((v) >> 32);  \
    case 3: x->blk[12] = (aes_08t)((v) >> 24);  \
    case 2: x->blk[13] = (aes_08t)((v) >> 16);  \
    case 1: x->blk[14] = (aes_08t)((v) >>  8);  \
            x->blk[15] = (aes_08t) (v); }

/* increment the 8, 7, 6, 5, 4, 3 or 2 byte CTR value   */

#define inc_ctr(x)							\
	switch(x->blk[0]) {						\
	case 7: if (!++(x->blk[15]) && !++(x->blk[14]) && !++(x->blk[13]) && !++(x->blk[12]) && \
		    !++(x->blk[11]) && !++(x->blk[10]) && !++(x->blk[ 9]) &&  ++(x->blk[ 8]) ); break; \
	case 6: if (!++(x->blk[15]) && !++(x->blk[14]) && !++(x->blk[13]) && !++(x->blk[12]) && \
		    !++(x->blk[11]) && !++(x->blk[10]) &&  ++(x->blk[ 9]) ); break; \
	case 5: if (!++(x->blk[15]) && !++(x->blk[14]) && !++(x->blk[13]) && !++(x->blk[12]) && \
		    !++(x->blk[11]) &&  ++(x->blk[10]) ); break;	\
	case 4: if (!++(x->blk[15]) && !++(x->blk[14]) && !++(x->blk[13]) && !++(x->blk[12]) && \
		    ++(x->blk[11]) ); break;				\
	case 3: if (!++(x->blk[15]) && !++(x->blk[14]) && !++(x->blk[13]) &&  ++(x->blk[12]) ); break; \
	case 2: if (!++(x->blk[15]) && !++(x->blk[14]) &&  ++(x->blk[13]) ); break; \
	case 1: if (!++(x->blk[15]) &&  ++(x->blk[14]) ); break; }

#endif

/*  This call initialises the CCM context by setting the encryption key 
    and setting up and calculating the the CBC value for the additional 
    authenication data. 
   
    This call is then followed by calls to CCM_encrypt() or CCM_decrypt() 
    to encrypt or decrypt the message bytes. Length values are in bytes. 
   
    The message length value for CCM_init() is the actual number of bytes 
    in the message without the authentication data. This is also true for 
    CCM_encrypt() but the length for CCM_decrypt() is the message length 
    plus the length in bytes of the added authentication data field that 
    is stored at the end of the message.

    CCM_encrypt() and CCM_decrypt() can be called more than once in order
    to process long messages but the overall sum of the lengths for the 
    set of calls used must match the message length (encrypt) or the sum
    of the message length and the authentication field length (decrypt).

    The number of bytes required in the nonce[] array depends on how many 
    bytes are needed to represent the message length in bytes. If 2, 3 or 
    4 bytes are needed to store the message length, then the number of 
    nonce bytes needed is 13, 12 or 11 respectively.
*/

ret_type CCM_init(
    const unsigned char key[], unsigned long key_len,/* the key value to be used          */
    const unsigned char nonce[],                     /* the nonce value                   */
    const unsigned char auth[], unsigned long ad_len,/* the additional authenticated data */
    const mlen_type msg_len,                         /* message data length               */
    const unsigned long auth_field_len,              /* the authentication field length   */
    CCM_ctx ctx[1])                                  /* the CCM context                   */
{   aes_32t    cnt;

	if(key_len != 16 && key_len != 24 && key_len != 32)
		return CCM_bad_key;

	aes_encrypt_key(key, key_len, ctx->aes);	

	if(auth_field_len < 2 || auth_field_len > 16 || (auth_field_len & 1))
        return CCM_bad_auth_field_length;   /* illegal authentication field size    */

    if(ad_len >= 65536ul - 256ul)
        return CCM_bad_auth_data_length;    /* too much added authetication data    */

    /* save length values and compile the blocks for the running CBC and CTR values */
    ctx->md_len = msg_len;
    ctx->af_len = auth_field_len;

#ifndef LONG_MESSAGES
    ctx->blk[0] = (ctx->md_len & 0xff000000 ? 3 : 
                   ctx->md_len & 0xffff0000 ? 2 : 1);
#else
    ctx->blk[0] = (ctx->md_len & 0xff00000000000000 ? 7 : 
                   ctx->md_len & 0xffff000000000000 ? 6 : 
                   ctx->md_len & 0xffffff0000000000 ? 5 : 
                   ctx->md_len & 0xffffffff00000000 ? 4 : 
                   ctx->md_len & 0xffffffffff000000 ? 3 : 
                   ctx->md_len & 0xffffffffffff0000 ? 2 : 1);
#endif

    /* move the nonce into the block    */
    for(cnt = 0; cnt < (aes_32t)AES_BLOCK_SIZE - ctx->blk[0] - 2; ++cnt)
        ctx->blk[cnt + 1] = nonce[cnt];

    set_ctr(ctx, ctx->md_len);                  /* set message length value     */
    memcpy(ctx->cbc, ctx->blk, AES_BLOCK_SIZE); /* and copy into running CBC    */
    ctx->cbc[0] |= (ad_len ? 0x40 : 0) + ((auth_field_len - 2) << 2);
    set_ctr(ctx, 1);                            /* initial counter value = 1    */

    aes_encrypt(ctx->cbc, ctx->cbc, ctx->aes);  /* encrypt the cbc block    */
    aes_encrypt(ctx->blk, ctx->sii, ctx->aes);  /* encrypt counter block    */

    if(ad_len)              /* if there is additional authentication data   */
    {
        cnt = 0;            /* set the two byte length field for the data   */
        ctx->cbc[0] ^= (aes_08t)(ad_len >> 8);
        ctx->cbc[1] ^= (aes_08t) ad_len;
        
        while(cnt < ad_len) /* perform the CBC calculation on the data      */
        {   
            /* xor data into the running CBC block                          */
            ctx->cbc[(cnt + 2) & BLOCK_MASK] ^= auth[cnt];

            /* if CBC block is full or at end of the authentication data    */
            if(!((++cnt + 2) & BLOCK_MASK) || cnt == ad_len)
                aes_encrypt(ctx->cbc, ctx->cbc, ctx->aes); 
        }
    }

    ctx->cnt = 0;
    return CCM_ok;
}

/*  Encrypt 'len' bytes data from imsg[] to omsg[]. This call can be repeated 
    for multiple blocks provided that the sum of the individual block lengths 
    is equal to the overall message length set in the call to CCM_init(). The
    last omsg[] buffer for which this routine is called must have extra space 
    for the authentication field bytes in addition to the message bytes (i.e. 
    the omsg[] buffer for the last call must have a length that is the sum of
    the input parameter 'len' and the 'auth_field_len' set when CCM_init() is
    called.  If the return value is negative an error has occurred, otherwise 
    the total number of bytes written to omsg[] is returned.
*/

#define lp(x)       ((unsigned long*)(x))         /* cast to unsigned long pointer	*/
#define aligned(x)  (!(((unsigned long)(x)) & 3)) /* check for long word alignment	*/

ret_type CCM_encrypt(unsigned char mbuf[],		/* the plaintext input message      */ 
					 mlen_type len,             /* the length of this block (bytes) */
					 CCM_ctx ctx[1])            /* the CCM context                  */

{   mlen_type   cnt = 0, b_pos = (ctx->cnt & AES_BLOCK_SIZE);

    if(ctx->cnt + len > ctx->md_len)
        return CCM_msg_length_error;

	if(((mbuf - ctx->blk) & 3) == 0)
	{
		unsigned long	*mp;

		while(!aligned(mbuf + cnt) && cnt < len && b_pos < AES_BLOCK_SIZE)
		{
			ctx->cbc[b_pos] ^= mbuf[cnt];		/* update the CBC   */
			mbuf[cnt++] ^= ctx->sii[b_pos++];	/* encrypt message  */
		}

		mp = lp(mbuf + cnt);
		if(b_pos >>= 2)
		{
			while(b_pos < (AES_BLOCK_SIZE >> 2) && cnt + 4 < len)
			{
				lp(ctx->cbc)[b_pos] ^= *mp; 
				*mp++ ^= lp(ctx->sii)[b_pos++];
				cnt += 4;
			}
		
			if(b_pos == (AES_BLOCK_SIZE >> 2))
			{
			        b_pos = 0; 
				inc_ctr(ctx); 
				aes_encrypt(ctx->blk, ctx->sii, ctx->aes);
				aes_encrypt(ctx->cbc, ctx->cbc, ctx->aes);
			}
		}

        while(cnt + AES_BLOCK_SIZE < len)
        {   
            lp(ctx->cbc)[0] ^= *mp; *mp++ ^= lp(ctx->sii)[0]; 
            lp(ctx->cbc)[1] ^= *mp; *mp++ ^= lp(ctx->sii)[1]; 
            lp(ctx->cbc)[2] ^= *mp; *mp++ ^= lp(ctx->sii)[2]; 
            lp(ctx->cbc)[3] ^= *mp; *mp++ ^= lp(ctx->sii)[3]; 
            inc_ctr(ctx);
            aes_encrypt(ctx->blk, ctx->sii, ctx->aes);
            aes_encrypt(ctx->cbc, ctx->cbc, ctx->aes);
            cnt += 16;
        }
	}

	while(cnt + AES_BLOCK_SIZE < len)
	{
		while(b_pos < AES_BLOCK_SIZE)
		{
	        ctx->cbc[b_pos] ^= mbuf[cnt];				/* update the CBC   */
		    mbuf[cnt++] ^= ctx->sii[b_pos++];			/* encrypt message  */
		}

		b_pos = 0; inc_ctr(ctx); 
        aes_encrypt(ctx->blk, ctx->sii, ctx->aes);  /* encrypt the CTR value    */
        aes_encrypt(ctx->cbc, ctx->cbc, ctx->aes);  /* encrypt the running CBC  */
	}

	while(cnt < len)
    {
        ctx->cbc[b_pos] ^= mbuf[cnt];		/* update the CBC   */
        mbuf[cnt++] ^= ctx->sii[b_pos++];	/* encrypt message  */

        if(b_pos == AES_BLOCK_SIZE)	/* if the current encryption block is full  */
        {
			b_pos = 0; inc_ctr(ctx); 
            aes_encrypt(ctx->blk, ctx->sii, ctx->aes);  /* encrypt the CTR value    */
            aes_encrypt(ctx->cbc, ctx->cbc, ctx->aes);  /* encrypt the running CBC  */
        }
    }

    ctx->cnt += cnt;
    if(ctx->cnt == ctx->md_len)			/* if at end of message         */
    {
        if(b_pos)						/* if a partial block remains   */
            aes_encrypt(ctx->cbc, ctx->cbc, ctx->aes);

        set_ctr(ctx, 0);                            /* set CTR to zero  */
        aes_encrypt(ctx->blk, ctx->sii, ctx->aes);  /* encrypt the CTR  */

        /* encrypt and store the authentication value   */
        for(b_pos = 0; b_pos < ctx->af_len; ++b_pos)
            mbuf[cnt + b_pos] = ctx->cbc[b_pos] ^ ctx->sii[b_pos];

        cnt += ctx->af_len;
    }
    
    return (ret_type)cnt;
}

/*  Decrypt 'len' bytes data from imsg[] to omsg[]. This call can be repeated 
    for multiple blocks provided that the sum of the individual block lengths 
    is equal to the sum of the message length and the authentication field
    length set in the call to CCM_init(). The last block for which this routine
    is called must include the _complete_ authentication field (i.e. the 'len'
    parameter for this call must include the remaining ciphertext and the full
    authentication field length set by 'auth_field_len' in the CCM_init() call.
    If the return value is negative an error has occurred, otherwise the number 
    of bytes written to omsg[] in the current call is returned.

    NOTE: this implementation is not fully compliant with the CCM specification
    when multiple calls to CCM_encrypt() are used because, in the event that an
    authentication error is detected when the last block is processed,  earlier
    decrypted blocks will already have been returned to the caller, which is in 
    violation of the specification.  This is costly to avoid for large messages 
    that cannot be memory resident as a single block - in this case the message
    would have to be processed twice so that the final authentication value can 
    be checked before the output is provided on a second pass.
*/

ret_type CCM_decrypt(unsigned char mbuf[],	/* the plaintext input message      */ 
					 mlen_type len,         /* the length of this block (bytes) */
					 CCM_ctx ctx[1])        /* the CCM context                  */
{   mlen_type   cnt = 0, b_pos = (ctx->cnt & AES_BLOCK_SIZE);
    
    if(ctx->cnt + len > ctx->md_len)
    {
        if(ctx->cnt + len != ctx->md_len + ctx->af_len)
            return CCM_msg_length_error;
		len = ctx->md_len - ctx->cnt;
    }
    
	if(((mbuf - ctx->blk) & 3) == 0)
	{
		unsigned long	*mp;

		while(!aligned(mbuf + cnt) && cnt < len && b_pos < AES_BLOCK_SIZE)
		{
	        mbuf[cnt] ^= ctx->sii[b_pos];		/* decrypt message  */ 
			ctx->cbc[b_pos++] ^= mbuf[cnt++];	/* update the CBC   */
		}

		mp = lp(mbuf + cnt);
		if(b_pos >>= 2)
		{
			while(b_pos < (AES_BLOCK_SIZE >> 2) && cnt + 4 < len)
			{
	            lp(ctx->cbc)[b_pos] ^= (*mp++ ^= lp(ctx->sii)[b_pos]);
				b_pos++; cnt += 4;
			}
		
			if(b_pos == (AES_BLOCK_SIZE >> 2))
			{
				b_pos = 0; inc_ctr(ctx); 
				aes_encrypt(ctx->blk, ctx->sii, ctx->aes);
				aes_encrypt(ctx->cbc, ctx->cbc, ctx->aes);
			}
		}

        while(cnt + AES_BLOCK_SIZE < len)
        {   
            lp(ctx->cbc)[0] ^= (*mp++ ^= lp(ctx->sii)[0]);
            lp(ctx->cbc)[1] ^= (*mp++ ^= lp(ctx->sii)[1]);
            lp(ctx->cbc)[2] ^= (*mp++ ^= lp(ctx->sii)[2]);
            lp(ctx->cbc)[3] ^= (*mp++ ^= lp(ctx->sii)[3]);
            inc_ctr(ctx);
            aes_encrypt(ctx->blk, ctx->sii, ctx->aes);
            aes_encrypt(ctx->cbc, ctx->cbc, ctx->aes);
            cnt += 16;
        }
	}

    while(cnt < len)
    {
        mbuf[cnt] ^= ctx->sii[b_pos];		/* decrypt message  */ 
        ctx->cbc[b_pos++] ^= mbuf[cnt++];	/* update the CBC   */
        
        if(b_pos == AES_BLOCK_SIZE)		/* if the current encryption block is full  */
        {
            b_pos = 0; inc_ctr(ctx);
            aes_encrypt(ctx->blk, ctx->sii, ctx->aes);  /* encrypt the CTR value    */
            aes_encrypt(ctx->cbc, ctx->cbc, ctx->aes);  /* encrypt the running CBC  */
        }
    }
	
	ctx->cnt += cnt;
    if(ctx->cnt == ctx->md_len)			/* if at end of message         */
    {       
        if(b_pos)						/* if a partial block remains   */
            aes_encrypt(ctx->cbc, ctx->cbc, ctx->aes);

        set_ctr(ctx, 0);                            /* set CTR to zero  */
        aes_encrypt(ctx->blk, ctx->sii, ctx->aes);  /* encrypt the CTR  */

        /* verify the encrypted authentication value */
        for(b_pos = 0; b_pos < ctx->af_len; ++b_pos)
            if(mbuf[cnt + b_pos] != (ctx->cbc[b_pos] ^ ctx->sii[b_pos]))
            {   
                /* if bad clear the message and authentication field    */
                memset(mbuf, 0, (size_t)cnt + ctx->af_len);
                return CCM_auth_failure;
            }
    }

    return (ret_type)cnt;
}

/* Perform an encryption or decryption in one call.  The message length is the 
   plaintext message length without the authentication field for both the 
   encryption and decryption calls. If the return value is negative an error 
   has occurred, otherwise the number of bytes written to omsg[] is returned.
*/

ret_type CCM_mode(
    const unsigned char key[], unsigned long key_len,/* the key value to be used          */
    const unsigned char nonce[],                     /* the nonce value                   */
    const unsigned char auth[], unsigned long ad_len,/* the additional authenticated data */
    unsigned char msg[], mlen_type msg_len,          /* the message data                  */
    unsigned long auth_field_len,                    /* the authentication field length   */ 
    int ed_flag)                                     /* 0 = encrypt, 1 = decrypt          */
{   CCM_ctx ctx[1];
    ret_type    ec;

    ec = CCM_init(key, key_len, nonce, auth, ad_len, msg_len, auth_field_len, ctx);
    return ec != CCM_ok ? ec : ed_flag 
        ? CCM_decrypt(msg, msg_len + auth_field_len, ctx) 
        : CCM_encrypt(msg, msg_len, ctx);
}

/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 8
 * End:
 */
