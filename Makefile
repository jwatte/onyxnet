
-include make/Reset.mk

all:	targets

clean:	
	rm -rf obj

-include $(wildcard lib/*/Makefile) $(wildcard test/*/Makefile) $(wildcard app/*/Makefile)

targets:	$(TARGETS)
