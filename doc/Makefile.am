
DB2MAN=@DB2MAN@

if CREATE_MANPAGE
man8_MANS = start_awds.8 stop_awds.8

endif 

if CREATE_MODDOCS 

MODDOCS=awds-daemon.5 awds-shell.5

XSLT_FILE = /usr/share/sgml/docbook/stylesheet/xsl/nwalsh/manpages/docbook.xsl

man5_MANS = $(MODDOCS)

awds-daemon.5: daemon.so.xml
	$(XSLTPROC) -''-nonet $(XSLT_FILE) $<

awds-shell.5: shell.so.xml
	$(XSLTPROC) -''-nonet $(XSLT_FILE) $<


endif


start_awds.8: start_awds.sgml
	$(DB2MAN) $^

stop_awds.8: stop_awds.sgml
	$(DB2MAN) $^

clean:
	rm -f start_awds.8 stop_awds.8

