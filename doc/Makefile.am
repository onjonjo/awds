
DB2MAN=@DB2MAN@

if CREATE_MANPAGE
man8_MANS = start_awds.8 stop_awds.8

endif 



start_awds.8: start_awds.sgml
	$(DB2MAN) $^

stop_awds.8: stop_awds.sgml
	$(DB2MAN) $^

clean:
	rm -f start_awds.8 stop_awds.8

