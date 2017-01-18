            AR := arm-none-eabi-ar
            CC := arm-none-eabi-gcc
           CXX := arm-none-eabi-g++
       OBJCOPY := arm-none-eabi-objcopy
        HOSTCC := cc
       HOSTCXX := c++
        HOSTLD := c++

    SUBMODULES := libopencm3 freetype2 agg
   OPENCM3_DIR := submodules/libopencm3
  FREETYPE_DIR := submodules/freetype2
       AGG_DIR := submodules/agg

      CPPFLAGS := -DSTM32F4
      CPPFLAGS += -Isrc                                                 \
                  -Iinclude                                             \
                  -I$(OPENCM3_DIR)/include                              \
                  -I$(FREETYPE_DIR)/include                             \
                  -I$(AGG_DIR)/include
   TARGET_ARCH := -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16
           OPT := -O0
        CFLAGS := -MD -std=gnu99                                        \
                  -Wall -Wundef -Wextra -Wshadow -Werror                \
                  -Wimplicit-function-declaration -Wredundant-decls     \
                  -Wmissing-prototypes -Wstrict-prototypes              \
                  -Wno-parentheses                                      \
                  -g $(OPT)
      CXXFLAGS := -MD -g $(OPT)
       LDFLAGS := --static -nostartfiles                                \
                  -Lsrc                                                 \
                  -L$(OPENCM3_DIR)/lib                                  \
                  -L$(FREETYPE_DIR)/objs                                \
                  -L$(AGG_DIR)/src                                      \
                  -Tstm32f429i-discovery.ld -Wl,--gc-sections
  POST_LDFLAGS += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group
LDLIB_FREETYPE := -lfreetype
 LDLIB_OPENCM3 := -lopencm3_stm32f4
     LDLIB_AGG := -lagg

# Included makefiles populate these.
        DFILES :=
          DIRT := 
          LIBS :=
 EXAMPLE_ELVES :=


ifeq (,$(findstring s,$(MAKEFLAGS)))
	ECHO := echo
else
	ECHO := :
endif


all: $(SUBMODULES) $(LIBS) examples

include src/Dir.make
include examples/Dir.make

clean:
	rm -rf $(DIRT) $(DFILES)

realclean: clean
	$(MAKE) -C $(OPENCM3_DIR) clean
	$(MAKE) -C $(FREETYPE_DIR) clean
	$(MAKE) -C $(AGG_DIR) clean
	$(MAKE) -C $(AGG_DIR)/examples/macosx_sdl clean
	find $(AGG_DIR) -name '*.o' -exec rm -f '{}' ';'

.PHONY: clean realcleanlibopencm3 freetype2 agg examples

libopencm3:
        # XXX libopencm3 can't stop rebuilding the world.
	@ [ -f $(OPENCM3_DIR)/lib/libopencm3_stm32f4.a ] ||             \
	$(MAKE) -C $(OPENCM3_DIR) TARGETS=stm32/f4

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

examples: $(EXAMPLE_ELVES)

TAGS::
	{                                                               \
	    find src                                                    \
	         examples                                               \
	         $(OPENCM3_DIR)/lib/{cm3,stm32}                         \
	         $(FREETYPE_DIR)/src                                    \
	         -name '*.c*';                                          \
	    find include                                                \
	         $(OPENCM3_DIR)/include/libopencm3/{cm3,stm32}          \
	         $(FREETYPE_DIR)/include                                \
	         -name '*.h*';                                          \
	} | xargs etags -I

ifeq ($(wildcard $(OPENCM3_DIR)/*),)
    missing_submodule := libopencm3
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

-include $(DFILES)
