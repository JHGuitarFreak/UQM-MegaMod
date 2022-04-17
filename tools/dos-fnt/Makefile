########################################################################
# This is a GNU makefile - tested on CYGWIN and Linux
#
# You may need to compile or install libpng and/or zlib before
# compiling this.
#

TARGET = undani
# These flags are needed to get it to compile with libpngX.dll on CYGWIN
CYGWINFLAGS = -DPNG_USE_DLL
#CYGWINFLAGS =
PNGFLAGS = -I. 

CC = gcc
CFLAGS = -W -Wall -O0 # -g
LIBS = -L/usr/local/lib -lpng -lm
LDFLAGS=

BINS = $(TARGET) $(TARGET).exe
SRCS = unfont.c
OBJS = unfont.o

.SUFFIXES: .c .o

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $*.c $(PNGFLAGS) $(CYGWINFLAGS)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS) $(LDFLAGS)

clean:
	rm -f $(BINS) *.o
