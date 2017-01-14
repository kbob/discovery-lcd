       D := examples

EXAMPLES := basic

include $(EXAMPLES:%=$D/%/Dir.make)
