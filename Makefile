# Use ?= to allow OpenWRT to override these from the environment
CC ?= $(CROSS_COMPILE)gcc
PKG_CONFIG ?= pkg-config

# 1. Feature Probing: Check if -largp is actually needed
# We try to compile a tiny program. If it fails without -largp, we add it.
NEED_ARGP := $(shell printf '#include <argp.h>\nint main(){argp_parse(0,0,0,0,0,0); return 0;}' | \
               $(CC) -x c -o /dev/null - >/dev/null 2>&1 || echo "-largp")

# 2. Use pkg-config for libusb (Standard on Ubuntu and OpenWRT)
USB_CFLAGS  := $(shell $(PKG_CONFIG) --cflags libusb-1.0 2>/dev/null)
USB_LIBS    := $(shell $(PKG_CONFIG) --libs libusb-1.0 2>/dev/null || echo "-lusb-1.0")

# 3. Handle Versioning safely
GIT_VERSION ?= $(shell git --no-pager describe --tags --always --dirty 2>/dev/null || echo "unknown")
VERSION_FLAGS := -DGIT_VERSION='$(GIT_VERSION)'

# Collect all flags
# We use += so that OpenWRT's TARGET_CFLAGS and TARGET_LDFLAGS are preserved
override CFLAGS += -Wall -Wextra $(USB_CFLAGS) $(VERSION_FLAGS)
override LDADD  += $(USB_LIBS) $(NEED_ARGP) -lb64

.PHONY: all clean distclean

all: aoa-proxy aoa-proxy.1

aoa-proxy: aoa-proxy.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDADD)

aoa-proxy.o: aoa-proxy.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Only build manpages if help2man exists (usually missing in OpenWRT buildroot)
aoa-proxy.1: aoa-proxy
	@if command -v help2man > /dev/null; then \
		help2man -o $@ --no-info ./$^ --name="Android Open Accessory Protocol proxy"; \
	else \
		echo "help2man not found, skipping manpage"; \
	fi

clean:
	rm -f aoa-proxy aoa-proxy.o aoa-proxy.1