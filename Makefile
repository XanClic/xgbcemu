CC = @gcc
LD = @gcc
ECHO = @echo
RM = -@rm -f

CFLAGS = -std=gnu99 -O3 -Wall -Wextra -pedantic -c -funsigned-char -masm=intel

OBJS = $(patsubst %.c,%.o,$(wildcard *.c))

.PHONY: all clean

all: gxemu

gxemu: $(OBJS)
	$(ECHO) "LINK    >$@"
	$(LD) $^ -o $@

%.o: %.c
	$(ECHO) "CC      $<"
	$(CC) $(CFLAGS) $< -o $@

clean:
	$(RM) $(OBJS) gxemu
