
; ---------------------------------------------------------------------------
; Copyright (c) 2002, Dr Brian Gladman <brg@gladman.me.uk>, Worcester, UK.
; All rights reserved.
;
; LICENSE TERMS
;
; The free distribution and use of this software in both source and binary
; form is allowed (with or without changes) provided that:
;
;   1. distributions of this source code include the above copyright
;      notice, this list of conditions and the following disclaimer;
;
;   2. distributions in binary form include the above copyright
;      notice, this list of conditions and the following disclaimer
;      in the documentation and/or other associated materials;
;
;   3. the copyright holder's name is not used to endorse products
;      built using this software without specific written permission.
;
; ALTERNATIVELY, provided that this notice is retained in full, this product
; may be distributed under the terms of the GNU General Public License (GPL),
; in which case the provisions of the GPL apply INSTEAD OF those given above.
;
; DISCLAIMER
;
; This software is provided 'as is' with no explicit or implied warranties
; in respect of its properties, including, but not limited to, correctness
; and/or fitness for purpose.
; ---------------------------------------------------------------------------
; Issue Date: 1/05/2003

; An AES (Rijndael) implementation for the Pentium MMX family using the NASM
; assembler <http://sourceforge.net/projects/nasm>.  This version implements
; the standard AES block length (128 bits, 16 bytes) with the same interface
; as that used in my C/C++ implementation.   This code does not preserve the
; eax, ecx or edx registers or the artihmetic status flags. However, the ebx,
; esi, edi, and ebp registers are preserved across calls.    Only encryption
; and decryption are implemented here, the key schedule code being that from
; compiling aes.c with USE_ASM defined.  This code uses VC++ register saving
; conentions; if it is used with another compiler, its conventions for using
; and saving registers will need to be checked (and calling conventions).

    section .text use32

; aes_rval aes_encrypt(const unsigned char in_blk[],
;                   unsigned char out_blk[], const aes_encrypt_ctx cx[1]);
; aes_rval aes_decrypt(const unsigned char in_blk[],
;                   unsigned char out_blk[], const aes_decrypt_ctx cx[1]);
;
; comment in/out the following lines to obtain the desired subroutines

%define ENCRYPTION  ; define if encryption is needed
;%define DECRYPTION  ; define if decryption is needed

; The DLL interface must use the _stdcall convention in which the number
; of bytes of parameter space is added after an @ to the sutine's name.
; We must also remove our parameters from the stack before return (see
; the do_ret macro).

tlen:   equ  1024   ; length of each of 4 'xor' arrays (256 32-bit words)
m0:     equ     0
m1:     equ     1
m2:     equ     2
m3:     equ     3

; offsets to parameters with one register pushed onto stack

in_blk: equ     4   ; input byte array address parameter
out_blk:equ     8   ; output byte array address parameter
ctx:    equ    12   ; AES context structure
stk_spc:equ    24   ; stack space

; offsets in context structure

ksch:   equ     0   ; encryption key schedule base address
nrnd:   equ   256   ; number of rounds
nblk:   equ   260   ; number of rounds

; register mapping for encrypt and decrypt subroutines

%define r0  eax
%define r1  ebx
%define r2  esi
%define r3  edi
%define r4  ecx
%define r5  edx
%define r6  ebp

%define eaxl  al
%define eaxh  ah
%define ebxl  bl
%define ebxh  bh
%define ecxl  cl
%define ecxh  ch
%define edxl  dl
%define edxh  dh

; This macro takes a 32-bit word representing a column and uses
; each of its four bytes to index into four tables of 256 32-bit
; words to obtain values that are then xored into the appropriate
; output registers r0, r1, r2 or r3.

; Parameters:
;   %1  out_state[0]
;   %2  out_state[1]
;   %3  out_state[2]
;   %4  out_state[3]
;   %5  table base address
;   %6  input register for the round (destroyed)
;   %7  scratch register for the round

%macro do_col 7

    movzx   %7,%6l
    xor     %1,[4*%7+%5+m0*tlen]
    movzx   %7,%6h
    shr     %6,16
    xor     %2,[4*%7+%5+m1*tlen]
    movzx   %7,%6l
    movzx   %6,%6h
    xor     %3,[4*%7+%5+m2*tlen]
    xor     %4,[4*%6+%5+m3*tlen]

