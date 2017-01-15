       D := examples

EXAMPLES := animate basic layers

include $(EXAMPLES:%=$D/%/Dir.make)
