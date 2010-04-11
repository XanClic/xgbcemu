// Advanced I/O support for Linux (using SDL)

#ifndef OS_ADVIO_H
#define OS_ADVIO_H

#include <stddef.h>

typedef int tcp_connection_t;
typedef int tcp_server_t;

#define INVALID_CONN_VALUE (-1)

void os_draw_line(int offx, int offy, int line);
void os_handle_events(void);
void os_open_screen(int width, int height);

void set_tcp_callbacks(void (*rcb)(tcp_connection_t, void *, size_t), void (*ccb)(tcp_connection_t));
tcp_server_t create_tcp_server(int port);
tcp_connection_t tcp_connect(const char *dest, int port);
void close_tcp_conn(tcp_connection_t connection);
void tcp_conn_poll(tcp_connection_t connection);
void tcp_server_poll(tcp_server_t server);
void tcp_send(tcp_connection_t connection, const void *data, size_t size);

#endif
