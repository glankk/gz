CC                  = mips64-gcc
CXX                 = mips64-g++
LD                  = mips64-g++
OBJCOPY             = mips64-objcopy
LUAPATCH            = luapatch
override CFLAGS    += -std=c11
override CXXFLAGS  += -std=c++14
override CPPFLAGS  += -Wall -O3 -ffunction-sections -fdata-sections -flto -ffat-lto-objects
override INCLUDE   += -I $(N64ROOT)/include
LDSCRIPT            = $(N64ROOT)/ldscripts/gl-n64.ld
override LDFLAGS   += -T $(LDSCRIPT) -nostdlib -O3 -flto -Wl,--gc-sections
override LDLIBS    += -lstdc++ -lsupc++ -lc -lg -lm -lgcc -lnosys
LUAFILE             = $(EMUDIR)/Lua/patch-data.lua
GZ_VERSIONS         = oot-1.0 oot-1.2
GZ_ADDRESS          = 80600000
LDR_ADDRESS         = 80000400
SRCDIR              = src
OBJDIR              = obj
BINDIR              = bin
CFILES              = *.c
CXXFILES            = *.cpp *.cxx *.cc *.c++
GSCFILES            = *.gsc

gz-oot-1.0          : CPPFLAGS += -DZ64_VERSION=Z64_OOT10
gz-oot-1.2          : CPPFLAGS += -DZ64_VERSION=Z64_OOT12

GZ                  = $(foreach v,$(GZ_VERSIONS),gz-$(v))
all                 : $(GZ)
cleanall            :
	rm -rf $(OBJDIR) $(BINDIR)
.PHONY              : all cleanall

.SECONDEXPANSION:

define bin_template =
 NAME-$(1)          = $(2)
 DIR-$(1)           = $(3)
 SRCDIR-$(1)        = $(4)
 OBJDIR-$(1)        = $(5)
 BINDIR-$(1)        = $(6)
 ADDRESS-$(1)       = $(7)
 CSRC-$(1)         := $$(foreach s,$$(CFILES),$$(wildcard $$(SRCDIR-$(1))/$$(s)))
 CXXSRC-$(1)       := $$(foreach s,$$(CXXFILES),$$(wildcard $$(SRCDIR-$(1))/$$(s)))
 GSC-$(1)          := $$(foreach s,$$(GSCFILES),$$(wildcard $$(DIR-$(1))/$$(s)))
 COBJ-$(1)          = $$(patsubst $$(SRCDIR-$(1))/%,$$(OBJDIR-$(1))/%.o,$$(CSRC-$(1)))
 CXXOBJ-$(1)        = $$(patsubst $$(SRCDIR-$(1))/%,$$(OBJDIR-$(1))/%.o,$$(CXXSRC-$(1)))
 OBJ-$(1)           = $$(COBJ-$(1)) $$(CXXOBJ-$(1))
 BIN-$(1)           = $$(BINDIR-$(1))/$$(NAME-$(1)).bin
 ELF-$(1)           = $$(BINDIR-$(1))/$$(NAME-$(1)).elf
 $(1)               : LDFLAGS += -Wl,--defsym,start=0x$$(ADDRESS-$(1))
 $(1)               : $$(BIN-$(1))
 clean$(1)          :
	rm -rf $$(OBJDIR-$(1)) $$(BINDIR-$(1))
 .PHONY             : $(1) clean$(1)
 $$(BIN-$(1))       : $$(ELF-$(1)) | $$$$(dir $$$$@)
	$$(OBJCOPY) $$< $$@ -O binary
 $$(ELF-$(1))       : $$(OBJ-$(1)) | $$$$(dir $$$$@)
	$$(LD) $$(LDFLAGS) $$^ $$(LDLIBS) -o $$@
 $$(COBJ-$(1))      : $$(OBJDIR-$(1))/%.o: $$(SRCDIR-$(1))/% | $$$$(dir $$$$@)
	$$(CC) -c $$(CFLAGS) $$(CPPFLAGS) $$(INCLUDE) $$< -o $$@
 $$(CXXOBJ-$(1))    : $$(OBJDIR-$(1))/%.o: $$(SRCDIR-$(1))/% | $$$$(dir $$$$@)
	$$(CXX) -c $$(CXXFLAGS) $$(CPPFLAGS) $$(INCLUDE) $$< -o $$@

endef

define lua_template =
 $(1)-lua           : $(1)
	$$(LUAPATCH) $(2) $$(patsubst %,-text % ,$$(GSC-$(1))) -bin $$(ADDRESS-$(1)) $$(BIN-$(1))
 clean$(1)-lua      : clean$(1)
	rm -rf $2
 .PHONY             : $(1)-lua clean$(1)-lua

endef

$(foreach v,$(GZ_VERSIONS),$(eval \
 $(call bin_template,gz-$(v),gz,$(v),$(SRCDIR)/gz,$(OBJDIR)/gz/$(v),$(BINDIR)/gz/$(v),$(GZ_ADDRESS)) \
 $(call lua_template,gz-$(v),$(LUAFILE)) \
))

$(eval $(call bin_template,ldr,ldr,,$(SRCDIR)/ldr,$(OBJDIR)/ldr,$(BINDIR)/ldr,$(LDR_ADDRESS)))

%/                  :
	mkdir -p $@
