
EXTRA_DIST = start_awds.in stop_awds awds.conf awds.key
sbin_SCRIPTS = start_awds stop_awds

sysconf_DATA = awds.conf awds.key


start_awds: start_awds.in ../config.status
	sed -e "s,@AWDS_CONF_DIR@,$(sysconfdir)," -e "s,@AWDS_MOD_DIR@,$(pkglibdir)," < start_awds.in  >$@ 

awds.conf: awds.conf.in ../config.status
	sed -e "s,@AWDS_CONF_DIR@,$(sysconfdir)," -e "s,@AWDS_MOD_DIR@,$(pkglibdir)," < awds.conf.in  >$@ 

stop_awds: stop_awds.in ../config.status
	sed -e "s,@AWDS_CONF_DIR@,$(sysconfdir)," -e "s,@AWDS_MOD_DIR@,$(pkglibdir)," < stop_awds.in  >$@ 

