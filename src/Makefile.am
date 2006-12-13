	SUBDIRS =  crypto 


module_flags = -module  -avoid-version -export-symbols-regex gea_main

#AM_CPPFLAGS= @GEAINC@ -pedantic -Wno-long-long
AM_CPPFLAGS=  -pedantic -Wno-long-long 

nobase_include_HEADERS = awds/*.h awds/ext/*.h

pkglib_LTLIBRARIES = \
	awdsrouting.la \
	udpbasic.la \
	pinger.la \
	tapiface.la \
	tapiface2.la \
	topowatch.la \
	shell.la \
	topodump.la \
	aesccm.la  \
	rawbasic.la

#	tapiface_crypto.la \
#

#AM_LDFLAGS = @GEALIB@ -module --avoid-version 
AM_LDFLAGS =  -module --avoid-version


awdsrouting_la_SOURCES =  \
        awds/AbstractId.h \
        awds/AwdsRouting.h \
        awds/BasePacket.h \
        awds/basic.h \
        awds/beacon.h \
        awds/Callback.h \
        awds/Flood.h \
        awds/FloodHistory.h \
        awds/FloodTypes.h \
        awds/NodeDescr.h \
        awds/NodeId.h \
        awds/NodeId.h \
        awds/routing.h \
        awds/settings.h \
        awds/SrcPacket.h \
        awds/toArray.h \
        awds/Topology.h \
        awds/TopoPacket.h \
        awds/UnicastPacket.h \
        AwdsRouting.cc \
	beacon.cc \
        FloodHistory.cc \
        Topology.cc \
        TopoPacket.cc \
	awds/sqrt_int.h sqrt_int.c 


awdsrouting_la_CPPFLAGS = -DMODULE_NAME=interf
awdsrouting_la_LDFLAGS = -module  -avoid-version -export-symbols-regex gea_main



udpbasic_la_SOURCES = udpBasic.cc awds/basic.h basic.cc
udpbasic_la_CPPFLAGS = -DMODULE_NAME=udpbasic
udpbasic_la_LDFLAGS = -module  -avoid-version -export-symbols-regex gea_main


rawbasic_la_SOURCES = rawbasic.cc basic.cc awds/basic.h awds/NodeId.h
rawbasic_la_CPPFLAGS = -DMODULE_NAME=rawbasic
rawbasic_la_LDFLAGS = -module  -avoid-version -export-symbols-regex gea_main


pinger_la_SOURCES = pinger.cc
pinger_la_LDFLAGS = -module  -avoid-version -export-symbols-regex gea_main


tapiface_la_SOURCES = TapInterface.cc tapiface.cc awds/tapiface.h
tapiface_la_LDFLAGS = -module  -avoid-version -export-symbols-regex gea_main


tapiface2_la_SOURCES = TapInterface.cc TapInterface2.cc awds/tapiface.h tapiface2.cc  awds/tapiface2.h
tapiface2_la_CPPFLAGS = -DMODULE_NAME=tapiface2
tapiface2_la_LDFLAGS = -module  -avoid-version -export-symbols-regex gea_main


topowatch_la_SOURCES  = topowatch.cc
topowatch_la_CPPFLAGS = -DMODULE_NAME=topowatch
topowatch_la_LDFLAGS  = -module  -avoid-version -export-symbols-regex gea_main

shell_la_SOURCES  = shell.cc
shell_la_CPPFLAGS = -DMODULE_NAME=shell
shell_la_LDFLAGS  = -module  -avoid-version -export-symbols-regex gea_main

topodump_la_SOURCES = dumpTopo.cc
topodump_la_LDFLAGS = -module  -avoid-version -export-symbols-regex gea_main



EXTRA_PROGRAMS =  awds 



if AES_MMX
AES_IMPL = crypto/gladman/aescrypt.mmx.asm
aesccm_la_LIBADD = aescrypt.mmx.o
else	
AES_IMPL = crypto/gladman/aescrypt.c
endif

aescrypt.mmx.o: crypto/gladman/aescrypt.mmx.asm
	nasm -O2 -felf crypto/gladman/aescrypt.mmx.asm -o aescrypt.mmx.o


aesccm_la_SOURCES = \
	crypto/AesCcmCryptoUnit.cc \
	crypto/AesCcmCryptoUnit.h \
	crypto/CryptoUnit.h \
	$(AES_IMPL) \
	crypto/gladman/aes.h \
	crypto/gladman/aeskey.c \
	crypto/gladman/aesopt.h \
	crypto/gladman/aestab.c \
	crypto/gladman/ccm.c \
	crypto/gladman/ccm.h \
	crypto/crypto_helpers.cc \
	crypto/crypto_helpers.h


aesccm_la_CPPFLAGS = -DMODULE_NAME=aesccm
aesccm_la_LDFLAGS  = -module  -avoid-version -export-symbols-regex gea_main



awds_SOURCES = awds.cc
awds_LDADD  = -lgea-posix rawbasic.la interf.la tapiface2.la 
awds_LDFLAGS = -static

#-fprofile-use



