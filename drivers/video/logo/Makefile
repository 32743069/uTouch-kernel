# Makefile for the Linux logos

obj-$(CONFIG_LOGO)			+= logo.o
obj-$(CONFIG_LOGO_LINUX_MONO)		+= logo_linux_mono.o
obj-$(CONFIG_LOGO_LINUX_VGA16)		+= logo_linux_vga16.o
obj-$(CONFIG_LOGO_LINUX_CLUT224_TR785)	+= tr785_logo_linux_clut224.o
obj-$(CONFIG_LOGO_LINUX_CLUT224_TR7088)	+= tr7088_logo_linux_clut224.o
obj-$(CONFIG_LOGO_LINUX_CLUT224_TR7078)	+= tr7078_logo_linux_clut224.o
obj-$(CONFIG_LOGO_LINUX_CLUT224_TR7088TN)	+= tr7088tn_logo_linux_clut224.o
obj-$(CONFIG_LOGO_LINUX_CLUT224_TR101Q)	+= tr101q_logo_linux_clut224.o
obj-$(CONFIG_TCHIP_TR101Q_HAVE_CHARGE_LOGO) += tr101q_charging_logo_linux_clut224.o
obj-$(CONFIG_LOGO_LINUX_CLUT224_TR838)	+= tr838_logo_linux_clut224.o
obj-$(CONFIG_LOGO_LINUX_CLUT224_XBT_3188)	+= xbt_3188_logo_linux_clut224.o
obj-$(CONFIG_LOGO_LINUX_CLUT224)	+= logo_linux_clut224.o
obj-$(CONFIG_LOGO_PIPO_CLUT224)	    += logo_pipo_clut224.o
obj-$(CONFIG_LOGO_BLACKFIN_CLUT224)	+= logo_blackfin_clut224.o
obj-$(CONFIG_LOGO_BLACKFIN_VGA16)	+= logo_blackfin_vga16.o
obj-$(CONFIG_LOGO_DEC_CLUT224)		+= logo_dec_clut224.o
obj-$(CONFIG_LOGO_MAC_CLUT224)		+= logo_mac_clut224.o
obj-$(CONFIG_LOGO_PARISC_CLUT224)	+= logo_parisc_clut224.o
obj-$(CONFIG_LOGO_SGI_CLUT224)		+= logo_sgi_clut224.o
obj-$(CONFIG_LOGO_SUN_CLUT224)		+= logo_sun_clut224.o
obj-$(CONFIG_LOGO_SUPERH_MONO)		+= logo_superh_mono.o
obj-$(CONFIG_LOGO_SUPERH_VGA16)		+= logo_superh_vga16.o
obj-$(CONFIG_LOGO_SUPERH_CLUT224)	+= logo_superh_clut224.o
obj-$(CONFIG_LOGO_M32R_CLUT224)		+= logo_m32r_clut224.o
obj-$(CONFIG_LOGO_G3_CLUT224)		+= logo_g3_clut224.o
obj-$(CONFIG_LOGO_LINUX_800x480_CLUT224)        += logo_linux_800x480_clut224.o
obj-$(CONFIG_LOGO_LINUX_BMP_SUNSET)	+= logo_sunset_bmp.o
obj-$(CONFIG_LOGO_LINUX_BMP_ANDROID)     += logo_android_bmp.o
obj-$(CONFIG_LOGO_LINUX_BMP_ANDROID_TR7888_KNC)     += tr7888_knc_logo_android_bmp.o
obj-$(CONFIG_LOGO_LOWERPOWER_WARNING)   += logo_linux_lowerpower_clut224.o

obj-$(CONFIG_LOGO_CHARGER_CLUT224)         += logo_charger00_clut224.o logo_charger01_clut224.o logo_charger02_clut224.o logo_charger03_clut224.o logo_charger04_clut224.o logo_charger05_clut224.o logo_charger06_clut224.o logo_charger07_clut224.o logo_charger08_clut224.o 

obj-$(CONFIG_SPU_BASE)			+= logo_spe_clut224.o

obj-y  += tchip_sysInf.o
# How to generate logo's

# Use logo-cfiles to retrieve list of .c files to be built
logo-cfiles = $(notdir $(patsubst %.$(2), %.c, \
              $(wildcard $(srctree)/$(src)/*$(1).$(2))))


# Mono logos
extra-y += $(call logo-cfiles,_mono,pbm)

# VGA16 logos
extra-y += $(call logo-cfiles,_vga16,ppm)

# 224 Logos
extra-y += $(call logo-cfiles,_clut224,ppm)

# Gray 256
extra-y += $(call logo-cfiles,_gray256,pgm)

extra-y += $(call logo-cfiles,_bmp,bmp)

bmptologo := scripts/bmptologo
pnmtologo := scripts/pnmtologo
sysinfgcc := scripts/tchip_sysInfgcc

# Create commands like "pnmtologo -t mono -n logo_mac_mono -o ..."
quiet_cmd_logo = LOGO    $@
	cmd_logo = $(pnmtologo) \
			-t $(patsubst $*_%,%,$(notdir $(basename $<))) \
			-n $(notdir $(basename $<)) -o $@ $<

$(obj)/tchip_sysInf.c:  $(sysinfgcc) FORCE
	        $(sysinfgcc)

quiet_cmd_bmplogo = LOGO    $@
	cmd_bmplogo = $(bmptologo) \
			-t $(patsubst $*_%,%,$(notdir $(basename $<))) \
			-n $(notdir $(basename $<)) -o $@ $<

$(obj)/%_mono.c: $(src)/%_mono.pbm $(pnmtologo) FORCE
	$(call if_changed,logo)

$(obj)/%_vga16.c: $(src)/%_vga16.ppm $(pnmtologo) FORCE
	$(call if_changed,logo)

$(obj)/%_clut224.c: $(src)/%_clut224.ppm $(pnmtologo) FORCE
	$(call if_changed,logo)

$(obj)/%_gray256.c: $(src)/%_gray256.pgm $(pnmtologo) FORCE
	$(call if_changed,logo)

$(obj)/%_bmp.c: $(src)/%_bmp.bmp $(bmptologo) FORCE
	$(call if_changed,bmplogo)

ifeq (y,$(CONFIG_LOGO_LINUX_CLUT224_TR101Q))
customelogo := $(src)/../../../../device/rockchip/tr101q/customer/tr101q_logo_linux_clut224.ppm
publiclogo := $(src)/tr101q_public_clut224.ppm
ifneq ( , $(wildcard $(customelogo)))
$(obj)/tr101q_logo_linux_clut224.c: $(customelogo) $(pnmtologo) FORCE
	$(pnmtologo) -t clut224 -n tr101q_logo_linux_clut224 -o $(src)/tr101q_logo_linux_clut224.c $(customelogo)
else
$(obj)/tr101q_logo_linux_clut224.c: $(publiclogo) $(pnmtologo) FORCE
	$(pnmtologo) -t clut224 -n tr101q_logo_linux_clut224 -o $(src)/tr101q_logo_linux_clut224.c $(publiclogo)
endif # have customelogo
endif # tr101q

# Files generated that shall be removed upon make clean
clean-files := *.o *_mono.c *_vga16.c *_clut224.c *_gray256.c *_bmp.c
