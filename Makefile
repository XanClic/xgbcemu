include make.config

ECHO = @echo
RM = -@rm -f

OBJS = $(patsubst %.c,%.o,$(wildcard *.c) $(wildcard cartridges/*.c)) $(OSOBJS)

.PHONY: all clean

all: xgbcemu

xgbcemu: $(OBJS)
	$(ECHO) "LINK    >$@"
	$(LINK) $(LDFLAGS) $^ -o $@

%.o: %.c
	$(ECHO) "CC      $<"
	$(CC) $(CFLAGS) $< -o $@

make.config: configure
	$(ECHO) "Executing ./configure..."
	@./configure

clean:
	$(RM) $(OBJS) xgbcemu
