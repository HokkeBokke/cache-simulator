# Authors: Øyvind Nohr

CC = gcc
CFLAGS = -Wall -Werror -Wno-unused -g -O2
OBJDIR = obj
PROGRAM = cachesim

OBJS = cpu.o memory.o cache.o
HEADERS = memory.h cache.h

all: $(PROGRAM) $(HEADERS) Makefile
.PHONY: clean 

dirs:
	@mkdir -p $(OBJDIR)

$(PROGRAM): $(patsubst %, $(OBJDIR)/%, $(OBJS))
	$(CC) $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o: %.c dirs
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o *~ $(PROGRAM) $(OBJDIR)
