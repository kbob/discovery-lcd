             D := examples/agg-alpha-gradient

          PROG := alpha-gradient
        CFILES := 
       CCFILES := main.cc
      ACCFILES := alpha_gradient.cpp
     $D_LDLIBS := -llcd $(LDLIB_AGG) -lm $(LDLIB_OPENCM3)

     $D_CFILES := $(CFILES:%=$D/%)
    $D_CCFILES := $(CCFILES:%=$D/%)
   $D_ACCFILES := $(ACCFILES:%=$(AGGEX_DIR)/%)
     $D_OFILES := $($D_CFILES:%.c=%.o)                                  \
                  $($D_CCFILES:%.cc=%.o)                                \
                  $(ACCFILES:%.cpp=$D/%.o)
        $D_ELF := $D/$(PROG).elf

        DFILES += $($D_CFILES:%.c=%.d)                                  \
                  $($D_CCFILES:%.cc=%.d)                                \
                  $(ACCFILES:%.cpp=$D/%.d)
          DIRT += $($D_ELF) $($D_OFILES)
 EXAMPLE_ELVES += $($D_ELF)

$D/%.o: $(AGGEX_DIR)/%.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

$($D_ELF): LDLIBS := $($D_LDLIBS)
$($D_ELF): $($D_OFILES) $(LIBGFX)
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) $(POST_LDFLAGS) -o $@
