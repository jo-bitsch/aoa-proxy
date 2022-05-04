all: aoa-proxy aoa-proxy.1

PHONY: clean version~

CC= $(CROSS_COMPILE)gcc
LDADD:= -lusb-1.0 -lb64

GIT_VERSION := $(shell git --no-pager describe --tags --always --dirty)
# recompile version.h dependants when GIT_VERSION changes, uses temporary file version~
version~:
	@echo '$(GIT_VERSION)' | cmp -s - $@ || echo '$(GIT_VERSION)' > $@
version.h: version~
	@echo "#ifndef GIT_VERSION"  > version.h
	@echo "#define GIT_VERSION \"$(GIT_VERSION)"\" >> version.h
	@echo "#endif" >> version.h
	@echo Git version $(GIT_VERSION)

aoa-proxy: version.h aoa-proxy.o
	$(CC) -o $@  $(LDFLAGS) $^ $(LDADD)

clean:
	find . \( -path ./aoa-proxy -o -path ./aoa-proxy.o -o -path ./version.h -o -path ./version~ \) -delete
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
