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
