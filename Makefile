#!/usr/bin/make -f
# Makefile for dark-plugins #
# ------------------------- #
# Created by falkTX
#

# ---------------------------------------------------------------------------------------------------------------------
# Read target compiler from environment

CXX ?= g++

# ---------------------------------------------------------------------------------------------------------------------
# Auto-detect target compiler if not defined

ifneq ($(shell echo -e escaped-by-default | grep -- '-e escaped-by-default'),-e escaped-by-default)
TARGET_COMPILER = $(shell echo -e '#ifdef __clang__\nclang\n#else\ngcc\n#endif' | $(CXX) -E -P -x c - 2>/dev/null)
else ifeq ($(shell echo '\#escaped-by-default' | grep -- '\#escaped-by-default'),\#escaped-by-default)
TARGET_COMPILER = $(shell echo '\#ifdef __clang__\nclang\n\#else\ngcc\n\#endif' | $(CXX) -E -P -x c - 2>/dev/null)
else
TARGET_COMPILER = $(shell echo '#ifdef __clang__\nclang\n#else\ngcc\n#endif' | $(CXX) -E -P -x c - 2>/dev/null)
endif

ifneq ($(CLANG),true)
ifneq ($(GCC),true)

ifneq (,$(findstring clang,$(TARGET_COMPILER)))
CLANG = true
else
GCC = true
endif

endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Auto-detect target OS if not defined

TARGET_MACHINE := $(shell $(CXX) -dumpmachine)

ifneq ($(BSD),true)
ifneq ($(HAIKU),true)
ifneq ($(HURD),true)
ifneq ($(LINUX),true)
ifneq ($(MACOS),true)
ifneq ($(WASM),true)
ifneq ($(WINDOWS),true)

ifneq (,$(findstring bsd,$(TARGET_MACHINE)))
BSD = true
else ifneq (,$(findstring haiku,$(TARGET_MACHINE)))
HAIKU = true
else ifneq (,$(findstring linux,$(TARGET_MACHINE)))
LINUX = true
else ifneq (,$(findstring gnu,$(TARGET_MACHINE)))
HURD = true
else ifneq (,$(findstring apple,$(TARGET_MACHINE)))
MACOS = true
else ifneq (,$(findstring mingw,$(TARGET_MACHINE)))
WINDOWS = true
else ifneq (,$(findstring msys,$(TARGET_MACHINE)))
WINDOWS = true
else ifneq (,$(findstring wasm,$(TARGET_MACHINE)))
WASM = true
else ifneq (,$(findstring windows,$(TARGET_MACHINE)))
WINDOWS = true
endif

endif # WINDOWS
endif # WASM
endif # MACOS
endif # LINUX
endif # HURD
endif # HAIKU
endif # BSD

# ---------------------------------------------------------------------------------------------------------------------
# Set build and link flags

FLAGS = -fPIC -Wall -Wextra -Wno-unused-parameter -pipe -MD -MP
FLAGS += -O3 -ffast-math -fdata-sections -ffunction-sections -fvisibility=hidden
FLAGS += -fno-strict-aliasing -flto
FLAGS += -DNDEBUG
ifeq ($(GCC),true)
FLAGS += -fno-gnu-unique
endif

CFLAGS += $(FLAGS) -std=gnu11
CXXFLAGS += $(FLAGS) -std=gnu++17
CXXFLAGS += -fvisibility-inlines-hidden
CXXFLAGS += -Idarkglass-lv2-extensions/dg-control-port-state-update.lv2

LDFLAGS += -flto -Werror=odr
ifeq ($(MACOS),true)
LDFLAGS += -Wl,-dead_strip,-dead_strip_dylibs,-x
else ifeq ($(WASM),true)
LDFLAGS += -O3
LDFLAGS += -sAGGRESSIVE_VARIABLE_ELIMINATION=1
LDFLAGS += -sENVIRONMENT=web
LDFLAGS += -sLLD_REPORT_UNDEFINED
LDFLAGS += -Wl,--gc-sections
else
LDFLAGS += -Wl,-O1,--as-needed,--gc-sections,--no-undefined,--strip-all
endif
ifeq ($(GCC),true)
LDFLAGS += -Werror=lto-type-mismatch
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set build targets

PLUGINS = dark-phaser dark-tremolo

TARGETS = $(PLUGINS:%=%.lv2/plugin.so)

# ---------------------------------------------------------------------------------------------------------------------
# Build rules

all: $(TARGETS)

%.so: $(subst .so,.cpp.o,%.so)
	$(CXX) $< $(LDFLAGS) -shared -o $@

%.cpp.o: %.cpp
	$(CXX) $< $(CXXFLAGS) -c -o $@

dark-phaser.lv2/%.cpp.o: CXXFLAGS += -Idsp-calf

dark-tremolo.lv2/%.cpp.o: CXXFLAGS += -Idsp-genlib

# ---------------------------------------------------------------------------------------------------------------------
# Cleanup

clean:
	rm -f *.lv2/*.so *.lv2/*.d *.lv2/*.o

# ---------------------------------------------------------------------------------------------------------------------
# Easy rebuilds

-include $(PLUGINS:%=%.lv2/plugin.cpp.d)
