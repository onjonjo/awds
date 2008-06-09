
DB2MAN=@DB2MAN@
EXTRA_DIST = shell.so.xml daemon.so.xml start_awds.sgml stop_awds.sgml

if CREATE_MANPAGE
man8_MANS = start_awds.8 stop_awds.8

endif 

if CREATE_MODDOCS 

MODDOCS=awds-daemon.5 awds-shell.5 awds-rawbasic.5 awds-udpbasic.5 awds-awdsrouting.5

XSLT_FILE = @DOCBOOKXSL@

man5_MANS = $(MODDOCS)

awds-daemon.5: daemon.so.xml
	$(XSLTPROC) -''-nonet $(XSLT_FILE) $<

awds-shell.5: shell.so.xml
	$(XSLTPROC) -''-nonet $(XSLT_FILE) $<

awds-rawbasic.5: rawbasic.so.xml
	$(XSLTPROC) -''-nonet $(XSLT_FILE) $<

awds-udpbasic.5: udpbasic.so.xml
	$(XSLTPROC) -''-nonet $(XSLT_FILE) $<

awds-awdsrouting.5: awdsrouting.so.xml
	$(XSLTPROC) -''-nonet $(XSLT_FILE) $<

endif

start_awds.8: start_awds.sgml
	$(DB2MAN) $^

stop_awds.8: stop_awds.sgml
	$(DB2MAN) $^

clean:
	rm -f start_awds.8 stop_awds.8 $(MODDOCS)

