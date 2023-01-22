include $(TOPDIR)/rules.mk
 
PKG_NAME:=aoa-proxy
PKG_VERSION:=0.1
PKG_RELEASE:=1
 
PKG_BUILD_DIR:=$(BUILD_DIR)/aoa-proxy-$(PKG_VERSION)
PKG_SOURCE:=aoa-proxy-$(PKG_VERSION).tar.gz
PKG_SOURCE_PROT:=git
PKG_SOURCE_URL:=https://github.com/jo-bitsch/aoa-proxy.git
#PKG_HASH:=9b7dc52656f5cbec846a7ba3299f73bd
 
include $(INCLUDE_DIR)/package.mk
 
define Package/aoa-proxy
  SECTION:=utils
  CATEGORY:=Network
  TITLE:=aoa-stuff
  #DESCRIPTION:=This variable is obsolete. use the Package/name/description define instead!
  URL:=https://github.com/jo-bitsch/aoa-proxy
  DEPENDS:=+libusb-1.0 +argp-standalone
endef
 
define Package/aoa-proxy/description
 aoa proxy long description
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
endef
 
$(eval $(call BuildPackage,aoa-proxy))