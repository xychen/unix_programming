ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

ifeq "$(PLATFORM)" "linux"
  EXTRALIBS=-lrt
endif
ifeq "$(PLATFORM)" "solaris"
  EXTRALIBS=-lrt
endif

PROGS =	14-27-mcopy
MOREPROGS = rot13c2

# all:	$(PROGS) $(MOREPROGS) lockfile.o
all:	$(PROGS) 

# rot13c2.c:	rot13c2.c.in $(LIBAPUE)
# 	./fixup.awk rot13a.c >xlate
# 	sed '/same/q' rot13c2.c.in >rot13c2.c
# 	cat xlate >>rot13c2.c
# 	sed '1,/same/d' rot13c2.c.in >>rot13c2.c

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o bin/$@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(PROGS) $(MOREPROGS) $(TEMPFILES) *.o xlate rot13c2.c

include $(ROOT)/Make.libapue.inc
