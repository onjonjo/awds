SUBDIRS = src scripts doc

EXTRA_DIST = \
	debian/awds.conffiles\
	debian/awds.copyright\
	debian/awds-dev.install\
	debian/awds.dirs\
	debian/awds.init\
	debian/awds.install\
	debian/awds.postinst.debhelper\
	debian/awds.postrm.debhelper\
	debian/awds.prerm.debhelper\
	debian/changelog\
	debian/compat\
	debian/control\
	debian/copyright\
	debian/rules\
	doc/awds.rules \
	doc/exit_policy.txt \
	doc/README.Debian \
	doc/start_awds.sgml \
	doc/stop_awds.sgml \
	awds.spec awds.spec.in



awds.spec: version.m4 awds.spec.in 
	m4 $^ > $@



