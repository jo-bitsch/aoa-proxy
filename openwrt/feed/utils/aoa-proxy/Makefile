include $(TOPDIR)/rules.mk
 
PKG_NAME:=aoa-proxy
PKG_VERSION:=0.1
PKG_RELEASE:=1
 
PKG_BUILD_DIR:=$(BUILD_DIR)/aoa-proxy-$(PKG_VERSION)
PKG_SOURCE:=aoa-proxy-$(PKG_VERSION).tar.gz
PKG_SOURCE_PROT:=git
PKG_SOURCE_URL:=https://github.com/jo-bitsch/aoa-proxy.git
PKG_HASH:=skip
#PKG_HASH:=9b7dc52656f5cbec846a7ba3299f73bd
 
include $(INCLUDE_DIR)/package.mk

TARGET_CFLAGS += -DNO_HID=1
 
define Package/aoa-proxy
  SECTION:=utils
  CATEGORY:=Network
  TITLE:=aoa-stuff
  URL:=https://github.com/jo-bitsch/aoa-proxy
  DEPENDS:=+libusb-1.0
  PKG_BUILD_DEPENDS:=+argp-standalone
endef
 
define Package/aoa-proxy/description
  interact with Android devices using the Android Open Accessory Protocol
endef
 
define Build/Configure
  $(call Build/Configure/Default)
endef

define Build/Compile
  $(call Build/Compile/Default,aoa-proxy OPENWRT=1)
endef
 
define Package/aoa-proxy/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/aoa-proxy $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/hotplug.d/usb
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/openwrt/etc/hotplug.d/usb/99-aoa-proxy $(1)/etc/hotplug.d/usb/
	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/openwrt/etc/init.d/aoa-proxy-forward $(1)/etc/init.d/
endef
 
$(eval $(call BuildPackage,aoa-proxy))