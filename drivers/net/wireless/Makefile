#
# Makefile for the Linux Wireless network device drivers.
#

#TSAI: RK
obj-y += rockchip_wlan/wifi_sys/rkwifi_sys_iface.o
obj-$(CONFIG_RTL8192CU)        += rockchip_wlan/rtl8192cu/
obj-$(CONFIG_RTL8192DU)        += rockchip_wlan/rtl8192du/
obj-$(CONFIG_RTL8188EU)	       += rockchip_wlan/rtl8188eu/
obj-$(CONFIG_RTL8189ES)	       += rockchip_wlan/rtl8189es/
obj-$(CONFIG_RTL8723AU)	       += rockchip_wlan/rtl8723au/
obj-$(CONFIG_RTL8723BU)	       += rockchip_wlan/rtl8723bu/
obj-$(CONFIG_RTL8812AU)	       += rockchip_wlan/rtl8812au/
obj-$(CONFIG_RKWIFI)           += rockchip_wlan/rkwifi/
obj-$(CONFIG_RTL8723BS)        += rockchip_wlan/rtl8723bs/
obj-$(CONFIG_RTL8723BS_VQ0)    += rockchip_wlan/rtl8723bs-vq0/
obj-$(CONFIG_RTL8189FS)        += rockchip_wlan/rtl8189fs/
obj-$(CONFIG_RTL8188FU)        += rockchip_wlan/rtl8188fu/
obj-$(CONFIG_ESP8089)   += rockchip_wlan/esp8089/

obj-$(CONFIG_IPW2100) += ipw2x00/
obj-$(CONFIG_IPW2200) += ipw2x00/

obj-$(CONFIG_HERMES)		+= orinoco/

obj-$(CONFIG_AIRO)		+= airo.o
obj-$(CONFIG_AIRO_CS)		+= airo_cs.o airo.o

obj-$(CONFIG_ATMEL)             += atmel.o
obj-$(CONFIG_PCI_ATMEL)         += atmel_pci.o 
obj-$(CONFIG_PCMCIA_ATMEL)      += atmel_cs.o

obj-$(CONFIG_AT76C50X_USB)      += at76c50x-usb.o

obj-$(CONFIG_PRISM54)		+= prism54/

obj-$(CONFIG_HOSTAP)		+= hostap/
obj-$(CONFIG_B43)		+= b43/
obj-$(CONFIG_B43LEGACY)		+= b43legacy/
obj-$(CONFIG_ZD1211RW)		+= zd1211rw/
obj-$(CONFIG_RTL8180)		+= rtl818x/
obj-$(CONFIG_RTL8187)		+= rtl818x/
obj-$(CONFIG_RTLWIFI)		+= rtlwifi/

# 16-bit wireless PCMCIA client drivers
obj-$(CONFIG_PCMCIA_RAYCS)	+= ray_cs.o
obj-$(CONFIG_PCMCIA_WL3501)	+= wl3501_cs.o

obj-$(CONFIG_USB_NET_RNDIS_WLAN)	+= rndis_wlan.o

obj-$(CONFIG_USB_ZD1201)	+= zd1201.o
obj-$(CONFIG_LIBERTAS)		+= libertas/

obj-$(CONFIG_LIBERTAS_THINFIRM)	+= libertas_tf/

obj-$(CONFIG_ADM8211)	+= adm8211.o

obj-$(CONFIG_MWL8K)	+= mwl8k.o

obj-$(CONFIG_IWLWIFI)	+= iwlwifi/
obj-$(CONFIG_IWLEGACY)	+= iwlegacy/
obj-$(CONFIG_RT2X00)	+= rt2x00/

obj-$(CONFIG_P54_COMMON)	+= p54/

obj-$(CONFIG_ATH_CARDS)		+= ath/

obj-$(CONFIG_MAC80211_HWSIM)	+= mac80211_hwsim.o

obj-$(CONFIG_WL_TI)	+= ti/

obj-$(CONFIG_MWIFIEX)	+= mwifiex/

obj-$(CONFIG_BRCMFMAC)	+= brcm80211/
obj-$(CONFIG_BRCMSMAC)	+= brcm80211/

obj-$(CONFIG_CW1200)	+= cw1200/
obj-$(CONFIG_RSI_91X)	+= rsi/

ifeq ($(TSAI_OPT),1) 
	CFLAGS_rkwifi_sys_iface.o += -O0
#	-O1 -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-inline -fno-unit-at-a-time
endif
