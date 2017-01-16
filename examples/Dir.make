       D := examples

EXAMPLES := animate basic clut layers text

include $(EXAMPLES:%=$D/%/Dir.make)
