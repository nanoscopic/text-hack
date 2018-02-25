CSRCS = text-hack.c

CC = gcc

NO_UNUSED = -ffunction-sections -fdata-sections -Wl,--gc-sections

CFLAGS = -g -Wall -pedantic -O3 $(NO_UNUSED) -Wno-char-subscripts

PROGRAM = text-hack

.PHONY:	clean

all: $(PROGRAM)

$(PROGRAM): 	$(CSRCS)
	$(CC) -o $(PROGRAM) $(CSRCS) $(CFLAGS)
	strip $(PROGRAM)

clean:			
	rm -f $(PROGRAM)




