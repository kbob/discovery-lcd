         D := src

    LIBGFX := $D/liblcd.a

    CFILES := clock.c dma2d.c gpio.c lcd.c lcd-pwm.c pixmap.c sdram.c   \
              systick.c

   $D_LIBS := $(LIBGFX)
 $D_CFILES := $(CFILES:%=$D/%)
 $D_OFILES := $($D_CFILES:%.c=%.o)

    DFILES += $($D_CFILES:%.c=%.d)
      DIRT += $($D_LIBS) $($D_OFILES)


lib:	$($D_LIBS)

$($D_LIBS): $(src_OFILES)
	rm -f $@
	$(AR) cr $@ $(src_OFILES)
