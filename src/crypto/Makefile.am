
if AES_MMX
AES_IMPL = gladman/aescrypt.mmx.asm
else	
AES_IMPL = gladman/aescrypt.c
endif

aescrypt.mmx.o: gladman/aescrypt.mmx.asm
	nasm -O2 -felf gladman/aescrypt.mmx.asm -o aescrypt.mmx.o


