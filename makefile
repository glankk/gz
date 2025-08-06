PACKAGE_TARNAME      ?= gz
PACKAGE_URL          ?= github.com/glankk/gz
ifeq ($(origin PACKAGE_VERSION), undefined)
PACKAGE_VERSION      := $(shell git describe --tags --match 'v[0-9]*' --dirty 2>/dev/null)
ifeq ('$(PACKAGE_VERSION)', '')
PACKAGE_VERSION       = Unknown version
endif
endif
TARGET_TOOLS          = mips64-ultra-elf mips64
target               := $(shell for i in $(TARGET_TOOLS); do if type $${i}-gcc >/dev/null 2>&1; then echo $${i}; break; fi done)
program_prefix        = $(target)-
AS                    = $(program_prefix)as
CCAS                  = $(program_prefix)gcc -x assembler-with-cpp
CPP                   = $(program_prefix)cpp
CC                    = $(program_prefix)gcc
CXX                   = $(program_prefix)g++
LD                    = $(program_prefix)g++
OBJCOPY               = $(program_prefix)objcopy
NM                    = $(program_prefix)nm
GENHOOKS              = AS='$(AS)' OBJCOPY='$(OBJCOPY)' NM='$(NM)' ./genhooks
LUAPATCH              = luapatch
GRC                   = AS='$(AS)' grc
LDSCRIPT              = gl-n64.ld
ALL_CPPFLAGS          = -DPACKAGE_TARNAME='$(PACKAGE_TARNAME)' -DPACKAGE_URL='$(PACKAGE_URL)' -DPACKAGE_VERSION='$(PACKAGE_VERSION)' -DF3DEX_GBI_2 $(CPPFLAGS) $(EXTRA_CPPFLAGS)
ALL_CFLAGS            = -std=gnu11 -Wall -ffunction-sections -fdata-sections -mno-check-zero-division $(CFLAGS) $(EXTRA_CFLAGS)
ALL_CXXFLAGS          = -std=gnu++14 -Wall -ffunction-sections -fdata-sections -mno-check-zero-division $(CXXFLAGS) $(EXTRA_CXXFLAGS)
ALL_LDFLAGS           = -T $(LDSCRIPT) -L$(LIBDIR) -nostartfiles -specs=nosys.specs -Wl,--gc-sections $(LDFLAGS) $(EXTRA_LDFLAGS)
ALL_LDLIBS            = $(LDLIBS)
LUAFILE               = $(EMUDIR)/Lua/patch-data.lua
RESDESC               = $(RESDIR)/resources.json
GZ_VERSIONS           = oot-1.0 oot-1.1 oot-1.2 oot-mq-j oot-mq-u oot-gc-j oot-gc-u oot-ce-j oot-1.0-vc oot-1.1-vc oot-1.2-vc oot-mq-j-vc oot-mq-u-vc oot-gc-j-vc oot-gc-u-vc oot-ce-j-vc
GZ_ADDRESS            = 80400060
LDR_ADDRESS           = 80000400
SRCDIR                = src
RESDIR                = res
LIBDIR                = lib
OBJDIR                = obj
BINDIR                = bin
HOOKDIR               = hooks
ASMFILES              = *.S
CFILES                = *.c
CXXFILES              = *.cpp *.cxx *.cc *.c++

