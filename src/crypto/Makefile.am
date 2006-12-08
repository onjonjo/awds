
if AES_MMX
AES_IMPL = gladman/aescrypt.mmx.asm
crypto_test_LDADD = aescrypt.mmx.o
crypto_test_CPPFLAGS=-DAES_ASM=1
else	
AES_IMPL = gladman/aescrypt.c
endif

aescrypt.mmx.o: gladman/aescrypt.mmx.asm
	nasm -O2 -felf gladman/aescrypt.mmx.asm -o aescrypt.mmx.o

EXTRA_PROGRAMS = crypto_test

crypto_test_SOURCES = \
	crypto_test.cc \
	$(AES_IMPL) \
	gladman/aes.h \
	gladman/aeskey.c \
	gladman/aesopt.h \
	gladman/aestab.c \
	gladman/ccm.c \
	gladman/ccm.h 


