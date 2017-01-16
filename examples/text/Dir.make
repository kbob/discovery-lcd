             D := examples/text

          PROG := text
        CFILES := main.c text.c
     $D_LDLIBS := -llcd $(LDLIB_FREETYPE) -lm $(LDLIB_OPENCM3)
  $D_FONT_FILE := $D/Lato-Light.ttf

     $D_CFILES := $(CFILES:%=$D/%)
     $D_OFILES := $($D_CFILES:%.c=%.o) $D/font.o
        $D_ELF := $D/$(PROG).elf

        DFILES += $($D_CFILES:%.c=%.d)
          DIRT += $($D_ELF) $($D_OFILES)
 EXAMPLE_ELVES += $($D_ELF)

$($D_ELF): LDLIBS := $($D_LDLIBS)
$($D_ELF): $($D_OFILES) $(LIBGFX)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) $(POST_LDFLAGS) -o $@

  RODATA_FLAGS := alloc,load,readonly,data,contents

$D/font.o: $($D_FONT_FILE)
	$(OBJCOPY) -I binary                                            \
	           -O elf32-littlearm                                   \
	           -B arm                                               \
	           --rename-section .data=.rodata,$(RODATA_FLAGS)       \
	            $^ $@