OBJ-OOT-1.0           = $(OBJ-gz-oot-1.0) $(OBJ-gz-oot-1.0-vc)
OBJ-OOT-1.1           = $(OBJ-gz-oot-1.1) $(OBJ-gz-oot-1.1-vc)
OBJ-OOT-1.2           = $(OBJ-gz-oot-1.2) $(OBJ-gz-oot-1.2-vc)
OBJ-OOT-MQ-J          = $(OBJ-gz-oot-mq-j) $(OBJ-gz-oot-mq-j-vc)
OBJ-OOT-MQ-U          = $(OBJ-gz-oot-mq-u) $(OBJ-gz-oot-mq-u-vc)
OBJ-OOT-GC-J          = $(OBJ-gz-oot-gc-j) $(OBJ-gz-oot-gc-j-vc)
OBJ-OOT-GC-U          = $(OBJ-gz-oot-gc-u) $(OBJ-gz-oot-gc-u-vc)
OBJ-OOT-CE-J          = $(OBJ-gz-oot-ce-j) $(OBJ-gz-oot-ce-j-vc)
ELF-OOT-1.0           = $(ELF-gz-oot-1.0) $(ELF-gz-oot-1.0-vc)
ELF-OOT-1.1           = $(ELF-gz-oot-1.1) $(ELF-gz-oot-1.1-vc)
ELF-OOT-1.2           = $(ELF-gz-oot-1.2) $(ELF-gz-oot-1.2-vc)
ELF-OOT-MQ-J          = $(ELF-gz-oot-mq-j) $(ELF-gz-oot-mq-j-vc)
ELF-OOT-MQ-U          = $(ELF-gz-oot-mq-u) $(ELF-gz-oot-mq-u-vc)
ELF-OOT-GC-J          = $(ELF-gz-oot-gc-j) $(ELF-gz-oot-gc-j-vc)
ELF-OOT-GC-U          = $(ELF-gz-oot-gc-u) $(ELF-gz-oot-gc-u-vc)
ELF-OOT-CE-J          = $(ELF-gz-oot-ce-j) $(ELF-gz-oot-ce-j-vc)
OBJ-N64               = $(OBJ-gz-oot-1.0) $(OBJ-gz-oot-1.1) $(OBJ-gz-oot-1.2) $(OBJ-gz-oot-mq-j) $(OBJ-gz-oot-mq-u) $(OBJ-gz-oot-gc-j) $(OBJ-gz-oot-gc-u) $(OBJ-gz-oot-ce-j)
OBJ-VC                = $(OBJ-gz-oot-1.0-vc) $(OBJ-gz-oot-1.1-vc) $(OBJ-gz-oot-1.2-vc) $(OBJ-gz-oot-mq-j-vc) $(OBJ-gz-oot-mq-u-vc) $(OBJ-gz-oot-gc-j-vc) $(OBJ-gz-oot-gc-u-vc) $(OBJ-gz-oot-ce-j-vc)
ELF-N64               = $(ELF-gz-oot-1.0) $(ELF-gz-oot-1.1) $(ELF-gz-oot-1.2) $(ELF-gz-oot-mq-j) $(ELF-gz-oot-mq-u) $(ELF-gz-oot-gc-j) $(ELF-gz-oot-gc-u) $(ELF-gz-oot-ce-j)
ELF-VC                = $(ELF-gz-oot-1.0-vc) $(ELF-gz-oot-1.1-vc) $(ELF-gz-oot-1.2-vc) $(ELF-gz-oot-mq-j-vc) $(ELF-gz-oot-mq-u-vc) $(ELF-gz-oot-gc-j-vc) $(ELF-gz-oot-gc-u-vc) $(ELF-gz-oot-ce-j-vc)

GZ                    = $(foreach v,$(GZ_VERSIONS),gz-$(v))
HOOKS                 = $(foreach v,$(GZ_VERSIONS),gz-$(v)-hooks)
all                   : $(GZ)
all-hooks             : $(HOOKS)
clean                 :
	rm -rf $(OBJDIR) $(BINDIR) $(HOOKDIR)
clean-hooks           : $(foreach v,$(HOOKS),clean-$(v))
distclean             :
	rm -rf ups *.z64 *.wad
.PHONY                : all clean distclean

all-homeboy           :
	cd homeboy && $(MAKE) all
clean-homeboy         :
	cd homeboy && $(MAKE) clean
.PHONY                : all-homeboy clean-homeboy

