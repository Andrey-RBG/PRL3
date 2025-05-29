SUBDIRS = libmysyslog myRPC-client myRPC-server

.PHONY: all clean deb

all:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done

clean:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

deb:
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir deb; \
	done
