#include <stdint.h>
#include <stdio.h>

#include "gbc.h"


static int int_wram_offset, vidram_offset;


extern uint8_t __start_save_state_data[], __stop_save_state_data[];


extern void os_replay_savestate_loaded(void);


static void xfer_r(FILE *sfp, void *buffer, size_t size)
{
    fread(buffer, 1, size, sfp);
}

static void xfer_w(FILE *sfp, void *buffer, size_t size)
{
    fwrite(buffer, 1, size, sfp);
}

static void transfer_data(FILE *sfp,
                          void (*xfer)(FILE *sfp, void *buffer, size_t size))
{
    uint8_t *ssdb = __start_save_state_data;
    uint8_t *ssde = __stop_save_state_data;

    xfer(sfp, ssdb, ssde - ssdb);
    xfer(sfp, int_ram, 4096);
    xfer(sfp, full_int_wram, 32768 - 4096);
    xfer(sfp, oam_io, 512);
    xfer(sfp, full_vidram, 16384);
    xfer(sfp, vidmem, 256 * 256 * 4);
    xfer(sfp, &int_wram_offset, sizeof(int_wram_offset));
    xfer(sfp, &vidram_offset, sizeof(vidram_offset));
}


void os_save_savestate(void)
{
    int_wram_offset = int_wram - full_int_wram;
    vidram_offset = vidram - full_vidram;

    FILE *sfp = fopen("/tmp/save-state", "w");
    transfer_data(sfp, xfer_w);
    fclose(sfp);

    printf("Saved state.\n");
}

void os_load_savestate(void)
{
    FILE *sfp = fopen("/tmp/save-state", "r");
    if (!sfp) {
        return;
    }
    transfer_data(sfp, xfer_r);
    fclose(sfp);

    int_wram = full_int_wram + int_wram_offset;
    vidram = full_vidram + vidram_offset;

    os_replay_savestate_loaded();

    printf("Loaded state.\n");
}