%endmacro

; initialise output registers from the key schedule

%macro do_fcol 8

    mov     %1,[%8]
    movzx   %7,%6l
    mov     %2,[%8+12]
    xor     %1,[4*%7+%5+m0*tlen]
    mov     %4,[%8+ 4]
    movzx   %7,%6h
    shr     %6,16
    xor     %2,[4*%7+%5+m1*tlen]
    movzx   %7,%6l
    movzx   %6,%6h
    xor     %4,[4*%6+%5+m3*tlen]
    mov     %6,%3           ; save an input register value
    mov     %3,[%8+ 8]
    xor     %3,[4*%7+%5+m2*tlen]

%endmacro

; initialise output registers from the key schedule

%macro do_icol 8

    mov     %1,[%8]
    movzx   %7,%6l
    mov     %2,[%8+ 4]
    xor     %1,[4*%7+%5+m0*tlen]
    mov     %4,[%8+12]
    movzx   %7,%6h
    shr     %6,16
    xor     %2,[4*%7+%5+m1*tlen]
    movzx   %7,%6l
    movzx   %6,%6h
    xor     %4,[4*%6+%5+m3*tlen]
    mov     %6,%3           ; save an input register value
    mov     %3,[%8+ 8]
    xor     %3,[4*%7+%5+m2*tlen]

%endmacro

; These macros implement either MMX or stack based local variables

%macro  save 2
    mov     [esp+4*%1],%2
%endmacro

%macro  restore 2
    mov     %1,[esp+4*%2]
%endmacro

; This macro performs a forward encryption cycle. It is entered with
; the first previous round column values in r0, r1, r2 and r3 and
; exits with the final values in the same registers, using the MMX
; registers mm0-mm1 or the stack for temporary storage

%macro fwd_rnd 1-2 _t_fn

; mov current column values into the MMX registers

    mov     r4,r0
    save    0,r1
    save    1,r3

; compute new column values

    do_fcol r0,r3,r2,r1, %2, r4,r5, %1  ; r4 = input r0
    do_col  r2,r1,r0,r3, %2, r4,r5      ; r4 = input r2 (in do_fcol)
    restore r4,0
    do_col  r1,r0,r3,r2, %2, r4,r5      ; r4 = input r1
    restore r4,1
    do_col  r3,r2,r1,r0, %2, r4,r5      ; r4 = input r3

%endmacro

; This macro performs an inverse encryption cycle. It is entered with
; the first previous round column values in r0, r1, r2 and r3 and
; exits with the final values in the same registers, using the MMX
; registers mm0-mm1 or the stack for temporary storage

%macro inv_rnd 1-2 _t_in

; mov current column values into the MMX registers

    mov     r4,r0
    save    0,r1
    save    1,r3

; compute new column values

    do_icol r0,r1,r2,r3, %2, r4,r5, %1
    do_col  r2,r3,r0,r1, %2, r4,r5
    restore r4,0
    do_col  r1,r2,r3,r0, %2, r4,r5
    restore r4,1
    do_col  r3,r0,r1,r2, %2, r4,r5

%endmacro

; the DLL has to implement the _stdcall calling interface on return
; In this case we have to take our parameters (3 4-byte pointers)
; off the stack

%macro  do_ret  0
%ifndef AES_DLL
    ret
%else
    pop edx     ; return address to edx (preserve return value in eax)
    add esp,12  ; take parameters off the stack
    jmp edx     ; jump to the return address in edx
%endif
%endmacro

%macro  do_name 1
%ifndef AES_DLL
    global  %1
%1:
%else
    global  %1@12
    export  %1@12
%1@12:
%endif
%endmacro

; AES (Rijndael) Encryption Subroutine

%ifdef  ENCRYPTION

    extern  _t_fn
    extern  _t_fl

    do_name _aes_encrypt

    sub     esp,stk_spc
    mov     [esp+20],ebp
    mov     [esp+16],ebx
    mov     [esp+12],esi
    mov     [esp+ 8],edi
    mov     r4,[esp+in_blk+stk_spc] ; input pointer
    mov     r6,[esp+ctx+stk_spc]    ; context pointer
    lea     r6,[r6+ksch]            ; key pointer

