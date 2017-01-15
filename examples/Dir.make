       D := examples

EXAMPLES := animate basic clut layers

include $(EXAMPLES:%=$D/%/Dir.make)
