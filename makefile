ifndef PACKAGE_TARNAME
 PACKAGE_TARNAME      = gz
endif
ifndef PACKAGE_URL
 PACKAGE_URL          = github.com/glankk/gz
endif
AS                    = mips64-gcc
CC                    = mips64-gcc
CXX                   = mips64-g++
LD                    = mips64-g++
OBJCOPY               = mips64-objcopy
NM                    = mips64-nm
READELF               = mips64-readelf
GENHOOKS              = ./genhooks
LUAPATCH              = luapatch
GRC                   = grc
LDSCRIPT              = gl-n64.ld
CPPFLAGS              = -DPACKAGE_TARNAME="$(PACKAGE_TARNAME)" -DPACKAGE_URL="$(PACKAGE_URL)" $(VER_CPPFLAGS)
CFLAGS                = -std=gnu11 -Wall -ffunction-sections -fdata-sections $(VER_CFLAGS)
CXXFLAGS              = -std=gnu++14 -Wall -ffunction-sections -fdata-sections $(VER_CXXFLAGS)
LDFLAGS               = -T $(LDSCRIPT) -nostartfiles -specs=nosys.specs -Wl,--gc-sections $(VER_LDFLAGS)
LDLIBS                =
LUAFILE               = $(EMUDIR)/Lua/patch-data.lua
RESDESC               = $(RESDIR)/resources.json
GZ_VERSIONS           = oot-1.0 oot-1.1 oot-1.2 oot-vc
GZ_ADDRESS            = 80400060
LDR_ADDRESS           = 80000400
SRCDIR                = src
RESDIR                = res
OBJDIR                = obj
BINDIR                = bin
ASMFILES              = *.S
CFILES                = *.c
CXXFILES              = *.cpp *.cxx *.cc *.c++

OOT-1.0               = $(OBJ-gz-oot-1.0) $(ELF-gz-oot-1.0) gz-oot-1.0-hooks
OOT-1.1               = $(OBJ-gz-oot-1.1) $(ELF-gz-oot-1.0) gz-oot-1.1-hooks
OOT-1.2               = $(OBJ-gz-oot-1.2) $(ELF-gz-oot-1.0) gz-oot-1.2-hooks
VC                    = $(OBJ-gz-oot-vc) $(ELF-gz-oot-vc) gz-oot-vc-hooks
N64                   = $(OOT-1.0) $(OOT-1.1) $(OOT-1.2)

GZ                    = $(foreach v,$(GZ_VERSIONS),gz-$(v))
all                   : $(GZ)
clean                 : $(foreach v,$(GZ_VERSIONS),clean-gz-$(v)-hooks)
	rm -rf $(OBJDIR) $(BINDIR)
