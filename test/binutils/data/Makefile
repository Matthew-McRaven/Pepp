# Regenerates every set of sample data by running make in each subdirectory, passing along variables such as PREFIX.
SUBDIRS := $(patsubst %/Makefile,%,$(wildcard */Makefile))

all clean: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: all clean $(SUBDIRS)
