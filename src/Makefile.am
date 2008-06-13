SUBDIRS =  crypto

LIBADD = -lgea3
module_flags = -module  -avoid-version -export-symbols-regex gea_main

#AM_CPPFLAGS= @GEAINC@ -pedantic -Wno-long-long
AM_CPPFLAGS=  -pedantic -Wno-long-long

COMMON_CPPFLAGS = -fvisibility=hidden @GEAINC@

nobase_include_HEADERS = awds/*.h awds/ext/*.h

pkglib_LTLIBRARIES = \
	awdsrouting.la \
	udpbasic.la \
	pinger.la \
	tapiface2.la \
	topowatch.la \
	shell.la \
	daemon.la \
	topodump.la \
	topolock.la \
	aesccm.la  \
	rawbasic.la \
	etxmetric.la \
	rttmetric.la \
	ttmetric.la \
	pktpair.la \
	src_filter.la \
	traffic.la

#	tapiface_crypto.la \
#

AM_LDFLAGS =  -module -avoid-version


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


awdsrouting_la_CPPFLAGS = -DMODULE_NAME=awdsrouting $(COMMON_CPPFLAGS)
awdsrouting_la_LDFLAGS = $(module_flags)
awdsrouting_la_LIBADD = -lgea3


udpbasic_la_SOURCES = udpBasic.cc awds/basic.h basic.cc SendQueue.cc
udpbasic_la_CPPFLAGS = -DMODULE_NAME=udpbasic $(COMMON_CPPFLAGS)
udpbasic_la_LDFLAGS = $(module_flags)
udpbasic_la_LIBADD = -lgea3

rawbasic_la_SOURCES = rawbasic.cc basic.cc awds/basic.h awds/NodeId.h SendQueue.cc
rawbasic_la_CPPFLAGS = -DMODULE_NAME=rawbasic $(COMMON_CPPFLAGS)
rawbasic_la_LDFLAGS = $(module_flags)
rawbasic_la_LIBADD = -lgea3 -lgea3-posix

pinger_la_SOURCES = pinger.cc
pinger_la_LDFLAGS = $(module_flags)
pinger_la_CPPFLAGS = -DMODULE_NAME=pinger $(COMMON_CPPFLAGS)
pinger_la_LIBADD = -lgea3

# tapiface is obsolete
#tapiface_la_SOURCES = TapInterface.cc tapiface.cc awds/tapiface.h
#tapiface_la_LDFLAGS = -module  -avoid-version -export-symbols-regex gea_main
#tapiface_la_LIBADD = -lgea3 -lgea3-posix

tapiface2_la_SOURCES = TapInterface.cc TapInterface2.cc awds/tapiface.h tapiface2.cc  awds/tapiface2.h
tapiface2_la_CPPFLAGS = -DMODULE_NAME=tapiface2 $(COMMON_CPPFLAGS)
tapiface2_la_LDFLAGS = $(module_flags)
tapiface2_la_LIBADD = -lgea3 -lgea3-posix

topowatch_la_SOURCES  = topowatch.cc
topowatch_la_CPPFLAGS = -DMODULE_NAME=topowatch $(COMMON_CPPFLAGS)
topowatch_la_LDFLAGS  = $(module_flags)
topowatch_la_LIBADD = -lgea3 -lgea3-posix

shell_la_SOURCES  = shell.cc unixfdostream.h
shell_la_CPPFLAGS = -DMODULE_NAME=shell $(COMMON_CPPFLAGS)
shell_la_LDFLAGS  = $(module_flags)
shell_la_LIBADD = -lgea3 -lgea3-posix

topodump_la_SOURCES = dumpTopo.cc
topodump_la_CPPFLAGS = -DMODULE_NAME=topodump $(COMMON_CPPFLAGS)
topodump_la_LDFLAGS = -module  -avoid-version -export-symbols-regex gea_main
topodump_la_LIBADD = -lgea3

topolock_la_SOURCES = TopoLock.cc
topolock_la_LDFLAGS = $(module_flags)
topolock_la_CPPFLAGS = -DMODULE_NAME=topolock $(COMMON_CPPFLAGS)
topolock_la_LIBADD  = -lgea3

etxmetric_la_SOURCES = awds/Metric.h awds/EtxMetric.h EtxMetric.cc
etxmetric_la_LDFLAGS = $(module_flags)
etxmetric_la_CPPFLAGS = -DMODULE_NAME=etxmetric $(COMMON_CPPFLAGS)
etxmetric_la_LIBADD = -lgea3

rttmetric_la_SOURCES = awds/Metric.h awds/UCMetricPacket.h UCMetricPacket.cc awds/ExtMetric.h \
	ExtMetric.cc awds/UCastMetric.h UCastMetric.cc awds/RTTMetric.h RTTMetric.cc
rttmetric_la_LDFLAGS  = $(module_flags)
rttmetric_la_CPPFLAGS = -DMODULE_NAME=rttmetric $(COMMON_CPPFLAGS)
rttmetric_la_LIBADD   = -lgea3

#signaler_la_SOURCES = signaler.cc
#signaler_la_CPPFLAGS = -DMODULE_NAME=signaler $(COMMON_CPPFLAGS)
#signaler_la_LDFLAGS = $(module_flags)
#signaler_la_LIBADD = -lgea3


EXTRA_PROGRAMS =  awds_static

#gea3mad_la_SOURCES = gea3mad.cc
#gea3mad_la_CPPFLAGS = -I/scratch/wienoebs/awds/ns2-gea/ns-2 -I/usr/include/tcl8.4
#gea3mad_la_LDFLAGS = $(module_flags)

ttmetric_la_SOURCES = awds/Metric.h awds/UCMetricPacket.h UCMetricPacket.cc awds/ExtMetric.h \
	ExtMetric.cc awds/UCastMetric.h UCastMetric.cc awds/TTMetric.h TTMetric.cc
ttmetric_la_LDFLAGS = $(module_flags)
ttmetric_la_CPPFLAGS = -DMODULE_NAME=ttmetric $(COMMON_CPPFLAGS)
ttmetric_la_LIBADD   = -lgea3

pktpair_la_SOURCES = awds/Metric.h awds/UCMetricPacket.h UCMetricPacket.cc awds/ExtMetric.h \
	ExtMetric.cc awds/UCastMetric.h UCastMetric.cc awds/PktPair.h PktPair.cc
pktpair_la_LDFLAGS = $(module_flags)
pktpair_la_CPPFLAGS = -DMODULE_NAME=pktpair $(COMMON_CPPFLAGS)
pktpair_la_LIBADD   = -lgea3


traffic_la_SOURCES  = awds/Traffic.h Traffic.cc
traffic_la_LDFLAGS  = $(module_flags)
traffic_la_CPPFLAGS = -DMODULE_NAME=traffic $(COMMON_CPPFLAGS)
traffic_la_LIBADD   = -lgea3

if AES_MMX
AES_IMPL = crypto/gladman/aescrypt.mmx.asm
aesccm_la_LIBADD = aescrypt.mmx.o -lgea3
else
AES_IMPL = crypto/gladman/aescrypt.c
aesccm_la_LIBADD = -lgea3
endif

aescrypt.mmx.o: crypto/gladman/aescrypt.mmx.asm
	nasm -O3 -felf crypto/gladman/aescrypt.mmx.asm -o aescrypt.mmx.o


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
aesccm_la_LDFLAGS  = $(module_flags)

daemon_la_SOURCES  = debug_cmd.cc
daemon_la_CPPFLAGS = -DMODULE_NAME=daemon $(COMMON_CPPFLAGS)
daemon_la_LDFLAGS  = $(module_flags)
daemon_la_LIBADD   = -lgea3 -lgea3-posix

src_filter_la_SOURCES  = SrcFilter.cc
src_filter_la_CPPFLAGS = -DMODULE_NAME=src_filter $(COMMON_CPPFLAGS)
src_filter_la_LDFLAGS  = $(module_flags)
src_filter_la_LIBADD   = -lgea3

awds_static_SOURCES = awds.cc
awds_static_LDADD  = -lgea3 -lgea3-posix rawbasic.la awdsrouting.la tapiface2.la
awds_static_LDFLAGS = -static
awds_static_CPPFLAGS = $(COMMON_CPPFLAGS)
