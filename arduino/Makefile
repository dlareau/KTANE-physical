TOPTARGETS := all clean

SUBDIRS := $(filter-out Libraries/.,$(wildcard */.))

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)