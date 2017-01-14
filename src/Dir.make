         D := src

    LIBGFX := $D/liblcd.a
#     CFILES := button.c gfx.c lcd.c gpio.c i2c.c pixtile.c systick.c touch.c
    CFILES := clock.c gpio.c lcd.c lcd-pwm.c systick.c

   $D_LIBS := $(LIBGFX)
 $D_CFILES := $(CFILES:%=$D/%)
 $D_OFILES := $($D_CFILES:%.c=%.o)

    DFILES += $($D_CFILES:%.c=%.d)
      DIRT += $($D_LIBS) $($D_OFILES)


lib:	$($D_LIBS)

$($D_LIBS): $(src_OFILES)
	rm -f $@
	$(AR) cr $@ $(src_OFILES)
