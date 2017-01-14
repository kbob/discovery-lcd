       D := examples

EXAMPLES := basic layers

include $(EXAMPLES:%=$D/%/Dir.make)
