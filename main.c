#include "gbc.h"

#include <stdnoreturn.h>
#include <string.h>

noreturn void do_help(void)
{
    os_eprint("Usage: xgbcemu <ROM> [Save] [--zoom|-z zoom]\n");
    os_eprint(" -- ROM: The ROM file to be used.\n");
    os_eprint(" -- Save: This file contains the external RAM, i.e., saved data.\n");
    os_eprint("          If the file doesn't exist, it will be created.\n");
    os_eprint(" -- zoom: Integer values from 1 to infinity are allowed, as long as\n");
    os_eprint("          your hardware is able to handle it.\n");
    exit_err();
}

void unrec_op_s(const char *s)
{
    os_eprint("Unrecognised option \"%s\"\n", s);
    exit_err();
}

void unrec_op_c(char c)
{
    os_eprint("Unrecognised option '%c'\n", c);
    exit_err();
}

void double_declared(const char *s)
{
    os_eprint("Don't know how to set %s (more than one time given)\n", s);
    exit_err();
}

int main(int argc, char *argv[])
{
    #ifdef os_init_console
    os_init_console
    #endif

    if (argc < 2)
        do_help();

    char *rom = NULL, *save = NULL, *zoom_stuff = NULL;
    int zoom = 1;
    char **next = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (next != NULL)
        {
            *next = argv[i];
            next = NULL;
            continue;
        }

        if (argv[i][0] != '-')
        {
            if (rom == NULL)
                rom = argv[i];
            else if (save == NULL)
                save = argv[i];
            else
                unrec_op_s(argv[i]);
        }
        else
        {
            if (argv[i][1] != '-')
            {
                for (int option = 1; argv[i][option]; option++)
                {
                    switch (argv[i][option])
                    {
                        case 'z':
                            if (zoom_stuff != NULL)
                                double_declared("zoom");
                            next = &zoom_stuff;
                            break;
                        case 'h':
                            do_help();
                        default:
                            unrec_op_c(argv[i][option]);
                    }
                }
            }
            else
            {
                if (!strcmp(argv[i] + 2, "zoom"))
                {
                    if (zoom_stuff != NULL)
                        double_declared("zoom");
                    next = &zoom_stuff;
                }
                else if (!strcmp(argv[i] + 2, "help"))
                    do_help();
                else
                    unrec_op_s(argv[i]);
            }
        }
    }

    if (next != NULL)
    {
        printf("%s needs (at least) one further argument.\n", argv[argc - 1]);
        exit_err();
    }

    if (zoom_stuff != NULL)
    {
        zoom = atoi(zoom_stuff);
        if (zoom < 1)
        {
            printf("Invalid zoom value given.\n");
            exit_err();
        }
    }

    if (save == NULL)
    {
        save = malloc(strlen(rom) + 5);
        strcpy(save, rom);
        char *l = strrchr(save, '.');
        if (l == NULL)
            strcat(save, ".sav");
        else
        {
            *(++l) = 's';
            *(++l) = 'a';
            *(++l) = 'v';
            *(++l) = 0;
        }
    }

    load_rom(rom, save, zoom);

    exit_ok();
}