; input four columns and xor in first round key

    mov     r0,[r4]
    xor     r0,[r6]
    mov     r1,[r4+4]
    xor     r1,[r6+4]
    mov     r2,[r4+8]
    xor     r2,[r6+8]
    mov     r3,[r4+12]
    xor     r3,[r6+12]

    cmp     byte [r6+nrnd],10
    je      .4
    cmp     byte [r6+nrnd],12
    je      .3
    cmp     byte [r6+nrnd],14
    jne     .5

.2: fwd_rnd r6+ 16          ; 14 rounds for 128-bit key
    fwd_rnd r6+ 32
    add     r6, 32

.3: fwd_rnd r6+ 16          ; 12 rounds for 128-bit key
    fwd_rnd r6+ 32
    add     r6, 32

.4: fwd_rnd r6+ 16          ; 10 rounds for 128-bit key
    fwd_rnd r6+ 32
    fwd_rnd r6+ 48
    fwd_rnd r6+ 64
    fwd_rnd r6+ 80
    fwd_rnd r6+ 96
    fwd_rnd r6+112
    fwd_rnd r6+128
    fwd_rnd r6+144
    fwd_rnd r6+160,_t_fl    ; last round uses a different table

; move final values to the output array

    mov     r6,[esp+out_blk+stk_spc]
    mov     [r6+12],r3
    mov     [r6+8],r2
    mov     [r6+4],r1
    mov     [r6],r0
.5:
    mov     ebp,[esp+20]
    mov     ebx,[esp+16]
    mov     esi,[esp+12]
    mov     edi,[esp+ 8]
    add     esp,stk_spc
    do_ret

%endif

; AES (Rijndael) Decryption Subroutine

%ifdef  DECRYPTION

    extern  _t_in
    extern  _t_il

    do_name _aes_decrypt

    sub     esp,stk_spc
    mov     [esp+20],ebp
    mov     [esp+16],ebx
    mov     [esp+12],esi
    mov     [esp+ 8],edi

    mov     r4,[esp+in_blk+stk_spc] ; input pointer
    mov     r6,[esp+ctx+stk_spc]    ; context pointer
    mov     r5,[r6+nrnd]            ; number of rounds
    lea     r6,[r6+ksch]            ; key pointer
    shl     r5,4
    add     r5,r6

; input four columns and xor in first round key

    mov     r0,[r4]
    xor     r0,[r5]
    mov     r1,[r4+4]
    xor     r1,[r5+4]
    mov     r2,[r4+8]
    xor     r2,[r5+8]
    mov     r3,[r4+12]
    xor     r3,[r5+12]

    cmp     byte [r6+nrnd],10
    je      .4
    cmp     byte [r6+nrnd],12
    je      .3
    cmp     byte [r6+nrnd],14
    jne     .5
.2:
    inv_rnd r6+208      ; 14 rounds for 128-bit key
    inv_rnd r6+192
.3:
    inv_rnd r6+176      ; 12 rounds for 128-bit key
    inv_rnd r6+160
.4:
    inv_rnd r6+144      ; 10 rounds for 128-bit key
    inv_rnd r6+128
    inv_rnd r6+112
    inv_rnd r6+ 96
    inv_rnd r6+ 80
    inv_rnd r6+ 64
    inv_rnd r6+ 48
    inv_rnd r6+ 32
    inv_rnd r6+ 16
    inv_rnd r6,_t_il    ; last round uses a different table

; move final values to the output array.  CAUTION: the
; order of these assigns rely on the register mappings

    mov     r6,[esp+out_blk+stk_spc]
    mov     [r6+12],r3
    mov     [r6+8],r2
    mov     [r6+4],r1
    mov     [r6],r0
.5:
    mov     ebp,[esp+20]
    mov     ebx,[esp+16]
    mov     esi,[esp+12]
    mov     edi,[esp+ 8]
    add     esp,stk_spc
    do_ret

%endif

    end
