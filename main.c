#include "gbc.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        os_eprint("Usage: gxemu <ROM> <Save>\n");
        exit_err();
    }

    load_rom(argv[1], argv[2]);

    exit_ok();
}
