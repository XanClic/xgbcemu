#include "gbc.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        os_eprint("Usage: xgbcemu <ROM> <Save>\n");
        os_eprint(" -- ROM: The ROM file to be used.\n");
        os_eprint(" -- Save: This file contains the external RAM, i.e., saved data.\n");
        os_eprint("          If the file doesn't exist, it will be created.\n");
        exit_err();
    }

    load_rom(argv[1], argv[2]);

    exit_ok();
}
