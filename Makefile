all: aoa-proxy aoa-proxy.1

PHONY: clean

CC= $(CROSS_COMPILE)gcc
LDADD:= -lusb-1.0 -lb64

aoa-proxy: aoa-proxy.o
	$(CC) -o $@  $(LDFLAGS) $^ $(LDADD)

clean:
	find . \( -path ./aoa-proxy -o -path ./aoa-proxy.o \) -delete
	find . -path ./aoa-proxy.1 -delete

distclean: clean

aoa-proxy.1: aoa-proxy
	help2man -o $@ --no-info ./$^ --name="interact with Android devices using the Android Open Accessory Protocol"

install:
	install -s aoa-proxy $(DESTDIR)/usr/sbin/
	install -m 644 aoa-proxy.1 $(DESTDIR)/usr/share/man/man1/
	install -m 644 bash-completion/aoa-proxy $(DESTDIR)/usr/share/bash-completion/completions/
	install systemd/aoa-proxy-announce $(DESTDIR)/usr/lib/
	install systemd/aoa-proxy-forward $(DESTDIR)/usr/lib/
	install -m 644 systemd/aoa-proxy-announce@.service $(DESTDIR)/usr/lib/systemd/system/
	install -m 644 systemd/aoa-proxy-forward@.service $(DESTDIR)/usr/lib/systemd/system/
	install -m 644 systemd/52-aoa-proxy.rules $(DESTDIR)/usr/lib/udev/rules.d/

uninstall:
	rm -f $(DESTDIR)/usr/sbin/aoa-proxy
	rm -f $(DESTDIR)/usr/share/man/man1/aoa-proxy.1
	rm -f $(DESTDIR)/usr/share/bash-completion/completions/aoa-proxy
	rm -f $(DESTDIR)/usr/lib/aoa-proxy-announce
	rm -f $(DESTDIR)/usr/lib/aoa-proxy-forward
	rm -f $(DESTDIR)/usr/lib/systemd/system/aoa-proxy-announce@.service
	rm -f $(DESTDIR)/usr/lib/systemd/system/aoa-proxy-forward@.service
	rm -f $(DESTDIR)/usr/lib/udev/rules.d/52-aoa-proxy.rules
