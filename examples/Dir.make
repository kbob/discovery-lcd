       D := examples

EXAMPLES := animate basic clut layers munch text
EXAMPLES += d2d-solid d2d-copy d2d-pfc d2d-blend d2d-clut
EXAMPLES += agg-aa-test agg-alpha-gradient

AGGEX_DIR := $(AGG_DIR)/examples

include $(EXAMPLES:%=$D/%/Dir.make)
