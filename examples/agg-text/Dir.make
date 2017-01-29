             D := examples/agg-text

          PROG := text
        CFILES := text.c
       CCFILES := main.cc agg-main.cc
      ACCFILES := agg_font_freetype.cpp
     $D_LDLIBS := -llcd $(LDLIB_AGG) $(LDLIB_FREETYPE) -lm $(LDLIB_OPENCM3)
  $D_FONT_FILE := $D/Lato-Light.ttf

     $D_CFILES := $(CFILES:%=$D/%)
    $D_CCFILES := $(CCFILES:%=$D/%)
   $D_ACCFILES := $(ACCFILES:%=$(AGGFT_DIR)/%)
     $D_OFILES := $($D_CFILES:%.c=%.o)                                  \
                  $($D_CCFILES:%.cc=%.o)                                \
                  $(ACCFILES:%.cpp=$D/%.o)                              \
                  $D/font.o
        $D_ELF := $D/$(PROG).elf

        DFILES += $($D_CFILES:%.c=%.d)                                  \
                  $($D_CCFILES:%.cc=%.d)                                \
                  $(ACCFILES:%.cpp=$D/%.d)
          DIRT += $($D_ELF) $($D_OFILES)
 EXAMPLE_ELVES += $($D_ELF)

$D/%.o: $D/%.cc
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$(ACCFILES:%.cpp=$D/%.o): CXXFLAGS := $(CXXFLAGS:-Wall=)
$D/%.o: $(AGGFT_DIR)/%.cpp
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

$($D_ELF): LDLIBS := $($D_LDLIBS)
$($D_ELF): $($D_OFILES) $(LIBGFX)
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) $(POST_LDFLAGS) -o $@

  RODATA_FLAGS := alloc,load,readonly,data,contents

$D/font.o: $($D_FONT_FILE)
	$(OBJCOPY) -I binary                                            \
	           -O elf32-littlearm                                   \
	           -B arm                                               \
	           --rename-section .data=.rodata,$(RODATA_FLAGS)       \
	            $^ $@
