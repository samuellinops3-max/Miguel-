ifeq ($(strip $(PSL1GHT)),)
$(error "PSL1GHT nao definido - vai instalar no build.yml")
endif

include $(PSL1GHT)/ppu/rules.mk

TITLE       := Little Nightmares - Sombra
APPID       := LNBR00001
CONTENTID   := UP0001-LNBR00001_00-0000000000000000

TARGET      := little_nightmares
BUILD_PRX   := 1

CFILES      := main.c
CXXFILES    :=
INCLUDE     := -I$(PSL1GHT)/ppu/include -I$(PSL1GHT)/common/include
LIBDIRS     := -L$(PSL1GHT)/ppu/lib -L$(PSL1GHT)/common/lib
LIBS        := -lrsx -lgcm_sys -lio -lsysutil -lsysmodule -lm

all: $(TARGET).elf $(TARGET).self

clean:
	rm -rf $(TARGET).elf $(TARGET).self *.pkg *.o