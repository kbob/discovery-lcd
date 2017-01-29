# tools
              AR := arm-none-eabi-ar
              CC := arm-none-eabi-gcc
             CXX := arm-none-eabi-g++
         OBJCOPY := arm-none-eabi-objcopy
          HOSTCC := cc
         HOSTCXX := c++
          HOSTLD := c++
            ECHO := $(if $(findstring s, $(MAKEFLAGS)),:,echo)

# submodules
      SUBMODULES := libopencm3 freetype2 agg
     OPENCM3_DIR := submodules/libopencm3
    FREETYPE_DIR := submodules/freetype2
         AGG_DIR := submodules/agg
   LDLIB_OPENCM3 := -lopencm3_stm32f4
  LDLIB_FREETYPE := -lfreetype
       LDLIB_AGG := -lagg

# compiler options
        CPPFLAGS := -DSTM32F4
        CPPFLAGS += -Isrc                                               \
                    -Iinclude                                           \
                    -I$(OPENCM3_DIR)/include                            \
                    -I$(FREETYPE_DIR)/include                           \
                    -I$(AGG_DIR)/include                                \
                    -I$(AGG_DIR)/font_freetype
     TARGET_ARCH := -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
             OPT := -O0
          CFLAGS := -MD -std=gnu99                                      \
                    -Wall -Wundef -Wextra -Wshadow -Werror              \
                    -Wimplicit-function-declaration -Wredundant-decls   \
                    -Wmissing-prototypes -Wstrict-prototypes            \
                    -Wno-parentheses                                    \
                    -g $(OPT)
        CXXFLAGS := -Wall -Werror -MD -g $(OPT)
         LDFLAGS := --static -nostartfiles                              \
                    -Lsrc                                               \
                    -L$(OPENCM3_DIR)/lib                                \
                    -L$(FREETYPE_DIR)/objs                              \
                    -L$(AGG_DIR)/src                                    \
                    -Tstm32f429i-discovery.ld -Wl,--gc-sections
    POST_LDFLAGS += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group

# ----------------------------------------------------------------------

all: $(SUBMODULES) $(LIBS) examples

# Included makefiles populate these.
          DFILES := # auto dependency files
            DIRT := # removed by `clean` targets
            LIBS := # project libraries
   EXAMPLE_ELVES := # ELF files

include src/Dir.make
include examples/Dir.make

examples: $(EXAMPLE_ELVES)

clean:
	rm -rf $(DIRT) $(DFILES)

realclean: clean
	$(MAKE) -C $(OPENCM3_DIR) clean
	$(MAKE) -C $(FREETYPE_DIR) clean
	$(MAKE) -C $(AGG_DIR) clean
	$(MAKE) -C $(AGG_DIR)/examples/macosx_sdl clean
	find $(AGG_DIR) -name '*.o' -exec rm -f '{}' ';'

# ---  libopencm3 submodule  -------------------------------------------

     OPENCM3_LIB := $(OPENCM3_DIR)/lib/libopencm3_stm32f4.a

libopencm3: $(OPENCM3_LIB)
        # XXX libopencm3 can't stop rebuilding the world.
        # XXX So if the library exists, do not invoke make.

$(OPENCM3_LIB):
	$(MAKE) -C $(OPENCM3_DIR) TARGETS=stm32/f4

# ---  freetype2 submodule  --------------------------------------------

   FREETYPE_CFGR := $(FREETYPE_DIR)/builds/unix/configure
 FREETYPE_CFG_MK := $(FREETYPE_DIR)/config.mk

freetype2: $(FREETYPE_CFG_MK)
        # Silent unless make does something
	@ $(MAKE) -s --no-print-dir -q -C $(FREETYPE_DIR) A=a O=o || {  \
	  $(ECHO) $(MAKE) -C $(FREETYPE_DIR) &&                         \
	  $(MAKE) -C $(FREETYPE_DIR)                                    \
	          AR="$(AR)"                                            \
	          CC="$(CC) $(TARGET_ARCH)"                             \
	          A=a                                                   \
	          O=o                                                   \
	          LINK_LIBRARY='$$(AR) cr $$@ $$(OBJECTS_LIST)'         \
	  ; }

$(FREETYPE_CFG_MK): $(FREETYPE_CFGR)
	$(MAKE) -C $(FREETYPE_DIR)                                      \
	        setup                                                   \
	        CFG="--disable-shared                                   \
	             --disable-mmap                                     \
	             --without-zlib                                     \
	             --without-bzip2                                    \
	             --without-png                                      \
	             --without-harfbuzz"

$(FREETYPE_CFGR):
	cd $(FREETYPE_DIR) && sh autogen.sh

# ---  agg submodule  --------------------------------------------------

agg:
        # Silent unless make does something
	@ $(MAKE) -s --no-print-dir -q -C $(AGG_DIR) || {               \
	    $(ECHO) $(MAKE) -C $(AGG_DIR) &&                            \
	    $(MAKE) -C $(AGG_DIR)                                       \
	            AR="$(AR)"                                          \
	            CC="$(CC)"                                          \
	            CXX="$(CXX) $(TARGET_ARCH)"                         \
	            LIB="$(AR) cr -s"                                   \
	            TARGET_ARCH="$(TARGET_ARCH)"                        \
	    ; }

# ---  Emacs tag file---------------------------------------------------

TAGS::
	{                                                               \
	    find src                                                    \
	         examples                                               \
	         $(OPENCM3_DIR)/lib/{cm3,stm32}                         \
	         $(FREETYPE_DIR)/src                                    \
	         $(AGG_DIR)                                             \
	         -name '*.c*';                                          \
	    find include                                                \
	         $(OPENCM3_DIR)/include/libopencm3/{cm3,stm32}          \
	         $(FREETYPE_DIR)/include                                \
	         $(AGG_DIR)                                             \
	         -name '*.h*';                                          \
	} | xargs etags -I

DIRT += TAGS

# ---  verify submodules populated  ------------------------------------

ifeq ($(wildcard $(OPENCM3_DIR)/*),)
    missing_submodule := libopencm3
endif

ifeq ($(wildcard $(FREETYPE_DIR)/*),)
    missing_submodule := freetype2
endif

ifeq ($(wildcard $(AGG_DIR)/*),)
    missing_submodule := agg
endif

ifdef missing_submodule
    # Hack: newline variable
    # https://stackoverflow.com/questions/17055773
    define n


    endef
    $(error $(missing_submodule) submodule is not initialized.$n\
            please run:$n\
            $$ git submodule init$n\
            $$ git submodule update$n\
            before running make)
endif

# ---  auto dependencies  ----------------------------------------------

-include $(DFILES)

.PHONY: all examples clean realclean libopencm3 freetype2 agg
