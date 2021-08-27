ROOT=..
PLATFORM=$(shell $(ROOT)/systype.sh)
include $(ROOT)/Make.defines.$(PLATFORM)

#PROGS =	add2 add2stdio devzero myuclc pipe1 pipe2 pipe4 popen1 popen2 tshm
PROGS =	15-5-pipe 15-6-pipe2 15-11-popen 15-14-uclc 15-15-popen2

# all:	$(PROGS) popen.o slock.o tellwait.o
all:	$(PROGS) 

%:	%.c $(LIBAPUE)
	$(CC) $(CFLAGS) $@.c -o bin/$@ $(LDFLAGS) $(LDLIBS)

# slock.o:	slock.c slock.h

clean:
	rm -f $(PROGS) $(TEMPFILES) *.o

include $(ROOT)/Make.libapue.inc
