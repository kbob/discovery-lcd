             D := examples/prep-text

          PROG := prep-text
        CFILES := main.c
     $D_LDLIBS := -llcd -lm $(LDLIB_OPENCM3)

     $D_CFILES := $(CFILES:%=$D/%)
     $D_OFILES := $($D_CFILES:%.c=%.o)
        $D_ELF := $D/$(PROG).elf

       PIXMAPS := $(patsubst %,%-pixmap.inc, fhpn)
    $D_PIXMAPS := $(PIXMAPS:%=$D/%)
   TEXT_STRING := "Hello, World!"
spitmap_CCFILES := spitmap.cc freetype.cc agg.cc graymap.cc unicode.cc output.cc
$D/spitmap_CCFILES := $(spitmap_CCFILES:%=$D/%)
$D/spitmap_OFILES := $($D/spitmap_CCFILES:%.cc=%.o)

        DFILES += $($D_CFILES:%.c=%.d) $($D/spitmap_CCFILES:%.cc=%.d)
          DIRT += $($D_ELF) $($D_OFILES)
          DIRT += $D/spitmap $($D/spitmap_OFILES)
          DIRT += $($D_PIXMAPS)
 EXAMPLE_ELVES += $($D_ELF)

$($D_ELF): LDLIBS := $($D_LDLIBS)
$($D_ELF): $($D_OFILES) $(LIBGFX)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) $(POST_LDFLAGS) -o $@

$D/main.o: $($D_PIXMAPS)

$D/fhpn-pixmap.inc: $D/spitmap
	$D/spitmap -o $@ --font=Lato-Light fhpn $(TEXT_STRING)

$D/spitmap:          CC := $(HOSTCXX)
$D/spitmap:     LDFLAGS :=
$D/spitmap:      LDLIBS :=
$D/spitmap:     LDFLAGS += -L/opt/local/lib
$D/spitmap:      LDLIBS += -lagg -lfreetype
# $D/spitmap:     LDFLAGS := -L$(AGG_DIR)/src -L/opt/local/lib
# $D/spitmap:      LDLIBS := $(AGG_DIR)/font_freetype/agg_font_freetype.o
# $D/spitmap:      LDLIBS += -lagg -lfreetype
$D/spitmap: TARGET_ARCH :=
$D/spitmap: $($D/spitmap_OFILES)

$($D/spitmap_OFILES):         CXX := $(HOSTCXX)
$($D/spitmap_OFILES):    CPPFLAGS := -I$(AGG_DIR)/include
$($D/spitmap_OFILES):    CPPFLAGS += -I$(AGG_DIR)/font_freetype
$($D/spitmap_OFILES):    CPPFLAGS += -I/opt/local/include/freetype2
$($D/spitmap_OFILES): TARGET_ARCH :=
$($D/spitmap_OFILES):    CXXFLAGS := -MD -g -Wall -Werror
$($D/spitmap_OFILES):    CXXFLAGS += -Wno-bitwise-op-parentheses
