include $(TOPDIR)/rules.mk

PKG_NAME:=3proxy
PKG_VERSION:=0.8-devel
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/3proxy
	SECTION:=net
	CATEGORY:=Network
	SUBMENU:=Web Servers/Proxies
	DEPENDS:=+libpthread +libopenssl
	TITLE:=3proxy free proxy server for openwrt
	URL:=http://www.3proxy.ru
	MAINTAINER:=imwifier@gmail.com
endef

define Package/3proxy/description
 3proxy is universal proxy server. It can be used to provide internal users
 with fully controllable access to external resources or to provide external
 users with access to internal resources.
endef

define Package/3proxy/conffiles
etc/3proxy/3proxy.cfg
etc/3proxy/bandlimiters
etc/3proxy/counters
etc/3proxy/passwd
endef

define Build/Configure
	$(CP) $(PKG_BUILD_DIR)/Makefile.Linux $(PKG_BUILD_DIR)/Makefile
endef

define Package/3proxy/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) \
		$(PKG_BUILD_DIR)/src/{3proxy,countersutil,dighosts,ftppr,mycrypt,pop3p,proxy,socks,tcppm,udppm} \
		$(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/3proxy
	$(INSTALL_CONF) $(PKG_BUILD_DIR)/scripts/3proxy.cfg $(1)/etc/3proxy
	touch $(1)/etc/3proxy{passwd,counters,bandlimiters}
	chmod 0600 $(1)/etc/3proxy*
	$(CP) $(PKG_BUILD_DIR)/scripts/add3proxyuser.sh $(1)/etc/3proxy
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/3proxy.init $(1)/etc/init.d
endef

$(eval $(call BuildPackage,3proxy))
