ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

ifeq ($(strip $(CTRULIB)),)
$(error "Please set CTRULIB in your environment. export DEVKITARM=<path to>ctrulib/libctru")
endif

ifeq ($(filter $(DEVKITARM)/bin,$(PATH)),)
export PATH:=$(DEVKITARM)/bin:$(PATH)
endif

DEFINES	:=	
LDPATH	:=	ccd00.ld

ifneq ($(strip $(LOADROPBIN)),)
	DEFINES	:=	$(DEFINES) -DLOADROPBIN=1
endif

ifneq ($(strip $(OTHERAPP)),)
	DEFINES	:=	$(DEFINES) -DOTHERAPP=1
	LDPATH	:=	otherapp.ld
endif

ifneq ($(strip $(QRINSTALLER)),)
	DEFINES	:=	$(DEFINES) -DQRINSTALLER=1
endif

ifneq ($(strip $(UDSPLOIT)),)
	DEFINES	:=	$(DEFINES) -DUDSPLOIT=1
endif

CC = arm-none-eabi-gcc
LINK = arm-none-eabi-gcc
# LINK = arm-none-eabi-ld
AS = arm-none-eabi-as
OBJCOPY = arm-none-eabi-objcopy
CFLAGS += -Wall -std=gnu11 -march=armv6 -Os -I"$(CTRULIB)/include" -I$(DEVKITPRO)/libnds/include $(DEFINES) -fPIE -static-pie -shared -fvisibility=hidden
LDFLAGS += -Wl,--script=$(LDPATH) -L"$(DEVKITARM)/arm-none-eabi/lib" -L"$(CTRULIB)/lib" -Wl,-Map=output.map -fPIE -static-pie -shared -fvisibility=hidden -Wl,--exclude-libs=ALL -nostartfiles -nostdlib

CFILES = $(wildcard source/*.c)
BINFILES = $(wildcard data/*.bin)
OFILES = $(BINFILES:data/%.bin=source/%.h)
OFILES += $(CFILES:source/%.c=build/%.o)
DFILES = $(CFILES:source/%.c=build/%.d)
SFILES = $(wildcard source/*.s)
OFILES += $(SFILES:source/%.s=build/%.o)
PROJECTNAME = ${shell basename "$(CURDIR)"}
CWD = "$(CURDIR)""

.PHONY:=all

all: $(PROJECTNAME).bin

ctrulib:
	cd "$(CTRULIB)" && make

$(PROJECTNAME).bin: $(PROJECTNAME).elf
	$(OBJCOPY) -O binary $< $@

$(PROJECTNAME).elf: $(OFILES)
	$(LINK) $(LDFLAGS) -o $(PROJECTNAME).elf $(filter-out build/crt0.o, $(OFILES)) -g -lctru -lm -lc -lg

clean:
	@rm -f build/*.o build/*.d
	@rm -f $(PROJECTNAME).elf $(PROJECTNAME).bin
	@rm -f source/menu_ropbin.h source/menu_payload_loadropbin.h source/menu_payload_regionfree.h source/cn_save_initial_loader.h
	@echo "all cleaned up !"

-include $(DFILES)

build/%.o: source/%.c
	$(CC) $(CFLAGS) -c $< -o $@
	@$(CC) -MM $< > build/$*.d

build/%.o: source/%.s
	$(CC) -x assembler-with-cpp $(CFLAGS) -c $< -o $@
	@$(CC) -MM $< > build/$*.d

source/%.h: data/%.bin
	@echo $(notdir $<)
	../bin2c/bin2c.exe $< $@ data_$(subst .,_,$(notdir $<))

