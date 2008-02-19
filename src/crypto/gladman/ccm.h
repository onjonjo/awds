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

 This code implements the CCM combined encryption and authentication mode
 specified by Doug Whiting, Russ Housley and Niels Ferguson.

 The additonal authenicated data in this version is memory resident in a
 single block and is limited to less than 65280 bytes. The length of the
 mesaage data must be less than 2^32 bytes unless LONG_MESSAGES has been
 defined. This can be input in multiple blocks.
*/

#ifdef __cplusplus

extern "C" {

#endif



#include <memory.h>

#include "aes.h"

#define BLOCK_MASK  (AES_BLOCK_SIZE - 1)

#ifndef LONG_MESSAGES
  typedef   aes_32t             mlen_type;
  typedef   long                ret_type;
#else
  #ifdef _MSC_VER
    typedef __int64             ret_type;
    typedef unsigned __int64    mlen_type;
  #else
    typedef long long           ret_type;
    typedef unsigned long long  mlen_type;
  #endif
#endif

/* CCM error codes  */

#define CCM_ok                       0
#define CCM_bad_key                 -1
#define CCM_bad_auth_field_length   -2
#define CCM_bad_auth_data_length    -3
#define CCM_auth_failure            -4
#define CCM_msg_length_error        -5

/* The CCM context  */

typedef struct
{   aes_08t         blk[AES_BLOCK_SIZE];    /* counter block                        */
    aes_08t         cbc[AES_BLOCK_SIZE];    /* running CBC value                    */
    aes_08t         sii[AES_BLOCK_SIZE];    /* encrypted counter block              */
    aes_32t         af_len;                 /* authentication field length (bytes)  */
    mlen_type       md_len;                 /* message length (bytes)               */
    mlen_type       cnt;                    /* message position counter             */
    aes_encrypt_ctx aes[1];                 /* AES context                          */
} CCM_ctx;

/* This call initialises the CCM context by setting the encryption key
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
   to process long messages but the overall sum of the data lengths for
   the set of calls must match the message length (encrypt) or the sum
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
    CCM_ctx ctx[1]);                                 /* the CCM context                   */

/* Encrypt 'len' bytes data from imsg[] to omsg[]. This call can be repeated
   for multiple blocks provided that the sum of the individual block lengths
   is equal to the overall message length set by the call to CCM_init(). The
   last (omsg) block for which this routine is called must have space for the
   authentication field bytes in addition to the message bytes (i.e. omsg[]
   for the last call must have a length of 'len' + 'auth_field_len' bytes).
   If the return value is negative an error has occurred, otherwise the number
   of bytes written to omsg[] is returned.
*/

ret_type CCM_encrypt(unsigned char mbuf[],	/* the plaintext input message      */
					 mlen_type len,         /* the length of this block (bytes) */
					 CCM_ctx ctx[1]);       /* the CCM context                  */

/* Decrypt 'len' bytes data from imsg[] to omsg[]. This call can be repeated
   for multiple blocks provided that the sum of the individual block lengths
   is equal to the sum of the message length and the authentication field
   length set in the call to CCM_init(). The last block for which this routine
   is called must include the _complete_ authentication field (i.e. imsg[] for
   the last call must have a length of 'len' + 'auth_field_len' bytes). If the
   return value is negative an error has occurred, otherwise the number of
   bytes written to omsg[] is returned.
*/

ret_type CCM_decrypt(unsigned char mbuf[],	/* the plaintext input message      */
					 mlen_type len,         /* the length of this block (bytes) */
					 CCM_ctx ctx[1]);       /* the CCM context                  */

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
    int ed_flag);                                    /* 0 = encrypt, 1 = decrypt          */

#ifdef __cplusplus

}

#endif
