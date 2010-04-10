#include <fcntl.h>
#include <netdb.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

static void (*client_callback)(int client);
static void (*recv_callback)(int connection, void *buffer, size_t size);

int create_tcp_server(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, getprotobyname("tcp")->p_proto);
    struct sockaddr_in addr =
    {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { 0 }
    };
    if (fd < 0)
        return -1;
    if (bind(fd, (void *)&addr, sizeof(addr)) < 0)
        return -1;
    if (listen(fd, 1) < 0)
        return -1;
    fcntl(fd, F_SETFL, O_NONBLOCK);
    int flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
    return fd;
}

int tcp_connect(const char *destname, int destport)
{
    int fd = socket(AF_INET, SOCK_STREAM, getprotobyname("tcp")->p_proto);
    struct sockaddr_in addr =
    {
        .sin_family = AF_INET,
        .sin_port = htons(destport),
        .sin_addr = { 0 }
    };
    struct hostent *host = gethostbyname(destname);

    if (host == NULL)
        return -1;

    memcpy(&addr.sin_addr, host->h_addr_list[0], sizeof(addr.sin_addr));

    if (fd < 0)
        return -1;

    if (connect(fd, (void *)&addr, sizeof(addr)) < 0)
        return -1;

    fcntl(fd, F_SETFL, O_NONBLOCK);
    int flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));

    return fd;
}

void close_tcp_conn(int fd)
{
    shutdown(fd, SHUT_RDWR);
    close(fd);
}

void set_tcp_callbacks(void (*rcb)(int, void *, size_t), void (*ccb)(int))
{
    recv_callback = rcb;
    client_callback = ccb;
}

void tcp_send(int fd, const void *data, size_t size)
{
    write(fd, data, size);
}

void tcp_server_poll(int fd)
{
    int new_conn = accept(fd, NULL, NULL);
    if (new_conn >= 0)
    {
        fcntl(new_conn, F_SETFL, O_NONBLOCK);
        int flag = 1;
        setsockopt(new_conn, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
        client_callback(new_conn);
    }
}

void tcp_conn_poll(int fd)
{
    char buffer[1024];
    ssize_t len = read(fd, buffer, 1024);
    if (len > 0)
        recv_callback(fd, buffer, len);
}
