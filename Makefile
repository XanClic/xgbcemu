include make.config

ECHO = @echo
RM = -@rm -f
ASM = @fasm > /dev/null

ifeq ($(EXT_ASM),yes)
	OBJS = $(patsubst %.c,%.o,$(wildcard *.c) $(wildcard cartridges/*.c)) $(patsubst %.asm,%.o,$(wildcard *.asm)) $(OSOBJS)
else
	OBJS = $(patsubst %.c,%.o,$(wildcard *.c) $(wildcard cartridges/*.c)) $(OSOBJS)
endif

.PHONY: all clean

all: xgbcemu

xgbcemu: $(OBJS)
	$(ECHO) "LINK    >$@"
	$(LINK) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(ECHO) "CC      $<"
	$(CC) $(CFLAGS) $< -o $@

%.o: %.asm
	$(ECHO) "ASM     $<"
	$(ASM) $< $@

make.config: configure
	$(ECHO) "Executing ./configure..."
	@./configure

clean:
	$(RM) $(OBJS) xgbcemu
