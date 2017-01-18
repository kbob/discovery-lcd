       D := examples

EXAMPLES := animate basic clut layers text
EXAMPLES += agg-aa-test agg-alpha-gradient

AGGEX_DIR := $(AGG_DIR)/examples

include $(EXAMPLES:%=$D/%/Dir.make)