distclean             :
	rm -rf patch/ups patch/*.z64
.PHONY                : all clean distclean

.SECONDEXPANSION      :

define bin_template   =
 NAME-$(1)            = $(2)
 DIR-$(1)             = $(3)
 SRCDIR-$(1)          = $(4)
 RESDIR-$(1)          = $(5)
 OBJDIR-$(1)          = $(6)
 BINDIR-$(1)          = $(7)
 ADDRESS-$(1)         = $(8)
 ASMSRC-$(1)         := $$(foreach s,$$(ASMFILES),$$(wildcard $$(SRCDIR-$(1))/$$(s)))
 CSRC-$(1)           := $$(foreach s,$$(CFILES),$$(wildcard $$(SRCDIR-$(1))/$$(s)))
 CXXSRC-$(1)         := $$(foreach s,$$(CXXFILES),$$(wildcard $$(SRCDIR-$(1))/$$(s)))
 RESSRC-$(1)         := $$(wildcard $$(RESDIR-$(1))/*)
 ASMOBJ-$(1)          = $$(patsubst $$(SRCDIR-$(1))/%,$$(OBJDIR-$(1))/%.o,$$(ASMSRC-$(1)))
 COBJ-$(1)            = $$(patsubst $$(SRCDIR-$(1))/%,$$(OBJDIR-$(1))/%.o,$$(CSRC-$(1)))
 CXXOBJ-$(1)          = $$(patsubst $$(SRCDIR-$(1))/%,$$(OBJDIR-$(1))/%.o,$$(CXXSRC-$(1)))
 RESOBJ-$(1)          = $$(patsubst $$(RESDIR-$(1))/%,$$(OBJDIR-$(1))/$$(RESDIR)/%.o,$$(RESSRC-$(1)))
 OBJ-$(1)             = $$(ASMOBJ-$(1)) $$(COBJ-$(1)) $$(CXXOBJ-$(1)) $$(RESOBJ-$(1))
 DEPS-$(1)            = $$(patsubst %.o,%.d,$$(OBJ-$(1)))
 BIN-$(1)             = $$(BINDIR-$(1))/$$(NAME-$(1)).bin
 ELF-$(1)             = $$(BINDIR-$(1))/$$(NAME-$(1)).elf
 BUILD-$(1)           = $(1)
 CLEAN-$(1)           = clean-$(1)
 -include $$(DEPS-$(1))
 $$(ELF-$(1))         : LDFLAGS              += -Wl,--defsym,start=0x$$(ADDRESS-$(1))
 $$(BUILD-$(1))       : $$(BIN-$(1))
 $$(CLEAN-$(1))       :
	rm -rf $$(OBJDIR-$(1)) $$(BINDIR-$(1))
 .PHONY               : $$(BUILD-$(1)) $$(CLEAN-$(1))
 $$(BIN-$(1))         : $$(ELF-$(1)) | $$$$(dir $$$$@)
	$$(OBJCOPY) -S -O binary $$< $$@
 $$(ELF-$(1))         : $$(OBJ-$(1)) | $$$$(dir $$$$@)
	$$(LD) $$(LDFLAGS) $$^ $$(LDLIBS) -o $$@
 $$(ASMOBJ-$(1))      : $$(OBJDIR-$(1))/%.o: $$(SRCDIR-$(1))/% | $$$$(dir $$$$@)
	$$(AS) -c -MMD -MP $$(CPPFLAGS) $$< -o $$@
 $$(COBJ-$(1))        : $$(OBJDIR-$(1))/%.o: $$(SRCDIR-$(1))/% | $$$$(dir $$$$@)
	$$(CC) -c -MMD -MP $$(CPPFLAGS) $$(CFLAGS) $$< -o $$@
 $$(CXXOBJ-$(1))      : $$(OBJDIR-$(1))/%.o: $$(SRCDIR-$(1))/% | $$$$(dir $$$$@)
	$$(CXX) -c -MMD -MP $$(CPPFLAGS) $$(CXXFLAGS) $$< -o $$@
 $$(RESOBJ-$(1))      : $$(OBJDIR-$(1))/$$(RESDIR)/%.o: $$(RESDIR-$(1))/% $$(RESDESC) | $$$$(dir $$$$@)
	$$(GRC) $$< -d $$(RESDESC) -o $$@

endef

define hooks_template =
 HOOKS-$(1)           = $$(DIR-$(1))/hooks.gsc
 $$(HOOKS-$(1))       : $$(ELF-$(1))
	AS="$$(AS)" CC="$$(CC)" NM="$$(NM)" READELF="$$(READELF)" CPPFLAGS="$$(subst ",\",$$(CPPFLAGS))" $$(GENHOOKS) $$< > $$@
 $(1)-hooks           : $$(HOOKS-$(1))
 clean-$(1)-hooks     :
	rm -f $$(HOOKS-$(1))
 .PHONY               : $(1)-hooks clean-$(1)-hooks

endef

define lua_template   =
 BUILD-$(1)-lua       = $(1)-lua
 CLEAN-$(1)-lua       = clean-$(1)-lua
 $(1)-lua             : $(1)
 clean-$(1)-lua       : clean-$(1)
 $$(BUILD-$(1)-lua)   : $(1)-hooks
	$$(LUAPATCH) $(2) -text $$(HOOKS-$(1)) -bin $$(ADDRESS-$(1)) $$(BIN-$(1))
 $$(CLEAN-$(1)-lua)   :
	rm -f $(2)
 .PHONY               : $$(BUILD-$(1)-lua) $$(CLEAN-$(1)-lua)

endef

$(foreach v,$(GZ_VERSIONS),$(eval \
 $(call bin_template,gz-$(v),gz,$(v),$(SRCDIR)/gz,$(RESDIR)/gz,$(OBJDIR)/gz/$(v),$(BINDIR)/gz/$(v),$(GZ_ADDRESS)) \
 $(call hooks_template,gz-$(v)) \
 $(call lua_template,gz-$(v),$(LUAFILE)) \
))

$(OOT-1.0)            : VER_CPPFLAGS          = -DZ64_VERSION=Z64_OOT10
$(OOT-1.1)            : VER_CPPFLAGS          = -DZ64_VERSION=Z64_OOT11
$(OOT-1.2)            : VER_CPPFLAGS          = -DZ64_VERSION=Z64_OOT12
$(VC)                 : VER_CPPFLAGS          = -DZ64_VERSION=Z64_OOT12 -DWIIVC
$(N64)                : VER_CFLAGS            = -O3 -flto -ffat-lto-objects
$(VC)                 : VER_CFLAGS            = -O1 -fno-reorder-blocks
$(N64)                : VER_CXXFLAGS          = -O3 -flto -ffat-lto-objects
$(VC)                 : VER_CXXFLAGS          = -O1 -fno-reorder-blocks
$(N64)                : VER_LDFLAGS           = -O3 -flto

$(eval $(call bin_template,ldr,ldr,,$(SRCDIR)/ldr,$(RESDIR)/ldr,$(OBJDIR)/ldr,$(BINDIR)/ldr,$(LDR_ADDRESS)))

%/                    :
	mkdir -p $@