define bin_template
NAME-$(1)             = $(2)
SRCDIR-$(1)           = $(3)
RESDIR-$(1)           = $(4)
OBJDIR-$(1)           = $(5)
BINDIR-$(1)           = $(6)
HOOKDIR-$(1)          = $(7)
OUTDIR-$(1)           = $$(OBJDIR-$(1)) $$(OBJDIR-$(1))/$$(RESDIR) $$(BINDIR-$(1)) $$(HOOKDIR-$(1))
ADDRESS-$(1)          = $(8)
ASMSRC-$(1)          := $$(foreach s,$$(ASMFILES),$$(wildcard $$(SRCDIR-$(1))/$$(s)))
CSRC-$(1)            := $$(foreach s,$$(CFILES),$$(wildcard $$(SRCDIR-$(1))/$$(s)))
CXXSRC-$(1)          := $$(foreach s,$$(CXXFILES),$$(wildcard $$(SRCDIR-$(1))/$$(s)))
RESSRC-$(1)          := $$(wildcard $$(RESDIR-$(1))/*)
ASMOBJ-$(1)           = $$(patsubst $$(SRCDIR-$(1))/%,$$(OBJDIR-$(1))/%.o,$$(ASMSRC-$(1)))
COBJ-$(1)             = $$(patsubst $$(SRCDIR-$(1))/%,$$(OBJDIR-$(1))/%.o,$$(CSRC-$(1)))
CXXOBJ-$(1)           = $$(patsubst $$(SRCDIR-$(1))/%,$$(OBJDIR-$(1))/%.o,$$(CXXSRC-$(1)))
RESOBJ-$(1)           = $$(patsubst $$(RESDIR-$(1))/%,$$(OBJDIR-$(1))/$$(RESDIR)/%.o,$$(RESSRC-$(1)))
OBJ-$(1)              = $$(ASMOBJ-$(1)) $$(COBJ-$(1)) $$(CXXOBJ-$(1)) $$(RESOBJ-$(1))
DEPS-$(1)             = $$(patsubst %.o,%.d,$$(OBJ-$(1)))
BIN-$(1)              = $$(BINDIR-$(1))/$$(NAME-$(1)).bin
ELF-$(1)              = $$(BINDIR-$(1))/$$(NAME-$(1)).elf
BUILD-$(1)            = $(1)
CLEAN-$(1)            = clean-$(1)
-include $$(DEPS-$(1))
$$(ELF-$(1))          : ALL_LDFLAGS          += -Wl,--defsym,start=0x$$(ADDRESS-$(1))
$$(BUILD-$(1))        : $$(BIN-$(1))
$$(CLEAN-$(1))        :
	rm -rf $$(OUTDIR-$(1))
.PHONY                : $$(BUILD-$(1)) $$(CLEAN-$(1))
$$(BIN-$(1))          : $$(ELF-$(1)) | $$(BINDIR-$(1))
	$$(OBJCOPY) -S -O binary $$< $$@
$$(ELF-$(1))          : $$(OBJ-$(1)) | $$(BINDIR-$(1))
	$$(LD) $$(ALL_LDFLAGS) $$^ $$(ALL_LDLIBS) -o $$@
$$(ASMOBJ-$(1))       : $$(OBJDIR-$(1))/%.o: $$(SRCDIR-$(1))/% | $$(OBJDIR-$(1))
	$$(CCAS) -c -MMD -MP $$(ALL_CPPFLAGS) $$< -o $$@
$$(COBJ-$(1))         : $$(OBJDIR-$(1))/%.o: $$(SRCDIR-$(1))/% | $$(OBJDIR-$(1))
	$$(CC) -c -MMD -MP $$(ALL_CPPFLAGS) $$(ALL_CFLAGS) $$< -o $$@
$$(CXXOBJ-$(1))       : $$(OBJDIR-$(1))/%.o: $$(SRCDIR-$(1))/% | $$(OBJDIR-$(1))
	$$(CXX) -c -MMD -MP $$(ALL_CPPFLAGS) $$(ALL_CXXFLAGS) $$< -o $$@
$$(RESOBJ-$(1))       : $$(OBJDIR-$(1))/$$(RESDIR)/%.o: $$(RESDIR-$(1))/% $$(RESDESC) | $$(OBJDIR-$(1))/$$(RESDIR)
	$$(GRC) $$< -d $$(RESDESC) -o $$@
$$(OUTDIR-$(1))       :
	mkdir -p $$@

endef

define hooks_template
HOOKS-$(1)            = $$(HOOKDIR-$(1))/gz.gsc
$$(HOOKS-$(1))        : $$(ELF-$(1)) | $$(HOOKDIR-$(1))
	$$(GENHOOKS) $$< >$$@
$(1)-hooks            : $$(HOOKS-$(1))
clean-$(1)-hooks      :
	rm -f $$(HOOKS-$(1))
.PHONY                : $(1)-hooks clean-$(1)-hooks

endef

define lua_template
BUILD-$(1)-lua        = $(1)-lua
CLEAN-$(1)-lua        = clean-$(1)-lua
$(1)-lua              : $(1)
clean-$(1)-lua        : clean-$(1)
$$(BUILD-$(1)-lua)    : $(1)-hooks
	$$(LUAPATCH) $(2) -text $$(HOOKS-$(1)) -bin $$(ADDRESS-$(1)) $$(BIN-$(1))
$$(CLEAN-$(1)-lua)    :
	rm -f $(2)
.PHONY                : $$(BUILD-$(1)-lua) $$(CLEAN-$(1)-lua)

endef

$(foreach v,$(GZ_VERSIONS),$(eval \
  $(call bin_template,gz-$(v),gz,$(SRCDIR)/gz,$(RESDIR)/gz,$(OBJDIR)/gz/$(v),$(BINDIR)/gz/$(v),$(HOOKDIR)/gz/$(v),$(GZ_ADDRESS)) \
  $(call hooks_template,gz-$(v)) \
  $(call lua_template,gz-$(v),$(LUAFILE)) \
))

$(OBJ-OOT-1.0)        : ALL_CPPFLAGS         += -DZ64_VERSION=Z64_OOT10
$(OBJ-OOT-1.1)        : ALL_CPPFLAGS         += -DZ64_VERSION=Z64_OOT11
$(OBJ-OOT-1.2)        : ALL_CPPFLAGS         += -DZ64_VERSION=Z64_OOT12
$(OBJ-OOT-MQ-J)       : ALL_CPPFLAGS         += -DZ64_VERSION=Z64_OOTMQJ
$(OBJ-OOT-MQ-U)       : ALL_CPPFLAGS         += -DZ64_VERSION=Z64_OOTMQU
$(OBJ-OOT-GC-J)       : ALL_CPPFLAGS         += -DZ64_VERSION=Z64_OOTGCJ
$(OBJ-OOT-GC-U)       : ALL_CPPFLAGS         += -DZ64_VERSION=Z64_OOTGCU
$(OBJ-OOT-CE-J)       : ALL_CPPFLAGS         += -DZ64_VERSION=Z64_OOTCEJ
$(ELF-OOT-1.0)        : ALL_LDLIBS           += -loot-1.0
$(ELF-OOT-1.1)        : ALL_LDLIBS           += -loot-1.1
$(ELF-OOT-1.2)        : ALL_LDLIBS           += -loot-1.2
$(ELF-OOT-MQ-J)       : ALL_LDLIBS           += -loot-mq-j
$(ELF-OOT-MQ-U)       : ALL_LDLIBS           += -loot-mq-u
$(ELF-OOT-GC-J)       : ALL_LDLIBS           += -loot-gc-j
$(ELF-OOT-GC-U)       : ALL_LDLIBS           += -loot-gc-u
$(ELF-OOT-CE-J)       : ALL_LDLIBS           += -loot-ce-j
ifeq '$(target)' 'mips64-ultra-elf'
$(OBJ-VC)             : ALL_CFLAGS           += -n64-wiivc
$(OBJ-VC)             : ALL_CXXFLAGS         += -n64-wiivc
$(ELF-VC)             : ALL_LDFLAGS          += -n64-wiivc
else
$(OBJ-VC)             : ALL_CPPFLAGS         += -DWIIVC
$(OBJ-VC)             : ALL_CFLAGS           += -fno-reorder-blocks -fno-optimize-sibling-calls
$(OBJ-VC)             : ALL_CXXFLAGS         += -fno-reorder-blocks -fno-optimize-sibling-calls
$(ELF-VC)             : ALL_LDFLAGS          += -fno-reorder-blocks -fno-optimize-sibling-calls
endif

$(OBJ-N64)            : CFLAGS               ?= -O2 -g -flto=auto
$(OBJ-N64)            : CXXFLAGS             ?= -O2 -g -flto=auto
$(ELF-N64)            : LDFLAGS              ?= -O2 -g -flto=auto
$(OBJ-VC)             : CFLAGS               ?= -Os -g -flto=auto
$(OBJ-VC)             : CXXFLAGS             ?= -Os -g -flto=auto
$(ELF-VC)             : LDFLAGS              ?= -Os -g -flto=auto

$(eval $(call bin_template,ldr,ldr,$(SRCDIR)/ldr,$(RESDIR)/ldr,$(OBJDIR)/ldr,$(BINDIR)/ldr,$(HOOKDIR)/ldr,$(LDR_ADDRESS)))
