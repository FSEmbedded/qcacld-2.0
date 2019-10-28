#KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
BUILDROOT_PATH = ~/fsimx6/buildroot-f+s
KERNEL_SRC = $(BUILDROOT_PATH)/output/build/linux-custom
INSTALL_MOD_PATH = $(BUILDROOT_PATH)/output/target
CROSS_COMPILE = $(BUILDROOT_PATH)/output/host/usr/bin/arm-linux-

KBUILD_OPTIONS := WLAN_ROOT=$(shell pwd)
KBUILD_OPTIONS += MODNAME=wlan

# Determine if the driver license is Open source or proprietary
# This is determined under the assumption that LICENSE doesn't change.
# Please change here if driver license text changes.
LICENSE_FILE ?= CORE/HDD/src/wlan_hdd_main.c
WLAN_OPEN_SOURCE = $(shell if grep -q "MODULE_LICENSE(\"Dual BSD/GPL\")" \
		$(LICENSE_FILE); then echo 1; else echo 0; fi)

#By default build for CLD
WLAN_SELECT := CONFIG_QCA_CLD_WLAN=m
KBUILD_OPTIONS += CONFIG_QCA_WIFI_ISOC=0
KBUILD_OPTIONS += CONFIG_QCA_WIFI_2_0=1
KBUILD_OPTIONS += $(WLAN_SELECT)
KBUILD_OPTIONS += WLAN_OPEN_SOURCE=$(WLAN_OPEN_SOURCE)

# Additonal settings taken from LEA.3.0, config.te-f30
KBUILD_OPTIONS += CONFIG_CLD_HL_SDIO_CORE=y
KBUILD_OPTIONS += CONFIG_LINUX_QCMBR=y
KBUILD_OPTIONS += SAP_AUTH_OFFLOAD=1
KBUILD_OPTIONS += CONFIG_QCA_LL_TX_FLOW_CT=1
KBUILD_OPTIONS += CONFIG_WLAN_FEATURE_FILS=y
KBUILD_OPTIONS += CONFIG_FEATURE_COEX_PTA_CONFIG_ENABLE=y
KBUILD_OPTIONS += CONFIG_QCA_SUPPORT_TXRX_DRIVER_TCP_DEL_ACK=y
KBUILD_OPTIONS += CONFIG_WLAN_WAPI_MODE_11AC_DISABLE=y
KBUILD_OPTIONS += CONFIG_WLAN_WOW_PULSE=y
# Set in config.te-f30, but according to Kbuild not available in SDIO mode
#KBUILD_OPTIONS += CONFIG_PER_VDEV_TX_DESC_POOL=1

KBUILD_OPTIONS += $(KBUILD_EXTRA) # Extra config if any

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(shell pwd) modules $(KBUILD_OPTIONS)

modules_install:
	$(MAKE) INSTALL_MOD_STRIP=1 INSTALL_MOD_PATH=$(INSTALL_MOD_PATH) \
	   -C $(KERNEL_SRC) M=$(shell pwd) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(shell pwd) clean
