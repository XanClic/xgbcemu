#include <stddef.h>
#include <string.h>

#include "gbc.h"

#ifdef ENABLE_LINK

int bits_to_be_transferred = 0;
extern int link_countdown;
static volatile enum
{
    INACTIVE,
    EXTERNAL_CLOCK,
    EXTERNAL_DATA,
    CLOCK,
    CLOCK_NAK,
    CLOCK_ACK
} status = INACTIVE;
static int last_arrived = 1;

void link_connect(const char *dest)
{
    current_connection = tcp_connect(dest, LINK_PORT);
    if (current_connection == INVALID_CONN_VALUE)
    {
        os_perr("Could not connect");
        return;
    }

    waiting_for_conn_ack = 1;
    while (waiting_for_conn_ack == 1)
        tcp_conn_poll(current_connection);

    if (waiting_for_conn_ack < 0)
    {
        os_eprint("Connection refused (by xgbcemu).\n");
        return;
    }

    status = INACTIVE;
    connection_master = 0;

    os_print("Connected.\n");
}

void link_unplug(void)
{
    tcp_send(current_connection, "CLOSE", 6);
    close_tcp_conn(current_connection);
    current_connection = INVALID_CONN_VALUE;
    os_print("Cable unplugged.\n");
    last_arrived = 1;
}

void link_clock(void)
{
    if ((current_connection == INVALID_CONN_VALUE) || !bits_to_be_transferred)
        return;

    tcp_send(current_connection, "CLOCK", 6);
    status = CLOCK;
    while (status == CLOCK)
        tcp_conn_poll(current_connection);

    if (status == CLOCK_NAK)
    {
        status = INACTIVE;
        io_regs->sb >>= 1;
        io_regs->sb |= last_arrived << 7;
        if (--bits_to_be_transferred)
            link_countdown = 128;
        else
        {
            io_regs->sc &= 0x01;
            io_regs->int_flag |= INT_SERIAL;
        }
        return;
    }
    else if (status == EXTERNAL_CLOCK)
        return;

    char sendbuf[6] = "DATA\0\0";
    sendbuf[4] = (io_regs->sb & 0x01) + '0';

    tcp_send(current_connection, sendbuf, 6);
    while (status == CLOCK_ACK)
        tcp_conn_poll(current_connection);
}

void link_start_ext_transfer(void)
{
    if (!bits_to_be_transferred)
        return;

    status = EXTERNAL_CLOCK;
}

void new_client(tcp_connection_t conn)
{
    if (current_connection != INVALID_CONN_VALUE)
    {
        tcp_send(conn, "REFUSED", 8);
        os_eprint("There's just one link cable slot, which is used right now, but someone tried to\n");
        os_eprint("plug in another cable (connection refused)!\n");
        return;
    }
    current_connection = conn;
    tcp_send(conn, "ACCEPTED", 9);
    connection_master = 1;
    os_print("Link cable plugged in.\n");
}

void link_data_arrived(tcp_connection_t conn, void *data, size_t size)
{
    (void)size;

    if (conn != current_connection)
    {
        os_eprint("Unknown data arrived...\n");
        return;
    }

    if (waiting_for_conn_ack)
    {
        if (!strcmp(data, "ACCEPTED"))
            waiting_for_conn_ack = 0;
        else
            waiting_for_conn_ack = -1;
        return;
    }

    if (!strcmp(data, "CLOSE"))
    {
        os_eprint("Link cable unplugged.\n");
        close_tcp_conn(current_connection);
        current_connection = INVALID_CONN_VALUE;
        return;
    }

    switch (status)
    {
        case CLOCK:
            if (!strcmp(data, "NAK"))
                status = CLOCK_NAK;
            else if (!strcmp(data, "ACK"))
                status = CLOCK_ACK;
            else if (!strcmp(data, "CLOCK"))
            {
                if (!connection_master)
                {
                    status = EXTERNAL_DATA;
                    tcp_send(current_connection, "ACK", 4);
                }
            }
            break;
        case CLOCK_ACK:
            if (!strcmp(data, "CLOCK"))
            {
                status = INACTIVE;
                bits_to_be_transferred = 0;
                tcp_send(current_connection, "NAK", 4);
                io_regs->sc &= 0x01;
                io_regs->sb = 0xFF;
                io_regs->int_flag |= INT_SERIAL;
            }
            else if (!strncmp(data, "DATA", 4))
            {
                io_regs->sb >>= 1;
                last_arrived = (((char *)data)[4] - '0');
                io_regs->sb |= last_arrived << 7;
                status = INACTIVE;
                if (--bits_to_be_transferred)
                    link_countdown = 128;
                else
                {
                    io_regs->sc &= 0x01;
                    io_regs->int_flag |= INT_SERIAL;
                }
            }
            break;
        case EXTERNAL_CLOCK:
            if (!strcmp(data, "CLOCK"))
            {
                tcp_send(current_connection, "ACK", 4);
                status = EXTERNAL_DATA;
            }
            break;
        case EXTERNAL_DATA:
            if (!strncmp(data, "DATA", 4))
            {
                char sendbuf[6] = "DATA\0\0";
                sendbuf[4] = (io_regs->sb & 0x01) + '0';
                tcp_send(current_connection, sendbuf, 6);
                io_regs->sb >>= 1;
                last_arrived = (((char *)data)[4] - '0');
                io_regs->sb |= last_arrived << 7;
                if (--bits_to_be_transferred)
                    status = EXTERNAL_CLOCK;
                else
                {
                    status = INACTIVE;
                    io_regs->sc &= 0x01;
                    io_regs->int_flag |= INT_SERIAL;
                }
            }
            break;
        case CLOCK_NAK:
            status = INACTIVE;
            break;
        case INACTIVE:
            if (!strcmp(data, "CLOCK"))
                tcp_send(current_connection, "NAK", 4);
    }
}

#endif
